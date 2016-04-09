/* 
 * File:   CPScheduler.h
 * Author: DrSobik
 *
 * Description : Class CPScheduler (re)schedules the given job shop using the 
 *				 local search procedure based on substituting a critical 
 *				 schedule-based conjunctive arc in the graph with its opposite 
 *				 arc, i. e. swapping two operations on the corresponding machine.
 * 
 * Created on March 28, 2012, 10:06 PM
 */

#ifndef CPSCHEDULER_H
#define	CPSCHEDULER_H

#include "IterativeAlg.h"
#include "ProcessModel.h"
#include "Resources.h"

#include "TGScheduler.h"

class CPScheduler : public IterativeAlg {
private:
	ProcessModel *pm; // Pointer to the process model
	Resources *rc; // Pointer to the resources of the system
	QSet<int> *M0; // Scheduled tool groups schedules of which will be reoptimized
	QHash<int, TGSelection> *toolid2selection; // Selections of the scheduled tool groups

	QList<ListDigraph::Node> terminals;

public:
	CPScheduler();
	virtual ~CPScheduler();

	/** Perform the (re)scheduling. */
	void schedule(ProcessModel &pm, Resources &rc, QSet<int> &M0, QHash<int, TGSelection> &toolid2selection);

	/** Initialize the scheduler. */
	virtual void init();

	/** Preprocessing actions for the search algorithm. */
	virtual void preprocessingActions();

	/** Postprocessing actions for the search algorithm. */
	virtual void postprocessingActions();

	/** One step of the VNS heuristic. */
	virtual void stepActions();

	/** Assess the new solution. */
	virtual void assessActions();

	/** Check whether the new solution should be accepted. */
	virtual bool acceptCondition();

	/** Actions in case the new solution is accepted. */
	virtual void acceptActions();

	/** Actions in case the new solution is declined. */
	virtual void declineActions();

	/** Stopping condition for the algorithm. */
	virtual bool stopCondition();

	/** Actions to be performed after the stop condition has been met. */
	virtual void stopActions();

};

#endif	/* CPSCHEDULER_H */

