#include "cpu/atomiccgra/pred/cgra_pred_unit.hh"

#include "base/trace.hh"
#include "debug/CGRAPred.hh"


CGRAPredUnit::CGRAPredUnit(const Params *params)
    : SimObject(params)
{
    DPRINTF(CGRAPred, "Created CGRA Prediction Unit\n");
}

