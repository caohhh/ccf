/**
 * CGRA.cpp
 * Implementation for CGRA.h
 * 
 * Cao Heng
 * 2024.3.20
*/

#include "CGRA.h"
#include "Instruction.h"

#include <algorithm>

namespace
{

// convert input direction to input mux
Instruction::PEInputMux
dirToMux(direction dir)
{
  if (dir == same)
    return Instruction::Self;
  else if (dir == up)
    return Instruction::Up;
  else if (dir == down)
    return Instruction::Down;
  else if (dir == left)
    return Instruction::Left;
  else if (dir == right)
    return Instruction::Right;
  else if (dir == constOp)
    return Instruction::Immediate;
  else if (dir == liveInData)
    return Instruction::Register;
  else
    FATAL("[D2M]ERROR! Should not convert this direction");
}

}; // end of anonymous namespace

CGRA::CGRA(int xSize, int ySize, int II)
{
  this->II = II;
  this->xSize = xSize;
  this->ySize = ySize;
  for (int t = 0; t < II; t++) {
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        peSet.push_back(new PE(t, x, y));
      }
    }
  }
}


void
CGRA::setNode(int t, int x, int y, std::map<int, std::pair<int, int>> nodes)
{
  if (t < II && x < xSize && y < ySize) {
    PE* pe = peSet[t * (xSize * ySize) + x * ySize + y];
    pe->setNodeId(nodes);
  } else
    FATAL("[CGRA]Node out of bounds");
}


