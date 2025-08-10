
#include "triad_parser.cpp"
#include "triad_vm.cpp"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace triad;

static std::string slurp(const std::string& path){
  std::ifstream f(path); std::ostringstream o; o<<f.rdbuf(); return o.str();
}

int main(int argc, char** argv){
  if (argc<2){ std::cout<<"usage: triadc <file.triad>\n"; return 0; }
  std::string src = slurp(argv[1]);
  Chunk ch = parse_to_chunk(src);
  VM vm; vm.exec(ch);
  return 0;
}
