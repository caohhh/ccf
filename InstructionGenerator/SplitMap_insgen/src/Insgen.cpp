/**
 * 
 * insgen.cpp
 * 
 * Used for instruction generation for a split map.
 * 
 * Cao Heng
 * 2024.3.16
 * 
*/

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <map>
#include <algorithm>

#include "Definitions.h"
#include "Node.h"
#include "CGRA.h"

int main(int argc, char *argv[])
{
  // argvs are in the order of:
  // X, Y, llvm node file, node scheme, cgra scheme, live out node, live out edge, control node, live in edge, live in node, obj
  // 1/ 2/              3/           4/           5/             6/             7/            8/            9/           10/  11
  DEBUG("[Insgen]Begin instruction generation");
  
  std::vector<Node*> nodeSet;
  // read in node scheme
  std::ifstream nodeSch(argv[4]);
  if (nodeSch.is_open()) {
    // in the format of:
    // nodeId insOp operand0 operand1 operand2
    for (std::string line; getline(nodeSch, line);) {
      std::istringstream lineIn(line);
      int nodeId;
      int insOp;
      lineIn >> nodeId >> insOp;
      Node* node = new Node(nodeId, (Instruction_Operation)insOp);
      for (int i = 0; i < 3; i++) {
        std::string op;
        lineIn >> op;
        std::pair<int, int> opPair;
        if (op.find("T") != std::string::npos) {
          // with path split, shoul be in the format of T00F00
          int opT = std::stoi(op.substr(1, op.find("F") - 1));
          int opF = std::stoi(op.substr(op.find("F") + 1));
          opPair = std::make_pair(opT, opF);
        } else {
          // no path split
          int opInt = std::stoi(op);
          opPair = std::make_pair(opInt, -1);
        }
        node->setOp(i, opPair);
      } // end of iterating through operands
      nodeSet.push_back(node);
    } // end of iterating through lines
  } else {
    FATAL("[Insgen]ERROR! Can't open node scheme");
  }
  nodeSch.close();
  DEBUG("[Insgen]Done getting node info");

  // update livein node info here
  std::vector<int> liveInNodes;
  std::ifstream liveInEdge(argv[9]);
  if (liveInEdge.is_open()) {
    for (std::string line; getline(liveInEdge, line);) {
      std::istringstream lineIn(line);
      int fromNodeId;
      int toNodeId;
      lineIn >> fromNodeId >> toNodeId;
      for (auto node : nodeSet) {
        if (node->getId() == toNodeId) {
          // need to add an extra source
          std::string tmp;
          int opOrder;
          lineIn >> tmp >> tmp >> opOrder;
          node->setOp(opOrder, std::make_pair(fromNodeId, -1));
          liveInNodes.push_back(fromNodeId);
        }
      }
    }
  } else 
    FATAL("[Insgen]ERROR! Can't open live in");
  DEBUG("[Insgen]Update live in complete");

  // map of constants in <nodeId, constant>
  std::map<int, int> constants;
  // map of <live in id: alignment>
  std::map<int, int> liveinAlign;
  // next we parse llvm node for constant and condBr
  std::ifstream llvmNode(argv[3]);
  if (llvmNode.is_open()) {
    // data needed is in the format of:
    // nodeId insOp nodeName
    for (std::string line; getline(llvmNode, line);) {
      std::istringstream lineIn(line);
      int nodeId;
      int insOp;
      std::string nodeName;
      lineIn >> nodeId >> insOp >> nodeName;
      // we first want constants from llvm node
      if ((Instruction_Operation)insOp == constant) {
        if (nodeName.find("ConstInt") != std::string::npos) {
          // this node is a constant int
          constants[nodeId] = std::stoi(nodeName.substr(8));
        }
      }
      int alignment;
      lineIn >> alignment;
      if ((Instruction_Operation)insOp == ld_add || (Instruction_Operation)insOp == st_add) {
        // set alignment for address generators
        Node* addNode = Node::getNode(nodeSet, nodeId);
        addNode->setAlignment(alignment);
      }
      if (std::find(liveInNodes.begin(), liveInNodes.end(), nodeId) != liveInNodes.end()) {
        // also populate the live in alignment map if it is an livein
        liveinAlign[nodeId] = alignment;
      }
      int tmp;
      int condBrId;
      lineIn >> tmp >> tmp >> condBrId;
      if (condBrId != -1) {
        Node* condNode = Node::getNode(nodeSet, nodeId);
        condNode->setCondBr(condBrId);
      }
    } // end of iterating through lines
  } else
    FATAL("[Insgen]ERROR! Can't open llvmnode");
  llvmNode.close();
  DEBUG("[Insgen]Done getting constants info");

  // safety check for the source count
  for (auto node : nodeSet)
    node->checkSource();
  DEBUG("[Insgen]Check source complete");

  // now the CGRA scheme
  int xSize = atoi(argv[1]);
  int ySize = atoi(argv[2]);
  CGRA* cgra = nullptr;
  std::ifstream cgraSch(argv[5]);
  if (cgraSch.is_open()) {
    // the first line is the II
    std::string line;
    int II;
    if (getline(cgraSch, line))
      II = std::stoi(line);
    else 
      FATAL("[Insgen]ERROR! Can't get II from CGRA scheme");
    cgra = new CGRA(xSize, ySize, II);
    // others are in the format of:
    // nodeId iteration
    // in the order of:
    // t->x->y->brPath
    for (int t = 0; t < II; t++) {
      for (int x = 0; x < xSize; x++) {
        for (int y = 0; y < ySize; y++) {
          // map of path : <nodeid, iteration>
          std::map<int, std::pair<int, int>> pathNodes;
          for (int path : {0, 1, 2}) {
            if (getline(cgraSch, line)) {
              std::istringstream lineIn(line);
              int nodeId;
              int iter;
              lineIn >> nodeId >> iter;
              pathNodes[path] = std::make_pair(nodeId, iter);
            } else
              FATAL("[Insgen]CGRA Scheme error");
          } // end of iterating through paths
          cgra->setNode(t, x, y, pathNodes);
        } // end of iterating through y
      } // end of iterating through x
    } // end of iterating through t
  } else
    FATAL("[Insgen]ERROR! Can't open CGRA scheme");
  cgraSch.close();
  DEBUG("[Insgen]Done getting CGRA info");

  // parse the live outs
  std::map<int, std::string> liveOutName;
  std::map<int, int> liveOutAlign;
  std::ifstream liveOutFile(argv[6]);
  if (liveOutFile.is_open()) {
    std::string line;
    while(getline(liveOutFile, line)) {
      std::istringstream lineIn(line);
      int nodeId;
      lineIn >> nodeId;
      Node* liveOutNode = Node::getNode(nodeSet, nodeId);
      if (liveOutNode == nullptr)
        FATAL("[Insgen]ERROR! Can't get the live out node");
      // we say that a node can only be associated with one live out
      getline(liveOutFile, line);
      lineIn = std::istringstream(line);
      int liveOutId;
      std::string name;
      int alignment;
      std::string tmp;
      lineIn >> liveOutId >> tmp >> name >> tmp >> alignment;
      liveOutNode->setLiveOut(liveOutId);
      liveOutName[liveOutId] = name;
      liveOutAlign[liveOutId] = alignment;
    }
  } else
    FATAL("[Insgen]ERROR! Can't open live out file");


  // next we update the data sources of each node
  cgra->updateDataSource(nodeSet, constants, liveInNodes);
  DEBUG("[Insgen]Done updating data sources");

  // parse control nodes
  // loop exit control node
  int splitBr = -1;
  std::ifstream ctrlNode(argv[8]);
  if (ctrlNode.is_open()) {
    std::string line;

    int leNodeId = -1;
    if (getline(ctrlNode, line))
      leNodeId = std::stoi(line);
    else
      FATAL("[Insgen]ERROR! Can't get le node");
    Node* leNode = Node::getNode(nodeSet, leNodeId);
    leNode->setLoopExit(true);

    bool exitCond;
    if (getline(ctrlNode, line))
      exitCond = std::stoi(line);
    else
      FATAL("[Insgen]ERROR! Can't get exit cond");
    leNode->setExitCond(exitCond);

    if (getline(ctrlNode, line))
      splitBr = std::stoi(line);
    else
      FATAL("[Insgen]ERROR! Can't get split branch");
    if (splitBr != -1) {
      // only set the split cond node if the DFG is split
      for (Node* node : nodeSet) {
        if (node->getCondBr() == splitBr)
          node->setSplitCond(true);
      }
    }
  } else 
    FATAL("[Insgen]ERROR! Can't open Control Node");


  // now generate the instructions for kernel
  cgra->generateIns();
  DEBUG("[Insgen]Done generating intructions in kernel");

  // for prologue, the phi instructions should be different
  cgra->generatePrologue();
  DEBUG("[Insgen]Done generating prologue phi instruction");

  // now we generate load ins for live in values
  // first get all the address needed
  // map of <live in node ID: name>
  std::map<int, std::string> liveInName;
  std::ifstream liveInNodeFile(argv[10]);
  if (liveInNodeFile.is_open()) {
    for (std::string line; getline(liveInNodeFile, line);) {
      std::istringstream lineIn(line);
      int nodeId;
      int op;
      std::string name;
      lineIn >> nodeId >> op >> name;
      if ((Instruction_Operation)op == constant) {
        // this will be the live in node
        liveInName[nodeId] = name;
      }
    }
  } else
    FATAL("[Insgen]ERROR! Can't open live in node file");
  // now get the address
  std::map<int, uint32_t> liveinAdd;
  std::string obj = argv[11];
  for (auto liveinIt : liveInName) {
    // get the address for each live in
    int nodeId = liveinIt.first;
    std::string nodeName = liveinIt.second;
    // command to get the address
    std::string command;
    command = "arm-linux-gnueabi-readelf -s ";
    command += obj + " | grep -w \"";
    command += nodeName + "\" | tr -s ' '|cut -d' ' -f3";
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
      FATAL("[Insgen]ERROR! Can't start command to read in address");
    while (fgets(buffer.data(), 128, pipe) != NULL)
      result += buffer.data();
    pclose(pipe);
    uint32_t address = stoul(result, nullptr, 16);
    liveinAdd[nodeId] = address;
    DEBUG("[Insgen]Live in node: " << nodeId << ", name: " << nodeName << ", add: " << std::hex << address);
  }
  DEBUG("[Insgen]Done getting live in address");

  // now generate the load instructions
  cgra->generateLiveinLoad(liveinAdd, liveinAlign);
  DEBUG("[Insgen]Done generating live in load instructions");

  // generate the store for liveout

  // load in -> prologue -> kernel -> epilogue -> store out


  return 0;
}