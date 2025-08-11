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
