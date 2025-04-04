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
        for (auto Succ = succ_begin(Block), BS = succ_end(Block);
             Succ != BS; ++Succ)
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

    // Iterate through each instruction-domain pair in Mem1
    for (auto &Pair : *Mem1)
    {
      // If the instruction is present in both memories, join their domains
      if (Mem2->find(Pair.first) != Mem2->end())
      {
        (*Result)[Pair.first] = Domain::join(Pair.second, (*Mem2)[Pair.first]);
      }
      else
      {
        // If the instruction is only in Mem1, add it to the result
        (*Result)[Pair.first] = Pair.second;
      }
    }

    // Iterate through each instruction-domain pair in Mem2
    for (auto &Pair : *Mem2)
    {
      // If the instruction is only in Mem2, add it to the result
      if (Mem1->find(Pair.first) == Mem1->end())
      {
        (*Result)[Pair.first] = Pair.second;
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
    // Iterate through each predecessor of the instruction
    for (auto Pred : getPredecessors(Inst))
    {
      // Get the Out Memory of the predecessor from the OutMap
      Memory *PredOut = OutMap[Pred];

      // Join the Out Memory with the current InMem
      Memory *JoinedMem = join(InMem, PredOut);

      InMem->insert(JoinedMem->begin(), JoinedMem->end());
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
     *   but not in both and the Domain of I is not UnInit, the memories are
     *   unequal.
     * If any instruction I is present in Mem1 with domain D1 and in Mem2
     *   with domain D2, if D1 and D2 are unequal, then the memories are unequal.
     */
    // Check if the sizes of the two memories are different
    if (Mem1->size() != Mem2->size())
    {
      return false;
    }

    // Iterate through each instruction-domain pair in Mem1
    for (auto &Pair : *Mem1)
    {
      // Check if the instruction is not present in Mem2
      if (Mem2->find(Pair.first) == Mem2->end())
      {
        // If the domain of the instruction is not Uninit, the memories are unequal
        if (Pair.second->Value != Domain::Uninit)
        {
          return false;
        }
      }
      // If the instruction is present in both memories, check if their domains are unequal
      else if (!Domain::equal(*Pair.second, *(*Mem2)[Pair.first]))
      {
        return false;
      }
    }

    // If all checks pass, the memories are equal
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
    Memory *JoinedMem = join(Pre, Post);

    if (!equal(OutMap[Inst], JoinedMem))
    {
      OutMap[Inst] = JoinedMem;
      for (auto Succ : getSuccessors(Inst))
      {
        WorkSet.insert(Succ);
      }
    }
    else
    {
      delete JoinedMem;
    }
  }

  void DivZeroAnalysis::doAnalysis(Function &F)
  {
    SetVector<Instruction *> WorkSet;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    {
      WorkSet.insert(&(*I));
    }
    /**
     * TODO: Write your code to implement the chaotic iteration algorithm
     * for the analysis.
     *
     * Initialize the WorkSet with all the instructions in the function.
     *
     * While the WorkSet is not empty:
     * - Pop an instruction from the WorkSet.
     * - Construct it's Incoming Memory using flowIn.
     * - Evaluate the instruction using transfer and create the OutMemory.
     * - Use flowOut along with the previous Out memory and the current Out
     *   memory, to check if there is a difference between the two to update the
     *   OutMap and add all successors to WorkSet.
     */
    while (!WorkSet.empty())
    {
      Instruction *Inst = WorkSet.pop_back_val();

      Memory *InMem = new Memory();
      flowIn(Inst, InMem);

      Memory *OutMem = new Memory(*InMem);
      transfer(Inst, InMem, *OutMem);

      flowOut(Inst, InMem, OutMem, WorkSet);

      delete InMem;
      delete OutMem;
    }
  }

} // namespace dataflow