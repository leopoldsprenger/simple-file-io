#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>

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

    // Bitwise operators for OpenMode flags
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
        std::fstream file;
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

            // Build open flags
            std::ios::openmode flags = std::ios::openmode{};
            if (canRead) flags |= std::ios::in;
            if (canWrite) flags |= std::ios::out | std::ios::trunc;
            if (canAppend) flags |= std::ios::out | std::ios::app;
            if (isBinary) flags |= std::ios::binary;

            // Open the single fstream and set exceptions
            try {
                file.open(path, flags);
                file.exceptions(std::ios::badbit);
            } catch (const std::ios_base::failure& e) {
                throw std::runtime_error(
                    "Failed to open file '" + path + "': " + e.what()
                );
            }
        }
    };

    inline File::File(const std::string& path, OpenMode mode)
        : pImpl(new Impl(path, mode)) {}

    inline File::~File() {
        if (!pImpl) return;

        if (pImpl->file.is_open()) {
            if (pImpl->canWrite || pImpl->canAppend)
                pImpl->file.flush();
            pImpl->file.close();
        }

        delete pImpl;
    }

    inline bool File::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool File::isOpen() const {
        return pImpl->file.is_open();
    }

    inline void File::flush() {
        pImpl->file.flush();
    }

    inline std::string File::readString() {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (pImpl->isBinary)
            throw std::runtime_error("readString() not supported in binary mode");

        pImpl->file.clear();

        // Move to end to get file size
        pImpl->file.seekg(0, std::ios::end);
        auto end = pImpl->file.tellg();
        if (end < 0)
            throw std::runtime_error("Failed to determine file size");

        size_t size = static_cast<size_t>(end);
        pImpl->file.seekg(0, std::ios::beg);

        std::string result(size, '\0');

        // Only wrap the actual read in try/catch
        try {
            pImpl->file.read(result.data(), size);
        } catch (const std::ios_base::failure& e) {
            throw std::runtime_error(
                "Error reading text file '" + pImpl->path + "': " + e.what()
            );
        }

        return result;
    }

    inline std::string File::readLine() {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (pImpl->isBinary)
            throw std::runtime_error("readLine() not supported in binary mode");

        std::string line;
        if (std::getline(pImpl->file, line))
            return line;

        // EOF reached
        if (pImpl->file.eof()) 
            throw std::out_of_range("End of file reached");

        throw std::ios_base::failure("Unknown iostream error");
    }

    inline std::vector<std::string> File::readLines(int numLines) {
        if (!pImpl->canRead)
            throw std::runtime_error("File not opened in read mode");
        if (pImpl->isBinary)
            throw std::runtime_error("readLines() not supported in binary mode");

        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        std::string line;
        while ((numLines == 0 || lines.size() < numLines) &&
            std::getline(pImpl->file, line)) {
            lines.push_back(std::move(line));
        }

        return lines;
    }

    inline void File::writeString(const std::string& data) {
        if (!pImpl->canWrite && !pImpl->canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (pImpl->isBinary)
            throw std::runtime_error("writeString() not supported in binary mode");

        if (pImpl->canAppend)
            pImpl->file.seekp(0, std::ios::end);

        pImpl->file << data;
    }

    inline void File::writeLine(const std::string& line) {
        if (!pImpl->canWrite && !pImpl->canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (pImpl->isBinary)
            throw std::runtime_error("writeLine() not supported in binary mode");

        if (pImpl->canAppend)
            pImpl->file.seekp(0, std::ios::end);

        pImpl->file << line << '\n';
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

        pImpl->file.clear();
        pImpl->file.seekg(0, std::ios::end);

        auto end = pImpl->file.tellg();
        if (end < 0)
            throw std::runtime_error("Failed to determine file size");

        size_t size = static_cast<size_t>(end);
        pImpl->file.seekg(0);

        std::vector<char> data(size);
        pImpl->file.read(data.data(), size);
        return data;
    }

    inline void File::writeBytes(const std::vector<char>& data) {
        if (!pImpl->canWrite && !pImpl->canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (!pImpl->isBinary)
            throw std::runtime_error("writeBytes() requires binary mode");

        // Move write pointer to end if in append mode
        if (pImpl->canAppend)
            pImpl->file.seekp(0, std::ios::end);

        pImpl->file.write(data.data(), data.size());
    }
}