void
CGRA::updateDataSource(std::vector<Node*> nodeSet, std::map<int, int> constants, std::vector<int> liveIns)
{
  // map of physical PE address to live data nodes, as in <<x, y> : ID>
  std::map<std::pair<int, int>, std::set<int>> liveInNode;
  std::map<std::pair<int, int>, std::set<int>> liveOutNode;
  for (PE* pe : peSet) {
    DEBUG("[Data Souerce]For pe: <" << std::get<0>(pe->getCoord()) << ", " << 
            std::get<1>(pe->getCoord()) << ", " << std::get<2>(pe->getCoord()) << ">");
    if (pe->hasNode()) {
      // if there are nodes mapped to this PE
      auto nodeIds = pe->getNode();
      for (nodePath path : {none, true_path, false_path}) {
        if (nodeIds[path] != -1) {
          // there is a node in this path
          // first get the node
          Node* node = Node::getNode(nodeSet, nodeIds[path]);
          if (node == nullptr)
            FATAL("[Data Source]Can't get node");
          // set the node
          pe->setNode(path, node);
          // next set the data source
          auto dataSource = node->getDataSource();
          // exceptions need to be made for load/store data node
          if (node->getOp() != ld_data && node->getOp() != st_data) {
            // we search the previous t for each data source
            for (int i = 0; i < 3; i++) {
              if (dataSource[i].first != -1) {
                // data source i exists
                if (std::find(liveIns.begin(), liveIns.end(), dataSource[i].first) != liveIns.end()) {
                  // it is an livein
                  pe->setSource(path, i, liveInData);
                  DEBUG("[Data Source]Source " << i << ": live in");
                  // here we also need to assign a register for this livein
                  // first get the physical coord of the PE
                  int x;
                  int y;
                  int t;
                  std::tie(t, x, y) = pe->getCoord();
                  // update the live in
                  liveInNode[std::make_pair(x, y)].insert(dataSource[i].first);
                } else if (constants.find(dataSource[i].first) != constants.end()) {
                  // is a constant
                  int sourceConst = constants[dataSource[i].first];
                  // set the source
                  pe->setSource(path, i, sourceConst);
                  DEBUG("[Data Source]Source " << i << ": constant " << sourceConst);
                } else {
                  //find its PE
                  PE* sourcePE = getPE(dataSource[i].first);
                  if (sourcePE == nullptr)
                    FATAL("[Data Source]Can't get source PE");
                  if (dataSource[i].second != -1) {
                    // also check if the second source is at the same PE
                    if (getPE(dataSource[i].second) != sourcePE)
                      FATAL("[Data Source]Source PE not matching");
                  }
                  // set the source
                  pe->setSource(path, i, getDirection(sourcePE, pe));
                  DEBUG("[Data Source]Source " << i << ": PE " << getDirection(sourcePE, pe));
                }
              }
            } // end of iterating through operands
          } else {
            if (node->getOp() == st_data) {
              // only get op1 for store data
              if (dataSource[1].first != -1) {
                // data source i exists
                if (std::find(liveIns.begin(), liveIns.end(), dataSource[1].first) != liveIns.end()) {
                  // it is an livein
                  pe->setSource(path, 1, liveInData);
                  DEBUG("[Data Source]Source " << 1 << ": live in");
                } else if (constants.find(dataSource[1].first) != constants.end()) {
                  // is a constant
                  int sourceConst = constants[dataSource[1].first];
                  // set the source
                  pe->setSource(path, 1, sourceConst);
                  DEBUG("[Data Source]Source " << 1 << ": constant " << sourceConst);
                } else {
                  //find its PE
                  PE* sourcePE = getPE(dataSource[1].first);
                  if (sourcePE == nullptr)
                    FATAL("[Data Source]Can't get source PE");
                  if (dataSource[1].second != -1) {
                    // also check if the second source is at the same PE
                    if (getPE(dataSource[1].second) != sourcePE)
                      FATAL("[Data Source]Source PE not matching");
                  }
                  // set the source
                  pe->setSource(path, 1, getDirection(sourcePE, pe));
                  DEBUG("[Data Source]Source " << 1 << ": PE " << getDirection(sourcePE, pe));
                }
              } else 
                FATAL("[Data Source]ERROR! store data with no data source");
            }
          }
          if (node->isLiveOut()) {
            // need to also note down the live out
            int x;
            int y;
            int t;
            std::tie(t, x, y) = pe->getCoord();
            // update the live out
            liveOutNode[std::make_pair(x, y)].insert(node->getLiveOut());
          }
        }
      } // end of iterating through paths
    } // end of if pe has node
  } // end of iterating through peSet
  // now all is done, we also pass the live in info to each PE
  for (auto liveInIt : liveInNode) {
    // first get the phys coord
    int x;
    int y;
    std::tie(x, y) = liveInIt.first;
    // next a map of <livein ID: reg ID>
    std::map<int, int> liveinReg;
    int i = 0;
    for (auto livein : liveInIt.second)
      liveinReg[livein] = i++;
    #ifndef NDEBUG
      DEBUG("[Data Source]Live in reg for PE: <" << x << ", " << y << ">");
      for (auto it : liveinReg)
        DEBUG("[Data Source]Live in: " << it.first << ", reg: " << it.second);
    #endif
    // should also add a check here to ensure not exceeding reg count
    // add this livein info to all PE of this phys coord
    for (int t = 0; t < II; t++) {
      PE* pe = peSet[t * (xSize * ySize) + x * ySize + y];
      pe->setLiveIn(liveinReg);
    }
  } // end of iterating through live in
  // now for live out
  for (auto liveOutIt : liveOutNode) {
    // first get the phys coord
    int x;
    int y;
    std::tie(x, y) = liveOutIt.first;
    // next a map of <livein ID: reg ID>
    std::map<int, int> liveOutReg;
    // start populating live out after live in
    int i = peSet[x * ySize + y]->getLivein().size();
    for (auto liveOut : liveOutIt.second)
      liveOutReg[liveOut] = i++;
    #ifndef NDEBUG
      DEBUG("[Data Source]Live out reg for PE: <" << x << ", " << y << ">");
      for (auto it : liveOutReg)
        DEBUG("[Data Soure]Live out: " << it.first << ", reg: " << it.second);
    #endif
    // should also add a check here to ensure not exceeding reg count
    // add this livein info to all PE of this phys coord
    for (int t = 0; t < II; t++) {
      PE* pe = peSet[t * (xSize * ySize) + x * ySize + y];
      pe->setLiveOut(liveOutReg);
    }
  } // end of iterating through live out
}


PE*
CGRA::getPE(int nodeId)
{
  for (auto pe : peSet) {
    auto nodes = pe->getNode();
    for (auto path : {none, true_path, false_path}) {
      if (nodes[path] == nodeId)
        return pe;
    }
  }
  return nullptr;
}


