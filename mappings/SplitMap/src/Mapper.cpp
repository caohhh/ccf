/**
 * Mapper.cpp
*/

#include "Mapper.h"
#include <queue>
#include <algorithm>

namespace 
{

// if the node through succ iteration can reach dest
bool
downReachable(Node* source, std::set<Node*> dest) {
  std::set<Node*> visited;
  std::queue<Node*> toVisit;
  toVisit.push(source);
  while (!toVisit.empty()) {
    Node* node = toVisit.front();
    toVisit.pop();
    // skip visited nodes
    if (visited.find(node) != visited.end()) 
      continue;
    visited.insert(node);
    for (Node* succ : node->getNextSameIter()) {
      // skip visited nodes
      if (visited.find(succ) != visited.end()) 
        continue;
      if (dest.find(succ) != dest.end())
          return true;
      toVisit.push(succ);
    }
  }
  return false;
}


bool
upReachable(Node* source, std::set<Node*> dest) {
  std::set<Node*> visited;
  std::queue<Node*> toVisit;
  toVisit.push(source);
  while (!toVisit.empty()) {
    Node* node = toVisit.front();
    toVisit.pop();
    // skip visited nodes
    if (visited.find(node) != visited.end()) 
      continue;
    visited.insert(node);
    for (Node* pred : node->getPrevSameIter()) {
      // skip visited nodes
      if (visited.find(pred) != visited.end()) 
        continue;
      if (dest.find(pred) != dest.end())
          return true;
      toVisit.push(pred);
    }
  }
  return false;
}


// get the nodes between source and dest
std::set<Node*>
getNodesBetween(std::set<Node*> source, std::set<Node*> dest) {
  std::set<Node*> nodesBetween;

  // first downward search
  std::queue<Node*> downToVisit;
  std::set<Node*> downVisited;
  for (Node* node : source) {
    if (downReachable(node, dest))
      downToVisit.push(node);
  }
  while (!downToVisit.empty()) {
    Node* node = downToVisit.front();
    downToVisit.pop();
    // skip visited nodes
    if (downVisited.find(node) != downVisited.end()) 
      continue;
    downVisited.insert(node);
    for (Node* succ : node->getNextSameIter()) {
      // skip visited
      if (downVisited.find(succ) != downVisited.end())
        continue;
      // skip nodes in dest
      if (dest.find(succ) != dest.end()) 
        continue;
      // skip nodes in source
      if (source.find(succ) != source.end())
        continue;
      if (downReachable(succ, dest)) {
        nodesBetween.insert(succ);
        downToVisit.push(succ);
      }
    } // end of iterating through succs
  } // end of !downToVisit.empty()

  // next upward search
  std::queue<Node*> upToVisit;
  std::set<Node*> upVisited;
  for (Node* node : source) {
    if (upReachable(node, dest))
      upToVisit.push(node);
  }
  while (!upToVisit.empty()) {
    Node* node = upToVisit.front();
    upToVisit.pop();
    // skip visited nodes
    if (upVisited.find(node) != upVisited.end()) 
      continue;
    upVisited.insert(node);
    for (Node* pred : node->getPrevSameIter()) {
      // skip visited
      if (upVisited.find(pred) != upVisited.end())
        continue;
      // skip nodes in dest
      if (dest.find(pred) != dest.end()) 
        continue;
      // skip nodes in source
      if (source.find(pred) != source.end())
        continue;
      if (upReachable(pred, dest)) {
        nodesBetween.insert(pred);
        upToVisit.push(pred);
      }
    } // end of iterating through preds
  } // end of !upToVisit.empty()
  return nodesBetween;
}


} // end of anonymous namespace


Mapper::Mapper(CGRA_Architecture cgraInfo, Mapping_Policy mappingPolicy)
{
  this->cgraInfo = cgraInfo;
  this->mappingPolicy = mappingPolicy;
  cgraSize = cgraInfo.X_Dim * cgraInfo.Y_Dim;
  // rng here at construction
  std::random_device rd;
  rng = std::mt19937(rd());
  asapFeasible = new schedule(cgraInfo.X_Dim, cgraInfo.Y_Dim);
  alapFeasible = new schedule(cgraInfo.X_Dim, cgraInfo.Y_Dim);
  modSchedule = new moduloSchedule(cgraInfo.X_Dim, cgraInfo.Y_Dim);
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

  // ALAP Feasible
  scheduleALAPFeasible(originalDFG, MII);
  DEBUG("[Mapper]ALAP Feasible schedule successful");

  DEBUG("[Mapper]Done premapping with II of " << MII);

  // get nodes sorted
  std::vector<Node*> sortedNodes = getSortedNodes(originalDFG);

  int currentII = MII;
  while (currentII <= mappingPolicy.MAX_II) {
    DEBUG("[Mapper]Mapping for II: " << currentII);
    // Fraction of scheduling space based on user input.
    int lambda =  mappingPolicy.LAMBDA * cgraSize * currentII * originalDFG->getNodeSize();
    // attempt count for current II
    int attempt = 0;
    for (int attemp = 0; attempt < lambda; attemp++) {
      DEBUG("[Mapper]attempt: " << attempt << "/" << lambda << "");
      bool modScheduleSuccess = false;
      DFG* routeDFG = nullptr;
      // here we do modulo schedule
      for (int modAttempt = 0; modAttempt < mappingPolicy.MODULO_SCHEDULING_ATTEMPTS; modAttempt++) {
        DEBUG("[Mapper]modulo schedule attempt " << modAttempt << " of " << mappingPolicy.MODULO_SCHEDULING_ATTEMPTS);
        if (scheduleModulo(originalDFG, sortedNodes, currentII)) {
          DEBUG("[Mapper]Modulo schedule complete and successful");
          modSchedule->print(originalDFG);
          // next, for nodes that can't be immediately accessed, insert routing nodes
          // a new DFG to insert all the routing nodes inserted
          routeDFG = new DFG(*originalDFG);
          if (!insertRoute(routeDFG)) {
            DEBUG("[Mapper]insert routing failed, retry modulo schedule");
            continue;
          } else {
            modScheduleSuccess = true;
            DEBUG("[Mapper]insert route success");
            break;
          }
        } else
          DEBUG("[Mapper]Modulo schedule attempt failed");
      } // end of modulo schedule attempts
      if (!modScheduleSuccess) {
        DEBUG("[Mapper]Failed to generate modulo schedule for current II, increasing II");
        currentII++;
        break;
      }
      modSchedule->print(routeDFG);
      timeExCgra = new CGRA(cgraInfo.X_Dim, cgraInfo.Y_Dim, currentII);
      // now falcon mapping
      falconPlace(routeDFG, currentII);
      exit(0);
    } // end of iterating through attempts
    break;
  } // end of while current II <= maxII

  return true;
}


