#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"

#include "base/trace.hh"
#include "debug/CGRAPred.hh"


CGRAPredUnit::CGRAPredUnit(const Params *params)
    : SimObject(params),
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
CGRAPredUnit::predict(Addr instPC)
{
    ++stats.lookups;

    ++stats.condPredicted;
    bool prediction = lookup(instPC);

    DPRINTF(CGRAPred, "Predictor predicted %i for PC %s\n",
            prediction, instPC);
    return prediction;
}


void
CGRAPredUnit::squash()
{
    ++stats.condIncorrect;
}