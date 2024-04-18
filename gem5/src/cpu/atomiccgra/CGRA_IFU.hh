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
     * Setup IFU for the execution of one loop
    */
    void setupExec(SimpleThread *thread, int loopID);
    void advanceState();
    void advanceTime();

    enum State
    {
        LIVEIN,
        KERN,
        LIVEOUT,
        FINISH
    };
    State getState();

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
    void setConditionalReg(bool reg);

    /**
     * After execution, expose the predication output to the IFU
    */
    void setPred(unsigned peIndex, bool predication);

    // returns the size, ptr, and address of live in instructions
    std::tuple<unsigned, uint64_t*, long> getLiveinReq();

    // returns the size, ptr and address of kernel instructions
    std::tuple<unsigned, uint64_t*, long> getKernelReq();

    // returns the size, ptr and address of liveout instructions
    std::tuple<unsigned, uint64_t*, long> getLiveoutReq();

    // returns the size, ptr and address of iteration info
    std::tuple<unsigned, int*, long> getIterReq();

    // print out all the instructions
    void printIns();

    // issue instructions to cgraInstructions
    void issue();

    // if need to set recovery
    int getRecoveryReg();

    // if need rollback
    int getRollbackReg();

    // returns the iteration count
    unsigned getIterCount();

  private:
    unsigned int cgraXDim;
    unsigned int cgraYDim;
    CGRAPredUnit* cgraPred;

    // CGRA state controlling variables

    int loopID;
    long liveinPC;
    long liveoutPC;
    long kernelPC;
    long iterPC;
    unsigned II;
    unsigned liveinLen;
    unsigned liveoutLen;
    unsigned len;
    unsigned iterCount;

    State state;

    uint64_t *liveinIns;
    uint64_t *liveoutIns;
    uint64_t *kernelIns;
    int *iterInfo;


    // instructions issued to the PEs
    uint64_t *cgraInstructions;

    // False: LE instruction execution indicates should exit loop
    bool conditionalReg;

    // True: issuing first iter, should use phi ins, False: kernel running, issue normal ins
    bool phiReg;

    // True: kernel executiong have ended, should switch to liveout state
    bool kernEndReg;


    // the predication output of all the PEs
    bool *predOutput;

    // if the split condition have been issued
    bool splitCondIssued;

    // the PE of a condition issued, -1 for not issued
    int condPE;

    // iteration of the loop exit
    int leIter;

    // -1: not a recovery point
    // other: need to update and the update pointer to update backup
    int recoveryReg;

    // -1: no need to rollback
    // other: need to rollback and the rollback pointer
    int rollbackReg;

    enum iterState
    {
        OFF,
        TRUE_PATH,
        FALSE_PATH
    };

    struct iterController
    {
        // the iteration this controller controls
        int iter;
        // the state this iteration should be in
        iterState state;
        // a counter for how many cycles this controller should remain in OFF state
        unsigned offCounter;
        // if the contoller's iteration is speculative
        bool speculative;
        // if we are entering epilogue
        bool epilogue;
    };
    iterController* controller;
    std::vector<std::vector<iterController>> backupController;
    // pointer pointing to the set of backup to rollback to
    unsigned rollbackPtr;
    // pointer pointer to the set of backup to update
    unsigned updatePtr;


    // the controller No. the split cond in fly belongs to
    int splitController;


    // returns if the instruction at t, x, y is an phi ins
    bool isPhi(int t, int x, int y);
};

#endif