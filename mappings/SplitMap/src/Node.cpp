/**
 * Node.cpp
*/

#include "Node.h"
#include <algorithm>
#include <set>

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


Node::Node(const Node& originalNode)
{
  uid = originalNode.uid;
  ins = originalNode.ins;
  dataType = originalNode.dataType;
  latency = originalNode.latency;
  name = originalNode.name;
  condBrId = originalNode.condBrId;
  brPath = originalNode.brPath;
  selfLoop = originalNode.selfLoop;
  liveOut = originalNode.liveOut;
  loopCtrl = originalNode.loopCtrl;
  loadOutputAddressBus = originalNode.loadOutputAddressBus;
  inputDataBus = originalNode.inputDataBus;
  storeOutputAddressBus = originalNode.storeOutputAddressBus;
  outputDataBus = originalNode.outputDataBus;
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
  memRelatedNode = readNode;
  latency = 1;
}


void 
Node::setLoadDataBusRead(Node* addressNode)
{
  inputDataBus = true;
  memRelatedNode = addressNode;
  latency = 1;
}


void 
Node::setStoreAddressGenerator(Node* writeNode)
{
  storeOutputAddressBus = true;
  memRelatedNode = writeNode;
  latency = 0;
}


void 
Node::setStoreDataBusWrite(Node* addressNode)
{
  outputDataBus = true;
  memRelatedNode = addressNode;
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
Node::isMemNode()
{
  if (inputDataBus || loadOutputAddressBus || storeOutputAddressBus || outputDataBus)
    return true;
  else
    return false;
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
Node::isStoreAddressGenerator()
{
  return storeOutputAddressBus;
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


std::vector<Node*> 
Node::getNextSameIter()
{
  std::vector<Node*> nextNodes;
  for (Arc* arc : outgoingArcs) {
    if (arc->getDistance() == 0)
      nextNodes.push_back(arc->getToNode());
  }
  return nextNodes;
}


std::vector<Node*> 
Node::getSuccSameIterExMemDep()
{
  std::vector<Node*> nextNodes;
  /*if (loadOutputAddressBus) 
    return relatedNode->getNextNodes();*/
  for (Arc* arc : outgoingArcs) {
    if (arc->getDependency() != LoadDep &&
        arc->getDependency() != StoreDep &&
        arc->getDistance() == 0) {
      nextNodes.push_back(arc->getToNode());
    }
  }
  return nextNodes;
}


std::vector<Node*> 
Node::getExDepPredPrevIter()
{
  std::vector<Node*> predNodes;
  for (auto arc : incommingArcs) {
    if (arc->getDependency() == TrueDep || arc->getDependency() == PredDep) {
      if (arc->getDistance() > 0)
        predNodes.push_back(arc->getFromNode());
    }
  }
  return predNodes;
}


std::vector<Node*> 
Node::getExDepSuccNextIter()
{
  std::vector<Node*> succNodes;
  for (auto arc : outgoingArcs) {
    if (arc->getDependency() == TrueDep || arc->getDependency() == PredDep) {
      if (arc->getDistance() > 0)
        succNodes.push_back(arc->getToNode());
    }
  }
  return succNodes;
}


std::vector<Node*>
Node::getInterIterRelatedNodes()
{
  std::set<Node*> interIterRelatedNodes;
  // pred nodes
  for (Node* node : this->getExDepPredPrevIter()) 
    interIterRelatedNodes.insert(node);
  
  // succ nodes
  for (Node* node : this->getExDepSuccNextIter())
    interIterRelatedNodes.insert(node);
  // new nodes to iterate through, start with all nodes
  std::vector<Node*> newNodes(interIterRelatedNodes.begin(), interIterRelatedNodes.end());
  
  while (newNodes.size() > 0) {
    // still have new nodes to iterate through
    std::vector<Node*> newNodesTemp;
    for (Node* node : newNodes) {
      for (Node* predNode : node->getExDepPredPrevIter()) {
        if (interIterRelatedNodes.find(predNode) != interIterRelatedNodes.end()) {
          // pred is a new node
          newNodesTemp.push_back(predNode);
          interIterRelatedNodes.insert(predNode);
        }
      }
      for (Node* succNode : node->getExDepSuccNextIter()) {
        if (interIterRelatedNodes.find(succNode) != interIterRelatedNodes.end()) {
          // succ is a new node
          newNodesTemp.push_back(succNode);
          interIterRelatedNodes.insert(succNode);
        }
      }
    } // end of iterating through newNodes
    newNodes = newNodesTemp;
  } // no new nodes left

  std::vector<Node*> ret(interIterRelatedNodes.begin(), interIterRelatedNodes.end());
  return ret;
}


Node*
Node::getMemRelatedNode()
{
  if (loadOutputAddressBus || inputDataBus || storeOutputAddressBus || outputDataBus)
    return memRelatedNode;
  else
    return nullptr;
}


bool
Node::isLiveOut()
{
  return liveOut;
}


bool
Node::isLoopCtrl()
{
  return this->loopCtrl;
}


/***********************routeNode********************************/
routeNode::routeNode(Node* prevNode, nodePath path) :
Node(route, prevNode->getDataType(), 1, -1, "route", path, -1)
{
}


routeNode::~routeNode()
{
}


void
routeNode::setId(int uid)
{
  this->uid = uid;
}