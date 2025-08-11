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

       // triad-pro\src\triad_vm.cpp

// --- Advanced Features: Method Execution, Purity, Optimization, Parallelism, PGO ---

       namespace triad {

           // --- Method body execution for user-defined classes (stub) ---
           struct UserClass {
               std::string name;
               std::map<std::string, Chunk> methods;
           };

           struct Object {
               UserClass* cls = nullptr;
               std::map<std::string, Value> fields;
           };

           Value execute_method(Object& obj, const std::string& method, const std::vector<Value>& args, bool trace = false) {
               if (!obj.cls) throw std::runtime_error("Null class in method call");
               auto it = obj.cls->methods.find(method);
               if (it == obj.cls->methods.end()) throw std::runtime_error("Method not found: " + method);
               VM vm;
               if (trace) vm.enable_trace();
               // TODO: Push 'this' and args to VM stack, set up frame, etc.
               vm.exec(it->second);
               // TODO: Return value from VM stack
               return Value::number(0); // placeholder
           }

           // --- Runtime purity enforcement and mutation tracking (stub) ---
           struct PurityContext {
               bool pure = true;
               std::set<void*> mutated_objects;
               void mark_mutation(void* obj) {
                   pure = false;
                   mutated_objects.insert(obj);
               }
               bool is_pure() const { return pure; }
           };

           PurityContext g_purity_ctx;

           void track_mutation(void* obj) {
               g_purity_ctx.mark_mutation(obj);
           }

           // --- LICM, register allocation, and peephole compression (stubs) ---
           void licm_pass(Chunk& ch) {
               // Loop-Invariant Code Motion: Move invariant code out of loops (stub)
           }

           void register_allocate(Chunk& ch) {
               // Register allocation: Assign stack slots/virtual registers (stub)
           }

           void peephole_compress(Chunk& ch) {
               // Peephole compression: Remove redundant instructions (stub)
           }

           // --- Lock-free parallel runtime for loops (stub) ---
#include <thread>
#include <atomic>
#include <future>

           template <typename Fn>
           void parallel_for(size_t begin, size_t end, Fn&& fn) {
               std::atomic<size_t> idx{ begin };
               size_t nthreads = std::thread::hardware_concurrency();
               std::vector<std::thread> threads;
               for (size_t t = 0; t < nthreads; ++t) {
                   threads.emplace_back([&] {
                       size_t i;
                       while ((i = idx.fetch_add(1)) < end) {
                           fn(i);
                       }
                       });
               }
               for (auto& th : threads) th.join();
           }

           // --- Profile-guided optimization (PGO) (stub) ---
           struct ProfileData {
               std::map<size_t, size_t> exec_counts; // instruction index -> count
               void record(size_t ip) { ++exec_counts[ip]; }
           };

           ProfileData g_profile;

           void instrument_profile(Chunk& ch) {
               // Insert counters or hooks into bytecode (stub)
           }

           void apply_pgo(Chunk& ch, const ProfileData& profile) {
               // Use profile data to reorder, inline, or specialize code (stub)
           }

       } // namespace triad

#ifdef TRIAD_VM_ADVANCED_FEATURES_MAIN
#include <iostream>

       int main() {
           using namespace triad;

           // Example: parallel_for
           std::atomic<int> sum{ 0 };
           parallel_for(0, 100, [&](size_t i) { sum += int(i); });
           std::cout << "Parallel sum 0..99 = " << sum.load() << "\n";

           // Example: method execution (stub)
           UserClass cls;
           cls.name = "Point";
           Object obj;
           obj.cls = &cls;
           try {
               execute_method(obj, "move", {});
           }
           catch (const std::exception& e) {
               std::cout << "Method exec: " << e.what() << "\n";
           }

           // Example: purity tracking
           int dummy;
           track_mutation(&dummy);
           std::cout << "Is pure? " << (g_purity_ctx.is_pure() ? "yes" : "no") << "\n";

           // Example: optimization passes (stubs)
           Chunk ch;
           licm_pass(ch);
           register_allocate(ch);
           peephole_compress(ch);

           // Example: PGO (stubs)
           instrument_profile(ch);
           apply_pgo(ch, g_profile);

           return 0;
       }
#endif

	   // --- Bytecode Dumping and Serialization/Deserialization ---

#include "triad_bytecode.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
       namespace triad {
  // Serialize a Chunk to a text stream (for debugging or simple persistence)
  inline void serialize_chunk(const Chunk& ch, std::ostream& os) {
    os << "Chunk:\n";
    for (const auto& instr : ch.code()) {
      os << "  " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (const auto& v : ch.consts()) {
      if (v.tag == Value::Num)
        os << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  Str: \"" << v.str << "\"\n";
    }
    os << "Names:\n";
    for (const auto& name : ch.names()) {
      os << "  " << name << "\n";
    }
  }
  // Deserialize a Chunk from a text stream (stub, not implemented)
  inline Chunk deserialize_chunk(std::istream& is) {
	  Chunk ch;
      std::string line;
    while (std::getline(is, line)) {
      if (line == "Chunk:") continue;
      if (line == "Constants:") continue;
      if (line == "Names:") continue;
      // Parse instructions, constants, and names from the line
      // This is a stub; actual parsing logic would go here
    }
	return ch;
    }
  // Dump a Chunk to an output stream (for debugging)
  inline void dump_chunk(const Chunk& ch, std::ostream& os) {
    os << "Chunk:\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "  [" << i << "] " << disasm_instr(instr, ch);
      // Show constant value if relevant
      if (instr.op == Op::PUSH_CONST && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
        const Value& v = ch.consts()[instr.a];
        if (v.tag == Value::Num)
          os << " ; const(num): " << v.num;
        else if (v.tag == Value::Str)
          os << " ; const(str): \"" << v.str << "\"";
      }
      // Show name if relevant
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
        os << " ; name: " << ch.names()[instr.a];
      }
      os << "\n";
    }
    os << "Constants:\n";
	for (size_t i = 0; i < ch.consts().size(); ++i) {
        const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
    }
    os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Disassemble an instruction into a human-readable string
  inline std::string disasm_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op) << " a:" << instr.a << " b:" << instr.b << " c:" << instr.c;
    if (instr.op == Op::PUSH_CONST && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
      if (v.tag == Value::Num)
        oss << " ; const(num): " << v.num;
      else if (v.tag == Value::Str)
        oss << " ; const(str): \"" << v.str << "\"";
    }
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
    return oss.str();
  }
  // Print a readable disassembly of a Chunk
  inline void disasm_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Disassembly ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "[" << i << "] " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
    }
    os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Serialize a Chunk to a file
  inline void serialize_chunk_to_file(const Chunk& ch, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) throw std::runtime_error("Failed to open file for writing: " + filename);
    serialize_chunk(ch, ofs);
  }
  // Deserialize a Chunk from a file (stub, not implemented)
  inline Chunk deserialize_chunk_from_file(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) throw std::runtime_error("Failed to open file for reading: " + filename);
    return deserialize_chunk(ifs); // This function needs to be implemented
  }
  } // namespace triad
  // --- Main function to run the VM or emit bytecode ---
