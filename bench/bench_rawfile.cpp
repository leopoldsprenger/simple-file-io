#include <chrono>
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>

int main() {
    const std::string filename = "bench/dummy_test.log";
    std::string data(10'000'000, 'x'); // 10 MB

    // --- Write benchmark ---
    auto start = std::chrono::high_resolution_clock::now();
    FILE* f = std::fopen(filename.c_str(), "w");
    if (!f) { perror("fopen"); return 1; }

    // Set large buffer
    static char buf[1 << 20];
    setvbuf(f, buf, _IOFBF, sizeof(buf));

    size_t written = std::fwrite(data.data(), 1, data.size(), f);
    if (written != data.size()) { perror("fwrite"); return 1; }
    std::fclose(f);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "[Raw FILE*] WriteAll: "
              << std::chrono::duration<double>(end - start).count()
              << " s\n";

    // --- Read benchmark ---
    start = std::chrono::high_resolution_clock::now();
    f = std::fopen(filename.c_str(), "r");
    if (!f) { perror("fopen"); return 1; }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    std::vector<char> buffer(size);
    size_t readBytes = std::fread(buffer.data(), 1, buffer.size(), f);
    if (readBytes != buffer.size()) { perror("fread"); return 1; }
    std::fclose(f);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "[Raw FILE*] ReadAll: "
              << std::chrono::duration<double>(end - start).count()
              << " s\n";

    return 0;
}