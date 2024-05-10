/*
 * PE.cpp
 *
/ * Created on: May 24, 2011
 * Author: mahdi
 *
 * Last edited: 30 March 2018
 * Author: Shail Dave
 *
 * Last edited: 10 July 2019
 * Author: Mahesh Balasubramanian
 * Update: Parameterized Rfsize and CGRA dims
 *
 */

#include "CGRAPE.h"
#include <iomanip>
#include <stdio.h>
#include <iostream>

CGRA_PE::CGRA_PE()
{
    DPRINTF(PE_DEBUG, "CGRAPE constructor1\n");
    Output=0;
}


CGRA_PE::~CGRA_PE()
{
}


void 
CGRA_PE::setRF_per_PE(int rs)
{
    DPRINTF(PE_DEBUG, "Setting up RF of size %d for each PE\n", rs); 
    RegFile.setRF(rs); 
    DPRINTF(PE_DEBUG, "Exiting setRF_per_PE()\n");
}


void 
CGRA_PE::setFPRF_per_PE(int rs)
{
    DPRINTF(PE_DEBUG, "Setting up FPRF of size %d for each PE\n", rs);
    FPRegFile.setFPRF(rs);
    DPRINTF(PE_DEBUG, "Exiting setFPRF_per_PE()\n");
}


int 
CGRA_PE::getOutput()
{
    DPRINTF(PE_DEBUG, "Inside getOutput()\n");
    return Output;
}


float 
CGRA_PE::getFPOutput()
{
    DPRINTF(PE_DEBUG, "Inside getFPOutput()\n");
    return FPOutput;
}


bool 
CGRA_PE::getPredOutput()
{
    DPRINTF(PE_DEBUG, "Inside getFPOutput()\n");
    return OutputP;
}


void 
CGRA_PE::Fetch(CGRA_Instruction* ins)
{
    DPRINTF(PE_DEBUG, "Inside Fetch()\n");
    //delete ins;
    this->ins = ins;
    DPRINTF(PE_DEBUG, "Exiting Fetch()\n");
}


Datatype 
CGRA_PE::GetDatatype()
{
    return dt; 
}


bool 
CGRA_PE::isNOOP()
{
    return ins->getOpCode() == NOOP;
}


