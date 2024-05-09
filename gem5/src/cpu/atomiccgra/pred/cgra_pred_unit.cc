#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"

#include "base/trace.hh"
#include "debug/CGRAPred.hh"


CGRAPredUnit::CGRAPredUnit(const Params *params)
    : SimObject(params),
      shiftAmt(params->shiftAmt),
      predictPtr(0),
      updatePtr(0),
      stats(this)
{
    DPRINTF(CGRAPred, "Created CGRA Prediction Unit\n");
}


CGRAPredUnit::CGRAPredUnitStats::CGRAPredUnitStats(Stats::Group *parent)
    : Stats::Group(parent),
      ADD_STAT(lookups, "Number of BP lookups"),
      ADD_STAT(condPredicted, "Number of conditional cmps predicted"),
      ADD_STAT(condIncorrect, "Number of conditional cmps incorrect")
{
}


bool
CGRAPredUnit::predict(Addr instPC, bool splitCond)
{
    ++stats.lookups;

    bool prediction = lookup(instPC);
    DPRINTF(CGRAPred, "Predictor predicted %i for PC %s\n",
            prediction, instPC);

    if (splitCond) {
        // if it is the split cond, we also update the predictors on the spot
        // since it will be the issueing of a new iteration
        ++stats.condPredicted;
        // advance the update pointer first
        updatePtr = (updatePtr + 1) % iterCount;
        update(instPC, prediction);
    }
    return prediction;
}


void
CGRAPredUnit::squash()
{
    ++stats.condIncorrect;
    // return to the recovery point
    updatePtr = (predictPtr + 1) % iterCount;
    // rollback resets the update set to the contents of predictPtr
    rollback();
}


void
CGRAPredUnit::update(Addr instPC, bool SplitCond, bool outcome, bool squashed)
{
    if (SplitCond)
        predictPtr = (predictPtr + 1) % iterCount;
    // we update the outcome only when the inst is not the split cond, or it is a squashed
    // a not squashed splitcond is updated when it's predicted
    if (!SplitCond || squashed)
        update(instPC, outcome);
}


void
CGRAPredUnit::setIterCount(unsigned count)
{
    iterCount = count;
    setupBackup();
    // reset the ptr
    updatePtr = 0;
    predictPtr = 0;
}