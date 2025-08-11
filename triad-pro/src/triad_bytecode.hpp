#include "triad_ast.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <variant>
#include <optional>

using namespace triad;
namespace fs = std::filesystem;
using std::string_view;

[[nodiscard]] static std::string slurp(const std::string& path) noexcept {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("Cannot open file: " + path);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void emit_to_file(const std::string& code, const std::string& outPath) {
  std::ofstream out(outPath);
  if (!out) throw std::runtime_error("Cannot write to: " + outPath);
  out << code;
}

static void run_vm(const Chunk& ch, bool trace = false) {
  VM vm;
  if (trace) vm.enable_trace(); // hypothetical hook
  vm.exec(ch);
}

static void run_ast(const std::string& src) {
  std::cout << "[AST interpreter not yet implemented]\n";
}

static void run_tests(bool verbose = false) {
  for (const auto& entry : fs::directory_iterator("tests")) {
    if (entry.path().extension() == ".triad") {
      std::cout << "Running: " << entry.path().filename() << "\n";
      std::string src = slurp(entry.path().string());
      Chunk ch = parse_to_chunk(src);
      if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
      run_vm(ch);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Triad Compiler CLI\n"
              << "Usage: triadc <mode> <file.triad | dir> [options]\n"
              << "Modes:\n"
              << "  run-vm       Execute via VM\n"
              << "  run-ast      Execute via AST interpreter\n"
              << "  emit-nasm    Emit NASM assembly\n"
              << "  emit-llvm    Emit LLVM IR\n"
              << "  run-tests    Execute all .triad files in /tests\n"
              << "Options:\n"
              << "  -o <file>    Output to file\n"
              << "  --verbose    Show debug info\n"
              << "  --show-ast   Print AST\n"
              << "  --show-bytecode Print bytecode\n"
              << "  --trace-vm   Trace VM execution\n"
              << "  --version    Show version\n";
    return 0;
  }

  std::string mode = argv[1];
  std::string target = argc > 2 ? argv[2] : "";
  std::string outFile;
  bool verbose = false, showAst = false, showBytecode = false, traceVm = false;

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
    else if (arg == "--verbose") verbose = true;
    else if (arg == "--show-ast") showAst = true;
    else if (arg == "--show-bytecode") showBytecode = true;
    else if (arg == "--trace-vm") traceVm = true;
    else if (arg == "--version") {
      std::cout << "Triad Compiler v0.9.1\n";
      return 0;
    }
  }

  try {
    if (mode == "run-tests") {
      run_tests(verbose);
      return 0;
    }

    std::string src = slurp(target);
    Chunk ch = parse_to_chunk(src);

    if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
    if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
    if (showAst) {
      std::cout << "[AST dump not yet implemented]\n";
      // ASTNode* ast = parse_to_ast(src); ast->dump();
    }

    if (mode == "run-vm") {
      run_vm(ch, traceVm);
    } else if (mode == "run-ast") {
      run_ast(src);
    } else if (mode == "emit-nasm") {
      std::string asm_code = emit_nasm(ch); // stub
      if (!outFile.empty()) emit_to_file(asm_code, outFile);
      else std::cout << asm_code;
    } else if (mode == "emit-llvm") {
      std::string llvm_code = emit_llvm(ch); // stub
      if (!outFile.empty()) emit_to_file(llvm_code, outFile);
      else std::cout << llvm_code;
    } else {
      std::cerr << "Unknown mode: " << mode << "\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
      std::cerr << "Error: No input file specified.\n";
       return 1;
     }
     std::string src = slurp(target);
     Chunk ch = parse_to_chunk(src);
     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
     if (showAst) {
       std::cout << "[AST dump not yet implemented]\n";
       // ASTNode* ast = parse_to_ast(src); ast->dump();
     }
     if (mode == "run-vm") {
       run_vm(ch, traceVm);
     } else if (mode == "run-ast") {
       run_ast(src);
     } else if (mode == "emit-nasm") {
         std::string asm_code = emit_nasm(ch); // stub
         if (!outFile.empty()) emit_to_file(asm_code, outFile);
       else std::cout << asm_code; // Print to stdout if no output file specified
     } else if (mode == "emit-llvm") {
       std::string llvm_code = emit_llvm(ch); // stub
       if (!outFile.empty()) emit_to_file(llvm_code, outFile);
       else std::cout << llvm_code; // Print to stdout if no output file specified
     } else {
       std::cerr << "Unknown mode: " << mode << "\n";
       return 1;
     }
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << "\n";
     return 1;
   }
   return 0;
   }
   std::cout << "Triad Compiler v0.9.1\n";
   std::string mode = argv[1];
   std::string target = argc > 2 ? argv[2] : "";
   std::string outFile;
   bool verbose = false, showAst = false, showBytecode = false, traceVm
       = false;
   for (int i = 3; i < argc; ++i) {
     std::string arg = argv[i];
     if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
     else if (arg == "--verbose") verbose = true;
	 else if (arg
         == "--show-ast") showAst = true;
     else if (arg == "--show-bytecode") showBytecode = true;
     else if (arg == "--trace-vm") traceVm = true;
     else if (arg == "--version") {
       std::cout << "Triad Compiler v0.9.1\n";
       return 0;
     } else {
       std::cerr << "Unknown argument: " << arg << "\n";
       return 1;
     }
     }
   try {
     if (mode == "run-tests") {
       run_tests(verbose);
       return 0;
     }
     if (target.empty()) {
       std::cerr << "Error: No input file specified.\n";
       return 1;
     }
     std::string src = slurp(target);
     Chunk ch = parse_to_chunk(src);
     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
     if (showAst) {
       std::cout << "[AST dump not yet implemented]\n";
       // ASTNode* ast = parse_to_ast(src); ast->dump();
     }
     if (mode == "run-vm") {
       run_vm(ch, traceVm);
     } else if (mode == "run-ast") {
       run_ast(src);
     } else if (mode == "emit-nasm") {
         std::string asm_code = emit_nasm(ch); // stub
         if (!outFile.empty()) emit_to_file(asm_code, outFile);
       else std::cout << asm_code; // Print to stdout if no output file specified
     } else if (mode == "emit-llvm") {
       std::string llvm_code = emit_llvm(ch); // stub
       if (!outFile.empty()) emit_to_file(llvm_code, outFile);
       else std::cout << llvm_code; // Print to stdout if no output file specified
     } else {
       std::cerr << "Unknown mode: " << mode << "\n";
       return 1;
     }
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << "\n";
     return 1;
   }
   return 0;
   }
   std::cout << "Triad Compiler v0.9.1\n";
   std::string mode = argv[1];
   std::string target = argc > 2 ? argv[2] : "";
   std::string outFile;
   bool verbose = false, showAst = false, showBytecode = false, traceVm
	   = false;
   for (int i = 3; i < argc; ++i) {
     std::string arg = argv[i];
     if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
     else if (arg == "--verbose") verbose = true;
     else if (arg == "--show-ast") showAst = true;
     else if (arg == "--show-bytecode") showBytecode = true;
     else if (arg == "--trace-vm") traceVm = true;
     else if (arg == "--version") {
       std::cout << "Triad Compiler v0.9.1\n";
       return 0;
     } else {
		 std::cerr << "Unknown argument: " << arg << "\n";
		 return 1;
         }
   }
   try {
     if (mode == "run-tests") {
       run_tests(verbose);
       return 0;
     }
     if (target.empty()) {
       std::cerr << "Error: No input file specified.\n";
       return 1;
     }
     std::string src = slurp(target);
     Chunk ch = parse_to_chunk(src);
     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
     if (showAst) {
       std::cout << "[AST dump not yet implemented]\n";
       // ASTNode* ast = parse_to_ast(src); ast->dump();
     }
     if (mode == "run-vm") {
       run_vm(ch, traceVm);
     } else if (mode == "run-ast") {
       run_ast(src);
     } else if (mode == "emit-nasm") {
         std::string asm_code = emit_nasm(ch); // stub
         if (!outFile.empty()) emit_to_file(asm_code, outFile);
       else std::cout << asm_code; // Print to stdout if no output file specified
     } else if (mode == "emit-llvm") {
       std::string llvm_code = emit_llvm(ch); // stub
       if (!outFile.empty()) emit_to_file(llvm_code, outFile);
       else std::cout << llvm_code; // Print to stdout if no output file specified
     } else {
       std::cerr << "Unknown mode: " << mode << "\n";
       return 1;
     }
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << "\n";
     return 1;
   }
   return 0;
   }
   std::cout << "Triad Compiler v0.9.1\n";
   std::string mode = argv[1];
   std::string target = argc > 2 ? argv[2] : "";
   std::string outFile;
   bool verbose = false, showAst = false, showBytecode = false, traceVm
	   = false;
   for (int i = 3; i < argc; ++i) {
     std::string arg = argv[i];
     if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
     else if (arg == "--verbose") verbose = true;
     else if (arg == "--show-ast") showAst = true;
     else if (arg == "--show-bytecode") showBytecode = true;
     else if (arg == "--trace-vm") traceVm = true;
     else if (arg == "--version") {
       std::cout << "Triad Compiler v0.9.1\n";
       return 0;
	 }
	 else {
         std::cerr << "Unknown argument: " << arg << "\n";
         return 1;
         }
   }
   try {
     if (mode == "run-tests") {
       run_tests(verbose);
       return 0;
     }
     if (target.empty()) {
       std::cerr << "Error: No input file specified.\n";
       return 1;
     }
     std::string src = slurp(target);
     Chunk ch = parse_to_chunk(src);
     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
     if (showAst) {
       std::cout << "[AST dump not yet implemented]\n";
       // ASTNode* ast = parse_to_ast(src); ast->dump();
     }
     if (mode == "run-vm") {
       run_vm(ch, traceVm);
     } else if (mode == "run-ast") {
       run_ast(src);
     } else if (mode == "emit-nasm") {
         std::string asm_code = emit_nasm(ch); // stub
         if (!outFile.empty()) emit_to_file(asm_code, outFile);
       else std::cout << asm_code; // Print to stdout if no output file specified
     } else if (mode == "emit-llvm") {
       std::string llvm_code = emit_llvm(ch); // stub
       if (!outFile.empty()) emit_to_file(llvm_code, outFile);
       else std::cout << llvm_code; // Print to stdout if no output file specified
     } else {
       std::cerr << "Unknown mode: " << mode << "\n";
       return 1;
     }
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << "\n";
     return 1;
   }
   return 0;
   }
   std::cout << "Triad Compiler v0.9.1\n";
   std::string mode = argv[1];
   std::string target = argc > 2 ? argv[2] : "";
   std::string outFile;
   bool verbose = false, showAst = false, showBytecode = false, traceVm
	   = false;
   for (int i = 3; i < argc; ++i) {
     std::string arg = argv[i];
     if (arg == "-o" && i + 1 < argc) outFile = argv[++i];
     else if (arg == "--verbose") verbose = true;
     else if (arg == "--show-ast") showAst = true;
     else if (arg == "--show-bytecode") showBytecode = true;
     else if (arg == "--trace-vm") traceVm = true;
     else if (arg == "--version") {
       std::cout << "Triad Compiler v0.9.1\n";
       return 0;
     } else {
		 std::cerr << "Unknown argument: " << arg << "\n";
         return 1;
         }
   }
   try {
     if (mode == "run-tests") {
       run_tests(verbose);
       return 0;
     }
     if (target.empty()) {
       std::cerr << "Error: No input file specified.\n";
       return 1;
     }
     std::string src = slurp(target);
     Chunk ch = parse_to_chunk(src);
     if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
     if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
     if (showAst) {
       std::cout << "[AST dump not yet implemented]\n";
       // ASTNode* ast = parse_to_ast(src); ast->dump();
     }
     if (mode == "run-vm") {
       run_vm(ch, traceVm);
     } else if (mode == "run-ast") {
       run_ast(src);
     } else if (mode == "emit-nasm") {
         std::string asm_code = emit_nasm(ch); // stub
         if (!outFile.empty()) emit_to_file(asm_code, outFile);
       else std::cout << asm_code; // Print to stdout if no output file specified
     } else if (mode == "emit-llvm") {
       std::string llvm_code = emit_llvm(ch); // stub
       if (!outFile.empty()) emit_to_file(llvm_code, outFile);
       else std::cout << llvm_code; // Print to stdout if no output file specified
     } else {
       std::cerr << "Unknown mode: " << mode << "\n";
       return 1;
     }
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << "\n";
     return 1;
   }
   return 0;
   }
