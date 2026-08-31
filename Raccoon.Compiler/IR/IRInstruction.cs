namespace Raccoon.Compiler.IR;

public abstract class IRInstruction : IRValue
{
    public string? ResultName { get; set; }
}

public class IRConstant : IRValue
{
    public object? Value { get; set; }
    public Type? ValueType { get; set; }

    public override Type GetType() => ValueType ?? new IntType();
}

public class IRVariable : IRValue
{
    public string Name { get; set; } = "";
    public Type? VariableType { get; set; }

    public override Type GetType() => VariableType ?? new IntType();
}

public class IRBinaryOp : IRInstruction
{
    public string Op { get; set; } = "";
    public IRValue Left { get; set; } = null!;
    public IRValue Right { get; set; } = null!;
    public Type? ResultType { get; set; }

    public override Type GetType() => ResultType ?? new IntType();
}

public class IRUnaryOp : IRInstruction
{
    public string Op { get; set; } = "";
    public IRValue Operand { get; set; } = null!;
    public Type? ResultType { get; set; }

    public override Type GetType() => ResultType ?? new IntType();
}

public class IRLoad : IRInstruction
{
    public IRValue Address { get; set; } = null!;
    public Type? LoadType { get; set; }

    public override Type GetType() => LoadType ?? new IntType();
}

public class IRStore : IRInstruction
{
    public IRValue Value { get; set; } = null!;
    public IRValue Address { get; set; } = null!;

    public override Type GetType() => new VoidType();
}

public class IRCall : IRInstruction
{
    public string FunctionName { get; set; } = "";
    public List<IRValue> Arguments { get; set; } = new();
    public Type? ReturnType { get; set; }

    public override Type GetType() => ReturnType ?? new VoidType();
}

public class IRReturn : IRInstruction
{
    public IRValue? Value { get; set; }

    public override Type GetType() => new VoidType();
}

public class IRBranch : IRInstruction
{
    public IRValue? Condition { get; set; }
    public string TrueLabel { get; set; } = "";
    public string FalseLabel { get; set; } = "";

    public override Type GetType() => new VoidType();
}

public class IRAssignment : IRInstruction
{
    public string VariableName { get; set; } = "";
    public IRValue Value { get; set; }
    
    public override Type GetType() => new VoidType();
}

public class IRBasicBlock
{
    public string Label { get; set; } = "";
    public List<IRInstruction> Instructions { get; set; } = [];
    public IRInstruction? Terminator { get; set; }
    public Dictionary<string, Type> LocalVariables { get; set; } = [];

    public override string ToString() => $"BasicBlock({Label})";
}

public class IRFunction
{
    public string Name { get; set; } = "";
    public Type ReturnType { get; set; } = new VoidType();
    public List<(string name, Type type)> Parameters { get; set; } = [];
    public List<IRBasicBlock> Blocks { get; set; } = [];
    public IRBasicBlock? EntryBlock { get; set; }

    private int _blockCounter = 0;

    public IRBasicBlock CreateBasicBlock(string label = "")
    {
        if (string.IsNullOrEmpty(label))
        {
            label = $"bb{_blockCounter++}";
        }

        var block = new IRBasicBlock { Label = label };
        Blocks.Add(block);
        return block;
    }

    public override string ToString() => $"Function({Name})";
}

public class IRModule
{
    public List<IRFunction> Functions { get; set; } = [];
    public Dictionary<string, ClassType> Types { get; set; } = [];

    public override string ToString() => $"Module({Functions.Count} functions)";
}