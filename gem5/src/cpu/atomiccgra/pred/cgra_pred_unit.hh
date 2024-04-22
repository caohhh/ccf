/**
 * The pred unit act as a wrapper for all predictors
*/

#ifndef __CGRA_PRED_UNIT_HH__
#define __CGRA_PRED_UNIT_HH__

#include "sim/sim_object.hh"
#include "base/statistics.hh"

#include "params/CGRAPredictor.hh"


class CGRAPredUnit : public SimObject
{
  public:
    typedef CGRAPredictorParams Params;

    /**
     * Basic constructor
    */
    CGRAPredUnit(const Params *p);

    /**
     * Predict the outcome of a cmp instruction
    */
    bool predict(Addr instPC, bool splitCond);

    /**
     * update the predictor with the correct outcome
     * @param instPC PC of the CMP instruction to be updated
     * @param SplitCond if this CMP is the split condition
     * @param outcome resolved outcome of this instruction
     * @param squashed if this update is after a squash
    */
    void update(Addr instPC, bool SplitCond, bool outcome, bool squashed);

    /**
     * Updates the predictor with the correct outcome
     * @param instPC PC of the CMP instruction to be updated
     * @param outcome resolved outcome of this instruction
    */
    virtual void update(Addr instPC, bool outcome) = 0;

    /**
     * Looks up a given PC in the predictor to see the outcome
     * @param instPC The PC to look up.
     * @return predicted outcome of the CMP instruction
     */
    virtual bool lookup(Addr instPC) = 0;

    /**
     * currently only a placeholder to update the incorrect
     * prediction stat
    */
    void squash();

    /**
     * set the iteration count, used as the number of backup sets to use
    */
    void setIterCount(unsigned count);

  protected:
    // bits address needs to be shifted before indexing
    unsigned shiftAmt;
    // the iteration count of the current task
    unsigned iterCount;
    // pointer to the set of predictor used for prediction (no speculative info is used to update this set)
    unsigned predictPtr;
    // pointer to the set of predictor to update
    unsigned updatePtr;

    /**
     * setup the backups
    */
    virtual void setupBackup() = 0;

    /**
     * roll back to the predict set
    */
    virtual void rollback() = 0;

  private:
    struct CGRAPredUnitStats : public Stats::Group {
        CGRAPredUnitStats(Stats::Group *parent);

        /** Stat for number of BP lookups. */
        Stats::Scalar lookups;
        /** Stat for number of conditional branches predicted. */
        Stats::Scalar condPredicted;
        /** Stat for number of conditional branches predicted incorrectly. */
        Stats::Scalar condIncorrect;
    } stats;
};



#endif