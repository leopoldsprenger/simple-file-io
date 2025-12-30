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
#include "SimpleFileIO.hpp"

using namespace SimpleFileIO;

// ---------------- Timer ----------------
template<typename Func>
double timeFuncMedian(Func f, int runs = 50) {
    std::vector<double> times;
    times.reserve(runs);

    // warm-up
    f();

    for (int i = 0; i < runs; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        f();
        auto end   = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        times.push_back(elapsed.count());
    }

    std::sort(times.begin(), times.end());
    return times[runs / 2];
}

// ---------------- Raw C helpers (steady-state) ----------------
void rawWriteString(FILE* f, const std::string& data) {
    std::fseek(f, 0, SEEK_SET);
    std::fwrite(data.data(), 1, data.size(), f);
    std::fflush(f);
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
}

void rawReadBytes(FILE* f, std::vector<char>& out) {
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size);
    std::fread(out.data(), 1, size, f);
}

// ---------------- Python runner ----------------
std::map<std::string,double> runPythonBenchmark() {
    std::map<std::string,double> results;
    FILE *pipe = popen("python3 benchmark_python.py", "r");
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

    std::string testStr(10'000'000, 'A');              // 10 MB
    std::vector<char> testBytes(10'000'000, 'A');
    std::vector<std::string> testLines;
    for (int i = 0; i < 1'000'000; i++)
        testLines.push_back("This is a line " + std::to_string(i) + "\n");

    std::string readStr;
    std::vector<char> readBytes;
    std::string singleLine;

    std::map<std::string,double> libTimes;
    std::map<std::string,double> rawTimes;
    std::map<std::string,double> pyTimes = runPythonBenchmark();

    // ---------------- Library objects (opened once) ----------------
    TextReader  tr(filename);
    TextWriter  tw(filename);
    ByteReader  br(filename);
    ByteWriter  bw(filename);

    // ---------------- Raw FILE* (opened once) ----------------
    FILE* fr = std::fopen(filename.c_str(), "rb");
    FILE* fw = std::fopen(filename.c_str(), "wb");

    // ---------------- Benchmarks ----------------
    libTimes["readLines"] = timeFuncMedian([&]{
        TextReader reader(filename);
        auto v = reader.readLines(); // reads entire file
    });

    libTimes["readLine"] = timeFuncMedian([&]{
        TextReader reader(filename);
        while (true) {
            try {
                singleLine = reader.readLine();
            } catch (const std::out_of_range&) {
                break; // EOF
            }
        }
    });

    libTimes["readBytes"] = timeFuncMedian([&]{
        ByteReader reader(filename);
        readBytes = reader.readBytes();
    });

    libTimes["writeString"] = timeFuncMedian([&]{
        TextWriter writer(filename);
        writer.writeString(testStr);
        writer.flush();
    });

    libTimes["writeLines"] = timeFuncMedian([&]{
        TextWriter writer(filename);
        writer.writeLines(testLines);
        writer.flush();
    });

    libTimes["writeLine"] = timeFuncMedian([&]{
        TextWriter writer(filename);
        writer.writeLine("Single line test");
        writer.flush();
    });

    libTimes["writeBytes"] = timeFuncMedian([&]{
        ByteWriter writer(filename);
        writer.writeBytes(testBytes);
        writer.flush();
    });

    // --- Raw benchmarks (all opens inside lambda) ---
    rawTimes["readLines"] = timeFuncMedian([&]{
        std::ifstream fin(filename);
        std::vector<std::string> lines;
        std::string tmp;
        while (std::getline(fin,tmp)) lines.push_back(tmp+"\n");
    });

    rawTimes["readLine"] = timeFuncMedian([&]{
        std::ifstream fin(filename);
        std::string line;
        while (std::getline(fin,line)) {}
    });

    rawTimes["readString"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "rb");
        rawReadString(f, readStr);
        std::fclose(f);
    });

    rawTimes["readBytes"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "rb");
        rawReadBytes(f, readBytes);
        std::fclose(f);
    });

    rawTimes["writeString"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "wb");
        rawWriteString(f, testStr);
        std::fclose(f);
    });

    rawTimes["writeLines"] = timeFuncMedian([&]{
        std::ofstream fout(filename);
        for (const auto &l : testLines) fout << l;
    });

    rawTimes["writeLine"] = timeFuncMedian([&]{
        std::ofstream fout(filename);
        fout << "Single line test\n";
    });

    rawTimes["writeBytes"] = timeFuncMedian([&]{
        FILE* f = std::fopen(filename.c_str(), "wb");
        rawWriteBytes(f, testBytes);
        std::fclose(f);
    });

    std::vector<std::string> opsOrder = {
        "readString","readLines","readLine","readBytes",
        "writeString","writeLines","writeLine","writeBytes"
    };

    // ---------------- Output ----------------
    std::cout << std::setw(15) << "Operation"
              << std::setw(6)  << ""
              << std::setw(12) << "SFIO (ms)"
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

        bool pass = (tLib <= tPy || tPy == 0.0) && (tLib <= tRaw || tRaw == 0.0);
        const char* mark = pass ? "✔" : "✘";

        std::cout << std::setw(15) << op
                  << std::setw(6)  << mark
                  << std::setw(12) << std::fixed << std::setprecision(2) << tLib
                  << std::setw(15) << (tPy  ? fmtDiff(tLib - tPy)  : "   n/a")
                  << std::setw(15) << (tRaw ? fmtDiff(tLib - tRaw) : "   n/a")
                  << "\n";
    }

    std::fclose(fr);
    std::fclose(fw);
}