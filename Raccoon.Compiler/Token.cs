namespace Raccoon.Compiler;

public class Token
{
    public TokenType Type { get; set; }
    public string Lexeme { get; set; } = "";
    public object? Literal { get; set; }
    public int Line { get; set; }
    public int Column { get; set; }

    public override string ToString() => $"Token({Type}, '{Lexeme}', line {Line})";
}