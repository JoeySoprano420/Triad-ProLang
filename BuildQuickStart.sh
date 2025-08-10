# from triad-pro/
cmake -S . -B build
cmake --build build --config Release
./build/triadc lex samples/AgentMain.triad
./build/triadc emit-nasm samples/AgentMain.triad > out.asm
./build/triadc emit-llvm samples/AgentMain.triad > out.ll
