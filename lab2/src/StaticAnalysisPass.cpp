#include "Instrument.h"
#include "Utils.h"

using namespace llvm;

namespace instrument
{

  const auto PASS_NAME = "StaticAnalysisPass";
  const auto PASS_DESC = "Static Analysis Pass";

  bool Instrument::runOnFunction(Function &F)
  {
    auto FunctionName = F.getName().str();
    outs() << "Running " << PASS_DESC << " on function " << FunctionName << "\n";

    outs() << "Locating Instructions\n";
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
      outs() << Line << ", " << Col << "\n";

      /**
       * TODO: Add code to check if the instruction is a BinaryOperator and if so,
       * print the information about its location and operands as specified in the
       * Lab document.
       */

      // Get kind of instruction. If it is Binary operator, print the information
      if (Inst.isBinaryOp())
      {
        auto opcode = static_cast<llvm::Instruction::BinaryOps>(Inst.getOpcode());
        auto symbol = getBinOpSymbol(opcode);
        auto name = getBinOpName(symbol);

        // Print out with format:
        // Division on Line 4, Column 13 with first operand %0 and second operand %1
        // <Operator> on Line <Line>, Column <Col> with first operand <OP1> and second
        // operand <OP2>
        outs() << name << " on Line " << Line << ", Col "
               << Col << " with first operand " << variable(Inst.getOperand(0))
               << " and second operand " << variable(Inst.getOperand(1)) << "\n";
      }
    }
    return false;
  }

  char Instrument::ID = 1;
  static RegisterPass<Instrument> X(PASS_NAME, PASS_NAME, false, false);

} // namespace instrument
