/* 
 * File:   TGScheduler.h
 * Author: DrSobik
 *
 * Description:	Class TGSelection is the selection of the conjunctive arcs for 
 *				the operations of some tool group. It contains the value of the 
 *				local objective function, as well as the directed arcs, which 
 *				reflect the scheduling decisions.
 * 
 * Description:	Class TGScheduler is a class for scheduling the operations on 
 *				one of the tool groups in the job shop. It considers the 
 *				precedence constraint for the operation in the graph.
 * 
 * Description: Class TGFIFOScheduler is a simple scheduler for the operations,
 *				which does not change the sequence of the operations except 
 *				those violating the found precedence constraints.
 * 
 * 
 * Created on January 30, 2012, 9:53 AM
 */

#ifndef TGSCHEDULER_H
#define	TGSCHEDULER_H

#include "Resources.h"
#include "ProcessModel.h"

#include "Clonable.h"

#include "Objective.h"

//#include "SBHScheduler.h"

#include <lemon/list_graph.h>

using namespace lemon;


class SBHScheduler;

class TGScheduler : public Clonable {
protected:
	ProcessModel *pm;
	ToolGroup *tg;
	QList<ListDigraph::Node> *opnodes;
	QList<ListDigraph::Node> *terminals;
	QHash<int, QList<double> > *dloc;
	TGSelection *tgselection;

	QMap<ListDigraph::Node, QList<ListDigraph::Node> > predecessors; // Contains nodes, which precede the key node (direct or semidirect predecessors)
	QMap<ListDigraph::Node, QList<ListDigraph::Node> > successors; // Contains nodes, which succeed the key node (direct or semidirect successors)

	QList<ListDigraph::Arc> selectionArcs; // Arcs of the current selection which have been inserted into the PM
	QMap<ListDigraph::Node, Operation> prevNodeOper; // All data corresponding to the operations

public:

	QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > locD; // Local due dates in view of different terminal nodes

	SBHScheduler *sbhscheduler;

	QMap < ListDigraph::Node, QList< QList < ListDigraph::Node> > > *node2predST;

	TGScheduler();
	TGScheduler(TGScheduler& orig);
	virtual ~TGScheduler();

	virtual Clonable* clone();

	/** Schedule the given tool group. */
	virtual void schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection);

	/** Find all of the predecessors (direct and semidirect) of the specified node for processing of the tool group. */
	virtual void findNodePredecessors(ProcessModel &pm, const int tgid, ListDigraph::Node &opnode);

	/** Find all of the successors (direct and semidirect) of the specified node for processing of the tool group. */
	virtual void findNodeSuccessors(ProcessModel &pm, const int tgid, ListDigraph::Node &opnode);

	/** Run the subproblem scheduling algorithm. */
	virtual void run();

	/** Generate the arc selection for the current tool group. */
	virtual void getTGSelection();

	/** Calculate the local objective value for the subproblem. */
	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection &tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);

	/** Removes the current selection. */
	virtual void removeSelection();

	/** Inserts a new selection into the pm. */
	virtual void insertSelection(TGSelection& selection);

private:

};

class TGFIFOScheduler : public TGScheduler {
public:

	TGFIFOScheduler();
	TGFIFOScheduler(TGFIFOScheduler& orig);
	virtual ~TGFIFOScheduler();

	virtual Clonable* clone();

	virtual void run();

	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection &tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
};

/** Weighted operation due date priority rule for tool group scheduling */
class TGWEODScheduler : public TGScheduler {
public:

	TGWEODScheduler();
	TGWEODScheduler(TGWEODScheduler& orig);
	virtual ~TGWEODScheduler();

	virtual Clonable* clone();

	virtual void run();

	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection &tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
};

class TGATCScheduler : public TGScheduler {
protected:
	double kappa;

	double kstart;
	double kfinish;
	double kstep;

	bool kappaoptim;

	double ff; // The flow factor

	QMap<ListDigraph::Node, double> dOrd;
	QMap<ListDigraph::Node, double> pRemain;
	QMap<ListDigraph::Node, int> lenRemain;
	
	QList<ListDigraph::Node> topolOrdering;

public:

	TGATCScheduler();
	TGATCScheduler(TGATCScheduler& orig);
	virtual ~TGATCScheduler();

	virtual Clonable* clone();

	virtual void clear();
	
	inline void setKappaLimits(const double& start, const double& finish) {
		kstart = start;
		kfinish = finish;
	}

	inline void setKappaStep(const double& step) {
		kstep = step;
	}

	inline void kappaOptim(const bool& optimize = true) {
		kappaoptim = optimize;
	}

	inline void setKappa(const double& kappa) {
		kappaOptim(false);
		this->kappa = kappa;
	}

	void preparePM();

	virtual void run();

	double I(const double &t, const double &kappa, const double &p_avg, const ListDigraph::Node &node);

	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
};

class TGATCSchedulerPinSin : public TGScheduler {
protected:
	double kappa;

	double kstart;
	double kfinish;
	double kstep;

	bool kappaoptim;

	double ff; // The flow factor

	QMap<ListDigraph::Node, double> dOrd;
	QMap<ListDigraph::Node, double> pRemain;
	QMap<ListDigraph::Node, int> lenRemain;
	
	QList<ListDigraph::Node> topolOrdering;

public:

	TGATCSchedulerPinSin();
	TGATCSchedulerPinSin(TGATCSchedulerPinSin& orig);
	virtual ~TGATCSchedulerPinSin();

	virtual Clonable* clone();

	virtual void clear();
	
	inline void setKappaLimits(const double& start, const double& finish) {
		kstart = start;
		kfinish = finish;
	}

	inline void setKappaStep(const double& step) {
		kstep = step;
	}

	inline void kappaOptim(const bool& optimize = true) {
		kappaoptim = optimize;
	}

	inline void setKappa(const double& kappa) {
		kappaOptim(false);
		this->kappa = kappa;
	}

	void preparePM();

	virtual void run();

	double I(const double &t, const double &kappa, const double &p_avg, const ListDigraph::Node &node);

	virtual double localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc);
};

#endif	/* TGSCHEDULER_H */

