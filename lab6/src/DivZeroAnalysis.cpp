#include "DivZeroAnalysis.h"
#include "Utils.h"

namespace dataflow
{

  //===----------------------------------------------------------------------===//
  // DivZero Analysis Implementation
  //===----------------------------------------------------------------------===//

  /**
   * PART 1
   * 1. Implement "check" that checks if a given instruction is erroneous or not.
   * 2. Implement "transfer" that computes the semantics of each instruction.
   *    This means that you have to complete "eval" function, too.
   *
   * PART 2
   * 1. Implement "doAnalysis" that stores your results in "InMap" and "OutMap".
   * 2. Implement "flowIn" that joins the memory set of all incoming flows.
   * 3. Implement "flowOut" that flows the memory set to all outgoing flows.
   * 4. Implement "join" to union two Memory objects, accounting for Domain value.
   * 5. Implement "equal" to compare two Memory objects.
   */

  bool DivZeroAnalysis::check(Instruction *Inst)
  {
    // Check if the instruction is a division instruction
    if (auto BinOp = dyn_cast<BinaryOperator>(Inst))
    {
      if (BinOp->getOpcode() == Instruction::UDiv || BinOp->getOpcode() == Instruction::SDiv)
      {
        // Get the divisor (second operand)
        auto Divisor = BinOp->getOperand(1);

        // Get the domain of the divisor from the memory
        auto DivisorDomain = getOrExtract(OutMap[Inst], Divisor);

        // Check if the divisor is Zero or MaybeZero
        if (DivisorDomain->Value == Domain::Zero || DivisorDomain->Value == Domain::MaybeZero)
        {
          return true;
        }
      }
    }

    // If the instruction is not a division or the divisor is not Zero/MaybeZero, return false
    return false;
  }

  bool DivZeroAnalysis::runOnFunction(Function &F)
  {
    outs() << "Running " << getAnalysisName() << " on " << F.getName() << "\n";

    // Initializing InMap and OutMap.
    for (inst_iterator Iter = inst_begin(F), End = inst_end(F); Iter != End; ++Iter)
    {
      auto Inst = &(*Iter);
      InMap[Inst] = new Memory;
      OutMap[Inst] = new Memory;
    }

    // The chaotic iteration algorithm is implemented inside doAnalysis().
    doAnalysis(F);

    // Check each instruction in function F for potential divide-by-zero error.
    for (inst_iterator Iter = inst_begin(F), End = inst_end(F); Iter != End; ++Iter)
    {
      auto Inst = &(*Iter);
      if (check(Inst))
        ErrorInsts.insert(Inst);
    }

    printMap(F, InMap, OutMap);
    outs() << "Potential Instructions by " << getAnalysisName() << ": \n";
    for (auto Inst : ErrorInsts)
    {
      outs() << *Inst << "\n";
    }

    for (auto Iter = inst_begin(F), End = inst_end(F); Iter != End; ++Iter)
    {
      delete InMap[&(*Iter)];
      delete OutMap[&(*Iter)];
    }
    return false;
  }

  char DivZeroAnalysis::ID = 1;
  static RegisterPass<DivZeroAnalysis> X("DivZero", "Divide-by-zero Analysis",
                                         false, false);
} // namespace dataflow
