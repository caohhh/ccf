#include "cpu/atomiccgra/pred/history_pred.hh"

HistoryPred::HistoryPred(const HistoryPredictorParams* params)
    : CGRAPredUnit(params),
      globalHistoryReg(0),
      globalHistoryBits(ceilLog2(params->globalPredictorSize)),
      choicePredictorSize(params->choicePredictorSize),
      choiceCtrBits(params->choiceCtrBits),
      globalPredictorSize(params->globalPredictorSize),
      globalCtrBits(params->globalCtrBits),
      choiceCounters(choicePredictorSize, SatCounter(choiceCtrBits)),
      takenCounters(globalPredictorSize, SatCounter(globalCtrBits)),
      notTakenCounters(globalPredictorSize, SatCounter(globalCtrBits))
{
    if (!isPowerOf2(choicePredictorSize))
        fatal("Invalid choice predictor size.\n");
    if (!isPowerOf2(globalPredictorSize))
        fatal("Invalid global history predictor size.\n");

    historyRegisterMask = mask(globalHistoryBits);
    choiceHistoryMask = choicePredictorSize - 1;
    globalHistoryMask = globalPredictorSize - 1;

    choiceThreshold = (ULL(1) << (choiceCtrBits - 1)) - 1;
    takenThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
    notTakenThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
}


void 
HistoryPred::update(Addr instPC, bool outcome)
{
}


bool
HistoryPred::lookup(Addr condAddr)
{
     
    // we use address to index choice 
    unsigned choiceHistoryIdx = ((condAddr >> shiftAmt)
                                & choiceHistoryMask);

    // use address hash with branch histroy to index predictor
    unsigned globalHistoryIdx = (((condAddr >> shiftAmt)
                                ^ (globalHistoryReg & historyRegisterMask))
                                & globalHistoryMask);

    assert(choiceHistoryIdx < choicePredictorSize);
    assert(globalHistoryIdx < globalPredictorSize);

    bool choicePrediction = choiceCounters[choiceHistoryIdx]
                            > choiceThreshold;
    bool takenGHBPrediction = takenCounters[globalHistoryIdx]
                              > takenThreshold;
    bool notTakenGHBPrediction = notTakenCounters[globalHistoryIdx]
                                 > notTakenThreshold;
    bool finalPrediction;

    if (choicePrediction) {
        finalPrediction = takenGHBPrediction;
    } else {
        finalPrediction = notTakenGHBPrediction;
    }

    // on prediction, the GHR is updated to issue multiple predictions
    // the prediction counters are not updated though
    updateGlobalHistReg(finalPrediction);

    return finalPrediction;
}


void
HistoryPred::updateGlobalHistReg(bool taken)
{
    globalHistoryReg = taken ? (globalHistoryReg << 1) | 1 :
                                (globalHistoryReg << 1);
}


HistoryPred*
HistoryPredictorParams::create()
{
    return new HistoryPred(this);
}