#include "triad_lexer.hpp"
#include "triad_ast.hpp"
#include "triad_bytecode.hpp"
#include <stdexcept>
#include <memory>
#include <iostream>

namespace triad {

struct Parser {
  std::vector<Token> t; size_t i=0;
  Parser(std::vector<Token> T):t(std::move(T)) {}
  const Token& P() const { return t[i]; }
  const Token& A(){ if (i<t.size()) ++i; return t[i-1]; }
  bool M(TokKind k){ if (P().k==k){ A(); return true; } return false; }
  void W(TokKind k,const char* m){ if(!M(k)) throw std::runtime_error(m); }

  // Codegen helpers
  Chunk ch;
  int K(double d){ return ch.addConst(Value::number(d)); }
  int KS(const std::string&s){ return ch.addConst(Value::string(s)); }
  int N(const std::string& s){ return ch.addName(s); }
  void E(Op op,int a=0,int b=0,int c=0){ ch.emit(op,a,b,c); }
  int EJ(Op op){ return ch.emit(op,-1,0,0); }

  Chunk parse(){
    while (P().k!=TokKind::Eof){
      parseStmt();
      M(TokKind::Semicolon);
    }
    E(Op::RET);
    return std::move(ch);
  }

  void parseStmt(){
    if (M(TokKind::KwIf)){ parseIf(); return; }
    if (M(TokKind::KwFor)){ parseFor(); return; }
    if (M(TokKind::KwSay)){ parseExpr(); E(Op::SAY); return; }
    if (M(TokKind::KwEcho)){ parseExpr(); E(Op::ECHO); return; }
    if (P().k==TokKind::Id && t[i+1].k==TokKind::Eq){ std::string n=A().s; A(); parseExpr(); E(Op::SET_VAR, N(n)); return; }
    parseExpr();
  }

  void parseBlock(){
    W(TokKind::LBrace,"{");
    while (P().k!=TokKind::RBrace && P().k!=TokKind::Eof){
      parseStmt();
      M(TokKind::Semicolon);
    }
    W(TokKind::RBrace,"}");
  }

  void parseIf(){
    W(TokKind::LParen,"("); parseExpr(); W(TokKind::RParen,")");
    int jElse = EJ(Op::IF_FALSE_JMP);
    parseBlock();
    int jEnd = EJ(Op::JMP);
    ch.code[jElse].a = (int)ch.code.size();
    if (M(TokKind::KwElse)) parseBlock();
    ch.code[jEnd].a = (int)ch.code.size();
  }

  void parseFor(){
    if (P().k!=TokKind::Id) throw std::runtime_error("for ident");
    std::string iv = A().s;
    W(TokKind::KwIn,"in");
    parseExpr(); W(TokKind::Range,".."); parseExpr();
    int ivar = N(iv);
    int zero = K(0), one=K(1);
    E(Op::PUSH_CONST, zero); E(Op::SET_VAR, ivar);
    int loopStart = (int)ch.code.size();
    E(Op::PUSH_VAR, ivar); E(Op::PUSH_CONST, one); E(Op::LT);
    int jExit = EJ(Op::IF_FALSE_JMP);
    parseBlock();
    E(Op::PUSH_VAR, ivar); E(Op::PUSH_CONST, one); E(Op::ADD); E(Op::SET_VAR, ivar);
    E(Op::JMP, loopStart);
    ch.code[jExit].a = (int)ch.code.size();
  }

  // Expr precedence
  void parseExpr(){ parseOr(); }
  void parseOr(){
    parseAnd();
    while (M(TokKind::KwOr)){
      int t=0; E(Op::SC_OR_BEGIN,t);
      int j=EJ(Op::SC_OR_EVAL);
      parseAnd();
      int end = ch.emit(Op::SC_OR_END);
      ch.code[j].b = end;
    }
  }
  void parseAnd(){
    parseCmp();
    while (M(TokKind::KwAnd)){
      int t=0; E(Op::SC_AND_BEGIN,t);
      int j=EJ(Op::SC_AND_EVAL);
      parseCmp();
      int end = ch.emit(Op::SC_AND_END);
      ch.code[j].b = end;
    }
  }
  void parseCmp(){
    parseAdd();
    while (P().k==TokKind::EqEq||P().k==TokKind::Ne||P().k==TokKind::Lt||P().k==TokKind::Le||P().k==TokKind::Gt||P().k==TokKind::Ge){
      TokKind k=A().k; parseAdd();
      E(k==TokKind::EqEq?Op::EQ: k==TokKind::Ne?Op::NE: k==TokKind::Lt?Op::LT: k==TokKind::Le?Op::LE: k==TokKind::Gt?Op::GT:Op::GE);
    }
  }
  void parseAdd(){
    parseMul();
    while (P().k==TokKind::Plus || P().k==TokKind::Minus){
      TokKind k=A().k; parseMul(); E(k==TokKind::Plus?Op::ADD:Op::SUB);
    }
  }
  void parseMul(){
    parseUnary();
    while (P().k==TokKind::Star||P().k==TokKind::Slash||P().k==TokKind::Percent){
      TokKind k=A().k; parseUnary(); E(k==TokKind::Star?Op::MUL: k==TokKind::Slash?Op::DIV:Op::MOD);
    }
  }
  void parseUnary(){
    if (M(TokKind::Minus)){ parseUnary(); E(Op::NEG); return; }
    if (M(TokKind::Bang)){ parseUnary(); E(Op::NOT); return; }
    parsePrimary();
  }

