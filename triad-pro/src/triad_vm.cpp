#include "triad_bytecode.hpp"
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>
#include <string>
#include <stdexcept>

namespace triad {

  using VMValue = std::variant<double, std::string>;
  using Frame = std::unordered_map<std::string, VMValue>;

  class VM {
    std::vector<VMValue> stack_;
    std::vector<Frame> frames_;
    const Chunk* chunk_ = nullptr;
    bool trace_ = false;

  public:
    VM() noexcept = default;

    void enable_trace() noexcept { trace_ = true; }

    void exec(const Chunk& ch) {
      chunk_ = &ch;
      frames_.emplace_back(); // global frame

      size_t ip = 0;
      while (ip < ch.code().size()) {
        const Instr& instr = ch.code()[ip];
        if (trace_) dump_instr(ip, instr);

        switch (instr.op) {
          case Op::PUSH_CONST:
            stack_.push_back(ch.consts()[instr.a]);
            break;

          case Op::SAY:
            print_top();
            break;

          case Op::ADD:
            binary_op(std::plus<>{});
            break;

          case Op::SUB:
            binary_op(std::minus<>{});
            break;

          case Op::MUL:
            binary_op(std::multiplies<>{});
            break;

          case Op::DIV:
            binary_op(std::divides<>{});
            break;

          case Op::MOD:
            binary_op([](double a, double b) { return std::fmod(a, b); });
            break;

          case Op::EQ:
            compare_op(std::equal_to<>{});
            break;

          case Op::NE:
            compare_op(std::not_equal_to<>{});
            break;

          case Op::LT:
            compare_op(std::less<>{});
            break;

          case Op::LE:
            compare_op(std::less_equal<>{});
            break;

          case Op::GT:
            compare_op(std::greater<>{});
            break;

          case Op::GE:
            compare_op(std::greater_equal<>{});
            break;

          case Op::JMP:
            ip = instr.a;
            continue;

          case Op::IF_FALSE_JMP:
            if (!truthy(pop())) {
              ip = instr.a;
              continue;
            }
            break;

          case Op::RET:
            return;

          default:
            throw std::runtime_error("Unhandled opcode");
        }

        ++ip;
      }
    }

  private:
    [[nodiscard]] VMValue pop() {
      if (stack_.empty()) throw std::runtime_error("Stack underflow");
      VMValue val = std::move(stack_.back());
      stack_.pop_back();
      return val;
    }

    void print_top() {
      const VMValue& val = stack_.back();
      std::visit([](auto&& v) { std::cout << v << "\n"; }, val);
    }

    template<typename Op>
    void binary_op(Op&& op) {
      auto b = std::get<double>(pop());
      auto a = std::get<double>(pop());
      stack_.push_back(op(a, b));
    }

    template<typename Op>
    void compare_op(Op&& op) {
      auto b = std::get<double>(pop());
      auto a = std::get<double>(pop());
      stack_.push_back(op(a, b) ? 1.0 : 0.0);
    }

    [[nodiscard]] bool truthy(const VMValue& val) const {
      return std::visit([](auto&& v) -> bool {
        if constexpr (std::is_same_v<decltype(v), double>) return v != 0.0;
        if constexpr (std::is_same_v<decltype(v), std::string>) return !v.empty();
        return false;
      }, val);
    }

    void dump_instr(size_t ip, const Instr& instr) const {
      std::cout << "IP " << ip << ": "
                << static_cast<int>(instr.op) << " "
                << instr.a << " " << instr.b << " " << instr.c << "\n";
    }
  };

  // Print the current stack contents (for debugging)
  void VM::dump_stack() const {
    std::cout << "Stack [";
    for (size_t i = 0; i < stack_.size(); ++i) {
      std::visit([](auto&& v) { std::cout << v; }, stack_[i]);
      if (i + 1 < stack_.size()) std::cout << ", ";
    }
    std::cout << "]\n";
  }

  // Clear the stack and all frames (reset VM state)
  void VM::reset() {
    stack_.clear();
    frames_.clear();
    chunk_ = nullptr;
  }

  // Utility: Convert VMValue to string for logging or debugging
  std::string vmvalue_to_string(const VMValue& val) {
    return std::visit([](auto&& v) -> std::string {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, double>)
        return std::to_string(v);
      else if constexpr (std::is_same_v<T, std::string>)
        return v;
      else
        return "<unknown>";
    }, val);
  }

  // --- Additional: VMValue visitor utility and stack depth accessor ---

  // Utility: Apply a visitor to all stack values (for debugging, inspection, or transformation)
  template <typename Visitor>
  void visit_stack(const VM& vm, Visitor&& visitor) {
    for (const auto& val : vm.stack_) {
      std::visit(visitor, val);
    }
  }

  // Accessor: Get the current stack depth
  std::size_t get_stack_depth(const VM& vm) {
    return vm.stack_.size();
  }

  // --- Additional: Utility to print the full VM state for debugging ---

  void print_vm_state(const VM& vm) {
    std::cout << "=== VM State ===\n";
    // Print stack
    std::cout << "Stack: [";
    for (size_t i = 0; i < vm.get_stack_depth(); ++i) {
      std::cout << vmvalue_to_string(vm.stack_[i]);
      if (i + 1 < vm.get_stack_depth()) std::cout << ", ";
    }
    std::cout << "]\n";
    // Print frames
    std::cout << "Frames: " << vm.frames_.size() << "\n";
    for (size_t f = 0; f < vm.frames_.size(); ++f) {
      std::cout << "  Frame " << f << ": {";
      size_t cnt = 0;
      for (const auto& pair : vm.frames_[f]) {
        std::cout << pair.first << ": " << vmvalue_to_string(pair.second);
        if (++cnt < vm.frames_[f].size()) std::cout << ", ";
      }
      std::cout << "}\n";
    }
    std::cout << "Trace enabled: " << (vm.trace_ ? "yes" : "no") << "\n";
  }

} // namespace triad

