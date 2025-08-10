
#include "triad_bytecode.hpp"
#include <unordered_map>
#include <iostream>
#include <cmath>

namespace triad {

struct VM {
  std::unordered_map<std::string, double> nums;
  std::unordered_map<std::string, std::string> strs;

  bool truthyStr(const std::string& s){ return !s.empty(); }
  bool truthyNum(double d){ return d!=0.0 && !std::isnan(d); }

  void exec(Chunk& ch){
    std::vector<double> nstack; std::vector<std::string> sstack;
    auto pushVal=[&](const Value& v){
      if (v.tag==Value::Num){ nstack.push_back(v.num); sstack.push_back(""); }
      else { sstack.push_back(v.str); nstack.push_back(NAN); }
    };
    auto popBool=[&](){
      auto ss=sstack.back(); sstack.pop_back();
      auto nn=nstack.back(); nstack.pop_back();
      if (!std::isnan(nn)) return truthyNum(nn);
      return truthyStr(ss);
    };
    size_t ip=0;
    while (ip<ch.code.size()){
      const Instr& I= ch.code[ip];
      switch(I.op){
        case Op::PUSH_CONST: pushVal(ch.consts[I.a]); ++ip; break;
        case Op::PUSH_VAR:   { const std::string& name=ch.names[I.a]; nstack.push_back(nums[name]); sstack.push_back(strs[name]); ++ip; break; }
        case Op::SET_VAR:    { const std::string& name=ch.names[I.a]; double v=nstack.back(); std::string s=sstack.back(); nstack.pop_back(); sstack.pop_back(); nums[name]=v; strs[name]=s; ++ip; break; }
        case Op::ADD: { double b=nstack.back(); nstack.pop_back(); sstack.pop_back(); double a=nstack.back(); nstack.pop_back(); sstack.pop_back(); nstack.push_back(a+b); sstack.push_back(""); ++ip; break; }
        case Op::SUB: { double b=nstack.back(); nstack.pop_back(); sstack.pop_back(); double a=nstack.back(); nstack.pop_back(); sstack.pop_back(); nstack.push_back(a-b); sstack.push_back(""); ++ip; break; }
        case Op::MUL: { double b=nstack.back(); nstack.pop_back(); sstack.pop_back(); double a=nstack.back(); nstack.pop_back(); sstack.pop_back(); nstack.push_back(a*b); sstack.push_back(""); ++ip; break; }
        case Op::DIV: { double b=nstack.back(); nstack.pop_back(); sstack.pop_back(); double a=nstack.back(); nstack.pop_back(); sstack.pop_back(); nstack.push_back(b==0?INFINITY:a/b); sstack.push_back(""); ++ip; break; }
        case Op::MOD: { double b=nstack.back(); nstack.pop_back(); sstack.pop_back(); double a=nstack.back(); nstack.pop_back(); sstack.pop_back(); nstack.push_back(std::fmod(a,b)); sstack.push_back(""); ++ip; break; }
        case Op::NEG: { double a=nstack.back(); nstack.pop_back(); sstack.pop_back(); nstack.push_back(-a); sstack.push_back(""); ++ip; break; }
        case Op::NOT: { bool b=popBool(); nstack.push_back(b?0:1); sstack.push_back(""); ++ip; break; }
        case Op::EQ: case Op::NE: case Op::LT: case Op::LE: case Op::GT: case Op::GE: {
          double b=nstack.back(); nstack.pop_back(); sstack.pop_back();
          double a=nstack.back(); nstack.pop_back(); sstack.pop_back();
          bool r=false;
          if (I.op==Op::EQ) r=(a==b);
          else if (I.op==Op::NE) r=(a!=b);
          else if (I.op==Op::LT) r=(a<b);
          else if (I.op==Op::LE) r=(a<=b);
          else if (I.op==Op::GT) r=(a>b);
          else if (I.op==Op::GE) r=(a>=b);
          nstack.push_back(r?1:0); sstack.push_back(""); ++ip; break;
        }
        case Op::GET_FIELD: { /* demo: treat as no-op path eval; push 0 */ nstack.push_back(0); sstack.push_back(""); ++ip; break; }
        case Op::CALL_METHOD:{ /* demo: pop argc args, keep 0 */ for(int k=0;k<I.b;++k){ nstack.pop_back(); sstack.pop_back(); } nstack.push_back(0); sstack.push_back(""); ++ip; break; }
        case Op::NEW_CLASS:   { nstack.push_back(0); sstack.push_back(""); ++ip; break; }
        case Op::MAKE_TUPLE:  { for(int k=0;k<I.a;++k){ nstack.pop_back(); sstack.pop_back(); } nstack.push_back(0); sstack.push_back(""); ++ip; break; }
        case Op::IF_FALSE_JMP:{ bool b=popBool(); ip = b? ip+1 : (size_t)I.a; break; }
        case Op::JMP: ip=(size_t)I.a; break;
        case Op::SAY: { auto ss=sstack.back(); auto nn=nstack.back(); nstack.pop_back(); sstack.pop_back();
                        if (!std::isnan(nn)) std::cout<<nn<<"\n"; else std::cout<<ss<<"\n"; ++ip; break; }
        case Op::ECHO:{ auto ss=sstack.back(); auto nn=nstack.back(); nstack.pop_back(); sstack.pop_back();
                        if (!std::isnan(nn)) std::cerr<<nn<<"\n"; else std::cerr<<ss<<"\n"; ++ip; break; }
        case Op::SC_AND_BEGIN: ++ip; break;
        case Op::SC_AND_EVAL:  { bool lhs=popBool(); if (!lhs) ip=(size_t)I.b; else ++ip; break; }
        case Op::SC_AND_END:   { bool rhs=popBool(); nstack.push_back(rhs?1:0); sstack.push_back(""); ++ip; break; }
        case Op::SC_OR_BEGIN:  ++ip; break;
        case Op::SC_OR_EVAL:   { bool lhs=popBool(); if (lhs) ip=(size_t)I.b; else ++ip; break; }
        case Op::SC_OR_END:    { bool rhs=popBool(); nstack.push_back(rhs?1:0); sstack.push_back(""); ++ip; break; }
        case Op::RET: return;
        default: ++ip; break;
      }
    }
  }
};

} // namespace triad
