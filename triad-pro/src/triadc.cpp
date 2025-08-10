#include "triad_parser.cpp"
#include "triad_vm.cpp"
#include "triad_ast.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

using namespace triad;
namespace fs = std::filesystem;

static std::string slurp(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("Cannot open file: " + path);
  std::ostringstream o;
  o << f.rdbuf();
  return o.str();
}

static void run_vm(const Chunk& ch) {
  VM vm;
  vm.exec(const_cast<Chunk&>(ch)); // VM expects non-const
}

static void run_ast(const std::string& src) {
  // Stub: interpret AST directly
  std::cout << "[AST interpreter not yet implemented]\n";
}

static void emit_to_file(const std::string& code, const std::string& outPath) {
  std::ofstream out(outPath);
  if (!out) throw std::runtime_error("Cannot write to: " + outPath);
  out << code;
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
      for (const auto& entry : fs::directory_iterator("tests")) {
        if (entry.path().extension() == ".triad") {
          std::cout << "Running: " << entry.path().filename() << "\n";
          std::string src = slurp(entry.path().string());
          Chunk ch = parse_to_chunk(src);
          run_vm(ch);
        }
      }
      return 0;
    }

    std::string src = slurp(target);
    Chunk ch = parse_to_chunk(src);

    if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
    if (showBytecode) ch.dump(); // Assuming Chunk::dump() exists
    if (showAst) {
      ASTNode* ast = parse_to_ast(src); // Stub
      ast->dump(); // Assuming ASTNode::dump() exists
    }

    if (mode == "run-vm") {
      if (traceVm) VM::enable_trace(); // Stub
      run_vm(ch);
    } else if (mode == "run-ast") {
      run_ast(src);
    } else if (mode == "emit-nasm") {
      std::string asm_code = emit_nasm(ch);
      if (!outFile.empty()) emit_to_file(asm_code, outFile);
      else std::cout << asm_code;
    } else if (mode == "emit-llvm") {
      std::string llvm_code = emit_llvm(ch);
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
