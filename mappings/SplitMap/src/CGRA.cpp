/**
 * CGRA.cpp
*/

#include "CGRA.h"
#include <iomanip>
#include <algorithm>

/*****************CGRA******************************/
CGRA::CGRA(int xDim, int yDim, int II)
{
  this->xDim = xDim;
  this->yDim = yDim;
  this->II = II;
  for (int t = 0; t < II; t++) {
    for (int x = 0; x < xDim; x++) {
      rowSet.push_back(new Row(x, t));
      for (int y = 0; y < yDim; y++)
        peSet.push_back(new PE(x, y, t));
    }
  }
}


PE*
CGRA::getPeMapped(int nodeId)
{
  for (auto pe : peSet) {
    if (pe->getNode() == nodeId)
      return pe;
  }
  return nullptr;
}


std::vector<PE*>
CGRA::getPeAtTime(int time)
{
  std::vector<PE*> pes;
  if (time > II - 1) {
    FATAL("[CGRA]ERROR! getting PE at time larger than II");
  } else {
    pes.assign(peSet.begin() + time * xDim * yDim, peSet.begin() + (time + 1) * xDim * yDim);
  }
  return pes;
}


std::vector<PE*>
CGRA::getPeAtRow(Row* row)
{
  std::vector<PE*> pes;
  int rowX = std::get<0>(row->getCoord());
  int rowT = std::get<1>(row->getCoord());
  pes.assign(peSet.begin() + rowT * xDim * yDim + rowX * yDim, peSet.begin() + rowT * xDim * yDim + (rowX + 1) * yDim);
  return pes;
}


Row*
CGRA::getRow(int x, int t)
{
  if (!(x < xDim) || !(t < II))
    FATAL("[CGRA]ERROR! Getting Row out of bounds");
  else
    return (rowSet.at(t * xDim + x));
}


PE*
CGRA::getPe(int x, int y, int t)
{
  if (!(x < xDim) || !(y < yDim) || !(t < II)) {
    FATAL("[CGRA]ERROR! Getting PE out of bounds");
  } else {
    return peSet.at(t * (xDim * yDim) + x * yDim + y);
  }
}


std::vector<Row*>
CGRA::getRowAtTime(int time)
{
  std::vector<Row*> rows;
  if (time > II - 1) {
    FATAL("[CGRA]ERROR! getting Row at time larger than II");
  } else {
    rows.assign(rowSet.begin() + time * xDim, rowSet.begin() + (time + 1) * xDim);
  }
  return rows;
}



void
CGRA::print()
{
  for (int time = 0; time < II; time++) {
    std::cout << "Time: " << time << std::endl;
    for (int x = 0; x < xDim; x++) {
      for (int y = 0; y < yDim; y++) {
        PE* pe = getPe(x, y, time);
        std::cout << std::setw(10) << pe->getNode();
      }
      std::cout << std::endl;
    }
  }
}


void
CGRA::print(std::vector<PE*> pes)
{
  for (int time = 0; time < II; time++) {
    std::cout << "Time: " << time << std::endl;
    for (int x = 0; x < xDim; x++) {
      for (int y = 0; y < yDim; y++) {
        PE* pe = getPe(x, y, time);
        if (std::find(pes.begin(), pes.end(), pe) != pes.end())
          std::cout << std::setw(10) << pe->getNode() << "(X)";
        else
          std::cout << std::setw(10) << pe->getNode();
      }
      std::cout << std::endl;
    }
  }
}


bool
CGRA::isAccessable(PE* fromPE, PE* toPE)
{ 
  int fromX = std::get<0>(fromPE->getCoord());
  int fromY = std::get<1>(fromPE->getCoord());
  int fromT = std::get<2>(fromPE->getCoord());

  int toX = std::get<0>(toPE->getCoord());
  int toY = std::get<1>(toPE->getCoord());
  int toT = std::get<2>(toPE->getCoord());
  // for now we only consider PEs with 1 time apart since we've added routing
  if (((fromT + 1) % II) == toT) {
    // time accessable, check physical coord 
    // we only use torus connection now
    if (fromX == toX) {
      if (fromY == toY) {
        // same
        return true;
      } else if (((fromY + 1) % yDim) == toY) {
        // up
        return true;
      } else if (((toY + 1) % yDim) == fromY) {
        // down
        return true;
      }
    } else {
      if (fromY == toY) {
        if (((fromX + 1) % xDim) == toX) {
          // right
          return true;
        } else if (((toX + 1) % xDim) == fromX) {
          // left
          return true;
        }
      }
    }
  }
  return false;
}


