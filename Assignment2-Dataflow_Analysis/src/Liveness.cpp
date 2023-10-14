/**
 * @file Liveness Dataflow Analysis
 */
#include "dfa/Framework.h"



namespace
{

class Variable
{
private:
    const Value * _val;
public:
    Variable(const Value * val) : _val(val) {}

    bool operator==(const Variable & val) const
    { return _val == val.getValue(); }

    const Value* getValue() const { return _val; }

    friend raw_ostream & operator<<(raw_ostream & outs, const Variable & val);
};

raw_ostream & operator<<(raw_ostream & outs, const Variable & var)
{
    outs << "[";  var._val->printAsOperand(outs, false);  outs << "]";
    return outs;
}


}

//* unordered_set 的 hash 函数
namespace std
{
  template <>
  struct hash <Variable> 
  {
    std::size_t operator()(const Variable &var) const 
    {
      std::hash <const Value *> value_ptr_hasher;

      //* 计算 hash 值
      std::size_t value_hash = value_ptr_hasher((var.getValue()));

      return value_hash;
    }
  };
};


namespace {

/// @todo Implement @c Liveness using the @c dfa::Framework interface.
class Liveness final : public dfa::Framework < Variable, 
                        dfa::Direction::Backward >
{
protected:
    virtual BitVector IC() const override
    {
        // @DONE OUT[B] = \Phi（空集）
        return BitVector(_domain.size(), false);
    }
    virtual BitVector BC() const override
    {
        // @DONE OUT[ENTRY] = \Phi（空集）
        return BitVector(_domain.size(), false);
    }

    //* 数据流中的 meet 操作实现
    //* 算的是 out 集，对所有后继基础块的 in 集合进行并运算
    virtual BitVector MeetOp(const BasicBlock & bb) const override
    {
		BitVector result(_domain.size(), false);

		for(const BasicBlock* block : successors(&bb))
		{
			//* 所有后继基础块的第一条 inst 的 in 集合就是当前基础块的 out 集合
			const Instruction& first_inst_in_block = block->front();
			BitVector curr_bv = _inst_bv_map.at(&first_inst_in_block);

			//* 对 phi 的特殊处理
			for(auto phi_iter = block->phis().begin();
				phi_iter != block->phis().end();
				phi_iter++)
			{
				const PHINode & phi_inst = *phi_iter;
                for(auto phi_inst_iter = phi_inst.block_begin(); 
                    phi_inst_iter != phi_inst.block_end(); phi_inst_iter++)
                {
					//* 获取 phi 指令中的各个前驱基础块
					BasicBlock* const &curr_bb = *phi_inst_iter;
					
					//* 如果当前前驱基础块不是现在的基础块，因为当前基本块的值已经在当前基本块内部处理过了，所以考虑不一样的
					if(curr_bb != &bb)
					{
						const Value* curr_val = phi_inst.getIncomingValueForBlock(curr_bb);
						//* 如果当前值存在于 domain 中
						int idx = getDomainIndex(Variable(curr_val));
						if(idx != -1)
						{
							assert(curr_bv[idx] = true);
							curr_bv[idx] = false;
						}
					}
                }
			}
			result |= curr_bv;

		}
		return result;
    }


	virtual bool TransferFunc(const Instruction & inst,
							  const BitVector & ibv,
							  BitVector & obv) override

	{
		//* ibv 是 out
		//* 该函数用于计算单个指令的输入（IN）集合
		BitVector new_obv = ibv;

		//* use 操作
		for(auto iter = inst.op_begin(); iter != inst.op_end(); iter++)
		{
			const Value* val = dyn_cast<Value>(*iter);
			assert(val != NULL);
			//* 如果 val 存在于 domain 中
			if(_domain.find(val) != _domain.end())
				new_obv[getDomainIndex(Variable(val))] = true;
		}

		//* def 操作
		//* defB：在基本块B中 定值，但是定值前在B中没有被 引用 的变量的集合。
		if(getDomainIndex(Variable(&inst)) != -1)
			new_obv[getDomainIndex(Variable(&inst))] = false;

        bool hasChanged = new_obv != obv;
        obv = new_obv;
        return hasChanged;
	}

    virtual void InitializeDomainFromInstruction(const Instruction & inst) override
	{
		//* 遍历给定指令的操作数，如果是指令或者参数，就加入到 domain 中
		for(auto iter = inst.op_begin(); iter != inst.op_end(); iter++)
		if (isa < Instruction > (*iter) || isa < Argument > (*iter))
			_domain.emplace(Variable(*iter));
	}

public:
    static char ID;

    Liveness() : dfa::Framework < domain_element_t, 
                       direction_c > (ID) {}
    virtual ~Liveness() override {}
};

char Liveness::ID = 1; 
RegisterPass < Liveness > Y ("liveness", "Liveness");

}