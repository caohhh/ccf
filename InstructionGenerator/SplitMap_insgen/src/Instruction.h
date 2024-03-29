/**
 * Instruction.h
 * Used for CGRA instruction word definitions
 * 
 * Cao Heng
 * 2024.3.22
*/

/*
  64-bit instruction word
  note that even though immediate takes up 34 bits, it's still limited to 32 bits due to data bus width of the CGRA
  maybe the extra 2 bits can be set as reserved

  Normal Instruction Decode:
  63 62 61 | 60 59 58 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 | 35 | 34 | 33 32  | 31 ... 0
  DT       |    OpCode   | P  | C  |   LMUX   |   RMUX   |      R1     |      R2     |      RW     | WE | AB | DB | Unused | Immediate

  P-Type Instruction Decode:
  63 62 61 | 60 59 58 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 35 34 | 33 32  |31 ... 0
  DT       |    OpCode   | 1  | 0  |   LMUX   |   RMUX   |      R1     |      R2     |      RP     |    PMUX  | Unused |Immediate
  
  new Condition Instruction replacing Loop Exit Instruction
  with cond instructions, we currently ensure it will be an compare op
  63 62 61 | 60 59 58 | 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 | 35 ... 26 | 25...0
  DT       |  OpCode  | SP | LE | 1  |   LMUX   |   RMUX   |      R1     |      R2     |      RW     | WE |   BrImm   | Imme
*/

#ifndef __INSGEN_INSTRUCTION_H__
#define __INSGEN_INSTRUCTION_H__

#include <cstdint>


