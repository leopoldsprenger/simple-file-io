#pragma once

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <algorithm>

/**
 * @defgroup Core Core Utilities
 * @brief Error handling and shared utilities.
 */

/**
 * @defgroup TextIO Text File I/O
 * @brief High-performance text file readers and writers.
 */

/**
 * @defgroup BinaryIO Binary File I/O
 * @brief High-performance binary file readers and writers.
 */

/**
 * @namespace SimpleFileIO
 * @brief Lightweight, fast, cross-platform file I/O utilities.
 *
 * @ingroup Core
 *
 * Provides optimized text and binary readers/writers with explicit buffering,
 * portable fast I/O wrappers, and structured error handling via exceptions.
 *
 * @note All classes use manual 1 MB buffers to reduce syscall overhead.
 * @warning These utilities are not thread-safe on the same file instance.
 */
namespace SimpleFileIO {
    // Wrapper macros for portable fast I/O
    #if defined(__linux__)
    #define SFIO_FWRITE fwrite_unlocked
    #define SFIO_FREAD  fread_unlocked
    #else
    #define SFIO_FWRITE std::fwrite
    #define SFIO_FREAD  std::fread
    #endif

    /**
     * @ingroup Core
     * @enum IOError
     * @brief Enumerates all supported I/O error categories.
     */
    enum class IOError {
        FileNotOpen,
        FileNotFound,
        PermissionDenied,
        ReadError,
        WriteError
    };

    
    /**
     * @ingroup Core
     * @class IOException
     * @brief Exception type thrown for all file I/O failures.
     *
     * Wraps a high-level error code, optional path information,
     * and a human-readable message.
     */
    class IOException : public std::runtime_error {
    public:
        IOError code;
        std::string detail;
        std::string path;

        
        /**
         * @param code    Error category
         * @param message Human-readable error description
         * @param path    Optional file path
         */
        IOException(IOError code,
                    std::string message,
                    std::string path = "")
            : std::runtime_error(std::move(message)),
            code(code),
            path(std::move(path)) {}
    };


    /**
     * @ingroup Core
     * @brief Converts an IOError into a readable error message.
     *
     * @param code   Error category
     * @param path   Optional file path related to the error
     * @param detail Optional low-level detail string
     * @return Formatted error message
     */
    inline std::string formatIOError(IOError code,
                                    const std::string& path = "",
                                    const std::string& detail = "") {
        switch (code) {
            case IOError::FileNotOpen:
                return "File operation failed 'file is not open': " + path;
            case IOError::FileNotFound:
                return "File not found: " + path;
            case IOError::PermissionDenied:
                return "Permission denied while accessing: " + path;
            case IOError::ReadError:
                return "Low-level read error" + 
                        (detail.empty() ? "" : (": " + detail));
            case IOError::WriteError:
                return "Low-level write error" + 
                        (detail.empty() ? "" : (": " + detail));
            default:
                return "Unknown I/O error.";
        }
    }

    /**
     * @ingroup TextIO
     * @class TextReader
     * @brief High-performance buffered text file reader.
     *
     * Optimized for sequential access using a manual 1 MB buffer.
     *
     * @note Newlines are normalized as-is; no CRLF conversion is performed.
     */
    class TextReader {
    public:
        /**
         * @brief Opens a text file for reading.
         * @param path Path to the file
         * @throws IOException if the file cannot be opened
         */
        inline TextReader(const std::string& path);
        
        /**
         * @brief Closes the file and releases resources.
         */
        inline ~TextReader();

        /**
         * @brief Checks whether a file exists.
         * @param path Path to the file
         * @return True if the file exists
         */
        inline static bool exists(const std::string& path);

        /**
         * @brief Reads the entire file into a string.
         *
         * @return String containing all file contents
         *
         * @throws IOException on read failure
         *
         * @complexity Time: O(n)  
         * @complexity Space: O(n)
         */
        inline std::string readString();
        
        /**
         * @brief Reads a single line from the file.
         *
         * The returned string does not include the trailing newline.
         *
         * @return The next line, or an empty string on EOF
         *
         * @throws IOException on read failure
         *
         * @complexity Amortized O(k), where k is line length
         */
        inline std::string readLine();

