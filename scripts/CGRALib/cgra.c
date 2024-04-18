// Author: Shail Dave

// Last edited: April 5 2022
// Author: Vinh TA
// Update: add support for prolog extension

#include "cgra.h"
#include <string.h>
#include <limits.h>
#include <stdint.h>
#ifdef __cplusplus
#include <iostream>
#endif

//#define DEBUG

//#define usleep(time)  myusleep(time)

//pthread_mutex_t mutex;


__attribute__((noinline))
extern volatile unsigned get_clock()
{
  volatile unsigned ret;
  asm("sub r13, r13, #4");
  asm("str r12, [r13]");
  asm("mov r12,%[value]" : :[value]"r" (SYS_CLOCK):);
  asm("mov r0, r12");
  asm("ldr r12, [r13]");
  asm("add r13, r13, #4");
  asm("str r0, [r13]");
  return ret;
}

void readInstructionSize()
{
  #ifdef DEBUG
    printf("Reading instruction size for %d loops.\n", totalLoops);
  #endif
  for (int loopID = 1; loopID <= totalLoops; loopID++) {
    FILE *liveinFile, *liveoutFile, *kernFile, *iterFile;
    char loopno[25];
    char directoryPath[20] = "./CGRAExec/L";
    
    sprintf(loopno,"%d",loopID);
    strcat(directoryPath,loopno);

    char liveinfileName[40] = "";
    char liveoutfileName[40] = "";
    char kernfileName[40] = "";
    char iterFileName[40] = "";

    strcat(liveinfileName,directoryPath);
    strcat(liveinfileName,"/live_in.bin");
    
    strcat(liveoutfileName,directoryPath);
    strcat(liveoutfileName,"/live_out.bin");
    
    strcat(kernfileName,directoryPath);
    strcat(kernfileName,"/kernel.bin");

    strcat(iterFileName,directoryPath);
    strcat(iterFileName,"/iter.bin");

    liveinFile = fopen(liveinfileName, "rb");
    liveoutFile = fopen(liveoutfileName, "rb");
    kernFile = fopen(kernfileName, "rb");
    iterFile = fopen(iterFileName, "rb");

    if (liveinFile == NULL) {
      printf("FATAL: Cannot open file %s\n", liveinfileName);
      exit(1);
    } else if (liveoutFile == NULL) {
      printf("FATAL: Cannot open file %s\n", liveoutfileName);
      exit(1);
    } else if (kernFile == NULL) {
      printf("FATAL: Cannot open file %s\n", kernfileName);
      exit(1);
    } else if (iterFile == NULL) {
      printf("FATAL: Cannot open file %s\n", iterFileName);
      exit(1);
    }
    
    unsigned liveinSize, liveoutSize, kernelSize, iterSize;
    fread(&liveinSize, sizeof(unsigned), 1,liveinFile);
    fread(&liveoutSize, sizeof(unsigned), 1, liveoutFile);
    fread(&kernelSize, sizeof(unsigned), 1, kernFile);
    fread(&iterSize, sizeof(unsigned), 1, iterFile);

    livein_size[loopID - 1] = liveinSize;
    kernel_size[loopID - 1] = kernelSize;
    liveout_size[loopID - 1] = liveoutSize;
    iter_size[loopID - 1] = iterSize;

    fclose(liveinFile);
    fclose(liveoutFile);
    fclose(kernFile);
    fclose(iterFile);
  }
}