  void parsePrimary(){
    if (M(TokKind::LParen)){
      int nargs=0; if (P().k!=TokKind::RParen){ do{ parseExpr(); ++nargs; } while (M(TokKind::Comma)); }
      W(TokKind::RParen,")"); if (nargs>1) E(Op::MAKE_TUPLE, nargs); return;
    }
    if (M(TokKind::Num)){ int k=K(t[i-1].n); E(Op::PUSH_CONST,k); return; }
    if (M(TokKind::Str)){ int k=KS(t[i-1].s); E(Op::PUSH_CONST,k); return; }
    if (M(TokKind::KwNew)){ if (P().k!=TokKind::Id) throw std::runtime_error("class"); std::string cls=A().s;
      W(TokKind::LParen,"("); int argc=0; if (P().k!=TokKind::RParen){ do{ parseExpr(); ++argc; } while(M(TokKind::Comma)); } W(TokKind::RParen,")");
      E(Op::NEW_CLASS, N(cls)); if (argc>0) E(Op::CALL_METHOD, N("init"), argc); return; }
    if (M(TokKind::Id)){ int k=N(t[i-1].s); E(Op::PUSH_VAR,k);
      // chain: .name or [index] and call .name(...)
      for(;;){
        if (M(TokKind::Dot)){
          if (P().k!=TokKind::Id) throw std::runtime_error("field/call");
          std::string nm = A().s;
          if (M(TokKind::LParen)){
            int argc=0; if (P().k!=TokKind::RParen){ do{ parseExpr(); ++argc; } while(M(TokKind::Comma)); } W(TokKind::RParen,")");
            E(Op::CALL_METHOD, N(nm), argc);
          } else {
            E(Op::GET_FIELD, N(nm));
          }
          continue;
        }
        if (M(TokKind::LBracket)){
          if (P().k!=TokKind::Num) throw std::runtime_error("index");
          int idx=(int)A().n; W(TokKind::RBracket,"]");
          E(Op::GET_FIELD, N(std::to_string(idx))); // treat index as dotted field segment
          continue;
        }
        break;
      }
      return;
    }
    throw std::runtime_error("expr");
  }
};

static Chunk parse_to_chunk(const std::string& src){
  Lexer lx(src); auto toks = lx.run();
  Parser p(std::move(toks)); return p.parse();
}

// --- Additional: Utility to pretty-print a Chunk for debugging ---

#include <iostream>

namespace triad {

void dump_chunk(const Chunk& ch) {
  std::cout << "Chunk dump:\n";
  for (size_t i = 0; i < ch.code.size(); ++i) {
    const Instr& instr = ch.code[i];
    std::cout << "  [" << i << "] Op: " << static_cast<int>(instr.op)
              << " a: " << instr.a << " b: " << instr.b << " c: " << instr.c << "\n";
  }
  std::cout << "Constants:\n";
  for (size_t i = 0; i < ch.consts.size(); ++i) {
    const Value& v = ch.consts[i];
    if (v.tag == Value::Num)
      std::cout << "  [" << i << "] Num: " << v.num << "\n";
    else if (v.tag == Value::Str)
      std::cout << "  [" << i << "] Str: \"" << v.str << "\"\n";
  }
  std::cout << "Names:\n";
  for (size_t i = 0; i < ch.names.size(); ++i) {
    std::cout << "  [" << i << "] " << ch.names[i] << "\n";
  }
}

} // namespace triad

// --- Additional: Utility to disassemble a single instruction for debugging ---

namespace triad {

std::string disasm_instr(const Instr& instr, const Chunk& ch) {
  std::string out = "Op: " + std::to_string(static_cast<int>(instr.op));
  out += " a: " + std::to_string(instr.a);
  out += " b: " + std::to_string(instr.b);
  out += " c: " + std::to_string(instr.c);

  // Optionally, show constant or name if relevant
  if (instr.op == Op::PUSH_CONST && instr.a >= 0 && instr.a < static_cast<int>(ch.consts.size())) {
    const Value& v = ch.consts[instr.a];
    if (v.tag == Value::Num)
      out += " ; const(num): " + std::to_string(v.num);
    else if (v.tag == Value::Str)
      out += " ; const(str): \"" + v.str + "\"";
  }
  if ((instr.op == Op::PUSH_VAR || instr.op == Op::SET_VAR || instr.op == Op::GET_FIELD || instr.op == Op::CALL_METHOD || instr.op == Op::NEW_CLASS)
      && instr.a >= 0 && instr.a < static_cast<int>(ch.names.size())) {
    out += " ; name: " + ch.names[instr.a];
  }
  return out;
}

} // namespace triad

#ifdef TRIAD_PARSER_TEST_MAIN
#include <string>

int main() {
  using namespace triad;
  std::string src = "say 1+2*3;";
  Chunk ch = parse_to_chunk(src);
  dump_chunk(ch);
  return 0;
}
#endif

#ifdef TRIAD_PARSER_DISASM_MAIN
#include <string>

int main() {
  using namespace triad;
  std::string src = "x = 42; say x;";
  Chunk ch = parse_to_chunk(src);
  std::cout << "Disassembly:\n";
  for (size_t i = 0; i < ch.code.size(); ++i) {
    std::cout << "[" << i << "] " << disasm_instr(ch.code[i], ch) << "\n";
  }
  return 0;
}
#endif
// --- Additional: Utility to serialize and deserialize Chunk to/from a simple text format ---
#include <sstream>
#include <fstream>
#include <string>
#include <stdexcept>
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
#ifdef TRIAD_SERIALIZE_MAIN
    int main(int argc, char** argv) {
        if (argc < 2) {
            std::cerr << "Usage: triad_serialize <input.triad> [output.chunk]\n";
            return 1;
        }
        std::string inputFile = argv[1];
        std::string outputFile = argc > 2 ? argv[2] : "";
        std::ifstream ifs(inputFile);
        if (!ifs) {
            std::cerr << "Error opening input file: " << inputFile << "\n";
            return 1;
        }
        Chunk ch = parse_to_chunk(ifs);
        ifs.close();
        if (!outputFile.empty()) {
            std::ofstream ofs(outputFile);
            if (!ofs) {
                std::cerr << "Error opening output file: " << outputFile << "\n";
                return 1;
            }
            serialize_chunk(ch, ofs);
            ofs.close();
            std::cout << "Serialized chunk to " << outputFile << "\n";
        } else {
            serialize_chunk(ch, std::cout);
        }
        return 0;
	}
#endif
} // namespace triad
// --- Additional: Utility to disassemble a Chunk from a .triad file ---
#ifdef TRIAD_DISASM_MAIN
#include <string>
#include <fstream>
int main(int argc, char** argv) {
  using namespace triad;
  if (argc < 2) {
    std::cout << "Usage: triad_disasm <file.triad>\n";
    return 1;
  }
  std::string inputFile = argv[1];
  std::ifstream ifs(inputFile);
  if (!ifs) {
    std::cerr << "Error opening file: " << inputFile << "\n";
    return 1;
  }
  Chunk ch = parse_to_chunk(ifs);
  ifs.close();
  std::cout << "Disassembly of " << inputFile << ":\n";
  for (size_t i = 0; i < ch.code.size(); ++i) {
    std::cout << "[" << i << "] " << disasm_instr(ch.code[i], ch) << "\n";
  }
  return 0;
}
#endif
// --- Main program to parse command line and run modes ---
#ifdef TRIAD_PARSER_MAIN
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
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
  std::string mode = argv[1];
  std::string input = argc > 2 ? argv[2] : "";
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
      // run_tests(verbose); // Implement test runner as needed
      std::cout << "[Test runner not yet implemented]\n";
      return 0;
    }
    if (input.empty()) {
      std::cerr << "Error: No input file specified.\n";
      return 1;
    }
	std::string src = slurp(input);
    Chunk ch = parse_to_chunk(src);
    if (verbose) std::cout << "[Parsed chunk with " << ch.code.size() << " instructions]\n";
    if (showBytecode) dump_chunk(ch);
    if (showAst) {
      std::cout << "[AST dump not yet implemented]\n";
    }
    // Handle other modes: run-vm, run-ast, emit-nasm, emit-llvm
    if (mode == "run-vm") {
      std::cout << "[VM execution not yet implemented]\n";
    } else if (mode == "run-ast") {
      std::cout << "[AST interpreter not yet implemented]\n";
    } else if (mode == "emit-nasm") {
      std::cout << "[NASM emission not yet implemented]\n";
    } else if (mode == "emit-llvm") {
      std::cout << "[LLVM IR emission not yet implemented]\n";
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
#endif
//       std::cout << "[AST dump not yet implemented]\n";
      return;
    }
    // Handle other modes: run-vm, run-ast, emit-nasm, emit-llvm
    if (mode == "run-vm") {
      std::cout << "[VM execution not yet implemented]\n";
    } else if (mode == "run-ast") {
      std::cout << "[AST interpreter not yet implemented]\n";
    } else if (mode == "emit-nasm") {
      std::cout << "[NASM emission not yet implemented]\n";
    } else if (mode == "emit-llvm") {
      std::cout << "[LLVM IR emission not yet implemented]\n";
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

