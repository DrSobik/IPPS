/* 
 * File:   Schedule.h
 * Author: DrSobik
 *
 * Created on July 1, 2011, 1:15 PM
 * 
 * Description: Schedule class contains all information about the assignments of
 *				the operations to the machines. It contains information about
 *				objective-value, which corresponds to the schedule, as well as 
 *				possible other statistical information.
 * 
 * Contained data:
 *				
 *				
 * 
 */

#ifndef SCHEDULE_H
#define	SCHEDULE_H

#include <QHash>
#include <QStack>

#include "MathExt"
#include "ProcessModel.h"
#include "Objective.h"
#include "Operation.h"

using namespace Common;

class Schedule {
private:
	/*
	double _prevObj;
	QHash<int, double> _prevOrdID2objContrib;
    
	QHash<int,  QList<QPair<int, double> > > _prevResourceID2OperIDStartTime;
	QHash<int,  QList<QPair<int, double> > > _prevResourceID2ItemIDStartTime;
	 */
	//ScalarObjective* obj; // An objective object

public:

	double objective; // Value of the optimization criterion on the scheduling level
	QHash<int, double> ordID2objContrib; // Value contributed to the objective by the order

	QList<Operation> operations; // Information about all the operations in the schedule
	QList<QPair<int, int> > operationPrecedences; // Precedence constraints between the operations in this schedule
	QList<bool > operationPrecedencesConj; // Indicates whether the corresponding precedence constraint is a technological or a schedule-based one

	ProcessModel pm; // PM corresponding to this schedule

	Schedule();
	//Schedule(const ScalarObjective* otherObj);
	Schedule(const Schedule& orig);
	virtual ~Schedule();

	/** Initialize the schedule. */
	virtual void init();

	virtual void clear();

	virtual Schedule& operator=(const Schedule& other);

	//virtual void save();

	//virtual void restore();

	/** Prepare the schedule based on the given process model. */
	virtual void fromPM(ProcessModel& pm, ScalarObjective& obj);

	//virtual Schedule& operator<<(ScalarObjective* obj);
	
	friend QTextStream& operator<<(QTextStream& out, Schedule& sched);

	friend QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Schedule& sched);

private:

};

#endif	/* SCHEDULE_H */