std::tuple<bool, int>
Mapper::checkModulo(Node* node)
{
  bool canSchedule = true;
  int scheduleTime = alapFeasible->getScheduleTime(node);
  // check succ nodes
  for (Node* succ : node->getSuccSameIterExMemDep()) {
    if (!modSchedule->isScheduled(succ)) {
      canSchedule = false;
      return std::make_tuple(canSchedule, scheduleTime);
    } else {
      int restrictTime = modSchedule->getScheduleTime(succ) - node->getLatency();
      if (restrictTime < scheduleTime)
        scheduleTime = restrictTime;
    }
  }
  return std::make_tuple(canSchedule, scheduleTime);
}


bool
Mapper::scheduleModulo(DFG* myDFG, std::vector<Node*> sortedNodes, int II)
{
  DEBUG("[Modulo]Start");
  modSchedule->clear();
  modSchedule->setII(II);
  // nodes to schedule, which is all the sorted nodes
  std::vector<Node*> toSchedule = sortedNodes;

  while (!toSchedule.empty()) {
    // get the node to schedule int he priority of:
    // lowest slack -> position in sorted nodes(longest cycle -> latest in alap)
    Node* scheduleNode = nullptr;
    int lowestSlack = INT32_MAX;
    // first get all the nodes ready to schedule
    for (Node* node : toSchedule) {
      // skip add gen
      if (node->isLoadAddressGenerator() || node->isStoreAddressGenerator())
        continue;
      if (node->isLoadDataBusRead() || node->isStoreDataBusWrite()) {
        // check if mem related node is ready for scheduling
        if (!(std::get<0>(checkModulo(node->getMemRelatedNode()))))
          continue;
      }
      if (std::get<0>(checkModulo(node))) {
        // node ready for modulo scheduling
        // update slack
        int slack = std::get<1>(checkModulo(node)) - asapFeasible->getScheduleTime(node);
        if (slack < lowestSlack) {
          lowestSlack = slack;
          scheduleNode = node;
        }
      }
    }
    if (scheduleNode == nullptr) {
      #ifdef DEBUG
        DEBUG("[Modulo]No node to schedule with " << toSchedule.size() << " nodes left: ");
        for (Node* node : toSchedule)
          DEBUG("[Modulo]node: " << node->getId() << " not scheduled");
      #endif
      return false;
    }
    DEBUG("[Modulo]Scheduling node: " << scheduleNode->getId());
    // now we check the time this node can be scheduled at
    int startTime = getModConstrainedTime(scheduleNode);
    // check loop carried dependencies of the node to schedule
    // as in, the node should start after LCD node is finished
    for(Node* pred : scheduleNode->getExDepPredPrevIter()) {
      if (!modSchedule->isScheduled(pred))
        FATAL("[Modulo]ERROR!Loop carried preds should be scheduled first");
      int distance = myDFG->getArc(pred, scheduleNode)->getDistance();
      int earliestStart = modSchedule->getScheduleTime(pred) - distance * II + pred->getLatency();
      if (earliestStart > startTime)
        startTime = earliestStart;
    }
    int endTime = std::get<1>(checkModulo(scheduleNode));
    DEBUG("[Modulo]start time: " << startTime <<", end time: " << endTime);
    if (startTime > endTime) {
      DEBUG("[Modulo]Failed: startTime > endTime");
      return false;
    }
    // random times to try and schedule the node
    std::vector<int> tryTime(endTime - startTime + 1);
    std::iota(tryTime.begin(), tryTime.end(), startTime);
    // a rand time between start and end to schedule the node
    std::shuffle(tryTime.begin(), tryTime.end(), rng);
    bool scheduled = false;

    // now to schedule the node at a tryTime
    for (int scheduleTime : tryTime) {
      int modScheduleTime = scheduleTime % II;
      DEBUG("[Modulo]schedule time: " << scheduleTime << ", modulo schedule time: " << modScheduleTime);
      if (scheduleNode->isLoadDataBusRead()) {
        // first deal with loads, since we've skipped address generators already
        Node* addNode;
        addNode = scheduleNode->getMemRelatedNode();
        // we've made sure related node is also ready earlier
        // add node should be scheduled a cycle earlier than data node for load
        int latestAdd = std::get<1>(checkModulo(addNode));
        if (latestAdd < (scheduleTime - 1)) {
          // latest address node schedule time earlier than intended, try another time
          continue;
        } else {
          // lateset address node schedule time later than intended, can be scheduled
          if (modSchedule->memLdResAvailable(scheduleTime - 1)) {
            // enough resources, schedule both nodes
            modSchedule->scheduleLd(addNode, scheduleNode, scheduleTime - 1);
            scheduled = true;
            for (auto it = toSchedule.begin(); it != toSchedule.end();) {
              if (*it == scheduleNode || *it == addNode) {
                it = toSchedule.erase(it);
              } else
                ++it;
            } // end of iterating through toSchedule
            DEBUG("[Modulo]Success in scheduling load data node");
            DEBUG("[Modulo]With the load address node: " << addNode->getId() 
                  << " at time: " << scheduleTime - 1 << " mod: " << (scheduleTime - 1) % II);
            break;
          } else {
            // not enough resources, try another time
            continue;
          }
        }

      } else if (scheduleNode->isStoreDataBusWrite()) {
        // with stores, note that we've skipped address generators earlier
        Node* addNode = scheduleNode->getMemRelatedNode();
        int latestAdd = std::get<1>(checkModulo(addNode));
        // for store, both address node and data node are scheduled at time
        if (latestAdd < scheduleTime) {
          // latest address node schedule time earlier than intended, try another time
          continue;
        } else {
          // lateset address node schedule time later than intended, can be scheduled
          if (modSchedule->memStResAvailable(scheduleTime)) {
            // enough resources, schedule both nodes
            modSchedule->scheduleSt(addNode, scheduleNode, scheduleTime);
            scheduled = true;
            for (auto it = toSchedule.begin(); it != toSchedule.end();) {
              if (*it == scheduleNode || *it == addNode) {
                it = toSchedule.erase(it);
              } else
                ++it;
            } // end of iterating through toSchedule
            DEBUG("[Modulo]Success in scheduling store data node");
            DEBUG("[Modulo]With the store address node: " << addNode->getId() 
                  << " at time: " << scheduleTime - 1 << " mod: " << (scheduleTime - 1) % II);
            break;
          } else {
            // not enough resources, try another time
            continue;
          }
        }

      } else {
        // regular nodes
        if (modSchedule->resAvailable(scheduleTime)) {
          modSchedule->scheduleOp(scheduleNode, scheduleTime);
          DEBUG("[Modulo]Schedule successful");
          scheduled = true;
          for (auto it = toSchedule.begin(); it != toSchedule.end(); ++it) {
            if (*(it) == scheduleNode) {
              toSchedule.erase(it);
              break;
            }
          } // end of iterating through toSchedule
          break;
        } else {
          DEBUG("[Modulo]Failed, to next try time");
          continue;
        }
      }
    } // end of iterating through tryTime
    if (!scheduled) {
      DEBUG("[Modulo]FAILED!!!!Can't schedule the node");
      return false;
    }
  } // end of !toSchedule.empty()
  return true;
}


