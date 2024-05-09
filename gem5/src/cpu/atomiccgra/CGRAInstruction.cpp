/*
* Instruction.cpp
*
* Created on: May 24, 2011
* Author: mahdi
*
* Last edited: 24 May 2017
* Author: Shail Dave
*/

#include "CGRAInstruction.h"
#include "debug/Instruction_Debug.hh"

/************************************CGRA_Instruction************************************/

CGRA_Instruction::CGRA_Instruction()
{
}

CGRA_Instruction::CGRA_Instruction(unsigned long Instructionword)
{
    InsWord = Instructionword;
    decodeInstruction();
}

CGRA_Instruction::CGRA_Instruction(Datatype dt,OPCode opc,PEInputMux LMuxSel, PEInputMux RMuxSel, 
                int RRegAdd1,int RRegAdd2, int WAdd, bool WE, int ImmVal, bool EDMAdd, bool DMData)
{
    DType=dt;
    opCode=opc;
    Predicator = false;
    cond = false;
    LeftMuxSelector=LMuxSel;
    RightMuxSelector=RMuxSel;
    ReadRegAddress1=RRegAdd1;
    ReadRegAddress2=RRegAdd2;
    WriteRegAddress=WAdd;
    WriteRegisterEnable=WE;
    ImmediateValue=ImmVal;
    SelectDataMemoryAddressBus=EDMAdd;
    SelectDataMemoryDataBus=DMData;
    encodeInstruction();
}

CGRA_Instruction::~CGRA_Instruction()
{
}

unsigned long 
CGRA_Instruction::getInsWord()
{
    return InsWord;
}


Datatype 
CGRA_Instruction::getDatatype()
{
    return DType;
}


OPCode 
CGRA_Instruction::getOpCode()
{
    DPRINTF(Instruction_Debug, "Opcode from get: %lx\n", (unsigned long)opCode); 
    return opCode;
}


bool 
CGRA_Instruction::getPredicator()
{
    return Predicator;
}


bool CGRA_Instruction::getCond()
{
    return cond;
}


PEInputMux 
CGRA_Instruction::getLeftMuxSelector()
{
    return LeftMuxSelector;
}


PEInputMux 
CGRA_Instruction::getRightMuxSelector()
{
    return RightMuxSelector;
}


int 
CGRA_Instruction::getReadRegAddress1()
{
    return ReadRegAddress1;
}


int 
CGRA_Instruction::getReadRegAddress2()
{
    return ReadRegAddress2;
}


int 
CGRA_Instruction::getWriteRegAddress()
{
    return WriteRegAddress;
}


bool 
CGRA_Instruction::getWriteRegisterEnable()
{
    return WriteRegisterEnable;
}


int 
CGRA_Instruction::getImmediateValue()
{
    return ImmediateValue;
}


bool 
CGRA_Instruction::getSelectDataMemoryAddressBus()
{
    return SelectDataMemoryAddressBus;
}


bool 
CGRA_Instruction::getSelectDataMemoryDataBus()
{
    return SelectDataMemoryDataBus;
}


