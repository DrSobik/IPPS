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

#ifndef TRIVIALSCHEDULER_H
#define TRIVIALSCHEDULER_H

#include <QtCore>

#include "Clonable"
#include "Solver"
#include "Loader"
#include "Exceptions"
#include "SmartPointer"

#include "IPPSDefinitions"

#include "Scheduler"
#include "Resources"
#include "ProcessModel"
#include "Schedule"
//#include "LocalSearchPM.h"
#include "Objective"

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>

using namespace lemon;
using namespace Common;
using namespace Common::Interfaces;
using namespace Common::Exceptions;
using namespace Common::Util;
using namespace Common::SmartPointers;

//typedef Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&> SchedSolver;

/** Uses several fast priority-based schedulers and selects the best result. */
class CombinedScheduler : public Scheduler {
private:

    QSet<int> allowedSchedIDs;

    //Settings settings; // The settings of this scheduler

public:
    //QList<Scheduler* > schedulers; // A list of all priority schedulers that should be used
    QList<SmartPointer<Scheduler> > schedulers; // A list of all priority schedulers that should be used

    QList<int> bestPerformingStat; // Statistics of the best performing schedulers
    int lastBestSchedulerIdx; // Indicates an index of a scheduler which performed the best during the scheduling

    ProcessModel _bestPM; // A PM corresponding to the best schedule

public:

    CombinedScheduler();
    CombinedScheduler(CombinedScheduler& orig);
    virtual ~CombinedScheduler();

    virtual void init();

    //virtual Clonable* clone();

    virtual void scheduleActions();

    /* Add a new scheduler */
    virtual CombinedScheduler& operator<<(Scheduler* sch);

    /* Last best performing scheduler */
    virtual Scheduler* lastBestScheduler();

    /** Return the best PM during the run. The PM corresponds to the best scheduler. */
    virtual ProcessModel& bestPM();

    /* Clear the scheduler */
    virtual void clear();

    virtual void setObjective(ScalarObjective& newObj);
    /*  ##################  Solver  ########################################  */

    CombinedScheduler* clone() override;

    virtual void parse(const SchedulerOptions& options) override;

    virtual Schedule solve(const SchedulingProblem&/*, const SchedulerOptions&*/) override;

    /*  ####################################################################  */

};

///** A scheduler which selects a priority-based schedule and then tries to improve it by a local search. */
//// IMPORTANT!!! This class is now, possibly, peprecated since LS may have an init scheduler
//class CombinedSchedulerLS : public Scheduler {
//protected:
//    CombinedScheduler cs; // 
//    LocalSearchPM ls;
//
//public:
//    CombinedSchedulerLS();
//    CombinedSchedulerLS(CombinedSchedulerLS& orig);
//    virtual ~CombinedSchedulerLS();
//
//    virtual void init();
//
//    //virtual Clonable* clone();
//
//
//    virtual void scheduleActions();
//
//    /** Return the combined scheduler object. */
//    CombinedScheduler& combinedSchedulerObject() {
//	return cs;
//    }
//
//    /** Return the LS object. */
//    LocalSearchPM& localSearchObject() {
//	return ls;
//    }
//
//    /*  ##################  Solver  ########################################  */
//
//    CombinedSchedulerLS* clone() override;
//
//    virtual void parse(const SchedulerOptions&) override;
//
//    virtual Schedule solve(const SchedulingProblem&/*, const SchedulerOptions&*/) override;
//
//    /*  ####################################################################  */
//
//};
//
///** A scheduler which selects a priority-based schedule and then tries to improve it by a local search. */
//class CombinedSchedulerModLS : public Scheduler {
//protected:
//    CombinedScheduler cs; // 
//    LocalSearchModPM ls;
//
//public:
//    CombinedSchedulerModLS();
//    CombinedSchedulerModLS(CombinedSchedulerModLS& orig);
//    virtual ~CombinedSchedulerModLS();
//
//    virtual void init();
//
//    virtual void scheduleActions();
//
//    /** Return the combined scheduler object. */
//    CombinedScheduler& combinedSchedulerObject() {
//	return cs;
//    }
//
//    /** Return the LS object. */
//    LocalSearchModPM& localSearchObject() {
//	return ls;
//    }
//
//    /*  ##################  Solver  ########################################  */
//
//    CombinedSchedulerModLS* clone();
//
//    virtual void parse(const SchedulerOptions&) override {
//	throw ErrMsgException<>("CombinedSchedulerModLS::parse not implemented!");
//    }
//
//    virtual Schedule solve(const SchedulingProblem&/*, const SchedulerOptions&*/) override {
//	throw ErrMsgException<>("CombinedSchedulerModLS::solve not implemented!");
//    }
//
//    /*  ####################################################################  */
//
//};
//
///** A scheduler which selects a priority-based schedule and then tries to improve it by an IMPROVED local search. */
//class CombinedSchedulerLSCP : public Scheduler {
//protected:
//    CombinedScheduler cs; // 
//    LocalSearchPMCP ls; // Local search with critical paths
//
//public:
//    CombinedSchedulerLSCP();
//    CombinedSchedulerLSCP(CombinedSchedulerLSCP& orig);
//    virtual ~CombinedSchedulerLSCP();
//
//    virtual void init();
//
//    virtual void scheduleActions();
//
//    /** Return the combined scheduler object. */
//    CombinedScheduler& combinedSchedulerObject() {
//	return cs;
//    }
//
//    /** Return the LS object. */
//    LocalSearchPMCP& localSearchObject() {
//	return ls;
//    }
//
//    /*  ##################  Solver  ########################################  */
//
//    CombinedSchedulerLSCP* clone();
//
//    virtual void parse(const SchedulerOptions&) override {
//	throw ErrMsgException<>("CombinedSchedulerLSCP::parse not implemented!");
//    }
//
//    virtual Schedule solve(const SchedulingProblem&/*, const SchedulerOptions&*/) override {
//	throw ErrMsgException<>("CombinedSchedulerLSCP::solve not implemented!");
//    }
//
//    /*  ####################################################################  */
//
//};

#endif /* TRIVIALSCHEDULER_H */

