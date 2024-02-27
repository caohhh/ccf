/**
 * Schedule.cpp
*/

#include "Schedule.h"
#include <fstream>
#include <string>


/***********schedule***************/

schedule::schedule()
{
  xDim = 4;
  yDim = 4;
  cgraSize = xDim * yDim;
}


schedule::~schedule()
{
}


schedule::schedule(int xDim, int yDim)
{
  this->xDim = xDim;
  this->yDim = yDim;
  cgraSize = xDim * yDim;
}


bool
schedule::isScheduled(Node* node)
{
  if (nodeSchedule.find(node->getId()) != nodeSchedule.end())
    return true;
  else
    return false;
}


int
schedule::getScheduleTime(Node* node)
{
  if (isScheduled(node))
    return nodeSchedule[node->getId()];
  else
    return -1;
}


bool
schedule::hasInterIterConfilct(Node* node, int time)
{
  for (auto relatedNode : node->getInterIterRelatedNodes()) {
    if (isScheduled(relatedNode)) {
      if (getScheduleTime(relatedNode) == time)
        return true;
    }
  }
  return false;
}


bool
schedule::memLdResAvailable(int time)
{
  if (!(addBusUsed[time] < perRowMem * xDim))
    return false;
  if (!(dataBusUsed[time + 1] < perRowMem * xDim))
    return false;
  if (!(peUsed[time] < cgraSize))
    return false;
  if (!(peUsed[time + 1] < cgraSize))
    return false;
  return true;
}


bool 
schedule::memStResAvailable(int time)
{
  if (!(addBusUsed[time] < perRowMem * xDim))
    return false;
  if (!(dataBusUsed[time] < perRowMem * xDim))
    return false;
  // 2 pe at same time for a store
  if (!(peUsed[time] < cgraSize - 1))
    return false;
  return true;
}


bool 
schedule::resAvailable(int time)
{
  if (peUsed[time] < cgraSize)
    return true;
  else
    return false;
}


void
schedule::scheduleLd(Node* addNode, Node* dataNode, int time)
{
  if (!memLdResAvailable(time))
    return;
  // allocate resources
  addBusUsed[time]++;
  dataBusUsed[time + 1]++;
  peUsed[time]++;
  peUsed[time + 1]++;
  //schedule operations
  nodeSchedule[addNode->getId()] = time;
  nodeSchedule[dataNode->getId()] = time + 1;
  timeSchedule[time].push_back(addNode->getId());
  timeSchedule[time + 1].push_back(dataNode->getId());
}


void
schedule::scheduleOp(Node* node, int time)
{
  if (!resAvailable(time))
    return;
  //allocate resource
  peUsed[time]++;
  // schedule the node
  nodeSchedule[node->getId()] = time;
  timeSchedule[time].push_back(node->getId());
}


void 
schedule::scheduleSt(Node* storeNode, Node* storeRelatedNode, int time)
{
  if (!memStResAvailable(time))
    return;
  //allocate resources
  addBusUsed[time]++;
  dataBusUsed[time]++;
  peUsed[time] += 2;
  //schedule both operations
  nodeSchedule[storeNode->getId()] = time;
  nodeSchedule[storeRelatedNode->getId()] = time;
  timeSchedule[time].push_back(storeNode->getId());
  timeSchedule[time].push_back(storeRelatedNode->getId());
}


int
schedule::getMaxTime()
{
  int maxTime = INT32_MIN;
  for (auto timeIt : timeSchedule) {
    int time = timeIt.first;
    if (time > maxTime)
      maxTime = time;
  }
  return maxTime;
}


/************moduloSchedule**************/

moduloSchedule::moduloSchedule(int xDim, int yDim) :
schedule(xDim, yDim)
{
}


moduloSchedule::~moduloSchedule()
{
}


void
moduloSchedule::setII(int II)
{
  this->II = II;
}


bool 
moduloSchedule::resAvailable(int scheduleTime)
{
  int modTime = scheduleTime % II;
  if (peUsed[modTime] < cgraSize)
    return true;
  else
    return false;
}


void
moduloSchedule::scheduleOp(Node* node, int time)
{
  int modTime = time % II;
  if (!resAvailable(time))
    return;
  //allocate resource
  peUsed[modTime]++;
  // schedule the node
  nodeSchedule[node->getId()] = time;
  timeSchedule[time].push_back(node->getId());
  // also mod
  nodeScheduleMod[node->getId()] = modTime;
  timeScheduleMod[modTime].push_back(node->getId());
}


bool 
moduloSchedule::memLdResAvailable(int time)
{
  int modTimeAdd = time % II;
  int modTimeData = (time + 1) % II;
  if (!(addBusUsed[modTimeAdd] < perRowMem * xDim))
    return false;
  if (!(dataBusUsed[modTimeData] < perRowMem * xDim))
    return false;
  if (!(peUsed[modTimeAdd] < cgraSize))
    return false;
  if (!(peUsed[modTimeData] < cgraSize))
    return false;
  return true;
}


