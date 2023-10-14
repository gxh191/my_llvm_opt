#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {

class StrengthReduction final : public FunctionPass {
public:
  static char ID;

  StrengthReduction() : FunctionPass(ID) {}

  /**
   * @todo(cscd70) Please complete the methods below.
   */

  virtual bool runOnFunction(Function &F) override { 
	bool Modified = false;

	std::list<Instruction*> deleteInst;
	for (BasicBlock &BB : F)
	{
		for (Instruction &Inst : BB)
		{
			//* 打印指令
			outs() << "Instruction:\n";
			Inst.print(outs());
			outs() << "\n";
			if(BinaryOperator *BO = dyn_cast<BinaryOperator>(&Inst))
			{
				//* 打印 bo
				// outs() << "BO:\n";
				// BO->print(outs());
				// outs() << "\n";

				//* 判断乘法 除法
				if(BO->getOpcode() == Instruction::Mul || BO->getOpcode() == Instruction::SDiv )
				{
					
					outs() << "jinlailejinlaile\n";
					//* 应该把两个操作数都取出来，可能是 x*2 也可能是 2*x
					//* 应该先判断类型，是常量还是变量
					//* 如果是常量，判断是否是2的幂次方
					ConstantInt *CI = 0;
					ConstantInt *CI_0 = dyn_cast<ConstantInt>(BO->getOperand(0));
					ConstantInt *CI_1 = dyn_cast<ConstantInt>(BO->getOperand(1));
					int operand_idx = 0;
					if(CI_0)
					{
						CI = CI_0;
						operand_idx = 1;
					}

					if(CI_1)
					{
						CI = CI_1;
						operand_idx = 0;
					}				
					// outs() << "CI: ";
					// outs() << CI->getValue() << "\n";

					// ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1));



					//* 输出  CI 的值 getValue
					// outs() << "CI: \n";
					// outs() << CI->getValue() << "\n";

					if(CI && CI->getValue().isPowerOf2())
					{
						// outs() << "jinlailejinlaile\n";
						if(BO->getOpcode() == Instruction::Mul)
						{
							// outs() << "shabishabishabishaisdjfoainfioqwenfiowenfoiqwe\n";
							IRBuilder<> builder(&Inst);
							// outs() << int(log2(CI->getSExtValue())) << "\n";
							Value* NewVal = builder.CreateShl(BO->getOperand(operand_idx), int(log2(CI->getSExtValue())), "strength_reduced");
							// Value* val = builder.CreateShl(BO->getOperand(operand_idx), log2(static_cast<double>()), "strength_reduced");

							// outs() << "baopcuo\n";
							// Value *NewVal = BinaryOperator::CreateShl(BO->getOperand(0), CI, "strength_reduced", &Inst);
							// BO->replaceAllUsesWith(NewVal);
							// outs() << "baopcuo\n";
							BO->eraseFromParent();
							// deleteInst.push_back(&Inst);

							Modified = true;
							// outs() << "baopcuo3\n";
						}
						else if(BO->getOpcode() == Instruction::SDiv)
						{
							Value *NewVal = BinaryOperator::CreateAShr(BO->getOperand(0), CI, "strength_reduced", &Inst);
							BO->replaceAllUsesWith(NewVal);
							BO->eraseFromParent();
							Modified = true;
						}
					}
			
				}
			}

		}
	}
	// Instruction必须在遍历BasicBlock之后再删除，否则可能会导致iterator指向错误的地方
	// for(Instruction* inst : deleteInst)
	// 	if(inst->isSafeToRemove())
	// 		// 注意，用erase而不是remove
	// 		inst->eraseFromParent();

	return Modified;
   }


  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

}; // class StrengthReduction

char StrengthReduction::ID = 0;
RegisterPass<StrengthReduction> X("strength-reduction",
                                  "CSCD70: Strength Reduction");

} // anonymous namespace
