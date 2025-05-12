# Minimalisp

A minimalistic Lisp interpreter in C featuring lexical scoping, matrix operations, and custom memory management.

## Features

- 🧠 Handwritten parser and evaluator for Lisp-like syntax
- 🧮 Native matrix operations: multiply, inverse, transpose, determinant
- 📦 Custom arena-based memory allocator with scoped "GC" behavior
- 📚 Lexical scoping, closures, and struct-like data types
- 🖥️ REPL and `.minlsp` file support
- 🧪 Valgrind-clean, with benchmarking tools for profiling execution time

## Usage
Minlisp can be compiled using the provided Makefile and run alone or with .minlsp files by
```bash
./minlisp
# Or
./minlisp ./example.minlsp
```

## Language Overview
Minlisp supports Lisp-like syntax with a few extentions and differences.
