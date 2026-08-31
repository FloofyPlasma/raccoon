namespace Raccoon.Compiler;

public class Lexer
{
    private string _source;
    private int _current = 0;
    private int _line = 1;
    private int _column = 1;
    private List<string> _errors = [];

    private static readonly Dictionary<string, TokenType> Keywords = new()
    {
        { "fn", TokenType.Fn },
        { "class", TokenType.Class },
        { "init", TokenType.Init },
        { "if", TokenType.If },
        { "else", TokenType.Else },
        { "while", TokenType.While },
        { "for", TokenType.For },
        { "in", TokenType.In },
        { "return", TokenType.Return },
        { "template", TokenType.Template },
        { "import", TokenType.Import },
        { "module", TokenType.Module },
        { "asm", TokenType.Asm },
        { "extern", TokenType.Extern },
        { "self", TokenType.Self },
        { "true", TokenType.True },
        { "false", TokenType.False },
    };

    public Lexer(string source)
    {
        _source = source;
    }

    public List<Token> Tokenize()
    {
        var tokens = new List<Token>();

        while (!IsAtEnd())
        {
            SkipWhitespaceAndComments();
            if (IsAtEnd())
            {
                break;
            }

            var startColumn = _column;
            char c = Peek();

            Token? token = c switch
            {
                '(' => MakeToken(TokenType.Lparen, "("),
                ')' => MakeToken(TokenType.Rparen, ")"),
                '{' => MakeToken(TokenType.Lbrace, "{"),
                '}' => MakeToken(TokenType.Rbrace, "}"),
                '[' => MakeToken(TokenType.Lbracket, "["),
                ']' => MakeToken(TokenType.Rbracket, "]"),
                ':' => MakeToken(TokenType.Colon, ":"),
                ',' => MakeToken(TokenType.Comma, ","),
                '.' => MakeToken(TokenType.Dot, "."),
                ';' => MakeToken(TokenType.Semicolon, ";"),
                '+' => MakeToken(TokenType.Plus, "+"),
                '/' => MakeToken(TokenType.Slash, "/"),
                '%' => MakeToken(TokenType.Percent, "%"),

                '*' => MakeToken(TokenType.Star, "*"),
                '-' => Peek(1) == '>' ? MakeToken(TokenType.Arrow, "->") : MakeToken(TokenType.Minus, "-"),
                '=' => Peek(1) == '=' ? MakeToken(TokenType.Eq, "==") : MakeToken(TokenType.Assign, "="),
                '!' => Peek(1) == '=' ? MakeToken(TokenType.Ne, "!=") : MakeToken(TokenType.Not, "!"),
                '<' => Peek(1) == '=' ? MakeToken(TokenType.Le, "<=") : MakeToken(TokenType.Lt, "<"),
                '>' => Peek(1) == '=' ? MakeToken(TokenType.Ge, ">=") : MakeToken(TokenType.Gt, ">"),
                '&'  => Peek(1) == '&' ? MakeToken(TokenType.And, "&&") : MakeToken(TokenType.Ampersand, "&"),
                '|' when Peek(1) == '|' => MakeToken(TokenType.Or, "||"),

                '"' => ScanString(),
                _ when char.IsDigit(c) => ScanNumber(),
                _ when char.IsLetter(c) || c == '_' => ScanIdentifier(),
                _ => ErrorToken($"Unexpected character: '{c}'"),
            };

            if (token != null)
            {
                tokens.Add(token);
            }
        }

        tokens.Add(new Token { Type = TokenType.Eof, Line = _line, Column = _column });
        return tokens;
    }

    private Token? MakeToken(TokenType type, string lexeme)
    {
        Advance(lexeme.Length);
        return new Token
        {
            Type = type,
            Lexeme = lexeme,
            Line = _line,
            Column = _column - lexeme.Length
        };
    }

