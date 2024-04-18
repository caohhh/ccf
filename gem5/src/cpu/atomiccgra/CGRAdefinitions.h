/*
* definitions.h
*
* Created on: May 24, 2011
* Author: mahdi
*
* Last edited: 24 May 2017
* Author: Shail Dave
*
* Last edited: 10 July 2019
* Author: Mahesh Balasubramanian
* Update: Parameterized Rfsize and CGRA dims
*
*/

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include "debug/CGRA.hh"
#include "debug/CGRA_Detailed.hh"
#include "cpu/atomiccgra/base.hh"

#include "CGRAException.h"

//#define IMEMSIZE 1024
//#define DMEMSIZE 256
//#define REGFILESIZE 4

//#define CGRA_XDim 4
//#define CGRA_YDim 4

#define CGRA_MEMORY_RESET 0
#define CGRA_MEMORY_READ 1
#define CGRA_MEMORY_WRITE 2

#define CGRA_MEMORY_INT 1
#define CGRA_MEMORY_FP 2

/*
  not used anymore
  Loop Exit Instruction Decode: 
  63 62 61 | 60 59 58 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 | 35 ... 26 | 25...0
  DT       | OpCode      | P  | 1  |   LMUX   |   RMUX   |      R1     |      R2     |      RW     | WE |   BrImm   | Imme
*/

/*
  64-bit instruction word
  note that even though immediate takes up 34 bits, it's still limited to 32 bits due to data bus width of the CGRA
  maybe the extra 2 bits can be set as reserved

  Normal Instruction Decode:
  63 62 61 | 60 59 58 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 | 35 | 34 | 33  | 32 | 31 ... 0
  DT       |    OpCode   | P  | C  |   LMUX   |   RMUX   |      R1     |      R2     |      RW     | WE | AB | DB | Phi |    |Immediate

  P-Type Instruction Decode:
  63 62 61 | 60 59 58 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 35 34 | 33 32  |31 ... 0
  DT       |    OpCode   | 1  | 0  |   LMUX   |   RMUX   |      R1     |      R2     |      RP     |    PMUX  | Unused |Immediate
  
  new Condition Instruction replacing Loop Exit Instruction
  with cond instructions, we currently ensure it will be an compare op
  63 62 61 | 60 59 58 | 57 | 56 | 55 | 54 53 52 | 51 50 49 | 48 47 46 45 | 44 43 42 41 | 40 39 38 37 | 36 | 35 ... 26 | 25...0
  DT       |  OpCode  | SP | LE | 1  |   LMUX   |   RMUX   |      R1     |      R2     |      RW     | WE |   BrImm   | Imme
*/


// the UL here is to supress the warning.
#define WIDTH_DATATYPE          0x7UL
#define WIDTH_OPCODE            0xfUL
#define WIDTH_PREDICT           0x1UL
#define WIDTH_C                 0x1UL
#define WIDTH_MUX               0x7UL
#define WIDTH_REGISTER          0xfUL
#define WIDTH_ENABLE            0x1UL
#define WIDTH_IMMEDIATE         0xffffffffUL

#define SHIFT_DATATYPE          61
#define SHIFT_OPCODE            57
#define SHIFT_PREDICT           56
#define SHIFT_COND              55
#define SHIFT_LMUX              52
#define SHIFT_RMUX              49
#define SHIFT_R1                45
#define SHIFT_R2                41
#define SHIFT_RW                37
#define SHIFT_WE                36
#define SHIFT_ABUS              35
#define SHIFT_DBUS              34
#define SHIFT_PHI               33
#define SHIFT_IMMEDIATE         00

#define INS_DATATYPE            (WIDTH_DATATYPE)<<SHIFT_DATATYPE
#define INS_OPCODE              (WIDTH_OPCODE)<<SHIFT_OPCODE
#define INS_PREDICT             (WIDTH_PREDICT)<<SHIFT_PREDICT
#define INS_COND                (WIDTH_C)<<SHIFT_COND
#define INS_LMUX                (WIDTH_MUX)<<SHIFT_LMUX
#define INS_RMUX                (WIDTH_MUX)<<SHIFT_RMUX
#define INS_R1                  (WIDTH_REGISTER)<<SHIFT_R1
#define INS_R2                  (WIDTH_REGISTER)<<SHIFT_R2
#define INS_RW                  (WIDTH_REGISTER)<<SHIFT_RW
#define INS_WE                  (WIDTH_ENABLE)<<SHIFT_WE
#define INS_AB                  (WIDTH_ENABLE)<<SHIFT_ABUS
#define INS_DB                  (WIDTH_ENABLE)<<SHIFT_DBUS
#define INS_PHI                 (WIDTH_ENABLE)<<SHIFT_PHI
#define INS_IMMEDIATE           (WIDTH_IMMEDIATE)<<SHIFT_IMMEDIATE

// P-Type
#define WIDTH_P_DATATYPE        0x7UL
#define WIDTH_P_OPCODE          0xfUL
#define WIDTH_P_MUX             0x7UL
#define WIDTH_P_REGISTER        0xfUL
#define WIDTH_P_IMMEDIATE       0xffffffffUL

