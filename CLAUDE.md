
The goal of this project is to use Gemini cli to create a small (pico) computer algebra system (CAS), called picocas. The CAS should be a faithful recreation of the core architecture (parser, pattern matcher, symbol table, evaluator) of Mathematica (or the Wolfram Language) and a recreation of the core simple mathematical functions of Mathematica (Plus, Times, Power, Divide, etc.)

-- No code changes should be made to any libraries in @src/external/

-- Before any coding takes place the document @SPEC.md should be read to get an understanding of the system. 

-- The code should be well documented, with performance and scalability in mind.

-- Every time a builtin function is implemented, we should add it to the symbol table so its accessible in the repl. 

-- Every time a builtin function is implemented or modified, we should update the picocas_spec.md file. 

-- Every time a builtin function is implemented we should also assign the appropriate Attributes to that function. 

-- Efficient and careful memory management is important. The system should track memory usage and leaks with valgrind. 

A summary of the existing functionality is given in @./picocas_spec.md After any change or improvement to the system is made, a summary of the features should be given in @./picocas_spec.md under an appropriate heading/section etc. 

-- Every builtin function should have an Information string that gives a concise, but complete description of the function (via symtab_set_docstring)
