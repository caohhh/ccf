/*
* PE.h
*
* Created on: May 24, 2011
* Author: mahdi
*
* Last edited: 22 May 2017
* Author: Shail Dave
*
* Last edited: 10 July 2019
* Author: Mahesh Balasubramanian
* Updates: 1. Parameterized Rfsize and CGRA dims.  
           2. Added support for FP execution.
           3. Added support fot FP registers.
*
*/

#include "CGRARegisterFile.h"
#include "CGRAFPRegisterFile.h"
#include "CGRAInstruction.h"
#include "debug/PE_DEBUG.hh"
#include "debug/CGRA_Execute.hh"
//#include "CGRA_MemoryPort.h"

#ifndef PE_H_
#define PE_H_

class CGRA_RegisterFile;
class CGRA_FPRegisterFile; 
class CGRA_Instruction;

class CGRA_PE
{
  private:
    int regsize;
    bool Controller_Reg;

    // inputs for internal use of the PEs
    int Input1;
    int Input2;
    //Support for Predicated Operations or P-type instructions
    bool InputP;
    bool OutputP;
    int Output;
    unsigned config_boundary;
    // the output to recover to
    int* outputRecover;
    bool* outputPRecover;

    // FP inputs and outputs
    float FPInput1;
    float FPInput2;
    float FPOutput;
  
    CGRA_RegisterFile RegFile;
    CGRA_FPRegisterFile FPRegFile;
    CGRA_Instruction* ins; 

    //Int lines
    int *leftIn;
    int *rightIn;
    int *upIn;
    int *downIn;

    // FP lines  
    float *FPleftIn;
    float *FPrightIn;  
    float *FPupIn;
    float *FPdownIn;

    // Pred lines
    bool *PredleftIn;
    bool *PredrightIn;
    bool *PredupIn;
    bool *PreddownIn; 


    int* BsStatus;  
    int* BsDatatype; 
    float* FdataBs;
    int* dataBs;
    uint64_t* addressBs;
    unsigned* alignmentBs;
    Datatype dt; 

  public:
    CGRA_PE(); //default constructor to set the RF size 4 per PE.
    virtual ~CGRA_PE();
    // setup the backup registers of the PE
    void setupBackup(unsigned iterCount);

    void Fetch(CGRA_Instruction* ins);
    void Decode();
    void IExecute();
    void FExecute();
    void WriteBack();

    int* getOutputPtr();
    float* getFPOutputPtr();
    bool *getPredOutputPtr(); 
    float getFPOutput();
    int getOutput();
    bool getPredOutput();   

    void SetNeighbours(int* Left, int* Right, int* Up, int* Down);
    void SetFPNeighbours(float* Left, float* Right, float* Up, float* Down);
    void SetPredNeighbours(bool *Left, bool *Right, bool *Up, bool *Down); 

    void advanceTime();
    void setRF_per_PE(int rs);
    void setFPRF_per_PE(int rs);
    void SetController_Reg();
    bool getController_Reg();
    void D_WriteRegfile(int address, int value);
    void ClearRegfile();
    Datatype GetDatatype(); 

    void setDataBus(int * data);
    void setFDataBus(float * data);
    void setAddressBus(uint64_t * addr);
    void setRWStatusBus(int * status);
    void setAlignmentBus(unsigned *alignment);
    void setDatatypeBus(int * dt);

    bool isNOOP();

    // set the recovery output as the current output
    void setRecovery(int updatePtr);
    // rollback to the recovery output
    void rollback(int rollbackPtr);
};


#endif /* PE_H_ */
