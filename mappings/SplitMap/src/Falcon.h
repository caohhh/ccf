/**
 * Falcon.h
 * 
 * This program finds a mapping using modified Falcon's algorithm.
 * Diverting from the previously used Clique based algorithm, this
 * method checks for a random mapping of each node based on MRRG. 
 * On a failure to map the node, Falcon tries to map the node, by
 * remapping some nodes in the timeslots.
*/

#ifndef __SPLITMAP_FALCON_H__
#define __SPLITMAP_FALCON_H__

#include "Parser.h"

class Falcon
{
  public:
    Falcon(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy);
    ~Falcon();
    
    // map with falcon algo, returns if mapping is successful
    bool generateMap(Parser* myParser);

  private:
    // target cgra architecture
    CGRA_Architecture cgraInfo;
    // mapping related restrictions and policies
    Mapping_Policy mappingPolicy;

    /**
     * Schedule operations as soon as all predecessors are completed
     * @param myDFG the DFG to be scheduled
     * @return the length of the schedule
    */
    int scheduleASAP(DFG* myDFG);
};

#endif