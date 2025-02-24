#include "CBIInstrument.h"

using namespace llvm;

namespace instrument
{

  const auto PASS_NAME = "CBIInstrument";
  const auto PASS_DESC = "Instrumentation for CBI";
  const auto CBI_BRANCH_FUNCTION_NAME = "__cbi_branch__";
  const auto CBI_RETURN_FUNCTION_NAME = "__cbi_return__";

  /**
   * @brief Instrument a BranchInst with calls to __cbi_branch__
   *
   * @param M Module containing Branch
   * @param Branch A conditional Branch Instruction
   * @param Line Line number of Branch
   * @param Col Coulmn number of Branch
   */
  void instrumentBranch(Module *M, BranchInst *Branch, int Line, int Col);

  /**
   * @brief Instrument the return value of CallInst using calls to __cbi_return__
   *
   * @param M Module containing Call
   * @param Call A Call instruction that returns an Int32.
   * @param Line Line number of the Call
   * @param Col Column number of the Call
   */
  void instrumentReturn(Module *M, CallInst *Call, int Line, int Col);

  bool CBIInstrument::runOnFunction(Function &F)
  {
    auto FunctionName = F.getName().str();
    outs() << "Running " << PASS_DESC << " on function " << FunctionName << "\n";

    LLVMContext &Context = F.getContext();
    Module *M = F.getParent();

    Type *VoidType = Type::getVoidTy(Context);
    Type *Int32Type = Type::getInt32Ty(Context);
    Type *BoolType = Type::getInt1Ty(Context);

    M->getOrInsertFunction(CBI_BRANCH_FUNCTION_NAME, VoidType, Int32Type,
                           Int32Type, BoolType);

    M->getOrInsertFunction(CBI_RETURN_FUNCTION_NAME, VoidType, Int32Type,
                           Int32Type, Int32Type);

    for (inst_iterator Iter = inst_begin(F), E = inst_end(F); Iter != E; ++Iter)
    {
      Instruction &Inst = (*Iter);
      llvm::DebugLoc DebugLoc = Inst.getDebugLoc();
      if (!DebugLoc)
      {
        // Skip Instruction if it doesn't have debug information.
        continue;
      }

      int Line = DebugLoc.getLine();
      int Col = DebugLoc.getCol();

      /**
       * TODO: Add code to check the type of instruction
       * and call appropriate instrumentation function.
       *
       * If Inst is a Branch Instruction then
       *   use instrumentBranch
       * Else if it is a Call returning an int then
       *   use instrumentReturn
       * @param Branch
       */
      if (auto *Branch = dyn_cast<BranchInst>(&Inst))
      {
        if (Branch->isConditional())
        {
          outs() << "Instrumenting Branch Instruction at Line: " << Line
                 << ", Column: " << Col << "\n";
          // Call the instrumentBranch function
          instrumentBranch(M, Branch, Line, Col);
        }
      }
      else if (auto *Call = dyn_cast<CallInst>(&Inst))
      {
        // call instrumentReturn when the call instruction returns an int
        if (Call->getType()->isIntegerTy(32))
        {
          outs() << "Instrumenting Call Instruction at Line: " << Line
                 << ", Column: " << Col << "\n";
          instrumentReturn(M, Call, Line, Col);
        }
      }
    }
    return true;
  }

  /**
   * Implement instrumentation for the branch scheme of CBI.
   */
  void instrumentBranch(Module *M, BranchInst *Branch, int Line, int Col)
  {
    auto &Context = M->getContext();
    auto Int32Type = Type::getInt32Ty(Context);

    /**
     * TODO: Add code to instrument the Branch Instruction.
     */

    // Insert a call to __cbi_branch__ before the branch instruction
    auto *CBIBranchFunction = M->getFunction(CBI_BRANCH_FUNCTION_NAME);
    if (!CBIBranchFunction)
    {
      llvm::errs() << "Error: CBI branch function not found\n";
      return;
    }

    // Create the arguments for the function call
    Value *LineValue = ConstantInt::get(Int32Type, Line);
    Value *ColValue = ConstantInt::get(Int32Type, Col);
    Value *ConditionValue = Branch->getCondition();

    // Create the call instruction
    std::vector<Value *> Args = {LineValue, ColValue, ConditionValue};
    CallInst *InstrumentedCall = CallInst::Create(CBIBranchFunction, Args, "", Branch);

    // // Set the debug location for the call instruction
    // InstrumentedCall->setDebugLoc(Branch->getDebugLoc());
    // // Insert the instrumentation call before the branch instruction
    // InstrumentedCall->insertBefore(Branch);
  }

  /**
   * Implement instrumentation for the return scheme of CBI.
   */
  void instrumentReturn(Module *M, CallInst *Call, int Line, int Col)
  {
    auto &Context = M->getContext();
    auto Int32Type = Type::getInt32Ty(Context);

    /**
     * TODO: Add code to instrument the Call Instruction.
     *
     * Note: CallInst::Create(.) follows Insert Before semantics.
     */

    // Insert a call to __cbi_return__ after the call instruction
    auto *CBIReturnFunction = M->getFunction(CBI_RETURN_FUNCTION_NAME);
    if (!CBIReturnFunction)
    {
      llvm::errs() << "Error: CBI return function not found\n";
      return;
    }

    // Create the arguments for the function call
    Value *LineValue = ConstantInt::get(Int32Type, Line);
    Value *ColValue = ConstantInt::get(Int32Type, Col);
    Value *ReturnValue = Call;

    // Create the call instruction
    std::vector<Value *> Args = {LineValue, ColValue, ReturnValue};
    CallInst *InstrumentedCall = CallInst::Create(CBIReturnFunction, Args);

    // Insert the instrumentation call after the original call instruction
    InstrumentedCall->insertAfter(Call);
  }

  char CBIInstrument::ID = 1;
  static RegisterPass<CBIInstrument> X(PASS_NAME, PASS_DESC, false, false);

} // namespace instrument