#ifdef TRIAD_VM_MAIN
#include <iostream>
#include <string>
  int main(int argc, char** argv) {
  using namespace triad;
  if (argc < 2) {
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
  std::string mode = argv[1];
  std::string input = argc > 2 ? argv[2] : "";
  std::string outputFile;
  bool verbose = false, showAst = false, showBytecode = false, traceVm = false;
  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-o" && i + 1 < argc) outputFile = argv[++i];
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
    if (input.empty()) {
      std::cerr << "Error: No input file specified.\n";
      return 1;
    }
    std::string src = slurp(input);
    Chunk ch = parse_to_chunk(src);
    if (verbose) std::cout << "[Parsed chunk with " << ch.code().size() << " instructions]\n";
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
      if (!outputFile.empty()) emit_to_file(asm_code, outputFile);
      else std::cout << asm_code; // Print to stdout if no output file specified
    } else if (mode == "emit-llvm") {
      std::string llvm_code = emit_llvm(ch); // stub
      if (!outputFile.empty()) emit_to_file(llvm_code, outputFile);
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
            << "Usage: triad_vm <mode> <file.triad | dir> [options]\n"
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
  // --- Main function to run the VM or emit bytecode ---
#ifdef TRIAD_VM_MAIN
#include <iostream>
  #include <string>
  int main(int argc, char** argv) {
    using namespace triad;
    if (argc < 2) {
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
    if (argc < 3) {
      std::cerr << "Error: No input file specified.\n"
               << "Usage: triad_vm <mode> <input> [options]\n"
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
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
      return 0;
	}
    if (argc < 3) {
      std::cerr << "Error: No input file specified.\n"
               << "Usage: triad_vm <mode> <input> [options]\n"
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
		if
            (!output.empty()) emit_to_file(llvm_code, output);
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
              << "Usage: triad_vm <mode> <file.triad | dir> [options]\n"
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
  // Print the current state of the VM for debugging
  inline void print_vm_state(const VM& vm) {
    std::cout << "VM State:\n";
    std::cout << "Stack: ";
    for (const auto& val : vm.stack_) {
      std::cout << vmvalue_to_string(val) << " ";
    }
    std::cout << "\n";
    std::cout << "Frame stack:\n";
    for (const auto& frame : vm.frames_) {
      std::cout << "  Frame: ip=" << frame.ip_ << ", sp=" << frame.sp_ << ", base=" << frame.base_ << "\n";
      std::cout << "  Locals: {";
      for (size_t i = 0; i < frame.locals_.size(); ++i) {
        if (i > 0) std::cout << ", ";
		std::cout << vmvalue_to_string(frame.locals_[i]);
        }
      std::cout << "}\n";
    }
    std::cout << "Constants:\n";
    for (const auto& v : vm.chunk_.consts()) {
      if (v.tag == Value::Num)
        std::cout << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        std::cout << "  Str: \"" << v.str << "\"\n";
    }
    std::cout << "Names:\n";
    for (const auto& name : vm.chunk_.names()) {
      std::cout << "  " << name << "\n";
    }
  }
  int main(int argc, char** argv) {
	  using namespace triad;
      if (argc < 2) {
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
      if (argc < 3) {
          std::cerr << "Error: No input file specified.\n"
               << "Usage: triad_vm <mode> <input> [options]\n"
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
      if (argc == 2 && std::string(argv[1]) == "--help") {
          std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
		  return 0;
          }
      if (argc < 3) {
          std::cerr << "Error: No input file specified.\n"
               << "Usage: triad_vm <mode> <input> [options]\n"
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
      std::cout << "Triad Compiler v0.9.1\n"
                << "Usage: triad_vm <mode> <file.triad | dir> [options]\n"
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
       // --- Advanced VM features ---
       namespace triad {
           // --- Method execution and object-oriented features ---
           struct Chunk {
               std::vector<Instr> code_;
               std::vector<Value> consts_;
               std::vector<std::string> names_;
               const std::vector<Instr>& code() const { return code_; }
               const std::vector<Value>& consts() const { return consts_; }
               const std::vector<std::string>& names() const { return names_; }
               void dump() const {
                   for (const auto& instr : code_) {
                       std::cout << disasm_instr(instr, *this) << "\n";
                   }
               }
		   };
           struct UserClass {
               std::string name;
               std::map<std::string, Chunk> methods; // method name -> bytecode chunk
           };
           struct Object {
               UserClass* cls;
               std::map<std::string, Value> fields; // field name -> value
           };
           Value execute_method(Object& obj, const std::string& method_name, const std::vector<Value>& args, bool trace = false) {
               auto it = obj.cls->methods.find(method_name);
               if (it == obj.cls->methods.end()) {
                   throw std::runtime_error("Method not found: " + method_name);
               }
               Chunk& ch = it->second;
			   VM vm(ch); // Assuming VM is a class that can execute bytecode
               vm.push_object(obj); // Push the object as the first argument
               for (const auto& arg : args) {
                   vm.push_value(arg); // Push additional arguments
               }
               if (trace) {
                   std::cout << "Executing method: " << method_name << " on class: " << obj.cls->name << "\n";
               }
               vm.run(); // Run the bytecode
               return vm.pop_value(); // Return the result
           }
           // --- Purity tracking and mutation detection ---
           struct PurityContext {
               bool pure = true;
               void track_mutation(void* ptr) {
                   pure = false; // If any mutation is detected, mark as impure
               }
               bool is_pure() const { return pure; }
           };
           PurityContext g_purity_ctx;
           void track_mutation(void* ptr) {
			   g_purity_ctx.track_mutation(ptr);
               }
           // --- Optimization passes ---
           void licm_pass(Chunk& ch) {
               // Perform loop-invariant code motion (stub)
           }
           void register_allocate(Chunk& ch) {
               // Perform register allocation (stub)
           }
           void peephole_compress(Chunk& ch) {
               // Perform peephole optimization (stub)
           }
           // --- Parallel execution ---
           template<typename Fn>
           void parallel_for(size_t start, size_t end, Fn fn) {
               std::vector<std::thread> threads;
               std::atomic<size_t> idx(start);
               size_t num_threads = std::thread::hardware_concurrency();
			   for (size_t t = 0; t < num_threads; ++t) {
                   threads.emplace_back([&]() {
                       while (true) {
                           size_t i = idx.fetch_add(1);
                           if (i >= end) break; // Exit when done
                           fn(i); // Execute the function for this index
                       }
                   });
               }
               for (auto& th : threads) {
                   th.join(); // Wait for all threads to finish
               }
           }
           // --- Profile-guided optimization (PGO) ---
           std::vector<ProfileData> g_profile; // Global profile data
           void instrument_profile(Chunk& ch) {
               // Instrument the chunk for profiling (stub)
           }
		   void apply_pgo(Chunk& ch, const std::vector<ProfileData>& profile) {
               // Apply profile-guided optimizations (stub)
           }
       } // namespace triad
	   // --- Example usage of advanced features ---
       #ifdef TRIAD_VM_MAIN
       #include <iostream>
       #include <string>
       int main(int argc, char** argv) {
           using namespace triad;
           // Example: creating a user class and executing a method
		   UserClass my_class;
           my_class.name = "MyClass";
           my_class.methods["my_method"] = parse_to_chunk("PUSH_CONST 0\nADD\nRETURN"); // Example bytecode
           Object obj;
		   obj.cls = &my_class;
           obj.fields["field1"] = Value(42); // Example field
           // Execute a method
           std::vector<Value> args; // No arguments for this example
           Value result = execute_method(obj, "my_method", args, true);
           std::cout << "Method result: " << result.num << "\n";
           // Track mutation
           track_mutation(&obj.fields["field1"]);
           // Check purity
           if (!g_purity_ctx.is_pure()) {
               std::cout << "Detected mutation, context is impure.\n";
           } else {
               std::cout << "Context is pure.\n";
           }
           // Run optimizations
           Chunk ch = parse_to_chunk("PUSH_CONST 1\nPUSH_CONST 2\nADD\nRETURN");
           licm_pass(ch);
           register_allocate(ch);
		   peephole_compress(ch);
           // Parallel execution example
           parallel_for(0, 10, [](size_t i) {
               std::cout << "Parallel task " << i << "\n";
           });
           // Profile-guided optimization (stub)
           instrument_profile(ch);
           apply_pgo(ch, g_profile);
           return 0;
       }
#endif // TRIAD_VM_MAIN
       // --- Serialization and deserialization of Chunks ---
  inline void serialize_chunk(const Chunk& ch, std::ostream& os) {
    os << "Chunk:\n";
    for (const auto& instr : ch.code()) {
      os << "  " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (const auto& v : ch.consts()) {
      if (v.tag == Value::Num)
        os << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  Str: \"" << v.str << "\"\n";
    }
    os << "Names:\n";
    for (const auto& name : ch.names()) {
      os << "  " << name << "\n";
    }
  }
  inline Chunk deserialize_chunk(std::istream& is) {
    Chunk ch;
    std::string line;
    while (std::getline(is, line)) {
      if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
      if (line == "Chunk:") continue; // Skip header
      if (line == "Constants:") continue; // Skip constants header
      if (line == "Names:") continue; // Skip names header
      // Parse instruction
      Instr instr;
      std::istringstream iss(line);
      std::string op_str;
      iss >> op_str >> instr.a >> instr.b >> instr.c;
      instr.op = string_to_op(op_str);
      ch.code().push_back(instr);
      // Handle constants and names
      if (instr.op == Op::PUSH_CONST && instr.a >= 0) {
        Value v;
        v.tag = Value::Num; // Assume num for simplicity, extend as needed
        v.num = static_cast<double>(instr.a); // Example conversion, adjust as needed
        ch.consts().push_back(v);
      }
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0) {
        ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
    }
    return ch;
  }
  // Disassemble a Chunk to a human-readable format
  inline void disasm_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Disassembly ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "[" << i << "] " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
    }
    os << "Names:\n";
	for (size_t i = 0; i < ch.names().size(); ++i) {
        os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Disassemble a single instruction to a human-readable format
  inline std::string disasm_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op);
    if (instr.a != -1) oss << " " << instr.a;
    if (instr.b != -1) oss << ", " << instr.b;
    if (instr.c != -1) oss << ", " << instr.c;
    // Add names for certain instructions
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
	}
	else if (instr.op == Op::PUSH_CONST
        && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
      if (v.tag == Value::Num) {
        oss << " ; const: " << v.num;
      } else if (v.tag == Value::Str) {
        oss << " ; const: \"" << v.str << "\"";
      }
    }
    return oss.str();
  }
  // Serialize a Chunk to a human-readable format
  inline void serialize_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "Chunk:\n";
    for (const auto& instr : ch.code()) {
      os << "  " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
		  os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
	}
    os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Deserialize a Chunk from a human-readable format
  inline Chunk deserialize_chunk(std::istream& is) {
    Chunk ch;
    std::string line;
    while (std::getline(is, line)) {
      if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
      if (line == "Chunk:") continue; // Skip header
      if (line == "Constants:") continue; // Skip constants header
      if (line == "Names:") continue; // Skip names header
      // Parse instruction
      Instr instr;
      std::istringstream iss(line);
      std::string op_str;
      iss >> op_str >> instr.a >> instr.b >> instr.c;
      instr.op = string_to_op(op_str);
      ch.code().push_back(instr);
      // Handle constants and names
      if (instr.op == Op::PUSH_CONST && instr.a >= 0) {
        Value v;
        v.tag = Value::Num; // Assume num for simplicity, extend as needed
        v.num = static_cast<double>(instr.a); // Example conversion, adjust as needed
        ch.consts().push_back(v);
      }
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0) {
        ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
    }
    return ch;
  }
  // Disassemble a Chunk to a human-readable format
  inline void disasm_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Disassembly ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "[" << i << "] " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
    }
	os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Disassemble a single instruction to a human-readable format
  inline std::string disasm_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op);
    if (instr.a != -1) oss << " " << instr.a;
    if (instr.b != -1) oss << ", " << instr.b;
    if (instr.c != -1) oss << ", " << instr.c;
    // Add names for certain instructions
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
    else if (instr.op == Op::PUSH_CONST
        && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
      if (v.tag == Value::Num) {
        oss << " ; const: " << v.num;
      } else if (v.tag == Value::Str) {
        oss << " ; const: \"" << v.str << "\"";
      }
    }
    return oss.str();
  }
  // Convert an Op enum to a string representation
  inline std::string op_to_string(Op op) {
    switch (op) {
      case Op::PUSH_CONST: return "PUSH_CONST";
      case Op::PUSH_VAR: return "PUSH_VAR";
      case Op::SET_VAR: return "SET_VAR";
      case Op::GET_FIELD: return "GET_FIELD";
      case Op::CALL_METHOD: return "CALL_METHOD";
      case Op::NEW_CLASS: return "NEW_CLASS";
      // Add other cases as needed
      default: return "UNKNOWN_OP";
    }
  }
  // Convert a string representation to an Op enum
  inline Op string_to_op(const std::string& str) {
    if (str == "PUSH_CONST") return Op::PUSH_CONST;
    if (str == "PUSH_VAR") return Op::PUSH_VAR;
    if (str == "SET_VAR") return Op::SET_VAR;
    if (str == "GET_FIELD") return Op::GET_FIELD;
    if (str == "CALL_METHOD") return Op::CALL_METHOD;
    if (str == "NEW_CLASS") return Op::NEW_CLASS;
	// Add
    // other cases as needed
    return Op::UNKNOWN_OP; // Default case
  }
  // Slurp a file into a string
  inline std::string slurp(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }
  // Emit code to a file
  inline void emit_to_file(const std::string& code, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) throw std::runtime_error("Could not open output file: " + filename);
    file << code;
  }
  // Run internal tests
  inline int run_tests(bool verbose = false) {
    using namespace triad;
    if (verbose) std::cout << "Running internal tests...\n";
	// Add test cases here
    try {
      // Example test case
      std::string test_src = "PUSH_CONST 42\nRETURN";
      Chunk ch = parse_to_chunk(test_src);
      if (verbose) std::cout << "[Parsed test chunk with " << ch.code().size() << " instructions]\n";
      if (ch.code().empty()) throw std::runtime_error("Test chunk is empty");
      run_vm(ch, false); // Run without tracing
      if (verbose) std::cout << "[Test passed]\n";
    } catch (const std::exception& e) {
      std::cerr << "Test failed: " << e.what() << "\n";
      return 1;
    }
    return 0; // All tests passed
  }
  inline void run_vm(const Chunk& ch, bool trace = false) {
    VM vm(ch);
    if (trace) {
      std::cout << "Running VM with chunk:\n";
      ch.dump();
    }
    vm.run(trace); // Run the VM
    if (trace) {
      print_vm_state(vm); // Print VM state after execution
    }
  }
  inline void run_ast(const std::string& src) {
    // Placeholder for AST execution
	  std::cout << "Running AST interpreter on source:\n" << src << "\n";
	  // Here you would parse the source   

      // triad-pro\src\triad_vm.cpp

// --- Mutation tracking, Symbolic overlays, Introspective debugging ---

#include <deque>
#include <mutex>
#include <map>

      namespace triad {

          // --- Mutation tracking: log changes in stack or frame values ---

          struct MutationLogEntry {
              std::string type; // "stack" or "frame"
              size_t index;
              std::string old_value;
              std::string new_value;
          };

          class MutationTracker {
              std::deque<MutationLogEntry> log_;
              std::mutex mtx_;
              static constexpr size_t max_log = 256;
          public:
              void log_stack_change(size_t idx, const std::string& old_val, const std::string& new_val) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  if (log_.size() >= max_log) log_.pop_front();
                  log_.push_back({ "stack", idx, old_val, new_val });
              }
              void log_frame_change(size_t frame, const std::string& key, const std::string& old_val, const std::string& new_val) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  if (log_.size() >= max_log) log_.pop_front();
                  log_.push_back({ "frame:" + std::to_string(frame) + ":" + key, 0, old_val, new_val });
              }
              void print_log(std::ostream& os = std::cout) const {
                  for (const auto& entry : log_) {
                      os << "[Mutation] " << entry.type << " idx=" << entry.index
                          << " old=" << entry.old_value << " new=" << entry.new_value << "\n";
                  }
              }
          };

          static MutationTracker g_mutation_tracker;

          // Example hook for stack mutation (call after stack_ is changed)
          void log_stack_mutation(const std::vector<VMValue>& old_stack, const std::vector<VMValue>& new_stack) {
              size_t n = std::min(old_stack.size(), new_stack.size());
              for (size_t i = 0; i < n; ++i) {
                  if (old_stack[i] != new_stack[i]) {
                      g_mutation_tracker.log_stack_change(i, vmvalue_to_string(old_stack[i]), vmvalue_to_string(new_stack[i]));
                  }
              }
              if (old_stack.size() < new_stack.size()) {
                  for (size_t i = old_stack.size(); i < new_stack.size(); ++i)
                      g_mutation_tracker.log_stack_change(i, "<none>", vmvalue_to_string(new_stack[i]));
              }
              if (old_stack.size() > new_stack.size()) {
                  for (size_t i = new_stack.size(); i < old_stack.size(); ++i)
                      g_mutation_tracker.log_stack_change(i, vmvalue_to_string(old_stack[i]), "<none>");
              }
          }

          // --- Symbolic overlays: annotate stack values with symbolic meaning ---

          using SymbolicMap = std::map<size_t, std::string>; // stack index -> symbolic annotation

          void annotate_stack_symbolic(const VM& vm, const SymbolicMap& sym_map, std::ostream& os = std::cout) {
              os << "Stack symbolic overlays:\n";
              size_t idx = 0;
              visit_stack(vm, [&](const auto& v) {
                  os << "  [" << idx << "] " << vmvalue_to_string(v);
                  auto it = sym_map.find(idx);
                  if (it != sym_map.end()) os << " ; symbolic: " << it->second;
                  os << "\n";
                  ++idx;
                  });
          }

          // --- Introspective debugging: extend print_vm_state() to show execution history or tracebacks ---

          struct ExecutionHistory {
              std::deque<std::string> trace;
              static constexpr size_t max_trace = 128;
              void record(const std::string& info) {
                  if (trace.size() >= max_trace) trace.pop_front();
                  trace.push_back(info);
              }
              void print(std::ostream& os = std::cout) const {
                  os << "Execution history (most recent last):\n";
                  for (const auto& s : trace) os << "  " << s << "\n";
              }
          };

          static ExecutionHistory g_exec_history;

          // Example: call this in VM::exec at each instruction if introspective debugging is enabled
          inline void record_exec_history(size_t ip, const Instr& instr) {
              std::ostringstream oss;
              oss << "IP " << ip << ": " << static_cast<int>(instr.op)
                  << " a=" << instr.a << " b=" << instr.b << " c=" << instr.c;
              g_exec_history.record(oss.str());
          }

          // Extend print_vm_state to show mutation log and execution history
          void print_vm_state_ext(const VM& vm) {
              print_vm_state(vm);
              std::cout << "\n--- Mutation Log ---\n";
              g_mutation_tracker.print_log();
              std::cout << "\n--- Execution History ---\n";
              g_exec_history.print();
          }

      } // namespace triad
	  inline int show_help() {
          std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
          return 0;
	  }
      inline void print_vm_state(const VM& vm) {
    std::cout << "VM State:\n";
    std::cout << "  IP: " << vm.ip_ << "\n";
    std::cout << "  SP: " << vm.sp_ << "\n";
    std::cout << "  Base: " << vm.base_ << "\n";
    std::cout << "  Stack: {";
    for (size_t i = 0; i < vm.stack_.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << vmvalue_to_string(vm.stack_[i]);
    }
    std::cout << "}\n";
    std::cout << "Frames:\n";
	for (const auto& frame : vm.frames_) {
        std::cout << "  Frame:\n";
      std::cout << "    Base: " << frame.base << "\n";
      std::cout << "    IP: " << frame.ip << "\n";
      std::cout << "    Stack: {";
      for (size_t i = 0; i < frame.stack.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << vmvalue_to_string(frame.stack[i]);
      }
      std::cout << "}\n";
    }
    std::cout << "Constants:\n";
    for (const auto& v : vm.chunk_.consts()) {
      if (v.tag == Value::Num)
		  std::cout << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        std::cout << "  Str: \"" << v.str << "\"\n";
	}
    std::cout << "Names:\n";
    for (const auto& name : vm.chunk_.names()) {
      std::cout << "  " << name << "\n";
    }
  }
  inline std::string vmvalue_to_string(const VMValue& v) {
    if (v.tag == VMValue::Num) return std::to_string(v.num);
    else if (v.tag == VMValue::Str) return "\"" + v.str + "\"";
    else if (v.tag == VMValue::Object) return "<Object>";
    else return "<Unknown>";
  }
  // Main entry point
  int main(int argc, char** argv) {
      if (argc < 2 || std::string(argv[1]) == "--help") {
          std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
              << "  --version    Show version\n"
               << "  --help       Show this help message\n";
          return 0;
      }
      if (argc == 2 && std::string(argv[1]) == "--version") {
          std::cout << "Triad Compiler v0.9.1\n";
          return 0;
      }
      if (argc == 2 && std::string(argv[1]) == "--help") {
		  std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
              << "  --version    Show version\n"
               << "  --help       Show this help message\n";
          return 0;
      }
      if (argc < 3) {
          std::cerr << "Error: Not enough arguments.\n"
               << "Usage: triad_vm <mode> <input> [options]\n"
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
              << "  --version    Show version\n"
               << "  --help       Show this help message\n";
          return 1;
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
          if (arg == "-o" && i + 1 < argc) {
              output = argv[++i];
          } else if (arg == "--verbose") {
              verbose = true;
          } else if (arg == "--show-ast") {
              show_ast = true;
          } else if (arg == "--show-bytecode") {
              show_bytecode = true;
          } else if (arg == "--trace-vm") {
              trace_vm = true;
          } else if (arg == "--version") {
              version = true;
          } else if (arg == "--help") {
              return show_help();
          } else {
              std::cerr << "Unknown option: " << arg << "\n";
              return 1;
          }
      }
      if (mode != "run-vm" && mode != "run-ast" && mode != "emit-nasm" && mode != "emit-llvm" && mode != "run-tests") {
          std::cerr << "Unknown mode: " << mode << "\n";
          return 1;
	  }
      try {
          if (mode == "run-tests") {
              return run_tests(verbose);
          }
          std::string src = slurp(input); // Read input file
          if (verbose) std::cout << "Read source code from " << input << "\n";
          Chunk ch = parse_to_chunk(src); // Parse to Chunk
          if (show_ast) {
              ch.dump(); // Print AST
          }
          if (show_bytecode) {
              ch.dump(); // Print bytecode
          }
          if (mode == "run-vm") {
              run_vm(ch, trace_vm); // Run VM
          } else if (mode == "run-ast") {
              run_ast(src); // Run AST interpreter
          } else if (mode == "emit-nasm") {
              std::string asm_code = emit_nasm(ch); // stub
			  if (!output.empty()) emit_to_file(asm_code, output);
              else std::cout << asm_code; // Print to stdout
          } else if (mode == "emit-llvm") {
			  std::string llvm_code = emit_llvm(ch); // stub
              if (!output.empty()) emit_to_file(llvm_code, output);
              else std::cout << llvm_code; // Print to stdout
          }
      } catch (const std::exception& e) {
          std::cerr << "Error: " << e.what() << "\n";
          return 1;
      }
      if (version) {
          std::cout << "Triad Compiler v0.9.1\n";
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
          << "  --version    Show version\n"
                << "  --help       Show this help message\n";
       std::cerr << "Unknown mode: " << mode << "\n"
                 << "Use --help for usage information.\n";
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
           << "  --version    Show version\n"
		   << "  --help       Show this help message\n";
	   return 1;
	   }
#include <iostream>
       namespace triad {
           // --- Bytecode representation ---
           enum class Op {
               PUSH_CONST, PUSH_VAR, SET_VAR, GET_FIELD, CALL_METHOD, NEW_CLASS,
               // Add other operations as needed
               UNKNOWN_OP
           };
           struct Instr {
               Op op;
               int a = -1; // First operand
               int b = -1; // Second operand
               int c = -1; // Third operand
           };
           struct Value {
               enum Tag { Num, Str, Object } tag;
               union {
                   double num;
                   std::string str;
                   void* obj; // Pointer to an object
               };
               Value(double n) : tag(Num), num(n) {}
               Value(const std::string& s) : tag(Str), str(s) {}
               Value(void* o) : tag(Object), obj(o) {}
           };
		   struct Chunk {
               std::vector<Instr> code_;
               std::vector<Value> consts_;
               std::vector<std::string> names_;
               // Add other fields as needed
               const std::vector<Instr>& code() const { return code_; }
               const std::vector<Value>& consts() const { return consts_; }
               const std::vector<std::string>& names() const { return names_; }
               void dump() const {
                   for (const auto& instr : code_) {
                       std::cout << disasm_instr(instr, *this) << "\n";
                   }
               }
           };
           // --- User-defined classes ---
           struct UserClass {
               std::string name;
               std::map<std::string, Chunk> methods; // Method name -> bytecode chunk
           };
           struct Object {
               UserClass* cls; // Pointer to class definition
               std::map<std::string, Value> fields; // Instance fields
           };
           // --- VM execution ---
		   Value execute_method(Object& obj, const std::string& method_name, const std::vector<Value>& args, bool trace = false) {
               if (trace) {
                   std::cout << "Executing method: " << method_name << " on object of class: " << obj.cls->name << "\n";
               }
               auto it = obj.cls->methods.find(method_name);
               if (it == obj.cls->methods.end()) {
                   throw std::runtime_error("Method not found: " + method_name);
               }
               Chunk& ch = it->second; // Get the method's bytecode chunk
               // Execute the chunk (stub, actual execution logic would go here)
               // For now, just return a dummy value
               return Value(0); // Replace with actual return value from method execution
           }
           // --- Purity context ---
           struct PurityContext {
               bool pure = true; // Assume pure by default
               void track_mutation(void* ptr) {
				   pure = false; // Mark as impure if any mutation is detected
                   }
               bool is_pure() const { return pure; }
           };
           static PurityContext g_purity_ctx; // Global purity context
           // --- Loop-invariant code motion (LICM) ---
           void licm_pass(Chunk& ch) {
               // Perform LICM on the chunk (stub)
           }
           // --- Register allocation ---
           void register_allocate(Chunk& ch) {
               // Perform register allocation on the chunk (stub)
		   }
           // --- Peephole compression ---
           void peephole_compress(Chunk& ch) {
               // Perform peephole optimizations on the chunk (stub)
           }
           // --- Parallel execution ---
           template<typename Func>
           void parallel_for(size_t start, size_t end, Func&& func) {
               std::vector<std::thread> threads;
               for (size_t i = start; i < end; ++i) {
                   threads.emplace_back(func, i);
               }
               for (auto& t : threads) {
                   t.join();
               }
		   }
           // --- Profile-guided optimization (PGO) ---
           struct Profile {
               // Placeholder for profiling data
           };
           static Profile g_profile; // Global profile data
           void instrument_profile(Chunk& ch) {
               // Instrument the chunk for profiling (stub)
           }
           void apply_pgo(Chunk& ch, const Profile& profile) {
               // Apply PGO optimizations based on the profile (stub)
           }
       } // namespace triad
	   // --- Main function ---
       #ifdef TRIAD_VM_MAIN
       int main(int argc, char** argv) {
           using namespace triad;
           // Initialize a user-defined class
           UserClass my_class;
           my_class.name = "MyClass";
           // Add a method to the class
           my_class.methods["my_method"] = parse_to_chunk("PUSH_CONST 42\nRETURN");
           // Create an object of that class
           Object obj;
		   obj.cls = &my_class;
           // Call the method on the object
           try {
               Value result = execute_method(obj, "my_method", {}, true);
               std::cout << "Method returned: " << result.num << "\n";
           } catch (const std::exception& e) {
               std::cerr << "Error executing method: " << e.what() << "\n";
           }
           // Check purity context
           if (!g_purity_ctx.is_pure()) {
			   std::cout << "Context is impure.\n";
               } else {
               std::cout << "Context is pure.\n";
           }
           // Perform optimizations
           Chunk ch = parse_to_chunk("PUSH_CONST 42\nPUSH_VAR 0\nSET_VAR 1\nGET_FIELD 2\nCALL_METHOD 3\nNEW_CLASS 4");
           licm_pass(ch);
           register_allocate(ch);
           peephole_compress(ch);
           // Run the VM with the optimized chunk
		   run_vm(ch, true); // Trace VM execution
           // Run internal tests
           return run_tests(true);
       }
#endif // TRIAD_VM_MAIN
       // Parse source code to a Chunk
  inline Chunk parse_to_chunk(const std::string& src) {
    Chunk ch;
    // For simplicity, let's assume src is a series of instructions separated by newlines
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
      if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
      Instr instr;
      std::istringstream instr_stream(line);
      std::string op_str;
      instr_stream >> op_str >> instr.a >> instr.b >> instr.c;
      instr.op = string_to_op(op_str);
      ch.code().push_back(instr);
      // Handle constants and names
      if (instr.op == Op::PUSH_CONST && instr.a >= 0) {
        Value v(static_cast<double>(instr.a)); // Example conversion, adjust as needed
        ch.consts().push_back(v);
      }
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0) {
        ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
    }
    return ch;
  }
  // Dump a Chunk to a human-readable format
  inline void dump_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Chunk Dump ==\n";
    for (const auto& instr : ch.code()) {
      os << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
	for (const auto& v : ch.consts()) {
        if (v.tag == Value::Num)
        os << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  Str: \"" << v.str << "\"\n";
      else if (v.tag == Value::Object)
        os << "  Object: <object pointer>\n"; // Placeholder for object representation
    }
    os << "Names:\n";
    for (const auto& name : ch.names()) {
      os << "  " << name << "\n";
    }
  }
  // Serialize a Chunk to a human-readable format
  inline std::string serialize_chunk(const Chunk& ch) {
    std::ostringstream oss;
    oss << "Chunk:\n";
    for (const auto& instr : ch.code()) {
      oss << disasm_instr(instr, ch) << "\n";
    }
    oss << "Constants:\n";
    for (const auto& v : ch.consts()) {
      if (v.tag == Value::Num)
        oss << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        oss << "  Str: \"" << v.str << "\"\n";
      else if (v.tag == Value::Object)
        oss << "  Object: <object pointer>\n"; // Placeholder for object representation
	}
    oss << "Names:\n";
    for (const auto& name : ch.names()) {
      oss << "  " << name << "\n";
    }
    return oss.str();
  }
  // Disassemble a Chunk to a human-readable format
  inline void disasm_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Disassembly ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "[" << i << "] " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
	}
    os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Serialize a single instruction to a human-readable format
  inline std::string serialize_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op);
    if (instr.a != -1) oss << " " << instr.a;
    if (instr.b != -1) oss << ", " << instr.b;
    if (instr.c != -1) oss << ", " << instr.c;
    // Add names for certain instructions
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
    else if (instr.op == Op::PUSH_CONST
        && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
	  if (v.tag == Value::Num) {
          oss << " ; const: " << v.num;
      } else if (v.tag == Value::Str) {
        oss << " ; const: \"" << v.str << "\"";
      }
    }
	return oss.str();
    }
  // Parse a Chunk from a human-readable format
  inline Chunk parse_from_string(const std::string& src) {
    Chunk ch;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
		if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
        Instr instr;
      std::istringstream instr_stream(line);
      std::string op_str;
	  instr_stream >> op_str >> instr.a >> instr.b >> instr.c;
      instr.op = string_to_op(op_str);
      ch.code().push_back(instr);
      // Handle constants and names
      if (instr.op == Op::PUSH_CONST && instr.a >= 0) {
		  // Example conversion, adjust as needed
		  Value v(static_cast<double>(instr.a)); // Assuming a numeric constant for simplicity
          ch.consts().push_back(v);
      }
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
        ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
      else if (instr.op == Op::PUSH_CONST
          && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
        const Value& v = ch.consts()[instr.a];
        if (v.tag == Value::Num) {
          ch.consts().push_back(Value(v.num)); // Assuming numeric constant for simplicity
        } else if (v.tag == Value::Str) {
          ch.consts().push_back(Value(v.str)); // Assuming string constant for simplicity
        }
      }
	  else if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
        ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
    }
	return ch;
    }
  // Print a Chunk in a human-readable format
  inline void print_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Chunk ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "[" << i << "] " << disasm_instr(instr, ch) << "\n";
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
      else if (v.tag == Value::Object)
        os << "  [" << i << "] Object: <object pointer>\n"; // Placeholder for object representation
    }
    os << "Names:\n";
	for (size_t i = 0; i < ch.names().size(); ++i) {
        os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Disassemble a single instruction to a human-readable format
  inline std::string disasm_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op);
    if (instr.a != -1) oss << " " << instr.a;
    if (instr.b != -1) oss << ", " << instr.b;
    if (instr.c != -1) oss << ", " << instr.c;
    // Add names for certain instructions
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
    else if (instr.op == Op::PUSH_CONST
        && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
	  if (v.tag == Value::Num) {
          oss << " ; const: " << v.num;
      } else if (v.tag == Value::Str) {
        oss << " ; const: \"" << v.str << "\"";
      }
    }
	return oss.str();
    }
  // Convert an operation string to Op enum
  inline Op string_to_op(const std::string& str) {
    if (str == "PUSH_CONST") return Op::PUSH_CONST;
    else if (str == "PUSH_VAR") return Op::PUSH_VAR;
    else if (str == "SET_VAR") return Op::SET_VAR;
    else if (str == "GET_FIELD") return Op::GET_FIELD;
    else if (str == "CALL_METHOD") return Op::CALL_METHOD;
    else if (str == "NEW_CLASS") return Op::NEW_CLASS;
    else return Op::UNKNOWN_OP; // Handle unknown operations
  }
  // Convert an Op enum to a string
  inline std::string op_to_string(Op op) {
    switch (op) {
      case Op::PUSH_CONST: return "PUSH_CONST";
      case Op::PUSH_VAR: return "PUSH_VAR";
      case Op::SET_VAR: return "SET_VAR";
      case Op::GET_FIELD: return "GET_FIELD";
      case Op::CALL_METHOD: return "CALL_METHOD";
      case Op::NEW_CLASS: return "NEW_CLASS";
      default: return "UNKNOWN_OP";
	}
    }
  // Read a file into a string
  inline std::string slurp(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
  }
  // Run internal tests
  inline int run_tests(bool verbose = false) {
    try {
      Chunk ch = parse_to_chunk("PUSH_CONST 42\nRETURN"); // Example test chunk
	  if (verbose) std::cout << "[Running tests]\n";
      run_vm(ch, false); // Run VM without tracing
      if (verbose) std::cout << "[Tests passed]\n";
      return 0; // Indicate success
    } catch (const std::exception& e) {
      std::cerr << "Test failed: " << e.what() << "\n";
      return 1; // Indicate failure
	}
    }
  // Emit a string to a file
  inline void emit_to_file(const std::string& content, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) throw std::runtime_error("Could not open file for writing: " + filename);
    file << content;
  }
  // Emit x86-64 assembly (stub)
  inline std::string emit_nasm(const Chunk& ch) {
    std::ostringstream oss;
    oss << "; NASM assembly output\n";
    for (const auto& instr : ch.code()) {
      oss << serialize_instr(instr, ch) << "\n";
	}
    oss << "; Constants:\n";
    for (const auto& v : ch.consts()) {
      if (v.tag == Value::Num) {
        oss << "  db " << v.num << "\n"; // Example, adjust as needed
      } else if (v.tag == Value::Str) {
        oss << "  db \"" << v.str << "\", 0\n"; // Null-terminated string
      }
    }
    return oss.str();
  }
  // Emit LLVM IR (stub)
  inline std::string emit_llvm(const Chunk& ch) {
    std::ostringstream oss;
    oss << "; LLVM IR output\n";
    for (const auto& instr : ch.code()) {
      oss << serialize_instr(instr, ch) << "\n";
    }
    return oss.str();
  }
  // --- Mutation tracking: track changes to stack and frames ---
  namespace triad {
          struct MutationTracker {
          private:
              struct LogEntry {
                  std::string type; // "stack" or "frame"
                  size_t index; // Stack index or frame number
                  std::string old_value;
                  std::string new_value;
              };
              std::deque<LogEntry> log_;
			  mutable std::mutex mtx_;
              public:
              void log_stack_change(size_t index, const std::string& old_value, const std::string& new_value) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  log_.push_back({"stack", index, old_value, new_value});
              }
              void log_frame_change(size_t frame_num, const std::string& old_value, const std::string& new_value) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  log_.push_back({"frame", frame_num, old_value, new_value});
              }
              void print_log(std::ostream& os = std::cout) const {
                  os << "Mutation Log:\n";
                  for (const auto& entry : log_) {
                      os << "  [" << entry.type << "] Index: " << entry.index
                         << " Old: " << entry.old_value
                         << " New: " << entry.new_value << "\n";
                  }
              }
          };
		  static MutationTracker g_mutation_tracker; // Global mutation tracker
          // Example usage in VM execution
          inline void track_stack_change(const VM& vm, size_t index, const VMValue& old_value, const VMValue& new_value) {
              if (g_mutation_tracker) {
                  g_mutation_tracker.log_stack_change(index, vmvalue_to_string(old_value), vmvalue_to_string(new_value));
              }
          }
          inline void track_frame_change(const VM& vm, size_t frame_num, const VMValue& old_value, const VMValue& new_value) {
              if (g_mutation_tracker) {
				  g_mutation_tracker.log_frame_change(frame_num, vmvalue_to_string(old_value), vmvalue_to_string(new_value));
                  }
          }
          // Print VM state with symbolic names
          inline void print_vm_state(const VM& vm, const std::unordered_map<size_t, std::string>& sym_map, std::ostream& os = std::cout) {
              os << "VM State:\n";
              os << "  IP: " << vm.ip_ << "\n";
              os << "  SP: " << vm.sp_ << "\n";
              os << "  Base: " << vm.base_ << "\n";
              os << "  Stack: {";
              for (size_t i = 0; i < vm.stack_.size(); ++i) {
                  if (i > 0) os << ", ";
                  os << vmvalue_to_string(vm.stack_[i]);
              }
              os << "}\n";
              os << "Frames:\n";
              for (const auto& frame : vm.frames_) {
                  os << "  Frame:\n";
                  os << "    Base: " << frame.base << "\n";
                  os << "    IP: " << frame.ip << "\n";
                  os << "    Stack: {";
                  for (size_t i = 0; i < frame.stack.size(); ++i) {
                      if (i > 0) os << ", ";
                      os << vmvalue_to_string(frame.stack[i]);
                  }
                  os << "}\n";
              }
              os << "Constants:\n";
              for (const auto& v : vm.chunk_.consts()) {
                  if (v.tag == Value::Num)
                      os << "  Num: " << v.num << "\n";
                  else if (v.tag == Value::Str)
                      os << "  Str: \"" << v.str << "\"\n";
                  else if (v.tag == Value::Object)
					  os << "  Object: <object pointer>\n"; // Placeholder for object representation
                  }
              os << "Names:\n";
              for (const auto& name : vm.chunk_.names()) {
                  os << "  " << name << "\n";
              }
              // Print symbolic names for stack and frames
              os << "Symbolic Names:\n";
              for (const auto& [index, sym] : sym_map) {
                  os << "  Index: " << index << " -> Symbol: " << sym << "\n";
              }
		  }
          inline int show_help() {
          std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
              << "  --trace-vm   Trace VM execution\n"
              << "  --version    Show version\n"
              << "  --help       Show this help message\n";
          return 0;
		  }
		  // Print VM state with symbolic names
		  inline void print_vm_state(const VM& vm, std::ostream& os = std::cout) {
			  os << "VM State:\n";
			  os << "  IP: " << vm.ip_ << "\n";
			  os << "  SP: " << vm.sp_ << "\n";
			  os << "  Base: " << vm.base_ << "\n";
              os << "  Stack: {";
      for (size_t i = 0; i < vm.stack_.size(); ++i) {
        if (i > 0) os << ", ";
        os << vmvalue_to_string(vm.stack_[i]);
      }
      os << "}\n";
      os << "Frames:\n";
      for (const auto& frame : vm.frames_) {
        os << "  Frame:\n";
        os << "    Base: " << frame.base << "\n";
        os << "    IP: " << frame.ip << "\n";
        os << "    Stack: {";
		for (size_t i = 0; i < frame.stack.size(); ++i) {
            if (i > 0) os << ", ";
          os << vmvalue_to_string(frame.stack[i]);
        }
        os << "}\n";
      }
      os << "Constants:\n";
      for (const auto& v : vm.chunk_.consts()) {
        if (v.tag == Value::Num)
          os << "  Num: " << v.num << "\n";
        else if (v.tag == Value::Str)
          os << "  Str: \"" << v.str << "\"\n";
        else if (v.tag == Value::Object)
          os << "  Object: <object pointer>\n"; // Placeholder for object representation
      }
      os << "Names:\n";
	  for (const auto& name : vm.chunk_.names()) {
          os << "  " << name << "\n";
      }
      // Print symbolic names for stack and frames
      os << "Symbolic Names:\n";
      for (const auto& [index, sym] : vm.sym_map_) {
        os << "  Index: " << index << " -> Symbol: " << sym << "\n";
      }
		  }
          inline int main(int argc, char** argv) {
      if (argc == 1) {
          std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
              << "  --version    Show version\n"
              << "  --help       Show this help message\n";
          return 0;
      }
      if (argc == 2 && std::string(argv[1]) == "--help") {
          std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
              << "  --version    Show version\n"
               << "  --help       Show this help message\n";
          return 0;
      }
      if (argc < 3) {
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
              << "  --version    Show version\n"
               << "  --help       Show this help message\n";
          return 1;
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
          if (arg == "-o" && i + 1 < argc) {
              output = argv[++i];
		  }
		  else if (arg == "--verbose") {
              verbose = true;
          } else if (arg == "--show-ast") {
              show_ast = true;
          } else if (arg == "--show-bytecode") {
              show_bytecode = true;
          } else if (arg == "--trace-vm") {
              trace_vm = true;
		  }
		  else if (arg == "--version") {
              version = true;
          } else if (arg == "--help") {
              return show_help();
		  }
		  else {
              std::cerr << "Unknown option: " << arg << "\n"
				  << "Use --help for usage information.\n";
              return 1;
          }
	  }
      if (mode == "run-vm" || mode == "run-ast" || mode == "emit-nasm" || mode == "emit-llvm" || mode == "run-tests") {
          if (verbose) std::cout << "Running in mode: " << mode << "\n";
          if (verbose) std::cout << "Input file: " << input << "\n";
          if (!output.empty()) std::cout << "Output file: " << output << "\n";
      } else {
          std::cerr << "Unknown mode: " << mode << "\n"
                    << "Use --help for usage information.\n";
          return 1;
      }
	  try {
          Chunk ch;
          if (mode == "run-vm" || mode == "run-ast") {
              std::string src = slurp(input);
              ch = parse_to_chunk(src);
              if (show_ast) {
                  dump_chunk(ch);
              }
              if (show_bytecode) {
                  disasm_chunk(ch);
              }
          } else if (mode == "emit-nasm" || mode == "emit-llvm") {
              ch = parse_to_chunk(slurp(input));
          } else if (mode == "run-tests") {
              return run_tests(verbose);
          }
          if (mode == "run-vm") {
              VM vm(ch, trace_vm); // Create VM instance
              vm.run(); // Run the VM
              if (verbose) print_vm_state(vm); // Print VM state if verbose
          } else if (mode == "run-ast") {
              // Execute AST interpreter logic here (stub)
          } else if (mode == "emit-nasm") {
			  std::string asm_code = emit_nasm(ch); // stub
              if (!output.empty()) {
                  emit_to_file(asm_code, output);
              } else {
                  std::cout << asm_code; // Print to stdout
              }
		  }
		  else if (mode == "emit-llvm") {
              std::string llvm_code = emit_llvm(ch); // stub
              if (!output.empty()) {
                  emit_to_file(llvm_code, output);
              } else {
                  std::cout << llvm_code; // Print to stdout
              }
          }
      } catch (const std::exception& e) {
          std::cerr << "Error: " << e.what() << "\n";
          return 1;
      }
      if (version) {
          std::cout << "Triad VM version 1.0.0\n"; // Replace with actual version
      }
	  return 0;
      } else {
          std::cerr << "Unknown mode: " << mode << "\n"
                    << "Use --help for usage information.\n";
          return 1;
      }
       }
	   std::cout << "Usage: triad_vm <mode> <input> [options]\n"
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
           << "  --version    Show version\n"
           << "  --help       Show this help message\n";
	   return 1;
	   }
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>
#include <fstream>
#include <thread>
#include <mutex>
#include <deque>
       namespace triad {
           // --- VM operations ---
           enum class Op {
               PUSH_CONST, PUSH_VAR, SET_VAR, GET_FIELD, CALL_METHOD, NEW_CLASS,
               // Add other operations as needed
               UNKNOWN_OP
           };
           inline std::string op_to_string(Op op) {
               switch (op) {
                   case Op::PUSH_CONST: return "PUSH_CONST";
                   case Op::PUSH_VAR: return "PUSH_VAR";
                   case Op::SET_VAR: return "SET_VAR";
                   case Op::GET_FIELD: return "GET_FIELD";
                   case Op::CALL_METHOD: return "CALL_METHOD";
                   case Op::NEW_CLASS: return "NEW_CLASS";
                   default: return "UNKNOWN_OP";
               }
           }
           struct Instr {
               Op op;
               int a = -1; // Operand A
               int b = -1; // Operand B
			   int c = -1; // Operand C
               Instr(Op op, int a = -1, int b = -1, int c = -1) : op(op), a(a), b(b), c(c) {}
           };
           // --- Value representation ---
           struct Value {
               enum Tag { Num, Str, Object } tag;
               union {
                   double num; // Numeric value
                   std::string str; // String value
                   void* obj; // Pointer to object
               };
               Value(double n) : tag(Num), num(n) {}
               Value(const std::string& s) : tag(Str), str(s) {}
               Value(void* o) : tag(Object), obj(o) {}
           };
           // --- Chunk representation ---
           struct Chunk {
               std::vector<Instr> code_;
			   std::vector<Value> consts_;
               std::vector<std::string> names_;
               const std::vector<Instr>& code() const { return code_; }
               std::vector<Instr>& code() { return code_; }
               const std::vector<Value>& consts() const { return consts_; }
               std::vector<Value>& consts() { return consts_; }
               const std::vector<std::string>& names() const { return names_; }
               std::vector<std::string>& names() { return names_; }
           };
           // --- User-defined class representation ---
           struct UserClass {
               std::string name;
               std::map<std::string, Chunk> methods; // Method name to bytecode mapping
           };
           // --- Object representation ---
           struct Object {
               UserClass* cls; // Pointer to the class definition
           };
           // --- VM state ---
           struct VM {
			   Chunk chunk_;
               std::vector<Value> stack_; // Stack for VM execution
               std::vector<std::string> names_; // Symbolic names for stack and frames
               size_t ip_ = 0; // Instruction pointer
               size_t sp_ = 0; // Stack pointer
               size_t base_ = 0; // Base pointer for frames
			   std::vector<Chunk> frames_; // Frames for method calls
               VM(const Chunk& chunk, bool trace = false) : chunk_(chunk), trace_(trace) {
                   // Initialize VM state
               }
               void run() {
                   // Run the VM execution loop (stub)
                   while (ip_ < chunk_.code().size()) {
                       const Instr& instr = chunk_.code()[ip_];
                       if (trace_) {
                           std::cout << "Executing: " << op_to_string(instr.op) << "\n";
                       }
                       switch (instr.op) {
                           case Op::PUSH_CONST:
                               stack_.push_back(chunk_.consts()[instr.a]);
                               break;
                           case Op::PUSH_VAR:
                               stack_.push_back(Value(stack_[instr.a])); // Example, adjust as needed
                               break;
                           case Op::SET_VAR:
                               stack_[instr.a] = stack_[sp_ - 1]; // Set variable
                               --sp_;
                               break;
                           case Op::GET_FIELD:
                               // Get field from object (stub)
                               break;
                           case Op::CALL_METHOD:
                               // Call method on object (stub)
                               break;
                           case Op::NEW_CLASS:
                               // Create new class instance (stub)
                               break;
                           default:
                               throw std::runtime_error("Unknown operation: " + op_to_string(instr.op));
                       }
                       ++ip_;
                   }
               }
           private:
               bool trace_; // Enable tracing of VM execution
           };
           // --- Execution ---
           Value execute_method(Object& obj, const std::string& method_name, const std::vector<Value>& args, bool trace = false) {
               auto it = obj.cls->methods.find(method_name);
               if (it == obj.cls->methods.end()) {
                   throw std::runtime_error("Method not found: " + method_name);
               }
			   Chunk& method_chunk = it->second;
               VM vm(method_chunk, trace);
			   vm.stack_ = args; // Initialize stack with arguments
               vm.run(); // Run the method
               if (vm.sp_ > 0) {
                   return vm.stack_[vm.sp_ - 1]; // Return last value on stack
               }
               return Value(0.0); // Default return value if stack is empty
           }
       } // namespace triad
	   // --- Main function ---
       #ifdef TRIAD_VM_MAIN
       int main(int argc, char** argv) {
           if (argc < 2) {
               std::cerr << "Usage: triad_vm <source_file>\n";
               return 1;
           }
           std::string source_file = argv[1];
           // Read source code from file
           std::string src = slurp(source_file);
           // Parse source code to a Chunk
           Chunk ch = parse_to_chunk(src);
           // Dump the Chunk for debugging
           dump_chunk(ch);
           // Run optimizations and VM execution
		   optimize_chunk(ch);
           VM vm(ch, false); // Create VM instance without tracing
           vm.run(); // Run the VM
           // Print final VM state
           print_vm_state(vm);
           return 0;
       }
