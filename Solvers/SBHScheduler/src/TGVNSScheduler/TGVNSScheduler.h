/* 
 * File:   TGVNSScheduler.h
 * Author: DrSobik
 * 
 * Description:	Class TGVNSScheduler implements a VNS approach for scheduling 
 *				the operations on the given tool group. It considers the 
 *				precedence constraints between the operations.
 * 
 * Created on February 27, 2012, 4:40 PM
 */

#ifndef TGVNSSCHEDULER_H
#define	TGVNSSCHEDULER_H

#include <QtCore>

#include <lemon/list_graph.h>

#include "RandExt"

#include "Resources.h"
#include "ProcessModel.h"

#include "TGScheduler.h"
#include "IterativeAlg.h"
#include "SBHScheduler.h"
#include "LocalSearchPM.h"


using namespace lemon;

class SBHScheduler;

// Tool group scheduler based on the local searc heuristic
class TGSchedulerLS : public TGScheduler{
public:
	
	TGScheduler *iniScheduler; // Scheduler for preparing initial schedules
	
	LocalSearchPM ls;
	
	
	TGSchedulerLS();
	TGSchedulerLS(TGSchedulerLS& other);
	virtual ~TGSchedulerLS();
	
	/** Initialize the scheduler. */
	virtual void init();
	
	/** Clear the scheduler */
	virtual void clear();
	
	virtual Clonable* clone();
	
	virtual void run();

	virtual void schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection);
	
	/** Apply ATC rule in order to get the initial assignment for VNS. */
	void initialAssignment(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes);
	
	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
	
};

class TGVNSScheduler : public TGScheduler, public IterativeAlg {
private:
	int kmax;
	int k;

	double prev_obj; // Previous objective value
	double cur_obj; // Currently found objective value
	double best_obj; // Best found objective value

	QHash<int, QList<ListDigraph::Node> > machID2opnodes; // Operation assignment to the machines (encoded solution). The sequence of operation processing on each machine is kept.
	QHash<int, QList<ListDigraph::Node> > prev_machID2opnodes;
	QHash<int, QList<ListDigraph::Node> > bestID2opnodes; // Operation assignment to the machines (encoded solution). The sequence of operation processing on each machine is kept.
	
	TGSelection bestTGSelection;

	QList<ListDigraph::Node> *prev_nodes;
	QHash<int, double> rs;

	TGSelection prev_tgselection;

	QHash<int, QPair<int, int> > tabus; // Tabu list for the operation moves (op_ID, (mach_ID, pos));

	bool acceptedWorse;
	
public:

	TGScheduler *iniScheduler; // Scheduler for preparing initial schedules

	TGVNSScheduler();
	TGVNSScheduler(TGVNSScheduler& orig);
	virtual ~TGVNSScheduler();

	virtual Clonable* clone();

	/* ---------------------  Flow control relevant  ------------------------ */

	/** Initialize the scheduler. */
	virtual void init();

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

	/** Preprocessing actions for the search algorithm. */
	virtual void preprocessingActions();

	/** Postprocessing actions for the search algorithm. */
	virtual void postprocessingActions();

	virtual void run();

	virtual void schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection);

	/** Schedule the current sequence of the operations on the tool group 
	 *  considering the precedence constraints between the operations.*/
	virtual void scheduleCurOpSeq();

	/** Preserve the initial values of the ready times of the operation. */
	virtual void preserveRs();

	/** Restore the initial values of the ready times of the operation. */
	virtual void restoreRs();

	/** Shaking step of the VNS algorithm. */
	virtual void shake();

	/** Randomly swap the given number of times randomly selected operations on
	 *  the randomly selected machines. */
	virtual void randomSwapOperations(const int &nops);

	/** Randomly swap the given number of times the randomly selected operations on
	 *  the same machine. */
	virtual void randomSwapOperationsSameMachine(const int& mid, const int &nops);
	
	/** Check whether swapping the given operations on the given machines is correct. */
	virtual bool swapPossible(const int &mid1, const int &mid2, const int &opidx1, const int &opidx2);

	/** Randomly move the given number of times randomly selected operation to 
	 *  the randomly selected machines. */
	virtual void randomMoveOperations(const int &nops);

	/** Randomly move the given number of times the randomly selected operation on the same machine. */
	virtual void randomMoveOperationsSameMachine(const int& mid, const int &nops);
	
	/** Check whether moving the operation is correct. */
	virtual bool movePossible(const int &from_mid, const int &to_mid, const int &from_opidx, const int &to_opidx);

	virtual void initialAssignmentRND(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes);

	/** Apply ATC rule in order to get the initial assignment for VNS. */
	void initialAssignment(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes);

	/** Perform local search near to the current solution. */
	virtual void localSearch();

	/** A simple local search.*/
	virtual void localSearchSimple();
	
	/** Local search based on the iterative local search heuristic with DTO.*/
	virtual void localSearchLS();
	
	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
	
	/** Extracts the operation-to-machine assignment from the selection */
	virtual QHash<int, QList<ListDigraph::Node> > assignmentFromSelection(TGSelection& selection);
};


