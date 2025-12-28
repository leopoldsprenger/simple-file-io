#include "SimpleFileIO/File.h"
#include <fstream>
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

}