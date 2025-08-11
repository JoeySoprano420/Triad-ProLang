// triad-pro\src\triad_bytecode.hpp

// --- Additional: Chunk disassembly and pretty-print utilities ---

namespace triad {

// Convert Op enum to string for readable disassembly
inline const char* op_to_string(Op op) {
  switch (op) {
    case Op::PUSH_CONST: return "PUSH_CONST";
    case Op::SAY: return "SAY";
    case Op::ADD: return "ADD";
    case Op::SUB: return "SUB";
    case Op::MUL: return "MUL";
    case Op::DIV: return "DIV";
    case Op::MOD: return "MOD";
    case Op::EQ: return "EQ";
    case Op::NE: return "NE";
    case Op::LT: return "LT";
    case Op::LE: return "LE";
    case Op::GT: return "GT";
    case Op::GE: return "GE";
    case Op::JMP: return "JMP";
    case Op::IF_FALSE_JMP: return "IF_FALSE_JMP";
    case Op::RET: return "RET";
    // Add more opcodes as needed
    default: return "UNKNOWN_OP";
  }
}

// Print a readable disassembly of a Chunk
inline void dump_chunk(const Chunk& ch, std::ostream& os = std::cout) {
  os << "== Bytecode Dump ==\n";
  for (size_t i = 0; i < ch.code.size(); ++i) {
    const Instr& instr = ch.code[i];
    os << "[" << i << "] " << op_to_string(instr.op)
       << " a:" << instr.a << " b:" << instr.b << " c:" << instr.c;

    // Show constant if relevant
    if (instr.op == Op::PUSH_CONST && instr.a >= 0 && instr.a < static_cast<int>(ch.consts.size())) {
      const Value& v = ch.consts[instr.a];
      if (v.tag == Value::Num)
        os << " ; const(num): " << v.num;
      else if (v.tag == Value::Str)
        os << " ; const(str): \"" << v.str << "\"";
    }
    // Show name if relevant
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names.size())) {
      os << " ; name: " << ch.names[instr.a];
    }
    os << "\n";
  }
  os << "Constants:\n";
  for (size_t i = 0; i < ch.consts.size(); ++i) {
    const Value& v = ch.consts[i];
    if (v.tag == Value::Num)
      os << "  [" << i << "] Num: " << v.num << "\n";
    else if (v.tag == Value::Str)
      os << "  [" << i << "] Str: \"" << v.str << "\"\n";
  }
  os << "Names:\n";
  for (size_t i = 0; i < ch.names.size(); ++i) {
    os << "  [" << i << "] " << ch.names[i] << "\n";
  }
}

// Optionally, add a member function to Chunk for convenience
inline void Chunk::dump(std::ostream& os) const {
  dump_chunk(*this, os);
}

} // namespace triad

#ifdef TRIAD_BYTECODE_DUMP_MAIN
#include <string>

int main(int argc, char** argv) {
  using namespace triad;
  if (argc < 2) {
    std::cout << "Usage: triad_bytecode_dump <file.triad>\n";
    return 1;
  }
  std::string src = slurp(argv[1]);
  Chunk ch = parse_to_chunk(src);
  ch.dump();
  return 0;
}
#endif

// --- Additional: Utility to serialize and deserialize Chunk to/from a simple text format ---

namespace triad {

// Serialize a Chunk to a text stream (for debugging or simple persistence)
inline void serialize_chunk(const Chunk& ch, std::ostream& os) {
  os << "CHUNK\n";
  os << "CODE " << ch.code.size() << "\n";
  for (const auto& instr : ch.code) {
    os << static_cast<int>(instr.op) << " " << instr.a << " " << instr.b << " " << instr.c << "\n";
  }
  os << "CONSTS " << ch.consts.size() << "\n";
  for (const auto& v : ch.consts) {
    if (v.tag == Value::Num)
      os << "N " << v.num << "\n";
    else if (v.tag == Value::Str)
      os << "S " << v.str << "\n";
  }
  os << "NAMES " << ch.names.size() << "\n";
  for (const auto& n : ch.names) {
    os << n << "\n";
  }
}

// Deserialize a Chunk from a text stream (for debugging or simple persistence)
inline Chunk deserialize_chunk(std::istream& is) {
  Chunk ch;
  std::string header;
  is >> header;
  if (header != "CHUNK") throw std::runtime_error("Invalid chunk header");

  std::string section;
  size_t count = 0;

  is >> section >> count;
  if (section != "CODE") throw std::runtime_error("Expected CODE section");
  for (size_t i = 0; i < count; ++i) {
    int op, a, b, c;
    is >> op >> a >> b >> c;
    ch.code.push_back({static_cast<Op>(op), a, b, c});
  }

  is >> section >> count;
  if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
  for (size_t i = 0; i < count; ++i) {
    char type;
    is >> type;
    if (type == 'N') {
      double num;
      is >> num;
      ch.consts.push_back(Value::number(num));
    } else if (type == 'S') {
      std::string str;
      is >> std::ws;
      std::getline(is, str);
      ch.consts.push_back(Value::string(str));
    }
  }

  is >> section >> count;
  if (section != "NAMES") throw std::runtime_error("Expected NAMES section");
  is >> std::ws;
  for (size_t i = 0; i < count; ++i) {
    std::string name;
    std::getline(is, name);
    ch.names.push_back(name);
  }

  return ch;
}

} // namespace triad

