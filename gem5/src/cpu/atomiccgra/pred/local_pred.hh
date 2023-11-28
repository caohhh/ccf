#ifndef __CGRA__LOCAL_PRED_HH__
#define __CGRA__LOCAL_PRED_HH__

#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"
#include "params/LocalPredictor.hh"

class LocalPred : public CGRAPredUnit
{
  public:
    /**
     * Default branch predictor constructor.
     */
    LocalPred(const LocalPredictorParams *params);
};


#endif
