/**
 * DFG.cpp
*/

#include "DFG.h"
#include <algorithm>
#include <limits>


DFG::DFG()
{
  nodeMaxId = 0;
  arcMaxId = 0;
  // currently set to 2: true path and false path
  // this can be extanted and set during parsing
  pathCount = 2;
  // rng here at construction
  std::random_device rd;
  rng = std::mt19937(rd());
}


DFG::~DFG()
{
}


bool
DFG::hasNode(int nodeId)
{
  for (auto nodeIT : nodeSet) {
    if (nodeIT->getId() == nodeId)
      return true;
  }
  return false;
}


void
DFG::insertNode(Node* node)
{
  if (node->getId() > nodeMaxId)
    nodeMaxId = node->getId();
  
  // seperate out constants
  if (node->getIns() == constant) 
    constants.push_back(node);
  else
    nodeSet.push_back(node);
}


void
DFG::makeArc(Node* nodeFrom, Node* nodeTo, int distance, DataDepType dep, int opOrder)
{
  //if they are already connected, ignore it
  if (nodeFrom->isConnectedTo(nodeTo))
    return;

  //source and destination are the same and the node has already a self loop, ignore it
  if (nodeFrom->getId() == nodeTo->getId() && nodeFrom->hasSelfLoop())
    return;

  Arc *newArc;
  //create an arc with given properties
  newArc = new Arc(nodeFrom, nodeTo, ++arcMaxId, distance, dep, opOrder);

  //if source and destination are the same, create a self loop
  if (nodeFrom->getId() == nodeTo->getId())
    nodeFrom->setSelfLoop(newArc);
  else {
    //connect those nodes through this new arc
    nodeTo->addPredArc(newArc);
    nodeFrom->addSuccArc(newArc);
  }

  //add the arc to the arc set
  arcSet.push_back(newArc);
}


void
DFG::makeConstArc(int nodeFromId, int nodeToId, int opOrder)
{
  Const_Arc newArc;
  newArc.fromNodeId = nodeFromId;
  newArc.toNodeId = nodeToId;
  newArc.opOrder = opOrder;
  constArcSet.push_back(newArc);
}


bool 
DFG::hasConstant(int ID)
{
  for (auto nodeIT : constants) {
    if (nodeIT->getId() == ID)
      return true;
  }
  return false;
}


Node*
DFG::getNode(int nodeId)
{
  for (auto nodeIT : nodeSet) {
    if (nodeIT->getId() == nodeId)
      return nodeIT;
  }
  return nullptr;
}


Arc* 
DFG::getArc(Node* nodeFrom, Node* nodeTo)
{
  for (auto arcIt : arcSet) {
    if (arcIt->getFromNode()->getId() == nodeFrom->getId() && arcIt->getToNode()->getId() == nodeTo->getId())
      return arcIt;
  }
  return nullptr;
}


void
DFG::removeArc(int arcId)
{
  for (auto arcIt = arcSet.begin(); arcIt != arcSet.end(); arcIt++) {
    if ((*arcIt)->getId() == arcId) {
      // delete arc from set
      arcSet.erase(arcIt, arcIt + 1);
      (*arcIt)->getFromNode()->removeSuccArc(arcId);
      (*arcIt)->getToNode()->removePredArc(arcId);
      return;
    }
  }
  DEBUG("Warning: Attempting to remove non-existing arc: " << arcId);
}



