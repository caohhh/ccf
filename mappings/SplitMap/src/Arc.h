/**
 * Arc.h
 * 
 * Header file for the arcs in DFG graph
*/

#ifndef __SPLITMAP_ARC_H__
#define __SPLITMAP_ARC_H__

#include "Node.h"
#include "DFG.h"

class Node;
class DFG;

class Arc
{
  public:
    Arc(Node* fromNode, Node* toNode, int ID, int distance, DataDepType dep, int opOrder, nodePath brPath);
    ~Arc();
    // constructor to copy existing arc to the new DFG
    Arc(const Arc& originalArc, Node* newFromNode, Node* newToNode);

    //Get the node that this edge is its incoming edge
    Node* getToNode();

    //Get the node that this edge is its outgoing edge
    Node* getFromNode();

    // get the dependency type of this edge
    DataDepType getDependency();
    
    // get the operand order of this edge
    int getOperandOrder();

    // returns the edge distance
    int getDistance();

    // return the unique ID of this arc
    int getId();

    // return the branch path of this arc
    nodePath getPath();

  private:
    // edge from inNode to outNode
    Node* from;
    Node* to;
    int id;
    int distance;
    DataDepType dependency;
    int operandOrder;
    nodePath brPath;
};

struct Const_Arc
{
  int fromNodeId;
  int toNodeId;
  int opOrder;
  nodePath brPath;
};

#endif