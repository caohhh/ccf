/**
 * CGRA.h
 * Representation for the time extanded CGRA
 * 
 * Cao Heng
 * 2024.3.20
*/

#ifndef __INSGEN_CGRA_H__
#define __INSGEN_CGRA_H__

#include <map>
#include <utility>
#include <vector>
#include <tuple>
#include <set>

#include "Definitions.h"
#include "Node.h"

class PE;


class CGRA
{
  public:
    // default constructor
    CGRA(int xSize, int ySize, int II);

    // set the node for a PE
    void setNode(int t, int x, int y, std::map<int, std::pair<int, int>> nodes);

    // update the operation and data source for each PE
    void updateDataSource(std::vector<Node*> nodeSet, std::map<int, int> constants, std::vector<int> liveIns);

    // generate instruction for each PE
    void generateIns();

    // generate the prologue phi ins
    void generatePrologue();

    // generate load instructions for live ins
    void generateLiveinLoad(std::map<int, uint32_t> liveinAdd, std::map<int, int> liveinAlign);

  private:
    // set of all the time extanded PEs in this CGRA
    std::vector<PE*> peSet;
    // II of this CGRA
    int II;
    // x dimension size
    int xSize;
    // y dimension size
    int ySize;
    // load in istructions
    std::vector<uint64_t> loadInIns;

    // returns the PE a node is mapped to
    PE* getPE(int nodeId);

    // returns the direction of sourcePE according to pe
    direction getDirection(PE* sourcePE, PE* pe);
};


class PE
{
  public:
    // default constructor
    PE(int t, int x, int y);
     
    // set the node id for this PE
    void setNodeId(std::map<int, std::pair<int, int>> nodes);

    // set the node pointer for this PE of given path
    void setNode(nodePath path, Node* node); 

    // returns if there is a node mapped to this PE
    bool hasNode();

    // returns the nodes of this PE int <N, T, F>
    std::map<nodePath, int> getNode();

    // set the source PE
    void setSource(nodePath path, int opOrder, direction dir);

    // set a constant source 
    void setSource(nodePath path, int opOrder, int constSource);

    // returns the coordinate of the PE in <t, x, y>
    std::tuple<int, int, int> getCoord();

    // generate instruction word
    void generateIns();

    // set the live in data register info
    void setLiveIn(std::map<int, int> liveinReg);

    // generate the prologue phi ins
    void generatePrologue();

    // returns the <livein id: reg No.>
    std::map<int, int> getLivein();

    // set the live out data register info
    void setLiveOut(std::map<int, int> liveOutReg);

  private:
    // x coord
    int x;
    // y coord
    int y;
    // t coord
    int t;
    // node ID of a certain path
    std::map<nodePath, int> nodeId;
    // nodes of a certain path
    std::map<nodePath, Node*> nodes;
    // iteration number
    int iter;
    // source direction of each operand
    std::map<nodePath, std::map<int, direction>> sourceDirections;
    // source constants of operaands
    std::map<nodePath, std::map<int, int>> sourceConstants;
    // instruction word for each path
    std::map<nodePath, uint64_t> insWord;
    // the register no. each live in data is stored in
    std::map<int, int> liveinReg;
    // the register no. each live out data is stored in
    std::map<int, int> liveOutReg;
    // if the node is this PE is a phi operation
    bool phiOp;
    // if this PE is a phi op, the different instruction in prologue
    uint64_t proIns;
};



#endif