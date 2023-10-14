/**
 * @file Available Expression Dataflow Analysis
 */


#include "dfa/Framework.h"


namespace {

class Expression
{
private:
    unsigned _opcode; const Value * _lhs, * _rhs;

public:
    Expression(const Instruction & inst)
    {
        _opcode = inst.getOpcode();
        _lhs = inst.getOperand(0);
        _rhs = inst.getOperand(1);
    }

    bool operator==(const Expression & Expr) const
    {
        //* 判断两个表达式是否相同
		//* 注意 1+2 和 2+1 是一样的
        switch(_opcode)
        {
            // 满足交换率
            case Instruction::Add:
            case Instruction::FAdd: //* 浮点数加法
            case Instruction::Mul:
            case Instruction::FMul:
                return ((Expr.getLHSOperand() == _lhs  && Expr.getRHSOperand() == _rhs) 
                || (Expr.getLHSOperand() == _rhs && Expr.getRHSOperand() == _lhs)
                    && Expr.getOpcode() == _opcode);
            default: 
                return Expr.getLHSOperand() == _lhs && Expr.getRHSOperand() == _rhs
                    && Expr.getOpcode() == _opcode;
        }
    }

    unsigned getOpcode() const { return _opcode; }
    const Value * getLHSOperand() const { return _lhs; }
    const Value * getRHSOperand() const { return _rhs; }

    friend raw_ostream & operator<<(raw_ostream & outs, const Expression & expr);

};

raw_ostream & operator<<(raw_ostream & outs, const Expression & expr)
{
    outs << "[" << Instruction::getOpcodeName(expr._opcode) << " ";
    expr._lhs->printAsOperand(outs, false); outs << ", ";
    expr._rhs->printAsOperand(outs, false); outs << "]";

    return outs;
}

}



namespace std
{
template <>
struct hash < Expression >
{
    std::size_t operator()(const Expression & expr) const
    {
        std::hash < unsigned > unsigned_hasher; std::hash < const Value * > pvalue_hasher;

        std::size_t opcode_hash = unsigned_hasher(expr.getOpcode()); //* 用无符号整数计算 hash
        std::size_t lhs_operand_hash = pvalue_hasher((expr.getLHSOperand())); //* 计算常量值的指针的 hash 值
        std::size_t rhs_operand_hash = pvalue_hasher((expr.getRHSOperand()));

        return opcode_hash ^ (lhs_operand_hash << 1) ^ (rhs_operand_hash << 1);
    }
};


}


namespace {
class AvailExpr final : public dfa::Framework < Expression, 
                        dfa::Direction::Forward >
{
	protected:
    virtual BitVector IC() const override
    {
        // @DONE OUT[B] = U （全集）
        return BitVector(_domain.size(), true);
    }
    virtual BitVector BC() const override
    {
        // @DONE OUT[ENTRY] = \Phi（空集）
        return BitVector(_domain.size(), false);
    }


	//* 计算交集
    virtual BitVector MeetOp(const BasicBlock & bb) const override
    {
        BitVector result(_domain.size(), true);
        for(const BasicBlock* block : predecessors(&bb))
        {
            // 所有前驱基础块的最后一条Instruction的OUT集合，就是整个基础块的IN集
			//* 根据可用表达式的算法, 前驱基础块的最后一条指令的 out 集合就是整个基础块的 in 集合
            const Instruction&  last_inst_in_block = block->back();
            BitVector curr_bv = _inst_bv_map.at(&last_inst_in_block);
            result &= curr_bv;
        }
        return result;
    }


    //* 计算单个指令的 out 集合
    virtual bool TransferFunc(const Instruction & inst,
                  const BitVector & ibv,
                  BitVector & obv) override
    {
		BitVector new_obv = ibv;

		//* kill 所有被引用的表达式
        //* 因为 out 要剪掉 kill 的集合，所以设置为 false
        //* 循环遍历 _domain 中的每个可用表达式（Expression 对象），并检查它们的左操作数和右操作数是否与当前指令 inst 相关，它会使一些先前可用的表达式变得不再可用
        for(auto elem : _domain)
            if(elem.getLHSOperand() == &inst || elem.getRHSOperand() == &inst)
                new_obv[getDomainIndex(elem)] = false;

        //* 将 gen 加入
        if(isa<BinaryOperator>(inst) && _domain.find(inst) != _domain.end())
            new_obv[getDomainIndex(Expression(inst))] = true;
        bool hasChanged = new_obv != obv;
        obv = new_obv;
        return hasChanged;

	}




    virtual void InitializeDomainFromInstruction(const Instruction & inst) override
    {
		//* 检查是不是二元操作，也就是有两个操作数的，比如 add sub 这种
        if (isa < BinaryOperator > (inst))
        {
            //* 相同的 expr 将不会被重复添加
            _domain.emplace(inst);
        }
    }








public:
    static char ID;

    AvailExpr() : dfa::Framework < domain_element_t, 
                       direction_c > (ID) {}
    virtual ~AvailExpr() override {}

};

char AvailExpr::ID = 1; 
RegisterPass < AvailExpr > Y ("avail_expr", "Available Expression");



}