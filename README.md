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

Benchmarking shows that SimpleFileIO is faster than Python for many common file operations and closely matches the speed of **Raw File*** (C `FILE*`-based I/O) for the tested workloads. The benchmarks are performed using durable writes (fsync) and explicit cache-dropping for reads to ensure an apples-to-apples comparison.

**Results (median timings / diffs):**

| Operation  | Mark | SFIO (ms) | vs Python | vs Raw File* |
|-----------:|:----:|----------:|---------:|-------------:|
| readString  | ✘ | 1.31 | -0.57 | +0.44 |
| readLines   | ✘ | 4.16 | +0.83 | -18.16 |
| readLine    | ✔ | 3.66 | -0.34 | -18.25 |
| readBytes   | ✘ | 1.30 | +0.11 | +0.50 |
| writeString | ✔ | 3.24 | -0.04 | +0.13 |
| writeLines  | ✔ | 3.69 | -3.46 | +0.07 |
| writeLine   | ✔ | 3.69 | -3.40 | +0.07 |
| writeBytes  | ✔ | 3.17 | +0.02 | +0.00 |

> **Notes:**
> - Negative numbers (e.g., -0.57) indicate that SimpleFileIO is faster than the comparison baseline (Python or Raw File*) by that many milliseconds.  
> - Small positive numbers (e.g., +0.07) mean SFIO is marginally slower than the baseline; differences under a millisecond are typically within system noise.  
> - “Mark” denotes whether the library was at least as fast as both Python and the raw FILE* implementation for that operation (✔) or not (✘).  
> - Benchmarks were collected using the `bench/benchmark_all.cpp` and `bench/benchmark_python.py` scripts (median of multiple runs).  

**Test details:**

- **Dataset:** 10 MB total per test. For line-based operations each line is ~1 KB (≈10,000 lines).
- **Runs & aggregation:** Each operation is executed **30 times** and the **median** timing is reported.
- **Read behavior:** For `readString`, `readBytes`, `readLines`, and the repeated `readLine` loop, each timed iteration reads the entire 10 MB file; caches are dropped before each read run to force disk I/O.
- **Write behavior:** Each write iteration is flushed and followed by `fsync` (so timings include durability costs and match Python's `os.fsync`).
- **Reproducibility:** For consistent results run the benchmark on an otherwise idle machine and repeat the whole suite to estimate variance.

---

## License

This project is licensed under the **MIT License**. See the LICENSE file for full terms.
