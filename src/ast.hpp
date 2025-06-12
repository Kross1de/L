#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

enum class TokenType {
    PLUS, MINUS, MUL, DIV, EXP, LPAR, RPAR, NUM, IDENTIFIER,
    FN, LBRACE, RBRACE, COMMA, END_OF_FILE, ERROR,
    U8, U16, U32, U64, CONST,
    INT, FLOAT
};

enum class NodeType {
    NUMBER, IDENTIFIER, BINARY_OP, FUNCTION, UNSIGNED_INT,
    SIGNED_INT, FLOAT
};

struct ASTNode {
    NodeType type;
    virtual ~ASTNode() = default;
    explicit ASTNode(NodeType t) : type(t) {}
};

struct NumberNode : public ASTNode {
    double value;
    explicit NumberNode(double v) : ASTNode(NodeType::NUMBER), value(v) {}
};

struct IdentifierNode : public ASTNode {
    std::string name;
    explicit IdentifierNode(const std::string& n) : ASTNode(NodeType::IDENTIFIER), name(n) {}
};

struct BinaryOpNode : public ASTNode {
    TokenType op;
    ASTNode* left;
    ASTNode* right;
    BinaryOpNode(TokenType o, ASTNode* l, ASTNode* r)
        : ASTNode(NodeType::BINARY_OP), op(o), left(l), right(r) {}
    ~BinaryOpNode() {
        delete left;
        delete right;
    }
};

struct FunctionNode : public ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::vector<TokenType> paramTypes;
    std::vector<bool> paramIsConst;
    std::vector<ASTNode*> body;
    FunctionNode(const std::string& n, const std::vector<std::string>& p,
                 const std::vector<TokenType>& pt, const std::vector<bool>& pc,
                 const std::vector<ASTNode*>& b)
        : ASTNode(NodeType::FUNCTION), name(n), params(p), paramTypes(pt), paramIsConst(pc), body(b) {}
    ~FunctionNode() {
        for (ASTNode* node : body) {
            delete node;
        }
    }
};

struct UnsignedIntNode : public ASTNode {
    TokenType type;
    std::string value;
    explicit UnsignedIntNode(TokenType t, const std::string& v)
        : ASTNode(NodeType::UNSIGNED_INT), type(t), value(v) {}
};

struct SignedIntNode : public ASTNode {
    TokenType type;
    std::string value;
    explicit SignedIntNode(TokenType t, const std::string& v)
        : ASTNode(NodeType::SIGNED_INT), type(t), value(v) {}
};

struct FloatNode : public ASTNode {
    TokenType type;
    std::string value;
    explicit FloatNode(TokenType t, const std::string& v)
        : ASTNode(NodeType::FLOAT), type(t), value(v) {}
};

#endif