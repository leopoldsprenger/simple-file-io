#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <filesystem>

namespace SimpleFileIO {
    // Bitmask flags for opening modes
    enum class OpenMode : uint8_t {
        None   = 0,
        Read   = 1 << 0,
        Write  = 1 << 1,
        Append = 1 << 2,
        Binary = 1 << 3
    };

    inline bool has(OpenMode value, OpenMode flag) {
        return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
    }

    inline OpenMode operator|(OpenMode a, OpenMode b) {
        return static_cast<OpenMode>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
        );
    }

    inline OpenMode operator&(OpenMode a, OpenMode b) {
        return static_cast<OpenMode>(
            static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
        );
    }

    class File {
    public:
        inline File(const std::string& path, OpenMode mode);
        inline ~File();

        inline bool isOpen() const;
        inline static bool exists(const std::string& path);
        inline void flush();

        inline std::string readString();
        inline std::string readLine();
        inline std::vector<std::string> readLines(int numLines = 0);

        inline void writeString(const std::string& data);
        inline void writeLine(const std::string& line);
        inline void writeLines(const std::vector<std::string>& lines);

        inline std::vector<char> readBytes();
        inline void writeBytes(const std::vector<char>& data);

    private:
        struct Impl;
        Impl* pImpl;
    };

    struct File::Impl {
        FILE* file = nullptr;
        OpenMode mode;
        std::string path;

        bool canRead;
        bool canWrite;
        bool canAppend;
        bool isBinary;

        inline Impl(const std::string& p, OpenMode m)
            : mode(m), path(p)
        {
            if (mode == OpenMode::None)
                throw std::logic_error("No mode specified");

            // Cache boolean flags
            canRead   = has(mode, OpenMode::Read);
            canWrite  = has(mode, OpenMode::Write);
            canAppend = has(mode, OpenMode::Append);
            isBinary  = has(mode, OpenMode::Binary);

            // Ensure exactly one of Read/Write/Append is set
            int access_count = static_cast<int>(canRead) + 
                            static_cast<int>(canWrite) + 
                            static_cast<int>(canAppend);
            if (access_count != 1)
                throw std::logic_error(
                    "Exactly one of Read, Write, or Append must be set"
                );

            // Determine fopen mode string
            const char* modeStr = nullptr;
            if (canRead)        modeStr = isBinary ? "rb" : "r";
            else if (canWrite)  modeStr = isBinary ? "wb" : "w";
            else if (canAppend) modeStr = isBinary ? "ab" : "a";

            // Open the file
            file = std::fopen(path.c_str(), modeStr);
            if (!file)
                throw std::runtime_error("Failed to open file '" + path + "'");

            // Optimize buffering: use a large 1 MB buffer for faster I/O
            static char buf[1 << 20]; // 1 MB
            if (std::setvbuf(file, buf, _IOFBF, sizeof(buf)) != 0) {
                throw std::runtime_error("Failed to set file buffer for '" + path + "'");
            }
        }
    };

    inline File::File(const std::string& path, OpenMode mode)
        : pImpl(new Impl(path, mode)) {}

    inline File::~File() {
        if (!pImpl) return;

        if (pImpl->file) {
            if (pImpl->canWrite || pImpl->canAppend)
                std::fflush(pImpl->file);
            std::fclose(pImpl->file);
        }

        delete pImpl;
    }

    inline bool File::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool File::isOpen() const {
        return pImpl->file != nullptr;
    }

    inline void File::flush() {
        if (!pImpl->file) return;
        std::fflush(pImpl->file);
    }

    inline std::string File::readString() {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (pImpl->isBinary)
            throw std::runtime_error("readString() not supported in binary mode");

        std::fseek(pImpl->file, 0, SEEK_END);
        long size = std::ftell(pImpl->file);
        if (size < 0) throw std::runtime_error("Failed to determine file size");
        std::fseek(pImpl->file, 0, SEEK_SET);

        std::string result(static_cast<size_t>(size), '\0');
        if (size > 0) {
            size_t readBytes = std::fread(result.data(), 1, result.size(), pImpl->file);
            if (readBytes != result.size())
                throw std::runtime_error("Failed to read full file '" + pImpl->path + "'");
        }

        return result;
    }

    inline std::string File::readLine() {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (pImpl->isBinary)
            throw std::runtime_error("readLine() not supported in binary mode");

        std::string line;
        line.reserve(128);
        int c;
        while ((c = std::fgetc(pImpl->file)) != EOF) {
            if (c == '\n') break;
            line += static_cast<char>(c);
        }
        if (line.empty() && std::feof(pImpl->file))
            throw std::out_of_range("End of file reached");

        return line;
    }

    inline std::vector<std::string> File::readLines(int numLines) {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (pImpl->isBinary)
            throw std::runtime_error("readLines() not supported in binary mode");

        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        try {
            while (numLines == 0 || lines.size() < numLines)
                lines.push_back(readLine());
        } catch (const std::out_of_range&) {
            // EOF reached, stop reading
        }

        return lines;
    }

    inline void File::writeString(const std::string& data) {
        if (!pImpl->canWrite && !pImpl->canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (pImpl->isBinary)
            throw std::runtime_error("writeString() not supported in binary mode");

        size_t written = std::fwrite(data.data(), 1, data.size(), pImpl->file);
        if (written != data.size())
            throw std::runtime_error("Failed to write full string to '" + pImpl->path + "'");
    }

    inline void File::writeLine(const std::string& line) {
        writeString(line + '\n');
    }

    inline void File::writeLines(const std::vector<std::string>& lines) {
        for (const auto& line : lines)
            writeLine(line);
    }

    inline std::vector<char> File::readBytes() {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (!pImpl->isBinary)
            throw std::runtime_error("readBytes() requires binary mode");

        std::fseek(pImpl->file, 0, SEEK_END);
        long size = std::ftell(pImpl->file);
        if (size < 0) throw std::runtime_error("Failed to determine file size");
        std::fseek(pImpl->file, 0, SEEK_SET);

        std::vector<char> data(static_cast<size_t>(size));
        if (size > 0) {
            size_t readBytes = std::fread(data.data(), 1, data.size(), pImpl->file);
            if (readBytes != data.size())
                throw std::runtime_error("Failed to read full file '" + pImpl->path + "'");
        }

        return data;
    }

    inline void File::writeBytes(const std::vector<char>& data) {
        if (!pImpl->canWrite && !pImpl->canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (!pImpl->isBinary)
            throw std::runtime_error("writeBytes() requires binary mode");

        size_t written = std::fwrite(data.data(), 1, data.size(), pImpl->file);
        if (written != data.size())
            throw std::runtime_error("Failed to write full data to '" + pImpl->path + "'");
    }
}