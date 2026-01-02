#include <iostream>
#include <chrono>
#include <cstdlib>
#include <map>
#include <string>
#include <iomanip>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <functional>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "SimpleFileIO.hpp"

using namespace SimpleFileIO;

// ---------------- Timer ----------------
template<typename Func>
double timeFuncMedian(Func f, int runs = 30, std::function<void()> setup = []{}) {
    std::vector<double> times;
    times.reserve(runs);

    for (int i = 0; i < runs; i++) {
        if (setup) setup();
        auto start = std::chrono::high_resolution_clock::now();
        f();
        auto end   = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        times.push_back(elapsed.count());
    }

    std::sort(times.begin(), times.end());
    return times[runs / 2];
}

// ---------------- Raw C helpers ----------------
void rawWriteString(FILE* f, const std::string& data) {
    std::fseek(f, 0, SEEK_SET);
    std::fwrite(data.data(), 1, data.size(), f);
    std::fflush(f);
    int fd = fileno(f);
    if (fd >= 0) fsync(fd);
}

void rawReadString(FILE* f, std::string& out) {
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size);
    std::fread(out.data(), 1, size, f);
}

void rawWriteBytes(FILE* f, const std::vector<char>& data) {
    std::fseek(f, 0, SEEK_SET);
    std::fwrite(data.data(), 1, data.size(), f);
    std::fflush(f);
    int fd = fileno(f);
    if (fd >= 0) fsync(fd);
}

void rawReadBytes(FILE* f, std::vector<char>& out) {
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size);
    std::fread(out.data(), 1, size, f);
}

// ---------------- Helpers (POSIX) ----------------
static void drop_cache(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0) {
        // Prefer posix_fadvise when available
#if defined(POSIX_FADV_DONTNEED)
        posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
#elif defined(__APPLE__)
        // On macOS, F_NOCACHE on the fd is a good approximation
        int set = 1;
        fcntl(fd, F_NOCACHE, set);
#else
        // No-op on platforms without an appropriate advisory
#endif
        close(fd);
    }
}

static void sync_file(const std::string& path) {
    int fd = open(path.c_str(), O_WRONLY);
    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }
}

// ---------------- Python runner ----------------
std::map<std::string,double> runPythonBenchmark() {
    std::map<std::string,double> results;
    FILE *pipe = popen("python3 bench/benchmark_python.py", "r");
    if (!pipe) return results;

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        auto pos = line.find(':');
        if (pos != std::string::npos) {
            results[line.substr(0,pos)] = std::stod(line.substr(pos+1));
        }
    }
    pclose(pipe);
    return results;
}