void 
CGRA_Instruction::decodeInstruction()
{
    switch (((unsigned long)(InsWord & INS_DATATYPE))>>SHIFT_DATATYPE) {
      case character:
        DType = character;
        break;
      case int32:
        DType = int32;
        break;
      case int16:
        DType = int16;
        break;
      case float32:
        DType = float32;
        break;
      case float64:
        DType = float64;
        break;
      case float16:
        DType = float16;
        break;
      case empty1:
        DType = empty1;
        break;
      case empty2:
        DType = empty2;
        break;
    } // DType

    switch ((InsWord & INS_OPCODE) >> SHIFT_OPCODE) {
      case Add:
        opCode = Add;
        break;
      case Sub:
        opCode = Sub;
        break;
      case Mult:
        opCode = Mult;
        break;
      case AND:
        opCode = AND;
        break;
      case OR:
        opCode = OR;
        break;
      case XOR:
        opCode = XOR;
        break;
      case cgraASR:
        opCode = cgraASR;
        break;
      case cgraASL:
        opCode = cgraASL;
        break;
      case NOOP:
        opCode= NOOP;
        break;
      case GT:
        opCode= GT;
        break;
      case LT:
        opCode= LT;
        break;
      case EQ:
        opCode= EQ;
        break;
      case NEQ:
        opCode= NEQ;
        break;
      case Div:
        opCode= Div;
        break;
      case Rem:
        opCode= Rem;
        break;
      //case Sqrt:
      case LSHR:
        opCode= LSHR;
        break;
    } // opCode
  
    Predicator= (InsWord & INS_PREDICT )>>SHIFT_PREDICT;
    cond= (InsWord & INS_COND) >> SHIFT_COND;
  
    switch ((InsWord & INS_LMUX ) >> SHIFT_LMUX) {
      case Register:
        LeftMuxSelector = Register;
        break;
      case Left:
        LeftMuxSelector = Left;
        break;
      case Right:
        LeftMuxSelector = Right;
        break;
      case Up:
        LeftMuxSelector = Up;
        break;
      case Down:
        LeftMuxSelector = Down;
        break;
      case DataBus:
        LeftMuxSelector = DataBus;
        break;
      case Immediate:
        LeftMuxSelector = Immediate;
        break;
      case Self:
        LeftMuxSelector= Self;
        break;
    } // LMUX

    switch ((InsWord & INS_RMUX ) >> SHIFT_RMUX) {
      case Register:
        RightMuxSelector = Register;
        break;
      case Left:
        RightMuxSelector = Left;
        break;
      case Right:
        RightMuxSelector = Right;
        break;
      case Up:
        RightMuxSelector = Up;
        break;
      case Down:
        RightMuxSelector = Down;
        break;
      case DataBus:
        RightMuxSelector = DataBus;
        break;
      case Immediate:
        RightMuxSelector = Immediate;
        break;
      case Self:
        RightMuxSelector= Self;
        break;
    } //RMUX

    ReadRegAddress1= (InsWord & INS_R1 )>>SHIFT_R1 ;
    ReadRegAddress2= (InsWord & INS_R2 )>>SHIFT_R2 ;
    WriteRegAddress= (InsWord & INS_RW )>>SHIFT_RW ;
    WriteRegisterEnable= (InsWord & INS_WE )>>SHIFT_WE ;
    ImmediateValue= (InsWord & INS_IMMEDIATE) >> SHIFT_IMMEDIATE;
    SelectDataMemoryAddressBus= (InsWord & INS_AB )>>SHIFT_ABUS ;
    SelectDataMemoryDataBus= (InsWord & INS_DB )>>SHIFT_DBUS ;
}


void 
CGRA_Instruction::encodeInstruction()
{
    InsWord = 0;
    InsWord |= ((0UL | DType) << SHIFT_DATATYPE) & INS_DATATYPE;
    InsWord |= ((0UL | opCode) << SHIFT_OPCODE) & INS_OPCODE;
    InsWord |= ((0UL | Predicator) << SHIFT_PREDICT) & INS_PREDICT;
    InsWord |= ((0UL | cond) << SHIFT_COND) & INS_COND;
    InsWord |= ((0UL | LeftMuxSelector) << SHIFT_LMUX) & INS_LMUX;
    InsWord |= ((0UL | RightMuxSelector) << SHIFT_RMUX) & INS_RMUX;
    InsWord |= ((0UL | ReadRegAddress1) << SHIFT_R1) & INS_R1;
    InsWord |= ((0UL | ReadRegAddress2) << SHIFT_R2) & INS_R2;
    InsWord |= ((0UL | WriteRegAddress) << SHIFT_RW) & INS_RW;
    InsWord |= ((0UL | WriteRegisterEnable) << SHIFT_WE) & INS_WE;
    InsWord |= ((0UL | SelectDataMemoryAddressBus) << SHIFT_ABUS) & INS_AB;
    InsWord |= ((0UL | SelectDataMemoryDataBus) << SHIFT_DBUS) & INS_DB;
    InsWord |= ((0UL | ImmediateValue) << SHIFT_IMMEDIATE) & INS_IMMEDIATE;
}


/************************************Pred_Instruction************************************/
Pred_Instruction::Pred_Instruction()
{
}


Pred_Instruction::Pred_Instruction(unsigned long Instructionword)
{
    PredInsWord = Instructionword;
    decodePredInstruction();
}


Pred_Instruction::Pred_Instruction(Datatype dt, PredOPCode popc, PEInputMux LMuxSel, PEInputMux RMuxSel, 
        PEInputMux PMuxSel, int RRegAdd1,int RRegAdd2, int RRegAddP, int ImmVal)
{
    DType = dt;
    popCode = popc;
    LeftMuxSelector = LMuxSel;
    RightMuxSelector = RMuxSel;
    PredMuxSelector = PMuxSel;
    ReadRegAddress1 = RRegAdd1;
    ReadRegAddress2 = RRegAdd2;
    ReadRegAddressP = RRegAddP;
    ImmediateValue = ImmVal;
    encodePredInstruction();
}