void 
CGRA_PE::Decode()
{
    DPRINTF(PE_DEBUG, "Inside Decode()\n");
    dt = ins->getDatatype();
    if (dt == character || dt == int16 || dt == int32) {
        DPRINTF(CGRA_Detailed,"LMUX: %u, RMUX: %u\n",ins->getLeftMuxSelector(),ins->getRightMuxSelector());
        switch (ins->getLeftMuxSelector()) {
          case Register:
            DPRINTF(CGRA_Detailed,"\n******** DB INPUT1 ReadAddress = %d************\n",ins->getReadRegAddress1());
            Input1 = RegFile.Read(ins->getReadRegAddress1());
            break;
          case Left:
            Input1 = (*leftIn);
            break;
          case Right:
            Input1 = *(rightIn);
            break;
          case Up:
            Input1 = *(upIn);
            break;
          case Down:
            Input1 = *(downIn);
            break;
          case DataBus:
            Input1 = *(dataBs);
            DPRINTF(CGRA_Detailed,"\n******** DB INPUT1 = %d************\n",Input1);
            break;
          case Immediate:
            Input1=ins->getImmediateValue();
            break;
          case Self:
            Input1=Output;
            break;
          default:
            throw new CGRAException("CGRA Left Mux selector out of range");
        } // end of ins->getLeftMuxSelector()

        switch (ins->getRightMuxSelector()) {
          case Register:
            DPRINTF(CGRA_Detailed,"\n******** DB INPUT2 ReadAddress = %d************\n",ins->getReadRegAddress2());
            Input2 = RegFile.Read(ins->getReadRegAddress2());
            break;
          case Left:
            Input2 = (*leftIn);
            break;
          case Right:
            Input2 = *(rightIn);
            break;
          case Up:
            Input2 = *(upIn);
            break;
          case Down:
            Input2 = *(downIn);
            break;
          case DataBus:
            Input2 = *(dataBs);
            DPRINTF(CGRA_Detailed,"\n******** DB INPUT2 = %d************\n",(int)Input2);
            break;
          case Immediate:
            Input2=ins->getImmediateValue();
            break;
          case Self:
            Input2=Output;
            break;
          default:
            throw new CGRAException("CGRA Right Mux selector out of range");
        } // end of ins->getRightMuxSelector()

        if ((ins->getPredicator() == 1) && (ins->getCond() == 0)) {
            Pred_Instruction *predIns = new Pred_Instruction(ins->getInsWord());
            switch (predIns->getPredMuxSelector()) {
              case Register:
                InputP = (bool)RegFile.Read(predIns->getReadRegAddressP());
                break;
              case Left:
                InputP = (*leftIn);
                break;
              case Right:
                InputP = (*rightIn);
                break;
              case Up:
                InputP = (*upIn);
                break;
              case Down:
                InputP = (*downIn);
                break;
              case Immediate:
                InputP=(ins->getImmediateValue()==1)? true : false;
                break;
              case Self:
                InputP=(OutputP);
                break;
              default:
                throw new CGRAException("CGRA Pred Mux selector out of range");
            }
        } // end of predicate instruction
    } else {  // end of if data type is integer
        // here, an exception is address generator, where left mux should use int input
        if ((ins->getPredicator() == 1) && (ins->getCond() == 0)) {
            Pred_Instruction *predIns = new Pred_Instruction(ins->getInsWord());
            if (predIns->getPredOpCode() == address_generator) {
                switch (ins->getLeftMuxSelector()) {
                  case Register:
                    DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT1 ReadAddress%d************\n",ins->getReadRegAddress1());
                    Input1 = RegFile.Read(ins->getReadRegAddress1()); // Inspect me: should I be RegFile or FPRegFile?
                    break;
                  case Left:
                    Input1 = (*leftIn);
                    break;
                  case Right:
                    Input1 = *(rightIn);
                    break;
                  case Up:
                    Input1 = *(upIn);
                    break;
                  case Down:
                    Input1 = *(downIn);
                    break;
                  case DataBus:
                    Input1 = *(dataBs); // Inspect me: should I be dataBs or FdataBs?
                    DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT1 %f************\n",Input1);  
                    break;
                  case Immediate:
                    Input1=ins->getImmediateValue();
                    break;
                  case Self:
                    Input1=Output;
                    break;
                  default:
                    throw new CGRAException("CGRA Left Mux selector out of range");
                }
            } else { // end of address generator
                switch (ins->getLeftMuxSelector()) {
                  case Register:
                    DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT1 ReadAddress %d************\n",ins->getReadRegAddress1());
                    FPInput1 = FPRegFile.Read(ins->getReadRegAddress1());
                    break;
                  case Left:
                    FPInput1 = (*FPleftIn);
                    break;
                  case Right:
                    FPInput1 = *(FPrightIn);
                    break;
                  case Up:
                    FPInput1 = *(FPupIn);
                    break;
                  case Down:
                    FPInput1 = *(FPdownIn);
                    break;
                  case DataBus:
                    FPInput1 = *(FdataBs);
                    DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT1 %f************\n",Input1);
                    break;
                  case Immediate:
                    FPInput1=ins->getImmediateValue();
                    break;
                  case Self:
                    FPInput1=FPOutput;
                    break;
                  default:
                    throw new CGRAException("CGRA Left Mux selector out of range");
                }
            } // end for left mux
            switch (ins->getRightMuxSelector()) {
              case Register:
                DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT2 ReadAddress%d************\n",ins->getReadRegAddress2());
                FPInput2 = FPRegFile.Read(ins->getReadRegAddress2());
                break;
              case Left:
                FPInput2 = (*FPleftIn);
                break;
              case Right:
                FPInput2 = *(FPrightIn);
                break;
              case Up:
                FPInput2 = *(FPupIn);
                break;
              case Down:
                FPInput2 = *(FPdownIn);
                break;
              case DataBus:
                FPInput2 = *(FdataBs);
                DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT2 %f************\n",FPInput2);
                break;
              case Immediate:
                FPInput2=(float)(ins->getImmediateValue());
                break;
              case Self:
                FPInput2=FPOutput;
                break;
              default:
                throw new CGRAException("CGRA Right Mux selector out of range");
            }

            switch (predIns->getPredMuxSelector()) {
              case Register:
                InputP = (bool)(FPRegFile.Read(predIns->getReadRegAddressP()));
                //DPRINTF(CGRA_Detailed,"\n******** INPUTP Reg %f************\n",(FPRegFile.Read(predIns->getReadRegAddressP())));
                break;
              case Left:
                InputP = (*PredleftIn);
                //DPRINTF(CGRA_Detailed,"\n******** INPUTP Left %f************\n",(*FPleftIn));
                break;
              case Right:
                InputP = (*PredrightIn);
                //DPRINTF(CGRA_Detailed,"\n******** INPUTP Right %f************\n",(*FPrightIn));
                break;
              case Up:
                InputP = (*PredupIn);
                //DPRINTF(CGRA_Detailed,"\n******** INPUTP up %f************\n",(*FPupIn));
                break;
              case Down:
                InputP = (*PreddownIn);
                //DPRINTF(CGRA_Detailed,"\n******** INPUTP down %f************\n",(*FPdownIn));
                break;
              case Immediate:
                InputP=(ins->getImmediateValue()==1) ? true: false;
                break;
              case Self:
                InputP=(OutputP);
                //DPRINTF(CGRA_Detailed,"\n******** INPUTP FPOUTPUT %f************\n",(FPOutput));
                break;
              default:
                throw new CGRAException("CGRA Pred Mux selector out of range");
            }
        } // end of pred ins
        switch (ins->getLeftMuxSelector()) {
          case Register:
            DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT1 ReadAddress %d************\n",ins->getReadRegAddress1());
            FPInput1 = FPRegFile.Read(ins->getReadRegAddress1());
            break;
          case Left:
            FPInput1 = (*FPleftIn);
            break;
          case Right:
            FPInput1 = *(FPrightIn);
            break;
          case Up:
            FPInput1 = *(FPupIn);
            break;
          case Down:
            FPInput1 = *(FPdownIn);
            break;
          case DataBus:
            FPInput1 = *(FdataBs);
            DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT1 %f************\n",Input1);
            break;
          case Immediate:
            FPInput1=ins->getImmediateValue();
            break;
          case Self:
            FPInput1=FPOutput;
            break;
          default:
            throw new CGRAException("CGRA Left Mux selector out of range");
        }
        switch (ins->getRightMuxSelector()) {
          case Register:
            DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT2 ReadAddress%d************\n",ins->getReadRegAddress2());
            FPInput2 = FPRegFile.Read(ins->getReadRegAddress2());
            break;
          case Left:
            FPInput2 = (*FPleftIn);
            break;
          case Right:
            FPInput2 = *(FPrightIn);
            break;
          case Up:
            FPInput2 = *(FPupIn);
            break;
          case Down:
            FPInput2 = *(FPdownIn);
            break;
          case DataBus:
            FPInput2 = *(FdataBs);
            DPRINTF(CGRA_Detailed,"\n******** DB FPINPUT2 %f************\n",FPInput2);
            break;
          case Immediate:
            FPInput2=(float)(ins->getImmediateValue());
            break;
          case Self:
            FPInput2=FPOutput;
            break;
          default:
            throw new CGRAException("CGRA Right Mux selector out of range");
        }
    } // end of FP type  
    DPRINTF(PE_DEBUG, "Exiting Decode()\n");
}


