#pragma once

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <algorithm>

// Wrapper macros for portable fast I/O
#if defined(__linux__)
#define SFIO_FWRITE fwrite_unlocked
#define SFIO_FREAD  fread_unlocked
#else
#define SFIO_FWRITE std::fwrite
#define SFIO_FREAD  std::fread
#endif

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

        std::vector<char> buffer; // 1MB buffer
        size_t cursor = 0;        // current position in buffer
        size_t bufferEnd = 0;     // end of valid data in buffer
    };

    inline TextReader::TextReader(const std::string& p)
        : path(p)
    {
        // Open the file in text read mode
        file = std::fopen(path.c_str(), "r");
        if (!file)
            throw std::runtime_error("Failed to open file '" + path + "'");

        // Allocate per-file 1 MB buffer for manual buffered reads
        buffer.resize(1 << 20);
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
            size_t readBytes = SFIO_FREAD(result.data(), 1, result.size(), file);
            if (readBytes != result.size())
                throw std::runtime_error("Failed to read full file '" + path + "'");
        }

        return result;
    }

    inline std::string TextReader::readLine() {
        if (!file) throw std::runtime_error("File not open");
        std::string line;

        while (true) {
            if (cursor >= bufferEnd) {
                // refill buffer
                size_t leftover = bufferEnd - cursor;
                if (leftover > 0) {
                    std::memmove(buffer.data(), buffer.data() + cursor, leftover);
                }
                size_t bytesRead = SFIO_FREAD(buffer.data() + leftover, 1, buffer.size() - leftover, file);
                if (bytesRead == 0) {
                    if (line.empty()) throw std::out_of_range("End of file reached");
                    break;
                }
                cursor = 0;
                bufferEnd = leftover + bytesRead;
            }

            // find newline
            size_t start = cursor;
            while (cursor < bufferEnd && buffer[cursor] != '\n') cursor++;

            line.append(buffer.data() + start, cursor - start);

            if (cursor < bufferEnd && buffer[cursor] == '\n') {
                cursor++; // skip newline
                break;
            }
            // continue loop if newline not found
        }

        return line;
    }

    inline std::vector<std::string> TextReader::readLines(int numLines) {
        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        try {
            while (numLines == 0 || lines.size() < static_cast<size_t>(numLines)) {
                lines.push_back(readLine());
            }
        } catch (const std::out_of_range&) {
            // Reached EOF - fail silently
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

        // Allocate per-file 1 MB buffer for assembling write payloads
        buffer.resize(1 << 20);
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
        // chunked write to avoid issues with extremely large strings
        const size_t chunkSize = 1 << 20;
        size_t offset = 0;
        while (offset < data.size()) {
            size_t toWrite = std::min(chunkSize, data.size() - offset);
            size_t written = SFIO_FWRITE(data.data() + offset, 1, toWrite, file);
            if (written != toWrite)
                throw std::runtime_error("Failed to write full string to '" + path + "'");
            offset += written;
        }
    }

    inline void TextWriter::writeLine(const std::string& line) {
        if (!file) throw std::runtime_error("File not open");
        // Use single preallocated buffer per TextWriter
        buffer.clear();
        buffer.insert(buffer.end(), line.begin(), line.end());
        buffer.push_back('\n');
        size_t written = SFIO_FWRITE(buffer.data(), 1, buffer.size(), file);
        if (written != buffer.size())
            throw std::runtime_error("Failed to write line to '" + path + "'");
    }

    inline void TextWriter::writeLines(const std::vector<std::string>& lines) {
        if (!file) throw std::runtime_error("File not open");
        if (lines.empty()) return;

        // Compute total size: sum of line sizes + missing newlines for lines that don't end with '\n'
        size_t totalSize = 0;
        size_t missingNewlines = 0;
        for (const auto &line : lines) {
            totalSize += line.size();
            if (line.empty() || line.back() != '\n') ++missingNewlines;
        }
        totalSize += missingNewlines;

        // Allocate once and memcpy all data into the buffer to avoid per-line overhead
        buffer.clear();
        buffer.resize(totalSize);
        char* ptr = buffer.data();

        for (const auto &line : lines) {
            size_t len = line.size();
            if (len > 0) {
                std::memcpy(ptr, line.data(), len);
                ptr += len;
            }
            if (len == 0 || line[len-1] != '\n') {
                *ptr++ = '\n';
            }
        }

        size_t written = SFIO_FWRITE(buffer.data(), 1, buffer.size(), file);
        if (written != buffer.size())
            throw std::runtime_error("Failed to write lines to '" + path + "'");
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

        // Allocate per-file 1 MB buffer for manual reads
        buffer.resize(1 << 20);
        // Do not use setvbuf() with our buffer here to avoid buffer aliasing with fread() calls
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
            size_t readBytes = SFIO_FREAD(data.data(), 1, data.size(), file);
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

        // Allocate per-file 1 MB buffer for assembling write payloads
        buffer.resize(1 << 20);
        // Do NOT pass this buffer to setvbuf(). Using the same memory for stdio's
        // internal buffer and as our write source can cause corrupted output when reused.
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
        const size_t chunkSize = 1 << 20; // 1 MB
        size_t offset = 0;
        while (offset < data.size()) {
            size_t toWrite = std::min(chunkSize, data.size() - offset);
            size_t written = SFIO_FWRITE(data.data() + offset, 1, toWrite, file);
            if (written != toWrite)
                throw std::runtime_error("Failed to write full data to '" + path + "'");
            offset += written;
        }
    }
}