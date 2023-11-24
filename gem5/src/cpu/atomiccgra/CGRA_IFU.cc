/**
 * Instruction Fetch Unit for the CGRA
 * 
 * Created on 2023/11/23
 * Cao Heng
 * 
 * 
*/

#include "cpu/atomiccgra/CGRA_IFU.hh"

#include "debug/CGRA_IFU.hh"

CGRA_IFU::CGRA_IFU()
{
}


CGRA_IFU::~CGRA_IFU()
{
}


void
CGRA_IFU::recordCMP(Addr PC, bool outcome)
{
    cmpHistory.push_back(std::make_pair(PC, outcome));
}


void
CGRA_IFU::printCMPHistory()
{
    DPRINTF(CGRA_IFU, "\n* * * * Printing out CMP History * * * * \n");
    for (const auto& pair : cmpHistory) {
        DPRINTF(CGRA_IFU, "PC: %x, outcome: %d\n", pair.first, pair.second);
    }
}