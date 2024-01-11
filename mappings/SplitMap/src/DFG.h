/**
 * DFG.h
 * 
 * Header file for the implementation of the DFG graph
*/

#ifndef __SPLITMAP_DFG_H__
#define __SPLITMAP_DFG_H__

#include <vector>
#include <random>
#include <set>
#include "Node.h"
#include "Arc.h"

class DFG
{
  public:
    DFG();
    virtual ~DFG();

    // returns if node of the given ID exists in the DFG
    bool hasNode(int nodeId);

    //returns true if DFG has a constant with the given ID
    bool hasConstant(int ID);

    // insert a node into the DFG
    void insertNode(Node* node);

    // make an edge between two nodes
    void makeArc(Node* nodeFrom, Node* nodeTo, int distance, DataDepType dep, int opOrder);

    // make an arc that includes a constant
    void makeConstArc(int nodeFromId, int nodeToId, int opOrder);

    //return a node with given ID 
    Node* getNode(int nodeId);

    // returns the arc between the given nodes
    Arc* getArc(Node* nodeFrom, Node* nodeTo);

    // remove the arc of the given Id
    void removeArc(int arcId);
    
    // applies in-degree and and outdegree constraints to the DFG
    void preProcess(unsigned maxInDegree, unsigned maxOutDegree);

    // return the calculated recurrent minimum II of this DFG
    int calculateRecMII();

    // return the node count of this DFG
    int getNodeSize();

    // return the number of load nodes
    int getLoadOpCount();

    // return the number of store nodes
    int getStoreOpCount();

    // returns set of nodes with no parent from current cycle
    std::vector<Node*> getStartNodes();

    // returns set of node indexes of this DFG
    std::set<int> getNodeIdSet();

  private:
    // set of nodes in the graph
    std::vector<Node*> nodeSet;
    // all the constants in the graph
    std::vector<Node*> constants;
    // set of edges in the graph
    std::vector<Arc*> arcSet;
    // set of edges with constants oprands
    std::vector<Const_Arc> constArcSet;
    // count of all the paths in this split DFG
    unsigned pathCount;
    // maximum of the node IDs in the graph
    int nodeMaxId;
    // maximum of the arc IDs in the graph
    int arcMaxId;
    // rng
    std::mt19937 rng;

    /**
     * recurssively get a path the produces the maximum II
     * @param currentNode the node currently on
     * @param destNode the destination to reach by iterating through previous nodes of currentNode
     * @param path the set of nodes that have been visited
     * @param prevLat previously accumulated latency of the path
     * @param prevDist previously accumulated distance of the path
     * @return tuple of found, latency, and distance of the path
    */
    std::tuple<bool, int, int> getPathMaxII(Node* currentNode, Node* destNode, std::set<Node*>& path, int prevLat, int prevDist);
};

#endif