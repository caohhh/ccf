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
#include "Schedule.h"

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
    // rng
    std::mt19937 rng;
    
    // the ASAP schedule considering resources available
    schedule* asapFeasible;
    // the ALAP schedule considering resources available
    schedule* alapFeasible;
    // the modulo schedule
    moduloSchedule* modSchedule;

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

    // schedule operations in ALAP manner considering number of available resources
    void scheduleALAPFeasible(DFG* myDFG, int II);

    /**
     * check if a node is ready for asap scheduling, as in all
     * predecessors  have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkASAP(Node* node);

    /**
     * check if a node is ready for alap scheduling, as in all
     * succssors have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkALAP(Node* node, int II);

    // returns all the nodes in the DFG sorted in priority of mapping
    // as in longer cycles -> shorter cycles -> nodes not in cycle
    std::vector<Node*> getSortedNodes(DFG* myDFG);

    /**
     * check if a node is ready for modulo scheduling, as in all
     * successor  have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkModulo(Node* node);


    // returnes the earliest time slot this node can be placed in modulo scheduling
    int getModConstrainedTime(Node* node);

    // returns if source node is constrained by its given predcessor in modulo scheduling
    bool isModConstrainedBy(Node* source, Node* pred);

    // modulo scheduling of the DFG
    bool scheduleModulo(DFG* myDFG, std::vector<Node*> sortedNodes, int II);

};

#endif