enum Token {
    Plus,
    Minus,
    Mul,
    Divide,
    Number(i64),
}

struct Lexer {
    input: Vec<char>,
    position: usize,
}

impl Lexer {
    fn new(input: &str) -> Lexer {
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

    fn tokenize(&mut self) -> Vec<Token> {
        let mut tokens = Vec::new();
        while let Some(token) = self.next_token() {
            tokens.push(token);
        }
        tokens
    }
}

fn main() {
    let input = "12 + 34 - 5 * 67 / 8";
    let mut lexer = Lexer::new(input);
    let tokens = lexer.tokenize();

    for token in tokens {
        match token {
            Token::Plus => println!("+"),
            Token::Minus => println!("-"),
            Token::Mul => println!("*"),
            Token::Divide => println!("/"),
            Token::Number(n) => println!("Number: {}", n),
        }
    }
}
