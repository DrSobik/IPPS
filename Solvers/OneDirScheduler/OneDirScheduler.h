/* 
 * File:   OneDirScheduler.h
 * Author: DrSobik
 *
 * Created on August 26, 2011, 10:24 AM
 */

#ifndef ONEDIRSCHEDULER_H
#define	ONEDIRSCHEDULER_H

#include "Scheduler.h"
#include "Resources.h"
#include "ProcessModel.h"
#include "Schedule.h"

#include <QMap>

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>

using namespace lemon;

class OneDirScheduler : public Scheduler {
public:
	OneDirScheduler();
	virtual ~OneDirScheduler();
	
	virtual bool schedule(ProcessModel &pm, Resources &resources, Schedule &schedule);
	
private:

};

#endif	/* ONEDIRSCHEDULER_H */

