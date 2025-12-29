#include "SimpleFileIO/File.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

using namespace SimpleFileIO;
namespace fs = std::filesystem;

const std::string textFile = "test.txt";
const std::string binaryFile = "binary.bin";

// Helper to clean up files before/after tests
void removeFile(const std::string& path) {
    if (fs::exists(path)) fs::remove(path);
}

TEST_CASE("File existence and open", "[File]") {
    removeFile(textFile);

    REQUIRE_FALSE(File::exists(textFile));

    File fWrite(textFile, OpenMode::Write);
    REQUIRE(fWrite.isOpen());

    REQUIRE(File::exists(textFile));
}

TEST_CASE("Write and read all (text)", "[File][Text]") {
    removeFile(textFile);

    const std::string content = "Hello world!\nSecond line";

    {
        File fWrite(textFile, OpenMode::Write);
        fWrite.writeAll(content);
    }

    {
        File fRead(textFile, OpenMode::Read);
        REQUIRE(fRead.readAll() == content);
    }
}

TEST_CASE("Write and read lines (text)", "[File][Text]") {
    removeFile(textFile);

    std::vector<std::string> lines = {"line1", "line2", "line3"};

    {
        File fWrite(textFile, OpenMode::Write);
        fWrite.writeLines(lines);
    }

    {
        File fRead(textFile, OpenMode::Read);
        auto readLines = fRead.readLines();
        REQUIRE(readLines == lines);
    }
}

TEST_CASE("Write line and read line (text)", "[File][Text]") {
    removeFile(textFile);

    const std::string line1 = "first line";
    const std::string line2 = "second line";

    {
        File fWrite(textFile, OpenMode::Write);
        fWrite.writeLine(line1);
        fWrite.writeLine(line2);
    }

    {
        File fRead(textFile, OpenMode::Read);
        REQUIRE(fRead.readLine() == line1);
        REQUIRE(fRead.readLine() == line2);
        REQUIRE_THROWS_AS(fRead.readLine(), std::out_of_range);
    }
}

TEST_CASE("Append mode works (text)", "[File][Text]") {
    removeFile(textFile);

    {
        File fWrite(textFile, OpenMode::Write);
        fWrite.writeLine("first");
    }

    {
        File fAppend(textFile, OpenMode::Append);
        fAppend.writeLine("second");
    }

    {
        File fRead(textFile, OpenMode::Read);
        auto lines = fRead.readLines();
        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "first");
        REQUIRE(lines[1] == "second");
    }
}

TEST_CASE("Binary write and read", "[File][Binary]") {
    removeFile(binaryFile);

    const std::string binaryData = std::string("\0\1\2\3\4\255", 6);

    {
        File fWrite(binaryFile, OpenMode::Write | OpenMode::Binary);
        fWrite.writeAll(binaryData);
    }

    {
        File fRead(binaryFile, OpenMode::Read | OpenMode::Binary);
        auto content = fRead.readAll();
        REQUIRE(content.size() == binaryData.size());
        REQUIRE(content == binaryData);
    }
}

TEST_CASE("Mode validation", "[File]") {
    removeFile(textFile);

    REQUIRE_THROWS_AS(File(textFile, OpenMode::None), std::logic_error);
    REQUIRE_THROWS_AS(File(textFile, OpenMode::Read | OpenMode::Write), std::logic_error);
}

TEST_CASE("Text vs Binary errors", "[File]") {
    removeFile(binaryFile);

    File fBinary(binaryFile, OpenMode::Write | OpenMode::Binary);
    REQUIRE_THROWS_AS(fBinary.writeLine("nope"), std::runtime_error);

    File fText(textFile, OpenMode::Write);
    REQUIRE_THROWS_AS(fText.readAll(), std::runtime_error);
}