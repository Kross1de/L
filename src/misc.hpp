#ifndef MISC_HPP
#define MISC_HPP

#include <string>
#include <fstream>
#include <stdexcept>

class Misc {
private:
    std::string input;
    size_t pos;
    size_t line;
    size_t column;

public:
    Misc(const std::string& filename);
    Misc(const std::string& src, size_t start_line = 1, size_t start_column = 1);
    char peek() const;
    char get();
    bool eof() const;
    size_t getLine() const { return line; } 
    size_t getColumn() const { return column; }
    std::string getRemaining() const;
};
#endif