int initializeParameters(unsigned int loopID)
{
  #ifdef DEBUG
    printf("cgra.c: Initialize Parameters - loopID: %d\n", loopID);
  #endif

  *(initCGRA + initCGRA_size*(loopID-1)) = 0;	//Livein length
  *(initCGRA + initCGRA_size*(loopID-1) + 1) = 0;	//II
  *(initCGRA + initCGRA_size*(loopID-1) + 2) = 0;	//Liveout length

  FILE *liveinFile, *liveoutFile, *kernFile, *iterFile, *configFile;
  char loopno[25];
  char directoryPath[20] = "./CGRAExec/L";

  sprintf(loopno,"%d",loopID);
  strcat(directoryPath,loopno);

  char liveinFileName[40] = "";
  char liveoutFileName[40] = "";
  char kernFileName[40] = "";
  char iterFileName[40] = "";
  char configFileName[40] = "";

  strcat(liveinFileName,directoryPath);
  strcat(liveinFileName,"/live_in.bin");

  strcat(liveoutFileName,directoryPath);
  strcat(liveoutFileName,"/live_out.bin");

  strcat(kernFileName,directoryPath);
  strcat(kernFileName,"/kernel.bin");

  strcat(iterFileName,directoryPath);
  strcat(iterFileName,"/iter.bin");

  strcat(configFileName, directoryPath);
  strcat(configFileName, "/CGRA_config.txt");


  //Make some error checking for fopen and fread
  liveinFile=fopen(liveinFileName,"rb");
  liveoutFile=fopen(liveoutFileName,"rb");
  kernFile=fopen(kernFileName,"rb");
  iterFile = fopen(iterFileName, "rb");
  configFile = fopen(configFileName, "r");

  if (liveinFile == NULL) {
    printf("FATAL: Cannot open file livein %s\n", liveinFileName);
    exit(1);
  } else if (liveoutFile == NULL) {
    printf("FATAL: Cannot open file liveout %s\n", liveoutFileName);
    exit(1);
  } else if (kernFile == NULL) {
    printf("FATAL: Cannot open file kernel %s\n", kernFileName);
    exit(1);
  } else if (iterFile == NULL) {
    printf("FATAL: Cannot open file iter %s\n", iterFileName);
    exit(1);
  } else if (configFile == NULL) {
    printf("FATAL: Cannot open file config %s\n", configFileName);
    exit(1);
  }

  int liveinSize,liveoutSize,kernelSize, iterSize;
  fread(&liveinSize,sizeof(unsigned),1,liveinFile);
  fread(&liveoutSize,sizeof(unsigned),1,liveoutFile);
  fread(&kernelSize,sizeof(unsigned),1,kernFile);
  fread(&iterSize,sizeof(unsigned),1,iterFile);

  #ifdef DEBUG
    printf("\n**********LIVEINSIZE: %d*********\n",liveinSize);
    printf("\n**********LIVEOUTSIZE: %d*********\n",liveoutSize);
    printf("\n**********KERNSIZE: %d*********\n",kernelSize);
    printf("\n**********ITERSIZE: %d*********\n",iterSize);
  #endif

  int livein_index = 0, kernel_index = 0, iter_index = 0, liveout_index = 0;
  for (unsigned i = 1; i < loopID; i++) {
    livein_index += livein_size[i-1];
    kernel_index += kernel_size[i-1];
    iter_index += iter_size[i-1];
    liveout_index += liveout_size[i-1];
  }

  #ifdef DEBUG
    printf("livein_index: %d - kernel_index: %d - iter_index: %d - liveout_index: %d\n", 
            livein_index, kernel_index, iter_index, liveout_index);
  #endif
  
  fread(&livein[livein_index],sizeof(unsigned long long),liveinSize,liveinFile);
  fread(&liveout[liveout_index],sizeof(unsigned long long),liveoutSize,liveoutFile);
  fread(&kernel[kernel_index],sizeof(unsigned long long),kernelSize,kernFile);
  fread(&iter[iter_index],sizeof(int),iterSize,iterFile);


  char line[256];
  int XDim=0, YDim=0;
  int lineNo=0; 

  while (fgets(line, sizeof(line), configFile)) {
    lineNo++;
    if (lineNo == 1)
      XDim = atoi(line);
    else if (lineNo == 2)
      YDim = atoi(line);
    else
      break;
  }

  #ifdef DEBUG
    printf("\n************XDIM and YDim are %d, %d\n", XDim, YDim);
  #endif

  int II = kernelSize/(XDim*YDim*3);
  int liveinLength = liveinSize/(XDim*YDim);
  int liveoutLength = liveoutSize/(XDim*YDim);
  int iterCount;
  fread(&iterCount,sizeof(int),1,iterFile);


  *(initCGRA + initCGRA_size*(loopID-1)) = liveinLength;
  *(initCGRA + initCGRA_size*(loopID-1) + 1) = II;
  *(initCGRA + initCGRA_size*(loopID-1) + 2) = liveoutLength;
  *(initCGRA + initCGRA_size*(loopID-1) + 3) = iterCount;

  fclose(liveinFile);
  fclose(liveoutFile);
  fclose(kernFile);
  fclose(configFile);
  fclose(iterFile);

  #ifdef DEBUG
    printf("Printing Livein:\n");
    for(unsigned i=0; i<liveinSize; i++)
      printf("%d: %lx - %llx\n", i, &livein[i], livein[i]);
    printf("Printing kernel:\n");
    for(unsigned i=0; i<kernelSize; i++)
      printf("%d: %lx - %llx\n", i, &kernel[i], kernel[i]);
    printf("Printing iter:\n");
    for(unsigned i=0; i<iterSize; i++)
      printf("%d: %lx - %d\n", i, &iter[i], iter[i]);
    printf("Printing Liveout:\n");
    for(unsigned i=0; i<liveoutSize; i++)
      printf("%d: %lx - %llx\n", i, &liveout[i], liveout[i]);
  #endif


  #ifdef DEBUG
    printf("From FILE: LIVEINPC= %lx, LIVEOUTPC=%lx,  KernelPC=%lx\n",
    (unsigned long) (&livein[livein_index]),(unsigned long) (&liveout[liveout_index]),(unsigned long) (&kernel[kernel_index]));
  #endif

  liveinPtr[loopID-1] = (unsigned long long) (&livein[livein_index]);
  liveoutPtr[loopID-1] = (unsigned long long) (&liveout[liveout_index]);
  kernelPtr[loopID-1] = (unsigned long long) (&kernel[kernel_index]);
  iterPtr[loopID-1] = (unsigned long long) (&iter[iter_index]);

  return 0;
}

