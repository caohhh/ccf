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

class PE;
class Row;

class CGRA
{
  public:
    CGRA(int xDim, int yDim, int II);
    ~CGRA();

    // return the PE this node is mapped at
    PE* getPeMapped(int nodeId);

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

    // print the current mapping of the CGRA with the provided PEs marked
    void print(std::vector<PE*>);

    // returns if toPE can access data produced by fromPE
    bool isAccessable(PE* fromPE, PE* toPE);

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

    // returns the uid of the node mapped at the PE, -1 for unmapped
    int getNode();

    // returns the <x, y, t> coordinate of the PE
    std::tuple<int, int, int> getCoord();

    // map a node with the ID at the PE
    void mapNode(int nodeId);

  private:
    // x coordinate of the PE
    int x;
    // y coordinate of the PE
    int y;
    // t coordinate of the time extanded PE
    int t;
    // id of the node mapped here, -1 for no node mapped
    int nodeId;
};

class Row
{
  public:
    Row(int y, int t);
    ~Row();

    // returns if this row's memory resource is available
    bool memResAvailable();

    // returns the <x, t> coordinate of the row
    std::tuple<int, int> getCoord();
    
  private:
    // x coordinate of the row
    int x;
    // t coordinate of the row
    int t;
    // id for the data node, -1 for no node
    int dataNodeId;
    // id for the address node, -1 for no node
    int addNodeId;
    // if this row is used for mem read
    bool read;
    // if this row is used for mem write
    bool write;
};

#endif //__SPLITMAP_CGRA_H__