g++ -I/usr/lib/llvm-11/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -fPIC   -c -o FunctionInfo.o FunctionInfo.cpp
g++ -dylib -fPIC -shared FunctionInfo.o -o FunctionInfo.so
clang-11 -O2 -emit-llvm -c ./test/Loop.c -o ./test/Loop.bc
llvm-dis-11 ./test/Loop.bc -o=./test/Loop.ll
# opt -load ./FunctionInfo.so -function-info ./test/Loop.bc -o ./test/Loop-opt.bc
opt-11 -load ./FunctionInfo.so -function-info ./test/Loop.bc -o test/Loop-opt.bc | /usr/lib/llvm-11/bin/FileCheck --match-full-lines ./test/Loop.c
