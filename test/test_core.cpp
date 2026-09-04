#include "core/FileManager.h"
#include "service/NetworkAddressSelector.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

namespace {

bool expect(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << '\n';
    return condition;
}

bool testNormalization()
{
    bool ok = true;
    ok &= expect(FileManager::normalizeRecordContent("a\nb") == "a\nb",
                 "single newline must be preserved");
    ok &= expect(FileManager::normalizeRecordContent("a\n\n\n b") == "a\n b",
                 "consecutive LF must collapse");
    ok &= expect(FileManager::normalizeRecordContent("a\r\n\r\nb\rc") == "a\nb\nc",
                 "mixed line endings must normalize");
    ok &= expect(FileManager::normalizeRecordContent("").isEmpty(),
                 "empty content must remain empty");
    return ok;
}

bool testParsing()
{
    const QString text =
        "09:00\r\nfirst\r\n\r\nstill first\r\n\r\n"
        "10:15\nsecond\nline\n\n25:99\nnot a record";
    const auto records = FileManager::parseContent(text);
    bool ok = true;
    ok &= expect(records.size() == 2, "parser must preserve two valid records");
    if (records.size() == 2) {
        ok &= expect(records[0].time == "09:00", "first time must parse");
        ok &= expect(records[0].content == "first\nstill first",
                     "content after an empty line must not be lost");
        ok &= expect(records[1].time == "10:15", "second time must parse");
        ok &= expect(records[1].content == "second\nline\n25:99\nnot a record",
                     "invalid time header must remain content");
    }
    return ok;
}

bool testRoundTripAndWrite()
{
    const QList<DailyRecord> input = {
        {"11:30", "alpha\n\n beta"},
        {"08:05", "early\r\nline"}
    };
    const QString serialized = FileManager::serializeRecords(input);
    const auto records = FileManager::parseContent(serialized);
    bool ok = true;
    ok &= expect(records.size() == 2, "round trip must keep record count");
    if (records.size() == 2) {
        ok &= expect(records[0].time == "08:05" && records[0].content == "early\nline",
                     "records must sort and normalize");
        ok &= expect(records[1].content == "alpha\n beta",
                     "serialization must remove consecutive newlines");
    }

    QTemporaryDir temp;
    FileManager files(temp.path());
    ok &= expect(files.writeDailyFile(2026, 9, 4, input), "daily file must write");
    QFile file(files.buildAbsolutePath(2026, 9, 4));
    ok &= expect(file.open(QIODevice::ReadOnly), "written daily file must open");
    if (file.isOpen()) {
        QString diskContent = QString::fromUtf8(file.readAll());
        diskContent.replace("\r\n", "\n");
        ok &= expect(diskContent == serialized,
                     "disk content must use canonical serialization");
    }
    return ok;
}

bool testNetworkAddressSelection()
{
    const NetworkAddressCandidate lan{"192.168.1.20", "wifi", true, true, false};
    const NetworkAddressCandidate tun{"198.18.0.1", "wintun", false, false, true};
    const NetworkAddressCandidate fallback{"100.64.0.8", "adapter", false, false, false};
    bool ok = true;
    ok &= expect(selectLanAddress({tun, fallback, lan}) == lan.address,
                 "physical private LAN address must win");
    ok &= expect(selectLanAddress({lan, tun, fallback}) == lan.address,
                 "selection must not depend on enumeration order");
    ok &= expect(selectLanAddress({tun, fallback}) == fallback.address,
                 "non-tunnel address must be used as fallback");
    ok &= expect(selectLanAddress({tun}) == "127.0.0.1",
                 "tunnel-only candidates must fall back to loopback");
    ok &= expect(selectLanAddress({}) == "127.0.0.1",
                 "empty candidates must fall back to loopback");
    return ok;
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const bool ok = testNormalization() && testParsing() && testRoundTripAndWrite()
        && testNetworkAddressSelection();
    return ok ? 0 : 1;
}