void
DFG::preProcess(unsigned maxInDegree, unsigned maxOutDegree)
{
  // note that for now, with true path and false path both existing
  // the constraint is applied to the maximum of the two
  // this may need to be changed

  // first apply in degree constraint
  for (auto nodeIt : nodeSet) {
    if (nodeIt->getNumberOfOperands() > maxInDegree)
      FATAL("Exceeding max in degree of " << maxInDegree << "with node " << nodeIt->getId());
  }

  // Minimize register requirements for inter iteration dependencies
  for (auto nodeIt : nodeSet) {
    std::vector<Node*> nextNodes = nodeIt->getSuccNextIterNodes();
    if (nextNodes.size() > 1) {
      // selected node has more than 1 child in next iterations
      // find the node with min distance
      int minDist = INT32_MAX;
      for (auto nextNodeIt : nextNodes) {
        int dist = getArc(nodeIt, nextNodeIt)->getDistance();
        if (dist < minDist) 
          minDist = dist;
      }
      // create new routing node
      Node* newRouteNode = new routeNode(++nodeMaxId, nodeIt, nodeIt->getBrPath());
      insertNode(newRouteNode);
      // we put the new route node the same iter as minNode
      makeArc(nodeIt, newRouteNode, minDist, TrueDep, 0);
      for (auto nextNodeIt: nextNodes) {
        //disconnect
        Arc* arcOld = getArc(nodeIt, nextNodeIt);
        removeArc(arcOld->getId());
        //connect
        makeArc(newRouteNode, nextNodeIt, arcOld->getDistance() - minDist, arcOld->getDependency(), arcOld->getOperandOrder());
      }
    }
  }

  // next out degree constraint
  // we apply this constraint to each path
  for (auto nodeIt : nodeSet) {
    auto mapPathNodeCount = nodeIt->getSuccSameIterNum();
    for (unsigned path = 0; path <= pathCount; path++) {
      unsigned nodeCount;
      if (path == 0)
        nodeCount = mapPathNodeCount[none];
      else
        nodeCount = mapPathNodeCount[none] + mapPathNodeCount[(nodePath)path];
      if (nodeCount > maxOutDegree) {
        // if outdegree > max, need to add routing node
        // with nodes of path:none and given path 
        // we should choose the larger set and favor path set
        unsigned removeNum = nodeCount - maxOutDegree;
        std::vector<Node*> nextNoneNodes = nodeIt->getSuccSameIterNodes(none);
        std::vector<Node*> nextPathNodes = nodeIt->getSuccSameIterNodes((nodePath)path);
        // number of succ nodes to move in none
        unsigned removeNumNone = 0;
        // number of succ nodes to move in path
        unsigned removeNumPath = 0;
        // get the number of succs to remove from each path
        if (nextNoneNodes.size() > nextPathNodes.size()) {
          // none set larger
          if (removeNum > (nextNoneNodes.size() - 1)) {
            // still larger with all none nodes but route gone
            removeNumNone = nextNoneNodes.size();
            removeNumPath = removeNum - removeNumNone + 1;
          } else 
            removeNumNone = removeNum + 1;
        } else {
          // path set larger
          if (removeNum > (nextPathNodes.size() - 1)) {
            // still larger with all none nodes but route gone
            removeNumPath = nextPathNodes.size();
            removeNumNone = removeNum - removeNumPath + 1;
          } else 
            removeNumPath = removeNum + 1;
        }

        if (removeNumNone != 0) {
          // add a routing node in none set
          Node* newRouteNode = new routeNode(++nodeMaxId, nodeIt, none);
          insertNode(newRouteNode);
          // connect it to the prev node
          makeArc(nodeIt, newRouteNode, 0, TrueDep, 0);
          // since the next nodes won't be large, just shuffle the whole vec will be ok
          std::shuffle(nextNoneNodes.begin(), nextNoneNodes.end(), rng);
          // pick out the nodes and move it to the route node
          for (unsigned i = 0; i < removeNumNone; i++) {
            // disconnect
            Arc* arcOld = getArc(nodeIt, nextNoneNodes[i]);
            removeArc(arcOld->getId());
            // connect
            makeArc(newRouteNode, nextNoneNodes[i], arcOld->getDistance(), arcOld->getDependency(), arcOld->getOperandOrder());
          }
        }

        if (removeNumPath != 0) {
          // add a routing node in none set
          Node* newRouteNode = new routeNode(++nodeMaxId, nodeIt, (nodePath)path);
          insertNode(newRouteNode);
          // connect it to the prev node
          makeArc(nodeIt, newRouteNode, 0, TrueDep, 0);
          // since the next nodes won't be large, just shuffle the whole vec will be ok
          std::shuffle(nextPathNodes.begin(), nextPathNodes.end(), rng);
          // pick out the nodes and move it to the route node
          for (unsigned i = 0; i < removeNumPath; i++) {
            // disconnect
            Arc* arcOld = getArc(nodeIt, nextPathNodes[i]);
            removeArc(arcOld->getId());
            // connect
            makeArc(newRouteNode, nextPathNodes[i], arcOld->getDistance(), arcOld->getDependency(), arcOld->getOperandOrder());
          }
        }
      } // end of nodeCount > maxOutDegree
    } // end of iterating through paths
  } // end of out degree iterating through nodes
}


