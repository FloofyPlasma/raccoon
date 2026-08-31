namespace Raccoon.Compiler;

using Raccoon.Compiler.IR;
using Raccoon.Compiler.Symbols;

public class Parser
{
    private List<Token> _tokens;
    private int _current = 0;
    private SymbolTable _symbols = new();
    private List<string> _errors = [];

    public Parser(List<Token> tokens)
    {
        _tokens = tokens;
    }

    public IRModule Parse()
    {
        var module = new IRModule();

        while (!IsAtEnd())
        {
            try
            {
                if (Check(TokenType.Fn))
                {
                    var func = ParseFunction();
                    if (func != null)
                    {
                        module.Functions.Add(func);
                    }
                }
                else if (Check(TokenType.Class))
                {
                    Advance(); // consume 'class'
                    var cls = ParseClass();
                    if (cls != null)
                    {
                        module.Classes.Add(cls);

                        foreach (var method in cls.Methods)
                        {
                            module.Functions.Add(method);
                        }
                    }
                }
                else if (Check(TokenType.Import))
                {
                    ParseImport();
                }
                else
                {
                    Error($"Unexpected token: {Peek().Lexeme}");
                    Advance();
                }
            }
            catch (Exception ex)
            {
                Error($"Parse error: {ex.Message}");
                Synchronize();
            }
        }

        return module;
    }

    private IRFunction? ParseFunction()
    {
        if (!Match(TokenType.Fn))
        {
            return null;
        }

        var nameToken = Consume(TokenType.Identifier, "Expected function name");
        string functionName = nameToken.Lexeme;

        var function = new IRFunction { Name = functionName };

        Consume(TokenType.Lparen, "Expected '(' after function name");

        _symbols.EnterScope();

        // Parse params
        while (!Check(TokenType.Rparen))
        {
            var paramNameToken = Consume(TokenType.Identifier, "Expected parameter name");
            string paramName = paramNameToken.Lexeme;

            Consume(TokenType.Colon, "Expected ':' after parameter name");

            Type paramType = ParseType();
            function.Parameters.Add((paramName, paramType));
            _symbols.Define(paramName, paramType);

            if (!Check(TokenType.Rparen))
            {
                Consume(TokenType.Comma, "Expected ',' between parameters");
            }
        }

        Consume(TokenType.Rparen, "Expected ')' after parameters");

        // Parse return type
        Type returnType = new VoidType();
        if (Match(TokenType.Arrow))
        {
            returnType = ParseType();
        }

        function.ReturnType = returnType;

        Consume(TokenType.Lbrace, "Expected '{' before function body");

        // Parse body
        var entryBlock = new IRBasicBlock { Label = "entry" };
        function.EntryBlock = entryBlock;
        function.Blocks.Add(entryBlock);

        ParseBlock(entryBlock, function);

        Consume(TokenType.Rbrace, "Expected '}' after function body");

        _symbols.ExitScope();

        return function;
    }

    private void ParseBlock(IRBasicBlock block, IRFunction function)
    {
        while (!Check(TokenType.Rbrace) && !IsAtEnd())
        {
            if (Check(TokenType.Rbrace) || Check(TokenType.Eof))
            {
                break;
            }
            else if (Match(TokenType.Return))
            {
                IRValue? returnValue = null;

                if (!Check(TokenType.Rbrace) && !Check(TokenType.Eof))
                {
                    returnValue = ParseExpression(block, function);
                }

                var returnInstr = new IRReturn { Value = returnValue };
                block.Instructions.Add(returnInstr);
                block.Terminator = returnInstr;

                Match(TokenType.Semicolon);
                break;
            }
            else if (Check(TokenType.Identifier))
            {
                int savePos = _current;
                Advance();

                if (Check(TokenType.Colon))
                {
                    _current = savePos;
                    ParseVariableDeclaration(block, function);
                }
                else
                {
                    _current = savePos;
                    var expr = ParseExpression(block, function);
                    Match(TokenType.Semicolon);
                }
            }
            else
            {
                var expr = ParseExpression(block, function);
                Match(TokenType.Semicolon);
                // Error($"Unexpected token in block: {Peek().Lexeme}");
                Advance();
            }
        }
    }

