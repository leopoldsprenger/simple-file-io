#include "SimpleFileIO/File.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

using SimpleFileIO::OpenMode;

namespace {
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
}

namespace SimpleFileIO {

struct File::Impl {
    std::ifstream inFile;
    std::ofstream outFile;
    OpenMode mode;
    std::string path;

    Impl(const std::string& p, OpenMode m) : mode(m), path(p) {
        try {
            if (mode == OpenMode::None) {
                throw std::logic_error("No mode specified");
            }

            int access_count = 
                (has(mode, OpenMode::Read) ? 1 : 0) +
                (has(mode, OpenMode::Write) ? 1 : 0) +
                (has(mode, OpenMode::Append) ? 1 : 0); 
            
            if (access_count != 1) {
                throw std::logic_error("Exactly one of Read, Write, or Append must be set");
            }

            std::ios::openmode flags = std::ios::openmode{};
            
            if (has(mode, OpenMode::Read))  flags |= std::ios::in;
            if (has(mode, OpenMode::Write)) flags |= std::ios::out | std::ios::trunc;
            if (has(mode, OpenMode::Append))  flags |= std::ios::out | std::ios::app;
            if (has(mode, OpenMode::Binary))  flags |= std::ios::binary;

            if (has(mode, OpenMode::Read)) {
                inFile.open(path, flags);
                inFile.exceptions(std::ios::failbit | std::ios::badbit);
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

File::File(const std::string& path, OpenMode mode)
    : pImpl(new Impl(path, mode)) {}

File::~File() {
    delete pImpl;
}

bool File::exists(const std::string& filename) {
    std::ifstream file(filename);
    return file.is_open();
}

bool File::isOpen() const {
    return pImpl->inFile.is_open() || pImpl->outFile.is_open();
}

std::string File::readAll() {
    if (!has(pImpl->mode, OpenMode::Read))
        throw std::runtime_error("File not opened in read mode");
    
    try {
        std::ostringstream buffer;
        buffer << pImpl->inFile.rdbuf();
        return buffer.str();
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error reading file '" + pImpl->path + "': " + e.what()
        );
    }
}

std::string File::readLine() {
    if (!has(pImpl->mode, OpenMode::Read))
        throw std::runtime_error("File not opened in read mode");
    if (has(pImpl->mode, OpenMode::Binary))
        throw std::runtime_error("readLine() not supported in binary mode");

    try {
        std::string line;
        if (std::getline(pImpl->inFile, line))
            return line;

        throw std::out_of_range("End of file reached");
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error reading line from '" + pImpl->path + "': " + e.what()
        );
    }
}

std::vector<std::string> File::readLines(int numLines) {
    if (!has(pImpl->mode, OpenMode::Read))
        throw std::runtime_error("File not opened in read mode");

    std::vector<std::string> lines;
    if (numLines > 0) lines.reserve(numLines);

    try {
        std::string line;
        int count = 0;

        while (std::getline(pImpl->inFile, line)) {
            lines.push_back(line);
            if (numLines > 0 && ++count >= numLines)
                break;
        }

        return lines;
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error reading lines from '" + pImpl->path + "': " + e.what()
        );
    }
}

void File::writeAll(const std::string& data) {
    if (!has(pImpl->mode, OpenMode::Write) && !has(pImpl->mode, OpenMode::Append))
        throw std::runtime_error("File not open in write mode or append mode");

    try {
        pImpl->outFile << data;
        pImpl->outFile.flush();
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error writing to '" + pImpl->path + "': " + e.what()
        );
    }
}

void File::writeLine(const std::string& line) {
    if (!has(pImpl->mode, OpenMode::Write) && !has(pImpl->mode, OpenMode::Append))
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

void File::writeLines(const std::vector<std::string>& lines) {
    if (!has(pImpl->mode, OpenMode::Write) && !has(pImpl->mode, OpenMode::Append))
        throw std::runtime_error("File not opened in write/append mode");

    try {
        for (const auto& line : lines)
            pImpl->outFile << line << '\n';
        pImpl->outFile.flush();
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error writing lines to '" + pImpl->path + "': " + e.what()
        );
    }
}

}