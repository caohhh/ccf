/**
 * Falcon.cpp
*/

#include "Falcon.h"


Falcon::Falcon(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy)
{
  this->cgraInfo = cgraInfo;
  this->mappingPolicy = mappingPolicy;
}


Falcon::~Falcon()
{
}


bool
Falcon::generateMap(Parser* myParser)
{
  //read input files and construct DFG
  DFG* originalDFG= new DFG();
  myParser->ParseDFG(originalDFG);
  DEBUG("[Falcon]Done parsing DFG");

  originalDFG->preProcess(cgraInfo.MAX_NODE_INDEGREE, cgraInfo.MAX_NODE_OUTDEGREE);
  DEBUG("[Falcon]Done preprocessing DFG");

  // get the recMII
  int recMII  = originalDFG->calculateRecMII();
  DEBUG("[Falcon]Done calculation RecMII of the DFG as " << recMII);

  // resMII
  int cgraSize = cgraInfo.X_Dim * cgraInfo.Y_Dim;
  int resMII = ceil(float(originalDFG->getNodeSize()) / cgraSize);
  DEBUG("[Falcon]Done calculating resMII as " << resMII);

  // find memory resource MII
  int memMII = ceil(float(originalDFG->getLoadOpCount() + originalDFG->getStoreOpCount()) / cgraInfo.Y_Dim);
  DEBUG("[Falcon]Done calculating memMII as " << memMII);

  // the MII is the largest of the three
  int MII = recMII;
  if (resMII > MII)
    MII = resMII;
  if(memMII > MII)
    MII = memMII;
  DEBUG("[Falcon]MII of the schedule is " << MII);

  // ASAP schedule
  int length = scheduleASAP(originalDFG);
  DEBUG("[Falcon]ASAP schedule successful with length of " << length);

  // ALAP schedule
  scheduleALAP(originalDFG, length);
  DEBUG("[Falcon]ALAP schedule successful");

  return true;
}


int 
Falcon::scheduleASAP(DFG* myDFG)
{
  DEBUG("[ASAP]started");
  // the asap schedule
  std::map<int, int> asapSchedule;
  // set of nodes to be scheduled
  std::vector<int> toSchedule;
  // latest time of the schedule, to be returned
  int latestTime = 0;
  // start with nodes with no predecessor in same iter
  for (auto node: myDFG->getStartNodes()) 
    toSchedule.push_back(node->getId());

  // start nodes are scheduled at time 0
  for (int node : toSchedule) {
    asapSchedule[node] = 0;
    DEBUG("[ASAP]node: " << node << " scheduled at 0" );
  }
  
  // nodes left to be scheduled
  std::set<int> rest;
  rest = myDFG->getNodeIdSet();

  // remove the nodes that have just been scheduled
  for (int nodeId : toSchedule)
    rest.erase(nodeId);
  toSchedule.clear();

  while (rest.size() > 0) {
    // while there are still unscheduled nodes
    for (int nodeId : rest) {
      // first check if node of nodeId is ready to be asap scheduled
      bool canSchedule = true;
      int scheduleTime = 0;
      std::vector<Node*> prevNodes = myDFG->getNode(nodeId)->getPrevSameIter();
      for (Node* prevNode : prevNodes) {
        if (asapSchedule.find(prevNode->getId()) == asapSchedule.end()) {
          // prev node not scheduled, this node not ready
          canSchedule = false;
          break;
        } else {
          // get the earliest time for this node limited by prevNode
          int ristrictTime = asapSchedule[prevNode->getId()] + prevNode->getLatency();
          if (scheduleTime < ristrictTime)
            scheduleTime = ristrictTime;
        }
      }
      if (canSchedule) {
        // node can be scheduled
        asapSchedule[nodeId] = scheduleTime;
        DEBUG("[ASAP]node: " << nodeId << " scheduled at " << scheduleTime);
        toSchedule.push_back(nodeId);
        if (latestTime < scheduleTime)
          latestTime = scheduleTime;
      }
    } // after a iteration over all the nodes left
    if (toSchedule.size() == 0) {
      // no node has been scheduled
      FATAL("[ASAP]ERROR: ASAP schedule not successful");
    }
    for (int nodeId : toSchedule)
      rest.erase(nodeId);
    toSchedule.clear();
  }
  // all nodes scheduled
  return latestTime + 1;
}


void 
Falcon::scheduleALAP(DFG* myDFG, int length)
{
  DEBUG("[ALAP]Started");
  // the ALAP schedule
  std::map<int, int> alapSchedule;
  // set of nodes to be scheduled
  std::vector<int> toSchedule;

  // start with nodes with no successor in same iter
  for (auto node: myDFG->getEndNodes()) 
    toSchedule.push_back(node->getId());

  // end nodes are scheduled at latest time (length - 1)
  for (int node : toSchedule) {
    alapSchedule[node] = length - 1;
    DEBUG("[ALAP]node: " << node << " scheduled at " << length - 1);
  }

  // nodes left to be scheduled
  std::set<int> rest;
  rest = myDFG->getNodeIdSet();

  // remove the nodes that have just been scheduled
  for (int nodeId : toSchedule)
    rest.erase(nodeId);
  toSchedule.clear();

  while (rest.size() > 0) {
    // while there are still unscheduled nodes
    for (int nodeId : rest) {
      // first check if node of nodeId is ready to be ALAP scheduled
      bool canSchedule = true;
      int scheduleTime = length;
      std::vector<Node*> nextNodes = myDFG->getNode(nodeId)->getNextSameIter();
      for (Node* nextNode : nextNodes) {
        if (alapSchedule.find(nextNode->getId()) == alapSchedule.end()) {
          // next node not scheduled, this node not ready
          canSchedule = false;
          break;
        } else {
          // get the latest time for this node limited by nextNode
          int ristrictTime = alapSchedule[nextNode->getId()] - myDFG->getNode(nodeId)->getLatency();
          if (scheduleTime > ristrictTime)
            scheduleTime = ristrictTime;
        }
      }
      if (canSchedule) {
        // node can be scheduled
        alapSchedule[nodeId] = scheduleTime;
        DEBUG("[ALAP]node: " << nodeId << " scheduled at " << scheduleTime);
        toSchedule.push_back(nodeId);
      }
    } // after a iteration over all the nodes left
    if (toSchedule.size() == 0) {
      // no node has been scheduled
      FATAL("[ALAP]ERROR: ALAP schedule not successful");
    }
    for (int nodeId : toSchedule)
      rest.erase(nodeId);
    toSchedule.clear();
  }
  // all nodes scheduled
}