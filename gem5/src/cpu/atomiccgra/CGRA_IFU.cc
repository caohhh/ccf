/**
 * Instruction Fetch Unit for the CGRA
 * 
 * Created on 2023/11/23
 * Cao Heng
 * 
 * 
*/

#include "cpu/atomiccgra/CGRA_IFU.hh"

#include <iostream>
#include <fstream>
#include <vector>

#include "debug/CGRA_IFU.hh"

CGRA_IFU::CGRA_IFU()
{
}


CGRA_IFU::CGRA_IFU(unsigned int Xdim, unsigned int Ydim)
{
    cgraXDim = Xdim;
    cgraYDim = Ydim;
    cgraInstructions = new uint64_t[Xdim * Ydim];
    predOutput = new bool[Xdim * Ydim];
    condPE = -1;
    splitCondIssued = false;
    recoveryReg = -1;
    rollbackReg = -1;
    leIter = -1;
    rollbackPtr = 0;
    updatePtr = 0;
}


CGRA_IFU::CGRA_IFU(unsigned int Xdim, unsigned int Ydim, CGRAPredUnit* predictor)
{
    cgraXDim = Xdim;
    cgraYDim = Ydim;
    cgraInstructions = new uint64_t[Xdim * Ydim];
    cgraPred = predictor;
    predOutput = new bool[Xdim * Ydim];
    condPE = -1;
    splitCondIssued = false;
    splitController = -1;
    recoveryReg = -1;
    rollbackReg = -1;
    leIter =-1;
    rollbackPtr = 0;
    updatePtr = 0;
}


CGRA_IFU::~CGRA_IFU()
{
}


void
CGRA_IFU::setupExec(SimpleThread *thread, int loopID)
{
    std::string directoryPath = "./CGRAExec/L" + std::to_string(loopID) + "/initCGRA.txt";

    this->loopID = loopID;
    std::ifstream initCGRAFile;
    initCGRAFile.open(directoryPath.c_str());

    initCGRAFile >> liveinLen;
    initCGRAFile >> II;
    initCGRAFile >> liveoutLen;
    initCGRAFile >> iterCount;
    initCGRAFile >> liveinPC;
    initCGRAFile >> kernelPC;
    initCGRAFile >> iterPC;
    initCGRAFile >> liveoutPC;

    initCGRAFile.close();

    len=liveinLen;
    state=LIVEIN;
    conditionalReg = true;
    phiReg = true;
    kernEndReg = false;

    DPRINTF(CGRA_IFU,"CGRA PARAMETERS: Livein=%d, Liveout=%d, II=%d, Iter=%d\n",
            liveinLen, liveoutLen, II, iterCount);
    DPRINTF(CGRA_IFU,"CGRA PARAMETERS: LiveinPC= %lx, LiveoutPC=%lx, KernelPC=%lx, iterPC=%lx\n",
            (unsigned int)liveinPC,(unsigned int)liveoutPC,(unsigned int)kernelPC,(unsigned int)iterPC);

    liveinIns = new uint64_t[cgraXDim * cgraYDim * liveinLen];
    liveoutIns = new uint64_t[cgraXDim * cgraYDim * liveoutLen];
    kernelIns = new uint64_t[cgraXDim * cgraYDim * II * 3];
    iterInfo = new int[cgraXDim * cgraYDim * II];
    
    // setup predictor
    cgraPred->setIterCount(iterCount);

    // initialize all the controllers
    controller = new iterController[iterCount];
    backupController.resize(iterCount, std::vector<iterController>(iterCount));
    updatePtr = (rollbackPtr + 1) % iterCount;
    for (unsigned i = 1; i < iterCount; i++) {
        controller[i].iter = 0;
        controller[i].state = OFF;
        controller[i].offCounter = i;
        controller[i].speculative = true;
    }
    // for controller0, it needs to start working immediately
    controller[0].iter = 0;
    Addr splitPC = cgraXDim * cgraYDim * II;
    bool predState = cgraPred->predict(splitPC, true);
    if (predState == true)
        controller[0].state = TRUE_PATH;
    else
        controller[0].state = FALSE_PATH;
    controller[0].offCounter = 0;
    controller[0].speculative = true;
    for (unsigned i = 0; i < iterCount; i++)
        for (unsigned j = 0; j < iterCount; j++)
            backupController[i][j] = controller[j];
}


