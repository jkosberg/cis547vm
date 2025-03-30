#include "Extractor.h"

#include "llvm/IR/Instruction.h"

void Extractor::addDef(const InstMapTy &InstMap, Value *X, Instruction *L)
{
  if (InstMap.find(X) == InstMap.end())
    return;
  DefFile << toString(X) << "\t" << toString(L) << "\n";
}

void Extractor::addUse(const InstMapTy &InstMap, Value *X, Instruction *L)
{
  if (Constant *C = dyn_cast<Constant>(X))
    return;
  if (InstMap.find(X) == InstMap.end())
    return;
  UseFile << toString(X) << "\t" << toString(L) << "\n";
}

void Extractor::addDiv(const InstMapTy &InstMap, Value *X, Instruction *L)
{
  if (Constant *C = dyn_cast<Constant>(X))
    return;
  if (InstMap.find(X) == InstMap.end())
    return;
  DivFile << toString(X) << "\t" << toString(L) << "\n";
}

void Extractor::addTaint(const InstMapTy &InstMap, Instruction *L)
{
  TaintFile << toString(L) << "\n";
}

void Extractor::addSanitizer(const InstMapTy &InstMap, Instruction *L)
{
  SanitizerFile << toString(L) << "\n";
}

void Extractor::addNext(const InstMapTy &InstMap, Instruction *X,
                        Instruction *Y)
{
  NextFile << toString(X) << "\t" << toString(Y) << "\n";
};

/**
 * @brief Collects Datalog facts for each instruction to corresponding facts
 * file.
 */
void Extractor::extractConstraints(const InstMapTy &InstMap, Instruction *I)
{
  /**
   * TODO: For each predecessor P of instruction I,
   *       add a new fact in the `next` relation.
   */

  /**
   * TODO:
   *
   *   For each of the instruction add appropriate facts:
   *     Add `def` and `use` relations.
   *   For `BinaryOperator` instructions involving division:
   *     Add a fact for the `div` relation.
   *   For `CallInst` instructions:
   *     Add a `def` relation only if it returns a non-void value.
   *     If its a call to tainted input,
   *       Add appropriate fact to `taint` relation.
   *     If its a call to sanitize,
   *       Add appropriate fact to `sanitizer` relation.
   *
   * NOTE: Many Values may be used in a single instruction,
   *       but at most one Value can be defined in one instruction.
   * NOTE: You can use `isTaintedInput()` and `isSanitizer()` function
   *       to check if a particular CallInst is a tainted input
   *       or sanitize respectively.
   */

  // For each predecessor P of instruction I, add a new fact in the `next` relation.
  for (Instruction *Pred : getPredecessors(I))
  {
    addNext(InstMap, Pred, I);
  }

  // Handle different instruction types
  if (AllocaInst *AI = dyn_cast<AllocaInst>(I))
  {
    // Do nothing, alloca is just a declaration.
  }
  else if (StoreInst *SI = dyn_cast<StoreInst>(I))
  {
    Value *StoredValue = SI->getValueOperand();
    Value *Pointer = SI->getPointerOperand();

    addDef(InstMap, Pointer, I);     // The pointer being written to is "defined"
    addUse(InstMap, StoredValue, I); // The value being stored is "used"
  }
  else if (LoadInst *LI = dyn_cast<LoadInst>(I))
  {
    Value *Pointer = LI->getPointerOperand();

    addUse(InstMap, Pointer, I); // The pointer being read from is "used"
    addDef(InstMap, LI, I);      // The result of the load is "defined"
  }
  else if (BinaryOperator *BI = dyn_cast<BinaryOperator>(I))
  {
    Value *Op1 = BI->getOperand(0);
    Value *Op2 = BI->getOperand(1);

    addUse(InstMap, Op1, I); // Operand 1 is "used"
    addUse(InstMap, Op2, I); // Operand 2 is "used"
    addDef(InstMap, BI, I);  // The result of the operation is "defined"

    // If the operation is a division, add a fact to the `div` relation
    if ((BI->getOpcode() == Instruction::SDiv || BI->getOpcode() == Instruction::UDiv) &&
        Op2->getType()->isIntegerTy())
    {
      addDiv(InstMap, Op2, I); // The second operand is the divisor
    }
  }
  else if (CallInst *CI = dyn_cast<CallInst>(I))
  {
    // Extract facts from CallInst
    Function *CalledFunc = CI->getCalledFunction();
    if (!CalledFunc)
      return; // Skip indirect calls

    // Handle special functions: tainted_input and sanitizer
    if (isTaintedInput(CI))
    {
      addTaint(InstMap, I); // Add a fact to the `taint` relation
    }
    else if (isSanitizer(CI))
    {
      addSanitizer(InstMap, I); // Add a fact to the `sanitizer` relation
    }
    addDef(InstMap, CI, I); // Add a `def` relation for the return value
    for (unsigned int i = 0; i < CI->getNumArgOperands(); i++)
    {
      addUse(InstMap, CI->getArgOperand(i), I); // Add `use` relations for arguments
    }
  }
  else if (CastInst *CI = dyn_cast<CastInst>(I))
  {
    Value *Operand = CI->getOperand(0);

    addUse(InstMap, Operand, I); // The operand is "used"
    addDef(InstMap, CI, I);      // The result of the cast is "defined"
  }
  else if (CmpInst *CI = dyn_cast<CmpInst>(I))
  {
    Value *Op1 = CI->getOperand(0);
    Value *Op2 = CI->getOperand(1);

    addUse(InstMap, Op1, I); // Operand 1 is "used"
    addUse(InstMap, Op2, I); // Operand 2 is "used"
    addDef(InstMap, CI, I);  // The result of the comparison is "defined"
  }
}