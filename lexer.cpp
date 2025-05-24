#include <iostream>
#include <string>
#include <vector>
#include <cctype>

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
        while (pos < input.length() && isNumberChar(input[pos])) {
            numStr += input[pos];
            pos++;
        }
        return Token(TokenType::NUM, numStr);
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
                if (std::isdigit(current)) {
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

int main() {
    std::string input;
    std::cout << ": ";
    std::getline(std::cin, input);

    Lexer lexer(input);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "\nTokens:\n";
    for (const auto& token : tokens) {
        std::cout << "Type: " << tokenTypeToString(token.type)
                  << ", Value: " << token.value << "\n";
    }

    return 0;
}
