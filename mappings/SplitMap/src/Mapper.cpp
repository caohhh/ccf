/**
 * Mapper.cpp
*/

#include "Mapper.h"


Mapper::Mapper(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy)
{
  this->cgraInfo = cgraInfo;
  this->mappingPolicy = mappingPolicy;
  cgraSize = cgraInfo.X_Dim * cgraInfo.Y_Dim;
}


Mapper::~Mapper()
{
}


bool
Mapper::generateMap(Parser* myParser)
{
  //read input files and construct DFG
  DFG* originalDFG= new DFG();
  myParser->ParseDFG(originalDFG);
  DEBUG("[Mapper]Done parsing DFG");

  originalDFG->preProcess(cgraInfo.MAX_NODE_INDEGREE, cgraInfo.MAX_NODE_OUTDEGREE);
  DEBUG("[Mapper]Done preprocessing DFG");

  // get the recMII
  int recMII  = originalDFG->calculateRecMII();
  DEBUG("[Mapper]Done calculation RecMII of the DFG as " << recMII);

  // resMII
  int resMII = ceil(float(originalDFG->getNodeSize()) / cgraSize);
  DEBUG("[Mapper]Done calculating resMII as " << resMII);

  // find memory resource MII
  int memMII = ceil(float(originalDFG->getLoadOpCount() + originalDFG->getStoreOpCount()) / cgraInfo.Y_Dim);
  DEBUG("[Mapper]Done calculating memMII as " << memMII);

  // the MII is the largest of the three
  int MII = recMII;
  if (resMII > MII)
    MII = resMII;
  if(memMII > MII)
    MII = memMII;
  DEBUG("[Mapper]MII of the schedule is " << MII);

  // ASAP schedule
  int length = scheduleASAP(originalDFG);
  DEBUG("[Mapper]ASAP schedule successful with length of " << length);

  // ALAP schedule
  scheduleALAP(originalDFG, length);
  DEBUG("[Mapper]ALAP schedule successful");

  // ASAP Feasible
  length = scheduleASAPFeasible(originalDFG);
  DEBUG("[Mapper]ASAP Feasible schedule successful with length of " << length);

  return true;
}


