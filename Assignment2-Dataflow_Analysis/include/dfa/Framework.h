#pragma once

#include <cassert>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "llvm/Pass.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace dfa {




/// Analysis Direction, used as Template Parameter
enum class Direction { Forward, Backward };

template < typename TDomainElement, Direction TDirection >
class Framework : public FunctionPass
{
    #define METHOD_ENABLE_IF_DIRECTION(dir, ret)                    \
    template < Direction _TDirection = TDirection >             \
    typename std::enable_if < _TDirection == dir, ret > ::type


protected:
    typedef TDomainElement domain_element_t;
    static constexpr Direction direction_c = TDirection;


    std::unordered_set < TDomainElement > _domain;

    std::unordered_map < const Instruction *, BitVector > _inst_bv_map;

    virtual BitVector IC() const = 0;

    virtual BitVector BC() const = 0;

private:
    void printDomainWithMask(const BitVector & mask) const
    {
        outs() << "{";

        assert(mask.size() == _domain.size() &&
               "The size of mask must be equal to the size of domain.");
        
        unsigned mask_idx = 0;
        for (const auto & elem : _domain)
        {
            if (!mask[mask_idx++])
            {
                continue;
            }
            outs() << elem << ", ";
        }  // for (mask_idx ∈ [0, mask.size()))
        outs() << "}";
    }

    void printInstBV(const Instruction & inst) const
    {
        const BasicBlock * const pbb = inst.getParent();
        if (&inst == &(*InstTraversalOrder(*pbb).begin()))
        {
            // 让编译器自己去推导类型，因为这个类型有两种可能
            auto meet_operands = MeetOperands(*pbb);
            
            // If the list of meet operands is empty, then we are at
            // the boundary, hence print the BC.
            if (meet_operands.begin() == meet_operands.end())
            {
                outs() << "BC:\t";
                printDomainWithMask(BC());
                outs() << "\n";
            }
            else
            {
                outs() << "MeetOp:\t";
                printDomainWithMask(MeetOp(*pbb));
                outs() << "\n";
            }
        }
        outs() << "Instruction: " << inst << "\n";
        outs() << "\t";
        printDomainWithMask(_inst_bv_map.at(&inst));
        outs() << "\n";
    }


protected:
    /// @brief Dump, ∀inst ∈ @p F, the associated bitvector.
    void printInstBVMap(const Function & F) const
    {
        outs() << "********************************************" << "\n";
        outs() << "* Instruction-BitVector Mapping         " << "\n";
        outs() << "********************************************" << "\n";

        for (const auto & inst : instructions(F))
        {
            printInstBV(inst);
        }
    }


    METHOD_ENABLE_IF_DIRECTION(Direction::Forward, pred_const_range)
    MeetOperands(const BasicBlock & bb) const
    {
        return predecessors(&bb);
    }



    METHOD_ENABLE_IF_DIRECTION(Direction::Backward, succ_const_range)
    MeetOperands(const BasicBlock & bb) const
    {
        return successors(&bb);
    }



    virtual BitVector MeetOp(const BasicBlock & bb) const = 0;




    virtual bool TransferFunc(const Instruction & inst,
                  const BitVector & ibv,
                  BitVector & obv) = 0;



    METHOD_ENABLE_IF_DIRECTION(Direction::Forward, 
        iterator_range < SymbolTableList<BasicBlock> ::const_iterator >)
    BBTraversalOrder(const Function & F) const
    {
        return make_range(F.begin(), F.end());
    }


    METHOD_ENABLE_IF_DIRECTION(Direction::Forward, 
    iterator_range < SymbolTableList<Instruction> ::const_iterator > )
    InstTraversalOrder(const BasicBlock & bb) const
    {
        return make_range(bb.begin(), bb.end());
    } 

    METHOD_ENABLE_IF_DIRECTION(Direction::Backward, 
        iterator_range < SymbolTableList<BasicBlock> ::const_reverse_iterator >)
    BBTraversalOrder(const Function & F) const
    {
        return make_range(F.getBasicBlockList().rbegin(), F.getBasicBlockList().rend());
    }
    /// @brief Return the traversal order of the instructions.
    METHOD_ENABLE_IF_DIRECTION(Direction::Backward, 
        iterator_range < SymbolTableList<Instruction> ::const_reverse_iterator > )
    InstTraversalOrder(const BasicBlock & bb) const
    {
        return make_range(bb.getInstList().rbegin(), bb.getInstList().rend());
    }



    int getDomainIndex(const TDomainElement& elem) const
    {
        //std::unordered_set < TDomainElement > _domain;
        auto inst_dom_iter = _domain.find(elem);
        if(inst_dom_iter == _domain.end())
            return -1;

        int idx = 0; 
        for(auto dom_iter = _domain.begin(); inst_dom_iter != dom_iter; ++dom_iter)
            ++idx;
        return idx;
    }

    //* 算活跃变量的算法是倒序的
    bool traverseCFG(const Function & func)
    {
        bool transform = false;
        
        for(const BasicBlock& basicBlock : BBTraversalOrder(func))
        {
            //* OUT[B] IN[B]
            BitVector ibv;
            if(&basicBlock == &(*BBTraversalOrder(func).begin()))


                //* 对于控制流图的入口基本块，通常会将其输入集合初始化为边界条件（BC）
                ibv = BC();
            else
                //* 计算前驱基本块的输出集合的合并
                ibv = MeetOp(basicBlock);
            
            //* 计算当前基础块的 OUT
            for(const Instruction& inst : InstTraversalOrder(basicBlock))
            {
                //* TransferFunc 计算单个指令的输入（IN）集合，存在 _inst_bv_map 中
                //* 前向数据流问题：OUT[B]=fB(IN[B])
                //* fB=fsn⋅…⋅fs2⋅fs1，遍历单条 inst 指令得到 out 集合
                transform |= TransferFunc(inst, ibv, _inst_bv_map[&inst]);
                //* 计算出的inst_out集合，是下一次transfer的in集合

                //* IN[si+1]=OUT[si]
                ibv = _inst_bv_map[&inst];
            }
        }
        return transform;
    }

public:
    Framework(char ID) : FunctionPass(ID) {}
    virtual ~Framework() override {}

    // We don't modify the program, so we preserve all analysis.
    virtual void getAnalysisUsage(AnalysisUsage & AU) const
    {
        AU.setPreservesAll();
    }
protected:
    /// @brief Initialize the domain from each instruction.
    /// 
    /// @todo  Override this method in every child class.
    virtual void InitializeDomainFromInstruction(const Instruction & inst) = 0;
public:
    virtual bool runOnFunction(Function & F) override final
    {
        for (const auto & inst : instructions(F))
        {
            InitializeDomainFromInstruction(inst);
        }
        for (const auto & inst : instructions(F))
        {
            _inst_bv_map.emplace(&inst, IC());
        }
        // keep traversing until changes have been made to the
        // instruction-bv mapping

        while (traverseCFG(F)) {}

        printInstBVMap(F);

        return false;
    }
#undef METHOD_ENABLE_IF_DIRECTION


};
}