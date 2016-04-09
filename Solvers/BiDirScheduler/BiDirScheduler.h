/* 
 * File:   BiDirScheduler.h
 * Author: DrSobik
 *
 * Created on August 22, 2011, 3:47 PM
 */

#ifndef BIDIRSCHEDULER_H
#define	BIDIRSCHEDULER_H

#include "Scheduler.h"
#include "Resources.h"
#include "ProcessModel.h"
#include "Schedule.h"

#include <QMap>

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>

using namespace lemon;

class BiDirScheduler : public Scheduler {
public:
	BiDirScheduler();
	virtual ~BiDirScheduler();

	virtual bool schedule(ProcessModel &pm, Resources &resources, Schedule &schedule);

private:

};

#endif	/* BIDIRSCHEDULER_H */