void
CGRA::mapNode(Node* node, PE* pe)
{
  pe->mapNode(node->getId());
  if (node->isMemNode()) {
    int xCoor = std::get<0>(pe->getCoord());
    int t = std::get<2>(pe->getCoord());
    Row* row = getRow(xCoor, t);
    row->mapNode(node);
  }
}


void
CGRA::removeNode(Node* node)
{
  PE* mappedPe = getPeMapped(node->getId());
  mappedPe->removeNode(node->getId());
  if (node->isMemNode()) {
    Row* row = getRow(std::get<0>(mappedPe->getCoord()), std::get<2>(mappedPe->getCoord()));
    row->removeNode(node);
  }
}


void
CGRA::clear()
{
  for (PE* pe : peSet)
    pe->clear();
  for (Row* row : rowSet)
    row->clear();
}

/************************PE******************************/
PE::PE(int x, int y, int t)
{
  this->x = x;
  this->y = y;
  this->t = t;
  nodeId = -1;
}


int
PE::getNode()
{
  return nodeId;
}


std::tuple<int, int, int> 
PE::getCoord()
{
  return std::make_tuple(x, y, t);
}


void
PE::mapNode(int nodeId)
{
  this->nodeId = nodeId;
}


void
PE::removeNode(int nodeId)
{
  if (this->nodeId != nodeId)
    FATAL("[PE]ERROR! removing a node not mapped to this PE");
  this->nodeId = -1;
}


void
PE::clear()
{
  nodeId = -1;
}

/************************Row******************************/
Row::Row(int x, int t)
{
  this->x = x;
  this->t = t;
  this->dataNodeId = -1;
  this->addNodeId = -1;
  this->read = false;
  this->write = false;
}


bool
Row::memResAvailable()
{
  if (!read && !write)
    return true;
  else if (read && write)
    FATAL("[Row]Row: <" << x << ", " << t <<"> used for read and write");
  else 
    return false;
}


std::tuple<int, int> 
Row::getCoord()
{
  return std::make_tuple(x, t);
}


void
Row::mapNode(Node* node)
{
  if (!node->isMemNode())
    FATAL("[Row]ERROR! Mapping a non-mem node to a row");
  if (node->isLoadAddressGenerator()) {
    read = true;
    addNodeId = node->getId();
  } else if (node->isLoadDataBusRead()) {
    read = true;
    dataNodeId = node->getId();
  } else if (node->isStoreAddressGenerator()) {
    write = true;
    addNodeId = node->getId();
  } else if (node->isStoreDataBusWrite()) {
    write = true;
    dataNodeId = node->getId();
  }
}


void
Row::removeNode(Node* node)
{
  if (read) {
    read = false;
    if (node->isLoadAddressGenerator()) {
      if (addNodeId == node->getId())
        addNodeId = -1;
      else 
        FATAL("[Row]ERROR! read add node not matching");
    } else if (node->isLoadDataBusRead()) {
      if (dataNodeId == node->getId())
        dataNodeId = -1;
      else 
        FATAL("[Row]ERROR! read data node not matching");
    } else 
      FATAL("[Row]ERROR! read writing not matching");
  } else if (write) {
    if (node->isStoreAddressGenerator()) {
      addNodeId = -1;
      if (dataNodeId == -1)
        write = false;
    } else if (node->isStoreDataBusWrite()) {
      dataNodeId = -1;
      if (addNodeId == -1)
        write = false;
    }
  } else
    FATAL("[Row]ERROR!removing node from a row not mapped");
}


void
Row::clear()
{
  dataNodeId = -1;
  addNodeId = -1;
  read = false;
  write = false;
}