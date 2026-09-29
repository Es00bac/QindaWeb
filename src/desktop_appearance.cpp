// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop_appearance.h"
#include <qindaqt/services/settings_client/settings_client.h>
#include <qindaqt/services/settings_client/qt_settings_transport.h>
#include <qindaqt/design_tokens/accessibility_inputs.h>
#include <qindaqt/design_tokens/token_facade.h>
#include <qindaqt/themes/theme_catalog.h>
#include <QDBusConnection>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QEventLoop>
#include <QStandardPaths>
#include <QTimer>
#include <QDir>
namespace QindaWeb {
namespace Settings = QindaQt::Services::SettingsClient;
class DesktopAppearance::Impl {
public:
    Impl(DesktopAppearance *owner, QQmlEngine *engine)
        : owner(owner), engine(engine), transport(QDBusConnection::sessionBus()),
          settings(transport, {QStringLiteral("appearance.theme"), QStringLiteral("accessibility.reducedMotion")})
    {
        QObject::connect(&settings, &Settings::SettingsClient::snapshotChanged, owner, [this] { apply(); });
        QObject::connect(&settings, &Settings::SettingsClient::stateChanged, owner, [this] {
            if (settings.state() != Settings::ClientState::Ready) {
                setState(false, QStringLiteral("Desktop appearance unavailable; using the last complete theme."));
            } else apply();
        });
    }
    void setState(bool value, const QString &message)
    {
        if (ready == value && error == message) return;
        ready = value;
        error = message;
        emit owner->changed();
    }
    bool load(QString *failure)
    {
        QQmlComponent registration(engine);
        registration.setData("import QtQuick\nimport QindaQt.Tokens 1.0\nQtObject { property bool ready: Tokens.ready }\n",
                             QUrl(QStringLiteral("inline:qindaweb-tokens.qml")));
        if (registration.status() == QQmlComponent::Loading) {
            QEventLoop loop;
            QTimer deadline;
            deadline.setSingleShot(true);
            QObject::connect(&registration, &QQmlComponent::statusChanged, &loop, [&loop](QQmlComponent::Status status) {
                if (status != QQmlComponent::Loading) loop.quit();
            });
            QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
            deadline.start(5000);
            loop.exec();
        }
        if (!registration.isReady()) { *failure = registration.errorString(); return false; }
        const std::unique_ptr<QObject> object(registration.create());
        if (!object) { *failure = registration.errorString(); return false; }
        facade = engine->singletonInstance<QindaQt::DesignTokens::TokenFacade *>(
            QStringLiteral("QindaQt.Tokens"), QStringLiteral("Tokens"));
        if (!facade) { *failure = QStringLiteral("Desktop token singleton unavailable."); return false; }
        for (const auto &root : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
            directories.append(root + QStringLiteral("/qindaqt/themes"));
        directories.append(QStringLiteral("/usr/share/qindaqt/themes"));
        directories.removeDuplicates();
        return true;
    }
    void apply()
    {
        if (!facade || settings.state() != Settings::ClientState::Ready || !settings.snapshot()) return;
        const auto &snapshot = *settings.snapshot();
        if (snapshot.owner != settings.currentOwner()) return;
        const QString id = snapshot.values.value(QStringLiteral("appearance.theme")).toString();
        bool found = false;
        for (const auto &directory : directories) {
            if (!QDir(directory).exists()) continue;
            QString failure;
            if (!catalog.loadDirectory(directory, &failure)) {
                setState(false, failure);
                return;
            }
            if (catalog.selectById(id)) { found = true; break; }
        }
        if (!found) {
            setState(false, QStringLiteral("The selected desktop theme is not installed; keeping the last complete theme."));
            return;
        }
        QindaQt::DesignTokens::AccessibilityInputs inputs;
        inputs.reducedMotion = snapshot.values.value(QStringLiteral("accessibility.reducedMotion")).toBool();
        QString failure;
        const bool published = facade->publish(catalog.themes().at(catalog.currentIndex()), inputs, &failure);
        setState(published, failure);
    }
    DesktopAppearance *owner;
    QQmlEngine *engine;
    Settings::QtSettingsTransport transport;
    Settings::SettingsClient settings;
    QindaQt::Themes::ThemeCatalog catalog;
    QStringList directories;
    QindaQt::DesignTokens::TokenFacade *facade = nullptr;
    bool ready = false;
    QString error;
};
DesktopAppearance::DesktopAppearance(QQmlEngine *engine, QObject *parent)
    : QObject(parent), m_impl(std::make_unique<Impl>(this, engine)) {}
DesktopAppearance::~DesktopAppearance() = default;
bool DesktopAppearance::start(QString *error)
{
    QString failure;
    if (!m_impl->load(&failure) || !m_impl->settings.start(&failure)) {
        m_impl->setState(false, failure);
        if (error) *error = failure;
        return false;
    }
    return true;
}
bool DesktopAppearance::ready() const { return m_impl->ready; }
QString DesktopAppearance::error() const { return m_impl->error; }
}
