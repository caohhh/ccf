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
moduloSchedule::resAvailable(nodePath path, int scheduleTime)
{
  int modTime = scheduleTime % II;

  // pe used
  int peU;

  if (path == none) {
    int truePe = peUsed[std::make_tuple(true_path, modTime)];
    int falsePe = peUsed[std::make_tuple(false_path, modTime)];
    peU = peUsed[std::make_tuple(none, modTime)] + ((truePe > falsePe) ? truePe : falsePe);
  } else {
    peU = peUsed[std::make_tuple(path, modTime)] + peUsed[std::make_tuple(none, modTime)];
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
  peUsed[std::make_tuple(path, modTime)]++;
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
  int addU;
  // data bus used
  int dataU;
  // pe used at address time slot
  int peAddU;
  // pe used at data time slot
  int peDataU;

  if (path == none) {
    int trueAdd = addBusUsed[std::make_tuple(true_path, modTimeAdd)];
    int falseAdd = addBusUsed[std::make_tuple(false_path, modTimeAdd)];
    int trueData = dataBusUsed[std::make_tuple(true_path, modTimeData)];
    int falseData = dataBusUsed[std::make_tuple(false_path, modTimeData)];
    int truePeAdd = peUsed[std::make_tuple(true_path, modTimeAdd)];
    int falsePeAdd = peUsed[std::make_tuple(false_path, modTimeAdd)];
    int truePeData = peUsed[std::make_tuple(true_path, modTimeData)];
    int falsePeData = peUsed[std::make_tuple(false_path, modTimeData)];
    addU = addBusUsed[std::make_tuple(none, modTimeAdd)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
    dataU = dataBusUsed[std::make_tuple(none, modTimeData)] + ((trueData > falseData) ? trueData : falseData);
    peAddU = peUsed[std::make_tuple(none, modTimeAdd)] + ((truePeAdd > falsePeAdd) ? truePeAdd : falsePeAdd);
    peDataU = peUsed[std::make_tuple(none, modTimeData)] + ((truePeData > falsePeData) ? truePeData : falsePeData);
  } else {
    addU = addBusUsed[std::make_tuple(path, modTimeAdd)] + addBusUsed[std::make_tuple(none, modTimeAdd)];
    dataU = dataBusUsed[std::make_tuple(path, modTimeData)] + dataBusUsed[std::make_tuple(none, modTimeData)];
    peAddU = peUsed[std::make_tuple(path, modTimeAdd)] + peUsed[std::make_tuple(none, modTimeAdd)];
    peDataU = peUsed[std::make_tuple(path, modTimeData)] + peUsed[std::make_tuple(none, modTimeData)];
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
  addBusUsed[std::make_tuple(path, modTimeAdd)]++;
  dataBusUsed[std::make_tuple(path, modTimeData)]++;
  peUsed[std::make_tuple(path, modTimeAdd)]++;
  peUsed[std::make_tuple(path, modTimeData)]++;
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
  int addU;
  // data bus used
  int dataU;
  // pe used
  int peU;

  if (path == none) {
    int trueAdd = addBusUsed[std::make_tuple(true_path, modTime)];
    int falseAdd = addBusUsed[std::make_tuple(false_path, modTime)];
    int trueData = dataBusUsed[std::make_tuple(true_path, modTime)];
    int falseData = dataBusUsed[std::make_tuple(false_path, modTime)];
    int truePe = peUsed[std::make_tuple(true_path, modTime)];
    int falsePe = peUsed[std::make_tuple(false_path, modTime)];
    addU = addBusUsed[std::make_tuple(none, modTime)] + ((trueAdd > falseAdd) ? trueAdd : falseAdd);
    dataU = dataBusUsed[std::make_tuple(none, modTime)] + ((trueData > falseData) ? trueData : falseData);
    peU = peUsed[std::make_tuple(none, modTime)] + ((truePe > falsePe) ? truePe : falsePe);
  } else {
    addU = addBusUsed[std::make_tuple(path, modTime)] + addBusUsed[std::make_tuple(none, modTime)];
    dataU = dataBusUsed[std::make_tuple(path, modTime)] + dataBusUsed[std::make_tuple(none, modTime)];
    peU = peUsed[std::make_tuple(path, modTime)] + peUsed[std::make_tuple(none, modTime)];
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
  addBusUsed[std::make_tuple(path, modTime)]++;
  dataBusUsed[std::make_tuple(path, modTime)]++;
  peUsed[std::make_tuple(path, modTime)] += 2;
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