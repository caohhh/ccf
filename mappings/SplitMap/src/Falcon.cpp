/**
 * Falcon.cpp
*/

#include "Falcon.h"


Falcon::Falcon(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy)
{
  this->cgraInfo = cgraInfo;
  this->mappingPolicy = mappingPolicy;
}


Falcon::~Falcon()
{
}


bool
Falcon::generateMap(Parser* myParser)
{
  //read input files and construct DFG
  DFG* originalDFG= new DFG();
  myParser->ParseDFG(originalDFG);
  DEBUG("Done parsing DFG");

  originalDFG->preProcess(cgraInfo.MAX_NODE_INDEGREE, cgraInfo.MAX_NODE_OUTDEGREE);
  DEBUG("Done preprocessing DFG");

  // get the recMII
  int recMII  = originalDFG->calculateRecMII();
  DEBUG("Done calculation RecMII of the DFG as " << recMII);

  // resMII
  int cgraSize = cgraInfo.X_Dim * cgraInfo.Y_Dim;
  int resMII = ceil(float(originalDFG->getNodeSize()) / cgraSize);
  DEBUG("Done calculating resMII as " << resMII);

  // find memory resource MII
  int memMII = ceil(float(originalDFG->getLoadOpCount() + originalDFG->getStoreOpCount()) / cgraInfo.Y_Dim);
  DEBUG("Done calculating memMII as " << memMII);

  int MII = recMII;
  if (resMII > MII)
    MII = resMII;
  if(memMII > MII)
    MII = memMII;
  DEBUG("MII of the schedule is " << MII);

  return true;
}