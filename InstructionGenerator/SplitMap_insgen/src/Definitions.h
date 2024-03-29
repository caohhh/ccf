/**
 * definitions.h
 * 
 * Cao Heng
 * 2024.3.18
*/

#ifndef __INSGEN_DEFINITIONS_H__
#define __INSGEN_DEFINITIONS_H__

#include <iostream>


enum Instruction_Operation
{
  add,              //0
  sub,              //1
  mult,             //2
  division,         //3
  shiftl,           //4
  shiftr,           //5 
  andop,            //6
  orop,             //7
  xorop,            //8
  cmpSGT,           //9
  cmpEQ,            //10
  cmpNEQ,           //11
  cmpSLT,           //12
  cmpSLEQ,          //13
  cmpSGEQ,          //14
  cmpUGT,           //15
  cmpULT,           //16
  cmpULEQ,          //17
  cmpUGEQ,          //18
  ld_add,           //19
  ld_data,          //20
  st_add,           //21
  st_data,          //22
  ld_add_cond,      //23
  ld_data_cond,     //24
  loopctrl,         //25
  cond_select,      //26
  route,            //27
  llvm_route,       //28
  cgra_select,      //29
  constant,         //30
  rem,              //31
  sext,             //32
  bitcast,          //33
  shiftr_logical,   //34
  rest              //35
};

enum nodePath
{
  none,
  true_path,
  false_path
};


enum direction
{
  same,
  up,
  down,
  left,
  right,
  constOp,
  liveInData,
  noop
};


#define FATAL(x) \
do { \
  std::cerr << x << std::endl; \
  exit(1); \
} while (0)

#ifndef NDEBUG
  #define DEBUG(x) \
  do { \
    std::cout << x << std::endl; \
  } while(0) 
#else
  #define DEBUG(x)
#endif 


#endif //__INSGEN_DEFINITIONS_H__