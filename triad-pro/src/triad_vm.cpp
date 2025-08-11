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
