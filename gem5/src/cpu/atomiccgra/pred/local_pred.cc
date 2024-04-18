#include "cpu/atomiccgra/pred/local_pred.hh"

#include "base/trace.hh"
#include "base/intmath.hh"
#include "debug/CGRAPred.hh"


LocalPred::LocalPred(const LocalPredictorParams *params)
    : CGRAPredUnit(params),
      localPredictorSize(params->localPredictorSize),
      localCtrBits(params->localCtrBits),
      localPredictorSets(localPredictorSize / localCtrBits),
      localCtrs(localPredictorSets, SatCounter(localCtrBits)),
      indexMask(localPredictorSets - 1)
{
    DPRINTF(CGRAPred,"Using Local Predictor\n");
    if (!isPowerOf2(localPredictorSize)) {
        fatal("Invalid local predictor size!\n");
    }

    if (!isPowerOf2(localPredictorSets)) {
        fatal("Invalid number of local predictor sets! Check localCtrBits.\n");
    }

}


void
LocalPred::update(Addr instPC, bool outcome)
{
    unsigned local_predictor_idx;

    // Update the local predictor.
    local_predictor_idx = getLocalIndex(instPC);

    DPRINTF(CGRAPred, "Looking up index %#x\n", local_predictor_idx);

    if (outcome) {
        DPRINTF(CGRAPred, "updated as true.\n");
        localCtrs[local_predictor_idx]++;
    } else {
        DPRINTF(CGRAPred, "updated as false.\n");
        localCtrs[local_predictor_idx]--;
    }

}


bool
LocalPred::lookup(Addr instPC)
{
    bool taken;
    unsigned local_predictor_idx = getLocalIndex(instPC);

    DPRINTF(CGRAPred, "Looking up index %#x\n",
            local_predictor_idx);

    uint8_t counter_val = localCtrs[local_predictor_idx];

    DPRINTF(CGRAPred, "prediction is %i.\n",
            (int)counter_val);

    taken = getPrediction(counter_val);

    return taken;
}


inline
unsigned
LocalPred::getLocalIndex(Addr instAddr)
{
    return (instAddr >> shiftAmt) & indexMask;
}


inline
bool
LocalPred::getPrediction(uint8_t &count)
{
    // Get the MSB of the count
    return (count >> (localCtrBits - 1));
}


LocalPred* 
LocalPredictorParams::create()
{
    return new LocalPred(this);
}