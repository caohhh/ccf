/**
 * Parser.cpp
*/

#include <fstream>
#include <sstream>
#include <iostream>
#include "Parser.h"
#include "definitions.h"

Parser::Parser(std::string nodeFile, std::string edgeFile)
{
  this->nodeFile = nodeFile;
  this->edgeFile = edgeFile;
}


Parser::~Parser()
{
}


void
Parser::ParseDFG(DFG* myDFG)
{
  std::ifstream fInNode;
  std::ifstream fInEdge;

  // parse node file
  fInNode.open(nodeFile.c_str());
  if (fInNode.is_open()) {
    for (std::string line; getline(fInNode, line);) {
      std::istringstream lineIn(line);
      int nodeId;
      int insOp;
      std::string nodeName;
      int alignment;
      int dataType;
      int brPath;
      int condBrId;
      lineIn >> nodeId >> insOp >> nodeName >> alignment >> dataType >> brPath >> condBrId;
      if (!myDFG->hasNode(nodeId)) {
        Node* newNode = new Node((Instruction_Operation)insOp, (Datatype)dataType, 1, nodeId, nodeName, (nodePath)brPath, condBrId);
        myDFG->insertNode(newNode);
      }
    }
  } else {
    FATAL("Can't open node file");
  }
  fInNode.close();

  // parse edge
  fInEdge.open(edgeFile.c_str());
  if (fInEdge.is_open()) {
    for (std::string line; getline(fInEdge, line);) {
      std::istringstream lineIn(line);
      int fromNodeId;
      int toNodeId;
      int distance;
      std::string edgeType;
      int operandOrder;
      lineIn >> fromNodeId >> toNodeId >> distance >> edgeType >> operandOrder;
      if (myDFG->hasConstant(fromNodeId)|| myDFG->hasConstant(toNodeId)) {
        // at least one of the nodes is a constant
        if (edgeType.compare("TRU") == 0) {
          myDFG->makeConstArc(fromNodeId, toNodeId, operandOrder);
        }
      } else {
        // no constants
        Node* nodeFrom;
        Node* nodeTo;
        if (!myDFG->hasNode(fromNodeId) || !myDFG->hasNode(toNodeId)) {
          FATAL("Arc file has node not in node file!!!!!");
        } else {
          // both nodes exists
          nodeFrom = myDFG->getNode(fromNodeId);
          nodeTo = myDFG->getNode(toNodeId);
        }
        if (edgeType.compare("LRE") == 0) {
          // load related dependency
          nodeFrom->setLoadAddressGenerator(nodeTo);
          nodeTo->setLoadDataBusRead(nodeFrom);
          myDFG->makeArc(nodeFrom, nodeTo, 0, LoadDep, 0);
        } else if (edgeType.compare("SRE") == 0) {
          // store related dependency
          nodeFrom->setStoreAddressGenerator(nodeTo);
          nodeTo->setStoreDataBusWrite(nodeFrom);
          myDFG->makeArc(nodeFrom, nodeTo, 0, StoreDep, 0);
        } else if (edgeType.compare("PRE")) {
          // predication dependency
          myDFG->makeArc(nodeFrom, nodeTo, distance, PredDep, operandOrder);
        } else if (edgeType.compare("MEM") == 0) {
          // memory dependency
          myDFG->makeArc(nodeFrom, nodeTo, distance, MemoryDep, operandOrder);
        } else if (edgeType.compare("LCE") == 0) {
          // loop control edge
          nodeFrom->setLoopCtrl();
          nodeTo->setLiveOut();
        } else {
          //allow live-in as true-data dependency to perform register allocation
          myDFG->makeArc(nodeFrom, nodeTo, distance, TrueDep, operandOrder);
        }
      }
    } // end of parsing lines
  } else {
    FATAL("Can't open edge file");
  }
  fInEdge.close();
}