/**
 * Arc.cpp
*/

#include "Arc.h"

Arc::Arc(Node* fromNode, Node* toNode, int ID, int distance, DataDepType dep, int opOrder)
{
  from = fromNode;
  to = toNode;
  this->id = ID;
  this->distance = distance;
  dependency = dep;
  this->operandOrder = opOrder;
}


Arc::~Arc()
{
}


Arc::Arc(const Arc& originalArc, Node* newFromNode, Node* newToNode)
{
  id = originalArc.id;
  distance = originalArc.distance;
  dependency = originalArc.dependency;
  operandOrder = originalArc.operandOrder;
  from = newFromNode;
  to = newToNode;
}



Node* 
Arc::getToNode()
{
  return to;
}


DataDepType
Arc::getDependency()
{
  return dependency;
}


int
Arc::getOperandOrder()
{
  return operandOrder;
}


Node*
Arc::getFromNode()
{
  return from;
}


int
Arc::getDistance()
{
  return distance;
}


int
Arc::getId()
{
  return id;
}