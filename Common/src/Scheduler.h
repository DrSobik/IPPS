/* 
 * File:   Scheduler.h
 * Author: DrSobik
 *
 * Created on July 1, 2011, 1:52 PM
 * 
 * Description: Scheduler class is a basis class for scheduling algorithms. It 
 *				is independent of the applied scheduling algorithms. It uses
 *				information about to production facilities, operations that have 
 *				to be scheduled on the machines and returns a schedule.
 * 
 */

#ifndef SCHEDULER_H
#define	SCHEDULER_H

#include "DebugExt.h"

#include "Operation.h"
#include "Resources.h"
#include "Schedule.h"
#include "ProcessModel.h"

#include "Objective.h"
//#include "LocalSearchPM.h"
#include "Clonable.h"

#include <QObject>
#include <QThread>

using namespace Common;

/** A class with general scheduling options */
class SchedulerOptions : public QHash<QString, QString> {
public:

	SchedulerOptions() {

	}

	SchedulerOptions(SchedulerOptions& orig) : QHash<QString, QString> (orig) {

	}

	SchedulerOptions(const SchedulerOptions& orig) : QHash<QString, QString> (orig) {

	}

	virtual ~SchedulerOptions() {

	}

	friend QTextStream& operator<<(QTextStream& out, SchedulerOptions& options) {
		for (SchedulerOptions::iterator iter = options.begin(); iter != options.end(); iter++) {
			out << iter.key() << "=" << iter.value() << endl;
		}
		return out;
	}

};

/** Base class for all schedulers. */
class Scheduler : public QObject, public Clonable {
	Q_OBJECT

public:
	int ID;

	ProcessModel pm; // Pointer to the current process model
	Resources rc; // Pointer to the current resources
	Schedule *sched; // Pointer to the current schedule which has to be found

	SchedulerOptions options; // Options for this scheduler

	ScalarObjective* obj; // Objective

	Scheduler();
	Scheduler(Scheduler& orig);
	virtual ~Scheduler();

	virtual Clonable* clone();

	/** Initialization of the scheduler. */
	virtual void init();

	virtual bool schedule(ProcessModel &pm, Resources &resources, Schedule &schedule);

	/** The scheduling is actually performed here. */
	virtual void scheduleActions();

	/** Universal virtual function for scheduling. */
	virtual bool schedule();

	/** Calculate the flow factor. */
	virtual double flowFactor();

	/** Set the objective */
	virtual void setObjective(ScalarObjective& newObj);
	
private:

signals:
	void sigFinished(const int&);
	void sigFinished();

public slots:
	void slotSchedule();

};

class SchedThread : public QThread {
	Q_OBJECT
public:
	SchedThread();
	SchedThread(QObject* orig);
	virtual ~SchedThread();

signals:
	void sigRunScheduler();

public slots:
	void runScheduler();

};

#endif	/* SCHEDULER_H */

