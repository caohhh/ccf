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

#include "debug/CGRA_IFU.hh"

CGRA_IFU::CGRA_IFU()
{
}


CGRA_IFU::CGRA_IFU(unsigned int Xdim, unsigned int Ydim)
{
    cgraXDim = Xdim;
    cgraYDim = Ydim;
    cgraInstructions = new uint64_t[Xdim * Ydim];
}


CGRA_IFU::~CGRA_IFU()
{
}


void
CGRA_IFU::recordCMP(Addr PC, bool outcome)
{
    cmpHistory.push_back(std::make_pair(PC, outcome));
}


void
CGRA_IFU::printCMPHistory()
{
    DPRINTF(CGRA_IFU, "\n* * * * Printing out CMP History * * * * \n");
    for (const auto& pair : cmpHistory) {
        DPRINTF(CGRA_IFU, "PC: %x, outcome: %d\n", pair.first, pair.second);
    }
}


void
CGRA_IFU::setupExec(SimpleThread *thread, int loopID)
{
    std::string directoryPath = "./CGRAExec/L" + std::to_string(loopID) + "/initCGRA.txt";

    unsigned long temp;
    std::ifstream initCGRAFile;
    initCGRAFile.open(directoryPath.c_str());

    initCGRAFile >> temp;
    initCGRAFile >> temp;

    initCGRAFile >> II;
    initCGRAFile >> epilogLen;
    initCGRAFile >> prologLen;
    initCGRAFile >> temp; // KernelCounter
    initCGRAFile >> temp; // LiveVar_St_Epilog

    initCGRAFile >> epilogPC;
    initCGRAFile >> prologPC;
    initCGRAFile >> kernelPC;

    initCGRAFile >> temp; // Prolog_extension_cycle
    initCGRAFile >> prologVersionCycle;
    initCGRAFile.close();

    len=prologLen;
    state=PRO;
    conditionalReg = true;
    prologBranchCycle = 0;
    cgraPC = prologPC;

    DPRINTF(CGRA_IFU,"CGRA PARAMETERS: PROLOG=%d, EPILOG=%d, II=%d, PROLOG_VERSION_LEN=%d\n",
            prologLen, epilogLen, II, prologVersionCycle);
    DPRINTF(CGRA_IFU,"CGRA PARAMETERS: PROLOGPC= %lx, EPILOGPC=%lx,  KernelPC=%lx\n",
            (unsigned int)prologPC,(unsigned int)epilogPC,(unsigned int)kernelPC);

    thread->pcState((Addr) cgraPC);
    DPRINTF(CGRA_IFU,"CGRA PC : %x\n",prologPC);
}


void
CGRA_IFU::advancePC(SimpleThread *thread)
{
    DPRINTF(CGRA_IFU,"*******IN ADVANCE PC******\n");
    DPRINTF(CGRA_IFU,"%s,%s,%d,PC:%x\n",
        __FILE__,__func__,__LINE__,(unsigned int) thread->instAddr());

    DPRINTF(CGRA_IFU,"current state: %u\n", state);

    if (len == 0) {
        if (state == PRO) {
            if (prologBranchCycle == 0) {
                DPRINTF(CGRA_IFU,"\nPROLOG->KERNEL\n");
                state = KERN;
                cgraPC = kernelPC;
                len = II;
                conditionalReg = 1;
            } else { 
                cgraPC += ((sizeof(long))*(cgraXDim*cgraYDim)*prologBranchCycle);
                DPRINTF(CGRA_IFU, "\nJumped to: %lx\n", cgraPC);
                len = prologVersionCycle;
                state = EPI;
            }
        } else if (state == KERN) { // end of state == PRO
            if (conditionalReg) {
                len = II;
                cgraPC = kernelPC;
            } else {
                DPRINTF(CGRA_IFU,"\nKERNEL->EPILOG\n");
                state = EPI;
                cgraPC = epilogPC;
                len = epilogLen;
            }
        } else if (state == EPI) // end of state == KERN
            state = FINISH;
    } else { // end of Len == 0
        if (prologBranchCycle == 0)
            cgraPC += ((sizeof(long))*(cgraXDim*cgraYDim));
        else {
            cgraPC += ((sizeof(long))*(cgraXDim*cgraYDim)*prologBranchCycle);
            DPRINTF(CGRA_IFU, "\nJumped to: %lx\n", cgraPC);
            len = prologVersionCycle;
            state = EPI;
        }
    }
    thread->pcState((Addr) cgraPC);
    DPRINTF(CGRA_IFU,"state after advance PC: %u\n", state);
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


uint64_t*
CGRA_IFU::getInstPtr()
{
    return cgraInstructions;
}


uint64_t
CGRA_IFU::getInsWord(unsigned position)
{
    return cgraInstructions[position];
}


void
CGRA_IFU::setPrologBranchCycle(unsigned cycles)
{   
    prologBranchCycle += cycles; 
}


void
CGRA_IFU::setConditionalReg(bool reg)
{
    conditionalReg &= reg;
}