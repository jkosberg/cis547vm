#include "DivZeroAnalysis.h"
#include "Utils.h"

namespace dataflow
{

  /**
   * @brief Is the given instruction a user input?
   *
   * @param Inst The instruction to check.
   * @return true If it is a user input, false otherwise.
   */
  bool isInput(Instruction *Inst)
  {
    if (auto Call = dyn_cast<CallInst>(Inst))
    {
      if (auto Fun = Call->getCalledFunction())
      {
        return (Fun->getName().equals("getchar") ||
                Fun->getName().equals("fgetc"));
      }
    }
    return false;
  }

  /**
   * Evaluate a PHINode to get its Domain.
   *
   * @param Phi PHINode to evaluate
   * @param InMem InMemory of Phi
   * @return Domain of Phi
   */
  Domain *eval(PHINode *Phi, const Memory *InMem)
  {
    if (auto ConstantVal = Phi->hasConstantValue())
    {
      return new Domain(extractFromValue(ConstantVal));
    }

    Domain *Joined = new Domain(Domain::Uninit);

    for (unsigned int i = 0; i < Phi->getNumIncomingValues(); i++)
    {
      auto Dom = getOrExtract(InMem, Phi->getIncomingValue(i));
      Joined = Domain::join(Joined, Dom);
    }
    return Joined;
  }

  /**
   * @brief Evaluate the +, -, * and / BinaryOperator instructions
   * using the Domain of its operands and return the Domain of the result.
   *
   * @param BinOp BinaryOperator to evaluate
   * @param InMem InMemory of BinOp
   * @return Domain of BinOp
   */
  Domain *eval(BinaryOperator *BinOp, const Memory *InMem)
  {
    auto LHS = getOrExtract(InMem, BinOp->getOperand(0));
    auto RHS = getOrExtract(InMem, BinOp->getOperand(1));

    switch (BinOp->getOpcode())
    {
    case Instruction::Add:
    case Instruction::Sub:
      // For addition and subtraction, if both operands are zero, the result is zero
      if (LHS->isZero() && RHS->isZero())
      {
        return new Domain(Domain::Zero);
      }
      else if (LHS->isZero() || RHS->isZero())
      {
        // If one operand is zero, the result can be non-zero
        return new Domain(Domain::NonZero);
      }
      // Otherwise, the result can be any value
      return new Domain(Domain::MaybeZero);

    case Instruction::Mul:
      // For multiplication, if either operand is zero, the result is zero
      if (LHS->isZero() || RHS->isZero())
      {
        return new Domain(Domain::Zero);
      }
      // Otherwise, the result can be any value
      return new Domain(Domain::MaybeZero);

    case Instruction::UDiv:
    case Instruction::SDiv:
      // For division, if RHS is zero, the result is undefined
      if (RHS->isZero())
      {
        return new Domain(Domain::Uninit);
      }
      else if (LHS->isZero())
      {
        // If LHS is zero, the result is zero
        return new Domain(Domain::Zero);
      }
      else if (RHS->Value == Domain::NonZero)
      {
        // If we got here, RHS is non-zero and LHS is non-zero, so the result can be 0 or non-zero depending on rounding
        return new Domain(Domain::MaybeZero);
      }

    default:
      // For other binary operations, return an uninitialized domain
      return new Domain(Domain::Uninit);
    }
  }

  /**
   * @brief Evaluate Cast instructions.
   *
   * @param Cast Cast instruction to evaluate
   * @param InMem InMemory of Instruction
   * @return Domain of Cast
   */
  Domain *eval(CastInst *Cast, const Memory *InMem)
  {
    // Get the operand of the cast instruction
    auto Operand = Cast->getOperand(0);

    // Get the domain of the operand from the memory
    auto OperandDomain = getOrExtract(InMem, Operand);

    // The domain of the cast instruction is the same as the domain of its operand
    return new Domain(OperandDomain->Value);
  }

