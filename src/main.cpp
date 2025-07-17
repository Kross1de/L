#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include "misc.hpp"
#include "codegen.hpp"
#include "ast.hpp"

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;

    Token(TokenType t, std::string v, size_t l, size_t c) 
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
private:
    Misc misc;
    size_t pos;

    void skipWhitespace() {
        while (!misc.eof() && std::isspace(misc.peek())) {
            misc.get();
        }
    }

    void skipComments() {
        if (!misc.eof() && misc.peek() == '/' && misc.getRemaining().length() > 1 && misc.getRemaining()[1] == '/') {
            misc.get();
            misc.get();
            while (!misc.eof() && misc.peek() != '\n') {
                misc.get();
            }
            if (!misc.eof() && misc.peek() == '\n') {
                misc.get();
            }
        }
    }

    bool isNumberChar(char c) const {
        return std::isdigit(c) || c == '.' || c == '-';
    }

    bool isIdentifierChar(char c) const {
        return std::isalnum(c) || c == '_';
    }

    Token getNumber() {
        std::string numStr;
        bool hasDecimal = false;
        size_t startLine = misc.getLine();
        size_t startColumn = misc.getColumn();

        if (!misc.eof() && misc.peek() == '-' &&
            (pos == 0 || std::isspace(misc.getRemaining()[pos-1]) || misc.getRemaining()[pos-1] == '(')) {
            numStr += misc.get();
        }

        while (!misc.eof() && isNumberChar(misc.peek())) {
            char c = misc.get();
            if (c == '.') {
                if (hasDecimal) {
                    return Token(TokenType::ERROR, "Multiple decimal points", startLine, startColumn);
                }
                hasDecimal = true;
            }
            numStr += c;
        }

        try {
            std::stod(numStr);
            return Token(TokenType::NUM, numStr, startLine, startColumn);
        } catch (...) {
            return Token(TokenType::ERROR, "Invalid number format: " + numStr, startLine, startColumn);
        }
    }

    Token getIdentifier() {
        std::string idStr;
        size_t startLine = misc.getLine();
        size_t startColumn = misc.getColumn();

        while (!misc.eof() && isIdentifierChar(misc.peek())) {
            idStr += misc.get();
        }

        if (idStr == "fn") {
            return Token(TokenType::FN, idStr, startLine, startColumn);
        }
        else if (idStr == "const") {
            return Token(TokenType::CONST, idStr, startLine, startColumn);
        }
        else if (idStr == "u8") {
            return Token(TokenType::U8, idStr, startLine, startColumn);
        }
        else if (idStr == "u16") {
            return Token(TokenType::U16, idStr, startLine, startColumn);
        }
        else if (idStr == "u32") {
            return Token(TokenType::U32, idStr, startLine, startColumn);
        }
        else if (idStr == "u64") {
            return Token(TokenType::U64, idStr, startLine, startColumn);
        }
        else if (idStr == "int") {
            return Token(TokenType::INT, idStr, startLine, startColumn);
        }
        else if (idStr == "float") {
            return Token(TokenType::FLOAT, idStr, startLine, startColumn);
        }
        return Token(TokenType::IDENTIFIER, idStr, startLine, startColumn);
    }

