/**
 * Instruction Fetch Unit for the CGRA
 * 
 * Created on 2023/11/23
 * Cao Heng
 * 
 * 
*/

#ifndef __CGRA_IFU_HH__
#define __CGRA_IFU_HH__

#include"cpu/exec_context.hh"

class CGRA_IFU
{
  public:

    CGRA_IFU();
    virtual ~CGRA_IFU();


    /**
     * register this result into the compare history
     * 
     * @param PC PC of the CMP instruction
     * @param outcome the result of this CMP instruction
    */
    void recordCMP(Addr PC, bool outcome);
    
    /**
     * Debug function to print CMP history
    */
    void printCMPHistory();



  private:
    // <PC, outcome> result of all the CMP instruction history
    std::vector<std::pair<Addr, bool>> cmpHistory;

};

#endif