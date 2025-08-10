
#pragma once
#include <vector>
#include <string>

namespace triad {

enum class Op : uint8_t {
  NOP,
  PUSH_CONST, PUSH_VAR, SET_VAR,
  ADD, SUB, MUL, DIV, MOD,
  EQ, NE, LT, LE, GT, GE,
  NOT, NEG,
  GET_FIELD, CALL_METHOD, NEW_CLASS, MAKE_TUPLE,
  IF_FALSE_JMP, JMP,
  SAY, ECHO, RET,
  // short-circuit
  SC_AND_BEGIN, SC_AND_EVAL, SC_AND_END,
  SC_OR_BEGIN,  SC_OR_EVAL,  SC_OR_END
};

struct Instr { Op op; int a=0,b=0,c=0; };
struct Value { enum {Num,Str} tag=Num; double num=0; std::string str; static Value number(double d){ Value v; v.tag=Num; v.num=d; return v; } static Value string(std::string s){ Value v; v.tag=Str; v.str=std::move(s); return v; } };

struct Chunk {
  std::vector<Instr> code;
  std::vector<Value> consts;
  std::vector<std::string> names;
  int addConst(Value v){ consts.push_back(std::move(v)); return (int)consts.size()-1; }
  int addName(const std::string& n){ names.push_back(n); return (int)names.size()-1; }
  int emit(Op op,int a=0,int b=0,int c=0){ code.push_back({op,a,b,c}); return (int)code.size()-1; }
};

} // namespace triad