public:
    Lexer(const std::string& src) : misc(src, 1, 1), pos(0) {}

    Token nextToken() {
        skipWhitespace();
        while (!misc.eof() && misc.peek() == '/' && misc.getRemaining().length() > 1 && misc.getRemaining()[1] == '/') {
            skipComments();
            skipWhitespace();
        }

        if (misc.eof()) {
            return Token(TokenType::END_OF_FILE, "", misc.getLine(), misc.getColumn());
        }

        size_t startLine = misc.getLine();
        size_t startColumn = misc.getColumn();
        char current = misc.get();
        pos++;

        switch (current) {
            case '+': return Token(TokenType::PLUS, "+", startLine, startColumn);
            case '-': return Token(TokenType::MINUS, "-", startLine, startColumn);
            case '*': return Token(TokenType::MUL, "*", startLine, startColumn);
            case '/': return Token(TokenType::DIV, "/", startLine, startColumn);
            case '^': return Token(TokenType::EXP, "^", startLine, startColumn);
            case '(': return Token(TokenType::LPAR, "(", startLine, startColumn);
            case ')': return Token(TokenType::RPAR, ")", startLine, startColumn);
            case '{': return Token(TokenType::LBRACE, "{", startLine, startColumn);
            case '}': return Token(TokenType::RBRACE, "}", startLine, startColumn);
            case ',': return Token(TokenType::COMMA, ",", startLine, startColumn);
			case '=': return Token(TokenType::EQUAL, "=", startLine, startColumn);
            default:
                if (std::isdigit(current) || current == '.') {
                    misc = Misc(std::string(1, current) + misc.getRemaining(), startLine, startColumn);
                    return getNumber();
                }
                if (std::isalpha(current) || current == '_') {
                    misc = Misc(std::string(1, current) + misc.getRemaining(), startLine, startColumn);
                    return getIdentifier();
                }
                return Token(TokenType::ERROR, std::string(1, current), startLine, startColumn);
        }
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        Token token = nextToken();
        while (token.type != TokenType::END_OF_FILE) {
            tokens.push_back(token);
            if (token.type == TokenType::ERROR) {
                break;
            }
            token = nextToken();
        }
        tokens.push_back(Token(TokenType::END_OF_FILE, "", misc.getLine(), misc.getColumn()));
        return tokens;
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
            currentToken = Token(TokenType::END_OF_FILE, "", 0, 0);
        }
    }

    ASTNode* factor() {
        if (currentToken.type == TokenType::NUM) {
            std::string value = currentToken.value;
            bool isFloat = value.find('.') != std::string::npos;
            advance();
            if (isFloat) {
                return new NumberNode(std::stod(value));
            } else {
                return new SignedIntNode(TokenType::INT, value);
            }
        } else if (currentToken.type == TokenType::IDENTIFIER) {
            std::string name = currentToken.value;
            advance();
            return new IdentifierNode(name);
        } else if (currentToken.type == TokenType::U8 ||
                   currentToken.type == TokenType::U16 ||
                   currentToken.type == TokenType::U32 ||
                   currentToken.type == TokenType::U64 ||
                   currentToken.type == TokenType::INT ||
                   currentToken.type == TokenType::FLOAT) {
            TokenType type = currentToken.type;
            std::string value = currentToken.value;
            advance();
            if (type == TokenType::INT) {
                return new SignedIntNode(type, value);
            } else if (type == TokenType::FLOAT) {
                return new FloatNode(type, value);
            } else {
                return new UnsignedIntNode(type, value);
            }
        } else if (currentToken.type == TokenType::LPAR) {
            advance();
            ASTNode* node = expr();
            if (currentToken.type != TokenType::RPAR) {
                throw std::runtime_error("Expected closing parenthesis at line " +
                    std::to_string(currentToken.line) + ", column " +
                    std::to_string(currentToken.column));
            }
            advance();
            return node;
        }
        throw std::runtime_error("Expected number, identifier, or parenthesis at line " +
            std::to_string(currentToken.line) + ", column " +
            std::to_string(currentToken.column));
    }
	
	ASTNode* statement() {
		if (currentToken.type == TokenType::IDENTIFIER) {
			std::string id = currentToken.value;
			advance();
			if (currentToken.type == TokenType::EQUAL) {
				advance();
				ASTNode* value = expr();
				return new AssignmentNode(id, value);
			}
			// if not an assignment, treat as expression
			pos--;
			currentToken = tokens[pos-1];
			return expr();
		}
		return expr();
	}

    ASTNode* power() {
        ASTNode* node = factor();
        while (currentToken.type == TokenType::EXP) {
            TokenType op = currentToken.type;
            advance();
            ASTNode* right = factor();
            node = new BinaryOpNode(op, node, right);
        }
        return node;
    }

    ASTNode* term() {
        ASTNode* node = power();
        while (currentToken.type == TokenType::MUL || currentToken.type == TokenType::DIV) {
            TokenType op = currentToken.type;
            advance();
            ASTNode* right = power();
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

    ASTNode* function() {
        if (currentToken.type != TokenType::FN) {
            throw std::runtime_error("Expected 'fn' at line " +
                std::to_string(currentToken.line) + ", column " +
                std::to_string(currentToken.column));
        }
        advance();
        if (currentToken.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected function name at line " +
                std::to_string(currentToken.line) + ", column " +
                std::to_string(currentToken.column));
        }
        std::string name = currentToken.value;
        advance();
        if (currentToken.type != TokenType::LPAR) {
            throw std::runtime_error("Expected '(' at line " +
                std::to_string(currentToken.line) + ", column " +
                std::to_string(currentToken.column));
        }
        advance();
        std::vector<std::string> params;
        std::vector<TokenType> paramTypes;
        std::vector<bool> paramIsConst;
        if (currentToken.type != TokenType::RPAR) {
            do {
                bool isConst = false;
                if (currentToken.type == TokenType::CONST) {
                    isConst = true;
                    advance();
                }
                TokenType paramType = TokenType::ERROR;
                if (currentToken.type == TokenType::U8 ||
                    currentToken.type == TokenType::U16 ||
                    currentToken.type == TokenType::U32 ||
                    currentToken.type == TokenType::U64 ||
                    currentToken.type == TokenType::INT ||
                    currentToken.type == TokenType::FLOAT) {
                    paramType = currentToken.type;
                    advance();
                }
                if (currentToken.type != TokenType::IDENTIFIER) {
                    throw std::runtime_error("Expected parameter name at line " +
                        std::to_string(currentToken.line) + ", column " +
                        std::to_string(currentToken.column));
                }
                params.push_back(currentToken.value);
                paramIsConst.push_back(isConst);
                if (paramType != TokenType::ERROR) {
                    paramTypes.push_back(paramType);
                }
                advance();
                if (currentToken.type == TokenType::COMMA) {
                    advance();
                } else {
                    break;
                }
            } while (true);
        }
        if (currentToken.type != TokenType::RPAR) {
            throw std::runtime_error("Expected ')' at line " +
                std::to_string(currentToken.line) + ", column " +
                std::to_string(currentToken.column));
        }
        advance();
        if (currentToken.type != TokenType::LBRACE) {
            throw std::runtime_error("Expected '{' at line " +
                std::to_string(currentToken.line) + ", column " +
                std::to_string(currentToken.column));
        }
        advance();
        std::vector<ASTNode*> body;
        while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::END_OF_FILE) {
            body.push_back(statement());
        }
        if (currentToken.type != TokenType::RBRACE) {
            throw std::runtime_error("Expected '}' at line " +
                std::to_string(currentToken.line) + ", column " +
                std::to_string(currentToken.column));
        }
        advance();
        return new FunctionNode(name, params, paramTypes, paramIsConst, body);
    }