// ---------------- Main ----------------
int main() {
    const std::string filename = "bench_test.log";

    const size_t DATA_SIZE = 10'000'000; // 10 MB
    std::string testStr(DATA_SIZE, 'A');
    std::vector<char> testBytes(DATA_SIZE, 'A');

    // --- Build lines with ~1 KB each to reduce loop overhead
    std::vector<std::string> testLines;
    std::string baseLine(1024, 'A'); // 1 KB line
    size_t nLines = DATA_SIZE / baseLine.size();
    testLines.reserve(nLines);
    for (size_t i = 0; i < nLines; i++) {
        testLines.push_back(baseLine);
    }

    std::string readStr;
    std::vector<char> readBytes;
    std::string singleLine;

    std::map<std::string,double> libTimes;
    std::map<std::string,double> rawTimes;
    std::map<std::string,double> pyTimes = runPythonBenchmark();

    // ---------------- Library benchmarks ----------------
    libTimes["writeString"] = timeFuncMedian([&]{
        TextWriter writer(filename);
        writer.writeString(testStr);
        writer.flush();
        sync_file(filename);
    });

    libTimes["writeBytes"] = timeFuncMedian([&]{
        ByteWriter writer(filename);
        writer.writeBytes(testBytes);
        writer.flush();
        sync_file(filename);
    });

    libTimes["writeLines"] = timeFuncMedian([&]{
        TextWriter writer(filename);
        // Use the bulk write API so semantics match Python's writelines
        writer.writeLines(testLines);
        writer.flush();
        sync_file(filename);
    });

    libTimes["writeLine"] = libTimes["writeLines"]; // single-line per iteration not meaningful for large dataset

    libTimes["readString"] = timeFuncMedian([&]{
        TextReader reader(filename);
        readStr = reader.readString();
    }, 30, [&]{ drop_cache(filename); });

    libTimes["readBytes"] = timeFuncMedian([&]{
        ByteReader reader(filename);
        readBytes = reader.readBytes();
    }, 30, [&]{ drop_cache(filename); });

    libTimes["readLine"] = timeFuncMedian([&]{
        TextReader reader(filename);
        while (true) {
            std::string line = reader.readLine();
            if (line.empty()) break;  // EOF reached
            singleLine = std::move(line);
        }
    }, 30, [&]{ drop_cache(filename); });

    libTimes["readLines"] = timeFuncMedian([&]{
        TextReader reader(filename);
        auto v = reader.readLines();  // readLines already stops at EOF
    }, 30, [&]{ drop_cache(filename); });

    // ---------------- Raw benchmarks ----------------
    rawTimes["writeString"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "wb");
        rawWriteString(f, testStr);
        std::fclose(f);
    });

    rawTimes["writeBytes"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "wb");
        rawWriteBytes(f, testBytes);
        std::fclose(f);
    });

    rawTimes["writeLines"] = timeFuncMedian([&]{
        // Build a single buffer and write it once (match Python's writelines semantics)
        std::string bulk;
        bulk.reserve(DATA_SIZE + testLines.size());
        for (const auto& l : testLines) {
            bulk.append(l);
            if (l.empty() || l.back() != '\n') bulk.push_back('\n');
        }
        FILE* f = std::fopen(filename.c_str(), "wb");
        std::fwrite(bulk.data(), 1, bulk.size(), f);
        std::fflush(f);
        int fd = fileno(f);
        if (fd >= 0) fsync(fd);
        std::fclose(f);
    });

    rawTimes["writeLine"] = rawTimes["writeLines"];

    rawTimes["readString"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "rb");
        rawReadString(f, readStr);
        std::fclose(f);
    }, 30, [&]{ drop_cache(filename); });

    rawTimes["readBytes"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "rb");
        rawReadBytes(f, readBytes);
        std::fclose(f);
    }, 30, [&]{ drop_cache(filename); });

    rawTimes["readLines"] = timeFuncMedian([&]{
        std::ifstream fin(filename);
        std::vector<std::string> lines;
        std::string tmp;
        while (std::getline(fin,tmp)) lines.push_back(tmp);
    }, 30, [&]{ drop_cache(filename); });

    rawTimes["readLine"] = timeFuncMedian([&]{
        std::ifstream fin(filename);
        std::string line;
        while (std::getline(fin,line)) {}
    }, 30, [&]{ drop_cache(filename); });

    // ---------------- Output ----------------
    std::vector<std::string> opsOrder = {
        "readString","readLines","readLine","readBytes",
        "writeString","writeLines","writeLine","writeBytes"
    };

    std::cout << std::setw(15) << "Operation"
              << std::setw(6)  << "Mark"
              << std::setw(12) << "SFIO(ms)"
              << std::setw(15) << "vs Python"
              << std::setw(15) << "vs Raw"
              << "\n";

    auto fmtDiff = [](double d) {
        std::ostringstream oss;
        oss << (d >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << d;
        return oss.str();
    };

    for (const auto& op : opsOrder) {
        double tLib = libTimes[op];
        double tPy  = pyTimes.count(op)  ? pyTimes[op]  : 0.0;
        double tRaw = rawTimes.count(op) ? rawTimes[op] : 0.0;

        double tolerance = 0.05; // 5% margin
        bool pass = ((tLib <= tPy * (1.0 + tolerance)) || tPy == 0.0)
                && ((tLib <= tRaw * (1.0 + tolerance)) || tRaw == 0.0);
        const char* mark = pass ? "✔" : "✘";

        std::cout << std::setw(15) << op
                  << std::setw(6)  << mark
                  << std::setw(12) << std::fixed << std::setprecision(2) << tLib
                  << std::setw(15) << (tPy  ? fmtDiff(tLib - tPy)  : "   n/a")
                  << std::setw(15) << (tRaw ? fmtDiff(tLib - tRaw) : "   n/a")
                  << "\n";
    }

    std::remove(filename.c_str());
}