  /**
   * @brief Evaluate the ==, !=, <, <=, >=, and > Comparision operators using
   * the Domain of its operands to compute the Domain of the result.
   *
   * @param Cmp Comparision instruction to evaluate
   * @param InMem InMemory of Cmp
   * @return Domain of Cmp
   */
  Domain *eval(CmpInst *Cmp, const Memory *InMem)
  {
    /**
     * TODO: Write your code here that evaluates:
     * ==, !=, <, <=, >=, and > based on the Domains of the operands.
     *
     * NOTE: There is a lot of scope for refining this, but you can just return
     * MaybeZero for comparisons other than equality.
     */
    // Get the operands of the comparison instruction
    auto LHS = getOrExtract(InMem, Cmp->getOperand(0));
    auto RHS = getOrExtract(InMem, Cmp->getOperand(1));

    switch (Cmp->getPredicate())
    {
    case CmpInst::ICMP_EQ:
      // If both operands are zero, the result is true (a non-zero value of 1)
      if (LHS->isZero() && RHS->isZero())
      {
        return new Domain(Domain::NonZero);
      }
      // If one operand is zero and the other is not, the result is false (a 0 value)
      if ((LHS->isZero() && RHS->Value == Domain::NonZero) ||
          (LHS->Value == Domain::NonZero && RHS->isZero()))
      {
        return new Domain(Domain::Zero);
      }
      // Otherwise, the result can be zero or non-zero
      return new Domain(Domain::MaybeZero);

    case CmpInst::ICMP_NE:
      // If both operands are zero, then we have 0 != 0, which is false (a 0 value)
      if (LHS->isZero() && RHS->isZero())
      {
        return new Domain(Domain::Zero);
      }
      // If one operand is zero and the other is not, then we have 0 != non-zero, which is true (a non-zero value)
      if ((LHS->isZero() && RHS->Value == Domain::NonZero) ||
          (LHS->Value == Domain::NonZero && RHS->isZero()))
      {
        return new Domain(Domain::NonZero);
      }
      // Otherwise, the result can be zero or non-zero
      return new Domain(Domain::MaybeZero);

      // case CmpInst::ICMP_ULT:
      // case CmpInst::ICMP_SLT:
      // case CmpInst::ICMP_ULE:
      // case CmpInst::ICMP_SLE:
      // case CmpInst::ICMP_UGT:
      // case CmpInst::ICMP_SGT:
      // case CmpInst::ICMP_UGE:
      // case CmpInst::ICMP_SGE:
      //   // For other comparisons, the result can be zero or non-zero
      //   return new Domain(Domain::MaybeZero);

    default:
      // For other predicates, return an uninitialized domain
      return new Domain(Domain::MaybeZero);
    }
  }

