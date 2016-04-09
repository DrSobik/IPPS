/* 
 * File:   LocalSearch.h
 * Author: DrSobik
 * 
 * Description : Class LocalSearchSM performs local search in the graph in order
 *		 to optimize the corresponding objective function. A techniques
 *		 based on swapping critical arcs in the graph described by Mati, Perez and Lahlou
 *		 is used. IMPORTANT!!! This technique is only capable of moving some 
 *		 operation on the single machine and not from one machine to another one.
 *                               
 *
 * Created on May 22, 2012, 10:50 AM
 */

#ifndef LOCALSEARCHSM_H
#define	LOCALSEARCHSM_H

#include "Resources.h"
#include "ProcessModel.h"

#include "IterativeAlg.h"
#include "SBHScheduler.h"

#include "DebugExt.h"

#include <lemon/list_graph.h>
#include <lemon/path.h>
#include <lemon/connectivity.h>
#include <lemon/bellman_ford.h>

using namespace lemon;
using namespace Common;

class LocalSearchSM : public IterativeAlg {
private:
	ProcessModel *pm; // Current process model

	QList<ListDigraph::Node> terminals; // Terminal nodes in the graph

	double bestobjimprov; // Objective value before the step of the algorithm during the improvement phase
	double prevobjimprov; //  Previous objective value during the improvement phase

	double bestobjinterm; // Objective value before the step of the algorithm during the improvement phase
	double prevobjinterm; //  Previous objective value during the improvement phase

	double curobjimprov; // Objective value after the step of the algorithm during the improvement phase
	double curobjinterm; // Objective value after the step of the algorithm during the intermediate phase

	ListDigraph::Arc reversed; // The arc which has been reversed in the at the last step of the algorithm

	bool acceptedworse; // Indicates whether a worse solution has been accepted

	int nisteps; // Number of non-improvement steps
	
	/*
	enum {
		IMPROV, INTERM
	} lsmode; // Optimization mode of the algorithm
	 */

	double alpha;

	TWT objimprov; // Primary optimization criterion
	TL objinterm; // Secondary optimization criterion

public:
	LocalSearchSM();
	virtual ~LocalSearchSM();

	/** Set the process model for the local search. */
	void setPM(ProcessModel *pm);

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

	/* ---------------------------------------------------------------------- */

	/* ------------------------  Search  relevant  -------------------------- */

	/** Find longest path from the start node of the graph to the given node. */
	Path<ListDigraph> longestPath(const ListDigraph::Node &node);

	/** Select a critical arc to swap in the graph. */
	ListDigraph::Arc selectArcToRevert(const Path<ListDigraph> &cpath);

	/** Revert the arc in the graph of the process model. Returns the new 
	 *  reverted arc. */
	ListDigraph::Arc reverseArc(const ListDigraph::Arc &carc);

	/** Select some terminal node to which the critical path will be searched.
	 *  The probability that the terminal is selected is proportional to its'
	 *  contribution to the criterion. */
	ListDigraph::Node selectTerminalContrib(QList<ListDigraph::Node> &terminals);

	/** Select some terminal node to which the critical path will be searched.
	 *  The terminal is selected randomly. */
	ListDigraph::Node selectTerminalRnd(QList<ListDigraph::Node> &terminals);

	/** Perform random diversification of the solution in order to avoid local
	 *  optima. */
	void diversify();

	/* ---------------------------------------------------------------------- */
};

#endif	/* LOCALSEARCHSM_H */
