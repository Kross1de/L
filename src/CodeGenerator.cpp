#include "ast.hpp"
#include <set>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>

class CodeGenerator {
public:
        std::set<std::string> globals;
        std::map<std::string, TokenType> globalTypes;

        void collectGlobals(const std::vector<ASTNode*>& nodes) {
                for (const auto& node : nodes) {
                        if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
                                globals.insert(varDecl->identifier);
                                globalTypes[varDecl->identifier] = varDecl->type;
                        } else if (auto func = dynamic_cast<FunctionNode*>(node)) {
                                collectGlobals(func->body);
                        }
                }
        }

        std::string generate(const std::vector<ASTNode*>& ast) {
                collectGlobals(ast);
                bool hasMain = hasFunctionMain(ast);

                std::stringstream ss;
                ss << "section .data\n";
                for (const auto& g : globals) {
                        TokenType type = globalTypes[g];
                        if (type == TokenType::U8) {
                                ss << g << ": db 0\n";
                        } else if (type == TokenType::U16) {
                                ss << g << ": dw 0\n";
                        } else if (type == TokenType::U32) {
                                ss << g << ": dd 0\n";
                        } else if (type == TokenType::U64 || type == TokenType::INT) {
                                ss << g << ": dq 0\n";
                        } else if (type == TokenType::FLOAT) {
                                ss << g << ": dq 0.0\n";
                        } else {
                                ss << g << ": dq 0\n";
                        }
                }
                ss << "section .text\n";
                ss << "global _start\n";
                ss << "_start:\n";
                if (hasMain) {
                        ss << "        call main\n";
                } else {
                        for (const auto& node : ast) {
                                if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
                                        if (varDecl->initializer) {
                                                std::stringstream varSS;
                                                varSS << generateExpression(varDecl->initializer, {});
                                                TokenType type = globalTypes[varDecl->identifier];
                                                if (type == TokenType::U8) {
                                                        varSS << "        mov     byte [" << varDecl->identifier << "], al\n";
                                                } else if (type == TokenType::U16) {
                                                        varSS << "        mov     word [" << varDecl->identifier << "], ax\n";
                                                } else if (type == TokenType::U32) {
                                                        varSS << "        mov     dword [" << varDecl->identifier << "], eax\n";
                                                } else {
                                                        varSS << "        mov     [" << varDecl->identifier << "], rax\n";
                                                }
                                                ss << varSS.str() << "\n";
                                        }
                                } else if (auto assign = dynamic_cast<AssignmentNode*>(node)) {
                                        ss << generateAssignment(assign, {}) << "\n";
                                } else {
                                        ss << generateExpression(node, {}) << "\n";
                                }
                        }
                }
                ss << "        mov     rax, 60\n";
                ss << "        xor     rdi, rdi\n";
                ss << "        syscall\n";
                for (const auto& node : ast) {
                        if (auto func = dynamic_cast<FunctionNode*>(node)) {
                                ss << generateFunction(func) << "\n";
                        }
                }
                return ss.str();
        }

