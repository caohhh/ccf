/**
 * Node.cpp
*/

#include "Node.h"
#include <algorithm>

Node::Node(Instruction_Operation ins, Datatype dt, int laten, int id, std::string nodeName, nodePath brPath, int condBrId)
{
  this->ins = ins;
  latency = laten;
  if (latency < 1)
    FATAL("Latency can't be smaller than 1");
  uid  = id;
  name = nodeName;
  dataType = dt;
  this->brPath = brPath;
  this->condBrId = condBrId;
  selfLoop = false;
  loadOutputAddressBus = false;
  inputDataBus = false;
  storeOutputAddressBus = false;
  outputDataBus = false;
  liveOut = false;
  loopCtrl = false;
}


Node::~Node()
{
}


int
Node::getId()
{
  return uid;
}


Instruction_Operation
Node::getIns()
{
  return ins;
}


bool 
Node::isConnectedTo(Node* nextNode)
{
  if (nextNode == this && selfLoop)
    return true;
  std::vector<Node*> nextNodes = getNextNodes();
  for (int i = 0; i < (int) nextNodes.size(); i++) {
    if (nextNodes[i]->getId() == nextNode->getId())
      return true;
  }
  return false;
}


std::vector<Node*> 
Node::getNextNodes()
{
  std::vector<Node*> nextNodes;
  int SizeOfOutGoing = outgoingArcs.size();
  for (int i = 0; i < SizeOfOutGoing; i++) {
    nextNodes.push_back(outgoingArcs[i]->getToNode());
  }
  return nextNodes;
}


std::vector<Node*>
Node::getPrevNodes()
{
  std::vector<Node*> prevNodes;
  for (auto incommingArcIt : incommingArcs) {
    prevNodes.push_back(incommingArcIt->getFromNode());
  }
  return prevNodes;
}


bool
Node::hasSelfLoop()
{
  return selfLoop;
}


void
Node::setSelfLoop(Arc* loopArc)
{
  selfLoop = true;
  selfLoopArc = loopArc;
}


void
Node::addPredArc(Arc* predArc)
{
  incommingArcs.push_back(predArc);
}


void
Node::addSuccArc(Arc* succArc)
{
  outgoingArcs.push_back(succArc);
}


void 
Node::removePredArc(int arcId)
{
  for (auto arcIt = incommingArcs.begin(); arcIt != incommingArcs.end(); arcIt++) {
    if ((*arcIt)->getId() == arcId) {
      incommingArcs.erase(arcIt, arcIt + 1);
      return;
    }
  }
  DEBUG("Warning: Attempting to remove non-existing pred arc: " << arcId << ", node: " << this->getId());
}


void 
Node::removeSuccArc(int arcId)
{
  for (auto arcIt = outgoingArcs.begin(); arcIt != outgoingArcs.end(); arcIt++) {
    if ((*arcIt)->getId() == arcId) {
      outgoingArcs.erase(arcIt, arcIt + 1);
      return;
    }
  }
  DEBUG("Warning: Attempting to remove non-existing succ arc: " << arcId << ", node: " << this->getId());
}


void 
Node::setLoadAddressGenerator(Node* readNode)
{
  loadOutputAddressBus = true;
  relatedNode = readNode;
  latency = 1;
}


void 
Node::setLoadDataBusRead(Node* addressNode)
{
  inputDataBus = true;
  relatedNode = addressNode;
  latency = 1;
}


void 
Node::setStoreAddressGenerator(Node* writeNode)
{
  storeOutputAddressBus = true;
  relatedNode = writeNode;
  latency = 0;
}


void 
Node::setStoreDataBusWrite(Node* addressNode)
{
  outputDataBus = true;
  relatedNode = addressNode;
  latency = 1;
}


void
Node::setLiveOut()
{
  liveOut = true;
}


void
Node::setLoopCtrl()
{
  loopCtrl = true;
}


nodePath
Node::getBrPath()
{
  return brPath;
}


unsigned
Node::getNumberOfOperands()
{
  // currently just get the max num of operands of the paths
  // map of operand count of each path
  std::map<nodePath, unsigned> pathCount;
  for (auto incommingEdge : incommingArcs) {
    if (incommingEdge->getDependency() == TrueDep) {
      // only count true dependency
      nodePath path = incommingEdge->getFromNode()->getBrPath();
      pathCount[path]++;
    }
  }
  auto maxPair = std::max_element(pathCount.begin(), pathCount.end(),
    [](const std::pair<nodePath, unsigned> & p1, const std::pair<nodePath, unsigned> & p2) 
    {return p1.second < p2.second;});
  return maxPair->second;
}


std::map<nodePath, unsigned> 
Node::getSuccSameIterNum()
{
  std::map<nodePath, unsigned> pathCount;
  for (auto arcIt : outgoingArcs) {
    if (arcIt->getDependency() == TrueDep || arcIt->getDependency() == PredDep) {
      if (arcIt->getDistance() == 0) {
        nodePath path = arcIt->getToNode()->getBrPath();
        pathCount[path]++;
      }
    }
  }
  return pathCount;
}


std::vector<Node*> 
Node::getSuccSameIterNodes()
{
  std::vector<Node*> succs;
  for (auto arcIt : outgoingArcs) {
    if (arcIt->getDependency() == TrueDep || arcIt->getDependency() == PredDep) {
      if (arcIt->getDistance() == 0) {
        succs.push_back(arcIt->getToNode());
      }
    }
  }
  return succs;
}


std::vector<Node*> 
Node::getSuccSameIterNodes(nodePath path)
{
  std::vector<Node*> succs;
  for (auto arcIt : outgoingArcs) {
    if (arcIt->getDependency() == TrueDep || arcIt->getDependency() == PredDep) {
      if (arcIt->getDistance() == 0) {
        if (arcIt->getToNode()->getBrPath() == path)
          succs.push_back(arcIt->getToNode());
      }
    }
  }
  return succs;
}


Datatype
Node::getDataType()
{
  return dataType;
}


std::vector<Node*> 
Node::getSuccNextIterNodes()
{
  std::vector<Node*> succs;
  for (auto arcIt : outgoingArcs) {
    if (arcIt->getDistance() > 0)
      succs.push_back(arcIt->getToNode());
  }
  return succs;
}


int
Node::getLatency()
{
  return latency;
}


bool 
Node::isLoadDataBusRead()
{
  return inputDataBus;
}


bool 
Node::isLoadAddressGenerator()
{
  return loadOutputAddressBus;
}


bool 
Node::isStoreDataBusWrite()
{
  return outputDataBus;
}


std::vector<Node*>
Node::getPrevSameIter()
{
  std::vector<Node*> prevNodes;
  for (auto edge : incommingArcs) {
    if (edge->getDistance() == 0)
      prevNodes.push_back(edge->getFromNode());
  }
  return prevNodes;
}


std::vector<Node*> 
Node::getPrevSameIterExMemDep()
{
  std::vector<Node*> prevNodes;
  for (auto arc : incommingArcs) {
    if (arc->getDependency() != LoadDep && 
        arc->getDependency() != StoreDep &&
        arc->getDistance() == 0) {
      prevNodes.push_back(arc->getFromNode());
    }
  }
  return prevNodes;
}


/***********************routeNode********************************/
routeNode::routeNode(int uid, Node* prevNode, nodePath path) :
Node(route, prevNode->getDataType(), 1, uid, "route", path, -1)
{
}


routeNode::~routeNode()
{
}