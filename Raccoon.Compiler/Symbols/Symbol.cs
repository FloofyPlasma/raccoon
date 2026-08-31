namespace Raccoon.Compiler.Symbols;

using Raccoon.Compiler.IR;

public class Symbol
{
    public string Name { get; set; } = "";
    public Type Type { get; set; } = null!;
    public int ScopeDepth { get; set; }
}

public class SymbolTable
{
    private Stack<Dictionary<string, Symbol>> _scopes = [];
    private int _scopeDepth = 0;

    public SymbolTable()
    {
        EnterScope();
    }

    public void EnterScope()
    {
        _scopes.Push([]);
        _scopeDepth++;
    }

    public void ExitScope()
    {
        _scopes.Pop();
        _scopeDepth--;
    }

    public void Define(string name, Type type)
    {
        var currentScope = _scopes.Peek();
        currentScope[name] = new Symbol
        {
            Name = name,
            Type = type,
            ScopeDepth = _scopeDepth,
        };
    }

    public Symbol? Lookup(string name)
    {
        foreach (var scope in _scopes.Reverse())
        {
            if (scope.TryGetValue(name, out var symbol))
            {
                return symbol;
            }
        }

        return null;
    }

    public bool IsDefined(string name) => Lookup(name) != null;
}