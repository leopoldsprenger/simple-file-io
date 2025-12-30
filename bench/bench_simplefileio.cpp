#include <chrono>
#include <iostream>
#include "SimpleFileIO.hpp"

using namespace SimpleFileIO;

int main() {
    const std::string filename = "bench/dummy_test.log";
    std::string data(10'000'000, 'x'); // 10 MB

    // --- Write benchmark ---
    auto start = std::chrono::high_resolution_clock::now();
    {
        File writer(filename, OpenMode::Write);
        writer.writeString(data);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "[SimpleFileIO] WriteString: "
              << std::chrono::duration<double>(end - start).count()
              << " s\n";

    // --- Read benchmark ---
    start = std::chrono::high_resolution_clock::now();
    {
        File reader(filename, OpenMode::Read);
        std::string content = reader.readString();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "[SimpleFileIO] ReadString: "
              << std::chrono::duration<double>(end - start).count()
              << " s\n";

    return 0;
}