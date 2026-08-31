namespace Raccoon.Compiler;

public enum TokenType
{
    // Literals
    Int,
    Float,
    String,
    Identifier,

    // Keywords
    Fn,
    Class,
    Init,
    If,
    Else,
    While,
    For,
    In,
    Return,
    Template,
    Import,
    Module,
    Asm,
    Extern,
    Self,
    True,
    False,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
    Not,
    Assign,
    Arrow,
    Ampersand,

    // Punctuation
    Lparen,
    Rparen,
    Lbrace,
    Rbrace,
    Lbracket,
    Rbracket,
    Colon,
    Comma,
    Dot,
    Semicolon,

    Eof,
    Error
}