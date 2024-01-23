/**
 * definitons.h
*/
#ifndef __SPLITMAP_DEFINITIONS_H__
#define __SPLITMAP_DEFINITIONS_H__
#include <iostream>

struct CGRA_Architecture
{
  unsigned X_Dim;
  unsigned Y_Dim;
  unsigned int R_Size;
  unsigned MAX_NODE_INDEGREE;
  unsigned MAX_NODE_OUTDEGREE;
  unsigned PER_ROW_MEM_AVAILABLE;
  unsigned MAX_PE_INDEGREE; 
  unsigned MAX_PE_OUTDEGREE;
  unsigned MAX_DATA_PER_TIMESLOT;  
};

struct Mapping_Policy
{
  float MAX_MAPPING_ATTEMPTS;
  int CLIQUE_ATTEMPTS;
  int MAX_LATENCY;
  bool ENABLE_REGISTERS;
  int MODULO_SCHEDULING_ATTEMPTS;
  int MAPPING_ATTEMPTS_PER_II;
  int MAPPING_MODE;
  int MAX_II;
  int MAX_SCHED_TRY;
  float LAMBDA;
};

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

enum Datatype
{
  character,  //0
  int32,      //1
  int16,      //2
  float32,    //3
  float64,    //4
  float16,    //5
};

enum nodePath
{
  none,
  true_path,
  false_path
};

enum DataDepType
{
  TrueDep,
  LoadDep,   //an arc between load address assertion node and load bus read node should impose 1 cycle delay between nodes
  StoreDep,  //an arc between store address assertion node and store bus assertion node, should impose 0 cycle
  MemoryDep, //not implemented yet
  PredDep,
  LoopControlDep
};

#define FATAL(x) \
do { \
  std::cerr << x << std::endl; \
  exit(1); \
} while (0)

#define DEBUG(x) \
do { \
  std::cout << x << std::endl; \
} while(0)
#endif //__SPLILTMAP_DEFINITIONS_H__