#endif // TRIAD_VM_MAIN
       // Parse a source string to a Chunk
  inline Chunk parse_to_chunk(const std::string& src) {
    Chunk ch;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
      if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
      Instr instr;
      std::istringstream instr_stream(line);
	  std::string op_str;
      instr_stream >> op_str >> instr.a >> instr.b >> instr.c;
      instr.op = string_to_op(op_str);
      ch.code().push_back(instr);
      // Handle constants and names
      if (instr.op == Op::PUSH_CONST && instr.a >= 0) {
        // Example conversion, adjust as needed
        Value v(static_cast<double>(instr.a)); // Assuming a numeric constant for simplicity
        ch.consts().push_back(v);
      }
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
          && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
        ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
      else if (instr.op == Op::PUSH_CONST
          && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
        const Value& v = ch.consts()[instr.a];
        if (v.tag == Value::Num) {
          ch.consts().push_back(Value(v.num)); // Assuming numeric constant for simplicity
        } else if (v.tag == Value::Str) {
          ch.consts().push_back(Value(v.str)); // Assuming string constant for simplicity
        }
      }
      else if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
		  && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
          ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
	}
    return ch;
  }
  // Dump a Chunk to a human-readable format
  inline std::string dump_chunk(const Chunk& ch) {
    std::ostringstream oss;
    oss << "== Chunk Dump ==\n";
    oss << "Code:\n";
    for (const auto& instr : ch.code()) {
      oss << "  " << disasm_instr(instr, ch) << "\n";
    }
    oss << "Constants:\n";
    for (const auto& v : ch.consts()) {
      if (v.tag == Value::Num)
		  oss << "  Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        oss << "  Str: \"" << v.str << "\"\n";
      else if (v.tag == Value::Object)
        oss << "  Object: <object pointer>\n"; // Placeholder for object representation
	}
    oss << "Names:\n";
    for (const auto& name : ch.names()) {
      oss << "  " << name << "\n";
    }
    return oss.str();
  }
  // Disassemble a Chunk to a human-readable format
  inline void disasm_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Disassembled Chunk ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
	  os << "[" << i << "] " << disasm_instr(instr, ch) << "\n"; // Use disasm_instr to format each instruction
      }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
        os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
      else if (v.tag == Value::Object)
        os << "  [" << i << "] Object: <object pointer>\n"; // Placeholder for object representation
    }
	os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Serialize an instruction to a human-readable format
  inline std::string serialize_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op);
    if (instr.a != -1) oss << " " << instr.a;
    if (instr.b != -1) oss << ", " << instr.b;
    if (instr.c != -1) oss << ", " << instr.c;
    // Add names for certain instructions
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
	else if (instr.op == Op::PUSH_CONST
        && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
      if (v.tag == Value::Num) {
        oss << " ; const: " << v.num;
      } else if (v.tag == Value::Str) {
        oss << " ; const: \"" << v.str << "\"";
      }
    }
    return oss.str();
  }
  // Parse a source string to a Chunk
  inline Chunk parse_to_chunk(const std::string& src) {
    Chunk ch;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
		if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
        Instr instr;
      std::istringstream instr_stream(line);
      std::string op_str;
      instr_stream >> op_str >> instr.a >> instr.b >> instr.c;
      instr.op = string_to_op(op_str);
      ch.code().push_back(instr);
      // Handle constants and names
      if (instr.op == Op::PUSH_CONST && instr.a >= 0) {
        // Example conversion, adjust as needed
        Value v(static_cast<double>(instr.a)); // Assuming a numeric constant for simplicity
        ch.consts().push_back(v);
      }
      if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
		  && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
          ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
      else if (instr.op == Op::PUSH_CONST
          && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
        const Value& v = ch.consts()[instr.a];
        if (v.tag == Value::Num) {
          ch.consts().push_back(Value(v.num)); // Assuming numeric constant for simplicity
        } else if (v.tag == Value::Str) {
          ch.consts().push_back(Value(v.str)); // Assuming string constant for simplicity
        }
      }
      else if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
		  && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
          ch.names().push_back("name_" + std::to_string(instr.a)); // Example name, adjust as needed
      }
	}
    return ch;
  }
  // Disassemble a Chunk to a human-readable format
  inline void disasm_chunk(const Chunk& ch, std::ostream& os = std::cout) {
    os << "== Disassembled Chunk ==\n";
    for (size_t i = 0; i < ch.code().size(); ++i) {
      const Instr& instr = ch.code()[i];
      os << "[" << i << "] " << disasm_instr(instr, ch) << "\n"; // Use disasm_instr to format each instruction
    }
    os << "Constants:\n";
    for (size_t i = 0; i < ch.consts().size(); ++i) {
      const Value& v = ch.consts()[i];
      if (v.tag == Value::Num)
		  os << "  [" << i << "] Num: " << v.num << "\n";
      else if (v.tag == Value::Str)
        os << "  [" << i << "] Str: \"" << v.str << "\"\n";
      else if (v.tag == Value::Object)
        os << "  [" << i << "] Object: <object pointer>\n"; // Placeholder for object representation
	}
    os << "Names:\n";
    for (size_t i = 0; i < ch.names().size(); ++i) {
      os << "  [" << i << "] " << ch.names()[i] << "\n";
    }
  }
  // Disassemble an instruction to a human-readable format
  inline std::string disasm_instr(const Instr& instr, const Chunk& ch) {
    std::ostringstream oss;
    oss << op_to_string(instr.op);
    if (instr.a != -1) oss << " " << instr.a;
    if (instr.b != -1) oss << ", " << instr.b;
    if (instr.c != -1) oss << ", " << instr.c;
    // Add names for certain instructions
    if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
	else if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
        && instr.a >= 0 && instr.a < static_cast<int>(ch.names().size())) {
      oss << " ; name: " << ch.names()[instr.a];
    }
    else if (instr.op == Op::PUSH_CONST
        && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
      const Value& v = ch.consts()[instr.a];
      if (v.tag == Value::Num) {
        oss << " ; const: " << v.num;
      } else if (v.tag == Value::Str) {
        oss << " ; const: \"" << v.str << "\"";
      }
    }
    return oss.str();
  }
  // Convert a string to an Op enum
  inline Op string_to_op(const std::string& str) {
    if (str == "PUSH_CONST") return Op::PUSH_CONST;
    else if (str == "PUSH_VAR") return Op::PUSH_VAR;
    else if (str == "SET_VAR") return Op::SET_VAR;
    else if (str == "GET_FIELD") return Op::GET_FIELD;
    else if (str == "CALL_METHOD") return Op::CALL_METHOD;
	else if (str == "NEW_CLASS") return Op::NEW_CLASS;
    else return Op::UNKNOWN_OP; // Handle unknown operations
  }
  // Slurp a file into a string
  inline std::string slurp(const std::string& filename
      ) {
	  std
          ::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream oss;
    oss << file.rdbuf();
	return oss.str();
    }
  // Emit a string to a file
  inline void emit_to_file(const std::string& content, const std::string& filename) {
	  std::ofstream
          file(filename);
	  if (!file) throw std::runtime_error("Could not open file for writing: " + filename);
      file << content;
	  if (!file) throw std::runtime_error("Error writing to file: " + filename);
  }
  // Emit x86-64 assembly (stub)
  inline std::string emit_nasm(const Chunk& ch) {
    std::ostringstream oss;
    oss << "; NASM x86-64 assembly output\n";
    oss << "section .text\n";
    oss << "global _start\n";
    oss << "_start:\n";
    for (const auto& instr : ch.code()) {
		// Serialize each instruction to NASM format
        oss << "  " << serialize_instr(instr, ch) << "\n";
      // Handle constants
      if (instr.op == Op::PUSH_CONST && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
        const Value& v = ch.consts()[instr.a];
        if (v.tag == Value::Num)
          oss << "  db " << v.num << "\n"; // Numeric constant
		else if (v.tag == Value::Str)
            oss << "  db \"" << v.str << "\", 0\n"; // String constant with null terminator
        else if (v.tag == Value::Object)
          oss << "  ; Object constant (not supported in NASM)\n"; // Placeholder for object representation
      }
    }
    oss << "section .data\n";
    // Add data section for constants if needed
    return oss.str();
  }
  // Emit LLVM IR (stub)
  inline std::string emit_llvm(const Chunk& ch) {
    std::ostringstream oss;
    oss << "; LLVM IR output\n";
    oss << "define i32 @main() {\n";
    oss << "  %result = alloca i32\n";
    for (const auto& instr : ch.code()) {
        // Serialize each instruction to LLVM IR format
        oss << "  ; " << serialize_instr(instr, ch) << "\n";
        if (instr.op == Op::PUSH_CONST && instr.a >= 0 && instr.a < static_cast<int>(ch.consts().size())) {
            const Value& v = ch.consts()[instr.a];
            if (v.tag == Value::Num)
                oss << "  store i32 " << static_cast<int>(v.num) << ", i32* %result\n"; // Numeric constant
            else if (v.tag == Value::Str)
                oss << "  ; String constant not supported in LLVM IR\n"; // Placeholder for string representation
            else if (v.tag == Value::Object)
                oss << "  ; Object constant not supported in LLVM IR\n"; // Placeholder for object representation
        }
    }
    oss << "  ret i32 0\n";
    oss << "}\n";
    return oss.str();
  }
          // --- Mutation tracking ---
          struct MutationTracker {
              struct LogEntry {
                  std::string type; // "stack" or "frame"
                  size_t index; // Index of the stack or frame
                  std::string old_value; // Old value as string
                  std::string new_value; // New value as string
              };
              std::vector<LogEntry> log_;
			  mutable std::mutex mtx_; // Mutex for thread-safe logging
              void log_stack_mutation(size_t index, const Value& old_value, const Value& new_value) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  log_.push_back({"stack", index, vmvalue_to_string(old_value), vmvalue_to_string(new_value)});
              }
              void log_frame_mutation(size_t index, const std::vector<Value>& old_stack, const std::vector<Value>& new_stack) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  log_.push_back({"frame", index, stack_to_string(old_stack), stack_to_string(new_stack)});
              }
              std::string stack_to_string(const std::vector<Value>& stack) const {
                  std::ostringstream oss;
                  oss << "{";
                  for (size_t i = 0; i < stack.size(); ++i) {
                      if (i > 0) oss << ", ";
                      oss << vmvalue_to_string(stack[i]);
                  }
                  oss << "}";
                  return oss.str();
              }
          };
          // Convert VMValue to string representation
          inline std::string vmvalue_to_string(const Value& v) {
              if (v.tag == Value::Num)
                  return std::to_string(v.num);
              else if (v.tag == Value::Str)
                  return "\"" + v.str + "\"";
              else if (v.tag == Value::Object)
				  return "<object pointer>"; // Placeholder for object representation
              else
                  return "<unknown value>";
          }
		  // Print VM state with symbolic names
          inline void print_vm_state(const VM& vm, std::ostream& os = std::cout) {
              os << "VM State:\n";
              os << "  IP: " << vm.ip_ << "\n";
              os << "  SP: " << vm.sp_ << "\n";
              os << "  Base: " << vm.base_ << "\n";
              os << "  Stack: {";
              for (size_t i = 0; i < vm.stack_.size(); ++i) {
                  if (i > 0) os << ", ";
                  os << vmvalue_to_string(vm.stack_[i]);
              }
              os << "}\n";
              os << "Frames:\n";
              for (const auto& frame : vm.frames_) {
                  os << "  Frame:\n";
                  os << "    Base: " << frame.base << "\n";
                  os << "    IP: " << frame.ip << "\n";
                  os << "    Stack: {";
                  for (size_t i = 0; i < frame.stack.size(); ++i) {
                      if (i > 0) os << ", ";
                      os << vmvalue_to_string(frame.stack[i]);
                  }
                  os << "}\n";
              }
              os << "Constants:\n";
              for (const auto& v : vm.chunk_.consts()) {
                  if (v.tag == Value::Num)
					  os << "  Num: " << v.num << "\n";
                  else if (v.tag == Value::Str)
                      os << "  Str: \"" << v.str << "\"\n";
                  else if (v.tag == Value::Object)
                      os << "  Object: <object pointer>\n"; // Placeholder for object representation
              }
              os << "Names:\n";
              for (const auto& name : vm.chunk_.names()) {
                  os << "  " << name << "\n";
              }
              // Print symbolic names for stack and frames
              os << "Symbolic Names:\n";
              for (const auto& [index, sym] : vm.sym_map_) {
                  os << "  Index: " << index << " -> Symbol: " << sym << "\n";
              }
		  }
          // Print VM state with symbolic names
          inline void print_vm_state(const VM& vm, std::ostream& os = std::cout) {
              os << "VM State:\n";
              os << "  IP: " << vm.ip_ << "\n";
              os << "  SP: " << vm.sp_ << "\n";
              os << "  Base: " << vm.base_ << "\n";
			  os << "  Stack: {";
              for (size_t i = 0; i < vm.stack_.size(); ++i) {
                  if (i > 0) os << ", ";
                  os << vmvalue_to_string(vm.stack_[i]);
              }
              os << "}\n";
      os << "Frames:\n";
      for (const auto& frame : vm.frames_) {
        os << "  Frame:\n";
		os << "    Base: " << frame.base << "\n";