#ifdef TRIAD_BYTECODE_SERIALIZE_MAIN
#include <fstream>
#include <string>

int main(int argc, char** argv) {
  using namespace triad;
  if (argc < 3) {
    std::cout << "Usage: triad_bytecode_serialize <input.triad> <output.txt>\n";
    return 1;
  }
  std::string src = slurp(argv[1]);
  Chunk ch = parse_to_chunk(src);

  // Serialize to file
  std::ofstream ofs(argv[2]);
  serialize_chunk(ch, ofs);
  ofs.close();

  // Deserialize and dump
  std::ifstream ifs(argv[2]);
  Chunk loaded = deserialize_chunk(ifs);
  loaded.dump();

  return 0;
}
#endif

#include <iostream>
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
		<< "  --trace-vm   Trace
        << "  --version    Show version\n";
    return 0;
  }
  std::cout << "Triad Compiler v0.9.1\n";
  std::string mode = argv[1];
  std::string target = argc > 2 ? argv[2] : "";
  if (mode == "run-vm") {
    if (target.empty()) {
      std::cerr << "Error: No input file specified for run-vm mode.\n";
      return 1;
    }
    std::string src = slurp(target);
    Chunk ch = parse_to_chunk(src);
    run_vm(ch, false); // Change to true for tracing
  } else if (mode == "run-ast") {
    if (target.empty()) {
      std::cerr << "Error: No input file specified for run-ast mode.\n";
      return 1;
    }
    std::string src = slurp(target);
    run_ast(src);
  }
  else if (mode == "emit-nasm") {
      if (target.empty()) {
      std::cerr << "Error: No input file specified for emit-nasm mode.\n";
      return 1;
    }
    std::string src = slurp(target);
    Chunk ch = parse_to_chunk(src);
    emit_nasm(ch, std::cout); // Change to output file if needed
  } else if (mode == "emit-llvm") {
    if (target.empty()) {
      std::cerr << "Error: No input file specified for emit-llvm mode.\n";
      return 1;
    }
    std::string src = slurp(target);
    Chunk ch = parse_to_chunk(src);
    emit_llvm(ch, std::cout); // Change to output file if needed
  } else if (mode == "run-tests") {
    run_tests();
  } else {
    std::cerr << "Error: Unknown mode '" << mode << "'.\n";
    return 1;
  }
  return 0;
}
  if (!outFile.empty()) {
        emit_to_file(asm_code, outFile);
      } else {
        std::cout << asm_code; // Print to stdout if no output file specified
      }
    } else if (mode == "emit-llvm") {
      std::string llvm_ir = emit_llvm(ch); // stub
      if (!outFile.empty()) emit_to_file(llvm_ir, outFile);
      else std::cout << llvm_ir; // Print to stdout if no output file specified
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
  if (
	  verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
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
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
    return 1;
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
  std::cout << "Triad Compiler v0.9.1\n";
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
		  << "  --trace-vm   Trace
		  << "  --version    Show version\n";
	  return 0;
  }
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
  // Function to dump the contents of a Chunk to an output stream
  inline void dump_chunk(const Chunk& ch, std::ostream& os) {
	  os << "Chunk Dump:\n";
      for (size_t i = 0; i < ch.code.size(); ++i) {
    const Instruction& instr = ch.code[i];
    os << "[" << i << "] " << static_cast<int>(instr.op) << " " << instr.a << " " << instr.b << " " << instr.c;
    // Show constant value if relevant
	if (instr.a >= 0 && instr.a < static_cast<int>(ch.consts.size())) {
        const Value& v = ch.consts[instr.a];
      if (v.tag == Value::Num)
        os << " ; const: Num(" << v.num << ")";
      else if (v.tag == Value::Str)
        os << " ; const: Str(\"" << v.str << "\")";
    }
    // Show name if relevant
	if (instr.op == Op::Name && instr.a >= 0 && instr.a < static_cast<int>(ch.names.size())
        ) {
      os << " ; name: " << ch.names[instr.a];
    }
    os << "\n";
  }
  os << "Constants:\n";
  for (size_t i = 0; i < ch.consts.size(); ++i) {
	  const Value& v = ch.consts[i];
      os << "  [" << i << "] ";
    if (v.tag == Value::Num) {
      os << "Num(" << v.num << ")";
    } else if (v.tag == Value::Str) {
      os << "Str(\"" << v.str << "\")";
    } else {
      os << "Unknown";
    }
	os << "\n";
    }
  os << "Names:\n";
  for (size_t i = 0; i < ch.names.size(); ++i) {
    os << "  [" << i << "] " << ch.names[i] << "\n";
  }
  }
#ifdef TRIAD_BYTECODE_DUMP_MAIN
#include <iostream>
#include <fstream>
  int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: triad_bytecode_dump <file.triad>\n";
    return 1;
  }
  try {
    std::string src = slurp(argv[1]);
    Chunk ch = parse_to_chunk(src);
    ch.dump();
  } catch (const std::exception& e) {
	  std::cerr << "Error: " << e.what() << "\n";
	  return 1;
  }
  return 0;
  }
