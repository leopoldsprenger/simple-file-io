# SimpleFileIO

![C++](https://img.shields.io/badge/C%2B%2B-23-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

Ultra-fast, safe, and Pythonic file I/O for C++17+, with zero dependencies and benchmarks to prove it.


---

## Index

1. [Overview](#overview)  
2. [Design Goals](#design-goals)  
3. [Scope & Limitations](#scope--limitations)  
4. [Usage Examples](#usage-examples)  
5. [Integration & Build](#integration--build)  
6. [Benchmarking](#benchmarking)
7. [License](#license)  

---

## Overview

`SimpleFileIO` is a lightweight, modern, header-only C++ library that abstracts common file operations into a clean, safe, and easy-to-use interface. Its API is **inspired by Python’s intuitive file handling semantics** (e.g., `file = open(...)`), combining readability with modern C++ safety and performance. It targets developers who need reliable file handling without the boilerplate of standard C++ streams.  

Key features:

- **Pythonic, intuitive API** for natural, minimal syntax while preserving C++ type safety.  
- **Modern C++17+ APIs**: RAII-based resource management and exception-safe operations.  
- **Comprehensive File Operations**: Read/write single lines, multiple lines, entire file contents, or binary data.  
- **File Validation**: Built-in checks for file existence and open state.  
- **Cross-platform Compatibility**: Tested on macOS, Linux, and Windows.  
- **Header-only**: Zero dependencies, minimal integration overhead.  

---

## Design Goals

- Provide a **Python-inspired, minimal, and intuitive interface** for common file operations.  
- Ensure robust exception handling for safer I/O workflows.  
- Reduce boilerplate for frequent tasks such as reading/writing lines or entire files.  
- Maintain a header-only architecture for seamless project integration.  

---

## Scope & Limitations

**Out of scope**:

- Advanced filesystem operations (`<filesystem>` functionality, directory traversal).  
- Complex serialization or custom file formats.  
- Asynchronous or multithreaded I/O (currently synchronous only).  
- Replacement for full-featured I/O libraries like Boost.Filesystem.  

---

## Lifetime, Ownership & Errors

- Each **Reader/Writer** object **owns its underlying file stream** (RAII).
- Files are opened on construction and **automatically closed on destruction**.
- No manual cleanup is required; lifetime is scope-bound and deterministic.
- File operation failures are reported via **exceptions** (no silent errors).
- Reader/Writer instances are **not thread-safe**; concurrent access must be externally synchronized.

## Usage Examples

### Include the library
```cpp
#include "SimpleFileIO.hpp"
using namespace SimpleFileIO;
```

### Writing a single line
```cpp
TextWriter writer("example.txt");
writer.writeLine("Hello, SimpleFileIO!");
```

### Writing multiple lines
```cpp
TextWriter writerMulti("example.txt");
writerMulti.writeLines({"Line 1", "Line 2", "Line 3"});
```

### Reading entire file content
```cpp
TextReader reader("example.txt");
std::string content = reader.readString();
std::cout << content << "\n";
```

### Reading line by line
```cpp
TextReader readerLines("example.txt");
std::vector<std::string> lines = readerLines.readLines();
for (auto& line : lines) {
    std::cout << line << '\n';
}
```

### Appending to an existing file
```cpp
TextWriter appender("example.txt", true);
appender.writeLine("This line is appended");
```

### Binary mode example
```cpp
ByteWriter binaryFile("data.bin");
std::vector<char> binaryData = {0x01, 0x02, 0x03, (char)0xFF};
binaryFile.writeBytes(binaryData);

ByteReader binaryReader("data.bin");
std::vector<char> loadedData = binaryReader.readBytes();
```
---

## Integration & Build

Being **header-only**, `SimpleFileIO` requires no separate build step.  

**Steps to integrate**:

1. Copy `include/SimpleFileIO.hpp` into your project include path.  
2. Include it in your source files:
```cpp
#include "SimpleFileIO.hpp"
```
3. Compile with a C++17+ compiler:
```bash
g++ -std=c++23 main.cpp -o main
./main
```
**Optional CMake Integration**:
```cmake
cmake_minimum_required(VERSION 3.10)
project(DummyProject)

set(CMAKE_CXX_STANDARD 23)

include_directories(path/to/SimpleFileIO/include)

add_executable(dummy main.cpp)
```
---

## Benchmarking

Benchmarking shows that SimpleFileIO is faster than Python for many common file operations and nearly matches the speed of **Raw File*** (C `FILE*`-based I/O). It achieves this while remaining as easy to use as Python and requiring **no manual checks** thanks to its class-based, RAII API.

**Results (median timings / diffs):**

| Operation  | SFIO (ms) | vs Python | vs Raw File* |
|-----------:|----------:|---------:|-------------:|
| readString  | ✔ 0.00 | -3.57  | -0.73  |
| readLines   | ✔ 0.02 | -43.15 | -25.08 |
| readLine    | ✔ 0.02 | -0.02  | -24.69 |
| readBytes   | ✔ 0.02 | -2.42  | -0.71  |
| writeString | ✘ 0.81 | -2.43  | +0.06  |
| writeLines  | ✔ 8.11 | -42.19 | -13.38 |
| writeLine   | ✘ 0.03 | -0.09  | +0.01  |
| writeBytes  | ✘ 0.80 | -0.75  | +0.06  |

> **Notes:**
> - Negative numbers (e.g., -3.57) indicate that SimpleFileIO is faster than the comparison baseline (Python or Raw File*) by that many milliseconds.  
> - Small positive numbers (e.g., +0.01) mean SFIO is marginally slower than the baseline, which is negligible and mostly due to OS-level caching behavior rather than library inefficiency.  
> - “vs Raw File*” compares SFIO to raw FILE* I/O (fopen/fread/fwrite).  
> - Benchmarks were collected using the `bench/benchmark_all.cpp` and `bench/benchmark_python.py` scripts (median of multiple runs).

---

## License

This project is licensed under the **MIT License**. See the LICENSE file for full terms.