direction
CGRA::getDirection(PE* sourcePE, PE* pe)
{
  int sourceT;
  int sourceX;
  int sourceY;
  std::tie(sourceT, sourceX, sourceY) = sourcePE->getCoord();
  int t;
  int x;
  int y;
  std::tie(t, x, y) = pe->getCoord();
  if (t != (sourceT + 1) % II)
    FATAL("[Direction]ERROR! PE not in adjacent time slot of source PE");
  if (sourceX == x) {
    if (sourceY == y)
      return same;
    else if (sourceY == (y + 1) % ySize)
      return up;
    else if (y == (sourceY + 1) % ySize)
      return down;
    else
      FATAL("[Direction]2 PE not connected with same x");
  } else if (sourceY == y) {
    // sourceX != x
    if (sourceX == (x + 1) % xSize)
      return right;
    else if (x == (sourceX + 1) % xSize)
      return left;
    else
      FATAL("[Direction]2 PE not connected with same y");

  } else
    FATAL("[Direction]2 PE not connected");
}


void
CGRA::generateIns()
{
  for (auto pe : peSet)
    pe->generateIns();
}


void
CGRA::generatePrologue()
{
  for (auto pe : peSet)
    pe->generatePrologue();
}


void
CGRA::generateLiveinLoad(std::map<int, uint32_t> liveinAdd, std::map<int, int> liveinAlign)
{
  // since all pe at the same physical coord will have the same reg
  // all liveins, <x: livein in row x>
  std::vector<std::pair<int, std::vector<std::tuple<int, int, int>>>> liveinAll;
  // we just check pe at t = 0
  for (int x = 0; x < xSize; x++) {
    // since only one mem op is done in a row
    // <livein ID, y, R>
    std::vector<std::tuple<int, int, int>> liveinRow;
    for (int y = 0; y < ySize; y++) {
      PE* pe = peSet[x * ySize + y];
      // get the live in nodes
      auto liveIn = pe->getLivein();
      // here we can reduce the load cycles by interweaving loads between PEs
      // but I'm too lazy to do that now
      for (auto liIt : liveIn) {
        int liveInId = liIt.first;
        int regNo = liIt.second;
        liveinRow.push_back(std::make_tuple(liveInId, y, regNo));
      }
    } // end of iterating through y
    if (!liveinRow.empty())
      liveinAll.push_back(std::make_pair(x, liveinRow));
  } // end of iterating through x
  // now that we get all the liveins, construct a time: load graph for each row
  // the time for each load is 2, so the total time will be 2 * max row time
  if (liveinAll.empty())
    return;
  // the max length of a row load
  unsigned maxT = 0;
  for (auto liveinRow : liveinAll) {
    if (liveinRow.second.size() > maxT)
      maxT = liveinRow.second.size();
  }
  DEBUG("[Live In]Live in length " << maxT * 2);
  // populate the load in instructions
  for (unsigned t = 0; t < maxT * 2; t++) {
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        loadInIns.push_back(Instruction::noopIns);
      }
    }
  }
  // now generate each load ins: addgen and loadin
  for (auto liveinRow : liveinAll) {
    int x = liveinRow.first;
    auto loadInfo = liveinRow.second;
    int t = 0;
    for (auto load : loadInfo) {
      int liveinId = std::get<0>(load);
      int y = std::get<1>(load);
      int regNo = std::get<2>(load);
      // get the address for the liveinId
      if (liveinAdd.find(liveinId) == liveinAdd.end())
        FATAL("[Live In]ERROR! Can't get live in address, id: " << liveinId);
      uint32_t address = liveinAdd[liveinId];
      // also get the alignment
      if (liveinAlign.find(liveinId) == liveinAlign.end())
        FATAL("[Live In]ERROR! Can't get live in alignment");
      uint32_t alignment = liveinAlign[liveinId];
      // first the address ins, 2 PEs are used, the PE to the right of load PE will provide the address
      // we do this by a OR of address with itself
      auto addProvide = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, false, Instruction::Immediate, 
                                    Instruction::Immediate, 0, 0, 0, false, false, false, address);
      loadInIns[t * (xSize * ySize) + x * ySize + (y + 1) % ySize] = addProvide;
      DEBUG("[Live In]PE <" << t << ", " << x << ", " << (y + 1) % ySize << ">: address provide ins: " << std::hex << addProvide);
      auto addGen = Instruction::encodePIns(Instruction::int32, Instruction::address_generator, 
                                  Instruction::Right, Instruction::Immediate, Instruction::Self, 0, 0, 0, alignment);
      loadInIns[t * (xSize * ySize) + x * ySize + y] = addProvide;
      DEBUG("[Live In]PE <" << t << ", " << x << ", " << y << ">: address generate ins: " << std::hex << addGen);
      // now the load in ins
      auto dataIns = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, false, 
                                  Instruction::DataBus, Instruction::Immediate, 0, 0, regNo, true, false, false, 0); 
      loadInIns[(t + 1) * (xSize * ySize) + x * ySize + y] = addProvide;
      DEBUG("[Live In]PE <" << t + 1 << ", " << x << ", " << y << ">: data ins: " << std::hex << dataIns);
      t += 2;
    } // end of iterating through loads in a row
  } // end of iterating through all loads
}


