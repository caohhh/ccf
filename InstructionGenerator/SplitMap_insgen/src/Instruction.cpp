/**
 * Instruction.cpp
 * Implementation for instructions.
 * 
 * Cao Heng
 * 2024.3.25
*/

#include "Instruction.h"

uint64_t
Instruction::encodeIns(Datatype dType, OPCode opCode, bool pred, bool cond, PEInputMux lMux, PEInputMux rMux,
              unsigned reg1, unsigned reg2, unsigned regW, bool we, bool aBus, bool dBus, int32_t imm)
{
  uint64_t insWord = 0;
  insWord |= ((0UL | (int)dType) << SHIFT_DATATYPE) & INS_DATATYPE;
  insWord |= ((0UL | opCode) << SHIFT_OPCODE) & INS_OPCODE;
  insWord |= ((0UL | pred) << SHIFT_PREDICT) & INS_PREDICT;
  insWord |= ((0UL | cond) << SHIFT_COND) & INS_COND;
  insWord |= ((0UL | lMux) << SHIFT_LMUX) & INS_LMUX;
  insWord |= ((0UL | rMux) << SHIFT_RMUX) & INS_RMUX;
  insWord |= ((0UL | reg1) << SHIFT_R1) & INS_R1;
  insWord |= ((0UL | reg2) << SHIFT_R2) & INS_R2;
  insWord |= ((0UL | regW) << SHIFT_RW) & INS_RW;
  insWord |= ((0UL | we) << SHIFT_WE) & INS_WE;
  insWord |= ((0UL | aBus) << SHIFT_ABUS) & INS_AB;
  insWord |= ((0UL | dBus) << SHIFT_DBUS) & INS_DB;
  insWord |= ((0UL | imm) << SHIFT_IMMEDIATE) & INS_IMMEDIATE;
  return insWord;
}


uint64_t 
Instruction::encodePIns(Datatype dType, PredOPCode opCode, PEInputMux lMux, PEInputMux rMux, PEInputMux pMux,
              unsigned reg1, unsigned reg2, unsigned regP, int32_t imm)
{
  uint64_t predInsWord = 0;
  predInsWord |= ((0UL | dType) << SHIFT_P_DATATYPE) & INS_P_DATATYPE;
  predInsWord |= ((0UL | opCode) << SHIFT_P_OPCODE) & INS_P_OPCODE;
  predInsWord |= ((0UL | 1UL) << SHIFT_PREDICT) & INS_PREDICT;
  predInsWord |= ((0UL | 0UL) << SHIFT_COND) & INS_COND;
  predInsWord |= ((0UL | lMux) << SHIFT_P_LMUX) & INS_P_LMUX;
  predInsWord |= ((0UL | rMux) << SHIFT_P_RMUX) & INS_P_RMUX;
  predInsWord |= ((0UL | reg1) << SHIFT_P_R1) & INS_P_R1;
  predInsWord |= ((0UL | reg2) << SHIFT_P_R2) & INS_P_R2;
  predInsWord |= ((0UL | regP) << SHIFT_P_RP) & INS_P_RP;
  predInsWord |= ((0UL | pMux) << SHIFT_P_PMUX) & INS_P_PMUX;
  predInsWord |= ((0UL | imm) << SHIFT_P_IMMEDIATE) & INS_P_IMMEDIATE;
  return predInsWord;
}


uint64_t
Instruction::encodeCIns(Datatype dType, CondOpCode opCode, bool loopExit, bool splitCond, 
              PEInputMux lMux, PEInputMux rMux, unsigned reg1, unsigned reg2, unsigned regW, bool we, 
              int32_t imm)
{
  uint64_t condInsWord = 0;
  condInsWord |= ((0UL | dType) << SHIFT_C_DATATYPE) & INS_C_DATATYPE;
  condInsWord |= ((0UL | opCode) << SHIFT_C_OPCODE) & INS_C_OPCODE;
  condInsWord |= ((0UL | splitCond) << SHIFT_C_SP) & INS_C_SP;
  condInsWord |= ((0UL | loopExit) << SHIFT_C_LE) & INS_C_LE;
  condInsWord |= ((0UL | 1UL) << SHIFT_COND) & INS_COND;
  condInsWord |= ((0UL | lMux) << SHIFT_C_LMUX) & INS_C_LMUX;
  condInsWord |= ((0UL | rMux) << SHIFT_C_RMUX) & INS_C_RMUX;
  condInsWord |= ((0UL | reg1) << SHIFT_C_R1) & INS_C_R1;
  condInsWord |= ((0UL | reg2) << SHIFT_C_R2) & INS_C_R2;
  condInsWord |= ((0UL | regW) << SHIFT_C_RW) & INS_C_RW;
  condInsWord |= ((0UL | we) << SHIFT_C_WE) & INS_C_WE;
  condInsWord |= ((0UL | imm) << SHIFT_C_IMMEDIATE) & INS_C_IMMEDIATE;
  return condInsWord;
}
