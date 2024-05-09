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
#include "CGRA.h"

class Mapper
{
  public:
    Mapper(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy);
    ~Mapper();
    
    // map with falcon algo, returns if mapping is successful
    bool generateMap(Parser* myParser);

    // with mapping done, output the finished kernel mapping
    void dumpKernel();

  private:
    // target cgra architecture
    CGRA_Architecture cgraInfo;
    // mapping related restrictions and policies
    Mapping_Policy mappingPolicy;
    // number of PEs in the CGRA
    unsigned cgraSize;
    // rng
    std::mt19937 rng;
    // cgra with the finished mapping
    CGRA* finishedCGRA;
    // the final DFG
    DFG* finalDFG;


    /**
     * Schedule operations as soon as all predecessors are completed
     * @param myDFG the DFG to be scheduled
     * @return <schedule length, asapSchedule>
    */
    std::tuple<int, std::map<int, int>> scheduleASAP(DFG* myDFG);

    /**
     * Schedule operations as late as all successors are luanched
     * @param myDFG the DFG to be scheduled
     * @param length 
     * @return alapSchedule
    */
    std::map<int, int> scheduleALAP(DFG* myDFG, int length);

    /**
     * schedule operations in ASAP manner considering number of available resources
     * @param myDFG the DFG to be scheduled
     * @param asapFeasible the schedule
     * @return the length of the schedule
    */
    int scheduleASAPFeasible(DFG* myDFG, schedule* asapFeasible);

    // schedule operations in ALAP manner considering number of available resources
    // this step is carried out after an asap feasible schedule is done
    bool scheduleALAPFeasible(DFG* myDFG, int length, schedule* alapFeasible, schedule* asapFeasible);

    /**
     * check if a node is ready for asap scheduling, as in all
     * predecessors  have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkASAP(Node* node, schedule* asapFeasible);

    /**
     * check if a node is ready for alap scheduling, as in all
     * succssors have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkALAP(Node* node, int length, schedule* alapFeasible);

    // returns all the nodes in the DFG sorted in priority of mapping
    // as in longer cycles -> shorter cycles -> nodes not in cycle
    std::vector<Node*> getSortedNodes(DFG* myDFG, schedule* asapFeasible);

    /**
     * check if a node is ready for modulo scheduling, as in all
     * successor  have been scheduled
     * @return tuple of if the given node is ready to be scheduled
     * and the scheduled time
    */
    std::tuple<bool, int> checkModulo(Node* node, moduloSchedule* modSchedule, schedule* alapFeasible);

    // returnes the earliest time slot this node can be placed in modulo scheduling
    int getModConstrainedTime(Node* node, moduloSchedule* modSchedule, schedule* asapFeasible);

    // modulo scheduling of the DFG
    bool scheduleModulo(DFG* myDFG, std::vector<Node*> sortedNodes, int II, 
                        moduloSchedule* modSchedule, schedule* asapFeasible, schedule* alapFeasible);

    // based on the finished modulo schedule, insert routing nodes
    bool insertRoute(DFG* myDFG, moduloSchedule* modSchedule);

    // the falcon mapping algorithm
    bool falconPlace(DFG* myDFG, int II, CGRA* timeExCgra, moduloSchedule* modSchedule);

    // returns the potential PEs this node can be mapped to
    std::vector<PE*> getPotentialPos(Node* node, CGRA* timeExCgra, moduloSchedule* modSchedule);

    // attempt to remap mapped predecesors and successors to find a
    // mapping for a node without mapping position
    // returns if a valid position is found
    bool remapBasic(Node* failedNode, CGRA* timeExCgra, moduloSchedule* modSchedule);

    /******the following 2 remap probably need adjustment to max remap attempt******/

    // try to remap all nodes in the same time slot as failed node 
    // and find a place for failed node
    // returns if remap is successful
    bool remapCurrT(Node* failedNode, DFG* myDFG, CGRA* timeExCgra, moduloSchedule* modSchedule);

    // remapping adjacent time slots and try to find a position for the failed node
    bool remapAdjT(Node* failedNode, DFG* myDFG, CGRA* timeExCgra, moduloSchedule* modSchedule);
};

#endif