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
    // Enums of all possible error types
    enum class IOError {
        FileNotOpen,
        FileNotFound,
        PermissionDenied,
        ReadError,
        WriteError
    };

    // Generall class for exceptions
    class IOException : public std::runtime_error {
    public:
        IOError code;
        std::string detail;
        std::string path;

        IOException(IOError code,
                    std::string message,
                    std::string path = "")
            : std::runtime_error(std::move(message)),
            code(code),
            path(std::move(path)) {}
    };

    // Translate error code into readable messages
    inline std::string formatIOError(IOError code,
                                    const std::string& path = "",
                                    const std::string& detail = "") {
        switch (code) {
            case IOError::FileNotOpen:
                return "File operation failed 'file is not open': " + path;
            case IOError::FileNotFound:
                return "File not found: " + path;
            case IOError::PermissionDenied:
                return "Permission denied while accessing: " + path;
            case IOError::ReadError:
                return "Low-level read error" + 
                        (detail.empty() ? "" : (": " + detail));
            case IOError::WriteError:
                return "Low-level write error" + 
                        (detail.empty() ? "" : (": " + detail));
            default:
                return "Unknown I/O error.";
        }
    }

    // Text reader: optimized for text input
    class TextReader {
    public:
        inline TextReader(const std::string& path);
        inline ~TextReader();

        inline static bool exists(const std::string& path);

        inline std::string readString();
        inline std::string readLine();
        inline std::vector<std::string> readLines(int numLines = 0);

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
        if (!file) {
            IOError code;
            switch (errno) {
                case ENOENT: // No such file or directory
                    code = IOError::FileNotFound;
                    break;
                case EACCES: // Permission denied
                    code = IOError::PermissionDenied;
                    break;
                default:
                    code = IOError::FileNotOpen;
            }
            throw IOException(code, formatIOError(code, path), path);
        }

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

    inline std::string TextReader::readString() {
        if (!file) 
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        std::string result;
        result.reserve(4 << 20); // start with 4 MB, grows dynamically if needed

        while (true) {
            size_t bytesRead = SFIO_FREAD(buffer.data(), 1, buffer.size(), file);
            if (bytesRead == 0) {
                if (ferror(file))
                    throw IOException(IOError::ReadError, formatIOError(IOError::ReadError, path), path);
                break;
            }
            result.append(buffer.data(), bytesRead);
        }
        return result;
    }

    inline std::string TextReader::readLine() {
        if (!file) 
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        std::string line;
        bool anyDataRead = false;

        while (true) {
            if (cursor >= bufferEnd) {
                // refill buffer
                size_t leftover = bufferEnd - cursor;
                if (leftover > 0) {
                    std::memmove(buffer.data(), buffer.data() + cursor, leftover);
                }
                size_t bytesRead = SFIO_FREAD(buffer.data() + leftover, 1, buffer.size() - leftover, file);
                cursor = 0;
                bufferEnd = leftover + bytesRead;

                if (bytesRead == 0) {
                    if (ferror(file))
                        throw IOException(IOError::ReadError, formatIOError(IOError::ReadError, path), path);
                    break; // normal EOF
                }
            }

            size_t start = cursor;
            while (cursor < bufferEnd && buffer[cursor] != '\n') cursor++;
            if (cursor > start) {
                line.append(buffer.data() + start, cursor - start);
                anyDataRead = true;
            }

            if (cursor < bufferEnd && buffer[cursor] == '\n') {
                cursor++; // skip newline
                break;
            }
        }

        if (!anyDataRead && std::feof(file))
            return {}; // EOF reached, return empty string without throwing

        return line;
    }

    inline std::vector<std::string> TextReader::readLines(int numLines) {
        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        while (numLines == 0 || lines.size() < static_cast<size_t>(numLines)) {
            std::string line = readLine();
            if (line.empty() && std::feof(file)) break; // stop at EOF
            lines.push_back(std::move(line));
        }

        return lines;
    }

    // Text writer: optimized for text output
    class TextWriter {
    public:
        inline TextWriter(const std::string& path, bool append = false);
        inline ~TextWriter();

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
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

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
                throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write string to file."), path);
            offset += written;
        }
    }

    inline void TextWriter::writeLine(const std::string& line) {
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        // Use single preallocated buffer per TextWriter
        buffer.clear();
        buffer.insert(buffer.end(), line.begin(), line.end());
        buffer.push_back('\n');
        size_t written = SFIO_FWRITE(buffer.data(), 1, buffer.size(), file);
        if (written != buffer.size())
            throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write line to file."), path);
    }

    inline void TextWriter::writeLines(const std::vector<std::string>& lines) {
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);
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
            throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write lines to file."), path);
    }

    // Byte reader: optimized for binary input
    class ByteReader {
    public:
        inline ByteReader(const std::string& path);
        inline ~ByteReader();

        inline static bool exists(const std::string& path);

        inline std::vector<char> readBytes();

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
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

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

    inline std::vector<char> ByteReader::readBytes() {
        std::vector<char> data;
        data.reserve(4 << 20); // start with 4 MB, grows dynamically if needed

        while (true) {
            size_t bytesRead = SFIO_FREAD(buffer.data(), 1, buffer.size(), file);
            if (bytesRead == 0) {
                if (ferror(file))
                    throw IOException(IOError::ReadError, formatIOError(IOError::ReadError, path), path);
                break;
            }
            data.insert(data.end(), buffer.data(), buffer.data() + bytesRead);
        }

        return data;
    }

    // Byte writer: optimized for binary output
    class ByteWriter {
    public:
        inline ByteWriter(const std::string& path, bool append = false);
        inline ~ByteWriter();

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
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

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
                throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write bytes to file."), path);
            offset += written;
        }
    }
}