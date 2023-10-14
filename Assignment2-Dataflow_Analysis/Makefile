OPTIMIZER := dfa.so

LLVM_VERSION ?= 10 # 很奇怪 用 11 就会找不到符号 pred_const_range
CLANG_VERSION ?= 10

CXXFLAGS := $(shell llvm-config-$(LLVM_VERSION) --cxxflags) -I./include -fPIC

OPT_OBJs := $(subst .cpp,.o,$(wildcard ./src/*.cpp))

TEST_SRCs := $(basename $(notdir $(wildcard ./test/*.c)))
TEST_RAW_LLs := $(subst .c,.ll,$(wildcard ./test/*.c))
TEST_OPT_LLs := $(addprefix ./test/,$(addsuffix -opt.ll,$(TEST_SRCs)))

all: $(TEST_OPT_LLs) $(TEST_RAW_LLs)

./test/%-opt.ll: ./test/%-opt.bc
	llvm-dis-$(LLVM_VERSION) $< -o=$@
	
./test/%.ll: ./test/%-m2r.bc
	llvm-dis-$(LLVM_VERSION) $< -o=$@

./test/%-opt.bc: ./test/%-m2r.bc $(OPTIMIZER)
	env LD_LIBRARY_PATH=. opt-$(LLVM_VERSION) -load $(OPTIMIZER) -$* $< -o $@

./test/%-m2r.bc: ./test/%.bc
	opt-$(LLVM_VERSION) -mem2reg $< -o $@

./test/%.bc: ./test/%.c
	clang-$(CLANG_VERSION) -O0 -Xclang -disable-O0-optnone -emit-llvm -c $< -o $@

$(OPTIMIZER): $(OPT_OBJs)
	$(CXX) -dylib -fPIC -shared $^ -o $@

$(OPT_OBJs): ./include/dfa/Framework.h

.PHONY: clean
clean:
	$(RM) $(TEST_OPT_LLs) $(TEST_RAW_LLs) $(OPTIMIZER) $(OPT_OBJs)
