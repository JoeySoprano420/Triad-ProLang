Here are the official release notes for the Triad language and compiler toolchain, version **v0.9.1**:

---

# 🚀 Triad Language v0.9.1 — Release Notes  
**Release Date:** August 2025  
**License:** Sovereign Universal Entity Technical License (S.U.E.T.) v1.0  
**Repository:** [Triad-ProLang on GitHub](https://github.com/JoeySoprano420/Triad-ProLang/blob/main/README.md)

---

## ✨ Highlights

- ✅ **Structured Exception Handling**: Full support for `try`, `catch`, and `finally` with typed filters and payload destructuring.
- ✅ **Object Model Execution**: VM now supports `NEW_CLASS`, `CALL_METHOD`, and `GET_FIELD` with vtables and heap-backed objects.
- ✅ **Macro Expansion**: Hygienic macros with body cloning and compile-time substitution.
- ✅ **Dual Codegen Targets**: Emit NASM and LLVM IR from Triad source.
- ✅ **Robust CLI Tool**: `triadc` supports lexing, parsing, AST/VM execution, codegen, and test automation.

---

## 🧠 Language Features

- Clean syntax with tuples, enums, structs, classes, and macros.
- Control flow: `if`, `else`, `for i in 0..N`, infinite `loop`, short-circuit `and`/`or`.
- Object construction and method calls: `new ClassName(...)`, `obj.method(...)`.
- Error handling: `try { } catch e as Type { } finally { }`.
- Purity annotations: `pure def Class.method(...)` for tooling and optimization.

---

## 🛠 Compiler & VM

- **Lexer**: Full tokenization with keyword and operator support.
- **Parser**: Recursive descent lowering directly to bytecode.
- **Bytecode**: Stack-based IR with support for arithmetic, control flow, method calls, and exception handling.
- **VM**: Executes bytecode with call frames, heap, and capsule-aware tracing.
- **Optimizations**:
  - Short-circuit lowering for `and`/`or`
  - Predicate hoisting
  - Chain fusion for field/index access

---

## 🧪 Tooling & Ecosystem

- **VS Code Extension**:
  - Syntax highlighting
  - Autocomplete for variables, fields, and pure methods
  - Signature help and hover tooltips
  - Workspace-wide indexing and Go to Definition
- **Grammar**: Formal EBNF + `.tmLanguage.json` for editor integration
- **Test Suite**: Automated tests for syntax, bytecode, VM, exceptions, and object model

---

## 📦 Project Structure

```
triad/
├── compiler/        # Lexer, parser, AST, bytecode, VM
├── grammar/         # EBNF + syntax highlighting
├── vscode/          # Editor extension
├── tests/           # Sample programs and unit tests
└── README.md        # Full documentation
```

---

## 🔮 Roadmap to v1.0

- Method body execution for user-defined classes
- Runtime purity enforcement and mutation tracking
- LICM, register allocation, and peephole compression
- Lock-free parallel runtime for loops
- Profile-guided optimization (PGO)

---