std::tuple<bool, int, int>
DFG::getPathMaxII(Node* currentNode, Node* destNode, std::set<Node*>& path, int prevLat, int prevDist)
{
  // the maximum II of all paths of prev nodes
  float maxII = 0;
  // latency of the max II path to return
  int latency = 0;
  // distance of the max II path to return
  int distance = 0;
  // if a path of currentNode -> destNode is found
  bool found = false;
  for (auto prevNode : currentNode->getPrevNodes()) {
    int pathLatency;
    int pathDistance;
    float pathII;
    bool pathFound = false;
    if (path.find(prevNode) == path.end()) {
      // node not in path
      if (prevNode->getId() == destNode->getId()) {
        // reached dest
        pathLatency = prevNode->getLatency() + prevLat;
        pathDistance = getArc(prevNode, currentNode)->getDistance() + prevDist;
        pathFound = true;
      } else {
        // not at dest yet
        // add node to path
        std::set<Node*> currentPath = path;
        currentPath.insert(prevNode);
        // accumulate latency and distance
        int accuLat = prevNode->getLatency() + prevLat;
        int accuDist = getArc(prevNode, currentNode)->getDistance() + prevDist;
        // recurssively get full path latency and distance
        std::tie(pathFound, pathLatency, pathDistance) = getPathMaxII(prevNode, destNode, currentPath, accuLat, accuDist);
      }
      if (pathFound) {
        // if there is a path to dest from this prev
        found = true;
        pathII = float(pathLatency) / float(pathDistance);
        if (maxII < pathII) {
          // get the prev path with max II one
          maxII = pathII;
          latency = pathLatency;
          distance = pathDistance;
        }
      }
    }
  }
  return std::make_tuple(found, latency, distance);
}


int
DFG::calculateRecMII()
{
  int recMII = 0;
  for (auto arcIt : arcSet) {
    if (arcIt->getDistance() > 0) {
      int currRecMII = 0;
      if (arcIt->getFromNode() == arcIt->getToNode())
        currRecMII = (arcIt->getFromNode()->getLatency()) / arcIt->getDistance();
      else {
        Node* fromNode = arcIt->getFromNode();
        Node* toNode = arcIt->getToNode();
        std::set<Node*> path;
        path.insert(fromNode);
        bool pathFound;
        int pathLatency;
        int pathDist;
        std::tie(pathFound, pathLatency, pathDist) = getPathMaxII(fromNode, toNode, path, fromNode->getLatency(), arcIt->getDistance());
        if (pathFound)
          currRecMII = ceil((float) pathLatency / pathDist);
      }
      if (recMII < currRecMII)
        recMII = currRecMII;
    }
  }
  return recMII;
}


int
DFG::getNodeSize()
{
  return nodeSet.size();
}


int 
DFG::getLoadOpCount()
{
  int count = 0;
  for (auto node : nodeSet) {
    if (node->isLoadDataBusRead() || node->isLoadAddressGenerator())
      count++;
  }
  return count;
}


int 
DFG::getStoreOpCount()
{
  int count = 0;
  for (auto node: nodeSet) {
    if (node->isStoreDataBusWrite())
      count++;
  }
  return count;
}


std::vector<Node*> 
DFG::getStartNodes()
{
  std::vector<Node*> startNodes;
  for (auto node : nodeSet) {
    if (node->getPrevSameIter().size() == 0)
      startNodes.push_back(node);
  }
  return startNodes;
}


std::set<int> 
DFG::getNodeIdSet()
{
  std::set<int> idSet;
  for (auto node : nodeSet) {
    if (idSet.find(node->getId()) != idSet.end())
      FATAL("ERROR: Nodes with the same id exist in this DFG");
    idSet.insert(node->getId());
  }
  return idSet;
}


std::vector<Node*> 
DFG::getEndNodes()
{
  std::vector<Node*> endNodes;
  for (Node* node : nodeSet) {
    if (node->getNextSameIter().size() == 0) {
      endNodes.push_back(node);
    }
  }
  return endNodes;
}