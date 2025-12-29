#include "SimpleFileIO/File.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

using SimpleFileIO::Mode;

namespace SimpleFileIO {

struct File::Impl {
    std::ifstream inFile;
    std::ofstream outFile;
    Mode mode;
    std::string path;

    Impl(const std::string& p, Mode m) : mode(m), path(p) {
        try {
            switch (mode) {
                case Mode::Read:
                    inFile.open(path);
                    inFile.exceptions(std::ios::failbit | std::ios::badbit);
                    break;

                case Mode::Write:
                    outFile.open(path, std::ios::out | std::ios::trunc);
                    outFile.exceptions(std::ios::failbit | std::ios::badbit);
                    break;

                case Mode::Append:
                    outFile.open(path, std::ios::out | std::ios::app);
                    outFile.exceptions(std::ios::failbit | std::ios::badbit);
                    break;

                case Mode::Binary:
                    throw std::logic_error("Binary mode not implemented");
            }
        } catch (const std::ios_base::failure& e) {
            throw std::runtime_error(
                "Failed to open file '" + path + "': " + e.what()
            );
        }
    }
};

File::File(const std::string& path, Mode mode)
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
    if (!pImpl->inFile.is_open())
        throw std::logic_error("File not open for reading");

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
    if (!pImpl->inFile.is_open())
        throw std::logic_error("File not open for reading");

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
    if (!pImpl->inFile.is_open())
        throw std::logic_error("File not open for reading");

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
    if (!pImpl->outFile.is_open())
        throw std::logic_error("File not open for writing");

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
    if (!pImpl->outFile.is_open())
        throw std::logic_error("File not open for writing");

    try {
        pImpl->outFile << line << '\n';
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error writing line to '" + pImpl->path + "': " + e.what()
        );
    }
}

void File::writeLines(const std::vector<std::string>& lines) {
    if (!pImpl->outFile.is_open())
        throw std::logic_error("File not open for writing");

    try {
        for (const auto& line : lines)
            pImpl->outFile << line << '\n';
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "Error writing lines to '" + pImpl->path + "': " + e.what()
        );
    }
}

}