void
CGRA_IFU::advanceState()
{
    DPRINTF(CGRA_IFU,"*******IN ADVANCE State******\n");
    recoveryReg = -1;
    rollbackReg = -1;
    DPRINTF(CGRA_IFU,"Controller states before:\n");
    DPRINTF(CGRA_IFU, "rollback pointer: %d, update pointer: %d\n", rollbackPtr, updatePtr);
    for (unsigned i = 0; i < iterCount; i++)
        DPRINTF(CGRA_IFU, "Controller: %d, iter: %d, state: %d, off counter: %d\n",\
                i, controller[i].iter, controller[i].state,controller[i].offCounter);


    // we need to update the predictor after execution is done
    if (condPE != -1) {
        // a cond is issued this cycle, need to update the predictor with its result
        bool condResult = predOutput[condPE];
        if (splitCondIssued) {
            // if there is a split cond in fly this t
            // this split should be of the oldest speculative in fly, which is in the controller having the largest iter
            // now get the state of the controller
            // we can skip this if the controller is not speculative
            if (controller[splitController].speculative) {
                // the speculative state
                bool specState;
                if (controller[splitController].state == TRUE_PATH)
                    specState = true;
                else if (controller[splitController].state == FALSE_PATH)
                    specState = false;
                else
                    panic("Split cond issued by an off controller, should not happen");
                // when a split cond is resolved, the rollbackPtr should always advance no matter
                // if it is an rollback
                // see if specState matches the actual state, and if a roll back is needed
                if (specState == condResult) {
                    // all good
                    DPRINTF(CGRA_IFU, "controller %d state predicted correct\n", splitController);
                    // set the controller as not speculative
                    controller[splitController].speculative = false;
                    // advance rollbackPtr
                    rollbackPtr = (rollbackPtr + 1) % iterCount;
                    // update predicter
                    Addr splitPC = cgraXDim * cgraYDim * II;
                    cgraPred->update(splitPC, true, condResult, false);
                } else {
                    // rollback needed
                    DPRINTF(CGRA_IFU, "controller %d state predicted incorrect, rollback needed\n", splitController);
                    // controller rollback
                    // the splitcontroller need to go back to iteration 0
                    DPRINTF(CGRA_IFU, "rolling back to pointer: %d\n", rollbackPtr);
                    for (unsigned i = 0; i < iterCount; i++) {
                        controller[i] = backupController[rollbackPtr][i];
                    }
                    // switch the output of all PEs
                    rollbackReg = rollbackPtr;
                    // also reset the updatePtr
                    updatePtr = (rollbackPtr + 1) % iterCount;
                    // advance rollbackPtr
                    rollbackPtr = (rollbackPtr + 1) % iterCount;
                    // correct split controller state
                    if (condResult == true)
                        controller[splitController].state = TRUE_PATH;
                    else
                        controller[splitController].state = FALSE_PATH;
                    controller[splitController].speculative = false;
                    // update predictor
                    cgraPred->squash();
                    Addr splitPC = cgraXDim * cgraYDim * II;
                    cgraPred->update(splitPC, true, condResult, true);
                    // reset len
                    len = II;
                    // rollback should also reset condition reg
                    conditionalReg = true;
                }
            }
        } else {
            // the issued cond is not the split cond, just need to update the predictor
            unsigned t = II - len + 1;
            Addr condPC = t * cgraXDim * cgraYDim + condPE;
            cgraPred->update(condPC, false, condResult, false);
        }
    }

    DPRINTF(CGRA_IFU,"current state: %u\n", state);

    // state transition for the entire CGRA
    if (len == 0) {
        if (state == LIVEIN) {
            DPRINTF(CGRA_IFU,"\nLIVEIN->KERNEL\n");
            state = KERN;
            len = II;
        } else if (state == KERN) { // end of state == LIVEIN
            if (!kernEndReg) {
                len = II;
                if (!conditionalReg) {
                    // we are entering epilogue, but still need to take care of speculation and rollback
                    // iter before LE iter: finish this iter and stop issuing new iter
                    // iter after LE iter: stop immediately
                    DPRINTF(CGRA_IFU,"Entering epilogue\n");
                    kernEndReg = true;
                    for (unsigned i = 0; i < iterCount; i++) {
                        if (controller[i].iter < leIter)
                            controller[i].state = OFF;
                        else if (controller[i].iter == iterCount - 1)
                            controller[i].state = OFF;
                        else {
                            controller[i].iter++;
                            kernEndReg = false;
                        }
                    }
                } else {
                    // the recover system:
                    // recover sets should be the same as iter count, when a new iter is to be issued
                    // two pointers exist, one to the set to update, one to the set to roll back to
                    // update pointer moves every new iter, 
                    // rollback pointer moves when a iter prediction is resolved to be correct
                    // in case of a roll back, update pointer should also move to the next of rollback pointer

                    int newIterController = -1;
                    for (unsigned i = 0; i < iterCount; i++) {
                        // new iter indicator
                        bool newIter = false;
                        if (controller[i].offCounter != 0) {
                            if (controller[i].offCounter == 1)
                                newIter = true;
                        } else {
                            if (controller[i].iter == iterCount - 1)
                                newIter = true;
                        }
                        if (newIter) {
                            if (newIterController != -1)
                                panic("2 controller with new iteration, should not happen");
                            else
                                newIterController = i;
                        }
                    }
                    if (newIterController != -1) {
                        // there is a new iteration to issue
                        DPRINTF(CGRA_IFU, "this is a recovery point\n");
                        recoveryReg = updatePtr;
                        // back up controllers are done later after the update
                    }

                    // update each iteration controller
                    for (unsigned i = 0; i < iterCount; i++) {
                        DPRINTF(CGRA_IFU, "Updating controller %d\n", i);
                        // new iter indicator
                        bool newIter = false;
                        if (controller[i].offCounter != 0) {
                            controller[i].offCounter--;
                            if (controller[i].offCounter == 0)
                                newIter = true;
                        } else {
                            controller[i].iter++;
                            if (controller[i].iter == iterCount) {
                                controller[i].iter = 0;
                                newIter = true;
                            }
                        }
                        if (newIter) {
                            // if a new iteration is issued, the state needs to be updated
                            // use the predictor for update
                            // this PC just have to be consistant for the same instruction
                            // for the splitcond, we set it to maxPC + 1, which will be II * xdim * ydim
                            Addr splitPC = II * cgraXDim * cgraYDim;
                            bool predState = cgraPred->predict(splitPC, true);
                            if (predState == true)
                                controller[i].state = TRUE_PATH;
                            else
                                controller[i].state = FALSE_PATH;
                            controller[i].speculative = true;
                            DPRINTF(CGRA_IFU,"new iteration, state updated as %d\n", controller[i].state);
                            // if controller0 enters an new iteration, it means that phi should be over
                            if (i == 0) {
                                phiReg = false;
                                DPRINTF(CGRA_IFU,"First phi stage is over\n");
                            }
                        }
                    }
                    // backup all the controllers here since when rollback, we also need to 
                    // update the controller
                    if (newIterController != -1) {
                        for (unsigned i = 0; i < iterCount; i++)
                            backupController[updatePtr][i] = controller[i];
                        // also advance the pointer
                        updatePtr = (updatePtr + 1) % iterCount;
                        DPRINTF(CGRA_IFU, "update pointer set to: %d\n", updatePtr);
                    }
                }
                DPRINTF(CGRA_IFU,"Controller states after:\n");
                for (unsigned i = 0; i < iterCount; i++)
                    DPRINTF(CGRA_IFU, "Controller: %d, iter: %d, state: %d, off counter: %d\n",\
                            i, controller[i].iter, controller[i].state,controller[i].offCounter);
            } else {
                DPRINTF(CGRA_IFU,"\nKERNEL->LIVEOUT\n");
                state = LIVEOUT;
                len = liveoutLen;
            }
        } else if (state == LIVEOUT) // end of state == KERN
            state = FINISH;
    }

    DPRINTF(CGRA_IFU,"state after advance state: %u\n", state);
}


