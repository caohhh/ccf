/**
 * Node.cpp
 * Implementation for the Node class.
 * 
 * Cao Heng
 * 2024.3.18
*/

#include "Node.h"

Node::Node(int id, Instruction_Operation op)
{
  uid = id;
  // add a check for the operation to be a valid one
  switch (op) {
    case cmpSLEQ:
    case cmpSGEQ:
    case cmpULEQ:
    case cmpUGEQ: {
      FATAL("[Node]LEQ or GEQ instruction should not exist, check LLVM end and transform to LT or GT");
    }
    case ld_add_cond:
    case ld_data_cond:
    case loopctrl:
    case llvm_route:
    case constant:
    case rest: 
    case bitcast: {
      FATAL("[Node]Ins: " << op << "should not exist");
    }
    default:
      break;
  }
  this->op = op;
  for (int i = 0; i < 3; i++) {
    dataSource[i] = std::make_pair(-1, -1);
  }
  condBr = -1;
  leCtrl = false;
  splitCond = false;
  // default to 4 for int32
  alignment = 4;
  liveOut = false;
  liveOutId = -1;
}


void
Node::setOp(int opOrder, std::pair<int, int> oprands)
{
  dataSource[opOrder] = oprands;
}


void
Node::checkSource()
{
  if (dataSource.size() != 3)
    FATAL("[Node]ERROR! Should input 3 op info: " << uid);
  
  // if this operand exist
  std::vector<bool> hasOp(3);
  for (int i = 0; i < 3; i++) {
    if (dataSource[i].first == -1 && dataSource[i].second == -1)
      hasOp[i] = false;
    else
      hasOp[i] = true;
  }
  // we also add a check for op count
  switch (op) {
    case cond_select:
      if (hasOp[0] == false || hasOp[1] == false || hasOp[2] == false)
        FATAL("[Node]Sel op should have 3 operands: " << uid);
      break;
    case ld_add:
    case st_add:
    case ld_data:
    case route:
      if (hasOp[0] == false || hasOp[1] == true || hasOp[2] == true)
        FATAL("[Node]This op should have 1 operand: " << uid);
      break;
    default:
      if (hasOp[0] == false || hasOp[1] == false || hasOp[2] == true)
        FATAL("[Node]Most op should have 2 operands: " << uid);
      break;
  }
}


int
Node::getId()
{
  return uid;
}


Instruction_Operation
Node::getOp()
{
  return op;
}


std::map<int, std::pair<int, int>> 
Node::getDataSource()
{
  return dataSource;
}


Node* 
Node::getNode(std::vector<Node*> nodeSet, int nodeId)
{
  for (Node* node : nodeSet) {
    if (node->getId() == nodeId)
      return node;
  }
  return nullptr;
}


void
Node::setCondBr(int condBr)
{
  this->condBr = condBr;
}


void
Node::setLoopExit(bool le)
{
  this->leCtrl = le;
}


void
Node::setSplitCond(bool splitCond)
{
  this->splitCond = splitCond;
}


bool
Node::isCType()
{
  if (condBr != -1 || leCtrl)
    return true;
  else 
    return false;
}


bool
Node::isPType()
{
  if (op == ld_add || op == st_add || op == sext || op == cond_select)
    return true;
  else
    return false;
}


bool
Node::isPhi()
{
  if (op == cgra_select)
    return true;
  else 
    return false;
}


void
Node::setExitCond(bool exitCond)
{
  if (!leCtrl)
    FATAL("[Node]ERROR! Setting exit condition for a non loop exit control node");
  this->exitCond = exitCond;
}


bool
Node::isLoopExit()
{
  return leCtrl;
}


bool
Node::isSplitCond()
{
  return splitCond;
}


int
Node::getCondBr()
{
  return condBr;
}


bool
Node::getExitCond()
{
  return exitCond;
}


void
Node::setAlignment(int alignment)
{
  if (op != ld_add && op != st_add)
    FATAL("[Node]ERROR! Setting alignment for a non addgen");
  this->alignment = alignment;
}


int
Node::getAlignment()
{
  return alignment;
}


void
Node::setLiveOut(int liveOutId)
{
  liveOut = true;
  this->liveOutId = liveOutId;
}


bool
Node::isLiveOut()
{
  return liveOut;
}


int
Node::getLiveOut()
{
  if (!liveOut)
    FATAL("[Node]ERROR! Getting live out ID from a non live out node");
  return liveOutId;
}