int
Mapper::getModConstrainedTime(Node* node)
{
  bool constrained = false;
  int constrainedTime = INT32_MIN;
  for (Node* pred : node->getPrevSameIter()) {
    if (modSchedule->isScheduled(pred)) {
      // if node is constrained by pred
      //if (isModConstrainedBy(node, pred)) {
        int earliestTime = modSchedule->getScheduleTime(pred) + pred->getLatency();
        if (earliestTime > constrainedTime) {
          constrained = true;
          constrainedTime = earliestTime;
        }
      //}
    }
  } // end of iterating preds
  if (!constrained)
    constrainedTime = asapFeasible->getScheduleTime(node);
  return constrainedTime;
}


bool
Mapper::isModConstrainedBy(Node* source, Node* pred) {
  std::set<Node*> visited;
  std::queue<Node*> toVisit;
  toVisit.push(source);
  while (!toVisit.empty()) {
    Node* node = toVisit.front();
    toVisit.pop();
    // skip visited nodes
    if (visited.find(node) != visited.end()) 
      continue;
    visited.insert(node);
    if (!modSchedule->isScheduled(node))
      continue;
    // nodes in same iter
    if (node != source) {
      for (Node* succ : node->getSuccSameIterExMemDep()) {
        // skip visited nodes
        if (visited.find(succ) != visited.end()) 
          continue;
        // skip not mod scheduled
        if (!modSchedule->isScheduled(succ))
          continue;
        // also check if node constrains the succ
        if (modSchedule->getScheduleTime(node) + node->getLatency() < modSchedule->getScheduleTime(succ)) {
          if (succ == pred)
            return true;
        }
        toVisit.push(succ);
      }
    }
    // nodes in next iter
    for (Node* succ : node->getExDepSuccNextIter()) {
      // skip visited nodes
      if (visited.find(succ) != visited.end()) 
        continue;
      // skip not mod scheduled
      if (!modSchedule->isScheduled(succ))
        continue;
      if (succ == pred)
        return true;
      toVisit.push(succ);
    }
  }
  return false;
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


std::tuple<bool, int> 
Mapper::checkASAP(Node* node)
{
  bool canSchedule = true;
  int scheduleTime = 0;
  for (Node* prevNode : node->getPrevSameIterExMemDep()) {
    if (!asapFeasible->isScheduled(prevNode)) {
      // prev node not scheduled, this node not ready
      canSchedule = false;
      return std::make_tuple(canSchedule, scheduleTime);
    } else {
      // get the earliest time for this node limited by prevNode
      int ristrictTime = asapFeasible->getScheduleTime(prevNode) + prevNode->getLatency();
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
      if (asapFeasible->isScheduled(dataNode))
        FATAL("[ASAP Feasible]ERROR! Data node " << dataNode->getId() << "scheduled before address node " << nodeId);

      for (int t = 0; t < mappingPolicy.MAX_LATENCY; t++) {
        // find a time slot
        // since we want to place nodes with inter iter dependency
        // in the same physical PE
        if (asapFeasible->hasInterIterConfilct(node, t)) {
          continue;
        }
        if (asapFeasible->hasInterIterConfilct(dataNode, t + 1)) {
          continue;
        }
        // now to check mem resources
        if (asapFeasible->memLdResAvailable(t)) {
          asapFeasible->scheduleLd(node, dataNode, t);
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
        if (!asapFeasible->resAvailable(t)) {
          // all pe used at time slot t
          continue;
        }
        if (asapFeasible->hasInterIterConfilct(node, t)) {
          continue;
        }
        asapFeasible->scheduleOp(node, t);
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
      std::tie(canSchedule, scheduleTime) = checkASAP(node);

      // also check related node
      Node* relatedNode = node->getMemRelatedNode();
      bool canScheduleRel = true;
      int scheduleTimeRel = 0;
      std::tie(canScheduleRel, scheduleTimeRel) = checkASAP(relatedNode);
      // make sure both nodes are ready
      if (canSchedule && canScheduleRel) {
        // latest time of the 2
        int startTime = (scheduleTime < scheduleTimeRel) ? scheduleTimeRel : scheduleTime;
        for (int t = startTime; t < mappingPolicy.MAX_LATENCY; t++) {
          // start from lastest time
          if (asapFeasible->hasInterIterConfilct(node, t)) {
            continue;
          }
          if (asapFeasible->hasInterIterConfilct(relatedNode, t)) {
            continue;
          }
          if (node->isLiveOut()) {
            // for live out node, we need to schedule it after loop control node
            Node* loopCtrlNode = myDFG->getLoopCtrlNode();
            // skip node if loop control not scheduled yet
            if (!asapFeasible->isScheduled(loopCtrlNode)) {
              // loop control node not scheduled
              break;
            } else {
              // loop control node scheduled
              // need to skip to until after lc node is scheduled 
              if (!(t > asapFeasible->getScheduleTime(loopCtrlNode)))
                continue;
            }
            DEBUG("[ASAP Feasible]" << nodeId << " live out, with ctrl " << loopCtrlNode->getId());
          }
          if (asapFeasible->memStResAvailable(t)) {
            asapFeasible->scheduleSt(node, relatedNode, t);
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
      std::tie(canSchedule, scheduleTime) = checkASAP(node);
      if (canSchedule) {
        for (int t = scheduleTime; t < mappingPolicy.MAX_LATENCY; t++) {
          if (asapFeasible->hasInterIterConfilct(node, t))
            continue;
          if (node->isLiveOut()) {
            // for live out node, we need to schedule it after loop control node
            Node* loopCtrlNode = myDFG->getLoopCtrlNode();
            // skip node if loop control not scheduled yet
            if (!asapFeasible->isScheduled(loopCtrlNode)) {
              // loop control node not scheduled
              break;
            } else {
              // loop control node scheduled
              // need to skip to until after lc node is scheduled 
              if (!(t > asapFeasible->getScheduleTime(loopCtrlNode)))
                continue;
            }
            DEBUG("[ASAP Feasible]live out node: " << nodeId << ", ctrl node: " << loopCtrlNode->getId());
          }
          // check resource available
          if (asapFeasible->resAvailable(t)) {
            asapFeasible->scheduleOp(node, t);
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
      std::tie(canScheduleAdd, scheduleTimeAdd) = checkASAP(addNode);

      bool canScheduleData = true;
      int scheduleTimeData = 0;
      std::tie(canScheduleData, scheduleTimeData) = checkASAP(dataNode);
      
      if (canScheduleAdd && canScheduleData) {
        int startTime = (scheduleTimeAdd < (scheduleTimeData - 1)) ? (scheduleTimeData - 1) : scheduleTimeAdd;
        for (int t = startTime; t < mappingPolicy.MAX_LATENCY; t++) {
          if (asapFeasible->hasInterIterConfilct(addNode, t))
            continue;
          if (asapFeasible->hasInterIterConfilct(dataNode, t + 1))
            continue;
          if (asapFeasible->memLdResAvailable(t)) {
            asapFeasible->scheduleLd(addNode, dataNode, t);
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


std::tuple<bool, int> 
Mapper::checkALAP(Node* node, int II)
{
  bool canSchedule = true;
  int scheduleTime = INT32_MAX;
  // first check succ nodes
  std::vector<Node*> succNodes = node->getSuccSameIterExMemDep();
  if (succNodes.size() > 0) {
    for (Node* succNode : node->getSuccSameIterExMemDep()) {
      if (!alapFeasible->isScheduled(succNode)) {
        canSchedule = false;
        return std::make_tuple(canSchedule, scheduleTime);
      } else {
        int restrictTime = alapFeasible->getScheduleTime(succNode) - node->getLatency();
        if (restrictTime < scheduleTime)
          scheduleTime = restrictTime;
      }
    }
  } else {
    // this is before the next apprearance of the node
    scheduleTime = asapFeasible->getScheduleTime(node) + II - node->getLatency();
  }
  return std::make_tuple(canSchedule, scheduleTime);
}


void
Mapper::scheduleALAPFeasible(DFG* myDFG, int II)
{
  DEBUG("[ALAP Feasible]start");
  // nodes left to be scheduled
  std::set<int> rest = myDFG->getNodeIdSet();
  // start with nodes without succ in same iter
  for (Node* node : myDFG->getEndNodes()) {
    // if there are nodes scheduled through an iter
    bool scheduled = false;
    // the last time this node can be scheduled at
    int lastTime = asapFeasible->getScheduleTime(node) + II - node->getLatency();
    if (node->isLoadDataBusRead()) {
      Node* addNode = node->getMemRelatedNode();
      for (int t = lastTime; t > -1; t--) {
        if (alapFeasible->hasInterIterConfilct(addNode, t - 1))
          continue;
        if (alapFeasible->hasInterIterConfilct(node, t))
          continue;
        if (alapFeasible->memLdResAvailable(t - 1)) {
          // enough resources
          alapFeasible->scheduleLd(addNode, node, t - 1);
          DEBUG("[ALAP Feasible]load add node: " << addNode->getId() << " scheduled at " << t - 1);
          DEBUG("[ALAP Feasible]load data node: " << node->getId() << " scheduled at " << t);
          rest.erase(node->getId());
          rest.erase(addNode->getId());
          scheduled = true;
          break;
        }
      } // end of iterating through time
    } else if (node->isStoreDataBusWrite()) {
      Node* addNode = node->getMemRelatedNode();
      for (int t = lastTime; t > -1; t--) {
        if (alapFeasible->hasInterIterConfilct(addNode, t))
          continue;
        if (alapFeasible->hasInterIterConfilct(node, t))
          continue;
        if (alapFeasible->memStResAvailable(t)) {
          alapFeasible->scheduleSt(addNode, node, t);
          DEBUG("[ALAP Feasible]store add node: " << addNode->getId() << " scheduled at " << t);
          DEBUG("[ALAP Feasible]store data node: " << node->getId() << " scheduled at " << t);
          //successfully scheduled an operation
          rest.erase(node->getId());
          rest.erase(addNode->getId());
          scheduled = true;
          break;
        }
      } // end of iterating through time
    } else {
      for (int t = lastTime; t > -1; t--) {
        if (alapFeasible->hasInterIterConfilct(node, t))
          continue;
        if (node->isLoopCtrl()) {
          // loop control node shoud be before live out nodes
          std::vector<Node*> liveOutNodes = myDFG->getLiveOutNodes();
          bool constrained = false;
          for (Node* liveOutNode : liveOutNodes) {
            if (!(asapFeasible->getScheduleTime(liveOutNode) > t)) {
              constrained = true;
              break;
            }
          } // end of iterating through live out nodes
          if (constrained)
            continue;
          else
            DEBUG("[ALAP Feasible]loop ctrl node: " << node->getId());
        }
        if (alapFeasible->resAvailable(t)) {
          alapFeasible->scheduleOp(node, t);
          DEBUG("[ALAP Feasible]node: " << node->getId() << " scheduled at " << t);
          //successfully scheduled an operation
          rest.erase(node->getId());
          scheduled = true;
          break;
        }
      } // end of iterating through time
    }
    if (!scheduled)
      FATAL("[ALAP Feasible]ERROR! Node " << node->getId() << " can't be scheduled");
  } // end of iterating through end nodes

  // now schedule rest of the nodes
  while (rest.size() > 0) {
    // while nodes left
    std::set<int> scheduledNodes;
    for (int nodeId : rest) {
      // skip scheduled nodes
      if (scheduledNodes.find(nodeId) != scheduledNodes.end())
        continue;
      Node* node = myDFG->getNode(nodeId);
      if (node->isStoreAddressGenerator() || node->isStoreDataBusWrite()) {
        // store node
        Node* relatedNode = node->getMemRelatedNode();
        bool canSchedule = true;
        int scheduleTime = 0;
        std::tie(canSchedule, scheduleTime) = checkALAP(node, II);

        // also check related node
        bool canScheduleRel = true;
        int scheduleTimeRel = 0;
        std::tie(canScheduleRel, scheduleTimeRel) = checkALAP(relatedNode, II);

        if (canSchedule & canScheduleRel) {
          // earlier of the two
          int lasttTime = (scheduleTime < scheduleTimeRel) ? scheduleTime : scheduleTimeRel;
          for (int t = lasttTime; t > -1; t--) {
            if (alapFeasible->hasInterIterConfilct(node, t))
              continue;
            if (alapFeasible->hasInterIterConfilct(relatedNode, t))
              continue;
            if (alapFeasible->memStResAvailable(t)) {
              alapFeasible->scheduleSt(node, relatedNode, t);
              DEBUG("[ALAP Feasible]store node: " << relatedNode->getId() << " scheduled at " << t);
              DEBUG("[ALAP Feasible]store node: " << node->getId() << " scheduled at " << t);
              //successfully scheduled an operation
              scheduledNodes.insert(nodeId);
              scheduledNodes.insert(relatedNode->getId());
              break;
            }
          }// end of iterating through time
        }
      } else if (node->isLoadAddressGenerator() || node->isLoadDataBusRead()) {
        // load node
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
        std::tie(canScheduleAdd, scheduleTimeAdd) = checkALAP(addNode, II);

        bool canScheduleData = true;
        int scheduleTimeData = 0;
        std::tie(canScheduleData, scheduleTimeData) = checkALAP(dataNode, II);
        
        if (canScheduleAdd && canScheduleData) {
          int lastTime = (scheduleTimeAdd < (scheduleTimeData - 1)) ? scheduleTimeAdd : (scheduleTimeData - 1);
          for (int t = lastTime; t > -1; t--) {
            if (alapFeasible->hasInterIterConfilct(addNode, t))
              continue;
            if (alapFeasible->hasInterIterConfilct(dataNode, t + 1))
              continue;
            if (alapFeasible->memLdResAvailable(t)) {
              // enough resources
              alapFeasible->scheduleLd(addNode, dataNode, t);
              DEBUG("[ALAP Feasible]load add node: " << addNode->getId() << " scheduled at " << t);
              DEBUG("[ALAP Feasible]load data node: " << dataNode->getId() << " scheduled at " << t + 1);
              scheduledNodes.insert(dataNode->getId());
              scheduledNodes.insert(addNode->getId());
              break;
            }
          } // end of iterating through time
        }
      } else {
        //regular node
        bool canSchedule = true;
        int scheduleTime = 0;
        std::tie(canSchedule, scheduleTime) = checkALAP(node, II);
        if (canSchedule) {
          for (int t = scheduleTime; t > -1; t--) {
            if (alapFeasible->hasInterIterConfilct(node, t))
              continue;
            if (node->isLoopCtrl()) {
              // make sure loop control is before all live out nodes
              std::vector<Node*> liveOutNodes = myDFG->getLiveOutNodes();
              bool constrained = false;
              for (Node* liveOutNode : liveOutNodes) {
                if (!(asapFeasible->getScheduleTime(liveOutNode) > t)) {
                  constrained = true;
                  break;
                }
              } // end of iterating through live out nodes
              if (constrained)
                continue;
              else
                DEBUG("[ALAP Feasible]loop ctrl node: " << node->getId());
            }
            if (alapFeasible->resAvailable(t)) {
              alapFeasible->scheduleOp(node, t);
              DEBUG("[ALAP Feasible]node: " << nodeId << " scheduled at " << t);
              //successfully scheduled an operation
              scheduledNodes.insert(nodeId);
              break;
            }
          } // end of iterating through time
        }
      }
    } // end of iterating throug rest
    if (scheduledNodes.size() > 0) {
      for (int nodeId : scheduledNodes)
        rest.erase(nodeId);
      scheduledNodes.clear();
    } else
      FATAL("[ALAP Feasible]ERROR! " << rest.size() << " nodes can't be scheduled");
  } // end of rest.size() > 0
}


std::vector<Node*>
Mapper::getSortedNodes(DFG* myDFG)
{
  std::vector<std::set<Node*>> sortedSets;
  std::set<Node*> visitedNodes;
  // first we sort all the cycles and the nodes connecting them
  for (auto cycle : myDFG->getCycles()) {
    // skip cycles with only 1 node
    if (cycle.size() == 1)
      continue;
    // first push in the nodes connecting this cycle to visited cycles
    std::set<Node*> nodesBetween = getNodesBetween(cycle, visitedNodes);
    sortedSets.push_back(nodesBetween);
    visitedNodes.insert(nodesBetween.begin(), nodesBetween.end());
    // next push in the cycle
    sortedSets.push_back(cycle);
    visitedNodes.insert(cycle.begin(), cycle.end());
  }
  // nodes left not in cycle or path between
  std::set<Node*> nodesLeft;
  for (int nodeId : myDFG->getNodeIdSet()) {
    Node* node = myDFG->getNode(nodeId);
    if (visitedNodes.find(node) == visitedNodes.end())
      nodesLeft.insert(node);
  }
  sortedSets.push_back(nodesLeft);

  std::vector<Node*> sortedNodes;
  // each set in the sortedSets are now sorted using asap schedule
  for (auto nodeSet : sortedSets) {
    // nodes in nodeSet sorted asap high to low
    std::vector<Node*> asapSorted;
    for (Node* node : nodeSet) {
      auto it = asapSorted.begin();
      for (; it != asapSorted.end(); ++it) {
        Node* nodeIt = *it;
        if (asapFeasible->getScheduleTime(node) > asapFeasible->getScheduleTime(nodeIt))
          break;
      }
      asapSorted.insert(it, node);
    }
    for (Node* node : asapSorted)
      sortedNodes.push_back(node);
  }
  DEBUG("[Sorted Nodes]Done sorting with nodes ordered as:");
  for (auto node : sortedNodes)
    DEBUG("[Sorted Nodes]" << node->getId());

  return sortedNodes;
}


bool
Mapper::insertRoute(DFG* myDFG)
{
  DEBUG("[Route]Started");
  // first get routes needed
  for (int nodeId : myDFG->getNodeIdSet()) {
    Node* node = myDFG->getNode(nodeId);
    // time data of this node is ready
    int readyTime = modSchedule->getScheduleTime(node) + node->getLatency();
    // make sure nodes are scheduled here
    if (!modSchedule->isScheduled(node))
      FATAL("ERROR!!!All nodes should be modulo scheduled by now");
    // vector for all the succs that need routing and their need time
    std::vector<std::tuple<Node*, int>> needRoute;
    // check all successors of the node
    for (Node* succ : node->getNextNodes()) {
      // skip mem nodes
      if (node->getMemRelatedNode() == succ)
        continue;
      // the time this data is needed for the succ node
      int needTime = modSchedule->getScheduleTime(succ) + myDFG->getArc(node, succ)->getDistance() * modSchedule->getII();
      if (needTime < readyTime) {
        FATAL("[Route]ERROR!!!!! Data ready later than needed" <<
              "Node: " << nodeId << " ready time: " << readyTime <<
              "Succ node: " << succ->getId() << " Need time: " << needTime);
      }
      if (needTime > readyTime) {
        // need routing node, need route is sorted from needtime low to high
        auto needRouteIt = needRoute.begin();
        for (;needRouteIt != needRoute.end(); ++needRouteIt) {
          if (needTime < std::get<1>(*needRouteIt))
            break;
        }
        needRoute.insert(needRouteIt, std::make_tuple(succ, needTime));
      }
    } // end of checking all succs
    if (!needRoute.empty()) {
      // if there need routing to succs for this node
      DEBUG("[Route]Node: " << nodeId << " with ready time: " << readyTime << " needs routing to");
      // lastest copy of the data ready at time
      int curReadyTime = readyTime;
      // lastest copy of the data ready at node
      Node* curReadyNode = node;
      for (auto needRouteIt : needRoute) {
        // data is needed at node
        Node* needNode = std::get<0>(needRouteIt);
        // data is needed at time
        int needTime = std::get<1>(needRouteIt);
        DEBUG("[Route]Succ: " << needNode->getId() << " need time: " << needTime);
        while (curReadyTime < needTime) {
          if (!modSchedule->resAvailable(curReadyTime)) {
            DEBUG("[Route]Not enough resources for a routing node at time " << curReadyTime);
            DEBUG("[Route]Getting a new modulo schedule");
            return false;
          }
          // enough resources for a routing node
          // add a routing node for the time
          routeNode* newRouteNode = new routeNode(curReadyNode, curReadyNode->getBrPath());
          myDFG->insertNode(newRouteNode);
          // connect it to the prev node
          myDFG->makeArc(curReadyNode, newRouteNode, 0, TrueDep, 0);
          // modulo shcedule the new route node
          modSchedule->scheduleOp(newRouteNode, curReadyTime);
          DEBUG("[Route]Route Node: " << newRouteNode->getId() << " at time " << curReadyTime << " added");
          // update current ready time and node
          curReadyNode = newRouteNode;
          curReadyTime += newRouteNode->getLatency();
        } // end of reaching need time
        // disconnect need node from original node
        Arc* arcOld = myDFG->getArc(node, needNode);
        myDFG->removeArc(arcOld->getId());
        // connect it to ready node
        myDFG->makeArc(curReadyNode, needNode, arcOld->getDistance(), arcOld->getDependency(), arcOld->getOperandOrder());
      } // end of checking all routes needed to be added
    }
  } // end of checking all nodes
  return true;
}


bool
Mapper::falconPlace(DFG* myDFG, int II)
{
  DEBUG("[Falcon]Start placing");
  // set of the ids of the nodes left to map, start with all the nodes in the DFG
  auto nodeSetToMap = myDFG->getNodeIdSet();
  int searchSpace = cgraSize * II * nodeSetToMap.size() * mappingPolicy.MAX_MAPPING_ATTEMPTS;
  DEBUG("[Falcon]Max search space: " << searchSpace);

  /************************************************************************************************/
  // first construct M
  // check out refine_M in CGRA.cpp for FalconCrimson
  // what is M
  // M is a matrix size of nodesize x tablesize
  // table is of the mapping pairs
  // if this node is the node of the mapping pair, it is a 1 in M

  // for adjacency list:
  // mode 5 is each node's pred, other mode is each node's succ

  // essentially, M will be our mapping result in the end,
  // each row is a node, and each column is a time-extanded pe

  // now maybe most of these won't even be used
  /************************************************************************************************/
  
  /**
   * for placing, we have several ways to find a node to map
   * 0: Completely random
   * 1: Priority of nodes having outgoing recurrent edges
   * 2: Priority of having incoming recurrent
   * 3: Priority of high fan-in fan-out
   * 4: Priority of having no incoming nodes
   * 5: Priority of having no outgoing edges
  */
  int mappingMode = mappingPolicy.MAPPING_MODE;
  while (!nodeSetToMap.empty()) {
    // the uid of the node to map
    int startNode = -1;  
    if (mappingMode == 0) {
      // completely random
      // ID of the nodes to map
      std::vector<int> toMap(nodeSetToMap.begin(), nodeSetToMap.end());
      std::uniform_int_distribution<std::size_t> uni(0, nodeSetToMap.size() - 1);
      // the node to map
      int randomIndex = uni(rng);
      startNode = toMap[randomIndex];
      DEBUG("[Falcon]Start node: " << startNode);
    } else if (mappingMode == 1) {
      // nodes with outgoing recurrent edges
    }
    if (startNode == -1)
      FATAL("[Falcon]Can't find start node to map");

    // map the node
    std::queue<int> mappingQueue;
    mappingQueue.push(startNode);
    // mark the node as visited, remove it from set to map
    nodeSetToMap.erase(startNode);
    while (!mappingQueue.empty()) {
      // the node to map
      Node* node = myDFG->getNode(mappingQueue.front());
      mappingQueue.pop();
      DEBUG("[Falcon]Mapping node: " << node->getId());
      // now based on the mapping status, get free coordinates this node can be mapped to
      std::vector<PE*> potentialPos = getPotentialPos(node);
      if (potentialPos.empty()) {
        DEBUG("[Falcon]No position, remapping");
        // no position for this node, try to remap and find a potential position
        bool remapSuccess = remapBasic(node);
        // in falcon, it goes as: shallow->shallow_n->1deep
        if (!remapSuccess)
          FATAL("not done yet");
      } else {
        // there is position for this node, choose a random one for now
        std::uniform_int_distribution<std::size_t> uni(0, potentialPos.size() - 1);
        // the PE selected to map the node on
        PE* selPE = potentialPos.at(uni(rng));
        // map the node
        timeExCgra->mapNode(node, selPE);
        DEBUG("[Falcon]Mapped at PE: <" << std::get<0>(selPE->getCoord()) << ", " << 
                std::get<1>(selPE->getCoord()) << ", " << std::get<2>(selPE->getCoord()) << ">");
        timeExCgra->print();
      }
      // with the node mapped, there is updating potential pos of its mapped preds and succs
      
      // now we add next nodes to map
      if (mappingMode == 5) {
        // mode 5 maps pred
        for (auto pred : node->getPrevNodes()) {
          if (nodeSetToMap.find(pred->getId()) != nodeSetToMap.end()) {
            mappingQueue.push(pred->getId());
            nodeSetToMap.erase(pred->getId());
          }
        }
      } else {
        // other mode maps succ
        for (auto succ : node->getNextNodes()) {
          // next we add succs of the node to the queue
          if (nodeSetToMap.find(succ->getId()) != nodeSetToMap.end()) {
            mappingQueue.push(succ->getId());
            nodeSetToMap.erase(succ->getId());
          }
        }
      }
    } // end of mappingQueue not empty
  } // end of nodeSetToMap not empty

  timeExCgra->print();
  return true;
}


std::vector<PE*>
Mapper::getPotentialPos(Node* node)
{
  DEBUG("[PotentialPos]Finding potential position for node: " << node->getId());
  bool mapped = false;
  PE* mappedPe = nullptr;
  // if the node is already mapped, we first remove the node and map it at the same PE in the end
  if (timeExCgra->getPeMapped(node->getId()) != nullptr) {
    // node already mapped
    // first note down the PE
    mapped = true;
    mappedPe = timeExCgra->getPeMapped(node->getId());
    // remove the node
    timeExCgra->removeNode(node);
  }
  // get the already mapped pred and succ of the node
  std::vector<Node*> mappedPreds;
  std::vector<Node*> mappedSuccs;
  for (Node* pred : node->getPrevNodes()) {
    if (timeExCgra->getPeMapped(pred->getId()) != nullptr) {
      // pred is mapped
      mappedPreds.push_back(pred);
    }
  } // end of iterating through preds
  for (Node* succ : node->getNextNodes()) {
    if (timeExCgra->getPeMapped(succ->getId()) != nullptr) {
      // the succ is mapped
      mappedSuccs.push_back(succ);
    }
  } // end of iterating through succs
  DEBUG("[PotentialPos]Number of mapped preds: " << mappedPreds.size() << " succs: " << mappedSuccs.size());
  // the potential positions to return
  std::vector<PE*> potentialPos;

  // for mem nodes, there are more restrictions
  if (node->isMemNode()) {
    if (node->isLoadAddressGenerator()) {
      // load address generator
      // first check for mapped succs, should only be the read node
      if (mappedSuccs.size() > 0) {
        // succ mapped for this load add gen, should only be 1 load read
        if (mappedSuccs.size() > 1)
          FATAL("[PotentialPos]ERROR! Load add gen with more than 1 succ");
        Node* dataNode = mappedSuccs[0];
        if (!dataNode->isLoadDataBusRead())
          FATAL("[PotentialPos]ERROR! Load add gen succ not a read");
        // made sure there is only one succ: data read
        int xCoor = std::get<0>(timeExCgra->getPeMapped(dataNode->getId())->getCoord());
        Row* row = timeExCgra->getRow(xCoor, modSchedule->getModScheduleTime(node));
        if (row->memResAvailable()) {
          for (auto pe: timeExCgra->getPeAtRow(row)) {
            if (pe->getNode() == -1)
              potentialPos.push_back(pe);
          } // end of iterating through PEs at row
        }
      } else { // end of mapped succ size > 0
        // with no mapped succ, get all free rows
        for (auto row : timeExCgra->getRowAtTime(modSchedule->getModScheduleTime(node))) {
          if (row->memResAvailable()) {
            for (auto pe: timeExCgra->getPeAtRow(row)) {
              if (pe->getNode() == -1)
                potentialPos.push_back(pe);
            } // end of iterating through PEs at row
          }
        } // end of iterating through rows at mod schedule time
      }
      // next check for mapped preds, there should at most be 1
      if (mappedPreds.size() > 1)
        FATAL("[PotentialPos]ERROR! load add gen should only have 1 pred");
      // next check if it is connected to mapped preds
      auto posIt = potentialPos.begin();
      for (; posIt != potentialPos.end(); ) {
        // if this pos is removed
        bool removed = false;
        for (auto pred : mappedPreds) {
          PE* predPE = timeExCgra->getPeMapped(pred->getId());
          if (!timeExCgra->isAccessable(predPE, *posIt)) {
            // not accessable, remove this pos
            posIt = potentialPos.erase(posIt);
            removed = true;
            break;
          }
        } // end of iterating through mapped preds
        if (!removed)
          ++posIt;
      } // end of iterating through potential positions

    } else if (node->isLoadDataBusRead()) {
      // load data bus read
      // check pred first, should only be the add node
      if (mappedPreds.size() > 0) {
        if (mappedPreds.size() > 1)
          FATAL("[PotentialPos]ERROR! read node with more than 1 pred");
        Node* addNode = mappedPreds[0];
        if (!addNode->isLoadAddressGenerator())
          FATAL("[PotentialPos]ERROR! read node pred node add gen");
        int xCoor = std::get<0>(timeExCgra->getPeMapped(addNode->getId())->getCoord());
        Row* row = timeExCgra->getRow(xCoor, modSchedule->getModScheduleTime(node));
        if (row->memResAvailable()) {
          for (auto pe: timeExCgra->getPeAtRow(row)) {
            if (pe->getNode() == -1)
              potentialPos.push_back(pe);
          } // end of iterating through PEs at row
        }
      } else { // end of mapped pred size > 0
        // with no mapped pred, get all free rows
        for (auto row : timeExCgra->getRowAtTime(modSchedule->getModScheduleTime(node))) {
          if (row->memResAvailable()) {
            for (auto pe: timeExCgra->getPeAtRow(row)) {
              if (pe->getNode() == -1)
                potentialPos.push_back(pe);
            } // end of iterating through PEs at row
          }
        } // end of iterating through rows at mod schedule time
      }
      // next check succs
      auto posIt = potentialPos.begin();
      for (; posIt != potentialPos.end(); ) {
        // if this pos is removed
        bool removed = false;
        for (auto succ : mappedSuccs) {
          PE* succPE = timeExCgra->getPeMapped(succ->getId());
          if (!timeExCgra->isAccessable(*posIt, succPE)) {
            // not accessable, remove this pos
            posIt = potentialPos.erase(posIt);
            removed = true;
            break;
          }
        } // end of iterating through mapped preds
        if (!removed)
          ++posIt;
      } // end of iterating through potential positions
    
    } else if (node->isStoreAddressGenerator()) {
      // check succ for the data node
      if (mappedSuccs.size() > 0) {
        // there should only be 1 succ
        Node* dataNode = mappedSuccs[0];
        int xCoor = std::get<0>(timeExCgra->getPeMapped(dataNode->getId())->getCoord());
        Row* row = timeExCgra->getRow(xCoor, modSchedule->getModScheduleTime(node));
        for (auto pe: timeExCgra->getPeAtRow(row)) {
          if (pe->getNode() == -1)
            potentialPos.push_back(pe);
        } // end of iterating through PEs at row
      } else {
        // with no mapped succ, get all free rows
        for (auto row : timeExCgra->getRowAtTime(modSchedule->getModScheduleTime(node))) {
          if (row->memResAvailable()) {
            for (auto pe: timeExCgra->getPeAtRow(row)) {
              if (pe->getNode() == -1)
                potentialPos.push_back(pe);
            } // end of iterating through PEs at row
          }
        } // end of iterating through rows at mod schedule time
      }
      // next check if it is connected to mapped preds, should only be 1
      auto posIt = potentialPos.begin();
      for (; posIt != potentialPos.end(); ) {
        // if this pos is removed
        bool removed = false;
        for (auto pred : mappedPreds) {
          PE* predPE = timeExCgra->getPeMapped(pred->getId());
          if (!timeExCgra->isAccessable(predPE, *posIt)) {
            // not accessable, remove this pos
            posIt = potentialPos.erase(posIt);
            removed = true;
            break;
          }
        } // end of iterating through mapped preds
        if (!removed)
          ++posIt;
      } // end of iterating through potential positions

    } else if (node->isStoreDataBusWrite()) {
      // for write, there are two preds, one add one data
      // the address gen pred node
      Node* addPred = nullptr;
      // the data provider pred node
      Node* dataPred = nullptr; 
      for (auto pred : mappedPreds) {
        if (pred->isStoreAddressGenerator()) {
          addPred = pred;
        } else {
          dataPred = pred;
        }
      }
      if (addPred != nullptr) {
        // address gen pred mapped, row fixed
        int xCoor = std::get<0>(timeExCgra->getPeMapped(addPred->getId())->getCoord());
        Row* row = timeExCgra->getRow(xCoor, modSchedule->getModScheduleTime(node));
        for (auto pe: timeExCgra->getPeAtRow(row)) {
          if (pe->getNode() == -1)
            potentialPos.push_back(pe);
        } // end of iterating through PEs at row
      } else {
        // add gen pred not mapped, can choose all rows with mem res available
        for (auto row : timeExCgra->getRowAtTime(modSchedule->getModScheduleTime(node))) {
          if (row->memResAvailable()) {
            for (auto pe: timeExCgra->getPeAtRow(row)) {
              if (pe->getNode() == -1)
                potentialPos.push_back(pe);
            } // end of iterating through PEs at row
          }
        } // end of iterating through rows at mod schedule time
      }
      if (dataPred != nullptr) {
        auto posIt = potentialPos.begin();
        for (; posIt != potentialPos.end(); ) {
          // if this pos is removed
          bool removed = false;
          PE* predPE = timeExCgra->getPeMapped(dataPred->getId());
          if (!timeExCgra->isAccessable(predPE, *posIt)) {
            // not accessable, remove this pos
            posIt = potentialPos.erase(posIt);
            removed = true;
            break;
          }
          if (!removed)
            ++posIt;
        } // end of iterating through potential positions
      }
      // sanity check for succs
      if (mappedSuccs.size() > 0)
        FATAL("[PotentialPos]ERROR! Store write node with succ mapped");
    } // end of is store write
  } else { // end of node is mem node
    // normal nodes
    // first get all the free PEs at time based on modulo schedule
    for (auto pe : timeExCgra->getPeAtTime(modSchedule->getModScheduleTime(node))) {
      if (pe->getNode() == -1)
        potentialPos.push_back(pe);
    } // end of iterating through PEs at modulo schedule time

    // next check if it is connected to mapped preds
    auto posIt = potentialPos.begin();
    for (; posIt != potentialPos.end(); ) {
      // if this pos is removed
      bool removed = false;
      for (auto pred : mappedPreds) {
        PE* predPE = timeExCgra->getPeMapped(pred->getId());
        if (!timeExCgra->isAccessable(predPE, *posIt)) {
          // not accessable, remove this pos
          posIt = potentialPos.erase(posIt);
          removed = true;
          break;
        }
      } // end of iterating through mapped preds
      if (!removed)
        ++posIt;
    } // end of iterating through potential positions

    // also check mapped succ
    posIt = potentialPos.begin();
    for (; posIt != potentialPos.end(); ) {
      // if this pos is removed
      bool removed = false;
      for (auto succ : mappedSuccs) {
        PE* succPE = timeExCgra->getPeMapped(succ->getId());
        if (!timeExCgra->isAccessable(*posIt, succPE)) {
          // not accessable, remove this pos
          posIt = potentialPos.erase(posIt);
          removed = true;
          break;
        }
      } // end of iterating through mapped preds
      if (!removed)
        ++posIt;
    } // end of iterating through potential positions
  } // end of not a mem node

  DEBUG("[PotentialPos]Potential Pos Size " << potentialPos.size());
  // restore the mapped node
  if (mapped) 
    timeExCgra->mapNode(node, mappedPe);
  return potentialPos;
}


bool
Mapper::remapBasic(Node* failedNode)
{
  DEBUG("[Remap Basic]Remapping failed node " << failedNode->getId());
  // first get all the mapped preds and succs constraining the node, we try and remap the constraints
  // get the already mapped pred and succ of the node, also remove them for remap
  std::vector<Node*> mappedNodes;
  // the original PE mapped for potential restore
  std::map<int, PE*> originalPos; 
  for (Node* pred : failedNode->getPrevNodes()) {
    if (timeExCgra->getPeMapped(pred->getId()) != nullptr) {
      // pred is mapped
      mappedNodes.push_back(pred);
      originalPos[pred->getId()] = timeExCgra->getPeMapped(pred->getId());
      timeExCgra->removeNode(pred);
    }
  } // end of iterating through preds
  for (Node* succ : failedNode->getNextNodes()) {
    if (timeExCgra->getPeMapped(succ->getId()) != nullptr) {
      // the succ is mapped
      mappedNodes.push_back(succ);
      originalPos[succ->getId()] = timeExCgra->getPeMapped(succ->getId());
      timeExCgra->removeNode(succ);
    }
  } // end of iterating through succs
  if (mappedNodes.empty())
    FATAL("[Remap Basic]ERROR! Probably should not happen, remapping a node without constraints");
  // we can check all the potential mappings of mapped preds and succs to find a mapping
  std::vector<std::vector<PE*>> positionsLeft;
  // if we should use positions in positionsLeft
  bool useLeft = false;
  int constraintIt = 0;
  while (constraintIt != -1) {
    Node* constraint = mappedNodes[constraintIt];
    std::vector<PE*> potentialPos;
    // see where potentialPos should come from first
    if (useLeft) {
      potentialPos = positionsLeft.back();
      positionsLeft.pop_back();
      timeExCgra->removeNode(constraint);
    } else {
      potentialPos = getPotentialPos(constraint);
      // shuffle to potentialPos here since potentialPos is ordered
      std::shuffle(potentialPos.begin(), potentialPos.end(), rng);
    }
    
    if (potentialPos.empty()) {
      // no position to map, return to last constraint and choose another position
      constraintIt--;
      useLeft = true;
    } else {
      timeExCgra->mapNode(constraint, potentialPos.back());
      potentialPos.pop_back();
      positionsLeft.push_back(potentialPos);
      constraintIt++;
      useLeft = false;
    }
    if (constraintIt == (int)mappedNodes.size()) {
      // found a position composition for all constraints
      auto remapPos = getPotentialPos(failedNode);
      if (!remapPos.empty()) {
        // there is position for this node, choose a random one for now
        std::uniform_int_distribution<std::size_t> uni(0, remapPos.size() - 1);
        PE* selPE = remapPos.at(uni(rng));
        // map the node
        timeExCgra->mapNode(failedNode, selPE);
        DEBUG("[Remap Basic]Remap success at PE: <" << std::get<0>(selPE->getCoord()) << ", " << 
                std::get<1>(selPE->getCoord()) << ", " << std::get<2>(selPE->getCoord()) << ">");
        timeExCgra->print();
        return true;
      } else {
        constraintIt--;
        useLeft = true;
      }
    }
  } // end of while constraintIt != -1 
  // remap failed, need to restore the constraint's location
  DEBUG("[Remap Basic]Remap failed, restoring constraints' location");
  for (auto constraint : mappedNodes) {
    timeExCgra->mapNode(constraint, originalPos[constraint->getId()]);
  }
  return false;
}