int configureCGRA(unsigned int loopID)
{
  #ifdef DEBUG
    printf("configureCGRA loop %d\n", loopID);
  #endif

  FILE* initFile;
  char loopno[25];
  char directoryPath[20] = "./CGRAExec/L";
  
  sprintf(loopno,"%d",loopID);
  strcat(directoryPath,loopno);

  strcat(directoryPath,"/initCGRA.txt");
  initFile = fopen(directoryPath, "wb");
  
  for(unsigned i = 0; i < initCGRA_size; i++)
    fprintf(initFile, "%d\n", *(initCGRA + initCGRA_size*(loopID-1) + i));
  fprintf(initFile, "%ld\n", (unsigned long long) liveinPtr[loopID-1]);
  fprintf(initFile, "%ld\n", (unsigned long long) kernelPtr[loopID-1]);
  fprintf(initFile, "%ld\n", (unsigned long long) iterPtr[loopID-1]);
  fprintf(initFile, "%ld\n", (unsigned long long) liveoutPtr[loopID-1]);
  fclose(initFile);

  return 0;
}

void checkTotalLoops( )
{
  #ifdef DEBUG
    printf("checkTotalLoops\n");
  #endif
  char myfile[40] = "./CGRAExec/total_loops.txt";
  FILE* count;
  count = fopen(myfile, "r");
  if (count == NULL) {
    printf("ERROR! Can't open total loops\n");
    exit(1);
  }
  fscanf(count, "%u", &totalLoops);
  fclose(count);
  #ifdef DEBUG
    printf("total loop count %d\n", totalLoops);
  #endif

}

//__attribute__((noinline))
__inline void* runOnCGRA()
{
#ifdef DEBUG
  printf("\n\nrunOnCGRA\n");
#endif
  asm("sub r13, r13, #4");
  asm("str r12, [r13]");
  asm("mov r12,%[value]" : :[value]"r" (CGRA_ACTIVATE):);
  asm("ldr r12, [r13]");
  asm("add r13, r13, #4");
  return NULL;
}

__inline void accelerateOnCGRA(unsigned int loopNo)
{
#ifdef DEBUG
  printf("\n\naccelerateOnCGRA\n");
#endif
    //int result = 0; 
    //int initializeloop = initializeParameters(loopNo);
    //int configure = configureCGRA(loopNo);
    //if(DEBUG) printf("configure = %d, loopNo = %d\n", configure, loopNo);
    /*if(configure == -1)
    {
      	printf("Error in configuring CGRA:\n");
	}*/
#ifdef DEBUG
    printf("Core will execute loop %u on CGRA\n",loopNo);
#endif

    /*const char *code = "\x00\x00\x00\x00\x00\x68";
    int (*func)();
    func = (int (*)()) code;
    (int)(*
    
    //__asm call code;*/
    //asm("mov s30,%[value]" : :[value]"r" (loopNo):);
    asm("sub r13, r13, #4");
    asm("str r11, [r13]");
    asm("mov r11, %[value]" : :[value]"r" (loopNo):);
    runOnCGRA();
    asm("ldr r11, [r13]");
    asm("add r13, r13, #4");
    
    //while(thread_cond_cpu != 1) {
    //	usleep(1);
    //  }
    
}