void
CGRA_IFU::advanceTime()
{
    len--;
}


CGRA_IFU::State
CGRA_IFU::getState()
{
    return state;
}


bool
CGRA_IFU::finishedIter()
{
    if (state == KERN && len == 0)
        return true;
    else
        return false;
}


uint64_t
CGRA_IFU::getInsWord(unsigned position)
{
    return cgraInstructions[position];
}


void
CGRA_IFU::setConditionalReg(bool reg)
{
    conditionalReg &= reg;
}


void
CGRA_IFU::setPred(unsigned peIndex, bool predication)
{
    predOutput[peIndex] = predication;
}


std::tuple<unsigned, uint64_t*, long> 
CGRA_IFU::getLiveinReq()
{
    unsigned size = cgraXDim * cgraYDim * liveinLen * sizeof(uint64_t);
    return std::make_tuple(size, liveinIns, liveinPC);
}


std::tuple<unsigned, uint64_t*, long> 
CGRA_IFU::getKernelReq()
{
    unsigned size = cgraXDim * cgraYDim * II * 3 * sizeof(uint64_t);
    return std::make_tuple(size, kernelIns, kernelPC);
}


std::tuple<unsigned, uint64_t*, long> 
CGRA_IFU::getLiveoutReq()
{
    unsigned size = cgraXDim * cgraYDim * liveoutLen * sizeof(uint64_t);
    return std::make_tuple(size, liveoutIns, liveoutPC);
}


