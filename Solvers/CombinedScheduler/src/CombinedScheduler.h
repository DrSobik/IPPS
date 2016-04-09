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
#define	TRIVIALSCHEDULER_H

#include "Scheduler.h"
#include "Resources.h"
#include "ProcessModel.h"
#include "Schedule.h"
#include "LocalSearchPM.h"
#include "Clonable.h"

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>

#include <QStack>

using namespace lemon;

/** Uses several fast priority-based schedulers and selects the best result. */
class CombinedScheduler : public Scheduler {
public:
    QList<Scheduler* > schedulers; // A list of all priority schedulers that should be used
	
	QList<int> bestPerformingStat; // Statistics of the best performing schedulers
	int lastBestSchedulerIdx; // Indicates an index of a scheduler which performed the best during the scheduling
	
	ProcessModel _bestPM; // A PM corresponding to the best schedule

public:	
	
    CombinedScheduler();
    CombinedScheduler(CombinedScheduler& orig);
    virtual ~CombinedScheduler();

	virtual void init();
	
    virtual Clonable* clone();
	
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

};

/** A scheduler which selects a priority-based schedule and then tries to improve it by a local search. */
class CombinedSchedulerLS : public Scheduler{
protected:
	CombinedScheduler cs; // 
	LocalSearchPM ls;

public:	
	CombinedSchedulerLS();
	CombinedSchedulerLS(CombinedSchedulerLS& orig);
	virtual ~CombinedSchedulerLS();
	
	virtual void init();
	
	virtual Clonable* clone();
	
	virtual void scheduleActions();
	
	/** Return the combined scheduler object. */
	CombinedScheduler& combinedSchedulerObject(){
		return cs;
	} 
	
	/** Return the LS object. */
	LocalSearchPM& localSearchObject(){
		return ls;
	}
	
};

/** A scheduler which selects a priority-based schedule and then tries to improve it by a local search. */
class CombinedSchedulerModLS : public Scheduler{
protected:
	CombinedScheduler cs; // 
	LocalSearchModPM ls;

public:	
	CombinedSchedulerModLS();
	CombinedSchedulerModLS(CombinedSchedulerModLS& orig);
	virtual ~CombinedSchedulerModLS();
	
	virtual void init();
	
	virtual Clonable* clone();
	
	virtual void scheduleActions();
	
	/** Return the combined scheduler object. */
	CombinedScheduler& combinedSchedulerObject(){
		return cs;
	} 
	
	/** Return the LS object. */
	LocalSearchModPM& localSearchObject(){
		return ls;
	}
	
};

/** A scheduler which selects a priority-based schedule and then tries to improve it by an IMPROVED local search. */
class CombinedSchedulerLSCP : public Scheduler{
protected:
	CombinedScheduler cs; // 
	LocalSearchPMCP ls; // Local search with critical paths

public:	
	CombinedSchedulerLSCP();
	CombinedSchedulerLSCP(CombinedSchedulerLSCP& orig);
	virtual ~CombinedSchedulerLSCP();
	
	virtual void init();
	
	virtual Clonable* clone();
	
	virtual void scheduleActions();
	
	/** Return the combined scheduler object. */
	CombinedScheduler& combinedSchedulerObject(){
		return cs;
	} 
	
	/** Return the LS object. */
	LocalSearchPMCP& localSearchObject(){
		return ls;
	}
	
};

#endif	/* TRIVIALSCHEDULER_H */

