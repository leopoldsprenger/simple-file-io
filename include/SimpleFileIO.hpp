#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>

namespace SimpleFileIO {
    // Text reader: optimized for text input
    class TextReader {
    public:
        inline TextReader(const std::string& path);
        inline ~TextReader();

        inline bool isOpen() const;
        inline static bool exists(const std::string& path);

        inline std::string readString();
        inline std::string readLine();
        inline std::vector<std::string> readLines(int numLines = 0);

        inline void flush();

    private:
        FILE* file = nullptr;
        std::string path;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline TextReader::TextReader(const std::string& p)
        : path(p)
    {
        // Open the file in text read mode
        file = std::fopen(path.c_str(), "r");
        if (!file)
            throw std::runtime_error("Failed to open file '" + path + "'");

        // Allocate per-file 1 MB buffer
        buffer.resize(1 << 20);
        if (std::setvbuf(file, buffer.data(), _IOFBF, buffer.size()) != 0) {
            throw std::runtime_error("Failed to set file buffer for '" + path + "'");
        }
    }

    inline TextReader::~TextReader() {
        if (!file) return;
        std::fclose(file);
    }

    inline bool TextReader::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool TextReader::isOpen() const {
        return file != nullptr;
    }

    inline void TextReader::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline std::string TextReader::readString() {
        std::fseek(file, 0, SEEK_END);
        long size = std::ftell(file);
        if (size < 0) throw std::runtime_error("Failed to determine file size");
        std::fseek(file, 0, SEEK_SET);

        std::string result(static_cast<size_t>(size), '\0');
        if (size > 0) {
            size_t readBytes = std::fread(result.data(), 1, result.size(), file);
            if (readBytes != result.size())
                throw std::runtime_error("Failed to read full file '" + path + "'");
        }

        return result;
    }

    inline std::string TextReader::readLine() {
        char buf[1024];
        std::string line;
        
        while (true) {
            // Read up to sizeof(buf)-1 characters, stops at newline or EOF
            if (!std::fgets(buf, sizeof(buf), file)) {
                if (line.empty() && std::feof(file))
                    throw std::out_of_range("End of file reached");
                break;
            }

            // Check if a newline is in the buffer
            char* nl = std::strchr(buf, '\n');
            if (nl) {
                *nl = '\0';          // remove newline
                line += buf;         // append the chunk up to newline
                break;               // line complete
            }

            line += buf;             // no newline, append entire chunk and continue
        }

        return line;
    }

    inline std::vector<std::string> TextReader::readLines(int numLines) {
        std::string data = readString(); // bulk read
        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        size_t prev = 0;
        size_t pos;
        while ((pos = data.find('\n', prev)) != std::string::npos) {
            lines.push_back(data.substr(prev, pos - prev));
            prev = pos + 1;
            if (numLines > 0 && lines.size() >= static_cast<size_t>(numLines))
                break;
        }

        // Add last line if it doesn't end with newline
        if ((numLines == 0 || lines.size() < static_cast<size_t>(numLines)) && prev < data.size()) {
            lines.push_back(data.substr(prev, data.size() - prev));
        }

        return lines;
    }

    // Text writer: optimized for text output
    class TextWriter {
    public:
        inline TextWriter(const std::string& path, bool append = false);
        inline ~TextWriter();

        inline bool isOpen() const;
        inline static bool exists(const std::string& path);
        inline void flush();

        inline void writeString(const std::string& data);
        inline void writeLine(const std::string& line);
        inline void writeLines(const std::vector<std::string>& lines);

    private:
        FILE* file = nullptr;
        std::string path;
        bool append = false;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline TextWriter::TextWriter(const std::string& p, bool a)
        : path(p), append(a)
    {
        const char* modeStr = append ? "a" : "w";
        file = std::fopen(path.c_str(), modeStr);
        if (!file)
            throw std::runtime_error("Failed to open file '" + path + "'");

        // Allocate per-file 1 MB buffer
        buffer.resize(1 << 20);
        if (std::setvbuf(file, buffer.data(), _IOFBF, buffer.size()) != 0) {
            throw std::runtime_error("Failed to set file buffer for '" + path + "'");
        }
    }

