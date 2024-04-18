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
    */
    void update(Addr instPC, bool SplitCond, bool outcome);

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

  protected:
    // bits address needs to be shifted before indexing
    unsigned shiftAmt;

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

    // how many unrosolved predictions are still in fly
    unsigned inFly;

};



#endif