std::tuple<unsigned, int*, long> 
CGRA_IFU::getIterReq()
{
    unsigned size = cgraXDim * cgraYDim * II * sizeof(int);
    return std::make_tuple(size, iterInfo, iterPC);
}


void
CGRA_IFU::printIns()
{
    DPRINTF(CGRA_IFU, "******LIVEIN*******\n");
    for (int i = 0; i < cgraXDim * cgraYDim * liveinLen; i++)
        DPRINTF(CGRA_IFU, "%d: %llx\n", i, liveinIns[i]);
    DPRINTF(CGRA_IFU,"******KERNEL*******\n");
    for (int i = 0; i < cgraXDim * cgraYDim * II * 3; i++)
        DPRINTF(CGRA_IFU, "%d: %llx\n", i, kernelIns[i]);
    DPRINTF(CGRA_IFU,"******ITER*******\n");
    for (int i = 0; i < cgraXDim * cgraYDim * II; i++)
        DPRINTF(CGRA_IFU, "%d: %d\n", i, iterInfo[i]);
    DPRINTF(CGRA_IFU,"******LIVEOUT*******\n");
    for (int i = 0; i < cgraXDim * cgraYDim * liveoutLen; i++)
        DPRINTF(CGRA_IFU, "%d: %llx\n", i, liveoutIns[i]);
    DPRINTF(CGRA_IFU,"******DONE*******\n");
}


