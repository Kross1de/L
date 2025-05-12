mod token;
mod lexer;

use crate::lexer::Lexer;
use crate::token::Token;

fn main() {
    let input = "12 + 34 - 5 * (67 / 8) ^ 2 % 3";
    let mut lexer = Lexer::new(input);
    let tokens = lexer.tokenize();

    for token in tokens {
        match token {
            Token::Plus => println!("+"),
            Token::Minus => println!("-"),
            Token::Mul => println!("*"),
            Token::Divide => println!("/"),
            Token::Power => println!("^"),
            Token::Modulo => println!("%"),
            Token::LParen => println!("("),
            Token::RParen => println!(")"),
            Token::Number(n) => println!("Number: {}", n),
        }
    }
}