  void DivZeroAnalysis::transfer(Instruction *Inst, const Memory *In,
                                 Memory &NOut, PointerAnalysis *PA,
                                 SetVector<Value *> PointerSet)
  {
    if (isInput(Inst))
    {
      // The instruction is a user controlled input, it can have any value.
      NOut[variable(Inst)] = new Domain(Domain::MaybeZero);
    }
    else if (auto Phi = dyn_cast<PHINode>(Inst))
    {
      // Evaluate PHI node
      NOut[variable(Phi)] = eval(Phi, In);
    }
    else if (auto BinOp = dyn_cast<BinaryOperator>(Inst))
    {
      // Evaluate BinaryOperator
      NOut[variable(BinOp)] = eval(BinOp, In);
    }
    else if (auto Cast = dyn_cast<CastInst>(Inst))
    {
      // Evaluate Cast instruction
      NOut[variable(Cast)] = eval(Cast, In);
    }
    else if (auto Cmp = dyn_cast<CmpInst>(Inst))
    {
      // Evaluate Comparision instruction
      NOut[variable(Cmp)] = eval(Cmp, In);
    }
    else if (auto Alloca = dyn_cast<AllocaInst>(Inst))
    {
      // Do nothing here.
    }
    else if (auto Store = dyn_cast<StoreInst>(Inst))
    {
      /**
       * TODO: Store instruction can either add new variables or overwrite existing variables into memory maps.
       * To update the memory map, we rely on the points-to graph constructed in PointerAnalysis.
       *
       * To build the abstract memory map, you need to ensure all pointer references are in-sync, and
       * will converge upon a precise abstract value. To achieve this, implement the following workflow:
       *
       * Iterate through the provided PointerSet:
       *   - If there is a may-alias (i.e., `alias()` returns true) between two variables:
       *     + Get the abstract values of each variable.
       *     + Join the abstract values using Domain::join().
       *     + Update the memory map for the current assignment with the joined abstract value.
       *     + Update the memory map for all may-alias assignments with the joined abstract value.
       *
       * Hint: You may find getOperand(), getValueOperand(), and getPointerOperand() useful.
       */

      // Value operand is the value being stored
      Value *ValueOperand = Store->getValueOperand();
      auto ValueOperandType = ValueOperand->getType();
      // Pointer operand is the location where the value is stored
      Value *PointerOperand = Store->getPointerOperand();
      auto PointerOperandType = PointerOperand->getType();

      // Get the domain of the value operand, aka domain of the value being stored
      Domain *ValueDomain = getOrExtract(In, ValueOperand);
      Domain *PointerDomain = getOrExtract(In, PointerOperand);

      std::string PointerOperandStr = variable(PointerOperand);
      std::string ValueOperandStr = variable(ValueOperand);
      // 4 cases to consider:
      // 1: Value operand and pointer operand are both pointers
      // 2: Value operand is a pointer, pointer operand is not
      // 3: Value operand is not a pointer, pointer operand is
      // 4: Value operand and pointer operand are not pointers

      // If the value operand is a pointer, we need to check the domain of what it points to
      if (ValueOperandType->isPointerTy())
      {
        if (PointerOperandType->isPointerTy())
        {
          for (auto Ptr : PointerSet)
          {
            auto PtrString = variable(Ptr);
            if (PA->alias(ValueOperandStr, PtrString))
            {
              // Get the abstract value of the pointer operand
              auto AliasDomain = getOrExtract(In, Ptr);

              // Update the memory map for the alias with the domain of the value operand
              NOut[PtrString] = Domain::join(ValueDomain, AliasDomain);
            }
          }
          NOut[PointerOperandStr] = ValueDomain;
        }
        else if (PointerOperandType->isIntegerTy())
        {
          // If the pointer operand is an integer type, it simply represents
          // the value of the variable so the abstract domain is the same as the
          // value operand
          // Example instruction: store i32 %2, i32* %1, align 4
          NOut[PointerOperandStr] = ValueDomain;
        }
        else
        {
          // errs() << "Pointer Operand: " << PointerOperandStr << "\n";
          // errs() << "Value Operand: " << ValueOperandStr << "\n";
        }
      }
      else
      {
        // If we get here, value being stored is not a pointer, but memory location storing it could be
        // First check if the pointer operand has any aliases
        if (PointerOperandType->isPointerTy())
        {
          if (ValueOperandType->isIntegerTy())
          {
            // If the value operand is an integer type, join it with the previous domain
            // Example instruction: store i32 0, i32* %1, align 4
            for (auto Ptr : PointerSet)
            {
              auto PtrString = variable(Ptr);
              if (PA->alias(PointerOperandStr, PtrString))
              {
                // Get the abstract value of the pointer operand
                auto AliasDomain = getOrExtract(In, Ptr);

                // Update the memory map for the alias with the domain of the value operand
                NOut[PtrString] = Domain::join(ValueDomain, AliasDomain);
              }
            }
            NOut[PointerOperandStr] = ValueDomain;
          }
          else
          {
            for (auto Ptr : PointerSet)
            {
              auto PtrString = variable(Ptr);
              if (PA->alias(PointerOperandStr, PtrString))
              {
                // errs() << "Pointer Operand: " << PointerOperandStr << "\n";
                // errs() << "Value Operand: " << ValueOperandStr << "\n";
                // Get the abstract value of the pointer operand
                auto AliasDomain = getOrExtract(In, Ptr);

                // Update the memory map for the alias with the domain of the value operand
                NOut[PtrString] = ValueDomain;
              }
            }
          }
        }
      }
    }
    else if (auto Load = dyn_cast<LoadInst>(Inst))
    {
      /**
       * TODO: Rely on the existing variables defined within the `In` memory to
       * know what abstract domain should be for the new variable
       * introduced by a load instruction.
       *
       * If the memory map already contains the variable, propagate the existing
       * abstract value to NOut.
       * Otherwise, initialize the memory map for it.
       *
       * Hint: You may use getPointerOperand().
       */
      // Get the pointer operand of the load instruction
      Value *PointerOperand = Load->getPointerOperand();
      auto PointerOperandType = PointerOperand->getType();
      if (PointerOperandType->isPointerTy())
      {
        // either *i32 or **i32
        auto PointerType = PointerOperandType->getPointerElementType();
        if (PointerType->isIntegerTy())
        {
          // If the pointer operand is an integer type, it simply represents
          // the value of the variable so the abstract domain is the same as the
          // value operand
          // Example instruction: %2 = load i32, i32* %1, align 4
          NOut[variable(Load)] = getOrExtract(In, PointerOperand);
        }
        else if (PointerType->isPointerTy())
        {
          // If the pointer operand is a pointer type, then we need to find out what it points
          // to and use that domain
          // Example instruction: %2 = load i32*, i32** %1, align 4
          for (auto Ptr : PointerSet)
          {
            auto PtrString = variable(PointerOperand);
            auto PointerOperandString = variable(PointerOperand);

            // Check if the PointerOperand is an alias of Ptr
            if (PA->alias(PointerOperandString, PtrString))
            {
              // Get the abstract value of the pointer operand
              // Update the memory map for the alias with the domain of the value operand
              NOut[variable(Load)] = getOrExtract(In, Ptr);
            }
          }
        }
      }
      else
      {
        // Check if the memory map already contains the variable
        auto it = In->find(variable(PointerOperand));
        if (it != In->end())
        {
          // Propagate the existing abstract value to NOut
          NOut[variable(Load)] = it->second;
        }
        else
        {
          // Initialize the memory map for the new variable
          NOut[variable(Load)] = new Domain(Domain::Uninit);
        }
      }
    }
    else if (auto Branch = dyn_cast<BranchInst>(Inst))
    {
      // Analysis is flow-insensitive, so do nothing here.
    }
    else if (auto Call = dyn_cast<CallInst>(Inst))
    {
      /**
       * TODO: Populate the NOut with an appropriate abstract domain.
       *
       * You only need to consider calls with int return type.
       */
      // Check if the call has an integer return type
      if (Call->getType()->isIntegerTy())
      {
        // Populate the NOut with an appropriate abstract domain
        // For simplicity, we assume the result can be any value
        NOut[variable(Call)] = new Domain(Domain::MaybeZero);
      }
    }
    else if (auto Return = dyn_cast<ReturnInst>(Inst))
    {
      // Analysis is intra-procedural, so do nothing here.
    }
    else
    {
      errs() << "Unhandled instruction: " << *Inst << "\n";
    }
  }

} // namespace dataflow