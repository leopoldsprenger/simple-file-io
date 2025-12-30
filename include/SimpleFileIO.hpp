#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <cstring>

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

        // User-facing functions: char-based
        inline std::vector<char> readBytes();
        inline void writeBytes(const std::vector<char>& data);

    private:
        struct Impl;
        Impl* pImpl;

        // Internal std::byte-based core
        inline std::vector<std::byte> readBytesInternal();
        inline void writeBytesInternal(const std::vector<std::byte>& data);
    };

    struct File::Impl {
        std::ifstream inFile;
        std::ofstream outFile;
        OpenMode mode;
        std::string path;

        inline Impl(const std::string& p, OpenMode m)
            : mode(m), path(p)
        {
            if (mode == OpenMode::None)
                throw std::logic_error("No mode specified");

            // Ensure exactly one of Read/Write/Append is set
            int access_count =
                (has(mode, OpenMode::Read)   ? 1 : 0) +
                (has(mode, OpenMode::Write)  ? 1 : 0) +
                (has(mode, OpenMode::Append) ? 1 : 0);

            if (access_count != 1)
                throw std::logic_error(
                    "Exactly one of Read, Write, or Append must be set"
                );

            std::ios::openmode flags = std::ios::openmode{};

            if (has(mode, OpenMode::Read))   flags |= std::ios::in;
            if (has(mode, OpenMode::Write))  flags |= std::ios::out | std::ios::trunc;
            if (has(mode, OpenMode::Append)) flags |= std::ios::out | std::ios::app;
            if (has(mode, OpenMode::Binary)) flags |= std::ios::binary;

            // Open the file with proper flags and enable exceptions
            try {
                if (has(mode, OpenMode::Read)) {
                    inFile.open(path, flags);
                    inFile.exceptions(std::ios::badbit); // only throw on badbit
                } else {
                    outFile.open(path, flags);
                    outFile.exceptions(std::ios::failbit | std::ios::badbit);
                }
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
        if (pImpl) {
            if (pImpl->outFile.is_open()) {
                pImpl->outFile.flush(); // ensure data is written
                pImpl->outFile.close();
            }
            if (pImpl->inFile.is_open()) {
                pImpl->inFile.close();
            }
            delete pImpl;
        }
    }

    inline bool File::exists(const std::string& path) {
        std::ifstream file(path);
        return file.is_open(); // simple existence check
    }

    inline bool File::isOpen() const {
        return pImpl->inFile.is_open() || pImpl->outFile.is_open();
    }

    inline void File::flush() {
        pImpl->outFile.flush();
    }

    inline std::string File::readString() {
        if (!has(pImpl->mode, OpenMode::Read))
            throw std::runtime_error("File not opened in read mode");
        if (has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("readString() not supported in binary mode");

        try {
            std::ostringstream buffer;
            buffer << pImpl->inFile.rdbuf(); // read full file into string
            return buffer.str();
        } catch (const std::ios_base::failure& e) {
            throw std::runtime_error(
                "Error reading text file '" + pImpl->path + "': " + e.what()
            );
        }
    }

    inline std::string File::readLine() {
        if (!has(pImpl->mode, OpenMode::Read))
            throw std::runtime_error("File not opened in read mode");
        if (has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("readLine() not supported in binary mode");

        try {
            std::string line;
            if (std::getline(pImpl->inFile, line))
                return line;

            throw std::out_of_range("End of file reached"); // EOF reached
        } catch (const std::ios_base::failure& e) {
            throw std::runtime_error(
                "Error reading line from '" + pImpl->path + "': " + e.what()
            );
        }
    }

    inline std::vector<std::string> File::readLines(int numLines) {
        if (!has(pImpl->mode, OpenMode::Read))
            throw std::runtime_error("File not opened in read mode");
        if (has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("readLines() not supported in binary mode");

        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        try {
            for (int i = 0; numLines == 0 || i < numLines; ++i) {
                lines.push_back(readLine()); // readLine() throws std::out_of_range at EOF
            }
        } catch (const std::out_of_range&) {
            // EOF reached, stop reading
        }

        return lines;
    }

    inline void File::writeString(const std::string& data) {
        if (!has(pImpl->mode, OpenMode::Write) &&
            !has(pImpl->mode, OpenMode::Append))
            throw std::runtime_error("File not opened in write/append mode");
        if (has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("writeString() not supported in binary mode");

        try {
            pImpl->outFile << data;
        } catch (const std::ios_base::failure& e) {
            throw std::runtime_error(
                "Error writing text to '" + pImpl->path + "': " + e.what()
            );
        }
    }

    inline void File::writeLine(const std::string& line) {
        if (!has(pImpl->mode, OpenMode::Write) &&
            !has(pImpl->mode, OpenMode::Append))
            throw std::runtime_error("File not opened in write/append mode");
        if (has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("writeLine() not supported in binary mode");

        try {
            pImpl->outFile << line << '\n';
        } catch (const std::ios_base::failure& e) {
            throw std::runtime_error(
                "Error writing line to '" + pImpl->path + "': " + e.what()
            );
        }
    }

    inline void File::writeLines(const std::vector<std::string>& lines) {
        for (const auto& line : lines)
            writeLine(line);
    }

    inline std::vector<std::byte> File::readBytesInternal() {
        if (!has(pImpl->mode, OpenMode::Read))
            throw std::runtime_error("File not opened in read mode");
        if (!has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("readBytes() requires binary mode");

        pImpl->inFile.clear();
        pImpl->inFile.seekg(0, std::ios::beg);

        // Read into a vector<char> first
        std::vector<char> buffer(
            (std::istreambuf_iterator<char>(pImpl->inFile)),
            std::istreambuf_iterator<char>()
        );

        // Convert to vector<std::byte>
        std::vector<std::byte> bytes(buffer.size());
        std::memcpy(bytes.data(), buffer.data(), buffer.size());

        return bytes;
    }

    inline void File::writeBytesInternal(const std::vector<std::byte>& data) {
        if (!has(pImpl->mode, OpenMode::Write) && !has(pImpl->mode, OpenMode::Append))
            throw std::runtime_error("File not opened in write/append mode");
        if (!has(pImpl->mode, OpenMode::Binary))
            throw std::runtime_error("writeBytes() requires binary mode");

        pImpl->outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    inline std::vector<char> File::readBytes() {
        auto bytes = readBytesInternal(); // std::byte
        std::vector<char> result(bytes.size());
        for (size_t i = 0; i < bytes.size(); ++i)
            result[i] = static_cast<char>(bytes[i]);
        return result;
    }

    inline void File::writeBytes(const std::vector<char>& data) {
        std::vector<std::byte> bytes(data.size());
        for (size_t i = 0; i < data.size(); ++i)
            bytes[i] = static_cast<std::byte>(data[i]);
        writeBytesInternal(bytes);
    }

}