Pred_Instruction::~Pred_Instruction()
{
}


unsigned long
Pred_Instruction::getPredInsWord()
{
    return PredInsWord;
}


Datatype 
Pred_Instruction::getDatatype()
{
    return DType;
}

PredOPCode 
Pred_Instruction::getPredOpCode()
{
    return popCode;
}


PEInputMux 
Pred_Instruction::getLeftMuxSelector()
{
    return LeftMuxSelector;
}


PEInputMux 
Pred_Instruction::getRightMuxSelector()
{
    return RightMuxSelector;
}


PEInputMux 
Pred_Instruction::getPredMuxSelector()
{
    return PredMuxSelector;
}


int 
Pred_Instruction::getReadRegAddress1()
{
    return ReadRegAddress1;
}


int 
Pred_Instruction::getReadRegAddress2()
{
    return ReadRegAddress2;
}


int 
Pred_Instruction::getReadRegAddressP()
{
    return ReadRegAddressP;
}


int 
Pred_Instruction::getImmediateValue()
{
    return ImmediateValue;
}


void 
Pred_Instruction::decodePredInstruction()
{
    switch(((unsigned long)(PredInsWord & INS_P_DATATYPE)) >> SHIFT_P_DATATYPE) {
      case character:
        DType = character;
        break;
      case int32:
        DType = int32;
        break;
      case int16:
        DType = int16;
        break;
      case float32:
        DType = float32;
        break;
      case float64:
        DType = float64;
        break;
      case float16:
        DType = float16;
        break;
      case empty1:
        DType = empty1;
        break;
      case empty2:
        DType = empty2;
        break;
    }
    switch((PredInsWord & INS_P_OPCODE) >> SHIFT_P_OPCODE) {
      case setConfigBoundry:
        popCode = setConfigBoundry;
        break;
      case LDi:
        popCode = LDi;
        break;
      case LDMi:
        popCode = LDMi;
        break;
      case LDUi:
        popCode = LDUi;
        break;
      case sel:
        popCode = sel;
        break;
      case address_generator:
        popCode = address_generator;
        break;
      case loopexit:
        popCode = loopexit;
        break;
      case nop:
        popCode = nop;
        break;
      case signExtend:
        popCode = signExtend;
        break;
    }

    switch((PredInsWord & INS_P_LMUX ) >> SHIFT_P_LMUX) {
      case Register:
        LeftMuxSelector = Register;
        break;
      case Left:
        LeftMuxSelector = Left;
        break;
      case Right:
        LeftMuxSelector = Right;
        break;
      case Up:
        LeftMuxSelector = Up;
        break;
      case Down:
        LeftMuxSelector = Down;
        break;
      case DataBus:
        LeftMuxSelector = DataBus;
        break;
      case Immediate:
        LeftMuxSelector = Immediate;
        break;
      case Self:
        LeftMuxSelector= Self;
        break;
    }
    switch((PredInsWord & INS_P_RMUX ) >> SHIFT_P_RMUX) {
      case Register:
        RightMuxSelector = Register;
        break;
      case Left:
        RightMuxSelector = Left;
        break;
      case Right:
        RightMuxSelector = Right;
        break;
      case Up:
        RightMuxSelector = Up;
        break;
      case Down:
        RightMuxSelector = Down;
        break;
      case DataBus:
        RightMuxSelector = DataBus;
        break;
      case Immediate:
        RightMuxSelector = Immediate;
        break;
      case Self:
        RightMuxSelector= Self;
        break;
    }
    switch((PredInsWord & INS_P_PMUX ) >> SHIFT_P_PMUX) {
      case Register:
        PredMuxSelector = Register;
        break;
      case Left:
        PredMuxSelector = Left;
        break;
      case Right:
        PredMuxSelector = Right;
        break;
      case Up:
        PredMuxSelector = Up;
        break;
      case Down:
        PredMuxSelector = Down;
        break;
      case DataBus:
        PredMuxSelector = DataBus;
        break;
      case Immediate:
        PredMuxSelector = Immediate;
        break;
      case Self:
        PredMuxSelector= Self;
        break;
    }

    ReadRegAddress1= (PredInsWord & INS_P_R1) >> SHIFT_P_R1;
    ReadRegAddress2= (PredInsWord & INS_P_R2) >> SHIFT_P_R2;
    ReadRegAddressP= (PredInsWord & INS_P_RP) >> SHIFT_P_RP;
    ImmediateValue= (PredInsWord & INS_P_IMMEDIATE) >> SHIFT_P_IMMEDIATE;
}


