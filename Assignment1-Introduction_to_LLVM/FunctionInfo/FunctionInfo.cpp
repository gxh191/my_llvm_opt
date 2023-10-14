#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class FunctionInfo final : public ModulePass {
public:
  static char ID;

  FunctionInfo() : ModulePass(ID) {}

  // We don't modify the program, so we preserve all analysis.
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

  virtual bool runOnModule(Module &M) override {
    outs() << "123123CSCD70 Function Information Pass"
           << "\n";

    /**
     * @todo(cscd70) Please complete this method.
     */
    outs() << "Name    " << "# Args    " << "# Calls    " << "# Blocks    " << "# Insts    " << "\n";

	//* check 的格式
	// CHECK-LABEL: Function Name: g_incr
	// CHECK-NEXT: Number of Arguments: 1
	// CHECK-NEXT: Number of Calls: 0
	// CHECK-NEXT: Number OF BBs: 1
	// CHECK-NEXT: Number of Instructions: 4

    //* 遍历模块中所有函数
    for (Function &F : M)
    {
		//* 忽略外部声明函数
		if (F.isDeclaration())
		{
			continue;
		}
		//* 获取函数名称
		outs() << "Function Name: " << F.getName() << "\n";

		//* 获取参数个数
		outs() << "Number of Arguments: " << F.arg_size() << "\n";

		//* 直接调用此函数的次数
		int NumDirectCallSites = 0;
		for(User* U : F.users())
		{
			if (Instruction* I = dyn_cast<Instruction>(U))
			{
				NumDirectCallSites++;
			}
		}
		outs() << "Number of Calls: " << NumDirectCallSites << "\n";

		//* 获取基本块个数
		int NumBasicBlocks = 0;
		for (BasicBlock &BB : F)
		{
			NumBasicBlocks++;
		}
		outs() << "Number OF BBs: " << NumBasicBlocks << "\n";

		//* 统计指令数
		int NumInstructions = 0;
		for (BasicBlock &BB : F)
		{
			for (Instruction &I : BB)
			{
				NumInstructions++;
			}
		}
		outs() << "Number of Instructions: " << NumInstructions << "\n";






    }


    return false;
  }
}; // class FunctionInfo

char FunctionInfo::ID = 0;
RegisterPass<FunctionInfo> X("function-info", "CSCD70: Function Information");
} // anonymous namespace