#define SHIFT_P_DATATYPE        61
#define SHIFT_P_OPCODE          57
#define SHIFT_P_LMUX            52
#define SHIFT_P_RMUX            49
#define SHIFT_P_R1              45
#define SHIFT_P_R2              41
#define SHIFT_P_RP              37
#define SHIFT_P_PMUX            34
#define SHIFT_P_IMMEDIATE       00

#define INS_P_DATATYPE          (WIDTH_P_DATATYPE)<<SHIFT_P_DATATYPE
#define INS_P_OPCODE            (WIDTH_P_OPCODE)<<SHIFT_P_OPCODE
#define INS_P_LMUX              (WIDTH_P_MUX)<<SHIFT_P_LMUX
#define INS_P_RMUX              (WIDTH_P_MUX)<<SHIFT_P_RMUX
#define INS_P_PMUX              (WIDTH_P_MUX)<<SHIFT_P_PMUX
#define INS_P_R1                (WIDTH_P_REGISTER)<<SHIFT_P_R1
#define INS_P_R2                (WIDTH_P_REGISTER)<<SHIFT_P_R2
#define INS_P_RP                (WIDTH_P_REGISTER)<<SHIFT_P_RP
#define INS_P_IMMEDIATE         (WIDTH_P_IMMEDIATE)<<SHIFT_P_IMMEDIATE

// C-Type
#define WIDTH_C_DATATYPE        0x7UL
#define WIDTH_C_OPCODE          0x7UL
#define WIDTH_C_SP              0x1UL
#define WIDTH_C_LE              0x1UL
#define WIDTH_C_MUX             0x7UL
#define WIDTH_C_REGISTER        0xfUL
#define WIDTH_C_ENABLE          0x1UL
#define WIDTH_C_BROFFSET        0x3ffUL
#define WIDTH_C_IMMEDIATE       0x3ffffffUL

#define SHIFT_C_DATATYPE        61
#define SHIFT_C_OPCODE          58
#define SHIFT_C_SP              57
#define SHIFT_C_LE              56
#define SHIFT_C_LMUX            52
#define SHIFT_C_RMUX            49
#define SHIFT_C_R1              45
#define SHIFT_C_R2              41
#define SHIFT_C_RW              37
#define SHIFT_C_WE              36
#define SHIFT_C_BROFFSET        26
#define SHIFT_C_IMMEDIATE       00

#define INS_C_DATATYPE          (WIDTH_C_DATATYPE)<<SHIFT_C_DATATYPE
#define INS_C_OPCODE            (WIDTH_C_OPCODE)<<SHIFT_C_OPCODE
#define INS_C_SP                (WIDTH_C_SP)<<SHIFT_C_SP
#define INS_C_LE                (WIDTH_C_LE)<<SHIFT_C_LE
#define INS_C_LMUX              (WIDTH_C_MUX)<<SHIFT_C_LMUX
#define INS_C_RMUX              (WIDTH_C_MUX)<<SHIFT_C_RMUX
#define INS_C_R1                (WIDTH_C_REGISTER)<<SHIFT_C_R1
#define INS_C_R2                (WIDTH_C_REGISTER)<<SHIFT_C_R2
#define INS_C_RW                (WIDTH_C_REGISTER)<<SHIFT_C_RW
#define INS_C_WE                (WIDTH_C_ENABLE)<<SHIFT_C_WE
#define INS_C_BROFFSET          (WIDTH_C_BROFFSET)<<SHIFT_C_BROFFSET
#define INS_C_IMMEDIATE         (WIDTH_C_IMMEDIATE)<<SHIFT_C_IMMEDIATE


// for FloatingPoint support
#define SHIFT_SIGN      63
#define SHIFT_EXPONENT  52
#define SHIFT_MANTISSA  00

#define WIDTH_SIGN      0x1UL
#define WIDTH_EXPONENT  0x7ffUL
#define WIDTH_MANTISSA  0xfffffffffffffUL

#define INS_SIGN      (WIDTH_SIGN)<<SHIFT_SIGN
#define INS_EXPONENT  (WIDTH_EXPONENT)<<SHIFT_EXPONENT
#define INS_MANTISSA  (WIDTH_MANTISSA)<<SHIFT_MANTISSA


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
    //LOAD IMMEDIATE INSTRUCTIONS
    Div,
    Rem,
    LSHR, //Sqrt,
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

typedef union {

    float f;
    struct
    {

        // Order is important. 
        // Here the members of the union data structure 
        // use the same memory (32 bits). 
        // The ordering is taken 
        // from the LSB to the MSB. 
        unsigned long mantissa : 52;
        unsigned int exponent : 11;
        unsigned int sign : 1;

    } raw;
} FLOAT;


//related to debugging messages
#define CGRA_DEBUG 1

#define DEBUG_CGRA(msg...)	\
do {	\
	PRINT_DEBUG(CGRA_DEBUG,msg);	\
} while(0)

#define PRINT_DEBUG(condition,msg...)	\
do {	\
	if((condition)) {	\
		std::cout << msg; \
	}	\
} while(0)

#endif /* DEFINITIONS_H_ */