/*****************************PE***********************************************/

PE::PE(int t, int x, int y)
{
  // first the coords
  this->t = t;
  this->x = x;
  this->y = y;
  for (auto path : {none, true_path, false_path}) {
    nodeId[path] = -1;
    nodes[path] =  nullptr;
    for (int i = 0; i < 3; i++) 
      sourceDirections[path][i] = noop;
    insWord[path] = Instruction::noopIns;
  }
  iter = -1;
  phiOp = false;
  proIns = Instruction::noopIns;
}


void
PE::setNodeId(std::map<int, std::pair<int, int>> nodes)
{
  // set nodes
  if (nodes[0].first != -1) {
    // node on none path
    if (nodes[1].first != -1 || nodes[2].first != -1)
      FATAL("[Node]PE with node on none path and other paths");
    nodeId[none] = nodes[0].first;
    iter = nodes[0].second;
    nodeId[true_path] = -1;
    nodeId[false_path] = -1;
  } else {
    if (nodes[1].first != -1 && nodes[2].first != -1) {
      // merged node
      if (nodes[1].second != nodes[2].second)
        FATAL("[Node]Error! Iteration not matching between paths");
      nodeId[none] = -1;
      nodeId[true_path] = nodes[1].first;
      nodeId[false_path] = nodes[2].first;
      iter = nodes[1].second;
    } else if (nodes[1].first == -1 && nodes[2].first == -1) {
      // no node on this PE
      for (auto path : {none, true_path, false_path})
        nodeId[path] = -1;
      iter = -1;
    } else {
      for (auto path : {none, true_path, false_path})
        nodeId[path] = nodes[(int)path].first;
      if (nodes[1].first != -1)
        iter = nodes[1].second;
      else
        iter = nodes[2].second;
    }
  }
}


void
PE::setNode(nodePath path, Node* node)
{
  nodes[path] = node;
}


bool
PE::hasNode()
{
  if (iter == -1)
    return false;
  else
    return true;
}


std::map<nodePath, int>
PE::getNode()
{
  return nodeId;
}


std::tuple<int, int, int>
PE::getCoord()
{
  return std::make_tuple(t, x, y);
}

void
PE::setSource(nodePath path, int opOrder, direction dir)
{
  sourceDirections[path][opOrder] = dir;
}


void
PE::setSource(nodePath path, int opOrder, int constSource)
{
  sourceDirections[path][opOrder] = constOp;
  sourceConstants[path][opOrder] = constSource;
}


