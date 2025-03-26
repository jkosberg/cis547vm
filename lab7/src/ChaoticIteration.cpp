#include "DivZeroAnalysis.h"
#include "Utils.h"

namespace dataflow
{

  /**
   * @brief Get the Predecessors of a given instruction in the control-flow graph.
   *
   * @param Inst The instruction to get the predecessors of.
   * @return Vector of all predecessors of Inst.
   */
  std::vector<Instruction *> getPredecessors(Instruction *Inst)
  {
    std::vector<Instruction *> Ret;
    auto Block = Inst->getParent();
    for (auto Iter = Block->rbegin(), End = Block->rend(); Iter != End; ++Iter)
    {
      if (&(*Iter) == Inst)
      {
        ++Iter;
        if (Iter != End)
        {
          Ret.push_back(&(*Iter));
          return Ret;
        }
        for (auto Pre = pred_begin(Block), BE = pred_end(Block); Pre != BE;
             ++Pre)
        {
          Ret.push_back(&(*((*Pre)->rbegin())));
        }
        return Ret;
      }
    }
    return Ret;
  }

  /**
   * @brief Get the successors of a given instruction in the control-flow graph.
   *
   * @param Inst The instruction to get the successors of.
   * @return Vector of all successors of Inst.
   */
  std::vector<Instruction *> getSuccessors(Instruction *Inst)
  {
    std::vector<Instruction *> Ret;
    auto Block = Inst->getParent();
    for (auto Iter = Block->begin(), End = Block->end(); Iter != End; ++Iter)
    {
      if (&(*Iter) == Inst)
      {
        ++Iter;
        if (Iter != End)
        {
          Ret.push_back(&(*Iter));
          return Ret;
        }
        for (auto Succ = succ_begin(Block), BS = succ_end(Block); Succ != BS;
             ++Succ)
        {
          Ret.push_back(&(*((*Succ)->begin())));
        }
        return Ret;
      }
    }
    return Ret;
  }

  /**
   * @brief Joins two Memory objects (Mem1 and Mem2), accounting for Domain
   * values.
   *
   * @param Mem1 First memory.
   * @param Mem2 Second memory.
   * @return The joined memory.
   */
  Memory *join(Memory *Mem1, Memory *Mem2)
  {
    /**
     * TODO: Write your code that joins two memories.
     *
     * If some instruction with domain D is either in Mem1 or Mem2, but not in
     *   both, add it with domain D to the Result.
     * If some instruction is present in Mem1 with domain D1 and in Mem2 with
     *   domain D2, then Domain::join D1 and D2 to find the new domain D,
     *   and add instruction I with domain D to the Result.
     */
    Memory *Result = new Memory();

    // Iterate over the first memory
    for (const auto &Pair : *Mem1)
    {
      const std::string &Inst = Pair.first;
      Domain *Domain1 = Pair.second;

      // Check if the instruction is present in the second memory
      if (Mem2->find(Inst) != Mem2->end())
      {
        // If the instruction is present in both memories, join the domains
        Domain *Domain2 = (*Mem2)[Inst];
        Domain *JoinedDomain = Domain::join(Domain1, Domain2);
        (*Result)[Inst] = JoinedDomain;
      }
      else
      {
        // If the instruction is only in the first memory, add it to the result
        (*Result)[Inst] = Domain1;
      }
    }

    // Iterate over the second memory to add instructions not in the first memory
    for (const auto &Pair : *Mem2)
    {
      const std::string &Inst = Pair.first;
      if (Mem1->find(Inst) == Mem1->end())
      {
        // If the instruction is only in the second memory, add it to the result
        (*Result)[Inst] = Pair.second;
      }
    }

    return Result;
  }

  void DivZeroAnalysis::flowIn(Instruction *Inst, Memory *InMem)
  {
    /**
     * TODO: Write your code to implement flowIn.
     *
     * For each predecessor Pred of instruction Inst, do the following:
     *   + Get the Out Memory of Pred using OutMap.
     *   + Join the Out Memory with InMem.
     */
    // Get the predecessors of the instruction
    std::vector<Instruction *> Predecessors = getPredecessors(Inst);

    // For each predecessor, get the Out Memory and join it with InMem
    for (Instruction *Pred : Predecessors)
    {
      Memory *OutMem = OutMap[Pred];
      Memory *JoinedMem = join(InMem, OutMem);
      *InMem = *JoinedMem;
      delete JoinedMem;
    }

    // Update InMap with the value of InMem
    InMap[Inst] = new Memory(*InMem);
  }

