#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {

class MultiInstOpt final : public FunctionPass {
public:
  static char ID;

  MultiInstOpt() : FunctionPass(ID) {}

  /**
   * @todo(cscd70) Please complete the methods below.
   */

    virtual bool runOnFunction(Function &F) override 
	{
		bool modified = false;

		std::list<Instruction*> deleteInst;
		BinaryOperator * debug = nullptr;
		bool shouldBreak = false;

		for (BasicBlock &BB : F)
		{

			for (Instruction &Inst : BB)
			{
				// if (shouldBreak) 
				// {
					// break; // 跳出整个循环
				// }
				//* 打印指令
				outs() << "Instruction:\n";
				Inst.print(outs());
				outs() << "\n";
				if (BinaryOperator *AddInst = dyn_cast<BinaryOperator>(&Inst)) 
				{
					if (AddInst->getOpcode() == Instruction::Add) 
					{
						for (auto &User : AddInst->uses())
						{
							outs() << "debug1\n";
							if (BinaryOperator *SubInst = dyn_cast<BinaryOperator>(User.getUser()))
							{
								if (SubInst->getOpcode() == Instruction::Sub)
								{
									outs() << "debug2\n";
									//* 𝑎 =𝑏 +𝑡,𝑐 =𝑎 −𝑡  → 𝑎 =𝑏 +𝑡,𝑐 =𝑏 (𝑡 isnot a constant) 
									if (AddInst->getOperand(1) == SubInst->getOperand(1))
									{
										outs() << "debug3\n";

										// outs() << "SubInst: ";
										// SubInst->print(outs());
										// outs() << "\n";

										// //* 打印 Inst
										// outs() << "Original Instruction:\n";
										// SubInst->print(outs());
										// outs() << "\n";

										SubInst->replaceAllUsesWith(AddInst->getOperand(0)); // 替换 c = a - t 中的 c
										debug = SubInst;
										// SubInst->setOperand(0, AddInst->getOperand(0));

										// SubInst->eraseFromParent(); 


										//* 打印 replaceAllUsesWith 之后的 Inst
										// outs() << "Replaced Instruction:\n";
										// SubInst->print(outs());
										// outs() << "\n";
										//* 应该删除的是 sub 指令 SubInst
										deleteInst.push_back(SubInst);
										// deleteInst.push_back(&Inst);
										modified = true;
										// shouldBreak = true; // 设置标志以跳出整个循环

										break;
									}
								}
							}
						}
					}
				}
			}
		}

		outs() << "start delete\n";
		for(Instruction* inst : deleteInst)
			if(inst->isSafeToRemove())
				inst->eraseFromParent();
			
	// 	//* 打印 replaceAllUsesWith 之后的 Inst
	// outs() << "Replaced Instruction:\n";
	// debug->print(outs());
	// outs() << "\n";

		return modified;
	}


  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }
}; // class MultiInstOpt

char MultiInstOpt::ID = 0;
RegisterPass<MultiInstOpt> X("multi-inst-opt",
                             "CSCD70: Multi-Instruction Optimization");

} // anonymous namespace
