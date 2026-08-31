namespace Raccoon.Compiler;

using System.Text;
using Raccoon.Compiler.IR;

public class Codegen
{
    private StringBuilder _output = new();

    public string Generate(IRModule module)
    {
        _output.Clear();

        _output.AppendLine("#include <stdio.h>");
        _output.AppendLine("#include <stdlib.h>");
        _output.AppendLine("#include <stdint.h>");
        _output.AppendLine();

        EmitTypeDefinitions(module);
        _output.AppendLine();

        foreach (var function in module.Functions)
        {
            EmitFunctionDeclaration(function);
        }

        _output.AppendLine();

        foreach (var function in module.Functions)
        {
            EmitFunction(function);
            _output.AppendLine();
        }

        return _output.ToString();
    }

    private void EmitTypeDefinitions(IRModule module)
    {
        foreach (var typeEntry in module.Types)
        {
            var classType = typeEntry.Value;
            _output.AppendLine($"typedef struct {classType.Name} {{");

            foreach (var field in classType.Fields)
            {
                _output.AppendLine("\t{field.Value.ToCCode()} {field.Key};");
            }

            _output.AppendLine($"}} {classType.Name}");
        }
    }

    private void EmitFunctionDeclaration(IRFunction function)
    {
        _output.Append($"{function.ReturnType.ToCCode()} {function.Name}(");

        for (int i = 0; i < function.Parameters.Count; i++)
        {
            if (i > 0)
            {
                _output.Append(", ");
            }

            var (name, type) = function.Parameters[i];
            _output.Append($"{type.ToCCode()} {name}");
        }

        _output.AppendLine(");");
    }

    private void EmitFunction(IRFunction function)
    {
        _output.Append($"{function.ReturnType.ToCCode()} {function.Name}(");

        for (int i = 0; i < function.Parameters.Count; i++)
        {
            if (i > 0)
            {
                _output.Append(", ");
            }

            var (name, type) = function.Parameters[i];
            _output.Append($"{type.ToCCode()} {name}");
        }

        _output.AppendLine(") {");

        var allLocals = new Dictionary<string, Type>();
        foreach (var block in function.Blocks)
        {
            foreach (var local in block.LocalVariables)
            {
                allLocals[local.Key] = local.Value;
            }
        }

        foreach (var local in allLocals)
        {
            _output.AppendLine($"\t{local.Value.ToCCode()} {local.Key};");
        }

        var tmpVars = new HashSet<string>();
        foreach (var block in function.Blocks)
        {
            foreach (var instr in block.Instructions)
            {
                if (instr.ResultName != null && !tmpVars.Contains(instr.ResultName))
                {
                    string tmpName = instr.ResultName.TrimStart('%');
                    _output.AppendLine($"\tint32_t {tmpName};");
                    tmpVars.Add(instr.ResultName);
                }
            }
        }

        if (allLocals.Count > 0 || tmpVars.Count > 0)
        {
            _output.AppendLine();
        }

        foreach (var block in function.Blocks)
        {
            if (block != function.EntryBlock)
            {
                _output.AppendLine($"{block.Label}:");
            }

            EmitBasicBlock(block);
        }

        _output.AppendLine("}");
    }

    private void EmitBasicBlock(IRBasicBlock block)
    {
        foreach (var instr in block.Instructions)
        {
            EmitInstruction(instr);
        }

        if (block.Terminator != null && block.Terminator is not IRReturn)
        {
            EmitInstruction(block.Terminator);
        }
    }

    private void EmitInstruction(IRInstruction instr)
    {
        switch (instr)
        {
            case IRBinaryOp binOp:
                EmitBinaryOp(binOp);
                break;

            case IRUnaryOp unaryOp:
                EmitUnaryOp(unaryOp);
                break;

            case IRLoad load:
                EmitLoad(load);
                break;

            case IRStore store:
                EmitStore(store);
                break;

            case IRCall call:
                EmitCall(call);
                break;

            case IRReturn ret:
                EmitReturn(ret);
                break;

            case IRBranch branch:
                EmitBranch(branch);
                break;
            
            case IRAssignment assign:
                EmitAssignment(assign);
                break;

            default:
                _output.AppendLine($"\t// Unknown instruction: {instr.GetType().ToString()}");
                break;
        }
    }

    private void EmitAssignment(IRAssignment assign)
    {
        string valueStr = ValueToString(assign.Value);
        _output.AppendLine($"\t{assign.VariableName} = {valueStr};");
    }

    private void EmitBinaryOp(IRBinaryOp binOp)
    {
        string op = binOp.Op switch
        {
            "+" => "+",
            "-" => "-",
            "*" => "*",
            "/" => "/",
            "%" => "%",
            "==" => "==",
            "!=" => "!=",
            "<" => "<",
            "<=" => "<=",
            ">" => ">",
            ">=" => ">=",
            "&&" => "&&",
            "||" => "||",
            _ => binOp.Op,
        };

        string leftStr = ValueToString(binOp.Left);
        string rightStr = ValueToString(binOp.Right);
        string resultName = binOp.ResultName?.TrimStart('%') ?? "tmp";

        _output.AppendLine($"    {resultName} = {leftStr} {op} {rightStr};");
    }

    private void EmitUnaryOp(IRUnaryOp unaryOp)
    {
        string op = unaryOp.Op;
        string operandStr = ValueToString(unaryOp.Operand);

        _output.AppendLine($"\t{unaryOp.ResultName} = {op}{operandStr};");
    }

    private void EmitLoad(IRLoad load)
    {
        string addrStr = ValueToString(load.Address);
        _output.AppendLine($"\t{load.ResultName} = *{addrStr};");
    }

    private void EmitStore(IRStore store)
    {
        string valueStr = ValueToString(store.Value);
        string addrStr = ValueToString(store.Address);
        _output.AppendLine("\t*{addrStr} = {valueStr};");
    }

    private void EmitCall(IRCall call)
    {
        var args = string.Join(", ", call.Arguments.Select(ValueToString));
        string resultName = call.ResultName?.TrimStart('%') ?? "tmp";

        if (call.ReturnType is VoidType)
        {
            _output.AppendLine($"    {call.FunctionName}({args});");
        }
        else
        {
            _output.AppendLine($"    {resultName} = {call.FunctionName}({args});");
        }
    }

    private void EmitReturn(IRReturn ret)
    {
        if (ret.Value == null)
        {
            _output.AppendLine($"\treturn;");
        }
        else
        {
            string valueStr = ValueToString(ret.Value);
            _output.AppendLine($"\treturn {valueStr};");
        }
    }

    private void EmitBranch(IRBranch branch)
    {
        if (branch.Condition == null)
        {
            _output.AppendLine($"\t goto {branch.TrueLabel};");
        }
        else
        {
            string condStr = ValueToString(branch.Condition);
            _output.AppendLine($"\tif ({condStr}) {{");
            _output.AppendLine($"\t\tgoto {branch.TrueLabel};");
            _output.AppendLine("\t} else {");
            _output.AppendLine($"\t\tgoto {branch.FalseLabel};");
            _output.AppendLine("\t}");
        }
    }

    private string ValueToString(IRValue value)
    {
        return value switch
        {
            IRConstant constant => constant.Value?.ToString() ?? "0",
            IRVariable variable => variable.Name,
            IRInstruction instr => (instr.ResultName ?? "tmp").TrimStart('%'),
            _ => "unknown",
        };
    }
}