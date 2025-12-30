#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include "SimpleFileIO.hpp"
#include <cstdio>

using namespace SimpleFileIO;

void benchmark_simplefileio() {
    const std::string filename = "dummy_test.txt";
    std::string data(10'000'000, 'x'); // 10 MB
    std::vector<char> bytes(data.begin(), data.end());

    std::cout << "--- SimpleFileIO ---\n";

    // writeString
    auto start = std::chrono::high_resolution_clock::now();
    {
        TextWriter f(filename);
        f.writeString(data);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "writeString: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // readString
    start = std::chrono::high_resolution_clock::now();
    {
        TextReader f(filename);
        std::string content = f.readString();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "readString: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // writeBytes
    start = std::chrono::high_resolution_clock::now();
    {
        ByteWriter f(filename);
        f.writeBytes(bytes);
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "writeBytes: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // readBytes
    start = std::chrono::high_resolution_clock::now();
    {
        ByteReader f(filename);
        std::vector<char> content = f.readBytes();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "readBytes: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // writeLines
    std::vector<std::string> lines(1'000'000, "x"); // 1 million lines
    start = std::chrono::high_resolution_clock::now();
    {
        TextWriter f(filename);
        f.writeLines(lines);
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "writeLines: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // readLines
    start = std::chrono::high_resolution_clock::now();
    {
        TextReader f(filename);
        std::vector<std::string> read_lines = f.readLines();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "readLines: " << std::chrono::duration<double>(end-start).count() << " s\n";
}

void benchmark_rawfile() {
    const std::string filename = "dummy_test.txt";
    std::string data(10'000'000, 'x'); // 10 MB
    std::vector<char> bytes(data.begin(), data.end());

    std::cout << "--- Raw FILE* ---\n";

    // writeString (as text)
    auto start = std::chrono::high_resolution_clock::now();
    FILE* f = std::fopen(filename.c_str(), "w");
    static char buf[1 << 20];
    setvbuf(f, buf, _IOFBF, sizeof(buf));
    std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "writeString: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // readString
    start = std::chrono::high_resolution_clock::now();
    f = std::fopen(filename.c_str(), "r");
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<char> buffer(size);
    std::fread(buffer.data(), 1, buffer.size(), f);
    std::fclose(f);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "readString: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // writeBytes
    start = std::chrono::high_resolution_clock::now();
    f = std::fopen(filename.c_str(), "wb");
    setvbuf(f, buf, _IOFBF, sizeof(buf));
    std::fwrite(bytes.data(), 1, bytes.size(), f);
    std::fclose(f);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "writeBytes: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // readBytes
    start = std::chrono::high_resolution_clock::now();
    f = std::fopen(filename.c_str(), "rb");
    std::fseek(f, 0, SEEK_END);
    size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    buffer.resize(size);
    std::fread(buffer.data(), 1, buffer.size(), f);
    std::fclose(f);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "readBytes: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // writeLines
    std::vector<std::string> lines(1'000'000, "x\n");
    start = std::chrono::high_resolution_clock::now();
    f = std::fopen(filename.c_str(), "w");
    setvbuf(f, buf, _IOFBF, sizeof(buf));
    for (auto& line : lines) std::fwrite(line.data(), 1, line.size(), f);
    std::fclose(f);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "writeLines: " << std::chrono::duration<double>(end-start).count() << " s\n";

    // readLines
    start = std::chrono::high_resolution_clock::now();
    f = std::fopen(filename.c_str(), "r");
    std::string line;
    std::vector<std::string> read_lines;
    char c;
    while (std::fread(&c, 1, 1, f) == 1) {
        if (c == '\n') { read_lines.emplace_back(line); line.clear(); }
        else line += c;
    }
    if (!line.empty()) read_lines.emplace_back(line);
    std::fclose(f);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "readLines: " << std::chrono::duration<double>(end-start).count() << " s\n";
}

int main() {
    benchmark_simplefileio();
    benchmark_rawfile();
    return 0;
}