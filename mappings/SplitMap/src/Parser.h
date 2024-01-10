/**
 * Parser.h
 * 
 * For parsing LLVM MultiDDGGen output
*/

#ifndef __SPLITMAP_PARSER_H__
#define __SPLITMAP_PARSER_H__

#include <string>
#include "DFG.h"

class Parser
{
  public:
    Parser(std::string nodeFile, std::string edgeFile);
    virtual ~Parser();
    
    // parse the nodefile and edgefile to construct a DFG
    void ParseDFG(DFG* myDFG);

  private:
    std::string nodeFile;
    std::string edgeFile;

};

#endif //__SPLITMAP_PARSER_H__