#endif
  namespace triad {
	  // Serialize a Chunk to a text stream (for debugging or simple persistence)
	  inline void serialize_chunk(const Chunk& ch, std::ostream& os) {
		  os << "CHUNK\n";
		  os << "CODE " << ch.code.size() << "\n";
		  for (const auto& instr : ch.code) {
              os << static_cast<int>(instr.op) << " " << instr.a << " " << instr.b << " " << instr.c << "\n";
		  }
		  os << "CONSTS " << ch.consts.size() << "\n";
          for (const auto& v : ch.consts) {
              if (v.tag == Value::Num) {
                  os << "N " << v.num << "\n";
              } else if (v.tag == Value::Str) {
                  os << "S " << v.str << "\n";
              } else {
                  os << "U\n"; // Unknown type
              }
		  }
		  os << "NAMES " << ch.names.size() << "\n";
          for (const auto& name : ch.names) {
              os << name << "\n";
		  }
	  }
	  // Deserialize a Chunk from a text stream
	  inline Chunk deserialize_chunk(std::istream& is) {
          Chunk ch;
          std::string section;
          size_t count;
          is >> section;
          if (section != "CHUNK") throw std::runtime_error("Expected CHUNK header");
          is >> section >> count;
          if (section != "CODE") throw std::runtime_error("Expected CODE section");
          ch.code.reserve(count);
          for (size_t i = 0; i < count; ++i) {
              int op, a, b, c;
              is >> op >> a >> b >> c;
              ch.code.push_back({static_cast<Op>(op), a, b, c});
          }
          is >> section >> count;
          if (section != "CONSTS") throw std::runtime_error("Expected CONSTS section");
          ch.consts.reserve(count);
          for (size_t i = 0; i < count; ++i) {
              char type;
              is >> type;
              if (type == 'N') {
                  double num;
                  is >> num;
                  ch.consts.push_back(Value::number(num));
              } else if (type == 'S') {
                  std::string str;
                  is >> std::ws;
                  std::getline(is, str);
                  ch.consts.push_back(Value::string(str));
              } else {
                  throw std::runtime_error("Unknown constant type: " + std::string(1, type));
              }
          }
          is >> section >> count;
          if (section != "NAMES") throw std::runtime_error("Expected NAMES section");
          ch.names.reserve(count);
          is >> std::ws; // Skip whitespace before names
          for (size_t i = 0; i < count; ++i) {
              std::string name;
              std::getline(is, name);
              ch.names.push_back(name);
          }
          return ch;
	  }
#ifdef TRIAD_SERIALIZE_MAIN
#include <iostream>
#include <fstream>
      int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: triad_serialize <file.triad>\n";
    return 1;
  }
  
  // Serialize
  std::ifstream ifs(argv[1]);
  if (!ifs) {
    std::cerr << "Error opening file: " << argv[1] << "\n";
    return 1;
  }
  
  Chunk ch = parse_to_chunk(ifs);
  std::ofstream ofs("output.chunk");
  serialize_chunk(ch, ofs);
  
  // Deserialize
  ofs.close();
  std::ifstream ifs2("output.chunk");
  if (!ifs2) {
	  std::cerr << "Error opening output file for reading: output.chunk\n";
	  return 1;
  }
  Chunk ch2 = deserialize_chunk(ifs2);
  ifs2.close();
  std::cout << "Deserialized chunk with " << ch2.code.size() << " instructions\n";
  for (const auto& instr : ch2.code) {
	  std::cout << static_cast<int>(instr.op) << " " << instr.a <<
		  " " << instr.b << " " << instr.c << "\n";
  }
  return 0;
	  }
