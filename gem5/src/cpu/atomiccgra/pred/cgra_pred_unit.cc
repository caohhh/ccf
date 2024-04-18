#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"

#include "base/trace.hh"
#include "debug/CGRAPred.hh"


CGRAPredUnit::CGRAPredUnit(const Params *params)
    : SimObject(params),
      shiftAmt(params->shiftAmt),
      stats(this),
      inFly(0)
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

    if (splitCond)
        ++stats.condPredicted;
    bool prediction = lookup(instPC);
    inFly++;
    DPRINTF(CGRAPred, "Predictor predicted %i for PC %s\n",
            prediction, instPC);
    return prediction;
}


void
CGRAPredUnit::squash()
{
    ++stats.condIncorrect;
    // return to the recovery point
}


void
CGRAPredUnit::update(Addr instPC, bool SplitCond, bool outcome)
{

    update(instPC, outcome);
}