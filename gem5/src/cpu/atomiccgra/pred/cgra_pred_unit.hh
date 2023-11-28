#ifndef __CGRA_PRED_UNIT_HH__
#define __CGRA_PRED_UNIT_HH__

#include "sim/sim_object.hh"
#include "params/CGRAPredictor.hh"


/**
 * Wrapper for all predictors
*/

class CGRAPredUnit : public SimObject
{
  public:
    typedef CGRAPredictorParams Params;
    /**
     * Basic constructor
    */
    CGRAPredUnit(const Params *p);

  protected:
    unsigned numThreads;
};



#endif