/**
 * Mapper.h
 * 
 * This program finds a mapping using modified Falcon's algorithm.
 * Diverting from the previously used Clique based algorithm, this
 * method checks for a random mapping of each node based on MRRG. 
 * On a failure to map the node, Falcon tries to map the node, by
 * remapping some nodes in the timeslots.
*/

#ifndef __SPLITMAP_MAPPER_H__
#define __SPLITMAP_MAPPER_H__

#include "Parser.h"

class Mapper
{
  public:
    Mapper(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy);
    ~Mapper();
    
    // map with falcon algo, returns if mapping is successful
    bool generateMap(Parser* myParser);

  private:
    // target cgra architecture
    CGRA_Architecture cgraInfo;
    // mapping related restrictions and policies
    Mapping_Policy mappingPolicy;
    // number of PEs in the CGRA
    unsigned cgraSize;
    // the ASAP schedule of nodeId:time
    std::map<int, int> asapSchedule;
    // the ALAP schedule of nodeId:time
    std::map<int, int> alapSchedule;

    struct schedule
    {
      // schedule of nodeID : time
      std::map<int, int> nodeSchedule;
      struct ts
      {
        // id of nodes scheduled at time
        std::vector<int> nodes;
        unsigned peUsed = 0;
        unsigned addBusUsed = 0;
        unsigned dataBusUsed = 0;
      };
      // schedule of given time
      std::map<int, ts> timeSchedule;
    };
    
    // the ASAP schedule considering resources available
    schedule asapFeasible;

    /**
     * Schedule operations as soon as all predecessors are completed
     * @param myDFG the DFG to be scheduled
     * @return the length of the schedule
    */
    int scheduleASAP(DFG* myDFG);

    /**
     * Schedule operations as late as all successors are luanched
     * @param myDFG the DFG to be scheduled
     * @param length 
    */
    void scheduleALAP(DFG* myDFG, int length);

    /**
     * schedule operations in ASAP manner considering number of available resources
     * @param myDFG the DFG to be scheduled
     * @return the length of the schedule
    */
    int scheduleASAPFeasible(DFG* myDFG);

    // returns if the given schedule has enough memory resources for a load at given time 
    bool memLdResAvailable(int time, schedule sch);

    // returns if the given schedule has enough memory resources for a store at given time 
    bool memStResAvailable(int time, schedule sch);

    /**
     * For a given schedule, return if the given node has inter
     * iteration related nodes mapped at the same time
    */
    bool hasInterIterConfilct(Node* node, int time, schedule sch);

    /**
     * check if a node is ready for asap scheduling, as in all
     * previous nodes have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkASAP(Node* node, schedule asapSchedule);
};

#endif