        /**
         * @brief Reads multiple lines from the file.
         *
         * @param numLines Maximum number of lines to read.
         *                 If zero, reads until EOF.
         * @return Vector containing the read lines
         *
         * @throws IOException on read failure
         *
         * @complexity Time: O(n)  
         * @complexity Space: O(n)
         */
        inline std::vector<std::string> readLines(int numLines = 0);

    private:
        FILE* file = nullptr;
        std::string path;

        std::vector<char> buffer; // 1MB buffer
        size_t cursor = 0;        // current position in buffer
        size_t bufferEnd = 0;     // end of valid data in buffer
    };

    inline TextReader::TextReader(const std::string& p)
        : path(p)
    {
        // Open the file in text read mode
        file = std::fopen(path.c_str(), "r");
        if (!file) {
            IOError code;
            switch (errno) {
                case ENOENT: // No such file or directory
                    code = IOError::FileNotFound;
                    break;
                case EACCES: // Permission denied
                    code = IOError::PermissionDenied;
                    break;
                default:
                    code = IOError::FileNotOpen;
            }
            throw IOException(code, formatIOError(code, path), path);
        }

        // Allocate per-file 1 MB buffer for manual buffered reads
        buffer.resize(1 << 20);
    }

    inline TextReader::~TextReader() {
        if (!file) return;
        std::fclose(file);
    }

    inline bool TextReader::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline std::string TextReader::readString() {
        if (!file) 
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        std::string result;
        result.reserve(4 << 20); // start with 4 MB, grows dynamically if needed

        while (true) {
            size_t bytesRead = SFIO_FREAD(buffer.data(), 1, buffer.size(), file);
            if (bytesRead == 0) {
                if (ferror(file))
                    throw IOException(IOError::ReadError, formatIOError(IOError::ReadError, path), path);
                break;
            }
            result.append(buffer.data(), bytesRead);
        }
        return result;
    }

    inline std::string TextReader::readLine() {
        if (!file) 
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        std::string line;
        bool anyDataRead = false;

        while (true) {
            if (cursor >= bufferEnd) {
                // refill buffer
                size_t leftover = bufferEnd - cursor;
                if (leftover > 0) {
                    std::memmove(buffer.data(), buffer.data() + cursor, leftover);
                }
                size_t bytesRead = SFIO_FREAD(buffer.data() + leftover, 1, buffer.size() - leftover, file);
                cursor = 0;
                bufferEnd = leftover + bytesRead;

                if (bytesRead == 0) {
                    if (ferror(file))
                        throw IOException(IOError::ReadError, formatIOError(IOError::ReadError, path), path);
                    break; // normal EOF
                }
            }

            size_t start = cursor;
            while (cursor < bufferEnd && buffer[cursor] != '\n') cursor++;
            if (cursor > start) {
                line.append(buffer.data() + start, cursor - start);
                anyDataRead = true;
            }

            if (cursor < bufferEnd && buffer[cursor] == '\n') {
                cursor++; // skip newline
                break;
            }
        }

        if (!anyDataRead && std::feof(file))
            return {}; // EOF reached, return empty string without throwing

        return line;
    }

    inline std::vector<std::string> TextReader::readLines(int numLines) {
        std::vector<std::string> lines;
        if (numLines > 0) lines.reserve(numLines);

        while (numLines == 0 || lines.size() < static_cast<size_t>(numLines)) {
            std::string line = readLine();
            if (line.empty() && std::feof(file)) break; // stop at EOF
            lines.push_back(std::move(line));
        }

        return lines;
    }

    /**
     * @ingroup TextIO
     * @class TextWriter
     * @brief High-performance buffered text file writer.
     *
     * Uses chunked writes and a reusable internal buffer to minimize
     * syscall overhead.
     */
    class TextWriter {
    public:
        /**
         * @brief Opens a text file for writing.
         * @param path   File path
         * @param append Append instead of overwrite
         * @throws IOException if the file cannot be opened
         */
        inline TextWriter(const std::string& path, bool append = false);

        /**
         * @brief Flushes buffers and closes the file.
         */
        inline ~TextWriter();

        /**
         * @brief Checks whether a file exists.
         */
        inline static bool exists(const std::string& path);
        
        /**
         * @brief Flushes buffered output to disk.
         */
        inline void flush();

        /**
         * @brief Writes a raw string to the file.
         *
         * @complexity Time: O(n)  
         * @complexity Space: O(1)
         *
         * @throws IOException on write failure
         */
        inline void writeString(const std::string& data);
        
