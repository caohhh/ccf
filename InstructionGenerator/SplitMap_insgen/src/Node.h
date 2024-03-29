/**
 * Node.h
 * Header file for node class.
 * 
 * Cao Heng
 * 2024.3.18
*/

#ifndef __INSGEN_NODE_H__
#define __INSGEN_NODE_H__

#include "Definitions.h"
#include <vector>
#include <utility>
#include <map>


class Node
{
  public:
    // constructor
    Node(int id, Instruction_Operation op);

    // set the operands of this node
    void setOp(int opOrder, std::pair<int, int> oprands);

    // returns the id of this node
    int getId();

    // returns the operation of this node
    Instruction_Operation getOp();

    // returns the data sources of this node
    std::map<int, std::pair<int, int>> getDataSource();

    // a check for if the data source count is correct
    void checkSource();

    // returns a node with the given ID
    static Node* getNode(std::vector<Node*> nodeSet, int nodeId);

    // set what branch this node is used as a condition for
    void setCondBr(int condBr);

    // set the node as the loop exit control node
    void setLoopExit(bool le);

    // seet the node as the split condition node
    void setSplitCond(bool splitCond);

    // if this node is ptype node
    bool isPType();

    // if this node is ctype node
    bool isCType();

    // if this node is a phi node
    bool isPhi();

    // set the exit condition for a le node
    void setExitCond(bool exitCond);

    // if this node is the loop exit control node
    bool isLoopExit();

    // if this node is the split condition node
    bool isSplitCond();

    // returns what branch this node is used as a condition for, -1 for not a cond
    int getCondBr();

    // returns the exit condition
    bool getExitCond();

    // set the alignment for a address generator
    void setAlignment(int alignment);

    // returns the data alignment bytes
    int getAlignment();

    // set the node as a liveout node
    void setLiveOut(int liveOutId);

    // if the node is a live out node
    bool isLiveOut();

    // get the liveout ID for a live out node
    int getLiveOut();

  private:
    // unique ID for this node
    int uid;
    // operation of this node
    Instruction_Operation op;
    // ID of the data source of this node
    std::map<int, std::pair<int, int>> dataSource;
    // what branch this node is used as a condition for, -1 for not a cond
    int condBr;
    // if this node is used as the loop exit control node
    bool leCtrl;
    // the exit condition if this node is the loop exit control node
    bool exitCond;
    // if this node is the split condition
    bool splitCond;
    // data alignment used for address generators 
    int alignment;
    // if this node is a live out
    bool liveOut;
    // if this node is a live out, it's associated live out ID
    int liveOutId;
};



#endif //__INSGEN_NODE_H__