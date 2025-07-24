#include "ast.hpp"
#include <set>
#include <vector>
#include <sstream>
#include <algorithm>

class CodeGenerator {
        public:
        std::string generate(const std::vector<ASTNode*>& ast) {
                std::set<std::string> globals;
                bool hasMain = false;
                collectGlobals(ast, globals);
                hasMain = hasFunctionMain(ast);

                std::stringstream ss;
                ss << "section .data\n";
                for (const auto& g : globals)
                        ss << g << ": dq 0\n";
                ss << "section .text\n";
                ss << "global _start\n";
                ss << "_start:\n";
                if (hasMain) {
                        ss << " call main\n";
                } else {
                        for (const auto& node : ast) {
                                if (auto assign = dynamic_cast<AssignmentNode*>(node)) {
                                        ss << generateAssignment(assign, {}) << "\n";
                                } else {
                                        ss << generateExpression(node, {}) << "\n";
                                }
                        }
                }
                ss << " mov     rax, 60\n";
                ss << " xor     rdi, rdi\n";
                ss << " syscall\n";
                for (const auto& node : ast) {
                        if (auto func = dynamic_cast<FunctionNode*>(node)) {
                                ss << generateFunction(func) << "\n";
                        }
                }
                return ss.str();
        }

        private:
        void collectGlobals(const std::vector<ASTNode*>& nodes, std::set<std::string>& globals) {
                for (const auto& node : nodes) {
                        if (auto assign = dynamic_cast<AssignmentNode*>(node)) {
                                globals.insert(assign->identifier);
                        } else if (auto func = dynamic_cast<FunctionNode*>(node)) {
                                collectGlobals(func->body, globals);
                        }
                }
        }

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
                        ss << " mov     rax, " << static_cast<int64_t>(num->value) << "\n";
                } else if (auto uint = dynamic_cast<UnsignedIntNode*>(node)) {
                        ss << " mov     rax, " << uint->value << "\n";
                } else if (auto sint = dynamic_cast<SignedIntNode*>(node)) {
                        ss << " mov     rax, " << sint->value << "\n";
                } else if (auto id = dynamic_cast<IdentifierNode*>(node)) {
                        auto it = std::find(params.begin(), params.end(), id->name);
                        if (it != params.end()) {
                                int index = std::distance(params.begin(), it);
                                std::vector<std::string> paramRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                                if (index < paramRegs.size()) {
                                        ss << " mov     rax, " << paramRegs[index] << "\n";
                                }
                        } else {
                                ss << " mov     rax, [" << id->name << "]\n";
                        }
                } else if (auto binop = dynamic_cast<BinaryOpNode*>(node)) {
                        ss << generateExpression(binop->left, params);
                        ss << " push    rax\n";
                        ss << generateExpression(binop->right, params);
                        ss << " mov     rcx, rax\n";
                        ss << " pop     rax\n";
                        if (binop->op == TokenType::PLUS) {
                                ss << " add     rax, rcx\n";
                        } else if (binop->op == TokenType::MINUS) {
                                ss << " sub     rax, rcx\n";
                        } else if (binop->op == TokenType::MUL) {
                                ss << " imul    rax, rcx\n";
                        } else if (binop->op == TokenType::DIV) {
                                ss << " mov     rdx, 0\n";
                                ss << " idiv    rcx\n";
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
                                ss << " mov     " << paramRegs[index] << ", rax\n";
                        }
                } else {
                        ss << " mov     [" << assign->identifier << "], rax\n";
                }
                return ss.str();
        }

        std::string generateFunction(FunctionNode* node) {
                std::stringstream ss;
                ss << node->name << ":\n";
                ss << " push    rbp\n";
                ss << " mov     rbp, rsp\n";
                std::vector<std::string> params = node->params;
                for (const auto& stmt : node->body) {
                        if (auto assign = dynamic_cast<AssignmentNode*>(stmt)) {
                                ss << generateAssignment(assign, params) << "\n";
                        } else {
                                ss << generateExpression(stmt, params) << "\n";
                        }
                }
                if (!node->body.empty() && !dynamic_cast<AssignmentNode*>(node->body.back())) {
                        //
                } else {
                        ss << " mov     rax, 0\n";
                }
                ss << " mov     rsp, rbp\n";
                ss << " pop     rbp\n";
                ss << " ret\n";
                return ss.str();
        }
};