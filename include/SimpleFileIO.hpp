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
        FILE* file = nullptr;
        std::string path;
        OpenMode mode;

        bool canRead = false;
        bool canWrite = false;
        bool canAppend = false;
        bool isBinary = false;

        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline File::File(const std::string& p, OpenMode m)
        : path(p), mode(m)
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

        // Allocate per-file 1 MB buffer
        buffer.resize(1 << 20);
        if (std::setvbuf(file, buffer.data(), _IOFBF, buffer.size()) != 0) {
            throw std::runtime_error("Failed to set file buffer for '" + path + "'");
        }
    }

    inline File::~File() {
        if (!file) return;
        if (canWrite || canAppend) std::fflush(file);
        std::fclose(file);
    }

    inline bool File::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline bool File::isOpen() const {
        return file != nullptr;
    }

    inline void File::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline std::string File::readString() {
        if (!canRead)
            throw std::runtime_error("File not opened in read mode");
        if (isBinary)
            throw std::runtime_error("readString() not supported in binary mode");

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

    inline std::string File::readLine() {
        if (!canRead)
            throw std::runtime_error("File not opened in read mode");
        if (isBinary)
            throw std::runtime_error("readLine() not supported in binary mode");

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

    inline std::vector<std::string> File::readLines(int numLines) {
        if (!canRead)
            throw std::runtime_error("File not opened in read mode");
        if (isBinary)
            throw std::runtime_error("readLines() not supported in binary mode");

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

    inline void File::writeString(const std::string& data) {
        if (!canWrite && !canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (isBinary)
            throw std::runtime_error("writeString() not supported in binary mode");

        size_t written = std::fwrite(data.data(), 1, data.size(), file);
        if (written != data.size())
            throw std::runtime_error("Failed to write full string to '" + path + "'");
    }

    inline void File::writeLine(const std::string& line) {
        writeString(line + '\n');
    }

    inline void File::writeLines(const std::vector<std::string>& lines) {
        std::string all;
        for (const auto& line : lines) {
            all += line;
            all += '\n';
        }
        writeString(all);
    }

    inline std::vector<char> File::readBytes() {
        if (!canRead)
            throw std::runtime_error("File not opened in read mode");
        if (!isBinary)
            throw std::runtime_error("readBytes() requires binary mode");

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

    inline void File::writeBytes(const std::vector<char>& data) {
        if (!canWrite && !canAppend)
            throw std::runtime_error("File not opened in write/append mode");
        if (!isBinary)
            throw std::runtime_error("writeBytes() requires binary mode");

        size_t written = std::fwrite(data.data(), 1, data.size(), file);
        if (written != data.size())
            throw std::runtime_error("Failed to write full data to '" + path + "'");
    }
}