#include "cpu/atomiccgra/pred/tournament_pred.hh"

#include "base/trace.hh"
#include "debug/CGRAPred.hh"

TournamentPred::TournamentPred(const TournamentPredictorParams* params)
    : CGRAPredUnit(params),
      localPredictorSize(params->localPredictorSize),
      localCtrBits(params->localCtrBits),
      localCtrs(localPredictorSize, SatCounter(localCtrBits)),
      localHistoryTableSize(params->localHistoryTableSize),
      localHistoryBits(ceilLog2(params->localPredictorSize)),
      globalPredictorSize(params->globalPredictorSize),
      globalCtrBits(params->globalCtrBits),
      globalCtrs(globalPredictorSize, SatCounter(globalCtrBits)),
      globalHistory(0),
      globalHistoryBits(
          ceilLog2(params->globalPredictorSize) >
          ceilLog2(params->choicePredictorSize) ?
          ceilLog2(params->globalPredictorSize) :
          ceilLog2(params->choicePredictorSize)),
      choicePredictorSize(params->choicePredictorSize),
      choiceCtrBits(params->choiceCtrBits),
      choiceCtrs(choicePredictorSize, SatCounter(choiceCtrBits))
{
    DPRINTF(CGRAPred, "Using Tournament Predictor\n");
    if (!isPowerOf2(localPredictorSize)) {
        fatal("Invalid local predictor size!\n");
    }

    if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global predictor size!\n");
    }

    localPredictorMask = mask(localHistoryBits);

    if (!isPowerOf2(localHistoryTableSize)) {
        fatal("Invalid local history table size!\n");
    }

    //Setup the history table for the local table
    localHistoryTable.resize(localHistoryTableSize);

    for (int i = 0; i < localHistoryTableSize; ++i)
        localHistoryTable[i] = 0;

    // Set up the global history mask
    // this is equivalent to mask(log2(globalPredictorSize)
    globalHistoryMask = globalPredictorSize - 1;

    if (!isPowerOf2(choicePredictorSize)) {
        fatal("Invalid choice predictor size!\n");
    }

    // Set up choiceHistoryMask
    // this is equivalent to mask(log2(choicePredictorSize)
    choiceHistoryMask = choicePredictorSize - 1;

    //Set up historyRegisterMask
    historyRegisterMask = mask(globalHistoryBits);

    //Check that predictors don't use more bits than they have available
    if (globalHistoryMask > historyRegisterMask) {
        fatal("Global predictor too large for global history bits!\n");
    }
    if (choiceHistoryMask > historyRegisterMask) {
        fatal("Choice predictor too large for global history bits!\n");
    }

    if (globalHistoryMask < historyRegisterMask &&
        choiceHistoryMask < historyRegisterMask) {
        inform("More global history bits than required by predictors\n");
    }

    // Set thresholds for the three predictors' counters
    // This is equivalent to (2^(Ctr))/2 - 1
    localThreshold  = (ULL(1) << (localCtrBits  - 1)) - 1;
    globalThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
    choiceThreshold = (ULL(1) << (choiceCtrBits - 1)) - 1;
}


bool
TournamentPred::lookup(Addr instPC)
{
    bool local_prediction;
    unsigned local_history_idx;
    unsigned local_predictor_idx;

    bool global_prediction;
    bool choice_prediction;
    // for global and local history, we should use the latest version
    // for the counters, we should use the non-speculative version

    //Lookup in the local predictor to get its branch prediction
    local_history_idx = calcLocHistIdx(instPC);
    local_predictor_idx = localHistoryTable[local_history_idx]
        & localPredictorMask;
    local_prediction = backupLocalCtrs[predictPtr][local_predictor_idx] > localThreshold;

    //Lookup in the global predictor to get its branch prediction
    global_prediction = globalThreshold <
      backupGlobalCtrs[predictPtr][globalHistory & globalHistoryMask];

    //Lookup in the choice predictor to see which one to use
    choice_prediction = choiceThreshold <
      backupChoiceCtrs[predictPtr][globalHistory & choiceHistoryMask];

    assert(local_history_idx < localHistoryTableSize);

    if (choice_prediction) {
        // using the global predictor
        return global_prediction;
    } else {
        // using the local predictor
        return local_prediction;
    }
}