void
CGRA_IFU::issue()
{
    condPE = -1;
    splitCondIssued = false;

    if (state == LIVEIN) {
        // for live in issue in order
        cgraInstructions = liveinIns + (size_t)((liveinLen - len) * cgraXDim * cgraYDim);
    } else if (state == LIVEOUT) {
        // for live out, also issue all ins in order
        cgraInstructions = liveoutIns + (size_t)((liveoutLen - len) * cgraXDim * cgraYDim);
    } else if (state == KERN) {
        // for kernel, it is much more complicated
        // the current time in II
        unsigned t = II - len;
        iterState iterControl[iterCount];
        for (unsigned i = 0; i < iterCount; i++)
            iterControl[i] = OFF;

        for (unsigned i = 0; i < iterCount; i++) {
            for (unsigned j = 0; j < iterCount; j++) {
                if (controller[j].state != OFF && controller[j].iter == i)
                    iterControl[i] = controller[j].state;
            } // end of iterating through controllers
        } // end of iterating through iterations

        for (unsigned i = 0; i < iterCount; i++)
            DPRINTF(CGRA_IFU, "Iter: %d, State: %d\n", i, iterControl[i]);

        for (unsigned x = 0; x < cgraXDim; x++)
            for (unsigned y = 0; y < cgraYDim; y++) {
                // for each physical PE in the time slot
                // first get the iter info
                int iteration = iterInfo[t * cgraXDim * cgraYDim + x * cgraYDim + y];
                if (iteration == -1) {
                    // just use the true slot, since both slots will be noop
                    cgraInstructions[x * cgraYDim + y] = \
                    kernelIns[(t * cgraXDim * cgraYDim + x * cgraYDim + y) * 3];
                } else {
                    // now it should depend on the controller for each iteration
                    if (iterControl[iteration] == OFF) {
                        // noop ins
                        cgraInstructions[x * cgraYDim + y] = \
                        CGRA_Instruction(int32, NOOP, Self, Self, 0, 0, 0, false, 0, false, false).getInsWord();
                    } else if (phiReg && isPhi(t, x, y) && controller[0].iter == iteration) {
                        // phi reg meaning this is the first time
                        // isPhi ensures this is a phi op
                        // matching controller0 this is the first iteration
                        cgraInstructions[x * cgraYDim + y] = \
                        kernelIns[(t * cgraXDim * cgraYDim + x * cgraYDim + y) * 3 + 2];
                    } else if (iterControl[iteration] == TRUE_PATH) {
                        cgraInstructions[x * cgraYDim + y] = \
                        kernelIns[(t * cgraXDim * cgraYDim + x * cgraYDim + y) * 3];
                    } else if (iterControl[iteration] == FALSE_PATH) {
                        cgraInstructions[x * cgraYDim + y] = \
                        kernelIns[(t * cgraXDim * cgraYDim + x * cgraYDim + y) * 3 + 1];
                    }
                }
                // we also mark out the c-type here
                if (CGRA_Instruction(cgraInstructions[x * cgraYDim + y]).getCond()) {
                    auto condIns = Cond_Instruction(cgraInstructions[x * cgraYDim + y]);
                    if (condIns.getloopExitEnable()) {
                        // this is the loop exit, we don't use the predictor on it
                        leIter = iterInfo[t * cgraXDim * cgraYDim + x * cgraYDim + y];
                        DPRINTF(CGRA_IFU, "This cond instruction is the loopexit, iteration: %d\n", leIter);
                    } else {
                        // here, only one cond can be fed to predictor in 1 cycle, we currently use the last PE
                        // if there is no split cond, outherwise, it will be the splitcond
                        DPRINTF(CGRA_IFU, "Cond instruction @PE %d\n", x*cgraYDim+y);
                        if (condIns.getSplitDirection()) {
                            // this is the split condition, mark out the PE and the controller
                            splitCondIssued = true;
                            condPE = x * cgraYDim + y;
                            int condIter = iterInfo[t * cgraXDim * cgraYDim + x * cgraYDim + y];
                            for (unsigned i = 0; i < iterCount; i++) {
                                if (controller[i].iter == condIter && controller[i].state != OFF)
                                    splitController = i;
                            }
                            DPRINTF(CGRA_IFU, "This cond instruction is the splitcond, controller: %d\n", splitController);
                        } else {
                            // not the split cond
                            if (!splitCondIssued)
                                condPE = x * cgraYDim + y;
                        }
                    }
                }
            }

    } else {
        panic("Issue should not reach FINISH state");
    }
}


bool
CGRA_IFU::isPhi(int t, int x, int y)
{
    uint64_t insWord = kernelIns[(t * cgraXDim * cgraYDim + x * cgraYDim + y) * 3 + 2];
    if (((insWord & INS_PHI) >> SHIFT_PHI))
        return false;
    else
        return true;
}


int
CGRA_IFU::getRecoveryReg()
{
    return recoveryReg;
}


int
CGRA_IFU::getRollbackReg()
{
    return rollbackReg;
}


unsigned
CGRA_IFU::getIterCount()
{
    return iterCount;
}