void deleteCGRA()
{
#ifdef DEBUG
  printf("\ndeleting cgra\n");
#endif
  /*
  thread_exit = 1;
  thread_cond_cgra = 1;

  printf("Main thread calling join w/ CGRA thread\n");
  //pthread_join(pth, NULL); 
  //printf("Exiting cgra.c deleteCGRA\n"); */
}


void createCGRA()
{
  #ifdef DEBUG
    printf("createCGRA\n");
  #endif
  
  checkTotalLoops();

  // for splitmap, we have live_in.bin, kernel.bin, live_out.bin, iter.bin

  initCGRA  = (int *)malloc(sizeof(int)*initCGRA_size*totalLoops);
  liveinPtr = (int *)malloc(sizeof(int)*totalLoops);
  kernelPtr = (int *)malloc(sizeof(int)*totalLoops);
  liveoutPtr = (int *)malloc(sizeof(int)*totalLoops);
  iterPtr = (int *)malloc(sizeof(int)*totalLoops);
  livein_size = (unsigned *)malloc(sizeof(unsigned)*totalLoops);
  kernel_size =	(unsigned *)malloc(sizeof(unsigned)*totalLoops);
  liveout_size =	(unsigned *)malloc(sizeof(unsigned)*totalLoops);
  iter_size = (unsigned *)malloc(sizeof(unsigned)*totalLoops);

  readInstructionSize();

  int total_size[4] = {0, 0, 0, 0}; // Size of livein/kernel/iter/liveout for all loops                                                  
  for (unsigned i = 1; i <= totalLoops; i++) {
    total_size[0] += livein_size[i-1];
    total_size[1] += kernel_size[i-1];
    total_size[2] += iter_size[i-1];
    total_size[3] += liveout_size[i-1];
  }

  mallopt(M_MMAP_THRESHOLD, total_size[0] * sizeof(unsigned long long) * 2);  // This tries to avoid memory allocation in stack when requesting over 128kB
  
  livein = (unsigned long long*)malloc(total_size[0]*sizeof(unsigned long long));
  kernel = (unsigned long long*)malloc(total_size[1]*sizeof(unsigned long long));
  iter = (unsigned*)malloc(total_size[2]*sizeof(unsigned));
  liveout = (unsigned long long*)malloc(total_size[3]*sizeof(unsigned long long));

  #ifdef DEBUG
    printf("Total livein: %d - total kernel: %d - total iter: %d - total liveout: %d\n"
          "base livein PC: %lx - base kernel PC: %lx - base iter PC: %lx - base liveout PC: %lx\n", 
          total_size[0], total_size[1], total_size[2], total_size[3], livein, kernel, iter, liveout);
  #endif

  for (unsigned i = 1; i <= totalLoops; i++) {
    initializeParameters(i);
    configureCGRA(i);
  }

  printf("Done loading CGRA @ cycle %d\n", get_clock());
  
  /*int total_size[3] = {0,0,0}; // Size of prolog/kernel/epilog for all loops
  for(i=1; i<=totalLoops; i++){
    total_size[0] += prolog_size[i-1];
    total_size[1] += kernel_size[i-1];
    total_size[2] += epilog_size[i-1];
  }
  
  printf("Total prolog: %d - total kernel: %d - total epilog: %d\n", total_size[0], total_size[1], total_size[2]);

  prolog=(unsigned long long*)malloc(total_size[0]*sizeof(unsigned long long));
  kernel=(unsigned long long*)malloc(total_size[1]*sizeof(unsigned long long));
  epilog=(unsigned long long*)malloc(total_size[2]*sizeof(unsigned long long));*/

    
  //pthread_mutex_init(&mutex, NULL);
  //printf("Main thread calling CGRA thread...\n");
  //asm("mov r8,%[value]" : :[value]"r" (CPU_idle):);

  //printf("ASM CPU_IDLE Instruction completed\n");
  //result = pthread_create(&pth, NULL, (void*) &runOnCGRA, NULL); 
  //printf("\n\nresult = %d\n\n", result);
  //fflush(NULL);
}
