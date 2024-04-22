#include "cpu/atomiccgra/pred/history_pred.hh"

#include "base/trace.hh"
#include "debug/CGRAPred.hh"


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
    DPRINTF(CGRAPred,"Using History Predictor\n");
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
    // we should use the GHR used for prediction, which the GHR should currently hold
    unsigned choiceHistoryIdx = ((instPC >> shiftAmt)
                            & choiceHistoryMask);
    unsigned globalHistoryIdx = (((instPC >> shiftAmt)
                                ^ globalHistoryReg)
                                & globalHistoryMask);
    assert(choiceHistoryIdx < choicePredictorSize);
    assert(globalHistoryIdx < globalPredictorSize);

    auto choiceSet = backupChoice[predictPtr];
    auto takenSet = backupTaken[predictPtr];
    auto notTakenSet = backupNotTaken[predictPtr];

    bool choicePrediction = choiceSet[choiceHistoryIdx]
                            > choiceThreshold;
    bool takenGHBPrediction = takenSet[globalHistoryIdx]
                            > takenThreshold;
    bool notTakenGHBPrediction = notTakenSet[globalHistoryIdx]
                                 > notTakenThreshold;
    bool finalPrediction;
    if (choicePrediction) {
        finalPrediction = takenGHBPrediction;
    } else {
        finalPrediction = notTakenGHBPrediction;
    }


    if (choicePrediction) {
        // if the taken array's prediction was used, update it
        if (outcome) {
            takenCounters[globalHistoryIdx]++;
        } else {
            takenCounters[globalHistoryIdx]--;
        }
    } else {
        // if the not-taken array's prediction was used, update it
        if (outcome) {
            notTakenCounters[globalHistoryIdx]++;
        } else {
            notTakenCounters[globalHistoryIdx]--;
        }
    }

    if (finalPrediction == outcome) {
       /* If the final prediction matches the actual branch's
        * outcome and the choice predictor matches the final
        * outcome, we update the choice predictor, otherwise it
        * is not updated. While the designers of the bi-mode
        * predictor don't explicity say why this is done, one
        * can infer that it is to preserve the choice predictor's
        * bias with respect to the branch being predicted; afterall,
        * the whole point of the bi-mode predictor is to identify the
        * atypical case when a branch deviates from its bias.
        */
        if (finalPrediction == choicePrediction) {
            if (outcome) {
                choiceCounters[choiceHistoryIdx]++;
            } else {
                choiceCounters[choiceHistoryIdx]--;
            }
        }
    } else {
        // always update the choice predictor on an incorrect prediction
        if (outcome) {
            choiceCounters[choiceHistoryIdx]++;
        } else {
            choiceCounters[choiceHistoryIdx]--;
        }
    }

    // last update the GHR
    updateGlobalHistReg(outcome);
    
    // now  update the backup
    backupGHR[updatePtr] = globalHistoryReg;
    backupChoice[updatePtr] = choiceCounters;
    backupTaken[updatePtr] = takenCounters;
    backupNotTaken[updatePtr] = notTakenCounters;
}


bool
HistoryPred::lookup(Addr condAddr)
{ 
    // we use address to index choice 
    unsigned choiceHistoryIdx = ((condAddr >> shiftAmt)
                                & choiceHistoryMask);

    // use address hash with branch histroy to index predictor
    // for the GHR, we always use the updated one
    unsigned globalHistoryIdx = (((condAddr >> shiftAmt)
                                ^ (globalHistoryReg & historyRegisterMask))
                                & globalHistoryMask);

    assert(choiceHistoryIdx < choicePredictorSize);
    assert(globalHistoryIdx < globalPredictorSize);

    // predict is done using the non-speculative set
    auto choiceSet = backupChoice[predictPtr];
    auto takenSet = backupTaken[predictPtr];
    auto notTakenSet = backupNotTaken[predictPtr];

    bool choicePrediction = choiceSet[choiceHistoryIdx]
                            > choiceThreshold;
    bool takenGHBPrediction = takenSet[globalHistoryIdx]
                              > takenThreshold;
    bool notTakenGHBPrediction = notTakenSet[globalHistoryIdx]
                                 > notTakenThreshold;
    bool finalPrediction;

    if (choicePrediction) {
        finalPrediction = takenGHBPrediction;
    } else {
        finalPrediction = notTakenGHBPrediction;
    }

    return finalPrediction;
}


void
HistoryPred::updateGlobalHistReg(bool taken)
{
    globalHistoryReg = taken ? (globalHistoryReg << 1) | 1 :
                                (globalHistoryReg << 1);
    globalHistoryReg &= historyRegisterMask;
}


void
HistoryPred::setupBackup()
{
    backupGHR.resize(iterCount, 0);
    backupChoice.resize(iterCount, std::vector<SatCounter>(choicePredictorSize, SatCounter(choiceCtrBits)));
    backupTaken.resize(iterCount, std::vector<SatCounter>(globalPredictorSize, SatCounter(globalCtrBits)));
    backupNotTaken.resize(iterCount, std::vector<SatCounter>(globalPredictorSize, SatCounter(globalCtrBits)));
}


void
HistoryPred::rollback()
{
    globalHistoryReg = backupGHR[predictPtr];
    choiceCounters = backupChoice[predictPtr];
    takenCounters = backupTaken[predictPtr];
    notTakenCounters = backupNotTaken[predictPtr];
}


HistoryPred*
HistoryPredictorParams::create()
{
    return new HistoryPred(this);
}