void
TournamentPred::update(Addr branch_addr, bool taken)
{
    bool local_prediction;
    unsigned local_history_idx;
    unsigned local_predictor_idx;

    bool global_prediction;

    //Lookup in the local predictor to get its branch prediction
    local_history_idx = calcLocHistIdx(branch_addr);
    local_predictor_idx = localHistoryTable[local_history_idx]
        & localPredictorMask;
    local_prediction = backupLocalCtrs[predictPtr][local_predictor_idx] > localThreshold;

    //Lookup in the global predictor to get its branch prediction
    global_prediction = globalThreshold <
      backupGlobalCtrs[predictPtr][globalHistory & globalHistoryMask];

    // Update the choice predictor to tell it which one was correct if
    // there was a prediction.
    if (local_prediction != global_prediction) {
        // If the local prediction matches the actual outcome,
        // decrement the cosunter. Otherwise increment the
        // counter.
        unsigned choice_predictor_idx = globalHistory & choiceHistoryMask;
        if (local_prediction == taken) {
            choiceCtrs[choice_predictor_idx]--;
        } else if (global_prediction == taken) {
            choiceCtrs[choice_predictor_idx]++;
        }
    }

    // Update the counters with the proper
    // resolution of the branch. Histories are updated
    // speculatively, restored upon squash() calls, and
    // recomputed upon update(squash = true) calls,
    // so they do not need to be updated.
    if (taken) {
        globalCtrs[globalHistory & globalHistoryMask]++;
        localCtrs[local_predictor_idx]++;
    } else {
        globalCtrs[globalHistory & globalHistoryMask]--;
        localCtrs[local_predictor_idx]--;
    }

    // last update the history regs
    updateGlobalHist(taken);
    updateLocalHist(local_history_idx, taken);

    // now update the backup
    backupLocalCtrs[updatePtr] = localCtrs;
    backupLocalHist[updatePtr] = localHistoryTable;
    backupGlobalCtrs[updatePtr] = globalCtrs;
    backupGlobalHist[updatePtr] = globalHistory;
    backupChoiceCtrs[updatePtr] = choiceCtrs;
}


inline
unsigned
TournamentPred::calcLocHistIdx(Addr &branch_addr)
{
    // Get low order bits after removing instruction offset.
    return (branch_addr >> shiftAmt) & (localHistoryTableSize - 1);
}


inline
void
TournamentPred::updateGlobalHist(bool taken)
{
    globalHistory = (globalHistory << 1) | taken;
    globalHistory = globalHistory & historyRegisterMask;
}


inline
void
TournamentPred::updateLocalHist(unsigned local_history_idx, bool taken)
{
    localHistoryTable[local_history_idx] =
        (localHistoryTable[local_history_idx] << 1) | taken;
}


void
TournamentPred::setupBackup()
{
    backupLocalCtrs.resize(iterCount, std::vector<SatCounter>(localPredictorSize, SatCounter(localCtrBits)));
    backupLocalHist.resize(iterCount, std::vector<unsigned>(localHistoryTableSize, 0));
    backupGlobalCtrs.resize(iterCount, std::vector<SatCounter>(globalPredictorSize, SatCounter(globalCtrBits)));
    backupGlobalHist.resize(iterCount, 0);
    backupChoiceCtrs.resize(iterCount, std::vector<SatCounter>(choicePredictorSize, SatCounter(choiceCtrBits)));
}


void
TournamentPred::rollback()
{
    localCtrs = backupLocalCtrs[predictPtr];
    localHistoryTable = backupLocalHist[predictPtr];
    globalCtrs = backupGlobalCtrs[predictPtr];
    globalHistory = backupGlobalHist[predictPtr];
    choiceCtrs = backupChoiceCtrs[predictPtr];
}


TournamentPred*
TournamentPredictorParams::create()
{
    return new TournamentPred(this);
}