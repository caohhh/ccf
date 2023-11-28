#include "cpu/atomiccgra/pred/local_pred.hh"

#include "base/trace.hh"

LocalPred::LocalPred(const LocalPredictorParams *params)
    : CGRAPredUnit(params)
{
}


LocalPred* 
LocalPredictorParams::create()
{
    return new LocalPred(this);
}