/*
 * Copyright (c) 2015 Nathan Osman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <QList>
#include <QObject>
#include <QTest>

#include <QHttpEngine/QHttpParser>
#include <QHttpEngine/QHttpSocket>
#include <QHttpEngine/QIByteArray>

typedef QList<QByteArray> QByteArrayList;

Q_DECLARE_METATYPE(QHttpSocket::Method)
Q_DECLARE_METATYPE(QHttpSocket::QueryStringMap)
Q_DECLARE_METATYPE(QHttpSocket::HeaderMap)

const QIByteArray Key1 = "a";
const QByteArray Value1 = "b";
const QByteArray Line1 = Key1 + ": " + Value1;

const QIByteArray Key2 = "c";
const QByteArray Value2 = "d";
const QByteArray Line2 = Key2 + ": " + Value2;

class TestQHttpParser : public QObject
{
    Q_OBJECT

public:

    TestQHttpParser();

private Q_SLOTS:

    void testSplit_data();
    void testSplit();

    void testParsePath_data();
    void testParsePath();

    void testParseHeaderList_data();
    void testParseHeaderList();

    void testParseHeaders_data();
    void testParseHeaders();

    void testParseRequestHeaders_data();
    void testParseRequestHeaders();

    void testParseResponseHeaders_data();
    void testParseResponseHeaders();

private:

    QHttpSocket::HeaderMap headers;
};

TestQHttpParser::TestQHttpParser()
{
    headers.insert(Key1, Value1);
    headers.insert(Key2, Value2);
}

void TestQHttpParser::testSplit_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("delim");
    QTest::addColumn<int>("maxSplit");
    QTest::addColumn<QByteArrayList>("parts");

    QTest::newRow("empty string")
            << QByteArray()
            << QByteArray(",")
            << 0
            << (QByteArrayList() << "");

    QTest::newRow("no delimiter")
            << QByteArray("a")
            << QByteArray(",")
            << 0
            << (QByteArrayList() << "a");

    QTest::newRow("delimiter")
            << QByteArray("a::b::c")
            << QByteArray("::")
            << 0
            << (QByteArrayList() << "a" << "b" << "c");

    QTest::newRow("empty parts")
            << QByteArray("a,,")
            << QByteArray(",")
            << 0
            << (QByteArrayList() << "a" << "" << "");

    QTest::newRow("maxSplit")
            << QByteArray("a,a,a")
            << QByteArray(",")
            << 1
            << (QByteArrayList() << "a" << "a,a");
}

void TestQHttpParser::testSplit()
{
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, delim);
    QFETCH(int, maxSplit);
    QFETCH(QByteArrayList, parts);

    QByteArrayList outParts;
    QHttpParser::split(data, delim, maxSplit, outParts);

    QCOMPARE(outParts, parts);
}

void TestQHttpParser::testParsePath_data()
{
    QTest::addColumn<QByteArray>("rawPath");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QHttpSocket::QueryStringMap>("map");

    QTest::newRow("no query string")
            << QByteArray("/path")
            << QString("/path")
            << QHttpSocket::QueryStringMap();

    QTest::newRow("single parameter")
            << QByteArray("/path?a=b")
            << QString("/path")
            << QHttpSocket::QueryStringMap{{"a", "b"}};
}

void TestQHttpParser::testParsePath()
{
    QFETCH(QByteArray, rawPath);
    QFETCH(QString, path);
    QFETCH(QHttpSocket::QueryStringMap, map);

    QString outPath;
    QHttpSocket::QueryStringMap outMap;

    QVERIFY(QHttpParser::parsePath(rawPath, outPath, outMap));

    QCOMPARE(path, outPath);
    QCOMPARE(map, outMap);
}

void TestQHttpParser::testParseHeaderList_data()
{
    QTest::addColumn<bool>("success");
    QTest::addColumn<QByteArrayList>("lines");
    QTest::addColumn<QHttpSocket::HeaderMap>("headers");

    QTest::newRow("empty line")
            << false
            << (QByteArrayList() << "");

    QTest::newRow("multiple lines")
            << true
            << (QByteArrayList() << Line1 << Line2)
            << headers;
}

void TestQHttpParser::testParseHeaderList()
{
    QFETCH(bool, success);
    QFETCH(QByteArrayList, lines);

    QHttpSocket::HeaderMap outHeaders;
    QCOMPARE(QHttpParser::parseHeaderList(lines, outHeaders), success);

    if (success) {
        QFETCH(QHttpSocket::HeaderMap, headers);
        QCOMPARE(outHeaders, headers);
    }
}

void TestQHttpParser::testParseHeaders_data()
{
    QTest::addColumn<bool>("success");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArrayList>("parts");

    QTest::newRow("empty headers")
            << false
            << QByteArray("");

    QTest::newRow("simple GET request")
            << true
            << QByteArray("GET / HTTP/1.0")
            << (QByteArrayList() << "GET" << "/" << "HTTP/1.0");
}

void TestQHttpParser::testParseHeaders()
{
    QFETCH(bool, success);
    QFETCH(QByteArray, data);

    QByteArrayList outParts;
    QHttpSocket::HeaderMap outHeaders;

    QCOMPARE(QHttpParser::parseHeaders(data, outParts, outHeaders), success);

    if (success) {
        QFETCH(QByteArrayList, parts);
        QCOMPARE(outParts, parts);
    }
}

void TestQHttpParser::testParseRequestHeaders_data()
{
    QTest::addColumn<bool>("success");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QHttpSocket::Method>("method");
    QTest::addColumn<QByteArray>("path");

    QTest::newRow("bad HTTP version")
            << false
            << QByteArray("GET / HTTP/0.9");

    QTest::newRow("GET request")
            << true
            << QByteArray("GET / HTTP/1.0")
            << QHttpSocket::GET
            << QByteArray("/");
}

void TestQHttpParser::testParseRequestHeaders()
{
    QFETCH(bool, success);
    QFETCH(QByteArray, data);

    QHttpSocket::Method outMethod;
    QByteArray outPath;
    QHttpSocket::HeaderMap outHeaders;

    QCOMPARE(QHttpParser::parseRequestHeaders(data, outMethod, outPath, outHeaders), success);

    if (success) {
        QFETCH(QHttpSocket::Method, method);
        QFETCH(QByteArray, path);

        QCOMPARE(method, outMethod);
        QCOMPARE(path, outPath);
    }
}

void TestQHttpParser::testParseResponseHeaders_data()
{
    QTest::addColumn<bool>("success");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QByteArray>("statusReason");

    QTest::newRow("invalid status code")
            << false
            << QByteArray("HTTP/1.0 600 BAD RESPONSE");

    QTest::newRow("404 response")
            << true
            << QByteArray("HTTP/1.0 404 NOT FOUND")
            << 404
            << QByteArray("NOT FOUND");
}

void TestQHttpParser::testParseResponseHeaders()
{
    QFETCH(bool, success);
    QFETCH(QByteArray, data);

    int outStatusCode;
    QByteArray outStatusReason;
    QHttpSocket::HeaderMap outHeaders;

    QCOMPARE(QHttpParser::parseResponseHeaders(data, outStatusCode, outStatusReason, outHeaders), success);

    if (success) {
        QFETCH(int, statusCode);
        QFETCH(QByteArray, statusReason);

        QCOMPARE(statusCode, outStatusCode);
        QCOMPARE(statusReason, outStatusReason);
    }
}

QTEST_MAIN(TestQHttpParser)
#include "TestQHttpParser.moc"