        /**
         * @brief Writes a single line with a trailing newline.
         *
         * @throws IOException on write failure
         */
        inline void writeLine(const std::string& line);
        
        /**
         * @brief Writes multiple lines to the file.
         *
         * @complexity Time: O(n)  
         * @complexity Space: O(n)
         *
         * @throws IOException on write failure
         */
        inline void writeLines(const std::vector<std::string>& lines);

    private:
        FILE* file = nullptr;
        std::string path;
        bool append = false;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline TextWriter::TextWriter(const std::string& p, bool a)
        : path(p), append(a)
    {
        const char* modeStr = append ? "a" : "w";
        file = std::fopen(path.c_str(), modeStr);
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        // Allocate per-file 1 MB buffer for assembling write payloads
        buffer.resize(1 << 20);
    }

    inline TextWriter::~TextWriter() {
        if (!file) return;
        std::fflush(file);
        std::fclose(file);
    }

    inline bool TextWriter::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline void TextWriter::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline void TextWriter::writeString(const std::string& data) {
        // chunked write to avoid issues with extremely large strings
        const size_t chunkSize = 1 << 20;
        size_t offset = 0;
        while (offset < data.size()) {
            size_t toWrite = std::min(chunkSize, data.size() - offset);
            size_t written = SFIO_FWRITE(data.data() + offset, 1, toWrite, file);
            if (written != toWrite)
                throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write string to file."), path);
            offset += written;
        }
    }

    inline void TextWriter::writeLine(const std::string& line) {
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        // Use single preallocated buffer per TextWriter
        buffer.clear();
        buffer.insert(buffer.end(), line.begin(), line.end());
        buffer.push_back('\n');
        size_t written = SFIO_FWRITE(buffer.data(), 1, buffer.size(), file);
        if (written != buffer.size())
            throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write line to file."), path);
    }

    inline void TextWriter::writeLines(const std::vector<std::string>& lines) {
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);
        if (lines.empty()) return;

        // Compute total size: sum of line sizes + missing newlines for lines that don't end with '\n'
        size_t totalSize = 0;
        size_t missingNewlines = 0;
        for (const auto &line : lines) {
            totalSize += line.size();
            if (line.empty() || line.back() != '\n') ++missingNewlines;
        }
        totalSize += missingNewlines;

        // Allocate once and memcpy all data into the buffer to avoid per-line overhead
        buffer.clear();
        buffer.resize(totalSize);
        char* ptr = buffer.data();

        for (const auto &line : lines) {
            size_t len = line.size();
            if (len > 0) {
                std::memcpy(ptr, line.data(), len);
                ptr += len;
            }
            if (len == 0 || line[len-1] != '\n') {
                *ptr++ = '\n';
            }
        }

        size_t written = SFIO_FWRITE(buffer.data(), 1, buffer.size(), file);
        if (written != buffer.size())
            throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write lines to file."), path);
    }

    /**
     * @ingroup BinaryIO
     * @class ByteReader
     * @brief High-performance binary file reader.
     *
     * Provides efficient sequential access to raw bytes using a manually
     * managed 1 MB buffer to minimize system call overhead.
     *
     * @note Intended for large, contiguous binary reads.
     * @warning Not safe for concurrent access from multiple threads.
     */
    class ByteReader {
    public:
        /**
         * @brief Opens a binary file for reading.
         *
         * @param path Path to the file
         *
         * @throws IOException if the file cannot be opened
         *         (e.g., file does not exist or permission is denied)
         *
         * @note The file is opened in binary mode ("rb").
         */
        inline ByteReader(const std::string& path);

        /**
         * @brief Closes the file and releases all associated resources.
         *
         * @note Automatically closes the underlying FILE handle.
         */
        inline ~ByteReader();

        /**
         * @brief Checks whether a file exists.
         *
         * @param path Path to the file
         * @return True if the file exists, false otherwise
         *
         * @note This function does not verify read permissions.
         */
        inline static bool exists(const std::string& path);

        /**
         * @brief Reads the entire file into a byte buffer.
         *
         * @return Vector containing all bytes read from the file
         *
         * @throws IOException on low-level read failure
         *
         * @complexity Time: O(n)  
         * @complexity Space: O(n)
         *
         * @note The returned vector grows dynamically as needed.
         * @warning Large files will be fully loaded into memory.
         */
        inline std::vector<char> readBytes();

