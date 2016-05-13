/* 
 * File:   Planner.h
 * Author: DrSobik
 *
 * Created on July 29, 2011, 9:59 AM
 * 
 * Description : Class Planner is the base class for searching the space of 
 *				 BOPs based on the Products in the manufacturing system. 
 *				 It operates on the set of BOP IDs for the products.
 * 
 * Contained data:
 *				
 *				pmm	-	informational server for the planner.
 * 
 *				scheduler
 *					-	is used to perform operation scheduling based on the 
 *						provided resources and the the current process model.
 *						This field is temporary since the scheduling must be 
 *						performed by the Scheduler Agent.
 * 
 *				schedule
 *					-	is the result of scheduling. This field is temporary 
 *						since the scheduling must be performed by the 
 *						Scheduler Agent.
 * 
 *				resource
 *					-	resources to perform scheduling on. This field is 
 *						temporary since the scheduling must be 
 *						performed by the Scheduler Agent.
 * 
 * 
 */

#ifndef PLANNER_H
#define	PLANNER_H

#include "IterativeAlg"

#include "IPPSDefinitions"

#include "Product"
#include "Order"

#include "Resources"
#include "Schedule"
#include "Scheduler"
#include "Protocol"

#include "ProcessModelManager"

#include <QList>
#include <QHash>
#include <QTime>

#include "Saveable"
#include "Plan"
#include "PlanSched"

#include "DebugExt.h"

#include "Objective"

using namespace Common;

/** A class which describes how a planning algorithm(agent should behave) */
class PlannerStrategy : public QObject {
	Q_OBJECT
private:
	QString strategyString; // A string which describes the current strategy

public:

	PlannerStrategy();
	PlannerStrategy(const PlannerStrategy&);
	virtual ~PlannerStrategy();

	/** Set a string wihch describes a strategy of the planner. */
	virtual bool setStrategy(const QString& strStrategy);

	/** Return the strategy string*/
	QString getStrategy() const;

	virtual PlannerStrategy& operator=(const PlannerStrategy&);

	virtual PlannerStrategy* clone();

protected:

	virtual bool parseStrategy(const QString& strStrategy);

signals:

	void sigStrategySchanged(const bool&); // Emitted every time a strategy of the planner changes

};


class Planner : public EventIterativeAlg {
	Q_OBJECT
protected:
	ProcessModelManager *pmm;

	Scheduler *scheduler;
	Resources *rc;

	Protocol protocol;

	ScalarObjective* objective; // Objective for the algorithm (its pointer)

public:

	Planner();
	virtual ~Planner();

	/** Set the scheduler. */
	virtual Planner& operator<<(Scheduler *scheduler) {
		this->scheduler = scheduler;
		return *this;
	}

	/** Set the schedule. */
	virtual Planner& operator<<(Schedule *) {
		//this->schedule = schedule;
		return *this;
	}

	/** Set the resources. */
	virtual Planner& operator<<(Resources *rc) {
		this->rc = rc;
		return *this;
	}

	/** Set the process model manger. */
	virtual Planner& operator<<(ProcessModelManager *pmm) {
		this->pmm = pmm;
		return *this;
	}

	/** Set the protocol. */
	virtual Planner& operator<<(Protocol *protocol) {
		this->protocol = *protocol;
		return *this;
	}

	/** Set the objective. */
	virtual Planner& operator<<(ScalarObjective* obj) {
		if (obj == NULL) {
			Debugger::err << "Planner& operator<<(ScalarObjective* obj) : Trying to clone a NULL objective!!!" << ENDL;
		}
		if (this->objective != NULL) {
			delete this->objective;
		}
		this->objective = obj->clone();
		return *this;
	}

	/** Set the strategy. */
	virtual Planner& operator<<(PlannerStrategy&) {

		Debugger::err << "Planner& operator<<(PlannerStrategy& strategy) : Not implemented!!!" << ENDL;

		return *this;
	}

};

#endif	/* PLANNER_H */

