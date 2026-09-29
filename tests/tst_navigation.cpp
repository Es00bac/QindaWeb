// SPDX-License-Identifier: GPL-3.0-or-later
#include "navigation.h"
#include <QtTest>
#include <QUrlQuery>
class NavigationTest : public QObject {
    Q_OBJECT
private slots:
    void acceptedAddresses() {
        QCOMPARE(QindaWeb::navigationUrl(QStringLiteral("example.org")), QUrl(QStringLiteral("https://example.org")));
        QCOMPARE(QindaWeb::navigationUrl(QStringLiteral("http://localhost:8080/a?q=x")), QUrl(QStringLiteral("http://localhost:8080/a?q=x")));
        QCOMPARE(QindaWeb::navigationUrl(QStringLiteral("localhost:8080/a")), QUrl(QStringLiteral("https://localhost:8080/a")));
        QCOMPARE(QindaWeb::navigationUrl(QStringLiteral("about:blank")), QUrl(QStringLiteral("about:blank")));
    }
    void refuseExecutableChrome() {
        for (const auto &input : {QStringLiteral("javascript:alert(1)"), QStringLiteral("chrome://browser/content/browser.xhtml"),
                                  QStringLiteral("data:text/html,hello"), QStringLiteral("https://")}) {
            QString error;
            QVERIFY(QindaWeb::navigationUrl(input, &error).isEmpty());
            QVERIFY(!error.isEmpty());
        }
    }
    void encodedSearch() {
        const QString term = QStringLiteral("QindaQt & Gecko #ブラウザー");
        const QUrl result = QindaWeb::navigationUrl(term);
        QCOMPARE(result.host(), QStringLiteral("duckduckgo.com"));
        QCOMPARE(QUrlQuery(result).queryItemValue(QStringLiteral("q")), term);
    }
};
QTEST_GUILESS_MAIN(NavigationTest)
#include "tst_navigation.moc"
