#include "misc.hpp"
#include <iostream>

Misc::Misc(const std::string& filename) : pos(0),line(1),column(1){
    std::ifstream file(filename);
    if(!file.is_open()){
        throw std::runtime_error("Could not open file '"+filename+"'");
    }
    std::string line;
    while(std::getline(file,line)){
        input+=line+'\n';
    }
    file.close();
}

Misc::Misc(const std::string& src,size_t start_line,size_t start_column)
    : input(src),pos(0),line(start_line),column(start_column){}

char Misc::peek() const {
    if(pos>=input.length()){
        return '\0';
    }
    return input[pos];
}

char Misc::get(){
    if(pos>=input.length()){
        return '\0';
    }
    char c = input[pos++];
    if(c=='\n'){
        line++;
        column=1;
    }else{
        column++;
    }
    return c;
}

bool Misc::eof() const {
    return pos>=input.length();
}

std::string Misc::getRemaining() const {
    return input.substr(pos);
}
