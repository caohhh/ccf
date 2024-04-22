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
  
  protected:
    void setupBackup();

    void rollback();

  private:
    void updateGlobalHistReg(bool taken);

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

    // backup and restore related
    std::vector<unsigned> backupGHR;
    std::vector<std::vector<SatCounter>> backupChoice;
    std::vector<std::vector<SatCounter>> backupTaken;
    std::vector<std::vector<SatCounter>> backupNotTaken;
};


#endif