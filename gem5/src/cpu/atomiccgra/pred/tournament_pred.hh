/**
 * Implements a tournament branch predictor, inspired by the one
 * used in the 21264.  It has a local predictor, which uses a local history
 * table to index into a table of counters, and a global predictor, which
 * uses a global history to index into a table of counters.  A choice
 * predictor chooses between the two.  Both the global history register
 * and the selected local history are speculatively updated.
 */


#ifndef __CGRA_TOURNAMENT_PRED_HH__
#define __CGRA_TOURNAMENT_PRED_HH__

#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"
#include "params/TournamentPredictor.hh"
#include "base/sat_counter.hh"

class TournamentPred : public CGRAPredUnit
{
  public:
    TournamentPred(const TournamentPredictorParams *params);
    void update(Addr instPC, bool outcome);
    bool lookup(Addr instPC);
  
  protected:
    void setupBackup();
    void rollback();

  private:
    /**
     * Returns the local history index, given a branch address.
     * @param branch_addr The branch's PC address.
     */
    inline unsigned calcLocHistIdx(Addr &branch_addr);

    /** Updates global history 
     * @param taken The result to update
    */
    inline void updateGlobalHist(bool taken);

    /**
     * Updates local histories
     * @param local_history_idx The local history table entry that
     * will be updated.
     * @param taken The result to update
     */
    inline void updateLocalHist(unsigned local_history_idx, bool taken);

    /** Number of counters in the local predictor. */
    unsigned localPredictorSize;

    /** Mask to truncate values stored in the local history table. */
    unsigned localPredictorMask;

    /** Number of bits of the local predictor's counters. */
    unsigned localCtrBits;

    /** Local counters. */
    std::vector<SatCounter> localCtrs;

    /** Array of local history table entries. */
    std::vector<unsigned> localHistoryTable;

    /** Number of entries in the local history table. */
    unsigned localHistoryTableSize;

    /** Number of bits for each entry of the local history table. */
    unsigned localHistoryBits;

    /** Number of entries in the global predictor. */
    unsigned globalPredictorSize;

    /** Number of bits of the global predictor's counters. */
    unsigned globalCtrBits;

    /** Array of counters that make up the global predictor. */
    std::vector<SatCounter> globalCtrs;

    /** Global history register. Contains as much history as specified by
     *  globalHistoryBits. Actual number of bits used is determined by
     *  globalHistoryMask and choiceHistoryMask. */
    unsigned globalHistory;

    /** Number of bits for the global history. Determines maximum number of
        entries in global and choice predictor tables. */
    unsigned globalHistoryBits;

    /** Mask to apply to globalHistory to access global history table.
     *  Based on globalPredictorSize.*/
    unsigned globalHistoryMask;

    /** Mask to apply to globalHistory to access choice history table.
     *  Based on choicePredictorSize.*/
    unsigned choiceHistoryMask;

    /** Mask to control how much history is stored. All of it might not be
     *  used. */
    unsigned historyRegisterMask;

    /** Number of entries in the choice predictor. */
    unsigned choicePredictorSize;

    /** Number of bits in the choice predictor's counters. */
    unsigned choiceCtrBits;

    /** Array of counters that make up the choice predictor. */
    std::vector<SatCounter> choiceCtrs;

    /** Thresholds for the counter value; above the threshold is taken,
     *  equal to or below the threshold is not taken.
     */
    unsigned localThreshold;
    unsigned globalThreshold;
    unsigned choiceThreshold;

    /**Backup sets*/
    std::vector<std::vector<SatCounter>> backupLocalCtrs;
    std::vector<std::vector<unsigned>> backupLocalHist;
    std::vector<std::vector<SatCounter>> backupGlobalCtrs;
    std::vector<unsigned> backupGlobalHist;
    std::vector<std::vector<SatCounter>> backupChoiceCtrs;
};


#endif