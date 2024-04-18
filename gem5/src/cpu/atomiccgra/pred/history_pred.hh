/**
 * bi mode global history predictor
 * 
 * Using addrsss hash with global branch history to index counter predictor entry
 * each entry has 2 counters, which to use is decided by a choice predictor indexed by address
*/


#ifndef __CGRA_HISTORY_PRED_HH__
#define __CGRA_HISTORY_PRED_HH__

#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"
#include "params/HistoryPredictor.hh"
#include "base/sat_counter.hh"

class HistoryPred : public CGRAPredUnit
{
  public:
    HistoryPred(const HistoryPredictorParams* params);

    void update(Addr instPC, bool outcome);
    bool lookup(Addr instPC);
    
  private:
    void updateGlobalHistReg(bool taken);

    // the GHR will be 32 bits, with the bits after defined used as backup
    unsigned globalHistoryReg;
    unsigned globalHistoryBits;
    unsigned historyRegisterMask;

    unsigned choicePredictorSize;
    unsigned choiceCtrBits;
    unsigned choiceHistoryMask;
    unsigned globalPredictorSize;
    unsigned globalCtrBits;
    unsigned globalHistoryMask;

    // choice predictors
    std::vector<SatCounter> choiceCounters;
    // taken direction predictors
    std::vector<SatCounter> takenCounters;
    // not-taken direction predictors
    std::vector<SatCounter> notTakenCounters;

    unsigned choiceThreshold;
    unsigned takenThreshold;
    unsigned notTakenThreshold;
};


#endif