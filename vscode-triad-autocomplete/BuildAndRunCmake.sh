cmake -S . -B build && cmake --build build --config Release

# AST interpreter
./build/triadc run-ast samples/AgentMain.triad

# Bytecode VM
./build/triadc run-vm samples/AgentMain.triad

# Emit code
./build/triadc emit-nasm samples/AgentMain.triad > out.asm
./build/triadc emit-llvm samples/AgentMain.triad > out.ll

# Unit tests
cmake --build build --target triad_tests && ./build/triad_tests
# or
./build/triadc test samples/AgentMain.triad