private:
        bool hasFunctionMain(const std::vector<ASTNode*>& ast) {
                for (const auto& node : ast) {
                        if (auto func = dynamic_cast<FunctionNode*>(node)) {
                                if (func->name == "main") {
                                        return true;
                                }
                        }
                }
                return false;
        }

        std::string generateExpression(ASTNode* node, const std::vector<std::string>& params = {}) {
                std::stringstream ss;
                if (auto num = dynamic_cast<NumberNode*>(node)) {
                        ss << "        mov     rax, " << static_cast<int64_t>(num->value) << "\n";
                } else if (auto uint = dynamic_cast<UnsignedIntNode*>(node)) {
                        ss << "        mov     rax, " << uint->value << "\n";
                } else if (auto sint = dynamic_cast<SignedIntNode*>(node)) {
                        ss << "        mov     rax, " << sint->value << "\n";
                } else if (auto id = dynamic_cast<IdentifierNode*>(node)) {
                        auto it = std::find(params.begin(), params.end(), id->name);
                        if (it != params.end()) {
                                int index = std::distance(params.begin(), it);
                                std::vector<std::string> paramRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                                if (index < paramRegs.size()) {
                                        ss << "        mov     rax, " << paramRegs[index] << "\n";
                                }
                        } else {
                                if (globalTypes.find(id->name) != globalTypes.end()) {
                                        TokenType type = globalTypes[id->name];
                                        if (type == TokenType::U8) {
                                                ss << "        movzx   rax, byte [" << id->name << "]\n";
                                        } else if (type == TokenType::U16) {
                                                ss << "        movzx   rax, word [" << id->name << "]\n";
                                        } else if (type == TokenType::U32) {
                                                ss << "        mov     eax, [" << id->name << "]\n";
                                                ss << "        movzx   rax, eax\n";
                                        } else {
                                                ss << "        mov     rax, [" << id->name << "]\n";
                                        }
                                } else {
                                        ss << "        mov     rax, [" << id->name << "]\n";
                                }
                        }
                } else if (auto binop = dynamic_cast<BinaryOpNode*>(node)) {
                        ss << generateExpression(binop->left, params);
                        ss << "        push    rax\n";
                        ss << generateExpression(binop->right, params);
                        ss << "        mov     rcx, rax\n";
                        ss << "        pop     rax\n";
                        if (binop->op == TokenType::PLUS) {
                                ss << "        add     rax, rcx\n";
                        } else if (binop->op == TokenType::MINUS) {
                                ss << "        sub     rax, rcx\n";
                        } else if (binop->op == TokenType::MUL) {
                                ss << "        imul    rax, rcx\n";
                        } else if (binop->op == TokenType::DIV) {
                                ss << "        mov     rdx, 0\n";
                                ss << "        idiv    rcx\n";
                        }
                }
                return ss.str();
        }

        std::string generateAssignment(ASTNode* node, const std::vector<std::string>& params = {}) {
                auto assign = dynamic_cast<AssignmentNode*>(node);
                std::stringstream ss;
                ss << generateExpression(assign->value, params);
                auto it = std::find(params.begin(), params.end(), assign->identifier);
                if (it != params.end()) {
                        int index = std::distance(params.begin(), it);
                        std::vector<std::string> paramRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                        if (index < paramRegs.size()) {
                                ss << "        mov     " << paramRegs[index] << ", rax\n";
                        }
                } else {
                        if (globalTypes.find(assign->identifier) != globalTypes.end()) {
                                TokenType type = globalTypes[assign->identifier];
                                if (type == TokenType::U8) {
                                        ss << "        mov     byte [" << assign->identifier << "], al\n";
                                } else if (type == TokenType::U16) {
                                        ss << "        mov     word [" << assign->identifier << "], ax\n";
                                } else if (type == TokenType::U32) {
                                        ss << "        mov     dword [" << assign->identifier << "], eax\n";
                                } else {
                                        ss << "        mov     [" << assign->identifier << "], rax\n";
                                }
                        } else {
                                ss << "        mov     [" << assign->identifier << "], rax\n";
                        }
                }
                return ss.str();
        }

        std::string generateFunction(FunctionNode* node) {
                std::stringstream code;
                code << node->name << ":\n";
                code << "        push    rbp\n";
                code << "        mov     rbp, rsp\n";
                std::vector<std::string> params = node->params;
                for (const auto& stmt : node->body) {
                        if (auto varDecl = dynamic_cast<VarDeclNode*>(stmt)) {
                                if (varDecl->initializer) {
                                        std::stringstream ss;
                                        ss << generateExpression(varDecl->initializer, params);
                                        TokenType type = globalTypes[varDecl->identifier];
                                        if (type == TokenType::U8) {
                                                ss << "        mov     byte [" << varDecl->identifier << "], al\n";
                                        } else if (type == TokenType::U16) {
                                                ss << "        mov     word [" << varDecl->identifier << "], ax\n";
                                        } else if (type == TokenType::U32) {
                                                ss << "        mov     dword [" << varDecl->identifier << "], eax\n";
                                        } else {
                                                ss << "        mov     [" << varDecl->identifier << "], rax\n";
                                        }
                                        code << ss.str();
                                }
                        } else if (auto assign = dynamic_cast<AssignmentNode*>(stmt)) {
                                code << generateAssignment(assign, params) << "\n";
                        } else {
                                code << generateExpression(stmt, params) << "\n";
                        }
                }
                if (!node->body.empty() && !dynamic_cast<AssignmentNode*>(node->body.back())) {
                        // no return value handling
                } else {
                        code << "        mov     rax, 0\n";
                }
                code << "        mov     rsp, rbp\n";
                code << "        pop     rbp\n";
                code << "        ret\n";
                return code.str();
        }
};