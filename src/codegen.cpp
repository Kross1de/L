#include "codegen.hpp"
#include <sstream>

CodeGenerator::CodeGenerator() {}

std::string CodeGenerator::generateCode(const std::vector<ASTNode*>& ast) {
    std::string code = "section .text\n";
    for (const auto* node : ast) {
        if (const auto* funcNode = dynamic_cast<const FunctionNode*>(node)) {
            code += generateFunction(funcNode);
        }
    }
    return code;
}

std::string CodeGenerator::generateFunction(const FunctionNode* node) {
    std::string code = "global " + node->name + "\n";
    code += node->name + ":\n";
    code += "    push rbp\n";
    code += "    mov rbp, rsp\n";

    std::unordered_map<std::string, int> varOffsets;
    int offset = 8; /* start after rbp */
    for (size_t i = 0; i < node->params.size(); ++i) {
        const auto& param = node->params[i];
        varOffsets[param] = offset;
        std::string reg = getRegisterForParam(i);
        if (!reg.empty()) {
            code += "    mov [rbp - " + std::to_string(offset) + "], " + reg + "\n";
        }
        offset += 8;
    }

    std::string bodyCode;
    for (const auto* stmt : node->body) {
        bodyCode += generateExpression(stmt, varOffsets);
    }
    code += bodyCode;
    code += "    pop rax\n";
    code += "    mov rsp, rbp\n";
    code += "    pop rbp\n";
    code += "    ret\n";
    return code;
}

std::string CodeGenerator::generateExpression(const ASTNode* node, const std::unordered_map<std::string, int>& varOffsets) {
    if (const auto* numNode = dynamic_cast<const NumberNode*>(node)) {
        return "    mov rax, " + std::to_string(numNode->value) + "\n" +
               "    push rax\n";
    } else if (const auto* idNode = dynamic_cast<const IdentifierNode*>(node)) {
        auto it = varOffsets.find(idNode->name);
        if (it != varOffsets.end()) {
            int offset = it->second;
            return "    mov rax, [rbp - " + std::to_string(offset) + "]\n" +
                   "    push rax\n";
        }
        return "";
    } else if (const auto* binOpNode = dynamic_cast<const BinaryOpNode*>(node)) {
        std::string leftCode = generateExpression(binOpNode->left, varOffsets);
        std::string rightCode = generateExpression(binOpNode->right, varOffsets);
        std::string opCode;
        if (binOpNode->op == TokenType::PLUS) {
            opCode = "add";
        } else if (binOpNode->op == TokenType::MINUS) {
            opCode = "sub";
        } else if (binOpNode->op == TokenType::MUL) {
            opCode = "imul";
        } else if (binOpNode->op == TokenType::DIV) {
            opCode = "idiv";
            return leftCode + rightCode +
                   "    pop rbx\n" +
                   "    pop rax\n" +
                   "    cqo\n" +
                   "    idiv rbx\n" +
                   "    push rax\n";
        } else if (binOpNode->op == TokenType::EXP) {
            return "";
        }
        return leftCode + rightCode +
               "    pop rbx\n" +
               "    pop rax\n" +
               "    " + opCode + " rax, rbx\n" +
               "    push rax\n";
    } else if (const auto* assignNode = dynamic_cast<const AssignmentNode*>(node)) {
		std::string valueCode = generateExpression(assignNode->value, varOffsets);
		auto it = varOffsets.find(assignNode->identifier);
		if (it != varOffsets.end()) {
			int offset = it->second;
			return valueCode +
				   "    pop rax\n" +
				   "    mov [rbp - " + std::to_string(offset) + "], rax\n";
		}
		return "";
	}
    return "";
}

std::string CodeGenerator::getRegisterForParam(size_t index) {
    switch (index) {
        case 0: return "rdi";
        case 1: return "rsi";
        case 2: return "rdx";
        case 3: return "rcx";
        case 4: return "r8";
        case 5: return "r9";
        default: return "";
    }
}