public:
    Parser(const std::vector<Token>& t) : tokens(t), pos(0), currentToken(TokenType::END_OF_FILE, "", 0, 0) {
        if (!tokens.empty()) {
            currentToken = tokens[0];
            pos = 1;
        }
    }

    std::vector<ASTNode*> parse() {
        std::vector<ASTNode*> nodes;
        while (currentToken.type != TokenType::END_OF_FILE) {
            if (currentToken.type == TokenType::FN) {
                nodes.push_back(function());
            } else {
                nodes.push_back(expr());
            }
        }
        if (nodes.empty()) {
            throw std::runtime_error("Empty input");
        }
        return nodes;
    }
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";
        case TokenType::EXP: return "EXP";
        case TokenType::LPAR: return "LPAR";
        case TokenType::RPAR: return "RPAR";
        case TokenType::NUM: return "NUM";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::FN: return "FN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR: return "ERROR";
        case TokenType::U8: return "U8";
        case TokenType::U16: return "U16";
        case TokenType::U32: return "U32";
        case TokenType::U64: return "U64";
        case TokenType::CONST: return "CONST";
        case TokenType::INT: return "INT";
        case TokenType::FLOAT: return "FLOAT";
		case TokenType::EQUAL: return "EQUAL";
        default: return "UNKNOWN";
    }
}

