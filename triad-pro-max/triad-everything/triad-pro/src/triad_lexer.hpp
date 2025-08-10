
#pragma once
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

namespace triad {

enum class TokKind {
  Eof, Id, Num, Str,
  LParen, RParen, LBrace, RBrace, LBracket, RBracket,
  Comma, Dot, Colon, Semicolon, Range,
  Plus, Minus, Star, Slash, Percent,
  Eq, EqEq, Ne, Lt, Le, Gt, Ge,
  Bang,
  KwMacro, KwEnd, KwStruct, KwClass, KwEnum,
  KwPure, KwDef, KwTry, KwCatch, KwFinally, KwThrow,
  KwIf, KwElse, KwFor, KwIn, KwLoop, KwNew, KwAnd, KwOr, KwSay, KwEcho, KwReturn
};

struct Token {
  TokKind k; std::string s; double n=0; int line=1; int col=1;
};

struct Lexer {
  std::string src; size_t i=0; int line=1; int col=1;
  Lexer(std::string s): src(std::move(s)) {}
  char peek(int o=0) const { return (i+o<src.size())? src[i+o]: '\0'; }
  char get(){ char c=peek(); if (c=='\n'){ ++line; col=1; } else { ++col; } if (i<src.size()) ++i; return c; }
  static bool id0(char c){ return std::isalpha((unsigned char)c) || c=='_'; }
  static bool idc(char c){ return std::isalnum((unsigned char)c) || c=='_'; }

  void skipSpace(){
    for(;;){
      char c=peek();
      if (std::isspace((unsigned char)c)){ get(); continue; }
      if (c=='/' && peek(1)=='/'){ while(peek() && peek()!='\n') get(); continue; }
      if (c=='/' && peek(1)=='*'){ get(); get(); while(peek()&&!(peek()=='*'&&peek(1)=='/')) get(); if (peek()){ get(); get(); } continue; }
      break;
    }
  }

  Token tok(TokKind k, const std::string& s="", double n=0){ return Token{k,s,n,line,col}; }

  Token number(){
    size_t j=i; while(std::isdigit((unsigned char)peek())) get();
    if (peek()=='.' && std::isdigit((unsigned char)peek(1))) get(), get();
    while(std::isdigit((unsigned char)peek())) get();
    double val = std::stod(src.substr(j, i-j));
    return tok(TokKind::Num, "", val);
  }

  Token string(){
    get(); std::string s;
    while(peek() && peek()!='"'){ s.push_back(get()); }
    if (peek()=='"') get();
    return tok(TokKind::Str, s);
  }

  TokKind kw(const std::string& id){
    std::string k; k.reserve(id.size());
    for(char c:id) k.push_back((char)std::tolower((unsigned char)c));
    if (k=="macro") return TokKind::KwMacro;
    if (k=="end") return TokKind::KwEnd;
    if (k=="struct") return TokKind::KwStruct;
    if (k=="class") return TokKind::KwClass;
    if (k=="enum") return TokKind::KwEnum;
    if (k=="pure") return TokKind::KwPure;
    if (k=="def") return TokKind::KwDef;
    if (k=="try") return TokKind::KwTry;
    if (k=="catch") return TokKind::KwCatch;
    if (k=="finally") return TokKind::KwFinally;
    if (k=="throw") return TokKind::KwThrow;
    if (k=="if") return TokKind::KwIf;
    if (k=="else") return TokKind::KwElse;
    if (k=="for") return TokKind::KwFor;
    if (k=="in") return TokKind::KwIn;
    if (k=="loop") return TokKind::KwLoop;
    if (k=="new") return TokKind::KwNew;
    if (k=="and") return TokKind::KwAnd;
    if (k=="or") return TokKind::KwOr;
    if (k=="say") return TokKind::KwSay;
    if (k=="echo") return TokKind::KwEcho;
    if (k=="return") return TokKind::KwReturn;
    return TokKind::Id;
  }

  std::vector<Token> run(){
    std::vector<Token> v;
    for(;;){
      skipSpace();
      char c=peek();
      if (!c){ v.push_back(tok(TokKind::Eof)); break; }
      if (c=='('){ get(); v.push_back(tok(TokKind::LParen,"(")); continue; }
      if (c==')'){ get(); v.push_back(tok(TokKind::RParen,")")); continue; }
      if (c=='{'){ get(); v.push_back(tok(TokKind::LBrace,"{")); continue; }
      if (c=='}'){ get(); v.push_back(tok(TokKind::RBrace,"}")); continue; }
      if (c=='['){ get(); v.push_back(tok(TokKind::LBracket,"[")); continue; }
      if (c==']'){ get(); v.push_back(tok(TokKind::RBracket,"]")); continue; }
      if (c==','){ get(); v.push_back(tok(TokKind::Comma,",")); continue; }
      if (c=='.' && peek(1)=='.'){ get(); get(); v.push_back(tok(TokKind::Range,"..")); continue; }
      if (c=='.'){ get(); v.push_back(tok(TokKind::Dot,".")); continue; }
      if (c==':'){ get(); v.push_back(tok(TokKind::Colon,":")); continue; }
      if (c==';'){ get(); v.push_back(tok(TokKind::Semicolon,";")); continue; }
      if (c=='+'){ get(); v.push_back(tok(TokKind::Plus,"+")); continue; }
      if (c=='-'){ get(); v.push_back(tok(TokKind::Minus,"-")); continue; }
      if (c=='*'){ get(); v.push_back(tok(TokKind::Star,"*")); continue; }
      if (c=='/'){ get(); v.push_back(tok(TokKind::Slash,"/")); continue; }
      if (c=='%'){ get(); v.push_back(tok(TokKind::Percent,"%")); continue; }
      if (c=='!'){ if (peek(1)=='='){ get(); get(); v.push_back(tok(TokKind::Ne,"!=")); } else { get(); v.push_back(tok(TokKind::Bang,"!")); } continue; }
      if (c=='='){ if (peek(1)=='='){ get(); get(); v.push_back(tok(TokKind::EqEq,"==")); } else { get(); v.push_back(tok(TokKind::Eq,"=")); } continue; }
      if (c=='<'){ if (peek(1)=='='){ get(); get(); v.push_back(tok(TokKind::Le,"<=")); } else { get(); v.push_back(tok(TokKind::Lt,"<")); } continue; }
      if (c=='>'){ if (peek(1)=='='){ get(); get(); v.push_back(tok(TokKind::Ge,">=")); } else { get(); v.push_back(tok(TokKind::Gt,">")); } continue; }
      if (c=='"') { v.push_back(string()); continue; }
      if (std::isdigit((unsigned char)c)){ v.push_back(number()); continue; }
      if (id0(c)){
        size_t j=i; while(idc(peek())) get();
        std::string id = src.substr(j, i-j);
        TokKind k = kw(id);
        if (k==TokKind::Id) v.push_back(tok(k, id));
        else v.push_back(tok(k, id));
        continue;
      }
      throw std::runtime_error("lex error");
    }
    return v;
  }
};

} // namespace triad