void 
Pred_Instruction::encodePredInstruction()
{
    PredInsWord = 0;
    PredInsWord |= ((0UL | DType) << SHIFT_P_DATATYPE) & INS_P_DATATYPE;
    PredInsWord |= ((0UL | popCode) << SHIFT_P_OPCODE) & INS_P_OPCODE;
    PredInsWord |= ((0UL | 1UL) << SHIFT_PREDICT) & INS_PREDICT;
    PredInsWord |= ((0UL | 0UL) << SHIFT_COND) & INS_COND;
    PredInsWord |= ((0UL | LeftMuxSelector) << SHIFT_P_LMUX) & INS_P_LMUX;
    PredInsWord |= ((0UL | RightMuxSelector) << SHIFT_P_RMUX) & INS_P_RMUX;
    PredInsWord |= ((0UL | ReadRegAddress1) << SHIFT_P_R1) & INS_P_R1;
    PredInsWord |= ((0UL | ReadRegAddress2) << SHIFT_P_R2) & INS_P_R2;
    PredInsWord |= ((0UL | ReadRegAddressP) << SHIFT_P_RP) & INS_P_RP;
    PredInsWord |= ((0UL | PredMuxSelector) << SHIFT_P_PMUX) & INS_P_PMUX;
    PredInsWord |= ((0UL | ImmediateValue) << SHIFT_P_IMMEDIATE) & INS_P_IMMEDIATE;
}



/************************************Cond_Instruction************************************/
Cond_Instruction::Cond_Instruction()
{
}


Cond_Instruction::Cond_Instruction(unsigned long Instructionword)
{
    condInsWord = Instructionword;
    decodeCondInstruction();
}


Cond_Instruction::Cond_Instruction(Datatype dt, CondOpCode opc, PEInputMux LMuxSel, PEInputMux RMuxSel,
        int RRegAdd1,int RRegAdd2, int WAdd, bool WE, int ImmVal, int BranchOffset, 
        bool splitDirectionBit, bool loopExitEn) 
{
    DType = dt;
    condOpCode = opc;
    splitDirection = splitDirectionBit;
    loopExitEnable = loopExitEn;
    LeftMuxSelector =LMuxSel;
    RightMuxSelector = RMuxSel;
    ReadRegAddress1 = RRegAdd1;
    ReadRegAddress2 = RRegAdd2;
    WriteRegAddress = WAdd;
    WriteRegisterEnable = WE;
    branchOffset = BranchOffset;
    ImmediateValue = ImmVal;
    encodeCondInstruction();
}


Cond_Instruction::~Cond_Instruction() 
{
}


Datatype 
Cond_Instruction::getDatatype() 
{
    return DType;
}


CondOpCode 
Cond_Instruction::getOpCode() 
{
    return condOpCode;
}


PEInputMux 
Cond_Instruction::getLeftMuxSelector()
{
    return LeftMuxSelector;
}


PEInputMux 
Cond_Instruction::getRightMuxSelector()
{
    return RightMuxSelector;
}


int 
Cond_Instruction::getReadRegAddress1()
{
    return ReadRegAddress1;
}


int 
Cond_Instruction::getReadRegAddress2()
{
    return ReadRegAddress2;
}


int 
Cond_Instruction::getWriteRegAddress()
{
    return WriteRegAddress;
}


bool 
Cond_Instruction::getWriteRegisterEnable()
{
    return WriteRegisterEnable;
}


unsigned 
Cond_Instruction::getBranchOffset()
{
    return branchOffset;
}


int 
Cond_Instruction::getImmediateValue()
{
    return ImmediateValue;
}


bool
Cond_Instruction::getSplitDirection()
{
    return splitDirection;
}


bool
Cond_Instruction::getloopExitEnable()
{
    return loopExitEnable;
}