    private:
        FILE* file = nullptr;
        std::string path;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline ByteReader::ByteReader(const std::string& p)
        : path(p)
    {
        file = std::fopen(path.c_str(), "rb");
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        // Allocate per-file 1 MB buffer for manual reads
        buffer.resize(1 << 20);
        // Do not use setvbuf() with our buffer here to avoid buffer aliasing with fread() calls
    }

    inline ByteReader::~ByteReader() {
        if (!file) return;
        std::fclose(file);
    }

    inline bool ByteReader::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline std::vector<char> ByteReader::readBytes() {
        std::vector<char> data;
        data.reserve(4 << 20); // start with 4 MB, grows dynamically if needed

        while (true) {
            size_t bytesRead = SFIO_FREAD(buffer.data(), 1, buffer.size(), file);
            if (bytesRead == 0) {
                if (ferror(file))
                    throw IOException(IOError::ReadError, formatIOError(IOError::ReadError, path), path);
                break;
            }
            data.insert(data.end(), buffer.data(), buffer.data() + bytesRead);
        }

        return data;
    }

    /**
     * @ingroup BinaryIO
     * @class ByteWriter
     * @brief High-performance binary file writer.
     *
     * Writes raw byte buffers efficiently using chunked I/O to avoid
     * excessive memory usage and system calls.
     *
     * @note Uses chunked writes for large buffers.
     * @warning Not safe for concurrent access from multiple threads.
     */
    class ByteWriter {
    public:
        /**
         * @brief Opens a binary file for writing.
         *
         * @param path   Path to the file
         * @param append If true, appends to the file instead of overwriting
         *
         * @throws IOException if the file cannot be opened
         *
         * @note The file is opened in binary mode ("wb" or "ab").
         */
        inline ByteWriter(const std::string& path, bool append = false);

        /**
         * @brief Flushes buffered output and closes the file.
         *
         * @note Destructor ensures that all pending data is flushed to disk.
         */
        inline ~ByteWriter();

        /**
         * @brief Checks whether a file exists.
         *
         * @param path Path to the file
         * @return True if the file exists, false otherwise
         */
        inline static bool exists(const std::string& path);

        /**
         * @brief Flushes any buffered output to disk.
         *
         * @throws IOException on flush failure
         *
         * @note This does not close the file.
         */
        inline void flush();

        /**
         * @brief Writes a byte buffer to the file.
         *
         * Performs chunked writes to safely handle very large buffers.
         *
         * @param data Byte buffer to write
         *
         * @throws IOException on write failure
         *
         * @complexity Time: O(n)  
         * @complexity Space: O(1)
         *
         * @warning Partial writes result in an exception; no retry is attempted.
         */
        inline void writeBytes(const std::vector<char>& data);

    private:
        FILE* file = nullptr;
        std::string path;
        bool append = false;
        std::vector<char> buffer; // per-file 1MB buffer
    };

    inline ByteWriter::ByteWriter(const std::string& p, bool a)
        : path(p), append(a)
    {
        const char* modeStr = append ? "ab" : "wb";
        file = std::fopen(path.c_str(), modeStr);
        if (!file)
            throw IOException(IOError::FileNotOpen, formatIOError(IOError::FileNotOpen, path), path);

        // Allocate per-file 1 MB buffer for assembling write payloads
        buffer.resize(1 << 20);
        // Do NOT pass this buffer to setvbuf(). Using the same memory for stdio's
        // internal buffer and as the write source can cause corrupted output when reused.
    }

    inline ByteWriter::~ByteWriter() {
        if (!file) return;
        std::fflush(file);
        std::fclose(file);
    }

    inline bool ByteWriter::exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    inline void ByteWriter::flush() {
        if (!file) return;
        std::fflush(file);
    }

    inline void ByteWriter::writeBytes(const std::vector<char>& data) {
        const size_t chunkSize = 1 << 20; // 1 MB
        size_t offset = 0;
        while (offset < data.size()) {
            size_t toWrite = std::min(chunkSize, data.size() - offset);
            size_t written = SFIO_FWRITE(data.data() + offset, 1, toWrite, file);
            if (written != toWrite)
                throw IOException(IOError::WriteError, formatIOError(IOError::WriteError, path, "Failed to write bytes to file."), path);
            offset += written;
        }
    }
}