    inline TextWriter::~TextWriter() {
        if (!file) return;
        std::fflush(file);
        std::fclose(file);
    }

    inline bool TextWriter::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool TextWriter::isOpen() const {
        return file != nullptr;
    }

    inline void TextWriter::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline void TextWriter::writeString(const std::string& data) {
        size_t written = std::fwrite(data.data(), 1, data.size(), file);
        if (written != data.size())
            throw std::runtime_error("Failed to write full string to '" + path + "'");
    }

    inline void TextWriter::writeLine(const std::string& line) {
        writeString(line + '\n');
    }

    inline void TextWriter::writeLines(const std::vector<std::string>& lines) {
        std::string all;
        for (const auto& line : lines) {
            all += line;
            all += '\n';
        }
        writeString(all);
    }

    // Byte reader: optimized for binary input
    class ByteReader {
    public:
        inline ByteReader(const std::string& path);
        inline ~ByteReader();

        inline bool isOpen() const;
        inline static bool exists(const std::string& path);

        inline std::vector<char> readBytes();
        inline void flush();

    private:
        FILE* file = nullptr;
        std::string path;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline ByteReader::ByteReader(const std::string& p)
        : path(p)
    {
        file = std::fopen(path.c_str(), "rb");
        if (!file)
            throw std::runtime_error("Failed to open file '" + path + "'");

        // Allocate per-file 1 MB buffer
        buffer.resize(1 << 20);
        if (std::setvbuf(file, buffer.data(), _IOFBF, buffer.size()) != 0) {
            throw std::runtime_error("Failed to set file buffer for '" + path + "'");
        }
    }

    inline ByteReader::~ByteReader() {
        if (!file) return;
        std::fclose(file);
    }

    inline bool ByteReader::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool ByteReader::isOpen() const {
        return file != nullptr;
    }

    inline void ByteReader::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline std::vector<char> ByteReader::readBytes() {
        std::fseek(file, 0, SEEK_END);
        long size = std::ftell(file);
        if (size < 0) throw std::runtime_error("Failed to determine file size");
        std::fseek(file, 0, SEEK_SET);

        std::vector<char> data(static_cast<size_t>(size));
        if (size > 0) {
            size_t readBytes = std::fread(data.data(), 1, data.size(), file);
            if (readBytes != data.size())
                throw std::runtime_error("Failed to read full file '" + path + "'");
        }

        return data;
    }

    // Byte writer: optimized for binary output
    class ByteWriter {
    public:
        inline ByteWriter(const std::string& path, bool append = false);
        inline ~ByteWriter();

        inline bool isOpen() const;
        inline static bool exists(const std::string& path);
        inline void flush();

        inline void writeBytes(const std::vector<char>& data);

    private:
        FILE* file = nullptr;
        std::string path;
        bool append = false;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline ByteWriter::ByteWriter(const std::string& p, bool a)
        : path(p), append(a)
    {
        const char* modeStr = append ? "ab" : "wb";
        file = std::fopen(path.c_str(), modeStr);
        if (!file)
            throw std::runtime_error("Failed to open file '" + path + "'");

        // Allocate per-file 1 MB buffer
        buffer.resize(1 << 20);
        if (std::setvbuf(file, buffer.data(), _IOFBF, buffer.size()) != 0) {
            throw std::runtime_error("Failed to set file buffer for '" + path + "'");
        }
    }

    inline ByteWriter::~ByteWriter() {
        if (!file) return;
        std::fflush(file);
        std::fclose(file);
    }

    inline bool ByteWriter::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool ByteWriter::isOpen() const {
        return file != nullptr;
    }

    inline void ByteWriter::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline void ByteWriter::writeBytes(const std::vector<char>& data) {
        size_t written = std::fwrite(data.data(), 1, data.size(), file);
        if (written != data.size())
            throw std::runtime_error("Failed to write full data to '" + path + "'");
    }
}