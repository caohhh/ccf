/**
 * CGRA.cpp
*/

#include "CGRA.h"
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <fstream>

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
CGRA::getPeMapped(Node* node)
{
  for (auto pe : peSet) {
    if (pe->getNode(node->getBrPath()) == node->getId())
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
        std::cout << std::setw(15);
        std::string os;
        bool mapped = false;
        if (pe->getNode(none) != -1) {
          os += std::to_string(pe->getNode(none)) + "(N"+ std::to_string(pe->getIter(none)) +")";
          mapped = true;
        }
        if (pe->getNode(true_path) != -1) {
          os += std::to_string(pe->getNode(true_path)) + "(T"+ std::to_string(pe->getIter(true_path)) +")";
          mapped = true;
        }
        if (pe->getNode(false_path) != -1) {
          os += std::to_string(pe->getNode(false_path)) + "(F"+ std::to_string(pe->getIter(false_path)) +")";
          mapped = true;
        }
        if (!mapped)
          os = "S";
        std::cout << os;
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
CGRA::mapNode(Node* node, PE* pe, int iter)
{
  pe->mapNode(node, iter);
  if (node->isMemNode()) {
    int xCoor = std::get<0>(pe->getCoord());
    int t = std::get<2>(pe->getCoord());
    Row* row = getRow(xCoor, t);
    row->mapNode(node, iter);
  }
}


void
CGRA::removeNode(Node* node)
{
  PE* mappedPe = getPeMapped(node);
  if (mappedPe == nullptr)
    return;
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


void
CGRA::dump(std::string cgraFile)
{
  std::ofstream dumpFile;
  dumpFile.open(cgraFile);
  dumpFile << II << std::endl;
  for (PE* pe : peSet) {
    dumpFile << pe->dump();
  }
  dumpFile.close();
}

/************************PE******************************/
PE::PE(int x, int y, int t)
{
  this->x = x;
  this->y = y;
  this->t = t;
  mappedNodes[none] = std::make_tuple(-1, -1);
  mappedNodes[true_path] = std::make_tuple(-1, -1);
  mappedNodes[false_path] = std::make_tuple(-1, -1);
}


std::tuple<int, int, int> 
PE::getCoord()
{
  return std::make_tuple(x, y, t);
}


void
PE::mapNode(Node* node, int iter)
{
  mappedNodes[node->getBrPath()] = std::make_tuple(node->getId(), iter);
}


void
PE::removeNode(int nodeId)
{ 
  bool removed = false;
  for (auto mappedNodesIt : mappedNodes) {
    if (std::get<0>(mappedNodesIt.second) == nodeId) {
      mappedNodes[mappedNodesIt.first] = std::make_tuple(-1, -1);
      removed = true;
      break;
    }
  }
  if (!removed)
    FATAL("[PE]ERROR! removing a node not mapped to this PE");
}


void
PE::clear()
{
  mappedNodes[none] = std::make_tuple(-1, -1);
  mappedNodes[true_path] = std::make_tuple(-1, -1);
  mappedNodes[false_path] = std::make_tuple(-1, -1);
}


bool
PE::available(nodePath path, int iter)
{
  // first check for none path
  if (std::get<0>(mappedNodes[none]) != -1)
    return false;
  // now based on the given path
  nodePath oppoPath;
  if (path == none) {
    // first the none path
    if (std::get<0>(mappedNodes[true_path]) == -1 && std::get<0>(mappedNodes[false_path]) == -1)
      return true;
    else
      return false;
  } else if (path == true_path)
    oppoPath = false_path;
  else
    oppoPath = true_path;
  
  // for true or false path
  if (std::get<0>(mappedNodes[path]) != -1)
    return false;

  // none path and path empty, check opposite path
  if (std::get<0>(mappedNodes[oppoPath]) == -1)
    return true;
  else {
    if (std::get<1>(mappedNodes[oppoPath]) == iter) {
      // the mapped node of opposite path is of the same iter, can be safely mapped to the same PE
      return true;
    } else
      return false;
  }
}

int
PE::getNode(nodePath path)
{
  return std::get<0>(mappedNodes[path]);
}

int
PE::getIter(nodePath path)
{
  return std::get<1>(mappedNodes[path]);
}


std::string
PE::dump()
{
  std::string info;
  for (nodePath path : {none, true_path, false_path}) {
    // node ID
    info += std::to_string(std::get<0>(mappedNodes[path])) + "\t";
    // iteration
    info += std::to_string(std::get<1>(mappedNodes[path])) + "\n";
  }
  return info;
}
/************************Row******************************/
Row::Row(int x, int t)
{
  this->x = x;
  this->t = t;
  dataNodeId[none] = std::make_tuple(-1, -1);
  dataNodeId[true_path] = std::make_tuple(-1, -1);
  dataNodeId[false_path] = std::make_tuple(-1, -1);

  addNodeId[none] = std::make_tuple(-1, -1);
  addNodeId[true_path] = std::make_tuple(-1, -1); 
  addNodeId[false_path] = std::make_tuple(-1, -1);
}


bool
Row::addAvailable(nodePath path, int iter)
{
  // first check for none path
  if (std::get<0>(addNodeId[none]) != -1)
    return false;
  // now based on the given path
  nodePath oppoPath;
  if (path == none) {
    // first the none path
    if (std::get<0>(addNodeId[true_path]) == -1 && std::get<0>(addNodeId[false_path]) == -1)
      return true;
    else
      return false;
  } else if (path == true_path)
    oppoPath = false_path;
  else
    oppoPath = true_path;
  
  // for true or false path
  if (std::get<0>(addNodeId[path]) != -1)
    return false;

  // none path and path empty, check opposite path
  if (std::get<0>(addNodeId[oppoPath]) == -1)
    return true;
  else {
    if (std::get<1>(addNodeId[oppoPath]) == iter) {
      // the mapped node of opposite path is of the same iter, can be safely mapped to the same PE
      return true;
    } else
      return false;
  }
}


bool
Row::dataAvailable(nodePath path, int iter)
{
  // first check for none path
  if (std::get<0>(dataNodeId[none]) != -1)
    return false;
  // now based on the given path
  nodePath oppoPath;
  if (path == none) {
    // first the none path
    if (std::get<0>(dataNodeId[true_path]) == -1 && std::get<0>(dataNodeId[false_path]) == -1)
      return true;
    else
      return false;
  } else if (path == true_path)
    oppoPath = false_path;
  else
    oppoPath = true_path;
  
  // for true or false path
  if (std::get<0>(dataNodeId[path]) != -1)
    return false;

  // none path and path empty, check opposite path
  if (std::get<0>(dataNodeId[oppoPath]) == -1)
    return true;
  else {
    if (std::get<1>(dataNodeId[oppoPath]) == iter) {
      // the mapped node of opposite path is of the same iter, can be safely mapped to the same PE
      return true;
    } else
      return false;
  }
}


std::tuple<int, int> 
Row::getCoord()
{
  return std::make_tuple(x, t);
}


void
Row::mapNode(Node* node, int iter)
{
  if (!node->isMemNode())
    FATAL("[Row]ERROR! Mapping a non-mem node to a row");
  nodePath path = node->getBrPath();
  if (node->isLoadAddressGenerator()) {
    addNodeId[path] = std::make_tuple(node->getId(), iter);
  } else if (node->isLoadDataBusRead()) {
    dataNodeId[path] = std::make_tuple(node->getId(), iter);
  } else if (node->isStoreAddressGenerator()) {
    addNodeId[path] = std::make_tuple(node->getId(), iter);
  } else if (node->isStoreDataBusWrite()) {
    dataNodeId[path] = std::make_tuple(node->getId(), iter);
  }
}


void
Row::removeNode(Node* node)
{
  nodePath path = node->getBrPath();
  if (node->isLoadAddressGenerator() || node->isStoreAddressGenerator()) {
    // node is add gen
    if (std::get<0>(addNodeId[path]) != node->getId())
      FATAL("[Row]ERROR! Removing a not mapped address node <" << t << ", " << x << ">, node: " << std::get<0>(addNodeId[path]));
    else
      addNodeId[path] = std::make_tuple(-1, -1);
  } else if (node->isLoadDataBusRead() || node->isStoreDataBusWrite()) {
    // node is data node
    if (std::get<0>(dataNodeId[path]) != node->getId())
      FATAL("[Row]ERROR! Removing a not mapped data node <" << t << ", " << x << ">, node: " << std::get<0>(dataNodeId[path]));
    else
      dataNodeId[path] = std::make_tuple(-1, -1);
  } else
    FATAL("[Row]ERROR! Removing a non-mem node");
}


void
Row::clear()
{
  dataNodeId[none] = std::make_tuple(-1, -1);
  dataNodeId[true_path] = std::make_tuple(-1, -1);
  dataNodeId[false_path] = std::make_tuple(-1, -1);

  addNodeId[none] = std::make_tuple(-1, -1);
  addNodeId[true_path] = std::make_tuple(-1, -1); 
  addNodeId[false_path] = std::make_tuple(-1, -1);
}