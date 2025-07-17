#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class CodeGenerator {
public:
    CodeGenerator();
    std::string generateCode(const std::vector<ASTNode*>& ast);
private:
    std::string generateFunction(const FunctionNode* node);
    std::string generateExpression(const ASTNode* node, const std::unordered_map<std::string, int>& varOffsets);
    std::string getRegisterForParam(size_t index);
};

#endif