void
PE::generateIns()
{
  DEBUG("[genIns]Generating instruction word for PE <" << t << ", " << x << ", " << y << ">");
  // instructions have 3 categories:
  // normal, ptype, and ctype
  for (nodePath path : {none, true_path, false_path}) {
    // node if of the path
    if (nodeId[path] == -1) {
      // if no node, use a default noop ins
      insWord[path] = Instruction::noopIns;
      DEBUG("[genIns]Noop ins word for path " << path << " is " << std::hex << insWord[path]);
    } else {
      // a valid node
      Node* node = nodes[path];
      if (node == nullptr)
        FATAL("[genIns]ERROR! Node pointer not valid");
      
      // generate instruction word based on the node
      if (node->isCType()) {
        // c type instruction
        // first for loop exit
        Instruction::CondOpCode opCode;
        switch (node->getOp()) {
          case cmpSGT:
          case cmpUGT:
            opCode = Instruction::CMPGT;
            break;
          case cmpEQ:
            opCode = Instruction::CMPEQ;
            break;
          case cmpNEQ:
            opCode = Instruction::CMPNEQ;
            break;
          case cmpSLT:
          case cmpULT:
            opCode = Instruction::CMPLT;
            break;
          default:
            FATAL("[genIns]ERROR! Unsupported ctype op code");
        }
        bool loopExit = false;
        bool spBit = false;
        int16_t brImm = 0;
        if (node->isLoopExit()) {
          loopExit = true;
          spBit = node->getExitCond();
          // for kernel loop exit, the br offset should be 0x3ff (all 1)
          brImm = 0x3ff;
        } else
          spBit = node->isSplitCond();
        // get the first 2 inputs
        Instruction::PEInputMux lMux = dirToMux(sourceDirections[path][0]);
        Instruction::PEInputMux rMux = dirToMux(sourceDirections[path][1]);
        // get the immediate value if needed
        int32_t imm = 0;
        if (lMux == Instruction::Immediate && rMux == Instruction::Immediate)
          FATAL("[genIns]ERROR! Both mux immediate");
        if (lMux == Instruction::Immediate) 
          imm = sourceConstants[path][0];
        else if (rMux == Instruction::Immediate)
          imm = sourceConstants[path][1];
        // get the registers
        int reg1 = 0;
        if (lMux == Instruction::Register) {
          // currently we only live in will be in register
          int liveinNode = node->getDataSource()[0].first;
          if (liveinReg.find(liveinNode) != liveinReg.end())
            reg1 = liveinReg[liveinNode];
          else
            FATAL("[genIns]ERROR! Can't find the register for live in");
        }
        int reg2 = 0;
        if (rMux == Instruction::Register) {
          int liveinNode = node->getDataSource()[1].first;
          if (liveinReg.find(liveinNode) != liveinReg.end())
            reg2 = liveinReg[liveinNode];
          else
            FATAL("[genIns]ERROR! Can't find the register for live in");
        }
        insWord[path] = Instruction::encodeCIns(Instruction::int32, opCode, loopExit, spBit, lMux, 
                        rMux, reg1, reg2, 0, false, brImm, imm);
        DEBUG("[genIns]C type ins word for path " << path << " is " << std::hex << insWord[path]);

      } else if (node->isPType()) {
        // p type instruction
        Instruction::PredOPCode opCode;
        switch (node->getOp()) {
          case ld_add:
          case st_add:
            opCode = Instruction::address_generator;
            break;
          case sext:
            opCode = Instruction::signExtend;
            break;
          case cond_select:
            opCode = Instruction::sel;
            break;
          default:
            FATAL("[genIns]ERROR! Unsupported p type op code");
        }
        // get the first 2 inputs
        Instruction::PEInputMux lMux = dirToMux(sourceDirections[path][0]);
        Instruction::PEInputMux rMux;
        int32_t imm = 0;
        if (opCode == Instruction::address_generator) {
          // addgen lMux: address, rMux: alignment
          rMux = Instruction::Immediate;
          imm = node->getAlignment();
        } else {
          rMux = dirToMux(sourceDirections[path][1]);
          // get the immediate value if needed
          if (lMux == Instruction::Immediate && rMux == Instruction::Immediate)
            FATAL("[genIns]ERROR! Both mux immediate");
          if (lMux == Instruction::Immediate) 
            imm = sourceConstants[path][0];
          else if (rMux == Instruction::Immediate)
            imm = sourceConstants[path][1];
        }
        // next the pMux
        Instruction::PEInputMux pMux = Instruction::Self;
        if (opCode == Instruction::sel) {
          // only sel will have predicate input
          pMux = dirToMux(sourceDirections[path][2]);
          if (pMux == Instruction::Immediate)
            FATAL("[genIns]ERROR! predicate inpute shouldn't be immediate");
        } else if (opCode == Instruction::address_generator) {
          // for addgen, we are using the address bus, so
          // with the 3bit pmux :  WE | AB | DB, it should be 0'b010
          pMux = (Instruction::PEInputMux)0b010;
        }
        // get the registers
        int reg1 = 0;
        if (lMux == Instruction::Register) {
          // currently we only live in will be in register
          int liveinNode = node->getDataSource()[0].first;
          if (liveinReg.find(liveinNode) != liveinReg.end())
            reg1 = liveinReg[liveinNode];
          else
            FATAL("[genIns]ERROR! Can't find the register for live in");
        }
        int reg2 = 0;
        if (rMux == Instruction::Register) {
          int liveinNode = node->getDataSource()[1].first;
          if (liveinReg.find(liveinNode) != liveinReg.end())
            reg2 = liveinReg[liveinNode];
          else
            FATAL("[genIns]ERROR! Can't find the register for live in");
        }
        int regP = 0;
        if (pMux == Instruction::Register) {
          int liveinNode = node->getDataSource()[2].first;
          if (liveinReg.find(liveinNode) != liveinReg.end())
            regP = liveinReg[liveinNode];
          else
            FATAL("[genIns]ERROR! Can't find the register for live in");
        }
        insWord[path] = Instruction::encodePIns(Instruction::int32, opCode, lMux, rMux, pMux, reg1, reg2, regP, imm);
        DEBUG("[genIns]P type ins word for path " << path << " is " << std::hex << insWord[path]);
      } else if (node->isPhi()) {
        // for phi type instructions, inside kernel it is just an route
        // we get the source from within the loop and do a orOp 0
        Instruction::PEInputMux lMux;
        if (sourceDirections[path][0] != constOp && sourceDirections[path][0] != liveInData) {
          // source 0 inside loop
          lMux = dirToMux(sourceDirections[path][0]);
        } else {
          if (sourceDirections[path][1] != constOp && sourceDirections[path][1] != liveInData)
            FATAL("[genIns]Phi op with 2 operand inside loop");
          // source 1 inside loop
          lMux = dirToMux(sourceDirections[path][1]);
        }
        insWord[path] = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, false, 
                                lMux, Instruction::Immediate, 0, 0, 0, false, false, false, 0);
        DEBUG("[genIns]Phi ins word for path " << path << " is " << std::hex << insWord[path]);
      } else {
        // regular ins word
        if (node->getOp() == ld_data) {
          // load data: will just be an or op with source from bus
          insWord[path] = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, 
                          false, Instruction::DataBus, Instruction::Immediate, 0, 0, 0, false, false, false, 0);
          DEBUG("[genIns]LD ins word for path " << path << " is " << std::hex << insWord[path]);
        } else if (node->getOp() == st_data) {
          // store data will be an or op of lmux with 0, with data bus asserted
          auto lMux = dirToMux(sourceDirections[path][1]);
          if (lMux == Instruction::Immediate)
            FATAL("[genIns]ERROR! Storing constant");
          insWord[path] = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, false, 
                          lMux, Instruction::Immediate, 0, 0, 0, false, false, true, 0);
          DEBUG("[genIns]ST ins word for path " << path << " is " << std::hex << insWord[path]);
        } else if (node->getOp() == route) {
          // route will be an or with 0
          auto lMux = dirToMux(sourceDirections[path][0]);
          if (lMux == Instruction::Immediate)
            FATAL("[genIns]ERROR! Routing constant");
          insWord[path] = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, false, 
                          lMux, Instruction::Immediate, 0, 0, 0, false, false, false, 0);
          DEBUG("[genIns]Route ins word for path " << path << " is " << std::hex << insWord[path]);
        } else {
          Instruction::OPCode opCode;
          switch (node->getOp()) {
            case add:
              opCode = Instruction::Add;
              break;
            case sub:
              opCode = Instruction::Sub;
              break;
            case mult:
              opCode = Instruction::Mult;
              break;
            case division:
              opCode = Instruction::Div;
              break;
            case shiftl:
              opCode = Instruction::cgraASL;
              break;
            case shiftr:
              opCode = Instruction::cgraASR;
              break;
            case andop:
              opCode = Instruction::AND;
              break;
            case orop:
              opCode = Instruction::OR;
              break;
            case xorop:
              opCode = Instruction::XOR;
              break;
            case cmpSGT:
            case cmpUGT:
              opCode = Instruction::GT;
              break;
            case cmpEQ:
              opCode = Instruction::EQ;
              break;
            case cmpNEQ:
              opCode = Instruction::NEQ;
              break;
            case cmpSLT:
            case cmpULT:
              opCode = Instruction::LT;
              break;
            case rem:
              opCode = Instruction::Rem;
              break;
            case shiftr_logical:
              opCode = Instruction::LSHR;
              break;
            default:
              FATAL("[genIns]ERROR! Unsupported op code for regular ins");
          }
          // get the first 2 inputs
          Instruction::PEInputMux lMux = dirToMux(sourceDirections[path][0]);
          Instruction::PEInputMux rMux = dirToMux(sourceDirections[path][1]);
          // get the immediate value if needed
          int32_t imm = 0;
          if (lMux == Instruction::Immediate && rMux == Instruction::Immediate)
            FATAL("[genIns]ERROR! Both mux immediate");
          if (lMux == Instruction::Immediate) 
            imm = sourceConstants[path][0];
          else if (rMux == Instruction::Immediate)
            imm = sourceConstants[path][1];
          // get the registers
          int reg1 = 0;
          if (lMux == Instruction::Register) {
            // currently we only live in will be in register
            int liveinNode = node->getDataSource()[0].first;
            if (liveinReg.find(liveinNode) != liveinReg.end())
              reg1 = liveinReg[liveinNode];
            else
              FATAL("[genIns]ERROR! Can't find the register for live in");
          }
          int reg2 = 0;
          if (rMux == Instruction::Register) {
            int liveinNode = node->getDataSource()[1].first;
            if (liveinReg.find(liveinNode) != liveinReg.end())
              reg2 = liveinReg[liveinNode];
            else
              FATAL("[genIns]ERROR! Can't find the register for live in");
          }
          insWord[path] = Instruction::encodeIns(Instruction::int32, opCode, false, false, 
                          lMux, rMux, reg1, reg2, 0, false, false, false, imm);
          DEBUG("[genIns]Regular ins word for path " << path << " is " << std::hex << insWord[path]);
        }
      }
    }
  } // end of iterating through node path
}


