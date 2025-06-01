#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include <string>
#include <vector>
#include "ast.hpp"

class CodeGen {
private:
    std::string output;
    int indentLevel;
    std::string currentFunction;

    void indent();
    void dedent();
    void addLine(const std::string& line);
    std::string tokenTypeToAsmType(TokenType type) const;
    void generateExpression(const ASTNode* node);

public:
    CodeGen();
    void generate(const std::vector<ASTNode*>& nodes);
    std::string getCode() const;
};

#endif