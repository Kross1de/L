#include "codegen.hpp"
#include "ast.hpp"
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <cstring>

CodeGen::CodeGen() : indentLevel(0),currentFunction("") 
{
        output = "section .text\n"
                 "global _start\n"
                 "extern pow\n"
                 "\n";
}

void CodeGen::indent()
{
        indentLevel += 4;
}

void CodeGen::dedent()
{
        indentLevel = std::max(0,indentLevel-4);
}

void CodeGen::addLine(const std::string& line)
{
        output += std::string(indentLevel,' ')+line+"\n";
}

std::string CodeGen::tokenTypeToAsmType(TokenType type) const
{
        switch(type)
        {
                case TokenType::U8: return "byte";
                case TokenType::U16: return "word";
                case TokenType::U32: return "dword";
                case TokenType::U64: return "qword";
                default: throw std::runtime_error("Invalid type for assembly code generation");
        }
}

void CodeGen::generateExpression(const ASTNode* node)
{
        if(node->type==NodeType::NUMBER)
        {
                const NumberNode* num = dynamic_cast<const NumberNode*>(node);
                double value = num->value;
                //Convert to hex representation
                uint64_t bits;
                std::memcpy(&bits,&value,sizeof(double));
                addLine("push qword 0x"+std::to_string(bits));
                addLine("fld qword [rsp]");
                addLine("add rsp, 8");
        } 
        else if(node->type==NodeType::IDENTIFIER)
        {
                const IdentifierNode* id = dynamic_cast<const IdentifierNode*>(node);
                addLine("fld qword [rbp - "+id->name+"]");
        }
        else if(node->type==NodeType::BINARY_OP)
        {
                const BinaryOpNode* bin = dynamic_cast<const BinaryOpNode*>(node);
                generateExpression(bin->left);
                generateExpression(bin->right);
                switch(bin->op)
                {
                        case TokenType::PLUS:
                                addLine("faddp");
                                break;
                        case TokenType::MINUS:
                                addLine("fsubp");
                                break;
                        case TokenType::MUL:
                                addLine("fmulp");
                                break;
                        case TokenType::DIV:
                                addLine("fdivp");
                                break;
                        case TokenType::EXP:
                                addLine("fstp qword [rsp - 8]");
                                addLine("fstp qword [rsp - 16]");
                                addLine("sub rsp, 16");
                                addLine("call pow");
                                addLine("add rsp, 16");
                                addLine("fld qword [rsp - 8]");
                                break;
                        default:
                                throw std::runtime_error("Invalid operator at line");
                }
        }
        else if(node->type==NodeType::UNSIGNED_INT)
        {
                const UnsignedIntNode* uint = dynamic_cast<const UnsignedIntNode*>(node);
                addLine("mov rax, "+uint->value);
                addLine("push rax");
                addLine("fild qword [rsp]");
                addLine("add rsp, 8");
        }
}

void CodeGen::generate(const std::vector<ASTNode*>& nodes)
{
        for(const ASTNode* node : nodes)
        {
                if(node->type==NodeType::FUNCTION)
                {
                        const FunctionNode* func = dynamic_cast<const FunctionNode*>(node);
                        currentFunction = func->name;
                        addLine("global "+func->name);
                        addLine(func->name+":");
                        indent();
                        addLine("push rbp");
                        addLine("mov rbp, rsp");

                        //Allocate stack space for parameters
                        int offset=16;
                        for(size_t i=0;i<func->params.size();++i)
                        {
                                std::string param = func->params[i];
                                addLine("; Parameter "+param);
                                addLine("sub rsp, 8");
                                if(i<4)
                                {
                                        std::string reg;
                                        if(i==0) reg="rdi";
                                        else if(i==1) reg="rsi";
                                        else if(i==2) reg="rdx";
                                        else reg="rcx";
                                        addLine("mov [rbp - ]"+param+"], "+reg);
                                }
                                else
                                {
                                        addLine("mov rax, [rbp + " + std::to_string(offset)+"]");
                                        addLine("mov [rbp - "+param+"], rax");
                                        offset+=8;
                                }
                        }

                        //Generate function body
                        for(const ASTNode* stmt : func->body)
                        {
                                generateExpression(stmt);
                                addLine("fstp dword [rsp - 8]");
                                addLine("movsd xmm0, [rsp - 8]");
                                addLine("add rsp, 8");
                                addLine("mov rsp, rbp");
                                addLine("pop rbp");
                                addLine("ret");
                        }

                        dedent();
                        addLine("");
                }
        }
}

std::string CodeGen::getCode() const
{
        return output;
}