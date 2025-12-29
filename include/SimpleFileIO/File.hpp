#pragma once

#include <string>
#include <vector>

namespace SimpleFileIO {

    // Enum for file modes
    enum class OpenMode : uint8_t {
        None   = 0,
        Read   = 1 << 0,
        Write  = 1 << 1,
        Append = 1 << 2,
        Binary = 1 << 3
    };

    inline bool has(OpenMode value, OpenMode flag) {
        return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
    }

    inline OpenMode operator|(OpenMode a, OpenMode b) {
        return static_cast<OpenMode>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
        );
    }

    inline OpenMode operator&(OpenMode a, OpenMode b) {
        return static_cast<OpenMode>(
            static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
        );
    }


    class File {
    public:
        // Constructor - open file with a path and mode
        File(const std::string& path, OpenMode mode);

        ~File();

        // Check if the file is successfully open
        bool isOpen() const;

        // Check if the file exists
        static bool exists(const std::string& path);

        // Read entire file into a string
        std::string readAll();

        // Read a single line from file
        std::string readLine();

        // Read file line by line
        std::vector<std::string> readLines(const int numLines = 0);

        // Write entire string to file
        void writeAll(const std::string& data);

        // Write a single line to file
        void writeLine(const std::string& line);

        // Write lines to file
        void writeLines(const std::vector<std::string>& lines);

        // Append string to file
        void append(const std::string& data);

        // Append lines to file
        void appendLines(const std::vector<std::string>& lines);

        // Append a single line to file
        void appendLine(const std::string& line);
    
    private:
        struct Impl;
        Impl* pImpl;
    };
}