#ifdef TRIAD_VM_TEST_MAIN
#include <sstream>

// Simple test harness for VM
int main() {
  using namespace triad;

  // Create a chunk with two constants and an ADD instruction
  Chunk chunk;
  int c0 = chunk.addConst(Value::number(2.5));
  int c1 = chunk.addConst(Value::number(4.0));
  chunk.emit(Op::PUSH_CONST, c0);
  chunk.emit(Op::PUSH_CONST, c1);
  chunk.emit(Op::ADD);
  chunk.emit(Op::SAY);
  chunk.emit(Op::RET);

  VM vm;
  vm.enable_trace();
  vm.exec(chunk);

  // Test VMValue to string
  VMValue v1 = 42.0;
  VMValue v2 = std::string("hello");
  std::cout << "VMValue 1: " << vmvalue_to_string(v1) << "\n";
  std::cout << "VMValue 2: " << vmvalue_to_string(v2) << "\n";

  return 0;
}
#endif

#ifdef TRIAD_VM_STATE_MAIN
#include <string>

int main() {
  using namespace triad;

  // Example: create a VM, run a simple chunk, and print state
  Chunk chunk;
  int c0 = chunk.addConst(Value::number(10));
  int c1 = chunk.addConst(Value::number(20));
  chunk.emit(Op::PUSH_CONST, c0);
  chunk.emit(Op::PUSH_CONST, c1);
  chunk.emit(Op::ADD);
  chunk.emit(Op::SAY);
  chunk.emit(Op::RET);

  VM vm;
  vm.enable_trace();
  vm.exec(chunk);

  print_vm_state(vm);

  return 0;
}
#endif
               << "  -o <file>    Output to file\n"
               << "  --verbose    Show debug info\n"
               << "  --show-ast   Print AST\n"
               << "  --show-bc    Print bytecode\n"
				   << "  --trace-vm   Trace
				   << "  --version    Show version\n";
			   return 0;
			   }
			   std::string mode = argv[1];
			   std::string input = argv[2];
			   std::string output;
			   bool verbose = false;
			   bool show_ast = false;
			   bool show_bytecode = false;
			   bool trace_vm = false;
			   bool version = false;
               for (int i = 3; i < argc; ++i) {
                   std::string arg = argv[i];
                   if (arg == "-o" && i + 1 < argc) output = argv[++i];
                   else if (arg == "--verbose") verbose = true;
                   else if (arg == "--show-ast") show_ast = true;
                   else if (arg == "--show-bytecode") show_bytecode = true;
                   else if (arg == "--trace-vm") trace_vm = true;
                   else if (arg == "--version") version = true;
			   }
               try {
                   if (version) {
                       std::cout << "Triad Compiler v0.9.1\n";
                       return 0;
                   }
                   if (mode == "run-tests") {
                       run_tests(verbose);
                       return 0;
                   }
                   if (input.empty()) {
                       std::cerr << "Error: No input file specified.\n";
                       return 1;
                   }
                   std::string src = slurp(input);
                   Chunk ch = parse_to_chunk(src);
                   if (verbose) std::cout << "[Parsed chunk with " << ch.code().size() << " instructions]\n";
                   if (show_bytecode) ch.dump(); // Assuming Chunk::dump() exists
                   if (show_ast) {
                       std::cout << "[AST dump not yet implemented]\n";
                       // ASTNode* ast = parse_to_ast(src); ast->dump();
                   }
                   if (mode == "run-vm") {
                       run_vm(ch, trace_vm);
                   } else if (mode == "run-ast") {
                       run_ast(src);
                   } else if (mode == "emit-nasm") {
                       std::string asm_code = emit_nasm(ch); // stub
                       if (!output.empty()) emit_to_file(asm_code, output);
                       else std::cout << asm_code; // Print to stdout if no output file specified
                   } else if (mode == "emit-llvm") {
                       std::string llvm_code = emit_llvm(ch); // stub
                       if (!output.empty()) emit_to_file(llvm_code, output);
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
           std::cerr << "Usage: triad_vm <mode> <input> [options]\n"
               << "Modes:\n"
               << "  run-vm       Execute bytecode in VM\n"
               << "  run-ast      Execute source via AST interpreter\n"
               << "  emit-nasm    Emit x86-64 assembly\n"
               << "  emit-llvm    Emit LLVM IR\n"
               << "  run-tests    Run internal tests\n"
			   << "Options:\n"
               << "  -o <file>    Output to file\n"
               << "  --verbose    Show debug info\n"
               << "  --show-ast   Print AST\n"
               << "  --show-bytecode Print bytecode\n"
			   << "  --trace-vm   Trace
               << "  --version    Show version\n";
           return 1;
	   }
#endif
	   << "  --trace-vm   Trace
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
			   std::string llvm_code = emit_llvm
                   (ch); // stub
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
       std::cerr << "Usage: triad_vm <mode> <input> [options]\n"
                 << "Modes:\n"
                 << "  run-vm       Execute bytecode in VM\n"
                 << "  run-ast      Execute source via AST interpreter\n"
                 << "  emit-nasm    Emit x86-64 assembly\n"
                 << "  emit-llvm    Emit LLVM IR\n"
                 << "  run-tests    Run internal tests\n"
                 << "Options:\n"
                 << "  -o <file>    Output to file\n"
                 << "  --verbose    Show debug info\n"
                 << "  --show-ast   Print AST\n"
                 << "  --show-bytecode Print bytecode\n"
		   << "  --trace-vm   Trace
           << "  --version    Show version\n";
	   return 1;
	   }