int 
Mapper::scheduleASAP(DFG* myDFG)
{
  DEBUG("[ASAP]started");
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
Mapper::scheduleALAP(DFG* myDFG, int length)
{
  DEBUG("[ALAP]Started");
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


bool
Mapper::hasInterIterConfilct(Node* node, int time, schedule sch)
{
  for (auto relatedNode : node->getInterIterRelatedNodes()) {
    if (sch.nodeSchedule.find(relatedNode->getId()) != sch.nodeSchedule.end()) {
      if (sch.nodeSchedule[relatedNode->getId()] == time)
        return true;
    }
  }
  return false;
}


bool
Mapper::memLdResAvailable(int time, schedule sch)
{
  if (!(sch.timeSchedule[time].addBusUsed < cgraInfo.PER_ROW_MEM_AVAILABLE * cgraInfo.X_Dim))
    return false;
  if (!(sch.timeSchedule[time + 1].dataBusUsed < cgraInfo.PER_ROW_MEM_AVAILABLE * cgraInfo.X_Dim))
    return false;
  if (!(sch.timeSchedule[time].peUsed < cgraSize))
    return false;
  if (!(sch.timeSchedule[time + 1].peUsed < cgraSize))
    return false;
  return true;
}


bool 
Mapper::memStResAvailable(int time, schedule sch)
{
  if (!(sch.timeSchedule[time].addBusUsed < cgraInfo.PER_ROW_MEM_AVAILABLE * cgraInfo.X_Dim))
    return false;
  if (!(sch.timeSchedule[time].dataBusUsed < cgraInfo.PER_ROW_MEM_AVAILABLE * cgraInfo.X_Dim))
    return false;
  // 2 pe at same time for a store
  if (!(sch.timeSchedule[time].peUsed < cgraSize - 1))
    return false;
  return true;
}


std::tuple<bool, int> 
Mapper::checkASAP(Node* node, schedule asapSchedule)
{
  bool canSchedule = true;
  int scheduleTime = 0;
  for (Node* prevNode : node->getPrevSameIterExMemDep()) {
    if (asapSchedule.nodeSchedule.find(prevNode->getId()) == asapSchedule.nodeSchedule.end()) {
      // prev node not scheduled, this node not ready
      canSchedule = false;
      return std::make_tuple(canSchedule, scheduleTime);
    } else {
      // get the earliest time for this node limited by prevNode
      int ristrictTime = asapSchedule.nodeSchedule[prevNode->getId()] + prevNode->getLatency();
      if (scheduleTime < ristrictTime)
        scheduleTime = ristrictTime;
    }
  }
  return std::make_tuple(canSchedule, scheduleTime);
}



int 
Mapper::scheduleASAPFeasible(DFG* myDFG)
{
  DEBUG("[ASAP Feasible]start");
  // set of nodes to be scheduled
  std::set<int> toSchedule;
  // latest time of the schedule, to be returned
  int latestTime = 0;
  // start with nodes with no predecessor in same iter
  for (auto node: myDFG->getStartNodes()) 
    toSchedule.insert(node->getId());

  // nodes scheduled in an interation through nodes left
  std::set<int> scheduledNodes;

  // nodes left to be scheduled
  std::set<int> rest = myDFG->getNodeIdSet();

  // for starting nodes
  for (int nodeId : toSchedule) {
    bool scheduled = false;
    Node* node = myDFG->getNode(nodeId);
    if (node->isLoadAddressGenerator()) {
      // if the node to be scheduled is load address generator
      Node* dataNode = node->getMemRelatedNode();
      // dataNode cannot be scheduled
      if (asapFeasible.nodeSchedule.find(dataNode->getId()) != asapFeasible.nodeSchedule.end())
        FATAL("[ASAP Feasible]ERROR! Data node " << dataNode->getId() << "scheduled before address node " << nodeId);

      for (int t = 0; t < mappingPolicy.MAX_LATENCY; t++) {
        // find a time slot
        if (!(asapFeasible.timeSchedule[t].peUsed < cgraSize)) {
          // all pe used at time slot t
          continue;
        }
        // since we want to place nodes with inter iter dependency
        // in the same physical PE
        if (hasInterIterConfilct(node, t, asapFeasible)) {
          continue;
        }
        if (hasInterIterConfilct(dataNode, t+1, asapFeasible)) {
          continue;
        }
        // now to check mem resources
        if (memLdResAvailable(t, asapFeasible)) {
          //if so, allocate those resources
          asapFeasible.timeSchedule[t].addBusUsed++;
          asapFeasible.timeSchedule[t + 1].dataBusUsed++;
          asapFeasible.timeSchedule[t].peUsed++;
          asapFeasible.timeSchedule[t + 1].peUsed++;
          //schedule operations
          asapFeasible.nodeSchedule[nodeId] = t;
          asapFeasible.nodeSchedule[dataNode->getId()] = t + 1;
          asapFeasible.timeSchedule[t].nodes.push_back(nodeId);
          asapFeasible.timeSchedule[t + 1].nodes.push_back(dataNode->getId());
          DEBUG("[ASAP Feasible]load add node: " << nodeId << " scheduled at " << t);
          DEBUG("[ASAP Feasible]load data node: " << dataNode->getId() << " scheduled at " << t + 1);
          //successfully scheduled an operation
          scheduledNodes.insert(nodeId);
          scheduledNodes.insert(dataNode->getId());
          scheduled = true;
          if ((t + 1) > latestTime)
            latestTime = t + 1;
          break;
        }
      } // end of accumulating time
      if (!scheduled)
        FATAL("[ASAP Feasible]ERROR! Reached max time: " << mappingPolicy.MAX_LATENCY << " node: " << nodeId);
    } else if (node->isStoreAddressGenerator()) { 
      // skip store add gen for now
      continue;
    } else if (node->isLoadDataBusRead()) {
      FATAL("[ASAP Feasible]load data " << nodeId << "should not be scheduled");
    } else if (node->isLiveOut()) {
      // also skip liveout nodes, since they need to be scheduled after loop ctrl
      continue;
    } else {
      //find a time slot
      for (int t = 0; t < mappingPolicy.MAX_LATENCY; t++) {
        if (!(asapFeasible.timeSchedule[t].peUsed < cgraSize)) {
          // all pe used at time slot t
          continue;
        }
        if (hasInterIterConfilct(node, t, asapFeasible)) {
          continue;
        }
        //allocate resource
        asapFeasible.timeSchedule[t].peUsed++;
        // schedule the node
        asapFeasible.nodeSchedule[nodeId] = t;
        asapFeasible.timeSchedule[t].nodes.push_back(nodeId);
        DEBUG("[ASAP Feasible]node: " << nodeId << " scheduled at " << t);
        //successfully schedule the operations
        scheduledNodes.insert(nodeId);
        scheduled  = true;
        if (t > latestTime)
          latestTime = t;
        break;
      }
      if (!scheduled) 
        FATAL("[ASAP Feasible]ERROR! Reached max time: " << mappingPolicy.MAX_LATENCY << " node: " << nodeId);
    }
  } // end of iterating through toSchedule nodes
  for (int nodeId : scheduledNodes)
    rest.erase(nodeId);
  scheduledNodes.clear();

  // nodes are mapped in the priority of store -> regular -> load
  // store nodes left to be scheduled
  std::set<int> storeLeft;
  // regular nodes left to be scheduled
  std::set<int> regLeft;
  // load nodes left to be scheduled
  std::set<int> loadLeft;
  for (int nodeId : rest) {
    Node* node = myDFG->getNode(nodeId);
    if (node->isStoreAddressGenerator() || node->isStoreDataBusWrite())
      storeLeft.insert(nodeId);
    else if (node->isLoadAddressGenerator() || node->isLoadDataBusRead())
      loadLeft.insert(nodeId);
    else
      regLeft.insert(nodeId);
  }

  while (rest.size() > 0) {
    // while there are still unscheduled nodes
    bool scheduled = false;

    // start with store nodes
    for (int nodeId : storeLeft) {
      Node* node = myDFG->getNode(nodeId);
      bool canSchedule = true;
      int scheduleTime = 0;
      std::tie(canSchedule, scheduleTime) = checkASAP(node, asapFeasible);

      // also check related node
      Node* relatedNode = node->getMemRelatedNode();
      bool canScheduleRel = true;
      int scheduleTimeRel = 0;
      std::tie(canScheduleRel, scheduleTimeRel) = checkASAP(relatedNode, asapFeasible);
      // make sure both nodes are ready
      if (canSchedule && canScheduleRel) {
        // latest time of the 2
        int startTime = (scheduleTime < scheduleTimeRel) ? scheduleTimeRel : scheduleTime;
        for (int t = startTime; t < mappingPolicy.MAX_LATENCY; t++) {
          // start from lastest time
          if (hasInterIterConfilct(node, t, asapFeasible)) {
            continue;
          }
          if (hasInterIterConfilct(relatedNode, t, asapFeasible)) {
            continue;
          }
          if (node->isLiveOut()) {
            // for live out node, we need to schedule it after loop control node
            Node* loopCtrlNode = myDFG->getLoopCtrlNode();
            // skip node if loop control not scheduled yet
            if (asapFeasible.nodeSchedule.find(loopCtrlNode->getId()) == asapFeasible.nodeSchedule.end()) {
              // loop control node not scheduled
              break;
            } else {
              // loop control node scheduled
              // need to skip to until after lc node is scheduled 
              if (!(t > asapFeasible.nodeSchedule[loopCtrlNode->getId()]))
                continue;
            }
            DEBUG("[ASAP Feasible]" << nodeId << " live out, with ctrl " << loopCtrlNode->getId());
          }
          if (memStResAvailable(t, asapFeasible)) {
            //allocate resources
            asapFeasible.timeSchedule[t].addBusUsed++;
            asapFeasible.timeSchedule[t].dataBusUsed++;
            asapFeasible.timeSchedule[t].peUsed += 2;

            //schedule both operations
            asapFeasible.nodeSchedule[nodeId] = t;
            asapFeasible.nodeSchedule[relatedNode->getId()] = t;
            asapFeasible.timeSchedule[t].nodes.push_back(nodeId);
            asapFeasible.timeSchedule[t].nodes.push_back(relatedNode->getId());
            DEBUG("[ASAP Feasible]store node: " << nodeId << " scheduled at " << t);
            DEBUG("[ASAP Feasible]store node: " << relatedNode->getId() << " scheduled at " << t);
            //successfully scheduled an operation
            scheduledNodes.insert(nodeId);
            scheduledNodes.insert(relatedNode->getId());
            scheduled = true;
            //update schedule length if needed
            if (t > latestTime)
              latestTime = t;
            break;
          }
        } // end of iterating through time
      }
      if (scheduled) {
        // there are shceduled nodes
        // start from beginning
        for (int nodeId : scheduledNodes) {
          rest.erase(nodeId);
          storeLeft.erase(nodeId);
        }
        scheduledNodes.clear();
        break;
      } 
    } // end of iterating through store nodes
    // start from beginning if scheduled
    if (scheduled)
      continue;

    // for regular nodes
    for (int nodeId : regLeft) {
      Node* node = myDFG->getNode(nodeId);
      bool canSchedule = true;
      int scheduleTime = 0;
      // check the earliest time to schedule the node
      std::tie(canSchedule, scheduleTime) = checkASAP(node, asapFeasible);
      if (canSchedule) {
        for (int t = scheduleTime; t < mappingPolicy.MAX_LATENCY; t++) {
          if (hasInterIterConfilct(node, t, asapFeasible))
            continue;
          if (node->isLiveOut()) {
            // for live out node, we need to schedule it after loop control node
            Node* loopCtrlNode = myDFG->getLoopCtrlNode();
            // skip node if loop control not scheduled yet
            if (asapFeasible.nodeSchedule.find(loopCtrlNode->getId()) == asapFeasible.nodeSchedule.end()) {
              // loop control node not scheduled
              break;
            } else {
              // loop control node scheduled
              // need to skip to until after lc node is scheduled 
              if (!(t > asapFeasible.nodeSchedule[loopCtrlNode->getId()]))
                continue;
            }
            DEBUG("[ASAP Feasible]live out node: " << nodeId << ", ctrl node: " << loopCtrlNode->getId());
          }
          // check resource available
          if (asapFeasible.timeSchedule[t].peUsed < cgraSize) {
            //allocate resource
            asapFeasible.timeSchedule[t].peUsed ++;
            //schedule node
            asapFeasible.nodeSchedule[nodeId] = t;
            asapFeasible.timeSchedule[t].nodes.push_back(nodeId);
            DEBUG("[ASAP Feasible]node: " << nodeId << " scheduled at " << t);
            //successfully scheduled an operation
            scheduledNodes.insert(nodeId);
            scheduled = true;
            //update schedule length if needed
            if (t > latestTime)
              latestTime = t;
            break;
          }
        } // end of iterating through time
      }
      if (scheduled) {
        // there are scheduled nodes
        // start from beginning
        for (int nodeId : scheduledNodes) {
          rest.erase(nodeId);
          regLeft.erase(nodeId);
        }
        scheduledNodes.clear();
        break;
      }
    } // end of iterating through regular nodes
    // start from beginning if nodes are scheduled
    if (scheduled)
      continue;
    
    // last for load nodes
    for (int nodeId : loadLeft) {
      Node* node = myDFG->getNode(nodeId);
      Node* addNode;
      Node* dataNode;
      if (node->isLoadAddressGenerator()) {
        addNode = node;
        dataNode = node->getMemRelatedNode();
      } else {
        addNode = node->getMemRelatedNode();
        dataNode = node;
      }
      bool canScheduleAdd = true;
      int scheduleTimeAdd = 0;
      std::tie(canScheduleAdd, scheduleTimeAdd) = checkASAP(addNode, asapFeasible);

      bool canScheduleData = true;
      int scheduleTimeData = 0;
      std::tie(canScheduleData, scheduleTimeData) = checkASAP(dataNode, asapFeasible);
      
      if (canScheduleAdd && canScheduleData) {
        int startTime = (scheduleTimeAdd < (scheduleTimeData - 1)) ? (scheduleTimeData - 1) : scheduleTimeAdd;
        for (int t = startTime; t < mappingPolicy.MAX_LATENCY; t++) {
          if (hasInterIterConfilct(addNode, t, asapFeasible))
            continue;
          if (hasInterIterConfilct(dataNode, t + 1, asapFeasible))
            continue;
          if (memLdResAvailable(t, asapFeasible)) {
            //allocate resources
            asapFeasible.timeSchedule[t].addBusUsed++;
            asapFeasible.timeSchedule[t + 1].dataBusUsed++;
            asapFeasible.timeSchedule[t].peUsed++;
            asapFeasible.timeSchedule[t + 1].peUsed++;
            //schedule operations
            asapFeasible.nodeSchedule[addNode->getId()] = t;
            asapFeasible.nodeSchedule[dataNode->getId()] = t + 1;
            asapFeasible.timeSchedule[t].nodes.push_back(addNode->getId());
            asapFeasible.timeSchedule[t + 1].nodes.push_back(dataNode->getId());
            DEBUG("[ASAP Feasible]load add node: " << addNode->getId() << " scheduled at " << t);
            DEBUG("[ASAP Feasible]load data node: " << dataNode->getId() << " scheduled at " << t + 1);

            //successfully scheduled an operation
            scheduledNodes.insert(addNode->getId());
            scheduledNodes.insert(dataNode->getId());
            scheduled = true;
            //update schedule length if needed
            if ((t + 1) > latestTime)
              latestTime = t + 1;
            break;
          }
        } // end of iterating through time
        if (scheduled) {
          // there are scheduled nodes
          // start from beginning
          for (int nodeId : scheduledNodes) {
            rest.erase(nodeId);
            loadLeft.erase(nodeId);
          }
          scheduledNodes.clear();
          break;
        }
      }
    } // end of iterating through load nodes
    if (!scheduled)
      FATAL("[ASAP Feasible]ERROR! " << rest.size() << " nodes can't be scheduled");
  } // end of while rest.size() > 0
  return latestTime + 1;
}

