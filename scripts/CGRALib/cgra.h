// Author: Shail Dave

#ifndef _CGRA_H
#define _CGRA_H

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>


#define CPU_STATE_REG 0
#define SIM_CLOCK_REG 0
#define CGRA_LOOPID_REG 10

#define CGRA_ACTIVATE 0xefefefef  // Previously 15
#define CGRA_DEACTIVATE 0xdfffffff  // Previously 17 
#define CGRA_SWITCH 0xcfffffff  // Previously 16
#define CGRA_EXEC_OVER 0xbfffffff  // Previously 66
#define SYS_CLOCK 0xafffffff

#define DEBUG

/*#define activate_CGRA 0xefffffff  // 15
#define stop_CGRA 0xcfffffff  // 16
#define deactivate_CGRA 0xdfffffff  // 17*/
//#define CPU_idle  71


extern volatile unsigned get_clock(void);
void accelerateOnCGRA(unsigned int loopNo);
void readInstructionSize(void);
int initializeParameters(unsigned int loopID);
int configureCGRA(unsigned int loopID);
void* runOnCGRA(void);
void deleteCGRA(); 

unsigned *livein_size, *kernel_size, *liveout_size, *iter_size; // size of instruction array for individual loopID
volatile unsigned long long *livein,*kernel,*liveout; // instruction array for all loops
volatile unsigned *iter;
unsigned int *ArrPtr;

volatile unsigned long long * liveinPtr, *kernelPtr, *liveoutPtr, *iterPtr;

// the liveinLength, II, liveoutLength
int *initCGRA;
#define initCGRA_size 3
unsigned totalLoops=0;

pthread_t pth;
static unsigned int thread_cond_cpu = 1;
static unsigned int thread_cond_cgra = 0;
static unsigned int thread_exit = 0;

int dynamicTCVal = 0;
#endif /* EXEC_CGRA_ */
