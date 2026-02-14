# Raccoon Programming Language

A statically-typed programming language compiled to native code via LLVM.

## Features

- Control flow: if/else/else if statements
- Loops: for and while with break/continue
- Boolean operations and expressions
- Static type checking with explicit type annotations
- Variable declarations with `let`
- LLVM-based code generation for native performance
- Comprehensive test suite with unit tests for each compiler phase

## Building

### Prerequisites
- LLVM 21.1.6 with development headers
- CMake 3.20+
- C++23 compatible compiler

### Build Instructions
```bash
git clone https://github.com/floofyplasma/raccoon
cd raccoon
mkdir build && cd build
cmake ..
make
```

### Running Tests
```bash
cd build
ctest
```

## Usage
```bash
# Compile a Raccoon source file
./raccoonc input.rac

# The compiler generates an executable in the current directory
./a.out
```

## Current Status

**Working:**
- Lexer with full tokenization
- Recursive descent parser generating AST
- Semantic analysis with symbol table and type checking
- Custom intermediate representation (IR)
- LLVM backend for native code generation
- Control flow (if/else) and loops (for/while)
- Boolean operations and comparisons
- Variable declarations with type annotations
- Break/continue statements
- Unary operators (!boolean, -i32)

**In Progress:**
- Function definitions and calls (partial implementation)

## Technical Details

**Compiler Architecture**

Raccoon follows a traditional multi-pass compiler design:

1. **Lexer** (`libs/lexer`) - Tokenizes source code
2. **Parser** (`libs/parser`) - Builds Abstract Syntax Tree (AST)
3. **Semantic Analysis** (`libs/semantic`) - Type checking and symbol table management
4. **IR Generation** (`libs/ir`) - Generates custom intermediate representation
5. **LLVM Backend** (`libs/llvm_backend`) - Converts IR to LLVM IR and generates native code
6. **Linker** (`libs/linker`) - Links generated object files into executable

Each phase is implemented as a separate library with its own unit tests, making the codebase modular and maintainable.

**Type System:**

Raccoon uses explicit type annotations with a simple type system:
- `i32` - 32-bit signed integer
- `bool` - Boolean type
- More types planned (arrays, structs)

## Example
```raccoon
fun main(): i32 {
    let found: bool = false;

    for (let i: i32 = 0; i < 10; i = i + 1) {
        if (i == 7) {
            found = true;
            break;
        }
    }

    if (found) {
        return 1;
    } else {
        return 0;
    }

    return -1;
}
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