  /**
   * @brief This function returns true if the two memories Mem1 and Mem2 are
   * equal.
   *
   * @param Mem1 First memory
   * @param Mem2 Second memory
   * @return true if the two memories are equal, false otherwise.
   */
  bool equal(Memory *Mem1, Memory *Mem2)
  {
    /**
     * TODO: Write your code to implement check for equality of two memories.
     *
     * If any instruction I is present in one of Mem1 or Mem2,
     *   but not in both and the Domain of I is not Uninit, the memories are
     *   unequal.
     * If any instruction I is present in Mem1 with domain D1 and in Mem2
     *   with domain D2, if D1 and D2 are unequal, then the memories are unequal.
     */
    // Iterate over the first memory
    for (const auto &Pair : *Mem1)
    {
      const std::string &Inst = Pair.first;
      Domain *Domain1 = Pair.second;

      // Check if the instruction is not present in the second memory
      if (Mem2->find(Inst) == Mem2->end())
      {
        // If the domain is not Uninit, the memories are unequal
        if (Domain1->Value != Domain::Uninit)
        {
          return false;
        }
      }
      else
      {
        // If the instruction is present in both memories, compare the domains
        Domain *Domain2 = (*Mem2)[Inst];
        if (!Domain::equal(*Domain1, *Domain2))
        {
          return false;
        }
      }
    }

    // Iterate over the second memory to check for instructions not in the first memory
    for (const auto &Pair : *Mem2)
    {
      const std::string &Inst = Pair.first;
      Domain *Domain2 = Pair.second;

      // Check if the instruction is not present in the first memory
      if (Mem1->find(Inst) == Mem1->end())
      {
        // If the domain is not Uninit, the memories are unequal
        if (Domain2->Value != Domain::Uninit)
        {
          return false;
        }
      }
    }

    // If all checks passed, the memories are equal
    return true;
  }

  void DivZeroAnalysis::flowOut(Instruction *Inst, Memory *Pre, Memory *Post,
                                SetVector<Instruction *> &WorkSet)
  {
    /**
     * TODO: Write your code to implement flowOut.
     *
     * For each given instruction, merge abstract domain from pre-transfer memory
     * and post-transfer memory, and update the OutMap.
     * If the OutMap changed then also update the WorkSet.
     */
    // Merge abstract domain from pre-transfer memory and post-transfer memory

    Memory *JoinedMem = join(Pre, Post);

    // Check if the OutMap has changed
    if (!equal(OutMap[Inst], JoinedMem))
    {
      if (isa<StoreInst>(Inst) || isa<LoadInst>(Inst))
      {
        auto newMem = new Memory(*Post);
        OutMap[Inst] = newMem;
      }
      else
      {
        OutMap[Inst] = JoinedMem;
      }

      // Add all successors to WorkSet
      std::vector<Instruction *> Successors = getSuccessors(Inst);
      for (Instruction *Succ : Successors)
      {
        WorkSet.insert(Succ);
      }
    }
    else
    {
      delete JoinedMem;
    }
  }

  void DivZeroAnalysis::doAnalysis(Function &F, PointerAnalysis *PA)
  {
    SetVector<Instruction *> WorkSet;
    SetVector<Value *> PointerSet;
    /**
     * TODO: Write your code to implement the chaotic iteration algorithm
     * for the analysis.
     *
     * First, find the arguments of function call and instantiate abstract domain values
     * for each argument.
     * Initialize the WorkSet and PointerSet with all the instructions in the function.
     * The rest of the implementation is almost similar to the previous lab.
     *
     * While the WorkSet is not empty:
     * - Pop an instruction from the WorkSet.
     * - Construct it's Incoming Memory using flowIn.
     * - Evaluate the instruction using transfer and create the OutMemory.
     *   Note that the transfer function takes two additional arguments compared to previous lab:
     *   the PointerAnalysis object and the populated PointerSet.
     * - Use flowOut along with the previous Out memory and the current Out
     *   memory, to check if there is a difference between the two to update the
     *   OutMap and add all successors to WorkSet.
     */
    // Initialize the WorkSet and PointerSet with all the instructions in the function
    for (auto &BasicBlock : F)
    {
      for (auto &Instruction : BasicBlock)
      {
        WorkSet.insert(&Instruction);
        if (auto *Pointer = dyn_cast<Value>(&Instruction))
        {
          PointerSet.insert(Pointer);
        }
      }
    }

    // While the WorkSet is not empty
    while (!WorkSet.empty())
    {
      // Pop an instruction from the WorkSet
      Instruction *Inst = WorkSet.pop_back_val();

      // Construct its Incoming Memory using flowIn
      Memory *InMem = new Memory();
      flowIn(Inst, InMem);

      // Evaluate the instruction using transfer and create the OutMemory
      Memory *OutMem = new Memory(*InMem);
      transfer(Inst, InMem, *OutMem, PA, PointerSet);

      // Use flowOut along with the previous Out memory and the current Out memory
      // to check if there is a difference between the two to update the OutMap
      // and add all successors to WorkSet
      flowOut(Inst, InMem, OutMem, WorkSet);

      delete InMem;
      delete OutMem;
    }
  }

} // namespace dataflow