void 
CGRA_PE::IExecute()
{
    DPRINTF(PE_DEBUG, "Inside IExecute()\n");
    bool predicateBit = ins->getPredicator();
    bool condBit = ins->getCond();  

    if (!predicateBit && !condBit) {
        // normal instruction
        OPCode insOpcode = ins->getOpCode();
        switch (insOpcode) {
          case Add:
            Output=Input1+Input2;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** SUM IN THIS PE %d************\n",Output);
            break;
          case Sub:
            Output=Input1-Input2;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** SUBTRACTION IN THIS PE %d************\n",Output);
            break;
          case Mult:
            Output=Input1*Input2;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** PRODUCT IN THIS PE %d************\n",Output);
            break;
          case AND:
            Output=Input1&Input2;
            OutputP=(bool)(Input1&Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** AND IN THIS PE %d************\n",Output);
            break;
          case OR:
            Output=(Input1|Input2);
            OutputP=(bool)(Input1|Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** OR IN THIS PE %d************\n",Output);
            break;
          case XOR:
            Output=(Input1^Input2);
            OutputP=(bool)(Input1^Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** XOR IN THIS PE %d************\n",Output);
            break;
          case cgraASR:
            Output=(Input1>>Input2);
            OutputP=(bool)(Input1>>Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** ASR IN THIS PE %d************\n",Output);
            break;
          case cgraASL:
            Output=(Input1<<Input2);
            OutputP=(bool)(Input1<<Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** ASL IN THIS PE %d************\n",Output);
            break;
          case GT:
            Output=Input1>Input2;
            OutputP=(bool)(Input1>Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Greater Than IN THIS PE %d************\n",OutputP);
            break;
          case LT:
            Output=Input1<Input2;
            OutputP=(bool)(Input1<Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Less Than IN THIS PE %d************\n",OutputP);
            break;
          case EQ:
            Output=Input1==Input2;
            OutputP=(bool)(Input1==Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** EQUALS IN THIS PE %d************\n",OutputP);
            break;
          case NEQ:
            Output=Input1!=Input2;
            OutputP=(bool)(Input1!=Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** NOT EQUALS IN THIS PE %d************\n",OutputP);
            break;
          case Div:
            Output=Input1/Input2;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** DIV IN THIS PE %d************\n",Output);
            break;
          case Rem:
            Output=Input1%Input2;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** REM IN THIS PE %d************\n",Output);
            break;
          case LSHR: {
            unsigned unsignedInput1 = (unsigned) Input1;
            Output = (unsigned)(unsignedInput1 >> (unsigned) Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\nunsignedInput1 = %d\tInput2 = %d\n",unsignedInput1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** LSHR IN THIS PE %d************\n",Output);
            DPRINTF(CGRA_Detailed,"\n******** LSHR IN THIS PE %u************\n",Output);
            break;
          }
          case NOOP:
            DPRINTF(CGRA_Detailed,"CGRA: NOOP\n");
            break;
          default:
            DPRINTF(CGRA_Detailed," 1. Opcode is %ld\n",(unsigned) insOpcode);
            throw new CGRAException("Unknown CGRA Opcode");
        }

        if (ins->getWriteRegisterEnable()) {
            int writeRegisterNumber = ins->getWriteRegAddress();
            RegFile.Write(writeRegisterNumber,Output);
            FPRegFile.Write(writeRegisterNumber,(float)Output);
            DPRINTF(CGRA_Detailed,"Writing output %d to register %d\n",Output,writeRegisterNumber);
        }

        if (ins->getSelectDataMemoryDataBus()) {
            DPRINTF(CGRA_Detailed,"\n******** DB Output %d************\n",Output);
            (*dataBs) = Output;
            (*BsDatatype) = CGRA_MEMORY_INT;
        }

    } else if (predicateBit && !condBit) {
        //pred instruction
        Pred_Instruction* predIns = new Pred_Instruction(ins->getInsWord());
        PredOPCode insOpCode = predIns->getPredOpCode();
        switch (insOpCode) { 
          case setConfigBoundry:
            RegFile.config_boundary = Input1;
            FPRegFile.config_boundary = Input1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Setting Configuration as %d************\n",Input1);
            break;
          case LDi:
            Output=Input1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** LDI IN THIS PE %d************\n",Output);
            break;
          case LDMi:
            Output=Input1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** LDMI IN THIS PE %d************\n",Output);
            break;
          case LDUi:
            Output=Input1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** LDUI IN THIS PE %d************\n",Output);
            break;
          case sel:
            Output = (InputP == true) ? Input1 : Input2;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\tInputP = %d\n",Input1, Input2, (int)InputP);
            DPRINTF(CGRA_Detailed,"\n******** Selection IN THIS PE %d************\n",Output);
            break;
          case loopexit:
            Output = ((Input1 == 1) && (Input2 == 0));
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Loop Exit Control IN THIS PE %d************\n",Output);
            break;
          case address_generator:
            Output=Input1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** ADDRESS GENERATED IN THIS PE %d************\n",Output);
            break;
          case signExtend: {
            bool maskedBit = (Input1 & (1 << (Input2-1)));
            int shiftAmount = ((1 << Input2)-1);
            int signExtendMask = 0xFFFFFFFF - shiftAmount;
            Output=Input1 & shiftAmount;
            Output = (maskedBit == 1) ? (Output + signExtendMask) : Output;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** SIGN EXTENDED IN THIS PE %d************\n",Output);
            break;
          }
          default:
            DPRINTF(CGRA_Detailed,"2. Opcode is %ld\n",(unsigned) insOpCode);
            throw new CGRAException("Unknown CGRA Opcode");
        }

        // here the LD instructions are more like normal instructions
        // TODO: Fix Micro-architecture for P-type (PredMux) rather than controlling write enable.
        if ((insOpCode==LDi) || (insOpCode==LDMi) || (insOpCode==LDUi)) {
            if (ins->getWriteRegisterEnable()) {
                DPRINTF(CGRA_Detailed,"\n************** WE *****************\n");
                int writeRegisterNumber = ins->getWriteRegAddress();
                if (insOpCode == LDMi) {
                    Output= (Output << 12) | (RegFile.Read(writeRegisterNumber));
                } else if (insOpCode == LDUi) {
                    Output=((int)Output << 24) | ( (int)RegFile.Read(writeRegisterNumber));
                }

                if ((insOpCode==LDi) || (insOpCode==LDMi) || (insOpCode==LDUi)) {
                    RegFile.Write(writeRegisterNumber,Output);
                    FPRegFile.Write(writeRegisterNumber,(float)Output);
                    DPRINTF(CGRA_Detailed,"Writing output %d to register %d\n",Output,writeRegisterNumber);
                }
            }
        }
        // same herre, PMUX is not used
        // address generstor provides address for a save or load at the same row
        if (insOpCode == address_generator) {
            if (ins->getSelectDataMemoryAddressBus()) {
                DPRINTF(CGRA_Detailed,"\n*********Setting Address %lx ******\n",(unsigned int)Output);
                (*addressBs) = (unsigned int)Output;
                (*BsStatus) = CGRA_MEMORY_READ;
                (*BsDatatype) = CGRA_MEMORY_INT; 
                (*alignmentBs) = Input2;
            }
        }
    } else { // end of predication_bit
        // now for cond instructions
        Cond_Instruction* condIns = new Cond_Instruction(ins->getInsWord());
        bool leBit = condIns->getloopExitEnable();
        CondOpCode insOpcode = condIns->getOpCode();
        switch (insOpcode) {
          case CMPGT:
            Output=Input1>Input2;
            OutputP=(bool)(Input1>Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Greater Than IN THIS PE %d************\n",OutputP);
            break;
          case CMPLT:
            Output=Input1<Input2;
            OutputP=(bool)(Input1<Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Less Than IN THIS PE %d************\n",OutputP);
            break;
          case CMPEQ:
            Output=Input1==Input2;
            OutputP=(bool)(Input1==Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** EQUALS IN THIS PE %d************\n",OutputP);
            break;
          case CMPNEQ:
            Output=Input1!=Input2;
            OutputP=(bool)(Input1!=Input2);
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** NOT EQUALS IN THIS PE %d************\n",OutputP);
            break;
          case CAND:
            Output=Input1&Input2;
            OutputP=(bool)(Input1&Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** CAND IN THIS PE %d************\n",Output);
            break;
          case COR:
            Output=(Input1|Input2);
            OutputP=(bool)(Input1|Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** COR IN THIS PE %d************\n",Output);
            break;
          case CXOR:
            Output=(Input1^Input2);
            OutputP=(bool)(Input1^Input2); 
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** CXOR IN THIS PE %d************\n",Output);
            break;
          default:
            DPRINTF(CGRA_Detailed," 1. Opcode is %ld\n",(unsigned) insOpcode);
            throw new CGRAException("Unknown CGRA Opcode");
        }

        if (ins->getWriteRegisterEnable()) {
            int writeRegisterNumber = ins->getWriteRegAddress();
            RegFile.Write(writeRegisterNumber,Output);
            FPRegFile.Write(writeRegisterNumber,(float)Output);
            DPRINTF(CGRA_Detailed,"Writing output %d to register %d\n",Output,writeRegisterNumber);
        }

        if (leBit) {
            bool leDest = condIns->getSplitDirection();
            DPRINTF(CGRA_Execute, "LE Branch Destination: %d\n",leDest);
            // for loop exit
            this->Controller_Reg = !(Output == leDest);  // Output == LE_dest to exit
            DPRINTF(CGRA_Detailed, "Set controller reg to %d\n", !((bool)Output == leDest));
        } // end of loop exit
    }

    DPRINTF(CGRA_Detailed,"Distance is: %d\n",RegFile.distance);
    DPRINTF(PE_DEBUG, "Exiting Execute()\n");
}


void 
CGRA_PE::FExecute()
{
    DPRINTF(PE_DEBUG, "Inside FExecute()\n");
    bool predicateBit = ins->getPredicator();
    bool condBit = ins->getCond();  

    if (!predicateBit && !condBit) {
        // normal ins
        OPCode insOpcode = ins->getOpCode();
        switch (insOpcode) {
          case Add:
            FPOutput=FPInput1+FPInput2;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** SUM IN THIS PE %f************\n",FPOutput);
            break;
          case Sub:
            FPOutput=FPInput1-FPInput2;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** SUBTRACTION IN THIS PE %f************\n",FPOutput);
            break;
          case Mult:
            FPOutput=FPInput1*FPInput2;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** PRODUCT IN THIS PE %f************\n",FPOutput);
            break;
          case AND:
            FPOutput=((int)FPInput1&(int)FPInput2);
            OutputP=(bool)((int)FPInput1&(int)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** AND IN THIS PE %f************\n",FPOutput);
            break;
          case OR:
            FPOutput=((int)FPInput1|(int)FPInput2);
            OutputP=(bool)((int)FPInput1|(int)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** OR IN THIS PE %f************\n",FPOutput);
            break;
          case XOR:
            FPOutput=((int)FPInput1^(int)FPInput2);
            OutputP=(bool)((int)FPInput1^(int)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** XOR IN THIS PE %f************\n",FPOutput);
            break;
          case cgraASR:
            FPOutput=((int)FPInput1>>(int)FPInput2);
            OutputP=(bool)((int)FPInput1>>(int)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** ASR IN THIS PE %f************\n",FPOutput);
            break;
          case cgraASL:
            FPOutput=((int)FPInput1<<(int)FPInput2);
            OutputP=(bool)((int)FPInput1<<(int)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** ASL IN THIS PE %f************\n",FPOutput);
            break;
          case GT:
            FPOutput=(float)FPInput1>(float)FPInput2;
            OutputP=(bool)((float)FPInput1>(float)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** Greater Than IN THIS PE %f************\n",OutputP);
            break;
          case LT:
            FPOutput=FPInput1<FPInput2;
            OutputP=(bool)(FPInput1<FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** Less Than IN THIS PE %f************\n",OutputP);
            break;
          case EQ:
            FPOutput=FPInput1==FPInput2;
            OutputP=(bool)(FPInput1==FPInput2); 
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** EQUALS IN THIS PE %f************\n",OutputP);
            break;
          case NEQ:
            FPOutput=FPInput1!=FPInput2;
            OutputP=(bool)(FPInput1!=FPInput2); 
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** NOT EQUALS IN THIS PE %f************\n",OutputP);
            break;
          case Div:
            FPOutput=FPInput1/FPInput2;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** DIV IN THIS PE %f************\n",FPOutput);
            break;
          case Rem:
            FPOutput=(int)FPInput1%(int)FPInput2;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** REM IN THIS PE %f************\n",FPOutput);
            break;
          case LSHR: {
            unsigned unsignedFPInput1 = (unsigned) FPInput1;
            FPOutput = (unsigned)(unsignedFPInput1 >> (unsigned) FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\nunsignedFPInput1 = %f\tFPInput2 = %f\n",unsignedFPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** LSHR IN THIS PE %f************\n",FPOutput);
            DPRINTF(CGRA_Detailed,"\n******** LSHR IN THIS PE %u************\n",FPOutput);
            break;
          }
          case NOOP:
            DPRINTF(CGRA_Detailed,"CGRA: NOOP.Execute()\n");
            break;
          default:
            DPRINTF(CGRA_Detailed," 1. Opcode is %ld\n",(unsigned) insOpcode);
            throw new CGRAException("Unknown CGRA Opcode");
        }

        if (ins->getWriteRegisterEnable()) {
            //DPRINTF(CGRA_Detailed,"\n************** WE *****************\n");
            int writeRegisterNumber = ins->getWriteRegAddress();

            RegFile.Write(writeRegisterNumber,(int)FPOutput);
            FPRegFile.Write(writeRegisterNumber,FPOutput);
            DPRINTF(CGRA_Detailed,"Writing output %f to register %d\n",FPOutput,writeRegisterNumber);
        }

        if (ins->getSelectDataMemoryDataBus()) {
            DPRINTF(CGRA_Detailed,"\n******** DB FPOutput %f************\n",FPOutput);
            (*FdataBs) = FPOutput;
            (*BsDatatype) = CGRA_MEMORY_FP;
        }
    } else if (predicateBit && !condBit) { //end of normal ins
        // pred ins
        Pred_Instruction* predIns = new Pred_Instruction(ins->getInsWord());
        PredOPCode insOpCode = predIns->getPredOpCode();
        switch (insOpCode) { //previously predIns->getPredOpCode().
          case setConfigBoundry:
            RegFile.config_boundary = (int)FPInput1;
            FPRegFile.config_boundary = (int)FPInput1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %f\tInput2 = %f\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\n******** Setting Configuration as %f************\n",Input1);
            break;
          case LDi:
            FPOutput=FPInput1;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** LDI IN THIS PE %f************\n",FPOutput);
            break;
          case LDMi:
            FPOutput=FPInput1;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** LDMI IN THIS PE %f************\n",FPOutput);
            break;
          case LDUi:
            FPOutput=FPInput1;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** LDUI IN THIS PE %f************\n",FPOutput);
            break;
          case sel:
            FPOutput = (InputP == true) ? FPInput2 : FPInput1;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\tInputP = %d\n",FPInput1, FPInput2, (int)InputP);
            DPRINTF(CGRA_Detailed,"\n******** Selection IN THIS PE %f************\n",FPOutput);
            break;
          case loopexit:
            FPOutput = ((FPInput1 == 1) && (FPInput2 == 0));
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\tInputP = %f\n",FPInput1, FPInput2, InputP);
            DPRINTF(CGRA_Detailed,"\n******** Loop Exit Control IN THIS PE %f************\n",FPOutput);
            break;
          case address_generator:
            Output=(int)Input1;
            DPRINTF(CGRA_Detailed,"\nInput1 = %d\tInput2 = %d\n",Input1, Input2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** ADDRESS GENERATED IN THIS PE %d************\n",Output);
            break;
          case signExtend: {
            bool maskedBit = ((int)FPInput1 & (1 << ((int)FPInput2-1)));
            int shiftAmount = ((1 << (int)FPInput2)-1);
            int signExtendMask = 0xFFFFFFFF - shiftAmount;
            FPOutput=(int)FPInput1 & shiftAmount;
            FPOutput = (maskedBit == 1) ? ((int)FPOutput + signExtendMask) :(int) FPOutput;
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** SIGN EXTENDED IN THIS PE %f************\n",FPOutput);
            break;
          }
          default:
            DPRINTF(CGRA_Detailed,"2. Opcode is %ld\n",(unsigned) insOpCode);
            throw new CGRAException("Unknown CGRA Opcode");
        }

        if ((insOpCode==LDi) || (insOpCode==LDMi) || (insOpCode==LDUi)) {
            if (ins->getWriteRegisterEnable()) {
                DPRINTF(CGRA_Detailed,"\n************** WE FP*****************\n");
                int writeRegisterNumber = ins->getWriteRegAddress();
                if (insOpCode==LDMi) {
                    FPOutput= ((int)FPOutput << 12) | ((int)FPRegFile.Read(writeRegisterNumber));
                } else if (insOpCode==LDUi) {
                    FPOutput=((int)FPOutput<<24) | ( (int)FPRegFile.Read(writeRegisterNumber));
                }

                //TODO: Fix Micro-architecture for P-type (PredMux) rather than controlling write enable.
                if ((insOpCode==LDi) || (insOpCode==LDMi)) {
                    RegFile.Write(writeRegisterNumber,(int)FPOutput);
                    FPRegFile.Write(writeRegisterNumber,FPOutput);
                    DPRINTF(CGRA_Detailed,"Writing output %f to register %d\n",FPOutput,writeRegisterNumber);
                }
                if ((insOpCode==LDUi)) {
                    FLOAT Output1; 
                    Output1.raw.mantissa = ((unsigned int)FPOutput & INS_MANTISSA)>>SHIFT_MANTISSA;
                    Output1.raw.exponent = ((unsigned int)FPOutput & INS_EXPONENT)>>SHIFT_EXPONENT;
                    Output1.raw.sign = ((unsigned int)FPOutput & INS_SIGN)>>SHIFT_SIGN; 
                    FPRegFile.Write(writeRegisterNumber,(Output1.f)); 
                    RegFile.Write(writeRegisterNumber,(int)(Output1.f));
                    DPRINTF(CGRA_Detailed,"LDUI Writing output value %f to FPregister %d\n",Output1.f,writeRegisterNumber);
                }
            }
        }

        if (insOpCode == address_generator) {
            if (ins->getSelectDataMemoryAddressBus()) {
                DPRINTF(CGRA_Detailed,"\n*********Setting Address %lx ******\n",(unsigned int)Output);
                (*addressBs) = (unsigned int)Output;
                (*BsStatus) = CGRA_MEMORY_READ;
                (*BsDatatype) = CGRA_MEMORY_FP;
                (*alignmentBs) = (int) FPInput2;
            }
        }
    } else {// end of predication_bit
        // cond ins
        Cond_Instruction* condIns = new Cond_Instruction(ins->getInsWord());
        bool LE_bit = condIns->getloopExitEnable();
        CondOpCode insOpcode = condIns->getOpCode();
        switch (insOpcode) {
          case CMPGT:
            FPOutput=(float)FPInput1>(float)FPInput2;
            OutputP=(bool)((float)FPInput1>(float)FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** Greater Than IN THIS PE %f************\n",OutputP);
            break;
          case CMPLT:
            FPOutput=FPInput1<FPInput2;
            OutputP=(bool)(FPInput1<FPInput2);
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** Less Than IN THIS PE %f************\n",OutputP);
            break;
          case CMPEQ:
            FPOutput=FPInput1==FPInput2;
            OutputP=(bool)(FPInput1==FPInput2); 
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** EQUALS IN THIS PE %f************\n",OutputP);
            break;
          case CMPNEQ:
            FPOutput=FPInput1!=FPInput2;
            OutputP=(bool)(FPInput1!=FPInput2); 
            DPRINTF(CGRA_Detailed,"\nFPInput1 = %f\tFPInput2 = %f\n",FPInput1, FPInput2);
            DPRINTF(CGRA_Detailed,"\n******** NOT EQUALS IN THIS PE %f************\n",OutputP);
            break;
          default:
            DPRINTF(CGRA_Detailed," 1. Opcode is %ld\n",(unsigned) insOpcode);
            throw new CGRAException("Unknown CGRA Opcode");
        }

        if (ins->getWriteRegisterEnable()) {
            //DPRINTF(CGRA_Detailed,"\n************** WE *****************\n");
            int writeRegisterNumber = ins->getWriteRegAddress();
            RegFile.Write(writeRegisterNumber,(int)FPOutput);
            FPRegFile.Write(writeRegisterNumber,FPOutput);
            DPRINTF(CGRA_Detailed,"Writing output %f to register %d\n",FPOutput,writeRegisterNumber);
        }

        if (LE_bit) {
            bool LE_dest = condIns->getSplitDirection();
            DPRINTF(CGRA_Execute, "LE Branch: %d\n", LE_dest);
            this->Controller_Reg = !(Output == LE_dest);  // Output == LE_dest to exit
        }
    }
    DPRINTF(CGRA_Detailed,"Distance is: %d\n",RegFile.distance);
    DPRINTF(PE_DEBUG, "Exiting Execute()\n");
}


void 
CGRA_PE::WriteBack()
{
    DPRINTF(PE_DEBUG, "Inside WB()\n");
    bool insPredicate = ins->getPredicator();
    bool insCond = ins->getCond();
    if (!insPredicate && !insCond) {
        if (ins->getSelectDataMemoryDataBus()) {
            (*BsStatus) = CGRA_MEMORY_WRITE;
            dt = ins->getDatatype();
            if (dt == character || dt == int16 || dt == int32)
                (*BsDatatype) = CGRA_MEMORY_INT;
            else
                (*BsDatatype) = CGRA_MEMORY_FP; 
        }
        DPRINTF(PE_DEBUG, "Exiting WB()\n");
    }
    delete ins;
}


void 
CGRA_PE::SetNeighbours(int* Left,int* Right,int* Up,int* Down)
{
    DPRINTF(PE_DEBUG, "Inside SetNeighbors()\n");
    leftIn = Left;
    rightIn = Right;
    upIn = Up;
    downIn = Down;
    DPRINTF(PE_DEBUG, "Exiting SetNeighbors()\n");
}


void 
CGRA_PE::SetFPNeighbours(float* Left,float* Right,float* Up,float* Down)
{
    DPRINTF(PE_DEBUG, "Inside SetFPNeighbors()\n");
    FPleftIn = Left;
    FPrightIn = Right;
    FPupIn = Up;
    FPdownIn = Down;
    DPRINTF(PE_DEBUG, "Exiting SetFPNeighbors()\n");
}


void 
CGRA_PE::SetPredNeighbours(bool *Left,bool *Right,bool *Up, bool* Down)
{
    DPRINTF(PE_DEBUG, "Inside SetPredNeighbors()\n");
    PredleftIn = Left;
    PredrightIn = Right;
    PredupIn = Up;
    PreddownIn = Down;
    DPRINTF(PE_DEBUG, "Exiting SetPredNeighbors()\n");
}


void 
CGRA_PE::SetController_Reg()
{
    Controller_Reg = true;
}

bool 
CGRA_PE::getController_Reg()
{
    return this->Controller_Reg;
}


void 
CGRA_PE::advanceTime()
{
    DPRINTF(PE_DEBUG, "Inside advanceTime()\n");
    RegFile.NextIteration();
    FPRegFile.NextIteration();
    DPRINTF(PE_DEBUG, "Exiting advanceTime()\n");
}


void 
CGRA_PE::D_WriteRegfile(int address, int value)
{
    DPRINTF(PE_DEBUG, "Inside WriteRegFile()\n");
    RegFile.Write(address,value);
    DPRINTF(PE_DEBUG, "Exiting WriteRegFile()\n");
}


void 
CGRA_PE::ClearRegfile()
{
    DPRINTF(PE_DEBUG, "Inside clearRegfile()\n");
    RegFile.ClearRF();
    DPRINTF(PE_DEBUG, "Exiting clearRegfile()\n");
}


float*
CGRA_PE::getFPOutputPtr()
{
    return((float*)(&(this->FPOutput)));
}


int*
CGRA_PE::getOutputPtr()
{
    return((&(this->Output)));
}


bool*
CGRA_PE::getPredOutputPtr()
{
    return((&(this->OutputP)));
}


void 
CGRA_PE::setDataBus(int * data)
{
  (this->dataBs) = data;
}


void 
CGRA_PE::setFDataBus(float * data)
{
  (this->FdataBs) = data;
}


void 
CGRA_PE::setAddressBus(uint64_t * addr)
{
  (this->addressBs) = addr;
}


void 
CGRA_PE::setRWStatusBus(int * status)
{
  (this->BsStatus) = status;
}


void 
CGRA_PE::setAlignmentBus(unsigned *alignment)
{
  (this->alignmentBs) = alignment;
}


void 
CGRA_PE::setDatatypeBus(int * dt)
{
  (this->BsDatatype) = dt; 
}


void
CGRA_PE::setRecovery(int updatePtr)
{
    outputRecover[updatePtr] = Output;
    outputPRecover[updatePtr] = OutputP;
    DPRINTF(CGRA_Detailed,"backup output: %d, outputp:%d\n", outputRecover, outputPRecover);
}


void
CGRA_PE::rollback(int rollbackPtr)
{
    Output = outputRecover[rollbackPtr];
    OutputP = outputPRecover[rollbackPtr];
    DPRINTF(CGRA_Detailed,"rolled back to output: %d, outputp:%d\n", Output, OutputP);
}


void
CGRA_PE::setupBackup(unsigned iterCount)
{
    outputRecover = new int[iterCount];
    outputPRecover = new bool[iterCount];
}