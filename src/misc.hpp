#ifndef MISC_HPP
#define MISC_HPP

#include <string>
#include <fstream>
#include <stdexcept>
#include <cstdint>

class Misc {
private:
	std::string input;
    	uint64_t pos;
    	uint64_t line;
	uint64_t column;

public:
	Misc(const std::string& filename);
	Misc(const std::string& src, uint64_t start_line = 1, uint64_t start_column = 1);
	char peek() const;
    	char get();
    	bool eof() const;
    	uint64_t getLine() const { return line; } 
    	uint64_t getColumn() const { return column; }
    	std::string getRemaining() const;
};
#endif