void
moduloSchedule::scheduleLd(Node* addNode, Node* dataNode, int time)
{
  int modTimeAdd = time % II;
  int modTimeData = (time + 1) % II;
  if (!memLdResAvailable(time))
    return;
  // allocate resources
  addBusUsed[modTimeAdd]++;
  dataBusUsed[modTimeData]++;
  peUsed[modTimeAdd]++;
  peUsed[modTimeData]++;
  //schedule operations
  nodeSchedule[addNode->getId()] = time;
  nodeSchedule[dataNode->getId()] = time + 1;
  timeSchedule[time].push_back(addNode->getId());
  timeSchedule[time + 1].push_back(dataNode->getId());
  // also mod
  nodeScheduleMod[addNode->getId()] = modTimeAdd;
  nodeScheduleMod[dataNode->getId()] = modTimeData;
  timeScheduleMod[modTimeAdd].push_back(addNode->getId());
  timeScheduleMod[modTimeData].push_back(dataNode->getId());
}


bool 
moduloSchedule::memStResAvailable(int time)
{
  int modTime = time % II;
  if (!(addBusUsed[modTime] < perRowMem * xDim))
    return false;
  if (!(dataBusUsed[modTime] < perRowMem * xDim))
    return false;
  // 2 pe at same time for a store
  if (!(peUsed[modTime] < cgraSize - 1))
    return false;
  return true;
}


void 
moduloSchedule::scheduleSt(Node* storeNode, Node* storeRelatedNode, int time)
{
  int modTime = time % II;
  if (!memStResAvailable(time))
    return;
  //allocate resources
  addBusUsed[modTime]++;
  dataBusUsed[modTime]++;
  peUsed[modTime] += 2;
  //schedule both operations
  nodeSchedule[storeNode->getId()] = time;
  nodeSchedule[storeRelatedNode->getId()] = time;
  timeSchedule[time].push_back(storeNode->getId());
  timeSchedule[time].push_back(storeRelatedNode->getId());
  // also mod
  nodeScheduleMod[storeNode->getId()] = modTime;
  nodeScheduleMod[storeRelatedNode->getId()] = modTime;
  timeScheduleMod[modTime].push_back(storeNode->getId());
  timeScheduleMod[modTime].push_back(storeRelatedNode->getId());
}


void
moduloSchedule::clear()
{
  nodeSchedule.clear();
  timeSchedule.clear();
  peUsed.clear();
  addBusUsed.clear();
  dataBusUsed.clear();
  nodeScheduleMod.clear();
  timeScheduleMod.clear();
}


void
moduloSchedule::print(DFG* myDFG)
{
  std::ofstream dotFile;
  std::string fileName = "Modulo_Schedule.dot";
  dotFile.open(fileName);

  dotFile << "digraph " << "Modulo_Schedule" << " { \n{\n";
  // print node
  for (auto nodeIt : nodeSchedule) {
    int nodeId = nodeIt.first;
    if (myDFG->getNode(nodeId)->isMemNode())
      dotFile << nodeId << " [color=blue];\n";
    else if (myDFG->getNode(nodeId)->getIns() == route)
      dotFile << nodeId << " [color=green];\n";
    else
      dotFile << nodeId << " [color=red];\n";
  }
  dotFile << "\n";

  // print arc
  for (int arcId : myDFG->getArcIdSet()) {
    Arc* arc = myDFG->getArc(arcId);
    dotFile << arc->getFromNode()->getId() << " -> " << arc->getToNode()->getId();
    if (arc->getDependency() == PredDep) {
      if (arc->getDistance() == 0) {
        dotFile << " [color=blue]\n";
      }
      else {
        dotFile << " [style=bold, color=blue, label=" << arc->getDistance() << "] \n";
      }
    } else if (arc->getDependency() == TrueDep) {
      if (arc->getDistance() == 0) {
        dotFile << " [color=red]\n";
      }
      else {
        dotFile << " [style=bold, color=red, label=" << arc->getDistance() << "] \n";
      }
    } else if (arc->getDependency() == LoadDep || arc->getDependency() == PredDep) {
      dotFile << " [style=dotted, color=blue, label= mem] \n";
    } else {
      FATAL("[Print Modulo]Encountered arc dependency not implemented " << arc->getDependency());
    }
  }
  dotFile << "}\n";

  // print time
  dotFile << "\n{\nnode [shape=plaintext]; \nT0";
  for (int t = 1; t < II ; t++) {
    dotFile << " -> T" << t;
  }
  dotFile << " -> T0;\n}\n";
  dotFile << "{\nrank = source; \n";
  dotFile << "T0" << ";\n} \n";
  for (int t = 0; t < II ; t++) {
    auto nodes = timeScheduleMod[t];
    dotFile << "{\nrank = same; \n";
    for (int id : nodes)
      dotFile << id << "; ";
    dotFile << "T" << t << ";";
    dotFile << "\n}\n";
  }
  dotFile << "\n} \n";
  dotFile.close();
}


int
moduloSchedule::getII()
{
  return II;
}


int
moduloSchedule::getModScheduleTime(Node* node)
{
  if (isScheduled(node))
    return nodeScheduleMod[node->getId()];
  else
    return -1;
}