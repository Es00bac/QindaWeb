// SPDX-License-Identifier: GPL-3.0-or-later
#include "firefox_engine.h"
#include "browser_view.h"
#include "gecko_surface_host.h"
#include "profile.h"
#include <QJsonArray>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QWaylandXdgSurface>
#include <utility>
namespace QindaWeb {
FirefoxEngine::FirefoxEngine(GeckoSurfaceHost *host, QObject *parent)
    : QObject(parent), m_host(host)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &FirefoxEngine::processOutput);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) fail(m_process.errorString());
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus) {
            if (!m_stopping) fail(QStringLiteral("Gecko exited (code %1).").arg(code));
        });
    connect(&m_bidi, &BidiClient::connected, this, &FirefoxEngine::establishSession);
    connect(&m_bidi, &BidiClient::event, this, &FirefoxEngine::engineEvent);
    connect(&m_bidi, &BidiClient::disconnected, this, [this](const QString &reason) {
        if (!m_stopping) fail(reason);
    });
    connect(host, &GeckoSurfaceHost::toplevelCreated, this, [this](QWaylandXdgSurface *surface) {
        m_surfaces.enqueue(surface);
        pairCreation();
    });
    m_startupTimer.setSingleShot(true);
    m_startupTimer.setInterval(30000);
    connect(&m_startupTimer, &QTimer::timeout, this, [this] {
        fail(QStringLiteral("Gecko did not finish starting within 30 seconds."));
    });
}
FirefoxEngine::~FirefoxEngine()
{
    m_stopping = true;
    m_bidi.close();
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(3000)) { m_process.kill(); m_process.waitForFinished(1000); }
    }
}
bool FirefoxEngine::start(const QString &program, const QString &profile, QString *error)
{
    if (program.isEmpty()) {
        if (error) *error = QStringLiteral("Install Firefox or QindaFox through Portage first.");
        return false;
    }
    if (!prepareProfile(profile, error)) return false;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("MOZ_ENABLE_WAYLAND"), QStringLiteral("1"));
    environment.insert(QStringLiteral("GDK_BACKEND"), QStringLiteral("wayland"));
    environment.insert(QStringLiteral("WAYLAND_DISPLAY"), m_host->displayName());
    environment.remove(QStringLiteral("WAYLAND_SOCKET"));
    // Never inherit a debugging session's sandbox override.
    environment.remove(QStringLiteral("MOZ_DISABLE_CONTENT_SANDBOX"));
    environment.remove(QStringLiteral("MOZ_DISABLE_RDD_SANDBOX"));
    environment.remove(QStringLiteral("MOZ_DISABLE_GMP_SANDBOX"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(program);
    m_process.setArguments({QStringLiteral("--no-remote"), QStringLiteral("--profile"), profile,
                           QStringLiteral("--remote-debugging-port=0"), QStringLiteral("about:blank")});
    m_process.start();
    m_startupTimer.start();
    return true;
}
void FirefoxEngine::processOutput()
{
    m_output += m_process.readAllStandardOutput();
    // Startup diagnostics may contain URLs. Do not print raw engine output.
    if (!m_connected) {
        static const QRegularExpression endpoint(QStringLiteral("WebDriver BiDi listening on (ws://127\\.0\\.0\\.1:[0-9]+)"));
        const auto match = endpoint.match(QString::fromUtf8(m_output));
        if (match.hasMatch()) {
            m_connected = true;
            m_bidi.open(QUrl(match.captured(1) + QStringLiteral("/session")));
        }
    }
    if (m_output.size() > 16384) m_output = m_output.right(8192);
}
void FirefoxEngine::establishSession()
{
    m_bidi.request(QStringLiteral("session.new"),
        {{QStringLiteral("capabilities"), QJsonObject{{QStringLiteral("alwaysMatch"),
            QJsonObject{{QStringLiteral("acceptInsecureCerts"), false}}}}}},
        [this](const QJsonObject &result, const QString &error) {
            if (!error.isEmpty()) { fail(error); return; }
            const auto capabilities = result.value(QStringLiteral("capabilities")).toObject();
            const QString name = capabilities.value(QStringLiteral("browserName")).toString();
            const QString version = capabilities.value(QStringLiteral("browserVersion")).toString();
            if (name != QStringLiteral("firefox")) {
                fail(QStringLiteral("The runtime did not identify itself as Firefox/Gecko."));
                return;
            }
            m_identity = QStringLiteral("Gecko / Firefox %1").arg(version);
            emit identityChanged();
            m_bidi.request(QStringLiteral("session.subscribe"),
                {{QStringLiteral("events"), QJsonArray{QStringLiteral("browsingContext.contextCreated"),
                    QStringLiteral("browsingContext.contextDestroyed"), QStringLiteral("browsingContext.navigationStarted"),
                    QStringLiteral("browsingContext.domContentLoaded"), QStringLiteral("browsingContext.load"),
                    QStringLiteral("browsingContext.fragmentNavigated")}}},
                [this](const QJsonObject &, const QString &subscribeError) {
                    if (!subscribeError.isEmpty()) { fail(subscribeError); return; }
                    m_bidi.request(QStringLiteral("browsingContext.getTree"), {},
                        [this](const QJsonObject &tree, const QString &treeError) {
                            if (!treeError.isEmpty()) { fail(treeError); return; }
                            const auto contexts = tree.value(QStringLiteral("contexts")).toArray();
                            if (contexts.size() != 1) {
                                fail(QStringLiteral("Gecko must start with one empty browsing window."));
                                return;
                            }
                            m_initialContext = contexts.first().toObject().value(QStringLiteral("context")).toString();
                            m_ready = true;
                            m_startupTimer.stop();
                            emit ready();
                            nextCreation();
                        });
                });
        });
}
void FirefoxEngine::addView(BrowserView *view, const QUrl &url)
{
    m_creation.enqueue({view, url, {}, false});
    nextCreation();
}
void FirefoxEngine::nextCreation()
{
    if (!m_ready || m_creation.isEmpty() || m_creation.head().submitted) return;
    if (!m_creation.head().view) { m_creation.dequeue(); nextCreation(); return; }
    auto &creation = m_creation.head();
    creation.submitted = true;
    if (!m_initialContext.isEmpty()) {
        creation.context = std::exchange(m_initialContext, {});
        pairCreation();
        return;
    }
    m_bidi.request(QStringLiteral("browsingContext.create"),
        {{QStringLiteral("type"), QStringLiteral("window")}},
        [this](const QJsonObject &result, const QString &error) {
            if (m_creation.isEmpty()) return;
            if (!error.isEmpty()) {
                if (m_creation.head().view) m_creation.head().view->setMessage(error);
                m_creation.dequeue();
                nextCreation();
                return;
            }
            m_creation.head().context = result.value(QStringLiteral("context")).toString();
            pairCreation();
        });
}
void FirefoxEngine::pairCreation()
{
    while (!m_surfaces.isEmpty() && !m_surfaces.head()) m_surfaces.dequeue();
    if (m_creation.isEmpty() || m_creation.head().context.isEmpty() || m_surfaces.isEmpty()) return;
    const auto creation = m_creation.dequeue();
    auto *surface = m_surfaces.dequeue().data();
    if (creation.view) {
        creation.view->setContextId(creation.context);
        creation.view->setSurface(surface);
        creation.view->setMessage({});
        m_views.insert(creation.context, creation.view);
        navigate(creation.view, creation.url);
    } else {
        surface->toplevel()->sendClose();
        m_bidi.request(QStringLiteral("browsingContext.close"), {{QStringLiteral("context"), creation.context}});
    }
    nextCreation();
}
void FirefoxEngine::navigate(BrowserView *view, const QUrl &url)
{
    if (!view->ready()) {
        for (auto &creation : m_creation) if (creation.view == view) creation.url = url;
        view->setUrl(url);
        return;
    }
    view->setLoading(true);
    view->setMessage({});
    viewCommand(view, QStringLiteral("browsingContext.navigate"),
                {{QStringLiteral("url"), url.toString(QUrl::FullyEncoded)},
                 {QStringLiteral("wait"), QStringLiteral("none")}});
}
void FirefoxEngine::viewCommand(BrowserView *view, const QString &method, QJsonObject params)
{
    if (!view || !view->ready()) return;
    params.insert(QStringLiteral("context"), view->contextId());
    m_bidi.request(method, params, [guard = QPointer<BrowserView>(view)](const QJsonObject &, const QString &error) {
        if (guard && !error.isEmpty()) { guard->setLoading(false); guard->setMessage(error); }
    });
}
void FirefoxEngine::reload(BrowserView *view)
{
    viewCommand(view, QStringLiteral("browsingContext.reload"), {{QStringLiteral("wait"), QStringLiteral("none")}});
}
void FirefoxEngine::traverse(BrowserView *view, int delta)
{
    viewCommand(view, QStringLiteral("browsingContext.traverseHistory"), {{QStringLiteral("delta"), delta}});
}
void FirefoxEngine::closeView(BrowserView *view)
{
    if (!view->contextId().isEmpty()) {
        m_views.remove(view->contextId());
        m_bidi.request(QStringLiteral("browsingContext.close"), {{QStringLiteral("context"), view->contextId()}});
    }
}
void FirefoxEngine::engineEvent(const QString &method, const QJsonObject &params)
{
    const QString context = params.value(QStringLiteral("context")).toString();
    auto *view = m_views.value(context).data();
    if (!view) {
        // App-requested creation is paired by its response. Unrequested top
        // levels are recorded separately; subframes never become app windows.
        if (method == QStringLiteral("browsingContext.contextCreated") && m_ready &&
            params.value(QStringLiteral("parent")).isNull() && m_creation.isEmpty())
            emit unsolicitedContext(context, QUrl(params.value(QStringLiteral("url")).toString()));
        return;
    }
    if (method == QStringLiteral("browsingContext.contextDestroyed")) {
        view->setContextId({});
        view->setMessage(QStringLiteral("This Gecko page was closed."));
        return;
    }
    if (params.contains(QStringLiteral("url"))) view->setUrl(QUrl(params.value(QStringLiteral("url")).toString()));
    if (method == QStringLiteral("browsingContext.navigationStarted")) view->setLoading(true);
    if (method == QStringLiteral("browsingContext.load") || method == QStringLiteral("browsingContext.domContentLoaded")) {
        view->setLoading(false);
        view->setMessage({});
    }
}
void FirefoxEngine::fail(const QString &message)
{
    m_startupTimer.stop();
    m_ready = false;
    for (auto view : m_views) if (view) { view->setLoading(false); view->setMessage(message); }
    for (auto creation : m_creation) if (creation.view) { creation.view->setLoading(false); creation.view->setMessage(message); }
    emit failed(message);
}
void FirefoxEngine::stop()
{
    m_stopping = true;
    m_bidi.request(QStringLiteral("browser.close"), {});
}
}
