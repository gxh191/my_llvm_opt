/**
 * @file Loop Invariant Code Motion
 */
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#define FOUND(list, elem) \
    std::find(list.begin(), list.end(), elem) == list.end()

using namespace llvm;

namespace {

class LoopInvariantCodeMotion final : public LoopPass
{
private:
	DominatorTree * dom_tree;
	std::list<Instruction *> markedAsInvariant;

	//* 检查该指令是否是循环不变量
	bool isInvariant(Instruction * I, Loop* loop)
	{
		bool is_invariant = true;
		for(auto iter = I->op_begin(); is_invariant && iter != I->op_end(); ++iter)
		{
			Value* val = dyn_cast<Value>(*iter);
			assert(val != NULL);

			//* 既不是常量 又不是函数参数
			if(!isa<Constant> (val) && !isa<Argument>(val))
			{
				if(Instruction* inst = dyn_cast<Instruction>(val))
				{                    
					//* 既不是循环不变量指令
					//* 又不是循环外的指令
					//* 那么该操作数就不是循环不变量
                    if(FOUND(markedAsInvariant, inst) && loop->contains(inst->getParent()))
                        is_invariant = false;
				}
				//* 还可能遍历到 label
				else
				{
					is_invariant = false;
				}
			}
		}
		return isSafeToSpeculativelyExecute(I) //* 检查异常
		&& !I->mayReadFromMemory() //* 检查是否是读取内存的指令
		&& !isa< LandingPadInst >(I) //* 检查是否是 landingpad 指令
		&& is_invariant;
	}

	//* 检查 s 所在的基本块是否是循环所有出口的支配节点
    bool isDomExitblocks(Instruction* inst, Loop* loop)
    {
        SmallVector<BasicBlock*, 0> exitblock;

		//* 获取循环中的所有出口基本块 放到 exitblock 中
        loop->getExitBlocks(exitblock);

		//* 遍历每个出口基本块
        for(BasicBlock* bb : exitblock)
			//* 如果 inst 所在的基本块没有支配 bb，则返回 false
            if(!dom_tree->dominates(inst->getParent(), bb))
                return false;
        return true;
    }

	//* 检查 loop 中是否没有其它语句对 x 进行赋值
    bool AssignOnce(Instruction* inst)
    {
        //* 由于SSA的特殊性，每个变量只能被赋值一次
        return true;
    }

    //* 检查 loop 中对 x 的引用是否仅由 s 到达
    bool OneWayToReferences(Instruction* inst)
    {
        //* 由于SSA的特殊性，每个变量只能由一条语句引用
        return true;
    }

	//* 将指令移动到前置首节点的末尾
    bool moveToPreheader(Instruction* inst, Loop* loop)
    {
        BasicBlock* preheader = loop->getLoopPreheader();
        assert(preheader != NULL);

		/* moveBefore 函数将指令 inst 移动到前置首节点的终结指令（terminator）之前。
		这是一个重要的细节，因为循环前置首节点的最后一条指令通常是终结指令，
		而将 inst 移动到终结指令之后可能导致错误，因为它们不会被执行。终结指令通常是跳转指令，
		用于控制程序的流程。 */
        inst->moveBefore(preheader->getTerminator());
        return false;
    }

public:
	static char ID;

	LoopInvariantCodeMotion() : LoopPass(ID) {}
	virtual ~LoopInvariantCodeMotion() override {}

    virtual void getAnalysisUsage(AnalysisUsage & AU) const
	{
		AU.addRequired < DominatorTreeWrapperPass > ();
        AU.addRequired < LoopInfoWrapperPass > ();
		AU.setPreservesCFG();
	}

    virtual bool runOnLoop(Loop * L, LPPassManager & LPM)
	{
		outs() << "\nLoop Invariant Code Motion\n";

		//* 前置首节点不存在
        if(!L->getLoopPreheader())
            return false;

		//* 获取支配树
		dom_tree = &(getAnalysis < DominatorTreeWrapperPass > ().getDomTree());
        markedAsInvariant.clear();
        bool IR_has_Changed = false;
        bool hasChanged;

		do{
            hasChanged = false;
			//* 遍历基础块
			for(BasicBlock* bb : L->blocks())
			{
				//* 当前循环的信息，不包括嵌套的
                LoopInfo* loopinfo = &(getAnalysis < LoopInfoWrapperPass > ().getLoopInfo());

				//* 只处理当前循环，不处理嵌套
                if(loopinfo->getLoopFor(bb) == L){
                    for(Instruction& inst : *bb){
						//* 如果该指令是循环不变量，而且没有被标记过
                        if(FOUND(markedAsInvariant, &inst)
                            && isInvariant(&inst, L)){
                            hasChanged = true;
                            markedAsInvariant.push_back(&inst);
                        }
                    } 
                }
			}
		}while(hasChanged);


        int moveCount = 0;
        for(Instruction* inst : markedAsInvariant)
		{
            /*
                移动代码的三个条件：
                    1. s所在的基本块是循环所有出口结点(有后继结点在循环外的结点)的支配结点
                    2. 循环中没有其它语句对x赋值
                    3. 循环中对语句s:x=y+z中，x的引用仅由s到达
            */

            if(isDomExitblocks(inst, L) && AssignOnce(inst) && OneWayToReferences (inst))
            {
                // 将指令移动到循环的前置首节点
                moveToPreheader(inst, L);
                outs() << "移出循环的指令 " << moveCount << " : " << *inst << "\n";
                IR_has_Changed = true;
                moveCount++;
            }
		}



        outs() << "循环不变量数量：\t\t" << markedAsInvariant.size() << "\n";
        outs() << "已移出循环的不变量数量：\t" <<  moveCount << "\n";
        outs() << "循环不变量代码移动：\t\t" << (IR_has_Changed ? "成功" : "失败") << "\n";
        return IR_has_Changed;
	}

};

char LoopInvariantCodeMotion::ID = 0;

RegisterPass < LoopInvariantCodeMotion > X (
	"loop-invariant-code-motion",
	"Loop Invariant Code Motion");

} // anonymous namespace
