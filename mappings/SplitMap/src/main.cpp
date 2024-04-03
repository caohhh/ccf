/**
 * main.cpp
 * 
 * Modified on FalconCrimson. In LLVM end, using Multi-DDG Gen to 
 * generate a split graph based on control flow. Using this mapping
 * to generate a mapping with multiple IIs.
*/

#include <string>
#include <vector>
#include <cstring>
#include "definitions.h"
#include "Parser.h"
#include "Mapper.h"

int main(int argc, char *argv[])
{
  CGRA_Architecture cgraInfo;
  // setup cgra configuration
  cgraInfo.MAX_NODE_INDEGREE = 2;
  cgraInfo.MAX_NODE_OUTDEGREE = 5;

  Mapping_Policy mappingPolicy;
  // setup mapping configurations
  mappingPolicy.CLIQUE_ATTEMPTS = 15;
  mappingPolicy.MAX_LATENCY = 2000;
  mappingPolicy.ENABLE_REGISTERS = true;
  mappingPolicy.MAX_SCHED_TRY = 1;
  mappingPolicy.MAX_II = 50;
  mappingPolicy.LAMBDA = 0.02;
  mappingPolicy.MODULO_SCHEDULING_ATTEMPTS = 1000;
  mappingPolicy.MAX_MAPPING_ATTEMPTS = 500;
  mappingPolicy.MAPPING_ATTEMPTS_PER_II = 10;
  mappingPolicy.MAPPING_MODE = 0;


  // Default inputs and settings
  cgraInfo.X_Dim = 4;
  cgraInfo.Y_Dim = 4;
  cgraInfo.R_Size = 4;
  cgraInfo.PER_ROW_MEM_AVAILABLE = 1;

  std::string node_file;
  std::string edge_file;
  // process program args
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (std::strcmp(argv[i], "-NODE") == 0) {
        node_file = argv[i + 1];
      } else if (std::strcmp(argv[i], "-EDGE") == 0) {
        edge_file = argv[i + 1];
      } else if (std::strcmp(argv[i], "-X") == 0) {
        cgraInfo.X_Dim = atoi(argv[i + 1]);
      } else if (std::strcmp(argv[i], "-Y") == 0) {
        cgraInfo.Y_Dim = atoi(argv[i + 1]);
        if (cgraInfo.Y_Dim < 2)
          FATAL("[main]ERROR! Y dimension need to be at least 2");
      } else if (std::strcmp(argv[i], "-R") == 0) {
        cgraInfo.R_Size = atoi(argv[i + 1]);
        if (cgraInfo.R_Size == 0)
          mappingPolicy.ENABLE_REGISTERS = false;
      } else if (std::strcmp(argv[i], "-MAPII") == 0) {
        mappingPolicy.MAPPING_ATTEMPTS_PER_II = atoi(argv[i + 1]);
      } else if (std::strcmp(argv[i], "-MSA") == 0) {
        mappingPolicy.MODULO_SCHEDULING_ATTEMPTS = atoi(argv[i + 1]);
      } else if (std::strcmp(argv[i], "-MAX_MAP") == 0) {
        mappingPolicy.MAX_MAPPING_ATTEMPTS = atof(argv[i + 1]);
      } else if (std::strcmp(argv[i], "-MAP_MODE") == 0) {
        mappingPolicy.MAPPING_MODE = atoi(argv[i + 1]);
      } else if (std::strcmp(argv[i], "-MAX_II") == 0) {
        mappingPolicy.MAX_II = atoi(argv[i + 1]);
      } else if (std::strcmp(argv[i], "-LAMBDA") == 0) {
        mappingPolicy.LAMBDA = atof(argv[i + 1]);
        if (mappingPolicy.LAMBDA > 1)
          FATAL("LAMBDA is percetntage and should be less than 1\n");      
      } else {
        FATAL("Unrecognized argument" << std::string(argv[i]));
      } 
    }
  }
  if (node_file.size() == 0 || edge_file.size() == 0)
    FATAL("ERROR!! Must include node file and edge file");
  DEBUG("[main]Parsed all arguments");
  if(cgraInfo.X_Dim == 1 && cgraInfo.Y_Dim == 1) {
    cgraInfo.MAX_PE_INDEGREE = 1; 
    cgraInfo.MAX_PE_OUTDEGREE = 1;
  } else {
    cgraInfo.MAX_PE_INDEGREE = cgraInfo.X_Dim + 2; 
    cgraInfo.MAX_PE_OUTDEGREE = cgraInfo.X_Dim + 2; 
  }
  
  // Number values that can be transfered via a timeslot
  cgraInfo.MAX_DATA_PER_TIMESLOT = cgraInfo.X_Dim * cgraInfo.Y_Dim;

  //Parser processes input files
  Parser* myParser = new Parser(node_file.c_str(), edge_file.c_str());

  DEBUG("[main]Mapping has started");
  bool mapSuccess = false;
  Mapper* falconMapper = new Mapper(cgraInfo, mappingPolicy);
  mapSuccess = falconMapper->generateMap(myParser);
  
  if (mapSuccess) {
    DEBUG("[main]Mapping is completed and successfull");
    falconMapper->dumpKernel();
  }
  else
    FATAL("[main]Sorry no mapping was found for max II " << mappingPolicy.MAX_II); 
  return 0;
}
