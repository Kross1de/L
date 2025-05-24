#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <stdexcept>

enum class TokenType {
    PLUS, MINUS, MUL, DIV, LPAR, RPAR, NUM, END_OF_FILE, ERROR
};

struct Token {
    TokenType type;
    std::string value;

    Token(TokenType t, std::string v) : type(t), value(v) {}
};

class Lexer {
private:
    std::string input;
    size_t pos;

    void skipWhitespace() {
        while (pos < input.length() && std::isspace(input[pos])) {
            pos++;
        }
    }

    bool isNumberChar(char c) {
        return std::isdigit(c) || c == '.';
    }

    Token getNumber() {
        std::string numStr;
        bool hasDecimal = false;
        size_t startPos = pos;

        while (pos < input.length() && isNumberChar(input[pos])) {
            if (input[pos] == '.') {
                if (hasDecimal) {
                    return Token(TokenType::ERROR, "Multiple decimal points");
                }
                hasDecimal = true;
            }
            numStr += input[pos];
            pos++;
        }

        // validate number format
        try {
            std::stod(numStr);
            return Token(TokenType::NUM, numStr);
        } catch (...) {
            return Token(TokenType::ERROR, "Invalid number format: " + numStr);
        }
    }

public:
    Lexer(const std::string& src) : input(src), pos(0) {}

    Token nextToken() {
        skipWhitespace();

        if (pos >= input.length()) {
            return Token(TokenType::END_OF_FILE, "");
        }

        char current = input[pos];
        pos++;

        switch (current) {
            case '+': return Token(TokenType::PLUS, "+");
            case '-': return Token(TokenType::MINUS, "-");
            case '*': return Token(TokenType::MUL, "*");
            case '/': return Token(TokenType::DIV, "/");
            case '(': return Token(TokenType::LPAR, "(");
            case ')': return Token(TokenType::RPAR, ")");
            default:
                if (std::isdigit(current) || current == '.') {
                    pos--;
                    return getNumber();
                }
                return Token(TokenType::ERROR, std::string(1, current));
        }
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        Token token = nextToken();
        while (token.type != TokenType::END_OF_FILE) {
            tokens.push_back(token);
            if (token.type == TokenType::ERROR) {
                break; // stop if error
            }
            token = nextToken();
        }
        tokens.push_back(Token(TokenType::END_OF_FILE, ""));
        return tokens;
    }
};

// AST node types
enum class NodeType {
    NUMBER,
    BINARY_OP
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

class Parser {
private:
    const std::vector<Token>& tokens;
    size_t pos;
    Token currentToken;

    void advance() {
        if (pos < tokens.size()) {
            currentToken = tokens[pos++];
        } else {
            currentToken = Token(TokenType::END_OF_FILE, "");
        }
    }

    ASTNode* factor() {
        if (currentToken.type == TokenType::NUM) {
            double value = std::stod(currentToken.value);
            advance();
            return new NumberNode(value);
        } else if (currentToken.type == TokenType::LPAR) {
            advance();
            ASTNode* node = expr();
            if (currentToken.type != TokenType::RPAR) {
                throw std::runtime_error("Expected closing parenthesis");
            }
            advance();
            return node;
        }
        throw std::runtime_error("Expected number or parenthesis");
    }

    ASTNode* term() {
        ASTNode* node = factor();
        while (currentToken.type == TokenType::MUL || currentToken.type == TokenType::DIV) {
            TokenType op = currentToken.type;
            advance();
            ASTNode* right = factor();
            node = new BinaryOpNode(op, node, right);
        }
        return node;
    }

    ASTNode* expr() {
        ASTNode* node = term();
        while (currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
            TokenType op = currentToken.type;
            advance();
            ASTNode* right = term();
            node = new BinaryOpNode(op, node, right);
        }
        return node;
    }

public:
    Parser(const std::vector<Token>& t) : tokens(t), pos(0), currentToken(TokenType::END_OF_FILE, "") {
        if (!tokens.empty()) {
            currentToken = tokens[0];
            pos = 1;
        }
    }  

    ASTNode* parse() {
        if (currentToken.type == TokenType::END_OF_FILE) {
            throw std::runtime_error("Empty expression");
        }
        ASTNode* node = expr();
        if (currentToken.type != TokenType::END_OF_FILE) {
            throw std::runtime_error("Unexpected tokens after expression");
        }
        return node;
    }
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";
        case TokenType::LPAR: return "LPAR";
        case TokenType::RPAR: return "RPAR";
        case TokenType::NUM: return "NUM";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void printAST(ASTNode* node, int indent = 0) {
    if (!node) return;
    std::string indentStr(indent, ' ');

    if (node->type == NodeType::NUMBER) {
        NumberNode* num = dynamic_cast<NumberNode*>(node);
        std::cout << indentStr << "Number: " << num->value << "\n";
    } else if (node->type == NodeType::BINARY_OP) {
        BinaryOpNode* bin = dynamic_cast<BinaryOpNode*>(node);
        std::cout << indentStr << "BinaryOp: " << tokenTypeToString(bin->op) << "\n";
        printAST(bin->left, indent + 2); 
        printAST(bin->right, indent + 2); 
    }
}

int main(int argc, char* argv[]) {
    std::string input;

    if (argc == 3 && std::string(argv[1]) == "-output") {
        std::ifstream file(argv[2]);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << argv[2] << "'\n";
            return 1;
        }
        std::string line;
        while (std::getline(file, line)) {
            input += line + "\n";
        }
        file.close();
    } else {
        std::cerr << "Usage: " << argv[0] << " [-output <filename>]\n";
        return 1;
    }

    try {
        Lexer lexer(input);
        std::vector<Token> tokens = lexer.tokenize();

        std::cout << "\nTokens:\n";
        for (const auto& token : tokens) {
            std::cout << "Type: " << tokenTypeToString(token.type)
                      << ", Value: " << token.value << "\n";
        }

        Parser parser(tokens);
        ASTNode* ast = parser.parse();
        
        std::cout << "\nAST:\n";
        printAST(ast);
        
        delete ast;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}