void 
Cond_Instruction::decodeCondInstruction()
{
    switch(((unsigned long)(condInsWord & INS_C_DATATYPE)) >> SHIFT_C_DATATYPE) {
      case character:
        DType = character;
        break;
      case int32:
        DType = int32;
        break;
      case int16:
        DType = int16;
        break;
      case float32:
        DType = float32;
        break;
      case float64:
        DType = float64;
        break;
      case float16:
        DType = float16;
        break;
      case empty1:
        DType = empty1;
        break;
      case empty2:
        DType = empty2;
        break;
    }

    switch((condInsWord & INS_C_OPCODE) >> SHIFT_C_OPCODE) {
      case CMPGT:
        condOpCode= CMPGT;
        break;
      case CMPLT:
        condOpCode= CMPLT;
        break;
      case CMPEQ:
        condOpCode= CMPEQ;
        break;
      case CMPNEQ:
        condOpCode= CMPNEQ;
        break;
      case CAND:
        condOpCode = CAND;
        break;
      case COR:
        condOpCode = COR;
        break;
      case CXOR:
        condOpCode = CXOR;
        break;
    }

    switch((condInsWord & INS_C_LMUX ) >> SHIFT_C_LMUX) {
      case Register:
        LeftMuxSelector = Register;
        break;
      case Left:
        LeftMuxSelector = Left;
        break;
      case Right:
        LeftMuxSelector = Right;
        break;
      case Up:
        LeftMuxSelector = Up;
        break;
      case Down:
        LeftMuxSelector = Down;
        break;
      case DataBus:
        LeftMuxSelector = DataBus;
        break;
      case Immediate:
        LeftMuxSelector = Immediate;
        break;
      case Self:
        LeftMuxSelector= Self;
        break;
    }

    switch((condInsWord & INS_C_RMUX ) >> SHIFT_C_RMUX) {
      case Register:
        RightMuxSelector = Register;
        break;
      case Left:
        RightMuxSelector = Left;
        break;
      case Right:
        RightMuxSelector = Right;
        break;
      case Up:
        RightMuxSelector = Up;
        break;
      case Down:
        RightMuxSelector = Down;
        break;
      case DataBus:
        RightMuxSelector = DataBus;
        break;
      case Immediate:
        RightMuxSelector = Immediate;
        break;
      case Self:
        RightMuxSelector= Self;
        break;
    }

    ReadRegAddress1 = (condInsWord & INS_C_R1) >> SHIFT_C_R1;
    ReadRegAddress2 = (condInsWord & INS_C_R2) >> SHIFT_C_R2;
    WriteRegAddress = (condInsWord & INS_C_RW) >> SHIFT_C_RW;
    WriteRegisterEnable = (condInsWord & INS_C_WE) >> SHIFT_C_WE;
    branchOffset = (condInsWord & INS_C_BROFFSET) >> SHIFT_C_BROFFSET;
    ImmediateValue = (condInsWord & INS_C_IMMEDIATE) >> SHIFT_C_IMMEDIATE;
    splitDirection = (condInsWord & INS_C_SP) >> SHIFT_C_SP;
    loopExitEnable = (condInsWord & INS_C_LE) >> SHIFT_C_LE;
}


void
Cond_Instruction::encodeCondInstruction()
{
    condInsWord = 0;
    condInsWord |= ((0UL | DType) << SHIFT_C_DATATYPE) & INS_C_DATATYPE;
    condInsWord |= ((0UL | condOpCode) << SHIFT_C_OPCODE) & INS_C_OPCODE;
    condInsWord |= ((0UL | splitDirection) << SHIFT_C_SP) & INS_C_SP;
    condInsWord |= ((0UL | loopExitEnable) << SHIFT_C_LE) & INS_C_LE;
    condInsWord |= ((0UL | 1UL) << SHIFT_COND) & INS_COND;
    condInsWord |= ((0UL | LeftMuxSelector) << SHIFT_C_LMUX) & INS_C_LMUX;
    condInsWord |= ((0UL | RightMuxSelector) << SHIFT_C_RMUX) & INS_C_RMUX;
    condInsWord |= ((0UL | ReadRegAddress1) << SHIFT_C_R1) & INS_C_R1;
    condInsWord |= ((0UL | ReadRegAddress2) << SHIFT_C_R2) & INS_C_R2;
    condInsWord |= ((0UL | WriteRegAddress) << SHIFT_C_RW) & INS_C_RW;
    condInsWord |= ((0UL | WriteRegisterEnable) << SHIFT_C_WE) & INS_C_WE;
    condInsWord |= ((0UL | branchOffset) << SHIFT_C_BROFFSET) & INS_C_BROFFSET;
    condInsWord |= ((0UL | ImmediateValue) << SHIFT_C_IMMEDIATE) & INS_C_IMMEDIATE;
}