class TGVNSSchedulerPinSin : public TGScheduler, public IterativeAlg {
private:
	int kmax;
	int k;

	double prev_obj; // Previous objective value
	double cur_obj; // Currently found objective value
	double best_obj; // Best found objective value

	QHash<int, QList<ListDigraph::Node> > machID2opnodes; // Operation assignment to the machines (encoded solution). The sequence of operation processing on each machine is kept.
	QHash<int, QList<ListDigraph::Node> > prev_machID2opnodes;
	QHash<int, QList<ListDigraph::Node> > bestID2opnodes; // Operation assignment to the machines (encoded solution). The sequence of operation processing on each machine is kept.
	
	TGSelection bestTGSelection;

	QList<ListDigraph::Node> *prev_nodes;
	QHash<int, double> rs;

	TGSelection prev_tgselection;

	QHash<int, QPair<int, int> > tabus; // Tabu list for the operation moves (op_ID, (mach_ID, pos));

	bool acceptedWorse;
	
public:

	TGScheduler *iniScheduler; // Scheduler for preparing initial schedules

	TGVNSSchedulerPinSin();
	TGVNSSchedulerPinSin(TGVNSSchedulerPinSin& orig);
	virtual ~TGVNSSchedulerPinSin();

	virtual Clonable* clone();

	/* ---------------------  Flow control relevant  ------------------------ */

	/** Initialize the scheduler. */
	virtual void init();

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

	/** Preprocessing actions for the search algorithm. */
	virtual void preprocessingActions();

	/** Postprocessing actions for the search algorithm. */
	virtual void postprocessingActions();

	virtual void run();

	virtual void schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection);

	/** Schedule the current sequence of the operations on the tool group 
	 *  considering the precedence constraints between the operations.*/
	virtual void scheduleCurOpSeq();

	/** Preserve the initial values of the ready times of the operation. */
	virtual void preserveRs();

	/** Restore the initial values of the ready times of the operation. */
	virtual void restoreRs();

	/** Shaking step of the VNS algorithm. */
	virtual void shake();

	/** Randomly swap the given number of times randomly selected operations on
	 *  the randomly selected machines. */
	virtual void randomSwapOperations(const int &nops);

	/** Randomly swap the given number of times the randomly selected operations on
	 *  the same machine. */
	virtual void randomSwapOperationsSameMachine(const int& mid, const int &nops);
	
	/** Check whether swapping the given operations on the given machines is correct. */
	virtual bool swapPossible(const int &mid1, const int &mid2, const int &opidx1, const int &opidx2);

	/** Randomly move the given number of times randomly selected operation to 
	 *  the randomly selected machines. */
	virtual void randomMoveOperations(const int &nops);

	/** Randomly move the given number of times the randomly selected operation on the same machine. */
	virtual void randomMoveOperationsSameMachine(const int& mid, const int &nops);
	
	/** Check whether moving the operation is correct. */
	virtual bool movePossible(const int &from_mid, const int &to_mid, const int &from_opidx, const int &to_opidx);

	virtual void initialAssignmentRND(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes);

	/** Apply ATC rule in order to get the initial assignment for VNS. */
	void initialAssignment(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes);

	/** Perform local search near to the current solution. */
	virtual void localSearch();

	/** A simple local search.*/
	virtual void localSearchSimple();
	
	/** Local search based on the iterative local search heuristic with DTO.*/
	virtual void localSearchLS();
	
	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
	
	/** Extracts the operation-to-machine assignment from the selection */
	virtual QHash<int, QList<ListDigraph::Node> > assignmentFromSelection(TGSelection& selection);
};

#endif	/* TGVNSSCHEDULER_H */

