#include "SimpleFileIO.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

using namespace SimpleFileIO;
namespace fs = std::filesystem;

const std::string textFile = "test.txt";
const std::string binaryFile = "binary.bin";

void removeFile(const std::string& path) {
    if (fs::exists(path)) fs::remove(path);
}

TEST_CASE("File existence and open", "[File]") {
    removeFile(textFile);

    REQUIRE_FALSE(TextWriter::exists(textFile));

    TextWriter fWrite(textFile);
    REQUIRE(fWrite.isOpen());

    REQUIRE(TextWriter::exists(textFile));
}

TEST_CASE("Write and read string (text)", "[File][Text]") {
    removeFile(textFile);

    const std::string content = "Hello world!\nSecond line";

    {
        TextWriter fWrite(textFile);
        fWrite.writeString(content);
    }

    {
        TextReader fRead(textFile);
        REQUIRE(fRead.readString() == content);
    }
}

TEST_CASE("Write and read lines (text)", "[File][Text]") {
    removeFile(textFile);

    std::vector<std::string> lines = {"line1", "line2", "line3"};

    {
        TextWriter fWrite(textFile);
        fWrite.writeLines(lines);
    }

    {
        TextReader fRead(textFile);
        auto readLines = fRead.readLines();
        REQUIRE(readLines == lines);
    }
}

TEST_CASE("Write line and read line (text)", "[File][Text]") {
    removeFile(textFile);

    const std::string line1 = "first line";
    const std::string line2 = "second line";

    {
        TextWriter fWrite(textFile);
        fWrite.writeLine(line1);
        fWrite.writeLine(line2);
    }

    {
        TextReader fRead(textFile);
        REQUIRE(fRead.readLine() == line1);
        REQUIRE(fRead.readLine() == line2);

        std::string eofLine;
        REQUIRE_THROWS_AS( [&]{ eofLine = fRead.readLine(); }(), std::out_of_range );
    }
}

TEST_CASE("Append mode works (text)", "[File][Text]") {
    removeFile(textFile);

    {
        TextWriter fWrite(textFile, false);
        fWrite.writeLine("first");
    }

    {
        TextWriter fAppend(textFile, true);
        fAppend.writeLine("second");
    }

    {
        TextReader fRead(textFile);
        auto lines = fRead.readLines();
        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == "first");
        REQUIRE(lines[1] == "second");
    }
}

TEST_CASE("Binary write and read bytes", "[File][Binary]") {
    removeFile(binaryFile);

    std::vector<char> data = {char(0), char(1), char(2), char(3), char(4), char(-1)};

    {
        ByteWriter fWrite(binaryFile);
        fWrite.writeBytes(data);
    }

    {
        ByteReader fRead(binaryFile);
        auto content = fRead.readBytes();
        REQUIRE(content.size() == data.size());

        for (size_t i = 0; i < content.size(); ++i)
            REQUIRE(content[i] == data[i]);
    }
}

TEST_CASE("Reader on missing file throws", "[File]") {
    removeFile(textFile);
    REQUIRE_THROWS_AS(TextReader(textFile), std::runtime_error);
}