void
PE::setLiveIn(std::map<int, int> liveinReg)
{
  this->liveinReg = liveinReg;
}


void
PE::generatePrologue()
{
  // phi ins should be on N path
  for (nodePath path : {none, true_path, false_path}) {
    Node* node = nodes[path];
    if (node == nullptr)
      continue;
    if (node->getOp() == cgra_select) {
      // is a phi op, we add a check to ensure it's on the none path
      if (path != none)
        FATAL("[Prologue]ERROR! Phi op not on none path");
      phiOp = true;
      DEBUG("[Prologue]Generating prologue phi instruction word for PE <" << t << ", " << x << ", " << y << ">");
      // for phi node, we just get the operand not in loop
      Instruction::PEInputMux lMux;
      int32_t imm = 0;
      int reg1 = 0;
      if (sourceDirections[none][0] == constOp || sourceDirections[none][0] == liveInData) {
        // source 0 from outside of loop
        if (sourceDirections[none][0] == constOp) {
          // constant
          lMux = Instruction::Immediate;
          imm = sourceConstants[none][0];
        } else {
          // live in
          lMux = Instruction::Register;
          if (liveinReg.find(node->getDataSource()[0].first) == liveinReg.end())
            FATAL("[Prologue]ERROR! Can't get live in register");
          reg1 = liveinReg[node->getDataSource()[0].first];
        }
      } else if (sourceDirections[none][1] == constOp || sourceDirections[none][1] == liveInData) {
        // source 1 from outside of loop
        if (sourceDirections[none][1] == constOp) {
          // constant
          lMux = Instruction::Immediate;
          imm = sourceConstants[none][1];
        } else {
          // live in
          lMux = Instruction::Register;
          if (liveinReg.find(node->getDataSource()[1].first) == liveinReg.end())
            FATAL("[Prologue]ERROR! Can't get live in register");
          reg1 = liveinReg[node->getDataSource()[1].first];
        }
      } else 
        FATAL("[Prologue]ERROR! No node outside loop for phi");

      // the op will be an OR with 0
      proIns = Instruction::encodeIns(Instruction::int32, Instruction::OR, false, false, 
                    lMux, Instruction::Immediate, reg1, 0, 0, false, false, false, imm);
      DEBUG("[Prologue]Phi ins word for path " << path << " is " << std::hex << insWord[path]);
    }
  }
}


std::map<int, int> 
PE::getLivein()
{
  return liveinReg;
}


void
PE::setLiveOut(std::map<int, int> liveOutReg)
{
  this->liveOutReg = liveOutReg;
}
