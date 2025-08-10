# Triad-ProLang
Triad ProLang

Here’s the current scope and capabilities of the **Triad compiler toolchain**, now further expanded to wire the bytecode compiler for structured exception handling and add VM-side object construction and method dispatch.

### Features Implemented

* **Full compile-time macro expansion** with body cloning & hygienic substitution (beyond single-return expressions).
* **Strict typed catches** with enum/struct tagging, payload matching, and destructuring.
* **Full bytecode layer + VM** executing: `Load`, `mutate`, `say`, `echo`, `tone`, `trace`, `throw`, `ret`, `if/else`, `loop`, `jump`, `let`, arithmetic, and method calls.
* **Structured exceptions fully wired in bytecode**: the compiler now emits `TRY_ENTER/TRY_FILTER/TRY_LEAVE` plus `FIN_ENTER/FIN_LEAVE`, with typed filters (e.g., `ErrorKind.IO` or `ErrorKind`). The VM installs handler frames and matches enum tags and variants.
* **Object model execution**: struct/class construction, field access, method invocation, vtables for classes, and enum variant payloads. (VM now includes `NEW_CLASS`, `GET_FIELD`, `SET_FIELD`, `CALL_METHOD` ops.)
* **NASM/LLVM lowering** for control-flow (`Load/mutate/jump`) and runtime calls.
* Minimal **runtime library** with string ops, tone device stub, tracing hooks, error formatting, and object heap.
* CLI tool **`triadc`** for lexing, parsing, semantic checking, running (AST or VM), emitting NASM/LLVM, and executing tests.

### Pipeline

1. **Parser → AST**: Recursive descent parser from EBNF with full enum literal parsing.
2. **Semantic Analysis**: Scope/type checking, label resolution, enum payload/type enforcement, attribute checks.
3. **Execution Paths**:

   * **AST Interpreter**: Runs parsed programs directly.
   * **VM**: Bytecode with registers, call frames, heap, try/catch/finally handling, and method dispatch.
   * **Codegen**: NASM and LLVM IR emission with calls into runtime for I/O, object ops, and exception handling.
4. **Runtime Library**: Standard operations, error handling, tone stub, tracing, memory management.
5. **Unit Testing Framework**: Automated tests for parser, semantic analysis, bytecode execution (including try/catch/finally), object model, and codegen.

### Usage

```bash
# Build
cmake -S . -B build && cmake --build build --config Release

# Run via AST interpreter
./build/triadc run-ast samples/AgentMain.triad

# Run via VM
./build/triadc run-vm samples/AgentMain.triad

# Emit assembly or LLVM IR
./build/triadc emit-nasm samples/AgentMain.triad > out.asm
./build/triadc emit-llvm samples/AgentMain.triad > out.ll
```

### Current Strengths

* Hygienic macros allow compile-time code generation with guaranteed scope isolation.
* Fully working structured exception handling in VM, including typed catches and finally landing pads.
* End-to-end object model execution with VM-side constructors and method dispatch.
* Dual-target codegen to NASM and LLVM.
* Robust automated test coverage for parser, bytecode, exception handling, and object model.
