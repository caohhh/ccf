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
schedule::memLdResAvailable(nodePath path, int time)
{ 
  // for none path, it should be none path + max of other paths(which is true and false for now)
  // for other path, it should be this path + none path

  // add bus used
  int addU;
  // data bus used
  int dataU;
  // pe used at address time slot
  int peAddU;
  // pe used at data time slot
  int peDataU;

  if (path == none) {
    int trueAdd = addBusUsed[std::make_tuple(true_path, time)];
    int falseAdd = addBusUsed[std::make_tuple(false_path, time)];
    int trueData = dataBusUsed[std::make_tuple(true_path, time + 1)];
    int falseData = dataBusUsed[std::make_tuple(false_path, time + 1)];
    int truePeAdd = peUsed[std::make_tuple(true_path, time)];
    int falsePeAdd = peUsed[std::make_tuple(false_path, time)];
    int truePeData = peUsed[std::make_tuple(true_path, time + 1)];
    int falsePeData = peUsed[std::make_tuple(false_path, time + 1)];
    addU = addBusUsed[std::make_tuple(none, time)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
    dataU = dataBusUsed[std::make_tuple(none, time + 1)] + ((trueData > falseData) ? trueData : falseData);
    peAddU = peUsed[std::make_tuple(none, time)] + ((truePeAdd > falsePeAdd) ? truePeAdd : falsePeAdd);
    peDataU = peUsed[std::make_tuple(none, time + 1)] + ((truePeData > falsePeData) ? truePeData : falsePeData);
  } else {
    addU = addBusUsed[std::make_tuple(path, time)] + addBusUsed[std::make_tuple(none, time)];
    dataU = dataBusUsed[std::make_tuple(path, time + 1)] + dataBusUsed[std::make_tuple(none, time + 1)];
    peAddU = peUsed[std::make_tuple(path, time)] + peUsed[std::make_tuple(none, time)];
    peDataU = peUsed[std::make_tuple(path, time + 1)] + peUsed[std::make_tuple(none, time + 1)];
  }

  if (!(addU < perRowMem * xDim))
    return false;
  if (!(dataU < perRowMem * xDim))
    return false;
  if (!(peAddU < cgraSize))
    return false;
  if (!(peDataU < cgraSize))
    return false;
  return true;
}


bool 
schedule::memStResAvailable(nodePath path, int time)
{
  // add bus used
  int addU;
  // data bus used
  int dataU;
  // pe used
  int peU;

  if (path == none) {
    int trueAdd = addBusUsed[std::make_tuple(true_path, time)];
    int falseAdd = addBusUsed[std::make_tuple(false_path, time)];
    int trueData = dataBusUsed[std::make_tuple(true_path, time)];
    int falseData = dataBusUsed[std::make_tuple(false_path, time)];
    int truePe = peUsed[std::make_tuple(true_path, time)];
    int falsePe = peUsed[std::make_tuple(false_path, time)];
    addU = addBusUsed[std::make_tuple(none, time)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
    dataU = dataBusUsed[std::make_tuple(none, time)] + ((trueData > falseData) ? trueData : falseData);
    peU = peUsed[std::make_tuple(none, time)] + ((truePe > falsePe) ? truePe : falsePe);
  } else {
    addU = addBusUsed[std::make_tuple(path, time)] + addBusUsed[std::make_tuple(none, time)];
    dataU = dataBusUsed[std::make_tuple(path, time)] + dataBusUsed[std::make_tuple(none, time)];
    peU = peUsed[std::make_tuple(path, time)] + peUsed[std::make_tuple(none, time)];
  }

  if (!(addU < perRowMem * xDim))
    return false;
  if (!(dataU < perRowMem * xDim))
    return false;
  // 2 pe at same time for a store
  if (!(peU < cgraSize - 1))
    return false;
  return true;
}


bool 
schedule::resAvailable(nodePath path, int time)
{
  // pe used
  int peU;

  if (path == none) {
    int truePe = peUsed[std::make_tuple(true_path, time)];
    int falsePe = peUsed[std::make_tuple(false_path, time)];
    peU = peUsed[std::make_tuple(none, time)] + ((truePe > falsePe) ? truePe : falsePe);
  } else {
    peU = peUsed[std::make_tuple(path, time)] + peUsed[std::make_tuple(none, time)];
  }

  if (peU < cgraSize)
    return true;
  else
    return false;
}


void
schedule::scheduleLd(Node* addNode, Node* dataNode, int time)
{
  // the 2 nodes should belong to the same path
  if (addNode->getBrPath() != dataNode->getBrPath())
    FATAL("ERROR!! address node and data node not in the same path");
  nodePath path = addNode->getBrPath();
  if (!memLdResAvailable(path, time))
    return;
  // allocate resources
  addBusUsed[std::make_tuple(path, time)]++;
  dataBusUsed[std::make_tuple(path, time + 1)]++;
  peUsed[std::make_tuple(path, time)]++;
  peUsed[std::make_tuple(path, time + 1)]++;
  //schedule operations
  nodeSchedule[addNode->getId()] = time;
  nodeSchedule[dataNode->getId()] = time + 1;
  timeSchedule[time].push_back(addNode->getId());
  timeSchedule[time + 1].push_back(dataNode->getId());
}


void
schedule::scheduleOp(Node* node, int time)
{
  nodePath path = node->getBrPath();
  if (!resAvailable(path, time))
    return;
  //allocate resource
  peUsed[std::make_tuple(path, time)]++;
  // schedule the node
  nodeSchedule[node->getId()] = time;
  timeSchedule[time].push_back(node->getId());
}


void 
schedule::scheduleSt(Node* storeNode, Node* storeRelatedNode, int time)
{
  // the 2 nodes should belong to the same path
  if (storeNode->getBrPath() != storeRelatedNode->getBrPath())
    FATAL("ERROR!! 2 store nodes not in the same path");
  nodePath path = storeNode->getBrPath();
  if (!memStResAvailable(path, time))
    return;
  //allocate resources
  addBusUsed[std::make_tuple(path, time)]++;
  dataBusUsed[std::make_tuple(path, time)]++;
  peUsed[std::make_tuple(path, time)] += 2;
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

moduloSchedule::moduloSchedule(int xDim, int yDim, int length) :
schedule(xDim, yDim)
{
  this->length = length;
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
moduloSchedule::resAvailable(nodePath path, int scheduleTime)
{
  int modTime = scheduleTime % II;

  // pe used
  int peU = 0;
  // for other iterations
  for (int t = 0; t < length; t++) {
    if ((t % II) == modTime && t != scheduleTime) {
      // time slots that will be mapped together
      int truePe = peUsed[std::make_tuple(true_path, t)];
      int falsePe = peUsed[std::make_tuple(false_path, t)];
      peU += peUsed[std::make_tuple(none, t)] + ((truePe > falsePe) ? truePe : falsePe);
    }
  }
  // for this iteration, which would be of scheduleTime
  if (path == none) {
    int truePe = peUsed[std::make_tuple(true_path, scheduleTime)];
    int falsePe = peUsed[std::make_tuple(false_path, scheduleTime)];
    peU += peUsed[std::make_tuple(none, scheduleTime)] + ((truePe > falsePe) ? truePe : falsePe);
  } else {
    peU += peUsed[std::make_tuple(path, scheduleTime)] + peUsed[std::make_tuple(none, scheduleTime)];
  }

  if (peU < cgraSize)
    return true;
  else
    return false;
}


void
moduloSchedule::scheduleOp(Node* node, int time)
{
  nodePath path = node->getBrPath();
  int modTime = time % II;
  if (!resAvailable(path, time))
    return;
  //allocate resource
  peUsed[std::make_tuple(path, time)]++;
  // schedule the node
  nodeSchedule[node->getId()] = time;
  timeSchedule[time].push_back(node->getId());
  // also mod
  nodeScheduleMod[node->getId()] = modTime;
  timeScheduleMod[modTime].push_back(node->getId());
}


bool 
moduloSchedule::memLdResAvailable(nodePath path, int scheduleTime)
{
  int modTimeAdd = scheduleTime % II;
  int modTimeData = (scheduleTime + 1) % II;

  // add bus used
  int addU = 0;
  // data bus used
  int dataU = 0;
  // pe used at address time slot
  int peAddU = 0;
  // pe used at data time slot
  int peDataU = 0;

  // first for add node
  // for other iterations
  for (int t = 0; t < length; t++) {
    if ((t % II) == modTimeAdd && t != scheduleTime) {
      // time slots that will be mapped together
      int truePe = peUsed[std::make_tuple(true_path, t)];
      int falsePe = peUsed[std::make_tuple(false_path, t)];
      peAddU += peUsed[std::make_tuple(none, t)] + ((truePe > falsePe) ? truePe : falsePe);
      int trueAdd = addBusUsed[std::make_tuple(true_path, t)];
      int falseAdd = addBusUsed[std::make_tuple(false_path, t)];
      addU += addBusUsed[std::make_tuple(none, t)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
    }
  }
  // for this iteration, which would be of scheduleTime
  if (path == none) {
    int truePe = peUsed[std::make_tuple(true_path, scheduleTime)];
    int falsePe = peUsed[std::make_tuple(false_path, scheduleTime)];
    peAddU += peUsed[std::make_tuple(none, scheduleTime)] + ((truePe > falsePe) ? truePe : falsePe);
    int trueAdd = addBusUsed[std::make_tuple(true_path, scheduleTime)];
    int falseAdd = addBusUsed[std::make_tuple(false_path, scheduleTime)];
    addU += addBusUsed[std::make_tuple(none, scheduleTime)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
  } else {
    peAddU += peUsed[std::make_tuple(path, scheduleTime)] + peUsed[std::make_tuple(none, scheduleTime)];
    addU += addBusUsed[std::make_tuple(path, scheduleTime)] + addBusUsed[std::make_tuple(none, scheduleTime)];
  }

  // next for data node
  // for other iterations
  for (int t = 0; t < length; t++) {
    if ((t % II) == modTimeData && t != (scheduleTime + 1)) {
      // time slots that will be mapped together
      int truePe = peUsed[std::make_tuple(true_path, t)];
      int falsePe = peUsed[std::make_tuple(false_path, t)];
      peDataU += peUsed[std::make_tuple(none, t)] + ((truePe > falsePe) ? truePe : falsePe);
      int trueData = dataBusUsed[std::make_tuple(true_path, t)];
      int falseData = dataBusUsed[std::make_tuple(false_path, t)];
      dataU += dataBusUsed[std::make_tuple(none, t)] + ((trueData > falseData) ? trueData : falseData);
    }
  }
  // for this iteration, which would be of scheduleTime
  if (path == none) {
    int truePe = peUsed[std::make_tuple(true_path, scheduleTime + 1)];
    int falsePe = peUsed[std::make_tuple(false_path, scheduleTime + 1)];
    peDataU += peUsed[std::make_tuple(none, scheduleTime + 1)] + ((truePe > falsePe) ? truePe : falsePe);
    int trueData = dataBusUsed[std::make_tuple(true_path, scheduleTime + 1)];
    int falseData = dataBusUsed[std::make_tuple(false_path, scheduleTime + 1)];
    dataU += dataBusUsed[std::make_tuple(none, scheduleTime + 1)] + ((trueData > falseData) ? trueData : falseData);
  } else {
    peDataU += peUsed[std::make_tuple(path, scheduleTime + 1)] + peUsed[std::make_tuple(none, scheduleTime + 1)];
    dataU += dataBusUsed[std::make_tuple(path, scheduleTime + 1)] + dataBusUsed[std::make_tuple(none, scheduleTime + 1)];
  }

  if (!(addU < perRowMem * xDim))
    return false;
  if (!(dataU < perRowMem * xDim))
    return false;
  if (!(peAddU < cgraSize))
    return false;
  if (!(peDataU < cgraSize))
    return false;
  return true;
}


void
moduloSchedule::scheduleLd(Node* addNode, Node* dataNode, int time)
{
  // the 2 nodes should belong to the same path
  if (addNode->getBrPath() != dataNode->getBrPath())
    FATAL("ERROR!! address node and data node not in the same path");
  nodePath path = addNode->getBrPath();
  int modTimeAdd = time % II;
  int modTimeData = (time + 1) % II;
  if (!memLdResAvailable(path, time))
    return;
  // allocate resources
  addBusUsed[std::make_tuple(path, time)]++;
  dataBusUsed[std::make_tuple(path, time + 1)]++;
  peUsed[std::make_tuple(path, time)]++;
  peUsed[std::make_tuple(path, time + 1)]++;
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
moduloSchedule::memStResAvailable(nodePath path, int scheduleTime)
{
  int modTime = scheduleTime % II;

  // add bus used
  int addU = 0;
  // data bus used
  int dataU = 0;
  // pe used
  int peU = 0;

  // for other iterations
  for (int t = 0; t < length; t++) {
    if ((t % II) == modTime && t != scheduleTime) {
      // time slots that will be mapped together
      int truePe = peUsed[std::make_tuple(true_path, t)];
      int falsePe = peUsed[std::make_tuple(false_path, t)];
      peU += peUsed[std::make_tuple(none, t)] + ((truePe > falsePe) ? truePe : falsePe);
      int trueAdd = addBusUsed[std::make_tuple(true_path, t)];
      int falseAdd = addBusUsed[std::make_tuple(false_path, t)];
      addU += addBusUsed[std::make_tuple(none, t)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
      int trueData = dataBusUsed[std::make_tuple(true_path, t)];
      int falseData = dataBusUsed[std::make_tuple(false_path, t)];
      dataU += dataBusUsed[std::make_tuple(none, t)] + ((trueData > falseData) ? trueData : falseData);
    }
  }
  // for this iteration, which would be of scheduleTime
  if (path == none) {
    int truePe = peUsed[std::make_tuple(true_path, scheduleTime)];
    int falsePe = peUsed[std::make_tuple(false_path, scheduleTime)];
    peU += peUsed[std::make_tuple(none, scheduleTime)] + ((truePe > falsePe) ? truePe : falsePe);
    int trueAdd = addBusUsed[std::make_tuple(true_path, scheduleTime)];
    int falseAdd = addBusUsed[std::make_tuple(false_path, scheduleTime)];
    addU += addBusUsed[std::make_tuple(none, scheduleTime)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
    int trueData = dataBusUsed[std::make_tuple(true_path, scheduleTime)];
    int falseData = dataBusUsed[std::make_tuple(false_path, scheduleTime)];
    dataU += dataBusUsed[std::make_tuple(none, scheduleTime)] + ((trueData > falseData) ? trueData : falseData);
  } else {
    peU += peUsed[std::make_tuple(path, scheduleTime)] + peUsed[std::make_tuple(none, scheduleTime)];
    addU += addBusUsed[std::make_tuple(path, scheduleTime)] + addBusUsed[std::make_tuple(none, scheduleTime)];
    dataU += dataBusUsed[std::make_tuple(path, scheduleTime)] + dataBusUsed[std::make_tuple(none, scheduleTime)];
  }

  if (!(addU < perRowMem * xDim))
    return false;
  if (!(dataU < perRowMem * xDim))
    return false;
  // 2 pe at same time for a store
  if (!(peU < cgraSize - 1))
    return false;
  return true;
}


void 
moduloSchedule::scheduleSt(Node* storeNode, Node* storeRelatedNode, int time)
{
  // the 2 nodes should belong to the same path
  if (storeNode->getBrPath() != storeRelatedNode->getBrPath())
    FATAL("ERROR!! 2 store nodes not in the same path");
  nodePath path = storeNode->getBrPath();
  int modTime = time % II;
  if (!memStResAvailable(path, time))
    return;
  //allocate resources
  addBusUsed[std::make_tuple(path, time)]++;
  dataBusUsed[std::make_tuple(path, time)]++;
  peUsed[std::make_tuple(path, time)] += 2;
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
moduloSchedule::print(DFG* myDFG, std::string name)
{
  std::ofstream dotFile;
  std::string fileName = "Modulo_Schedule_" + name + ".dot";
  dotFile.open(fileName);

  dotFile << "digraph " << "Modulo_Schedule" << " { \n{\n";
  // print node
  for (auto nodeIt : nodeSchedule) {
    int nodeId = nodeIt.first;
    dotFile << nodeId;
    dotFile << " [label=\"" << nodeId << "(" << (int)floor(nodeSchedule[nodeId]/II) << ")\"]";

    if (myDFG->getNode(nodeId)->isMemNode())
      dotFile << " [color=blue]";
    else if (myDFG->getNode(nodeId)->getIns() == route)
      dotFile << " [color=green]";
    else
      dotFile << " [color=red]";
    
    if (myDFG->getNode(nodeId)->getBrPath() == true_path)
      dotFile << " [style=filled, fillcolor=lightblue];\n";
    else if (myDFG->getNode(nodeId)->getBrPath() == false_path)
      dotFile << " [style=filled, fillcolor=lightcoral];\n";
    else
      dotFile << ";\n";
  }
  dotFile << "\n";

  // print arc
  for (int arcId : myDFG->getArcIdSet()) {
    Arc* arc = myDFG->getArc(arcId);
    dotFile << arc->getFromNode()->getId() << " -> " << arc->getToNode()->getId();
    if (arc->getDependency() == PredDep || arc->getDependency() == TrueDep) {
      if (arc->getDistance() != 0) {
        dotFile << " [style=bold, label=" << arc->getDistance() << "]";
      }
    } else if (arc->getDependency() == LoadDep) {
      dotFile << " [style=dotted, label= mem]";
    } else {
      FATAL("[Print Modulo]Encountered arc dependency not implemented " << arc->getDependency());
    }

    if (arc->getPath() == true_path) {
      dotFile << " [color=blue]\n";
    } else if (arc->getPath() == false_path) {
      dotFile << " [color=red]\n";
    } else {
      dotFile << "[color=black]\n";
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


int
moduloSchedule::getIter(Node* node)
{
  return (int)floor(nodeSchedule[node->getId()] / II);
}