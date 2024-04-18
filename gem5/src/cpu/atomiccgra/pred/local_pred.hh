/**
 * Local counter based last time predictor
*/

#ifndef __CGRA__LOCAL_PRED_HH__
#define __CGRA__LOCAL_PRED_HH__

#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"
#include "params/LocalPredictor.hh"
#include "base/sat_counter.hh"

class LocalPred : public CGRAPredUnit
{
  public:
    /**
     * Default branch predictor constructor.
     */
    LocalPred(const LocalPredictorParams *params);

    /**
     * Updates the predictor with the correct outcome
     * @param instPC PC of the CMP instruction to be updated
     * @param outcome resolved outcome of this instruction
    */
    void update(Addr instPC, bool outcome);

    bool lookup(Addr instPC);


  private:
    /**
     *  Returns the taken/not taken prediction given the value of the
     *  counter.
     *  @param count The value of the counter.
     *  @return The prediction based on the counter value.
     */
    inline bool getPrediction(uint8_t &count);


     /** Size of the local predictor. */
    const unsigned localPredictorSize;

    /** Number of bits of the local predictor's counters. */
    const unsigned localCtrBits;

    /** Calculates the local index based on the PC. */
    inline unsigned getLocalIndex(Addr PC);

    /** Number of sets. */
    const unsigned localPredictorSets;

    /** Array of counters that make up the local predictor. */
    std::vector<SatCounter> localCtrs;

    /** Mask to get index bits. */
    const unsigned indexMask;
};


#endif
