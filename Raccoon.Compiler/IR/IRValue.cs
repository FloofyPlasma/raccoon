namespace Raccoon.Compiler.IR;

public abstract class IRValue
{
    public abstract Type GetType();
}

public abstract class Type
{
    public abstract string ToCCode();
    public abstract int GetSize();
    public virtual bool IsCompatible(Type other) => this.GetType() == other.GetType();

    public override string ToString() => GetType().Name;
}

public class IntType : Type
{
    public int BitWidth { get; set; } = 32;

    public override string ToCCode() => BitWidth switch
    {
        8 => "int8_t",
        16 => "int16_t",
        32 => "int32_t",
        64 => "int64_t",
        _ => throw new InvalidOperationException($"Invalid int bitwidth: {BitWidth}"),
    };

    public override int GetSize() => BitWidth / 8;
}

public class FloatType : Type
{
    public int BitWidth { get; set; } = 32;

    public override string ToCCode() => BitWidth switch
    {
        32 => "float",
        64 => "double",

        _ => throw new InvalidOperationException($"Invalid float bitwidth: {BitWidth}"),
    };
    
    public override int GetSize() => BitWidth / 8;
}

public class BoolType : Type
{
    public override string ToCCode() => "int";
    public override int GetSize() => 1;
}

public class CharType : Type
{
    public override string ToCCode() => "char";
    public override int GetSize() => 1;
}

public class StringType : Type
{
    public override string ToCCode() => "char*";
    public override int GetSize() => 8; // pointer size
}

public class VoidType : Type
{
    public override string ToCCode() => "void";
    public override int GetSize() => 0;
}

public class PointerType : Type
{
    public Type PointsTo { get; set; }

    public PointerType(Type pointsTo)
    {
        PointsTo = pointsTo;
    }
    
    public override string ToCCode() => PointsTo.ToCCode() + "*";
    public override int GetSize() => 64;
}

public class ArrayType : Type
{
    public Type ElementType { get; set; }
    public int ElementCount  { get; set; }

    public ArrayType(Type elementType, int count)
    {
        ElementType = elementType;
        ElementCount = count;
    }

    public override string ToCCode() => $"{ElementType.ToCCode()}[{ElementCount}]";
    public override int GetSize() => 0;
}

public class FunctionType : Type
{
    public Type ReturnType { get; set; }
    public List<Type> ParameterTypes { get; set; } = [];

    public override string ToCCode() => throw new InvalidOperationException("Cannot directly emit function type as C");
    public override int GetSize() => 0;
}

public class ClassType : Type
{
    public string Name { get; set; }
    public Dictionary<string, Type> Fields { get; set; } = [];
    public Dictionary<string, FunctionType> Methods { get; set; } = [];

    public ClassType(string name)
    {
        Name = name;
    }

    public override string ToCCode() => $"struct {Name}";
    public override int GetSize() => Fields.Values.Sum(f => f.GetSize());
}