/* 
 * File:   TrivialScheduler.h
 * Author: DrSobik
 *
 * Description:	Class TrivialScheduler performs trivial scheduling of the nodes
 *				based on their availability.
 * 
 *				Class TrivialBalanceScheduler performs scheduling of the nodes
 *				based on their availability and trying to balance the load of
 *				the machines in the whole production system.
 * 
 * Created on July 21, 2011, 9:59 AM
 */

#ifndef PRIORITYSCHEDULER_H
#define PRIORITYSCHEDULER_H

#include <QtCore>

#include "Clonable"
#include "Solver"
#include "Loader"
#include "SmartPointer"

#include "IPPSDefinitions"

#include "Scheduler"
#include "Resources"
#include "ProcessModel"
#include "Schedule"
#include "Objective"
#include "FlowFactor"

//#include "LocalSearchPM.h"
//#include "CombinedScheduler.h"

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>

using namespace lemon;
using namespace Common::Interfaces;
using namespace Common::SmartPointers;

//typedef Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&> SchedSolver;

class TrivialScheduler : public Scheduler {
public:
    TrivialScheduler();
    virtual ~TrivialScheduler();


    virtual bool schedule(ProcessModel &pm, Resources &resources, Schedule &schedule);

private:

};

class TrivialBalanceScheduler : public Scheduler {
public:
    TrivialBalanceScheduler();
    virtual ~TrivialBalanceScheduler();


    virtual bool schedule(ProcessModel &pm, Resources &resources, Schedule &schedule);

private:

};

class PriorityScheduler : public Scheduler {//LSScheduler {
//    Q_OBJECT
protected:
    QList<ListDigraph::Node> topolOrdering; // Topological ordering of the graph

    //QList<ListDigraph::Arc> selectionArcs; // The arcs representing scheduling decisions

    QSet<int> availIDs; // IDs of the available nodes for scheduling
    QSet<int> schedIDs; // IDs of the nodes which have been scheduled

    QHash<int, ListDigraph::Node> opID2Node; // Nodes of the graph by the IDs of the corresponding operations

    double smallestD; // Smallest due date in the PM    

    double totalW;
    double totalD;

    /** Prepare the process model for scheduling. */
    virtual void preparePM();

    /** Restore the state of the process model. */
    virtual void restorePM();

    /** Prepare the schedule */
    virtual void prepareSched();

public:

    PriorityScheduler();
    PriorityScheduler(PriorityScheduler& orig);
    virtual ~PriorityScheduler();

    virtual PriorityScheduler* clone();

    virtual void init();

    virtual void scheduleActions();

    /** Return the priority of the corresponding operation. */
    virtual double priority(const ListDigraph::Node& node);

    /* ---------------------  Solver  --------------------------------------- */

    //virtual void parse(const SchedulerOptions&) = 0;

    //virtual Schedule solve(const SchedulingProblem& problem, const SchedulerOptions& options) override;
    virtual Schedule solve(const SchedulingProblem& problem) override;

    /* ---------------------------------------------------------------------- */

};

class RNDScheduler : public PriorityScheduler {
public:

    RNDScheduler();
    RNDScheduler(RNDScheduler& orig);
    virtual ~RNDScheduler();

    virtual double priority(const ListDigraph::Node& node);

    virtual RNDScheduler* clone() override;

    virtual void parse(const SchedulerOptions&) override;

};

class WFIFOScheduler : public PriorityScheduler {
protected:
    bool _weightedFIFO;
public:

    WFIFOScheduler();
    WFIFOScheduler(WFIFOScheduler& orig);
    virtual ~WFIFOScheduler();

    virtual WFIFOScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weights should be considered. */
    inline void weightedFIFO(const bool& on = true) {
	_weightedFIFO = on;
    }

    virtual void parse(const SchedulerOptions&) override;

};

class WScheduler : public PriorityScheduler {
public:

    WScheduler();
    WScheduler(WScheduler& orig);
    virtual ~WScheduler();

    virtual WScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    virtual void parse(const SchedulerOptions&) override {

    }

};

/** The operation with potentially highest weighted tardiness is the most critical. */
class WTScheduler : public PriorityScheduler {
protected:

