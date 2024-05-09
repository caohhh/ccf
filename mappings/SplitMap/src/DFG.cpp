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
  pathCount = 0;
  // rng here at construction
  std::random_device rd;
  rng = std::mt19937(rd());
  splitSource = false;
}


DFG::~DFG()
{
}


DFG::DFG(const DFG& originalDFG)
{
  // rng here at construction
  std::random_device rd;
  rng = std::mt19937(rd());
  pathCount = originalDFG.pathCount;
  nodeMaxId = originalDFG.nodeMaxId;
  arcMaxId = originalDFG.arcMaxId;
  splitSource = originalDFG.splitSource;

  // get all the nodes and make new node
  for (Node* node : originalDFG.nodeSet)
    nodeSet.push_back(new Node(*node));
  for (Node* constNode : originalDFG.constants)
    constants.push_back(new Node(*constNode));
  // get all the arcs and make new arc
  constArcSet = originalDFG.constArcSet;
  for (Arc* arc : originalDFG.arcSet) {
    int oldFromId = arc->getFromNode()->getId();
    int oldToId = arc->getToNode()->getId();
    Node* nodeFrom = nullptr;
    Node* nodeTo = nullptr;
    for (Node* node : nodeSet) {
      if (node->getId() == oldFromId)
        nodeFrom = node;
      if (node->getId() == oldToId)
        nodeTo = node;
    }
    if (nodeFrom == nullptr || nodeTo == nullptr)
      FATAL("[DFG Copy]Error: Arc with node not in set");
    Arc* newArc = new Arc(*arc, nodeFrom, nodeTo);
    if (newArc->getDependency() == LoadDep) {
      nodeFrom->setLoadAddressGenerator(nodeTo);
      nodeTo->setLoadDataBusRead(nodeFrom);
    }
    if (newArc->getDependency() == StoreDep) {
      nodeFrom->setStoreAddressGenerator(nodeTo);
      nodeTo->setStoreDataBusWrite(nodeFrom);
    }
    if (nodeFrom->getId() == nodeTo->getId())
      nodeFrom->setSelfLoop(newArc);
    else {
      //connect those nodes through this new arc
      nodeTo->addPredArc(newArc);
      nodeFrom->addSuccArc(newArc);
    }
    arcSet.push_back(newArc);
  }
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
DFG::insertNode(routeNode* newRouteNode)
{
  newRouteNode->setId(++nodeMaxId);
  nodeSet.push_back(newRouteNode);  
}


void
DFG::makeArc(Node* nodeFrom, Node* nodeTo, int distance, DataDepType dep, int opOrder, nodePath path)
{
  //if they are already connected, ignore it
  if (nodeFrom->isConnectedTo(nodeTo)) {
    // here we should also check for op order
    // but that would mean changing too much, mainly the getArc with from and to node
    // so I'll defer this to when there is a problem :P
    DEBUG("[Make Arc]WARNNING!!! Making an arc between 2 already connected nodes.");
    return;
  }

  //source and destination are the same and the node has already a self loop, ignore it
  if (nodeFrom->getId() == nodeTo->getId() && nodeFrom->hasSelfLoop())
    return;

  Arc *newArc;
  //create an arc with given properties
  newArc = new Arc(nodeFrom, nodeTo, ++arcMaxId, distance, dep, opOrder, path);

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
DFG::makeConstArc(int nodeFromId, int nodeToId, int opOrder, nodePath path)
{
  Const_Arc newArc;
  newArc.fromNodeId = nodeFromId;
  newArc.toNodeId = nodeToId;
  newArc.opOrder = opOrder;
  newArc.brPath = path;
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


Arc* 
DFG::getArc(int arcId)
{
  for (auto arc : arcSet) {
    if (arc->getId() == arcId)
      return arc;
  }
  return nullptr;
}


void
DFG::removeArc(int arcId)
{
  for (auto arcIt = arcSet.begin(); arcIt != arcSet.end(); arcIt++) {
    if ((*arcIt)->getId() == arcId) {
      if ((*arcIt)->getFromNode() != (*arcIt)->getToNode()) {
        (*arcIt)->getFromNode()->removeSuccArc(arcId);
        (*arcIt)->getToNode()->removePredArc(arcId);
      } else {
        (*arcIt)->getFromNode()->resetSelfLoop();
      }
      // delete arc from set
      arcSet.erase(arcIt, arcIt + 1);
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
      DEBUG("[Preprocess]Inter iteration dependency reduction");
      int minDist = INT32_MAX;
      for (auto nextNodeIt : nextNodes) {
        int dist = getArc(nodeIt, nextNodeIt)->getDistance();
        if (dist < minDist) 
          minDist = dist;
      }
      // create new routing node
      routeNode* newRouteNode = new routeNode(nodeIt, nodeIt->getBrPath());
      insertNode(newRouteNode);
      // we put the new route node the same iter as minNode
      makeArc(nodeIt, newRouteNode, minDist, TrueDep, 0, none);
      for (auto nextNodeIt: nextNodes) {
        //disconnect
        Arc* arcOld = getArc(nodeIt, nextNodeIt);
        removeArc(arcOld->getId());
        //connect
        makeArc(newRouteNode, nextNodeIt, arcOld->getDistance() - minDist, 
                arcOld->getDependency(), arcOld->getOperandOrder(), arcOld->getPath());
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
        DEBUG("[Preprocess]Limiting outdegree");
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
          routeNode* newRouteNode = new routeNode(nodeIt, none);
          insertNode(newRouteNode);
          // connect it to the prev node
          makeArc(nodeIt, newRouteNode, 0, TrueDep, 0, none);
          // since the next nodes won't be large, just shuffle the whole vec will be ok
          std::shuffle(nextNoneNodes.begin(), nextNoneNodes.end(), rng);
          // pick out the nodes and move it to the route node
          for (unsigned i = 0; i < removeNumNone; i++) {
            // disconnect
            Arc* arcOld = getArc(nodeIt, nextNoneNodes[i]);
            removeArc(arcOld->getId());
            // connect
            makeArc(newRouteNode, nextNoneNodes[i], arcOld->getDistance(), 
                    arcOld->getDependency(), arcOld->getOperandOrder(), arcOld->getPath());
          }
        }

        if (removeNumPath != 0) {
          // add a routing node in path set
          routeNode* newRouteNode = new routeNode(nodeIt, (nodePath)path);
          insertNode(newRouteNode);
          // connect it to the prev node
          makeArc(nodeIt, newRouteNode, 0, TrueDep, 0, none);
          // since the next nodes won't be large, just shuffle the whole vec will be ok
          std::shuffle(nextPathNodes.begin(), nextPathNodes.end(), rng);
          // pick out the nodes and move it to the route node
          for (unsigned i = 0; i < removeNumPath; i++) {
            // disconnect
            Arc* arcOld = getArc(nodeIt, nextPathNodes[i]);
            removeArc(arcOld->getId());
            // connect
            makeArc(newRouteNode, nextPathNodes[i], arcOld->getDistance(), 
                    arcOld->getDependency(), arcOld->getOperandOrder(), arcOld->getPath());
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
        // if we reached dest, this forms a cycle in the DFG, record the cycle
        pathII = float(pathLatency) / float(pathDistance);
        std::set<Node*> foundPath = path;
        foundPath.insert(destNode);
        // in cycles, find where pathII > cycleII
        auto cycleIt = cycles.begin();
        for (; cycleIt != cycles.end(); ++cycleIt) {
          if (pathII > std::get<1>(*cycleIt))
            break;
        }
        cycles.insert(cycleIt, std::make_tuple(foundPath, pathII));
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


std::set<int> 
DFG::getArcIdSet()
{
  std::set<int> idSet;
  for (auto arc : arcSet) {
    if (idSet.find(arc->getId()) != idSet.end())
      FATAL("ERROR: Arcs with the same id exist in this DFG");
    idSet.insert(arc->getId());
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


Node*
DFG::getLoopCtrlNode()
{
  for (Node* node : nodeSet) {
    if (node->isLoopCtrl())
      return node;
  }
  return nullptr;
}


std::vector<Node*>
DFG::getLiveOutNodes()
{
  std::vector<Node*> liveOutNodes;
  for (Node* node : nodeSet) {
    if (node->isLiveOut())
      liveOutNodes.push_back(node);
  }
  return liveOutNodes;
}


std::vector<std::set<Node*>> 
DFG::getCycles()
{
  std::vector<std::set<Node*>> retCycles;
  for (auto cycle : cycles)  {
    // we also omit cycles completely contained in larger cycles
    bool contained = false;
    for (auto prevCycle : retCycles) {
      contained = true;
      for (Node* node : std::get<0>(cycle)) {
        if (prevCycle.find(node) == prevCycle.end()) {
          contained = false;
          break;
        }
      }
      if (contained)
        break;
    }
    if (!contained) {
      retCycles.push_back(std::get<0>(cycle));
    }
  }
  return retCycles;
}


bool 
DFG::canBeSplit()
{
  return splitSource;
}


void
DFG::setSplitSource(int splitPath)
{
  if (splitPath == -1)
    splitSource = false;
  else {
    splitSource = true;
    for (auto node : nodeSet)
      if (node->getCondBr() == splitPath)
        node->setSplitCond();
  }
}


void
DFG::setPathCount(int pathCount)
{
  this->pathCount = pathCount;
}


std::tuple<DFG*, DFG*> 
DFG::split()
{
  if (!splitSource)
    FATAL("[Split]ERROR! Splitting a DFG that can't be split");
  
  // basically the copy creator, just don't copy nodes belonging to other path
  // we can just copy create 2 DFGs and remove the unwanted nodes
  DFG* trueDFG = new DFG(*this);
  DFG* falseDFG = new DFG(*this);
  trueDFG->removePath(false_path);
  falseDFG->removePath(true_path);
  trueDFG->setSplitSource(false);
  falseDFG->setSplitSource(false);
  return std::make_tuple(trueDFG, falseDFG);
}


void
DFG::removePath(nodePath path)
{
  if (path != true_path && path != false_path)
    FATAL("[Remove Path]ERROR! Removing a path other than true or false");
  // iterate through all nodes, remove nodes belonging to the path and the arcs
  for (auto nodeIt = nodeSet.begin(); nodeIt != nodeSet.end();) {
    Node* node = *nodeIt;
    if (node->getBrPath() == path) {
      // remove the node and arc
      // first all arcs
      std::vector<int> arcsToRemove;
      for (Arc* arc : arcSet) {
        if (arc->getFromNode() == node || arc->getToNode() == node)
          arcsToRemove.push_back(arc->getId());
      } // end of iterating through arcSet
      // remove arcs
      for (int arcId : arcsToRemove) {
        removeArc(arcId);
      }
      // now all arcs are gone, remove the node
      nodeIt = nodeSet.erase(nodeIt);
    } else 
      ++nodeIt;
  } // end of iterating through nodeSet

  // also iterate through the constants
  for (auto constIt = constants.begin(); constIt != constants.end();) {
    Node* constNode = *constIt;
    if (constNode->getBrPath() == path) {
      // remove the constant and the arc
      // first all arcs
      for (auto arcIt = constArcSet.begin(); arcIt != constArcSet.end();) {
        Const_Arc constArc = *arcIt;
        if (constArc.fromNodeId == constNode->getId() || constArc.toNodeId == constNode->getId())
          arcIt = constArcSet.erase(arcIt);
        else
          ++arcIt;
      } // end of iterating through constArcSet
      // now all arcs are gone, remove the node
      constIt = constants.erase(constIt);
    } else 
      ++constIt;
  } // end of iterating through constants

  // now for the arcs originally the data source of sel
  std::vector<Arc*> arcsToRemove;
  for (Arc* arc : arcSet) {
    if (arc->getPath() == path)
      arcsToRemove.push_back(arc);
  }
  for (Arc* arc : arcsToRemove) {
    removeArc(arc->getId());
  }
  // also the constArc set
  for (auto arcIt = constArcSet.begin(); arcIt != constArcSet.end();) {
    if ((*arcIt).brPath == path) 
      arcIt = constArcSet.erase(arcIt);
    else
      ++arcIt;
  } // end of iterating through constArc set
}


void
DFG::padPath()
{
  DEBUG("[Pad Path]Padding paths of this DFG");
  if (!splitSource) {
    DEBUG("[Pad Path]Not a split source, no need for padding");
    return;
  }
  // DFG is a split source, check all the arcs belonging to a path

  // arcs to pad, accessed by their from node
  std::map<Node*, std::vector<Arc*>>  toPad;
  for (Arc* arc :  arcSet) {
    if (arc->getPath() != none) {
      // sanity check to ensure arcs with path is to a node with none path
      if (arc->getToNode()->getBrPath() != none)
        FATAL("[Pad Path]ERROR!!Edge with branch path should always be to a node with none path: " << arc->getToNode()->getId());
      if (arc->getFromNode()->getBrPath() == none) {
        // if from node is of none path, then padding is needed
        toPad[arc->getFromNode()].push_back(arc);
      }
    }
  } // end of iterating through arcSet

  // now do the padding, doing this seperately because we need to remove the padded arc from arcset
  // and I'm too lazy to change the removeArc function to return an iterator position
  for (auto it : toPad) {
    Node* fromNode = it.first;
    std::vector<Arc*> arcsToPad = it.second;
    // path of the arcs, should be the same across the vector
    nodePath path = arcsToPad[0]->getPath();
    // first the new routing node
    routeNode* newRouteNode = new routeNode(fromNode, path);
    insertNode(newRouteNode);
    // connect it to from node, we make the routing node in the same iter as the from node
    makeArc(fromNode, newRouteNode, 0, TrueDep, 0, none);
    // now iterate through the arcs
    for (auto arc : arcsToPad) {
      DEBUG("[Pad Path]Padding between node: " << fromNode->getId() <<" to node: " << arc->getToNode()->getId() << 
        " of path " << arc->getPath());
      if (arc->getPath() != path)
        FATAL("[Pad Path]ERROR!! All arc from the same node should be of the same path");
      // connect
      makeArc(newRouteNode, arc->getToNode(), arc->getDistance(), arc->getDependency(), arc->getOperandOrder(), arc->getPath());
      // remove arc
      removeArc(arc->getId());
    } // end of iterating through arcs
  } // end of iterating through toPad

  // also need to check the const arc
  std::map<int, std::vector<Const_Arc>> constToPad;
  for (auto constArc : constArcSet) {
    if (constArc.brPath != none) {
      Node* toNode = getNode(constArc.toNodeId);
      if (toNode->getBrPath() != none)
        FATAL("[Pad Path]ERROR!!Edge with branch path should always be to a node with none path: " << toNode->getId());
      // for a const arc with path, we also need to add a route node 
      constToPad[constArc.fromNodeId].push_back(constArc);
    }
  }

  // now do the padding
  for (auto it : constToPad) {
    int fromNodeId = it.first;
    std::vector<Const_Arc> constArcsToPad = it.second;
    // path of the arcs, should be the same across the vector
    nodePath path = constArcsToPad[0].brPath;
    // first the new routing node
    // this is to create a int32 routing node, may be need to be changed in the future to support
    // more data types
    routeNode* newRouteNode = new routeNode(new Node(rest, int32, 1, -1, "", none, -1), path);
    insertNode(newRouteNode);
    // connect it to from node, we make the routing node in the same iter as the from node
    makeConstArc(fromNodeId, newRouteNode->getId(), 0, none);
    for (auto constArc : constArcsToPad) {
      DEBUG("[Pad Path]Padding between node: " << fromNodeId <<" to node: " << constArc.toNodeId << 
            " of path " << constArc.brPath);
      if (constArc.brPath != path)
        FATAL("[Pad Path]ERROR!! All arc from the same node should be of the same path");
      // connect
      makeArc(newRouteNode, getNode(constArc.toNodeId), 0, TrueDep, constArc.opOrder, path);
      // remove arc
      for (auto constIt = constArcSet.begin(); constIt != constArcSet.end(); ++constIt) {
        if ((*constIt).fromNodeId == constArc.fromNodeId &&
            (*constIt).toNodeId == constArc.toNodeId &&
            (*constIt).opOrder == constArc.opOrder) {
          constArcSet.erase(constIt);
          break;
        }
      }
    } // end of iterating through const arcs
  } // end of iterating through const to pad
}


void
DFG::mergeNodes()
{
  // map of the nodes to merge, where the key is the common toNode of the 2 nodes to merge
  std::map<Node*, std::vector<Node*>> toMerge;
  // first we iterate through the arc set to get all the arcs with a path
  for (auto arc : arcSet) {
    if (arc->getPath() != none) {
      // if the arc is of none path, add it to the to merge list
      if (arc->getToNode()->getBrPath() != none)
        FATAL("[Merge Nodes]ERROR! Edge with a path should be to a node of none path");
      if (arc->getFromNode()->getBrPath() != arc->getPath())
        FATAL("[Merge Nodes]ERROR! Edge with a path should be from a node of the same path");
      toMerge[arc->getToNode()].push_back(arc->getFromNode());
    }
  }
  // now we check each toMerge and set the nodes
  for (auto toMergeIt : toMerge) {
    std::vector<Node*> nodesToMerge = toMergeIt.second;
    if (nodesToMerge.size() != 2) {
      FATAL("[Merge Nodes]ERROR! There should always be 2 nodes to merge");
    }
    if (nodesToMerge[0]->getBrPath() == nodesToMerge[1]->getBrPath())
      FATAL("[Merge Nodes]ERROR! The 2 nodes to merge should be of different paths");
    // now all checks are done, merge the 2 nodes
    nodesToMerge[0]->setMergeNode(nodesToMerge[1]);
    nodesToMerge[1]->setMergeNode(nodesToMerge[0]);
    DEBUG("[Merge Nodes]Merged nodes: " << nodesToMerge[0]->getId() << ", " << nodesToMerge[1]->getId());
  }
}


std::vector<Const_Arc>
DFG::getConstArcs()
{
  return constArcSet;
}