/* 
 * File:   VNSScheduler.h
 * Author: DrSobik
 *
 * Created on June 13, 2014, 6:14 PM
 */

#ifndef VNSSCHEDULER_H
#define	VNSSCHEDULER_H

#include "IterativeAlg.h"

#include "Scheduler.h"
//#include "TrivialScheduler.h"
#include "PriorityScheduler.h"
#include "CombinedScheduler.h"
#include "LocalSearchPM.h"


class VNSScheduler : public virtual Scheduler, public virtual IterativeAlg {

public:

protected:

	CombinedScheduler cs;
	LocalSearchPM ls;


private:

	int kMax; // VNS parameter defining the largest size of the neighborhoods
	int kCur; // VNS parameter defining the current neighborhood
	int kStep; // VNS parameter defining the step size

	int smR; // Swapping/Moving radius of operations in the shaking phase
	
	double prevObj; // Previous objective value
	double curObj; // Currently found objective value
	double bestObj; // Best found objective value so far

	Schedule bestSchedule; // The best found schedule
	
	QHash<int, QList<ListDigraph::Node> > curMachID2opNodes; // Operation assignment to the machines (encoded solution). The sequence of operation processing on each machine is kept.
	QHash<int, QList<ListDigraph::Node> > prevMachID2opNodes; // Used for backup
	QHash<int, QList<ListDigraph::Node> > bestMachID2opNodes; // Operation assignment to the machines (encoded solution). The sequence of operation processing on each machine is kept.
	
	QMap<ListDigraph::Node, QList<ListDigraph::Node> > predecessors; // Node predecessors (semi-)direct
	QMap<ListDigraph::Node, QList<ListDigraph::Node> > successors; // Node successors (semi-)direct
	
	QMap<ListDigraph::Node, QSet<int> > node2MachIDs; // Set of machines which can process the corresponding node
	
	bool acceptedWorse; // Define whether the VNS has accepted a worse solution
	
	QList<QPair<ListDigraph::Node, ListDigraph::Node> > origArcs; // Original arcs
	QList<bool> origConj; // Original conjunctive
	QList<double> origLen; // Original arcs' length
	QMap<ListDigraph::Node,Operation> origOpers; // Original operations
	
	bool timeBasedAcceptance; // Time-based acceptance in the acceptCondition()
	
public:

	VNSScheduler();
	VNSScheduler(VNSScheduler& orig);
	virtual ~VNSScheduler();

	virtual void init();

	virtual Clonable* clone();

	virtual void scheduleActions();

	void setTimeBasedAcceptance(const bool& on = true){
		timeBasedAcceptance = on;
	}
	
	/** Return the combined scheduler object. */
	CombinedScheduler& getCS() {
		return cs;
	}

	/** Return the LS object. */
	LocalSearchPM& getLS() {
		return ls;
	}
	
	void preservePM();
	void restorePM();
	
	/*********************  Iterative algorithm  ******************************/

	virtual void preprocessingActions();

	virtual void postprocessingActions();

	virtual void stepActions();

	virtual void assessActions();

	virtual bool acceptCondition();

	virtual void acceptActions();

	virtual void declineActions();

	virtual void stopActions();

	/**************************************************************************/
	
	
	/*********************  VNS-specific  *************************************/
	
	/** The shaking step of the algorithm. */
	virtual void shake();
	
	/** Perform randomly nOps operation swaps. */
	virtual void randomSwapOperations(const int &nOps, const bool& sameMachOnly = false);

	/** Randomly move the given number of times randomly selected operation to 
	 *  the randomly selected machines. */
	virtual void randomMoveOperations(const int &nOps, const bool& sameMachOnly = false);
	
	/** Get the solution encoding from the given PM. */
	virtual void solutionFromPM(ProcessModel& pm);
	
	/** For each node, find (semi-)direct predecessors which can be processed by the same machines. */
	virtual void findPredecessorsSameMachs();
	
	/** For each node, find (semi-)direct successors which can be processed by the same machines. */
	virtual void findSuccessorsSameMachs();
	
	/** For each node, find a set of machines able to process the corresponding operation. */
	virtual void findNode2MachIDs();
	
	/** Predecessor nodes which require the target machine. */
	virtual QList<ListDigraph::Node> findPredecessorsTargetMach(const int& mID, const ListDigraph::Node& newPred, const ListDigraph::Node& node);
	
	/** Successor nodes which require the target machine. */
	virtual QList<ListDigraph::Node> findSuccessorsTargetMach(const int& mID,  const ListDigraph::Node& newSucc, const ListDigraph::Node& node);
	
	/** Check whether the given node is in a cycle. */
	virtual bool isInCycle(const ListDigraph::Node& node);
	/**************************************************************************/
};

#endif	/* VNSSCHEDULER_H */

