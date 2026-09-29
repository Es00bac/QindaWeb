// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_instance.h"
#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>
class InstanceTest : public QObject {
    Q_OBJECT
private slots:
    void aSecondProcessForwardsTheLaunch() {
        QTemporaryDir profile;
        QVERIFY(profile.isValid());
        QindaWeb::ApplicationInstance primary(profile.path());
        QString error;
        QCOMPARE(primary.start({QStringLiteral("about:blank")}, false, &error), QindaWeb::ApplicationInstance::Result::Primary);
        QSignalSpy requested(&primary, &QindaWeb::ApplicationInstance::openRequested);
        primary.setReady();
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--forward"), profile.path()});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 6000);
        QCOMPARE(child.exitCode(), 0);
        QCOMPARE(requested.count(), 1);
        QCOMPARE(requested.at(0).at(0).toStringList(), QStringList{QStringLiteral("https://example.org/?q=ブラウザー")});
        QCOMPARE(requested.at(0).at(1).toBool(), true);
        QVERIFY(!primary.OpenUrls(QStringList(33, QStringLiteral("about:blank")), false));
        QCOMPARE(requested.count(), 1);
    }
};
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == QStringLiteral("--forward")) {
        QindaWeb::ApplicationInstance secondary(app.arguments().value(2));
        QString error;
        return secondary.start({QStringLiteral("https://example.org/?q=ブラウザー")}, true, &error)
            == QindaWeb::ApplicationInstance::Result::Forwarded ? 0 : 1;
    }
    InstanceTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_instance.moc"