void printAST(const ASTNode* node, int indent = 0) {
    if (!node) return;
    std::string indentStr(indent, ' ');
    if (node->type == NodeType::NUMBER) {
        const NumberNode* num = dynamic_cast<const NumberNode*>(node);
        std::cout << indentStr << "Number: " << num->value << "\n";
    } else if (node->type == NodeType::IDENTIFIER) {
        const IdentifierNode* id = dynamic_cast<const IdentifierNode*>(node);
        std::cout << indentStr << "Identifier: " << id->name << "\n";
    } else if (node->type == NodeType::BINARY_OP) {
        const BinaryOpNode* bin = dynamic_cast<const BinaryOpNode*>(node);
        std::cout << indentStr << "BinaryOp: " << tokenTypeToString(bin->op) << "\n";
        printAST(bin->left, indent + 2);
        printAST(bin->right, indent + 2);
    } else if (node->type == NodeType::FUNCTION) {
        const FunctionNode* func = dynamic_cast<const FunctionNode*>(node);
        std::cout << indentStr << "Function: " << func->name << "\n";
        std::cout << indentStr << "  Parameters:\n";
        for (size_t i = 0; i < func->params.size(); ++i) {
            std::string paramStr = func->paramIsConst[i] ? "const " : "";
            paramStr += (func->paramTypes.size() > i ? tokenTypeToString(func->paramTypes[i]) + " " : "") + func->params[i];
            std::cout << indentStr << "    - " << paramStr << "\n";
        }
        std::cout << indentStr << "  Body:\n";
        for (const ASTNode* stmt : func->body) {
            printAST(stmt, indent + 4);
        }
    } else if (node->type == NodeType::UNSIGNED_INT) {
        const UnsignedIntNode* uint = dynamic_cast<const UnsignedIntNode*>(node);
        std::cout << indentStr << "UnsignedInt: " << uint->value << "\n";
    } else if (node->type == NodeType::SIGNED_INT) {
        const SignedIntNode* sint = dynamic_cast<const SignedIntNode*>(node);
        std::cout << indentStr << "SignedInt: " << sint->value << "\n";
    } else if (node->type == NodeType::FLOAT) {
        const FloatNode* flt = dynamic_cast<const FloatNode*>(node);
        std::cout << indentStr << "Float: " << flt->value << "\n";
    } else if (node->type == NodeType::ASSIGNMENT) {
		const auto* assign = dynamic_cast<const AssignmentNode*>(node);
		std::cout << indentStr << "Assignment: " << assign->identifier << "\n";
		printAST(assign->value, indent + 2);
	}
}

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "-output") {
        std::cerr << "Usage: " << argv[0] << " -output <filename.l>\n";
        return 1;
    }

    std::string filename(argv[2]);
    if (filename.length() < 2 || filename.substr(filename.length() - 2) != ".l") {
        std::cerr << "Error: Input file must have .l extension\n";
        return 1;
    }

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file '" + filename + "'");
        }
        std::string src;
        std::string line;
        while (std::getline(file, line)) {
            src += line + '\n';
        }
        file.close();

        Lexer lexer(src);
        std::vector<Token> tokens = lexer.tokenize();

        std::cout << "\nTokens:\n";
        for (const auto& token : tokens) {
            std::cout << "Type: " << tokenTypeToString(token.type)
                      << ", Value: " << token.value
                      << ", Line: " << token.line
                      << ", Column: " << token.column << "\n";
        }

        Parser parser(tokens);
        std::vector<ASTNode*> asts = parser.parse();

        std::cout << "\nASTs:\n";
        for (size_t i = 0; i < asts.size(); ++i) {
            std::cout << "Node " << i + 1 << ":\n";
            printAST(asts[i]);
            std::cout << "\n";
        }
		
		CodeGenerator codeGen;
		std::string asmCode = codeGen.generateCode(asts);
		std::ofstream outFile("out.asm");
		outFile << asmCode;
		outFile.close();

        for (ASTNode* ast : asts) {
            delete ast;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}