    private Token? ScanString()
    {
        var startLine = _line;
        var startColumn = _column;
        Advance(); // Consume opening quote

        var value = new System.Text.StringBuilder();

        while (!IsAtEnd() && Peek() != '"')
        {
            if (Peek() == '\n')
            {
                _line++;
                _column = 0;
            }

            if (Peek() == '\\')
            {
                Advance();
                char escaped = Peek();
                char result = escaped switch
                {
                    'n' => '\n',
                    't' => '\t',
                    'r' => '\r',
                    '\\' => '\\',
                    '"' => '"',
                    _ => escaped,
                };
                value.Append(result);
                Advance();
            }
            else
            {
                value.Append(Peek());
                Advance();
            }
        }

        if (IsAtEnd())
        {
            _errors.Add($"Unterminated string at line {startLine}, column {startColumn}");
            return ErrorToken("Unterminated string");
        }

        Advance(); // consume closing quote
        return new Token
        {
            Type = TokenType.String,
            Lexeme = value.ToString(),
            Literal = value.ToString(),
            Line = startLine,
            Column = startColumn
        };
    }

    private Token? ScanNumber()
    {
        var startColumn = _column;
        var number = new System.Text.StringBuilder();

        while (!IsAtEnd() && char.IsDigit(Peek()))
        {
            number.Append(Peek());
            Advance();
        }

        // Check for float
        if (!IsAtEnd() && Peek() == '.' && char.IsDigit(Peek(1)))
        {
            number.Append(Peek());
            Advance();

            while (!IsAtEnd() && char.IsDigit(Peek()))
            {
                number.Append(Peek());
                Advance();
            }

            return new Token
            {
                Type = TokenType.Float,
                Lexeme = number.ToString(),
                Literal = double.Parse(number.ToString()),
                Line = _line,
                Column = startColumn,
            };
        }

        return new Token
        {
            Type = TokenType.Int,
            Lexeme = number.ToString(),
            Literal = long.Parse(number.ToString()),
            Line = _line,
            Column = startColumn,
        };
    }

    private Token? ScanIdentifier()
    {
        var startColumn = _column;
        var identifier = new System.Text.StringBuilder();

        while (!IsAtEnd() && (char.IsLetterOrDigit(Peek()) || Peek() == '_'))
        {
            identifier.Append(Peek());
            Advance();
        }

        var text = identifier.ToString();
        var type = Keywords.ContainsKey(text) ? Keywords[text] : TokenType.Identifier;

        return new Token
        {
            Type = type,
            Lexeme = text,
            Line = _line,
            Column = startColumn,
        };
    }

    private Token? ErrorToken(string message)
    {
        _errors.Add($"Error at line {_line}: {message}");
        return null;
    }

    private void SkipWhitespaceAndComments()
    {
        while (!IsAtEnd())
        {
            char c = Peek();

            if (c == ' ' || c == '\t' || c == '\r')
            {
                Advance();
            }
            else if (c == '\n')
            {
                _line++;
                _column = 0;
                Advance();
            }
            else if (c == '/' && Peek(1) == '/')
            {
                // Skip line comment
                while (!IsAtEnd() && Peek() != '\n')
                {
                    Advance();
                }
            }
            else if (c == '/' && Peek(1) == '*')
            {
                // Skip block comment
                Advance(); // /
                Advance(); // *
                while (!IsAtEnd() && !(Peek() == '*' && Peek(1) == '/'))
                {
                    if (Peek() == '\n')
                    {
                        _line++;
                        _column = 0;
                    }

                    Advance();
                }

                if (!IsAtEnd())
                {
                    Advance(); // *
                    Advance(); // /
                }
            }
            else
            {
                break;
            }
        }
    }

    private char Peek(int offset = 0)
    {
        var pos = _current + offset;
        return pos >= _source.Length ? '\0' : _source[pos];
    }

    private void Advance(int count = 1)
    {
        for (var i = 0; i < count; i++)
        {
            if (_current < _source.Length)
            {
                _current++;
                _column++;
            }
        }
    }

    private bool IsAtEnd() => _current >= _source.Length;

    public List<string> Errors => _errors;
}