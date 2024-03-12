/**
 * Schedule.h
 * Class to store scheduel info
*/

#ifndef __SPLITMAP_SCHEDULE_H__
#define __SPLITMAP_SCHEDULE_H__

#include "Node.h"

class schedule
{
  public:
    schedule();
    schedule(int xDim, int yDim);
    ~schedule();

    // returns if the node is scheduled
    bool isScheduled(Node* node);

    // returns the time node is scheduled at
    int getScheduleTime(Node* node);

    // returns if the given schedule has enough resources for a load at given time 
    virtual bool memLdResAvailable(nodePath path, int time);

    // returns if the given schedule has enough resources for a store at given time 
    virtual bool memStResAvailable(nodePath path, int time);

    // returns if the given schedule has enough resources for a regular op at given time 
    virtual bool resAvailable(nodePath path, int time);

    // schedule a load address at time, and data at time + 1
    virtual void scheduleLd(Node* addNode, Node* dataNode, int time);

    // schedule a regular op at time
    virtual void scheduleOp(Node* node, int time);

    // schedule a store address and data at time
    virtual void scheduleSt(Node* storeNode, Node* storeRelatedNode, int time);

    // returns the max time of this schedule
    int getMaxTime();

  protected:
    // cgra size info x dimansion
    int xDim;
    // y dimansion of cgra
    int yDim;
    // size of cgra as in x*y
    int cgraSize;
    // memory resource available per row, fixed at 1 for now
    int perRowMem = 1;
    // map of node : time
    std::map<int, int> nodeSchedule;
    // map of time : nodeID scheduled at time
    std::map<int, std::vector<int>> timeSchedule;
    // map of path, time : peused 
    std::map<std::tuple<nodePath, int>, int> peUsed;
    // map of path, time : peused
    std::map<std::tuple<nodePath, int>, int> addBusUsed;
    // map of path, time : peused
    std::map<std::tuple<nodePath, int>, int> dataBusUsed;
};


class moduloSchedule : public schedule
{
  public:
    moduloSchedule(int xDim, int yDim);
    ~moduloSchedule();

    // set the II for this modulo schedule
    void setII(int II);

    // returns if the given schedule has enough resources for 
    // a regular op at given time considering modulo overlap
    bool resAvailable(nodePath path, int scheduleTime) override;

    // returns if the given schedule has enough resources for a load at given time 
    // considering modulo scheduling
    bool memLdResAvailable(nodePath path, int scheduleTime) override;

    // returns if the given schedule has enough resources for a store at given time 
    // considering modulo scheduling
    bool memStResAvailable(nodePath path, int scheduleTime) override;


    // schedule a regular op at time considering modulo scheduling
    void scheduleOp(Node* node, int time) override;

    // schedule a load address at time, and data at time + 1 considering modulo scheduling
    void scheduleLd(Node* addNode, Node* dataNode, int time) override;

    // schedule a store address and data at time considering modulo scheduling
    void scheduleSt(Node* storeNode, Node* storeRelatedNode, int time) override;

    // reset the modulo schedule
    void clear();

    // outputs a dot file of the modulo schedule based on the given DFG
    void print(DFG* myDFG, std::string name);

    // returns the II for this modulo schedule
    int getII();

    // returns the modulo schedule time node is scheduled at
    int getModScheduleTime(Node* node);


  private:
    int II;
    // map of node : mod time
    std::map<int, int> nodeScheduleMod;
    // map of mod time : nodeID scheduled at mod time
    std::map<int, std::vector<int>> timeScheduleMod;
};


#endif