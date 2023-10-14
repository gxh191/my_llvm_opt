#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {

class AlgebraicIdentity final : public FunctionPass {
public:
  static char ID;

  AlgebraicIdentity() : FunctionPass(ID) {}

  /**
   * @todo(cscd70) Please complete the methods below.
   */
  virtual bool runOnFunction(Function &F) override
  {
	bool Modified = false;
	for (BasicBlock &BB : F)
	{
		for (Instruction &I : BB)
		{
			//* 打印指令
			outs() << "Instruction:\n";
			I.print(outs());
			outs() << "\n";
			
			if(BinaryOperator *BO = dyn_cast<BinaryOperator>(&I))
			{
				//* check 乘以 1
				if (BO->getOpcode() == Instruction::Mul)
				{
					if (ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1)))
					{
						if (CI->isOne())
						{
							BO->replaceAllUsesWith(BO->getOperand(0));
							BO->eraseFromParent();
							Modified = true;
						}
					}
				}
				//* check 加 0
				else if (BO->getOpcode() == Instruction::Add)
				{
					if (ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1)))
					{
						if (CI->isZero())
						{
							BO->replaceAllUsesWith(BO->getOperand(0));
							BO->eraseFromParent();
							Modified = true;
						}
					}
				}

				return Modified;	
			}
		}
	}
  }

  
	//* 保持函数的控制流图 不变
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
	AU.setPreservesCFG();
  }

}; // class AlgebraicIdentity

char AlgebraicIdentity::ID = 0;
RegisterPass<AlgebraicIdentity> X("algebraic-identity",
                                  "CSCD70: Algebraic Identity");

} // anonymous namespace
