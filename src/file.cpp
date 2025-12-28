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

    Impl(const std::string& p, Mode m) : path(p), mode(m) {
        // Open the file according to mode
        switch (mode) {
            case Mode::Read:
                inFile.open(path);
                if (!inFile.is_open()) throw std::runtime_error("Cannot open file for reading");
                break;
            case Mode::Write:
                outFile.open(path, std::ios::trunc);
                if (!outFile.is_open()) throw std::runtime_error("Cannot open file for writing");
                break;
            case Mode::Append:
                outFile.open(path, std::ios::app);
                if (!outFile.is_open()) throw std::runtime_error("Cannot open file for appending");
                break;
            case Mode::Binary:
                // Handle binary mode later
                break;
        }
    }

    ~Impl() {
        if (inFile.is_open()) inFile.close();
        if (outFile.is_open()) outFile.close();
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
    if (!pImpl->inFile.is_open()) {
        throw std::runtime_error("File not open for reading");
    }
    
    try {
        std::string data((std::istreambuf_iterator<char>(pImpl->inFile)),
                         std::istreambuf_iterator<char>());
        return data;
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("Error reading file: " + std::string(e.what()));
    }
}

std::string File::readLine() {
    if (!pImpl->inFile.is_open()) {
        throw std::runtime_error("File not open for reading");
    }

    std::string line;
    try {
        if (std::getline(pImpl->inFile, line)) {
            return line;
        } else if (pImpl->inFile.eof()) {
            pImpl->inFile.clear();
            throw std::runtime_error("End of file reached");
        } else {
            throw std::runtime_error("Error reading line from file");
        }
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("File read failure: " + std::string(e.what()));
    }
}

std::vector<std::string> File::readLines(int numLines) {
    if (!pImpl->inFile.is_open()) {
        throw std::runtime_error("File not open for reading");
    }

    std::vector<std::string> lines;
    std::string line;

    if (numLines > 0) lines.reserve(numLines);

    try {
        int count = 0;
        while (std::getline(pImpl->inFile, line)) {
            lines.push_back(line);
            if (numLines > 0 && ++count >= numLines) break;
        }
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("Error reading lines: " + std::string(e.what()));
    }

    pImpl->inFile.clear();
    pImpl->inFile.seekg(0);

    return lines;
}

void File::writeAll(const std::string& data) {
    if (!pImpl->outFile.is_open()) {
        throw std::runtime_error("File not open for writing");
    }

    try {
        pImpl->outFile << data;
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("Error writing file: " + std::string(e.what()));
    }
}

void File::writeLine(const std::string& line) {
    if (!pImpl->outFile.is_open()) {
        throw std::runtime_error("File not open for writing");
    }

    try {
        pImpl->outFile << line << '\n';
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("Error writing line to file: " + std::string(e.what()));
    }
}

void File::writeLines(const std::vector<std::string>& lines) {
    if (!pImpl->outFile.is_open()) {
        throw std::runtime_error("File not open for writing");
    }

    try {
        std::ostringstream buffer;
        for (const auto& line : lines) buffer << line << '\n';
        pImpl->outFile << buffer.str();
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("Error writing lines: " + std::string(e.what()));
    }
}

}