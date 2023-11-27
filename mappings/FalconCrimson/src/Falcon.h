/* This program finds a mapping using modified Falcon's algorithm.
 * Diverting from the previously used Clique based algorithm, this
 * method checks for a random mapping of each node based on MRRG. 
 * On a failure to map the node, Falcon tries to map the node, by
 * remapping some nodes in the timeslots.
 *
 * Author: Mahesh Balasubramanian
 * Date  : Sept 13, 2019
 * Note  : All the functions are implemented in header file. 
 * This file includes support for 2 scheduling algorithms 
 *
 * Last edit: Mar 25 2022
 * Author: Vinh TA
 */        


#ifndef FALCON_H
#define FALCON_H

#include "Parser.h"
#include "INTERITERATIONDEPENDENCY.h"
#include <math.h>

class Falcon
{
  public:
    Falcon() { } //default constructor

    ~Falcon() {  } //default destructor

    // This function adds routing nodes and schedules them to the DFG
    bool Route_And_Reschedule(DFG* copy_DFG, int id, int II, int number_of_resources);

    bool Map(Parser myParser);
}; 


#endif
