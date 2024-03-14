/**
 * CGRA.h
 * 
 * For a representation of the CGRA in mapping
*/
#ifndef __SPLITMAP_CGRA_H__
#define __SPLITMAP_CGRA_H__

#include <vector>
#include <tuple>
#include "definitions.h"
#include "Node.h"

class PE;
class Row;

class CGRA
{
  public:
    CGRA(int xDim, int yDim, int II);
    ~CGRA();

    // return the PE this node is mapped at
    PE* getPeMapped(Node* node);

    // return the PE at the given coordinate
    PE* getPe(int x, int y, int t);

    // return all the time extanded PEs at given time
    std::vector<PE*> getPeAtTime(int time);

    // return all the PEs at given time extanded row
    std::vector<PE*> getPeAtRow(Row* row);

    // return all the rows at given time
    std::vector<Row*> getRowAtTime(int time);

    // return the row at the given coordinate
    Row* getRow(int x, int t);

    // print the current mapping of the CGRA
    void print();

    // returns if toPE can access data produced by fromPE
    bool isAccessable(PE* fromPE, PE* toPE);

    // map a node at the given PE
    void mapNode(Node* node, PE* pe, int iter);

    // remove a mapped node
    void removeNode(Node* node);

    // clear all mapped nodes
    void clear();

  private:
    // size x of this CGRA, as in number of rows
    int xDim;
    // size y of this CGRA, as in number of columns
    int yDim;
    // II of the time extanded CGRA
    int II;
    // vector of all the time extanded PEs
    std::vector<PE*> peSet;
    // rows are used to store memory resource usage
    std::vector<Row*> rowSet;
};

class PE
{
  public:
    PE(int x, int y, int t);
    ~PE();

    // returns the <x, y, t> coordinate of the PE
    std::tuple<int, int, int> getCoord();

    // map a node at the PE
    void mapNode(Node* node, int iter);
    
    // remove a node with the ID at the PE
    void removeNode(int nodeId);

    // remove mapped node
    void clear();

    // if the PE is available to map a node in path at the given iter
    bool available(nodePath path, int iter);

    // get the id of the node mapped to a certain path, -1 for no node mapped
    int getNode(nodePath path);

    // returns the iteration number of the given path
    int getIter(nodePath path);

  private:
    // x coordinate of the PE
    int x;
    // y coordinate of the PE
    int y;
    // t coordinate of the time extanded PE
    int t;
    // map of <path: <nodeId, node iter>>
    std::map<nodePath, std::tuple<int, int>> mappedNodes;
};

class Row
{
  public:
    Row(int y, int t);
    ~Row();

    // returns if this row's address bus is available of a certain path
    bool addAvailable(nodePath path, int iter);

    // returns if this row's data bus is available of a certain path
    bool dataAvailable(nodePath path, int iter);

    // returns the <x, t> coordinate of the row
    std::tuple<int, int> getCoord();

    // map a mem node at the given row
    void mapNode(Node* node, int iter);
    
    // remove a mem node at the given row
    void removeNode(Node* node);

    // reset the row
    void clear();

  private:
    // x coordinate of the row
    int x;
    // t coordinate of the row
    int t;
    // id for the data node, map of <path: <nodeId, node iter>>
    std::map<nodePath, std::tuple<int, int>> dataNodeId;
    // id for the address node, map of <path: <nodeId, node iter>>
    std::map<nodePath, std::tuple<int, int>> addNodeId;
};

#endif //__SPLITMAP_CGRA_H__