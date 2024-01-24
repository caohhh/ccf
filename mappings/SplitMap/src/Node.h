/**
 * Node.h
 * 
 * Header file for the nodes in DFG graph
*/

#ifndef __SPLITMAP_NODE_H__
#define __SPLITMAP_NODE_H__

#include <string>
#include <vector>
#include <map>
#include "definitions.h"
#include "Arc.h"

class Arc;

class Node
{
  public:
    Node(Instruction_Operation ins, Datatype dt, int laten, int id, std::string nodeName, nodePath brPath, int condBrId);
    ~Node();
    Node(const Node& originalNode);

    // returns the unique ID of the node
    int getId();

    // returns the instruction operation of this node
    Instruction_Operation getIns();

    // returns the data type of this node
    Datatype getDataType();

    //returns true if there is an edge between this node and next node
    bool isConnectedTo(Node* nextNode);

    //return the set of outgoing nodes
    std::vector<Node*> getNextNodes();

    // get previous nodes
    std::vector<Node*> getPrevNodes();

    //returns if the node is connected to itself
    bool hasSelfLoop();

    // sets the node as self loop, and set the self loop arc
    void setSelfLoop(Arc* loopArc);

    //add a new incomming edge
    void addPredArc(Arc* predArc);

    //add a new outgoing edge
    void addSuccArc(Arc* succArc);

    // remove a existing incomming edge
    void removePredArc(int arcId);

    // remove a existing outgoing edge
    void removeSuccArc(int arcId);

    // set the node as load address generator and its related read node
    void setLoadAddressGenerator(Node* readNode);

    // set the node as load read node and its related address generator
    void setLoadDataBusRead(Node* addressNode);

    // set the node as store address generator and its related write node
    void setStoreAddressGenerator(Node* writeNode);

    // set the node as store write node and its related address generator
    void setStoreDataBusWrite(Node* addressNode);

    // set this node as a live out node
    void setLiveOut();

    // set this node as a loop control node
    void setLoopCtrl();

    // get the branch path this node belongs to
    nodePath getBrPath();

    // get the number of incomming edges with true dep
    // with multiple paths, this number is of the path with the maximum number
    unsigned getNumberOfOperands();

    // returns a map of the successor (true + pred) count of each path
    std::map<nodePath, unsigned> getSuccSameIterNum();

    //return all successors (true + pred) in same iteration
    std::vector<Node*> getSuccSameIterNodes();

    //return successors (true + pred) in same iteration of given path
    std::vector<Node*> getSuccSameIterNodes(nodePath path);
    
    //return successors with distance
    std::vector<Node*> getSuccNextIterNodes();

    //return the latency for this node
    int getLatency();

    // return if the node is a load read node
    bool isLoadDataBusRead();

    // return if the node is load address generator
    bool isLoadAddressGenerator();

    // return if the node is store address generator
    bool isStoreAddressGenerator();

    // return if the node is a store write node
    bool isStoreDataBusWrite();

    // get all predecessor nodes of the same iteration
    std::vector<Node*> getPrevSameIter();

    //return predecessors of the same iteration excluding load store address dependencies 
    std::vector<Node*> getPrevSameIterExMemDep();

    // get all successor nodes of the same iteration
    std::vector<Node*> getNextSameIter();

    //return successors with distance = 0 excluding load store address dependency
    std::vector<Node*> getSuccSameIterExMemDep();

    // return predecessors with true + pred dependency and distance
    std::vector<Node*> getExDepPredPrevIter();

    // return predecessors with true + pred dependency and distance
    std::vector<Node*> getExDepSuccNextIter();

    // get all nodes connected with inter-iter dependency arc
    std::vector<Node*> getInterIterRelatedNodes();

    // get the store or load memory related node
    Node* getMemRelatedNode();

    // return if the node is a live out node
    bool isLiveOut();

    // return if the node is a loop control node
    bool isLoopCtrl();

  private:
    // unique id of Node in DFG
    int uid;
    // instruction of the node
    Instruction_Operation ins;
    // data type of the operation
    Datatype dataType;
    // num of cycles for this operation
    int latency;
    // name for the node
    std::string name;
    // which branch this node is used as a condition for, -1 for not a cond
    int condBrId;
    // the branch path this node belongs to
    nodePath brPath;
    // node is connected to self
    bool selfLoop;
    // set of arcs to this node 
    std::vector<Arc*> incommingArcs;
    // set of arcs from this node
    std::vector<Arc*> outgoingArcs;
    // the arc from and to this node
    Arc* selfLoopArc;
    // this node is used as load address generator, outputs to address bus
    bool loadOutputAddressBus;
    // this node is used as load data read, reads data bus input
    bool inputDataBus;
    // this node is used as store address generator, outputs to address bus
    bool storeOutputAddressBus;
    // this node is used as store data write, writes data bus output
    bool outputDataBus;
    // for load or store, the address bus or data bus node related
    Node* memRelatedNode;
    // if this node is a live out node
    bool liveOut;
    // if this node is a loop control node
    bool loopCtrl;
};


class routeNode : public Node
{
  public:
    routeNode(int uid, Node* prevNode, nodePath path);
    ~routeNode();
};

#endif