/**
 * Instruction Fetch Unit for the CGRA
 * 
 * Even though this is called the instruction fetch unit,
 * the actual fetching of instructions is left in the actomiccgra.
 * A seperate fetching unit for a atomic cpu would be too much work.
 * 
 * So this is more like a PC and state controller unit.
 * 
 * Created on 2023/11/23
 * Cao Heng
 * 
 * 
*/

#ifndef __CGRA_IFU_HH__
#define __CGRA_IFU_HH__

#include "cpu/atomiccgra/exec_context.hh"
#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"
#include "cpu/atomiccgra/CGRAInstruction.h"

class CGRA_IFU
{
  public:

    CGRA_IFU();
    CGRA_IFU(unsigned int Xdim, unsigned int Ydim);
    CGRA_IFU(unsigned int Xdim, unsigned int Ydim, CGRAPredUnit* predictor);
    virtual ~CGRA_IFU();

    /**
     * Debug function to print CMP history
    */
    void printCMPHistory();

    /**
     * Setup IFU for the execution of one loop
    */
    void setupExec(SimpleThread *thread, int loopID);
    void advancePC(SimpleThread *thread);
    void advanceTime();

    enum State
    {
        PRO,
        KERN,
        EPI,
        FINISH
    };
    State getState();
    // returns the pointer to the instructions in CGRA
    uint64_t* getInstPtr();

    /**
     * returns the instruction word given a PE's position
     * 
     * @param position yPosition * yDim + xPosition
    */
    uint64_t getInsWord(unsigned position);
    /**
     * CGRA finished the execution of an iteration of the kernal in 
     * this cycle
    */
    bool finishedIter();
    void setPrologBranchCycle(unsigned cycles);
    void setConditionalReg(bool reg);

    /**
     * Before individule PEs fetch instructions from IFU, mark out
     * all the cond instructions
    */
    void markCond();

    /**
     * After execution, expose the predication output to the IFU
    */
    void setPred(unsigned peIndex, bool predication);

    /**
     * Use the predictor to predict the outcome of all the CMP 
     * instructions in fly
    */
    void predict();

  private:
    // <PC, outcome> result of all the CMP instruction history
    std::vector<std::pair<Addr, bool>> cmpHistory;
    unsigned int cgraXDim;
    unsigned int cgraYDim;
    CGRAPredUnit* cgraPred;

    // CGRA state controlling variables

    int loopID;
    long epilogPC;
    long prologPC;
    long kernelPC;
    unsigned II;
    unsigned epilogLen;
    unsigned prologLen;
    unsigned prologVersionCycle;
    unsigned len;

    // PC CGRA is on, should be the same pc as the inst in pe0
    unsigned long cgraPC;
    unsigned prologBranchCycle;
    State state;

    uint64_t *cgraInstructions;

    // False: LE instruction execution indicates should exit loop
    bool conditionalReg;

    // If the instruction in the PE is a Cond instruction
    bool *isCond;

    // the predication output of all the PEs
    bool *predOutput;

    // prediction of all the predications
    bool *predPredicted;

    // with the predication output of the PEs, update the predictor
    void updatePredictor();

    /**
     * register this result into the compare history
     * 
     * @param PC PC of the CMP instruction
     * @param outcome the result of this CMP instruction
    */
    void recordCMP(Addr PC, bool outcome);
};

#endif