    private void ParseVariableDeclaration(IRBasicBlock block, IRFunction function)
    {
        var nameToken = Consume(TokenType.Identifier, "Expected variable name");
        string varName = nameToken.Lexeme;

        Consume(TokenType.Colon, "Expected ':' after variable name");
        Type varType = ParseType();

        _symbols.Define(varName, varType);
        block.LocalVariables[varName] = varType;

        IRValue? initialValue = null;
        if (Match(TokenType.Assign))
        {
            initialValue = ParseExpression(block, function);

            var assignInstr = new IRAssignment
            {
                VariableName = varName,
                Value = initialValue,
            };
            block.Instructions.Add(assignInstr);
        }

        Match(TokenType.Semicolon);
    }

    private IRValue ParseExpression(IRBasicBlock block, IRFunction function)
    {
        return ParseBinaryOp(block, function, 0);
    }

    private IRValue ParseBinaryOp(IRBasicBlock block, IRFunction function, int minPrecedence)
    {
        var left = ParsePrimary(block, function);

        while (IsBinaryOp(Peek()) && GetPrecedence(Peek()) >= minPrecedence)
        {
            string op = Advance().Lexeme;
            int precedence = GetPrecedence(_tokens[_current - 1]);

            var right = ParseBinaryOp(block, function, precedence + 1);

            var leftType = left.GetType();
            var rightType = right.GetType();

            // TODO: class safety stuff
            // if (!leftType.IsCompatible(rightType))
            // {
            //     Error($"Type mismatch: {leftType} and {rightType}");
            // }

            var binOp = new IRBinaryOp
            {
                Op = op,
                Left = left,
                Right = right,
                ResultType = leftType,
                ResultName = $"%tmp{block.Instructions.Count}"
            };

            block.Instructions.Add(binOp);
            left = binOp;
        }

        return left;
    }

    private IRValue ParsePrimary(IRBasicBlock block, IRFunction function)
    {
        var value = ParseAtom(block, function);

        while (Check(TokenType.Dot))
        {
            Advance(); // consume '.'
            string memberName;
            if (Check(TokenType.Identifier))
            {
                memberName = Advance().Lexeme;
            }
            else if (Check(TokenType.Init))
            {
                memberName = "init";
                Advance();
            }
            else
            {
                Error("Expected member or method name");
                return value;
            }

            if (Check(TokenType.Lparen))
            {
                Advance(); // consume '('
                var args = new List<IRValue>();
                args.Add(value); // 'self' is first argument

                while (!Check(TokenType.Rparen) && !IsAtEnd())
                {
                    args.Add(ParseExpression(block, function));
                    if (!Check(TokenType.Rparen))
                    {
                        Consume(TokenType.Comma, "Expected ',' between arguments");
                    }
                }

                Consume(TokenType.Rparen, "Expected ')' after arguments");

                var callInstr = new IRCall
                {
                    FunctionName = $"{GetTypeNameFromValue(value)}_{memberName}",
                    Arguments = args,
                    ReturnType = new IntType { BitWidth = 32 },
                    ResultName = $"%call{block.Instructions.Count}",
                };

                block.Instructions.Add(callInstr);
                value = callInstr;
            }
            else
            {
                var fieldAccess = new IRMemberAccess
                {
                    Object = value,
                    MemberName = memberName,
                    ResultName = $"%member{block.Instructions.Count}",
                };

                block.Instructions.Add(fieldAccess);
                value = fieldAccess;
            }
        }

        return value;
    }

