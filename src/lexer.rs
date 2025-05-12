use crate::token::Token;

pub struct Lexer {
    input: Vec<char>,
    position: usize,
}

impl Lexer {
    pub fn new(input: &str) -> Lexer {
        Lexer {
            input: input.chars().collect(),
            position: 0,
        }
    }

    fn next_token(&mut self) -> Option<Token> {
        self.skip_whitespace();

        if self.position >= self.input.len() {
            return None;
        }

        let ch = self.input[self.position];
        self.position += 1;

        match ch {
            '+' => Some(Token::Plus),
            '-' => Some(Token::Minus),
            '*' => Some(Token::Mul),
            '/' => Some(Token::Divide),
            '^' => Some(Token::Power),
            '%' => Some(Token::Modulo),
            '(' => Some(Token::LParen),
            ')' => Some(Token::RParen),
            '0'..='9' => {
                let mut number = ch.to_digit(10).unwrap() as i64;
                while self.position < self.input.len() && self.input[self.position].is_digit(10) {
                    number = number * 10 + self.input[self.position].to_digit(10).unwrap() as i64;
                    self.position += 1;
                }
                Some(Token::Number(number))
            }
            _ => None,
        }
    }

    fn skip_whitespace(&mut self) {
        while self.position < self.input.len() && self.input[self.position].is_whitespace() {
            self.position += 1;
        }
    }

    pub fn tokenize(&mut self) -> Vec<Token> {
        let mut tokens = Vec::new();
        while let Some(token) = self.next_token() {
            tokens.push(token);
        }
        tokens
    }
}