    bool _weightedT;

public:

    WTScheduler();
    WTScheduler(WTScheduler& orig);
    virtual ~WTScheduler();

    virtual WTScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weighted tardiness should be considered. */
    inline void weightedT(const bool& on = true) {
	_weightedT = on;
    }

    virtual void parse(const SchedulerOptions&) override {

    }

};

class WSPTScheduler : public PriorityScheduler {
protected:
    bool _weightedSPT;

public:

    WSPTScheduler();
    WSPTScheduler(WSPTScheduler& orig);
    virtual ~WSPTScheduler();

    virtual WSPTScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weighted processing times should be considered. */
    inline void weightedSPT(const bool& on = true) {
	_weightedSPT = on;
    }

    virtual void parse(const SchedulerOptions&) override {

    }

};

/** Earliest operation due date scheduler. */
class EODScheduler : public PriorityScheduler {
public:

    EODScheduler();
    EODScheduler(EODScheduler& orig);
    virtual ~EODScheduler();

    virtual EODScheduler* clone() override;

    virtual void preparePM();

    virtual double priority(const ListDigraph::Node& node);

    virtual void parse(const SchedulerOptions&) override {

    }

};

/** Weighted earliest operation due date scheduler. */
class WEODScheduler : public PriorityScheduler {
protected:
    bool _weightedEOD;

public:

    WEODScheduler();
    WEODScheduler(WEODScheduler& orig);
    virtual ~WEODScheduler();

    virtual WEODScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weighted processing times should be considered. */
    inline void weightedEOD(const bool& on = true) {
	_weightedEOD = on;
    }

    virtual void parse(const SchedulerOptions&) override;

};

/** Weighted earliest job due date scheduler. */
class WEDDScheduler : public PriorityScheduler {
protected:
    bool _weightedEDD;

public:

    WEDDScheduler();
    WEDDScheduler(WEDDScheduler& orig);
    virtual ~WEDDScheduler();

    virtual WEDDScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    virtual void preparePM();

    /** Decide whether the weighted processing times should be considered. */
    inline void weightedEDD(const bool& on = true) {
	_weightedEDD = on;
    }

    virtual void parse(const SchedulerOptions&) override;

};

/** Weighted shortest due date scheduler with w^2. */
class WEDD2Scheduler : public PriorityScheduler {
protected:
    bool _weightedEDD;

public:

    WEDD2Scheduler();
    WEDD2Scheduler(WEDD2Scheduler& orig);
    virtual ~WEDD2Scheduler();

    virtual WEDD2Scheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weighted processing times should be considered. */
    inline void weightedEDD(const bool& on = true) {
	_weightedEDD = on;
    }

    virtual void parse(const SchedulerOptions&) override {

    }

};

/** Weighted modified operation due date scheduler. */
class WMODScheduler : public PriorityScheduler {
protected:
    bool _weightedMOD;

public:

    WMODScheduler();
    WMODScheduler(WMODScheduler& orig);
    virtual ~WMODScheduler();

    virtual WMODScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weighted processing times should be considered. */
    inline void weightedMOD(const bool& on = true) {
	_weightedMOD = on;
    }

    virtual void parse(const SchedulerOptions&) override;

};

/** Weighted modified job due date scheduler. */
class WMDDScheduler : public PriorityScheduler {
protected:
    bool _weightedMDD;

    ListDigraph::NodeMap<double> dOrd;
    ListDigraph::NodeMap<double> pRemain;
    ListDigraph::NodeMap<int> lenRemain;

public:

    WMDDScheduler();
    WMDDScheduler(WMDDScheduler& orig);
    virtual ~WMDDScheduler();

    virtual WMDDScheduler* clone() override;

    virtual void preparePM();

    virtual double priority(const ListDigraph::Node& node);

    /** Decide whether the weighted processing times should be considered. */
    inline void weightedMDD(const bool& on = true) {
	_weightedMDD = on;
    }

    virtual void parse(const SchedulerOptions&) override;

};

/** Schedule the operations in view of machine utilization. */
class MUBScheduler : public PriorityScheduler {
public:

    MUBScheduler();
    MUBScheduler(MUBScheduler& orig);
    virtual ~MUBScheduler();

