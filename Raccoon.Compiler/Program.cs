namespace Raccoon.Compiler;

class Program
{
    static void Main(string[] args)
    {
        if (args.Length == 0)
        {
            Console.WriteLine("Usage: raccoon <file.rac>");
            Environment.Exit(1);
        }

        var sourceFile = args[0];

        if (!File.Exists(sourceFile))
        {
            Console.Error.WriteLine($"Error: File not found: {sourceFile}");
            Environment.Exit(1);
        }

        try
        {
            string source = File.ReadAllText(sourceFile);

            Console.WriteLine($"[1/3] Lexing {sourceFile}");
            var lexer = new Lexer(source);
            var tokens = lexer.Tokenize();

            if (lexer.Errors.Count > 0)
            {
                Console.Error.WriteLine("Lexer errors:");
                foreach (var error in lexer.Errors)
                {
                    Console.Error.WriteLine($"\t{error}");
                }

                Environment.Exit(1);
            }

            Console.WriteLine($"[2/3] Parsing ({tokens.Count} tokens)...");
            var parser = new Parser(tokens);
            var module = parser.Parse();

            if (parser.Errors.Count > 0)
            {
                Console.Error.WriteLine("Parser errors:");
                foreach (var error in parser.Errors)
                {
                    Console.Error.WriteLine($"\t{error}");
                }

                Environment.Exit(1);
            }

            Console.WriteLine($"[3/3] Generating C code ({module.Functions.Count} functions)...");
            var codegen = new Codegen();
            string cCode = codegen.Generate(module);

            string cOutputFile = Path.ChangeExtension(sourceFile, ".c");
            File.WriteAllText(cOutputFile, cCode);
            Console.WriteLine($"\tGenerated: {cOutputFile}");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Fatal error: {ex.Message}");
            Console.Error.WriteLine(ex.StackTrace);
            Environment.Exit(1);
        }
    }
}