    private IRValue ParseAtom(IRBasicBlock block, IRFunction function)
    {
        if (Match(TokenType.Int))
        {
            var value = (long)_tokens[_current - 1].Literal!;
            return new IRConstant
            {
                Value = value,
                ValueType = new IntType { BitWidth = 32 },
            };
        }

        if (Match(TokenType.Float))
        {
            var value = (double)_tokens[_current - 1].Literal!;
            return new IRConstant
            {
                Value = value,
                ValueType = new FloatType { BitWidth = 32 },
            };
        }

        if (Match(TokenType.String))
        {
            var value = (string)_tokens[_current - 1].Literal!;
            return new IRConstant
            {
                Value = value,
                ValueType = new StringType()
            };
        }

        if (Match(TokenType.Self))
        {
            var symbol = _symbols.Lookup("self");
            if (symbol == null)
            {
                Error("'self' not available in this context");
                return new IRConstant { Value = 0, ValueType = new IntType() };
            }

            return new IRVariable
            {
                Name = "self",
                VariableType = symbol.Type
            };
        }

        if (Match(TokenType.Identifier))
        {
            string name = _tokens[_current - 1].Lexeme;

            if (Check(TokenType.Lparen))
            {
                Advance(); // consume '('
                var args = new List<IRValue>();

                while (!Check(TokenType.Rparen) && !IsAtEnd())
                {
                    args.Add(ParseExpression(block, function));
                    if (!Check(TokenType.Rparen))
                    {
                        Consume(TokenType.Comma, "Expected ',' between arguments");
                    }
                }

                Consume(TokenType.Rparen, "Expected ')' after arguments");

                // TODO: track function signatures properly
                var callInstr = new IRCall
                {
                    FunctionName = name,
                    Arguments = args,
                    ReturnType = new IntType { BitWidth = 32 },
                    ResultName = $"%call{block.Instructions.Count}",
                };

                block.Instructions.Add(callInstr);
                return callInstr;
            }

            var symbol = _symbols.Lookup(name);

            if (symbol == null)
            {
                Error($"Undefined variable: {name}");
                return new IRConstant { Value = 0, ValueType = new IntType() };
            }

            return new IRVariable
            {
                Name = name,
                VariableType = symbol.Type,
            };
        }

        if (Match(TokenType.Lparen))
        {
            var expr = ParseExpression(block, function);
            Consume(TokenType.Rparen, "Expected ')' after arguments");
            return expr;
        }

        Error($"Unexpected token: {Peek().Lexeme}");
        return new IRConstant { Value = 0, ValueType = new IntType() };
    }

    private string GetTypeNameFromValue(IRValue value)
    {
        var valueType = value.GetType();
        if (valueType is PointerType ptr && ptr.PointsTo is ClassType cls)
        {
            return cls.Name;
        }

        if (valueType is ClassType classType)
        {
            return classType.Name;
        }

        return "unknown";
    }

    private Type ParseType()
    {
        if (Match(TokenType.Identifier))
        {
            string typeName = _tokens[_current - 1].Lexeme;
            return typeName switch
            {
                "i32" => new IntType { BitWidth = 32 },
                "i64" => new IntType { BitWidth = 64 },
                "f32" => new FloatType { BitWidth = 32 },
                "f64" => new FloatType { BitWidth = 64 },
                "bool" => new BoolType(),
                "char" => new CharType(),
                "void" => new VoidType(),
                _ => new ClassType(typeName)
            };
        }

        if (Match(TokenType.Star))
        {
            Type pointsTo = ParseType();
            return new PointerType(pointsTo);
        }

        Error("Expected type");
        return new IntType();
    }

    private IRClass? ParseClass()
    {
        var nameToken = Consume(TokenType.Identifier, "Expected class name");
        string className = nameToken.Lexeme;

        Consume(TokenType.Lbrace, "Expected '{' after class name");

        var irClass = new IRClass { Name = className };
        var classType = new ClassType(className);

        _symbols.EnterScope();

        while (!Check(TokenType.Rbrace) && !IsAtEnd())
        {
            if (Match(TokenType.Init))
            {
                var method = ParseMethod(className, "init");
                if (method != null)
                {
                    irClass.Methods.Add(method);
                }
            }
            else if (Check(TokenType.Identifier))
            {
                int savePos = _current;
                var nameToken2 = Advance();
                string memberName = nameToken2.Lexeme;

                if (Check(TokenType.Colon))
                {
                    // Field decl
                    _current = savePos;
                    var fieldNameToken = Consume(TokenType.Identifier, "Expected field name");
                    string fieldName = fieldNameToken.Lexeme;

                    Consume(TokenType.Colon, "Expected ':' after field name");
                    Type fieldType = ParseType();

                    irClass.Fields.Add((fieldName, fieldType));
                    classType.Fields[fieldName] = fieldType;

                    Match(TokenType.Semicolon);
                }
                else if (Check(TokenType.Lparen))
                {
                    _current = savePos;
                    var nameTokenMethod = Consume(TokenType.Identifier, "Expected method name");
                    string methodName = nameTokenMethod.Lexeme;

                    var method = ParseMethod(className, methodName);
                    if (method != null)
                    {
                        irClass.Methods.Add(method);
                    }
                }
                else
                {
                    Error($"Unexpected in class: expected ':' or '(' after {memberName}");
                    Advance();
                }
            }
            else
            {
                Error($"Unexpected token in class: {Peek().Lexeme}");
                Advance();
            }
        }

        Consume(TokenType.Rbrace, "Expected '}' after class body");

        _symbols.ExitScope();

        return irClass;
    }