    virtual MUBScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    virtual void parse(const SchedulerOptions&) override {

    }

};

/** Schedule the operations in view of machine time balancing, i.e. try to utilize machines which are available earlier, first. */
class MTBScheduler : public PriorityScheduler {
public:

    MTBScheduler();
    MTBScheduler(MTBScheduler& orig);
    virtual ~MTBScheduler();

    virtual MTBScheduler* clone() override;

    virtual double priority(const ListDigraph::Node& node);

    virtual void parse(const SchedulerOptions&) override {

    }

};

/** ATC scheduler that considers only the available nodes. Unavailable successors for the current node can be considered. */
class ATCANScheduler : public PriorityScheduler {
private:
    double p_avg;

    double kappaPAvg;
    double kappaRPAvg;

    double totalPRemain; // Total remaining processing time for all jobs (including the unavailable ones)

    //CombinedScheduler* cs; // CS used for estimating the flow factor
    SmartPointer<SchedSolver> cs; // CS (current scheduler) used for estimating the flow factor

protected:
    bool kappaoptim;
    double kappa;
    bool considersucc; // Consider ATC indices of the successors of the available nodes

    double ff; // The flow factor
    double kappaR; // Kappa for the ready time term

    ListDigraph::NodeMap<double> dOrd;
    ListDigraph::NodeMap<double> pRemain;
    ListDigraph::NodeMap<int> lenRemain;

public:

    ATCANScheduler();
    ATCANScheduler(ATCANScheduler& orig);
    virtual ~ATCANScheduler();

    virtual ATCANScheduler* clone() override;

    virtual void preparePM();

    virtual double priority(const ListDigraph::Node& node);

    virtual void scheduleActions();

    /** ATC-Specific modification. */
    void schedAct();

    /** Schedule several operations at the same time. */
    void schedAct1();

    /** Decide whether the kappa should be optimized. */
    inline void kappaOptim(const bool& on = true) {
	kappaoptim = on;
    }

    inline void setKappa(const double& kappa = 2.0) {
	this->kappa = kappa;
	kappaOptim(false);
    }

    inline void considerSucc(const bool& on = true) {
	considersucc = on;
    }

    /** ATC index */
    double I(const double &kappapavg, const double &kappaRpAvg, const ListDigraph::Node &node);

    virtual void setObjective(ScalarObjective& newObj);

    virtual void parse(const SchedulerOptions&) override;

};

/** ATC scheduler that considers only the available nodes. Unavailable successors for the current node can be considered. */
class ATCSchedulerTest : public PriorityScheduler {
private:
    double p_avg;

    double kappaPAvg;
    double kappaRPAvg;

    double totalPRemain; // Total remaining processing time for all jobs (including the unavailable ones)

protected:
    bool kappaoptim;
    double kappa;
    bool considersucc; // Consider ATC indices of the successors of the available nodes

    double ff; // The flow factor
    double kappaR; // Kappa for the ready time term

    ListDigraph::NodeMap<double> dOrd;
    ListDigraph::NodeMap<double> pRemain;
    ListDigraph::NodeMap<int> lenRemain;

public:

    ATCSchedulerTest();
    ATCSchedulerTest(ATCSchedulerTest& orig);
    virtual ~ATCSchedulerTest();

    virtual ATCSchedulerTest* clone() override;

    virtual void preparePM();

    virtual double priority(const ListDigraph::Node& node);

    virtual void scheduleActions();

    /** ATC-Specific modification. */
    void schedAct();

    /** Schedule several operations at the same time. */
    void schedAct1();

    /** Decide whether the kappa should be optimized. */
    inline void kappaOptim(const bool& on = true) {
	kappaoptim = on;
    }

    inline void setKappa(const double& kappa = 2.0) {
	this->kappa = kappa;
	kappaOptim(false);
    }

    inline void considerSucc(const bool& on = true) {
	considersucc = on;
    }

    /** ATC index */
    double I(const double &kappapavg, const double &kappaRpAvg, const ListDigraph::Node &node);

    virtual void parse(const SchedulerOptions&) override {

    }

};

#endif /* PRIORITYSCHEDULER_H */

