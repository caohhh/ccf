/*
 * CGRAInstruction.h
 *
 * Created on: May 24, 2011
 * Author: mahdi
 *
 * edited: 24 May 2017
 * Auhtor: Shail Dave
 *
 * Last edited: 5 April 2022
 * Author: Vinh Ta
 * Update: Added new field (LoopExit) to instruction word
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "CGRAdefinitions.h"

class CGRA_Instruction
{
  public:
    CGRA_Instruction();
    CGRA_Instruction(unsigned long InstructionWord);
    CGRA_Instruction(Datatype DType,OPCode opc,PEInputMux LMuxSel,PEInputMux RMuxSel,
            int RRegAdd1,int RRegAdd2, int WAdd, bool WE, int ImmVal, bool EDMAdd, bool DMData);
    virtual ~CGRA_Instruction();
  
    unsigned long getInsWord();
    Datatype getDatatype();
    OPCode getOpCode();
    bool getPredicator();
    bool getCond();
    PEInputMux getLeftMuxSelector();
    PEInputMux getRightMuxSelector();
    int getReadRegAddress1();
    int getReadRegAddress2();
    int getWriteRegAddress();
    bool getWriteRegisterEnable();
    int getImmediateValue();
    bool getSelectDataMemoryAddressBus();
    bool getSelectDataMemoryDataBus();

  private:
    Datatype DType;
    OPCode opCode;
    bool Predicator;
    bool cond;
    PEInputMux LeftMuxSelector;
    PEInputMux RightMuxSelector;
    int ReadRegAddress1;
    int ReadRegAddress2;
    int WriteRegAddress;
    bool WriteRegisterEnable;
    int ImmediateValue;
    bool SelectDataMemoryAddressBus;
    bool SelectDataMemoryDataBus;
    unsigned long InsWord;

    // instruction word -> attributes
    void decodeInstruction();

    // attributes of instruction -> instruction word
    void encodeInstruction();
};


class Pred_Instruction
{
  public:
    Pred_Instruction();
    Pred_Instruction(unsigned long InstructionWord);
    Pred_Instruction(Datatype DType, PredOPCode popc, PEInputMux LMuxSel, PEInputMux RMuxSel, PEInputMux PMuxSel,
            int RRegAdd1,int RRegAdd2, int RRegAddP, int ImmVal);
    virtual ~Pred_Instruction();

    unsigned long getPredInsWord();
    Datatype getDatatype();
    PredOPCode getPredOpCode();
    PEInputMux getLeftMuxSelector();
    PEInputMux getRightMuxSelector();
    PEInputMux getPredMuxSelector();
    int getReadRegAddress1();
    int getReadRegAddress2();
    int getReadRegAddressP();
    int getImmediateValue();

  private:
    Datatype DType;
    PredOPCode popCode;
    PEInputMux LeftMuxSelector;
    PEInputMux RightMuxSelector;
    PEInputMux PredMuxSelector;
    int ReadRegAddress1;
    int ReadRegAddress2;
    int ReadRegAddressP;
    int ImmediateValue;
    unsigned long PredInsWord;

    // instruction word -> attributes
    void decodePredInstruction();

    // instruction attributes -> instruction word
    void encodePredInstruction();
};


class Cond_Instruction
{
  public:
    Cond_Instruction();
    Cond_Instruction(unsigned long InstructionWord);
    Cond_Instruction(Datatype dt, CondOpCode opc, PEInputMux LMuxSel, PEInputMux RMuxSel,
            int RRegAdd1,int RRegAdd2, int WAdd, bool WE, int ImmVal, 
            bool splitDirectionBit, bool loopExitEn);
    virtual ~Cond_Instruction();

    unsigned long getCondInsWord();
    Datatype getDatatype();
    CondOpCode getOpCode();
    PEInputMux getLeftMuxSelector();
    PEInputMux getRightMuxSelector();
    int getReadRegAddress1();
    int getReadRegAddress2();
    int getWriteRegAddress();
    bool getWriteRegisterEnable();
    int getImmediateValue();
    bool getSplitDirection();
    bool getloopExitEnable();

  private:
    Datatype DType;
    CondOpCode condOpCode;
    PEInputMux LeftMuxSelector;
    PEInputMux RightMuxSelector;
    int ReadRegAddress1;
    int ReadRegAddress2;
    int WriteRegAddress;
    bool WriteRegisterEnable;
    int ImmediateValue;
    bool splitDirection;
    bool loopExitEnable;
    unsigned long condInsWord;

    // instruction word -> attributes
    void decodeCondInstruction();

    // instruction attributes -> instruction word
    void encodeCondInstruction();
};
  
#endif /* INSTRUCTION_H_ */