    private void ParseImport()
    {
        // TODO: Implement import parsin
        Match(TokenType.Import);
    }

    private int GetPrecedence(Token token)
    {
        return token.Type switch
        {
            TokenType.Or => 1,
            TokenType.And => 2,
            TokenType.Eq or TokenType.Ne => 3,
            TokenType.Lt or TokenType.Le or TokenType.Gt or TokenType.Ge => 4,
            TokenType.Plus or TokenType.Minus => 5,
            TokenType.Star or TokenType.Slash or TokenType.Percent => 6,
            _ => 0,
        };
    }

    private bool IsBinaryOp(Token token)
    {
        return token.Type switch
        {
            TokenType.Plus or TokenType.Minus or TokenType.Star or TokenType.Slash or TokenType.Percent or TokenType.Eq
                or TokenType.Ne or TokenType.Lt or TokenType.Le or TokenType.Gt or TokenType.Ge or TokenType.And
                or TokenType.Or => true,
            _ => false,
        };
    }

    private bool Match(TokenType type)
    {
        if (Check(type))
        {
            Advance();
            return true;
        }

        return false;
    }

    private bool Check(TokenType type) => Peek().Type == type;

    private Token Peek() => _tokens[_current >= _tokens.Count ? _tokens.Count - 1 : _current];

    private Token Advance() => _tokens[_current++];

    private bool IsAtEnd() => _current >= _tokens.Count || _tokens[_current].Type == TokenType.Eof;

    private Token Consume(TokenType type, string message)
    {
        if (Check(type))
        {
            return Advance();
        }

        Error(message);
        return Peek();
    }

    private void Error(string message)
    {
        _errors.Add($"Parse error at {Peek().Line}:{Peek().Column}: {message}");
    }

    private void Synchronize()
    {
        Advance();

        while (!IsAtEnd())
        {
            if (Peek().Type == TokenType.Fn || Peek().Type == TokenType.Class)
            {
                return;
            }

            Advance();
        }
    }

    public List<string> Errors => _errors;

    private IRFunction? ParseMethod(string className, string methodName)
    {
        Consume(TokenType.Lparen, "Expected '(' after method name");

        var method = new IRFunction { Name = $"{className}_{methodName}" };

        _symbols.EnterScope();

        var selfType = new ClassType(className);
        method.Parameters.Add(("self", new PointerType(selfType)));
        _symbols.Define("self", selfType);

        while (!Check(TokenType.Rparen))
        {
            var paramNameToken = Consume(TokenType.Identifier, "Expected parameter name");
            string paramName = paramNameToken.Lexeme;

            Consume(TokenType.Colon, "Expected ':' after parameter name");
            Type paramType = ParseType();

            method.Parameters.Add((paramName, paramType));
            _symbols.Define(paramName, paramType);

            if (!Check(TokenType.Rparen))
            {
                Consume(TokenType.Comma, "Expected ',' between parameters");
            }
        }

        Consume(TokenType.Rparen, "Expected ')' after parameters");

        Type returnType = new VoidType();
        if (methodName != "init" && Match(TokenType.Arrow))
        {
            returnType = ParseType();
        }

        method.ReturnType = returnType;

        Consume(TokenType.Lbrace, "Expected '{' before method body");

        var entryBlock = new IRBasicBlock { Label = "entry" };
        method.EntryBlock = entryBlock;
        method.Blocks.Add(entryBlock);

        ParseBlock(entryBlock, method);

        Consume(TokenType.Rbrace, "Expected '}' after method body");

        _symbols.ExitScope();

        return method;
    }
}