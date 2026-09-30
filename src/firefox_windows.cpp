// SPDX-License-Identifier: GPL-3.0-or-later
#include "firefox_engine.h"
#include "browser_view.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QWaylandXdgSurface>
#include <QUuid>
#include <utility>
namespace QindaWeb {
namespace {
QString jsString(const QString &text)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact)) + QStringLiteral("[0]");
}
}
void FirefoxEngine::addView(BrowserView *view, const QUrl &url)
{
    m_creation.enqueue({view, url, {}, {}, {}, false, false, true});
    nextCreation();
}
void FirefoxEngine::adoptReadyView(BrowserView *view, const QString &context, const QUrl &url, QWaylandXdgSurface *surface)
{
    m_views.insert(context, view);
    view->setContextId(context);
    view->setSurface(surface);
    view->setUrl(url);
    view->setLoading(false);
    view->setMessage({});
}
void FirefoxEngine::offerCandidates()
{
    // A contextCreated notification precedes the response to our create call.
    // Fence it until that response identifies the app-requested context.
    if (!m_ready || (!m_creation.isEmpty() && m_creation.head().submitted && m_creation.head().context.isEmpty())) return;
    const auto candidates = std::exchange(m_candidates, {});
    for (auto it = candidates.cbegin(); it != candidates.cend(); ++it) {
        bool requested = false;
        for (const auto &creation : m_creation) if (creation.context == it.key()) requested = true;
        if (!requested && !m_views.contains(it.key()) && !m_bootstrapContexts.contains(it.key()))
            m_creation.enqueue({nullptr, it.value(), it.key(), {}, {}, false, false, false, true});
    }
    nextCreation();
}
void FirefoxEngine::nextCreation()
{
    if (!m_ready || m_creation.isEmpty() || m_creation.head().submitted) return;
    if (!m_creation.head().view && !m_creation.head().unsolicited) { m_creation.dequeue(); nextCreation(); return; }
    auto &creation = m_creation.head();
    creation.submitted = true;
    if (!creation.context.isEmpty()) { markCreation(); return; }
    m_bidi.request(QStringLiteral("browsingContext.create"),
        {{QStringLiteral("type"), QStringLiteral("window")}},
        [this](const QJsonObject &result, const QString &error) {
            if (m_creation.isEmpty()) return;
            if (!error.isEmpty()) { rejectCreation(error); return; }
            m_creation.head().context = result.value(QStringLiteral("context")).toString();
            m_candidates.remove(m_creation.head().context);
            markCreation();
            offerCandidates();
        });
}
void FirefoxEngine::markCreation()
{
    auto &creation = m_creation.head();
    // DOMContentLoaded and load can both arrive before evaluation completes.
    // Serialize remarking so an obsolete callback cannot discard the original
    // title, or set another nonce after the context/surface has been adopted.
    if (creation.marking) { creation.remarkPending = true; return; }
    creation.marking = true;
    const QString context = creation.context;
    const QString previousMarker = creation.marker;
    creation.marker = QStringLiteral("QindaWeb surface ") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    creation.marked = false;
    const QString marker = creation.marker;
    // Pair by an unpredictable marker on this exact owned browsing context,
    // not arrival order: a web popup may race an app-requested window.
    const QString expression = QStringLiteral("(() => { const old = document.title; document.title = %1; return old; })()")
                                   .arg(jsString(creation.marker));
    m_bidi.request(QStringLiteral("script.evaluate"),
        {{QStringLiteral("expression"), expression}, {QStringLiteral("awaitPromise"), false},
         {QStringLiteral("target"), QJsonObject{{QStringLiteral("context"), context}}}},
        [this, context, marker, previousMarker](const QJsonObject &result, const QString &error) {
            if (m_creation.isEmpty() || m_creation.head().context != context || m_creation.head().marker != marker) return;
            if (!error.isEmpty() || result.value(QStringLiteral("type")).toString() != QStringLiteral("success")) {
                rejectCreation(QStringLiteral("Gecko could not identify its page window."));
                return;
            }
            const QString title = result.value(QStringLiteral("result")).toObject().value(QStringLiteral("value")).toString();
            if (title != previousMarker) m_creation.head().originalTitle = title;
            m_creation.head().marking = false;
            if (m_creation.head().remarkPending) {
                m_creation.head().remarkPending = false;
                markCreation();
                return;
            }
            m_creation.head().marked = true;
            m_pairTimer.start();
            pairCreation();
        });
}
void FirefoxEngine::rejectCreation(const QString &error)
{
    m_pairTimer.stop();
    if (m_creation.isEmpty()) return;
    const auto rejected = m_creation.dequeue();
    if (rejected.view) { rejected.view->setLoading(false); rejected.view->setMessage(error); }
    else if (rejected.unsolicited) qWarning().noquote() << "QindaWeb popup creation:" << error;
    if (!rejected.context.isEmpty())
        m_bidi.request(QStringLiteral("browsingContext.close"), {{QStringLiteral("context"), rejected.context}});
    nextCreation();
    offerCandidates();
}
void FirefoxEngine::pairCreation()
{
    if (m_creation.isEmpty() || !m_creation.head().marked) return;
    const QString marker = m_creation.head().marker;
    QWaylandXdgSurface *surface = nullptr;
    for (qsizetype i = m_surfaces.size() - 1; i >= 0; --i) {
        if (!m_surfaces.at(i)) { m_surfaces.removeAt(i); continue; }
        if (m_surfaces.at(i)->toplevel()->title().startsWith(marker)) {
            surface = m_surfaces.takeAt(i);
            break;
        }
    }
    if (!surface) return;
    m_pairTimer.stop();
    const auto creation = m_creation.dequeue();
    if (creation.view || creation.unsolicited) {
        // Transient bootstrap tabs do not get outer windows. Unsolicited pages
        // first prove their own context/surface pair on the private display.
        if (creation.unsolicited) emit unsolicitedWindow(creation.context, creation.url, surface, creation.originalTitle);
        else {
            m_views.insert(creation.context, creation.view);
            creation.view->setContextId(creation.context);
            creation.view->setSurface(surface);
            creation.view->setTitle(creation.originalTitle);
            creation.view->setUrl(creation.url);
            creation.view->setMessage({});
        }
        const auto bootstrap = std::exchange(m_bootstrapContexts, {});
        for (const auto &context : bootstrap)
            m_bidi.request(QStringLiteral("browsingContext.close"), {{QStringLiteral("context"), context}});
        if (creation.navigate) navigate(creation.view, creation.url);
        else {
            if (creation.view) creation.view->setLoading(false);
            const QString restore = QStringLiteral("if (document.title === %1) document.title = %2;")
                                        .arg(jsString(creation.marker), jsString(creation.originalTitle));
            m_bidi.request(QStringLiteral("script.evaluate"),
                {{QStringLiteral("expression"), restore}, {QStringLiteral("awaitPromise"), false},
                 {QStringLiteral("target"), QJsonObject{{QStringLiteral("context"), creation.context}}}});
        }
    } else {
        surface->toplevel()->sendClose();
        m_bidi.request(QStringLiteral("browsingContext.close"), {{QStringLiteral("context"), creation.context}});
    }
    nextCreation();
    offerCandidates();
}
}