namespace Instruction
{

const uint64_t WIDTH_DATATYPE = 0x7UL;
const uint64_t WIDTH_OPCODE = 0xfUL;
const uint64_t WIDTH_PREDICT = 0x1UL;
const uint64_t WIDTH_C = 0x1UL;
const uint64_t WIDTH_MUX = 0x7UL;
const uint64_t WIDTH_REGISTER = 0xfUL;
const uint64_t WIDTH_ENABLE = 0x1UL;
const uint64_t WIDTH_IMMEDIATE = 0xffffffffUL;

const uint64_t SHIFT_DATATYPE = 61;
const uint64_t SHIFT_OPCODE = 57;
const uint64_t SHIFT_PREDICT = 56;
const uint64_t SHIFT_COND = 55;
const uint64_t SHIFT_LMUX = 52;
const uint64_t SHIFT_RMUX = 49;
const uint64_t SHIFT_R1 = 45;
const uint64_t SHIFT_R2 = 41;
const uint64_t SHIFT_RW = 37;
const uint64_t SHIFT_WE = 36;
const uint64_t SHIFT_ABUS = 35;
const uint64_t SHIFT_DBUS = 34;
const uint64_t SHIFT_IMMEDIATE = 00;

const uint64_t INS_DATATYPE = (WIDTH_DATATYPE)<<SHIFT_DATATYPE;
const uint64_t INS_OPCODE = (WIDTH_OPCODE)<<SHIFT_OPCODE;
const uint64_t INS_PREDICT = (WIDTH_PREDICT)<<SHIFT_PREDICT;
const uint64_t INS_COND = (WIDTH_C)<<SHIFT_COND;
const uint64_t INS_LMUX = (WIDTH_MUX)<<SHIFT_LMUX;
const uint64_t INS_RMUX = (WIDTH_MUX)<<SHIFT_RMUX;
const uint64_t INS_R1 = (WIDTH_REGISTER)<<SHIFT_R1;
const uint64_t INS_R2 = (WIDTH_REGISTER)<<SHIFT_R2;
const uint64_t INS_RW = (WIDTH_REGISTER)<<SHIFT_RW;
const uint64_t INS_WE = (WIDTH_ENABLE)<<SHIFT_WE;
const uint64_t INS_AB = (WIDTH_ENABLE)<<SHIFT_ABUS;
const uint64_t INS_DB = (WIDTH_ENABLE)<<SHIFT_DBUS;
const uint64_t INS_IMMEDIATE = (WIDTH_IMMEDIATE)<<SHIFT_IMMEDIATE;

// P-Type
const uint64_t WIDTH_P_DATATYPE = 0x7UL;
const uint64_t WIDTH_P_OPCODE = 0xfUL;
const uint64_t WIDTH_P_MUX = 0x7UL;
const uint64_t WIDTH_P_REGISTER = 0xfUL;
const uint64_t WIDTH_P_IMMEDIATE = 0xffffffffUL;

const uint64_t SHIFT_P_DATATYPE = 61;
const uint64_t SHIFT_P_OPCODE = 57;
const uint64_t SHIFT_P_LMUX = 52;
const uint64_t SHIFT_P_RMUX = 49;
const uint64_t SHIFT_P_R1 = 45;
const uint64_t SHIFT_P_R2 = 41;
const uint64_t SHIFT_P_RP = 37;
const uint64_t SHIFT_P_PMUX = 34;
const uint64_t SHIFT_P_IMMEDIATE = 00;

const uint64_t INS_P_DATATYPE = (WIDTH_P_DATATYPE)<<SHIFT_P_DATATYPE;
const uint64_t INS_P_OPCODE = (WIDTH_P_OPCODE)<<SHIFT_P_OPCODE;
const uint64_t INS_P_LMUX = (WIDTH_P_MUX)<<SHIFT_P_LMUX;
const uint64_t INS_P_RMUX = (WIDTH_P_MUX)<<SHIFT_P_RMUX;
const uint64_t INS_P_PMUX = (WIDTH_P_MUX)<<SHIFT_P_PMUX;
const uint64_t INS_P_R1 = (WIDTH_P_REGISTER)<<SHIFT_P_R1;
const uint64_t INS_P_R2 = (WIDTH_P_REGISTER)<<SHIFT_P_R2;
const uint64_t INS_P_RP = (WIDTH_P_REGISTER)<<SHIFT_P_RP;
const uint64_t INS_P_IMMEDIATE = (WIDTH_P_IMMEDIATE)<<SHIFT_P_IMMEDIATE;

// C-Type
const uint64_t WIDTH_C_DATATYPE = 0x7UL;
const uint64_t WIDTH_C_OPCODE = 0x7UL;
const uint64_t WIDTH_C_SP = 0x1UL;
const uint64_t WIDTH_C_LE = 0x1UL;
const uint64_t WIDTH_C_MUX = 0x7UL;
const uint64_t WIDTH_C_REGISTER = 0xfUL;
const uint64_t WIDTH_C_ENABLE = 0x1UL;
const uint64_t WIDTH_C_BROFFSET = 0x3ffUL;
const uint64_t WIDTH_C_IMMEDIATE = 0x3ffffffUL;

const uint64_t SHIFT_C_DATATYPE = 61;
const uint64_t SHIFT_C_OPCODE = 58;
const uint64_t SHIFT_C_SP = 57;
const uint64_t SHIFT_C_LE = 56;
const uint64_t SHIFT_C_LMUX = 52;
const uint64_t SHIFT_C_RMUX = 49;
const uint64_t SHIFT_C_R1 = 45;
const uint64_t SHIFT_C_R2 = 41;
const uint64_t SHIFT_C_RW = 37;
const uint64_t SHIFT_C_WE = 36;
const uint64_t SHIFT_C_BROFFSET = 26;
const uint64_t SHIFT_C_IMMEDIATE = 00;

const uint64_t INS_C_DATATYPE = (WIDTH_C_DATATYPE)<<SHIFT_C_DATATYPE;
const uint64_t INS_C_OPCODE = (WIDTH_C_OPCODE)<<SHIFT_C_OPCODE;
const uint64_t INS_C_SP = (WIDTH_C_SP)<<SHIFT_C_SP;
const uint64_t INS_C_LE = (WIDTH_C_LE)<<SHIFT_C_LE;
const uint64_t INS_C_LMUX = (WIDTH_C_MUX)<<SHIFT_C_LMUX;
const uint64_t INS_C_RMUX = (WIDTH_C_MUX)<<SHIFT_C_RMUX;
const uint64_t INS_C_R1 = (WIDTH_C_REGISTER)<<SHIFT_C_R1;
const uint64_t INS_C_R2 = (WIDTH_C_REGISTER)<<SHIFT_C_R2;
const uint64_t INS_C_RW = (WIDTH_C_REGISTER)<<SHIFT_C_RW;
const uint64_t INS_C_WE = (WIDTH_C_ENABLE)<<SHIFT_C_WE;
const uint64_t INS_C_BROFFSET = (WIDTH_C_BROFFSET)<<SHIFT_C_BROFFSET;
const uint64_t INS_C_IMMEDIATE = (WIDTH_C_IMMEDIATE)<<SHIFT_C_IMMEDIATE;


enum OPCode
{
  Add=0,
  Sub,
  Mult,
  AND,
  OR,
  XOR,
  //aritmatic shift right
  cgraASR,
  //No Operation
  NOOP,
  //aritmatic shift left
  cgraASL,
  Div,
  Rem,
  LSHR, //logic shift right,
  //COMPARE INSTRUCTIONS
  EQ, //==
  NEQ, // !=
  GT, //>
  LT //<
};


enum PredOPCode
{
  setConfigBoundry=0,
  LDi,
  LDMi,
  LDUi,
  sel,
  loopexit,
  address_generator,
  nop,
  signExtend
};


enum PEInputMux
{
  Register=0,
  Left,
  Right,
  Up,
  Down,
  DataBus,
  Immediate,
  Self
};


enum CondOpCode
{
  CMPEQ,  // ==
  CMPNEQ, // !=
  CMPGT,  // >
  CMPLT   // <
};


enum Datatype
{
  character=0,    //0
  int32,          //1
  int16,          //2
  float32,        //3
  float64,        //4
  float16,        //5
  empty1,         //6
  empty2          //7
};

/**
 * encode a regular instruction
 * @param dType data type
 * @param opCode operation code
 * @param pred predicate enable, should be 0 for regular instruction
 * @param cond condition enable, should be 0 for regular instruction
 * @param lMux left input mux selector
 * @param rMux right input mux selector
 * @param reg1 read registor address 1
 * @param reg2 read registor address 2
 * @param regW write registor address
 * @param we write registor enable
 * @param aBus memory address bus enable
 * @param dBus memory data bus enable
 * @param imm immediate value
 * @return the encoded instruction word
*/
uint64_t encodeIns(Datatype dType, OPCode opCode, bool pred, bool cond, PEInputMux lMux, PEInputMux rMux,
          unsigned reg1, unsigned reg2, unsigned regW, bool we, bool aBus, bool dBus, int32_t imm);


const uint64_t noopIns = encodeIns(int32, NOOP, 0, 0, Self, Self, 0, 0, 0, false, false, false, 0);


/**
 * encode a P-Type instruction
 * @param dType data type
 * @param opCode operation code
 * @param pred predicate enable, should be 0 for regular instruction
 * @param cond condition enable, should be 0 for regular instruction
 * @param lMux left input mux selector
 * @param rMux right input mux selector
 * @param pMux predicate input mux selector
 * @param reg1 read registor address 1
 * @param reg2 read registor address 2
 * @param regP predicate read registor address
 * @param imm immediate value
 * @return the encoded P-Type instruction word
*/
uint64_t encodePIns(Datatype dType, PredOPCode opCode, PEInputMux lMux, PEInputMux rMux, PEInputMux pMux, 
          unsigned reg1, unsigned reg2, unsigned regP, int32_t imm);


/**
 * encode a C-Type instruction
 * @param dType data type
 * @param opCode operation code
 * @param loopExit if this C instruction is the loop exit condition
 * @param splitCond for loop exit: the exit condition, for other condition: if this ins is the split cond
 * @param lMux left input mux selector
 * @param rMux right input mux selector
 * @param reg1 read registor address 1
 * @param reg2 read registor address 2
 * @param regW write registor address
 * @param we write registor enable
 * @param brImm branch jump offset value
 * @param imm immediate value
 * @return the encoded C-Type instruction word
*/
uint64_t encodeCIns(Datatype dType, CondOpCode opCode, bool loopExit, bool splitCond, PEInputMux lMux, PEInputMux rMux,
          unsigned reg1, unsigned reg2, unsigned regW, bool we, int16_t brImm, int32_t imm);

}; // end of namespace Instruction

#endif