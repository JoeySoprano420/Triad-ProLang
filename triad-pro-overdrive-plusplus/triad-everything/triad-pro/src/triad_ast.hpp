
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace triad {

struct Expr; struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

enum class BinOp { Add, Sub, Mul, Div, Mod, Eq, Ne, Lt, Le, Gt, Ge, And, Or };
enum class UnOp  { Neg, Not };

struct Expr {
  virtual ~Expr() = default;
};
struct ENum : Expr { double v; ENum(double d):v(d){} };
struct EStr : Expr { std::string v; EStr(std::string s):v(std::move(s)){} };
struct EId  : Expr { std::string name; EId(std::string n):name(std::move(n)){} };
struct EBin : Expr { BinOp op; ExprPtr a,b; EBin(BinOp o,ExprPtr A,ExprPtr B):op(o),a(A),b(B){} };
struct EUn  : Expr { UnOp op; ExprPtr x; EUn(UnOp o,ExprPtr X):op(o),x(X){} };
struct ECall: Expr { ExprPtr recv; std::string name; std::vector<ExprPtr> args; ECall(ExprPtr r,std::string n,std::vector<ExprPtr> a):recv(r),name(std::move(n)),args(std::move(a)){} };
struct EField: Expr { ExprPtr base; std::string path; EField(ExprPtr b,std::string p):base(b),path(std::move(p)){} };
struct ETuple: Expr { std::vector<std::string> names; std::vector<ExprPtr> vals; };
struct ENew : Expr { std::string cls; std::vector<ExprPtr> args; };

struct Stmt { virtual ~Stmt() = default; };
struct SExpr : Stmt { ExprPtr e; SExpr(ExprPtr X):e(X){} };
struct SAssign: Stmt { std::string name; ExprPtr e; SAssign(std::string n,ExprPtr X):name(std::move(n)),e(X){} };
struct SBlock : Stmt { std::vector<StmtPtr> body; };
struct SIf    : Stmt { ExprPtr cond; std::shared_ptr<SBlock> thenB, elseB; };
struct SFor   : Stmt { std::string iv; ExprPtr a,b; std::shared_ptr<SBlock> body; };
struct STry   : Stmt {
  std::shared_ptr<SBlock> body;
  struct Catch { std::string id; std::vector<std::string> types; std::shared_ptr<SBlock> body; };
  std::vector<Catch> catches;
  std::shared_ptr<SBlock> fin;
};

} // namespace triad
