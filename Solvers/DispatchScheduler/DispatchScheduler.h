/* 
 * File:   DispatchScheduler.h
 * Author: DrSobik
 *
 * Created on July 1, 2011, 3:39 PM
 */

#ifndef DISPATCHSCHEDULER_H
#define	DISPATCHSCHEDULER_H

#include "Scheduler.h"
#include "Resources.h"
#include "ProcessModel.h"
#include "Schedule.h"

class DispatchScheduler : public Scheduler {
public:
	DispatchScheduler();
	virtual ~DispatchScheduler();
	
	virtual bool schedule(ProcessModel &pm, Resources &resources, Schedule &schedule);
	
private:

};

#endif	/* DISPATCHSCHEDULER_H */