#endif
	  } // namespace triad
#include <iostream>
#include <fstream>
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
		<< "  --trace-vm   Trace
        << "  --version    Show version\n";
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
    if (mode == "emit-llvm") {
      std::string llvm_ir = emit_llvm(target); // Assuming emit_llvm() takes a file path
	  if (!outFile.empty()) emit_to_file(llvm_ir, outFile);
      else std::cout << llvm_ir; // Print to stdout if no output file specified
      return 0;
    }
    if (mode == "emit-nasm") {
		std::string nasm_code = emit_nasm(target); // Assuming emit_nasm() takes a file path
        if (!outFile.empty()) emit_to_file(nasm_code, outFile);
      else std::cout << nasm_code; // Print to stdout if no output file specified
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
	}
	else if (mode == "emit-nasm") {
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
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
    return 1;
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
    return 1;
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
	}
	else if (mode == "emit-nasm") {
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
	}
	else if (mode == "run-ast") {
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
        std::cout << "Triad Compiler v0.9.1\n"
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
			<< "  --trace-vm   Trace
            << "  --version    Show version\n";
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
    else if (arg == "--trace-vm") trace
        Vm = true;
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
	}
	else if (mode == "emit-llvm") {
        std::string llvm_code = emit_llvm(ch); // stub
        if (!outFile.empty()) emit_to_file(llvm_code, outFile);
      else std::cout << llvm_code; // Print to stdout if no output file specified
    } else {
      std::cerr << "Unknown mode: " << mode << "\n";
      return 1;
    }
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
    else if (arg == "--trace-vm") trace
		Vm
        = true;
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
  // Example serialization/deserialization
  Chunk ch;
  ch.code.push_back({ OpCode::LOAD, 1, 0, 0
	  });
  ch.code.push_back({ OpCode::ADD, 1, 2, 3 });
  ch.code.push_back({ OpCode::STORE, 1, 0, 0
	  });
  std::cout << "Original chunk with " << ch.code.size() << " instructions\n";
  for (const auto& instr : ch.code) {
	  std::cout << static_cast<int>(instr.op) << " " << instr.a <<
		  " " << instr.b << " " << instr.c << "\n";
  }
  std::string serialized = ch.serialize();
  std::cout << "Serialized chunk: " << serialized << "\n";
  Chunk deserialized = Chunk::deserialize(serialized);
  std::cout << "Deserialized chunk with " << deserialized.code.size() <<
	  " instructions\n";
  for (const auto& instr : deserialized.code) {
	  std::cout << static_cast<int>(instr.op) << " " << instr.a <<
		  " " << instr.b << " " << instr.c << "\n";
  }
  std::cout << "Triad Compiler v0.9.1\n"
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
	  << "  --trace-vm   Trace
	  << "  --version    Show version\n";
  return 0;
  }
#include <iostream>
#include <string>
#include "triad.h" // Assuming this header contains necessary declarations
#include "chunk.h" // Assuming this header contains Chunk class definition
#include "vm.h" // Assuming this header contains VM-related functions
#include "ast.h" // Assuming this header contains AST-related functions
#include "nasm.h" // Assuming this header contains NASM-related functions
#include "llvm.h" // Assuming this header contains LLVM-related functions
#include "tests.h" // Assuming this header contains test functions
#include "utils.h" // Assuming this header contains utility functions like slurp()
#include "serialize.h" // Assuming this header contains serialization functions
  int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Triad Compiler v0.9.1\n"
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
	}
	else {
        std::cerr << "Unknown mode: " << mode << "\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
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
    else if (arg == "--trace-vm") trace
        Vm = true;
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
	}
	else {
        std::cerr << "Unknown mode: " << mode << "\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
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
    else if (arg == "--trace-vm") trace
        Vm = true;
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
	}
	else if (mode == "run-ast") {
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
  std::cout << "Triad Compiler v0.9.1\n"
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
	  << "  --trace-vm   Trace
	  << "  --version    Show version\n";
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
    else if (arg == "--trace-vm") trace
        Vm = true;
    else if (arg == "--version") {
      std::cout << "Triad Compiler v0.9.1\n";
	  return 0;
      } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return 1;
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
    else if (arg == "--trace-vm") trace
        Vm = true;
    else if (arg == "--version") {
      std::cout << "Triad Compiler v0.9.1\n";
	  return 0;
      } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return 1;
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
  std::cout << "Triad Compiler v0.9.1\n"
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
	  << "  --trace-vm   Trace
	  << "  --version    Show version\n";
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
	else if (arg == "--trace-vm") trace
        Vm = true;
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
  std::cout << "Triad Compiler v0.9.1\n"
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
