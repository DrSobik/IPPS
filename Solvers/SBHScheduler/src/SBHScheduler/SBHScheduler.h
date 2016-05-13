/* 
 * File:   SBHScheduler.h
 * Author: DrSobik
 *
 * Description: Class SBHScheduler implements the classical SBH scheduling with
 *				the main step of introducing the most critical machine or machine 
 *				group and the reoptimization step, which can (and will be) 
 *				overloaded to perform some local search technique.
 * 
 * Contained data:
 * 
 *				
 * 
 * 
 * Created on January 17, 2012, 1:15 PM
 */

#ifndef SBHSCHEDULER_H
#define	SBHSCHEDULER_H

#include <QtCore>

#include <stdio.h>
#include <stdexcept>

#include <lemon/bellman_ford.h>
#include <lemon/bits/graph_extender.h>
#include <lemon/list_graph.h>

#include "Scheduler.h"
#include "Resources.h"
#include "ProcessModel.h"
#include "Schedule.h"

#include "IterativeAlg.h"

#include "TGScheduler.h"
#include "TGVNSScheduler.h"
//#include "LocalSearchSM.h"
#include "LocalSearchPM.h"
#include "PriorityScheduler.h"
#include "CombinedScheduler.h"

#include "Objective.h"

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>
#include <lemon/bellman_ford.h>

using namespace lemon;

class SBHScheduler : public LSScheduler, public IterativeAlg {
public:

    TGScheduler *tgscheduler; // Tool group scheduler for generating initial schedule

    QList<ListDigraph::Node> terminals; // List of terminal nodes, which will be used for the SBH
    QHash<int, double> terminalopid2d; // Global due date for the terminal node
    QHash<int, double> terminalopid2w; // Weight for the terminal node

    QHash<int, double> _opid2preservedr; // Preserved ready times of the operations
    QHash<int, double> _opid2preservedd; // Preserved due dates of the operations
    QHash<int, double> _opid2preservedp; // Preserved processing times of the operations
    QHash<int, double> _opid2preserveds; // Preserved start times of the operations
    QHash<int, int> _opid2preservedm; // Preserved machine assignment of the operations

    QList<int> btnseq; // Sequence of bottlenecks corresponding to the nodes of the tree
    QSet<int> M0; // Set of scheduled tool groups corresponding to the current node of the search tree
    QSet<int> M; // Set of unscheduled tool groups corresponding to the current node of the search tree

    QHash<int, TGSelection> TG2Selection; // TG selections corresponding to different nodes of the search tree
    QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node> > > TG2Arcs; // Arcs created in the graph of the current node after inserting selections of the tool groups
    QHash<int, QList<ListDigraph::Node> > TG2Nodes; // Nodes which correspond to the tool groups for the given node of the search tree
    QHash<int, QList<double> > dloc; // Local due dates corresponding to the nodes of the search tree
    QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > locD; // Local due dates in view of different terminal nodes

    QList<ListDigraph::Node> topolOrdering; // Topological ordering of all nodes of the graph

    QMap < ListDigraph::Node, QList< QList < ListDigraph::Node> > > node2predST; // IMPORTANT!!! NOT IMPLEMENTED YET!!! Node 2 predecessors considering the same type of nodes. QMap is used because it can compare ListDigraph::Node

    ListDigraph::NodeMap<double> dOrd;
    ListDigraph::NodeMap<double> pRemain;
    ListDigraph::NodeMap<int> lenRemain;
    
	// Selection related
	QMap<int, QList<ListDigraph::Arc> > tgID2SelectionArcs; // Arcs of the current selections which have been inserted into the PM
	QMap<int, QMap<ListDigraph::Node, Operation> > tgID2PrevNodeOper; // All data corresponding to the operations
	
	QHash<int, double> tgID2Criticality; // Criticality measure of the machine groups
	
public:
    SBHScheduler();
    SBHScheduler(SBHScheduler& orig);
    virtual ~SBHScheduler();

    virtual Clonable* clone();

    /* ---------------------  Flow control relevant  ------------------------ */

    /** Initialize the scheduler. */
    virtual void init();

    /** Clear all data collected during the scheduling process. */
    virtual void clear();

    /** One step of the SBH heuristic: locate bottleneck, insert the bottleneck,
     *  perform reoptimization. */
    virtual void stepActions();

    /** No assessment needed for the SBH step. */
    virtual void assessActions() {
    }

    /** No acceptance condition needed for the SBH step. */
    virtual bool acceptCondition() {
        return true;
    }

    /** No accept actions needed for the SBH step. */
    virtual void acceptActions() {
    }

    /** No decline actions needed for the SBH step. */
    virtual void declineActions() {
    }

    /** The logical stopping condition of the algorithm.  Usually
     *	the set of the machines without selection of disjunctive arcs must be 
     *  empty. */
    virtual bool stopCondition();

    virtual void stopActions();

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions();

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions();

    /* ---------------------------------------------------------------------- */

    /* ----------------------  Scheduling relevant  ------------------------- */

    /** Here is the entry point to start the scheduler. */
    virtual void scheduleActions();

    virtual bool schedule(ProcessModel& pm, Resources& resources, Schedule& schedule);

    /** Find predecessors with the same tool group for every node in the graph. */
    virtual void findPredecessorsSameTG();

    /** Returns at most n priorized bottlenecks which correspond to the currently 
     *  considered node of the search tree. */
    virtual int bottleneckTG(const int& n);

    /** Insert the arcs corresponding to the operation processing sequences
     *  on the given machine/machine group into the graph of the process
     *  model corresponding to the given node of the search tree. */
    virtual void insertTGSelection(const int id);

    /** Remove the arcs corresponding to the operation processing sequences
     *  on the given machine/machine group from the graph of the process model
     *  model corresponding to the given node of the search tree. */
    virtual void removeTGSelection(const int id);

    /** Reoptimization in the current node of the search tree. */
    virtual void reoptimize(const int last_bottleneck_id);

    /** Simple SBH reoptimization in the current node of the search tree. */
    virtual void reoptimizeSimple(const int last_bottleneck_id);

    /** Reoptimization using the neighborhoods of Dauzere-Peres and Paulli
     *  for the case of parallel machines. */
    virtual void reoptimizePM(const int last_bottleneck_id);

    /** Calculate the head of the specified operation, i.e., the longest path 
     *  from the start node to the current operation. */
    void updateOperationHeads(const QList<ListDigraph::Node>& opnodes);

    /** Calculate the tails of the specified operation, i.e., makespan minus 
     *  the longest path from the operation to the k-th terminal node. */
    void updateOperationTails(const QList<ListDigraph::Node>& opnodes);

    /** Update operation tails for the specified process modes and the 
     *  specified nodes. */
    void updateOperationTails(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Schedule the tool group considering the current node of the search tree. */
    void scheduleTG(const int tgid);

    /** Schedule the tool group considering the current node of the search tree
     *  during the reoptimization process. */
    void reoptScheduleTG(const int tgid);

    /** Calculate local objective for the operations of the specified tool group 
     *  for the current node of the search tree. */
    virtual double calculateLocalObj(const int tgid);

    /** Calculate the general objective for the given node of the search tree. 
     *  Note: all operations must be scheduled. */
    virtual double calculateObj();

    /* ---------------------------------------------------------------------- */

    /* -----------  Preserving and restoring schedule data  ----------------- */

    /** Preserve the current ready times of the given operations. */
    void preserveOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved ready times of the given operations. */
    void restoreOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current due dates of the given operations. */
    void preserveOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved due dates of the given operations. */
    void restoreOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current processing times of the given operations. */
    void preserveOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved processing times of the given operations. */
    void restoreOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current start times of the given operations. */
    void preserveOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved start times of the given operations. */
    void restoreOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current machine assignment of the given operations. */
    void preserveOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved machine assignment of the given operations. */
    void restoreOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current schedule state of the given operations. */
    void preserveOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved schedule state of the given operations. */
    void restoreOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    void restoreInitialR(QList<ListDigraph::Node> &opnodes);

	/** Removes the current selection. */
	virtual void removeSelection(const int& tgID);
	
	/** Inserts a new selection into the pm. */
	virtual void insertSelection(const int& tgID);
	
	/** Reoptimize a specified machine group */
	void reoptimizeTG(const int& curTGID);
	
    /* ---------------------------------------------------------------------- */

    /** ---------------------  DEBUG  ----------------------------------- */
    void debugCheckReachability(const int& mid, ProcessModel& pm);

    bool reachable(ProcessModel& pm, const ListDigraph::Node& s, const ListDigraph::Node& t);
    /* ---------------------------------------------------------------------- */

private:

};





class SBHSchedulerVNS : public LSScheduler, public IterativeAlg {
public:

    TGScheduler *tgscheduler; // Tool group scheduler for generating initial schedule

    QList<ListDigraph::Node> terminals; // List of terminal nodes, which will be used for the SBH
    QHash<int, double> terminalopid2d; // Global due date for the terminal node
    QHash<int, double> terminalopid2w; // Weight for the terminal node

    QHash<int, double> _opid2preservedr; // Preserved ready times of the operations
    QHash<int, double> _opid2preservedd; // Preserved due dates of the operations
    QHash<int, double> _opid2preservedp; // Preserved processing times of the operations
    QHash<int, double> _opid2preserveds; // Preserved start times of the operations
    QHash<int, int> _opid2preservedm; // Preserved machine assignment of the operations

    QList<int> btnseq; // Sequence of bottlenecks corresponding to the nodes of the tree
    QSet<int> M0; // Set of scheduled tool groups corresponding to the current node of the search tree
    QSet<int> M; // Set of unscheduled tool groups corresponding to the current node of the search tree

    QHash<int, TGSelection> TG2Selection; // TG selections corresponding to different nodes of the search tree
    QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node> > > TG2Arcs; // Arcs created in the graph of the current node after inserting selections of the tool groups
    QHash<int, QList<ListDigraph::Node> > TG2Nodes; // Nodes which correspond to the tool groups for the given node of the search tree
    QHash<int, QList<double> > dloc; // Local due dates corresponding to the nodes of the search tree
    QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > locD; // Local due dates in view of different terminal nodes

    QList<ListDigraph::Node> topolOrdering; // Topological ordering of all nodes of the graph

    QMap < ListDigraph::Node, QList< QList < ListDigraph::Node> > > node2predST; // IMPORTANT!!! NOT IMPLEMENTED YET!!! Node 2 predecessors considering the same type of nodes. QMap is used because it can compare ListDigraph::Node

    ListDigraph::NodeMap<double> dOrd;
    ListDigraph::NodeMap<double> pRemain;
    ListDigraph::NodeMap<int> lenRemain;
    
	// Selection related
	QMap<int, QList<ListDigraph::Arc> > tgID2SelectionArcs; // Arcs of the current selections which have been inserted into the PM
	QMap<int, QMap<ListDigraph::Node, Operation> > tgID2PrevNodeOper; // All data corresponding to the operations
	
	QHash<int, double> tgID2Criticality; // Criticality measure of the machine groups
	
public:
    SBHSchedulerVNS();
    SBHSchedulerVNS(SBHSchedulerVNS& orig);
    virtual ~SBHSchedulerVNS();

    virtual Clonable* clone();

    /* ---------------------  Flow control relevant  ------------------------ */

    /** Initialize the scheduler. */
    virtual void init();

    /** Clear all data collected during the scheduling process. */
    virtual void clear();

    /** One step of the SBH heuristic: locate bottleneck, insert the bottleneck,
     *  perform reoptimization. */
    virtual void stepActions();

    /** No assessment needed for the SBH step. */
    virtual void assessActions() {
    }

    /** No acceptance condition needed for the SBH step. */
    virtual bool acceptCondition() {
        return true;
    }

    /** No accept actions needed for the SBH step. */
    virtual void acceptActions() {
    }

    /** No decline actions needed for the SBH step. */
    virtual void declineActions() {
    }

    /** The logical stopping condition of the algorithm.  Usually
     *	the set of the machines without selection of disjunctive arcs must be 
     *  empty. */
    virtual bool stopCondition();

    virtual void stopActions();

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions();

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions();

    /* ---------------------------------------------------------------------- */

    /* ----------------------  Scheduling relevant  ------------------------- */

    /** Here is the entry point to start the scheduler. */
    virtual void scheduleActions();

    virtual bool schedule(ProcessModel& pm, Resources& resources, Schedule& schedule);

    /** Find predecessors with the same tool group for every node in the graph. */
    virtual void findPredecessorsSameTG();

    /** Returns at most n priorized bottlenecks which correspond to the currently 
     *  considered node of the search tree. */
    virtual int bottleneckTG(const int& n);

    /** Insert the arcs corresponding to the operation processing sequences
     *  on the given machine/machine group into the graph of the process
     *  model corresponding to the given node of the search tree. */
    virtual void insertTGSelection(const int id);

    /** Remove the arcs corresponding to the operation processing sequences
     *  on the given machine/machine group from the graph of the process model
     *  model corresponding to the given node of the search tree. */
    virtual void removeTGSelection(const int id);

    /** Reoptimization in the current node of the search tree. */
    virtual void reoptimize(const int last_bottleneck_id);

    /** Simple SBH reoptimization in the current node of the search tree. */
    virtual void reoptimizeSimple(const int last_bottleneck_id);

    /** Reoptimization using the neighborhoods of Dauzere-Peres and Paulli
     *  for the case of parallel machines. */
    virtual void reoptimizePM(const int last_bottleneck_id);

    /** Calculate the head of the specified operation, i.e., the longest path 
     *  from the start node to the current operation. */
    void updateOperationHeads(const QList<ListDigraph::Node>& opnodes);

    /** Calculate the tails of the specified operation, i.e., makespan minus 
     *  the longest path from the operation to the k-th terminal node. */
    void updateOperationTails(const QList<ListDigraph::Node>& opnodes);

    /** Update operation tails for the specified process modes and the 
     *  specified nodes. */
    void updateOperationTails(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Schedule the tool group considering the current node of the search tree. */
    void scheduleTG(const int tgid);

    /** Schedule the tool group considering the current node of the search tree
     *  during the reoptimization process. */
    void reoptScheduleTG(const int tgid);

    /** Calculate local objective for the operations of the specified tool group 
     *  for the current node of the search tree. */
    virtual double calculateLocalObj(const int tgid);

    /** Calculate the general objective for the given node of the search tree. 
     *  Note: all operations must be scheduled. */
    virtual double calculateObj();

    /* ---------------------------------------------------------------------- */

    /* -----------  Preserving and restoring schedule data  ----------------- */

    /** Preserve the current ready times of the given operations. */
    void preserveOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved ready times of the given operations. */
    void restoreOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current due dates of the given operations. */
    void preserveOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved due dates of the given operations. */
    void restoreOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current processing times of the given operations. */
    void preserveOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved processing times of the given operations. */
    void restoreOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current start times of the given operations. */
    void preserveOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved start times of the given operations. */
    void restoreOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current machine assignment of the given operations. */
    void preserveOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved machine assignment of the given operations. */
    void restoreOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current schedule state of the given operations. */
    void preserveOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved schedule state of the given operations. */
    void restoreOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    void restoreInitialR(QList<ListDigraph::Node> &opnodes);

	/** Removes the current selection. */
	virtual void removeSelection(const int& tgID);
	
	/** Inserts a new selection into the pm. */
	virtual void insertSelection(const int& tgID);
	
	/** Reoptimize a specified machine group */
	void reoptimizeTG(const int& curTGID);
	
    /* ---------------------------------------------------------------------- */

    /** ---------------------  DEBUG  ----------------------------------- */
    void debugCheckReachability(const int& mid, ProcessModel& pm);

    bool reachable(ProcessModel& pm, const ListDigraph::Node& s, const ListDigraph::Node& t);
    /* ---------------------------------------------------------------------- */

private:

};







class SBHSchedulerPinSin : public LSScheduler, public IterativeAlg {
public:

    TGScheduler *tgscheduler; // Tool group scheduler for generating initial schedule

    QList<ListDigraph::Node> terminals; // List of terminal nodes, which will be used for the SBH
    QHash<int, double> terminalopid2d; // Global due date for the terminal node
    QHash<int, double> terminalopid2w; // Weight for the terminal node

    QHash<int, double> _opid2preservedr; // Preserved ready times of the operations
    QHash<int, double> _opid2preservedd; // Preserved due dates of the operations
    QHash<int, double> _opid2preservedp; // Preserved processing times of the operations
    QHash<int, double> _opid2preserveds; // Preserved start times of the operations
    QHash<int, int> _opid2preservedm; // Preserved machine assignment of the operations

    QList<int> btnseq; // Sequence of bottlenecks corresponding to the nodes of the tree
    QSet<int> M0; // Set of scheduled tool groups corresponding to the current node of the search tree
    QSet<int> M; // Set of unscheduled tool groups corresponding to the current node of the search tree

    QHash<int, TGSelection> TG2Selection; // TG selections corresponding to different nodes of the search tree
    QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node> > > TG2Arcs; // Arcs created in the graph of the current node after inserting selections of the tool groups
    QHash<int, QList<ListDigraph::Node> > TG2Nodes; // Nodes which correspond to the tool groups for the given node of the search tree
    QHash<int, QList<double> > dloc; // Local due dates corresponding to the nodes of the search tree
    QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > locD; // Local due dates in view of different terminal nodes

    QList<ListDigraph::Node> topolOrdering; // Topological ordering of all nodes of the graph

    QMap < ListDigraph::Node, QList< QList < ListDigraph::Node> > > node2predST; // IMPORTANT!!! NOT IMPLEMENTED YET!!! Node 2 predecessors considering the same type of nodes. QMap is used because it can compare ListDigraph::Node

    ListDigraph::NodeMap<double> dOrd;
    ListDigraph::NodeMap<double> pRemain;
    ListDigraph::NodeMap<int> lenRemain;
    
	// Selection related
	QMap<int, QList<ListDigraph::Arc> > tgID2SelectionArcs; // Arcs of the current selections which have been inserted into the PM
	QMap<int, QMap<ListDigraph::Node, Operation> > tgID2PrevNodeOper; // All data corresponding to the operations
	
	QHash<int, double> tgID2Criticality; // Criticality measure of the machine groups
	
public:
    SBHSchedulerPinSin();
    SBHSchedulerPinSin(SBHSchedulerPinSin& orig);
    virtual ~SBHSchedulerPinSin();

    virtual Clonable* clone();

    /* ---------------------  Flow control relevant  ------------------------ */

    /** Initialize the scheduler. */
    virtual void init();

    /** Clear all data collected during the scheduling process. */
    virtual void clear();

    /** One step of the SBH heuristic: locate bottleneck, insert the bottleneck,
     *  perform reoptimization. */
    virtual void stepActions();

    /** No assessment needed for the SBH step. */
    virtual void assessActions() {
    }

    /** No acceptance condition needed for the SBH step. */
    virtual bool acceptCondition() {
        return true;
    }

    /** No accept actions needed for the SBH step. */
    virtual void acceptActions() {
    }

    /** No decline actions needed for the SBH step. */
    virtual void declineActions() {
    }

    /** The logical stopping condition of the algorithm.  Usually
     *	the set of the machines without selection of disjunctive arcs must be 
     *  empty. */
    virtual bool stopCondition();

    virtual void stopActions();

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions();

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions();

    /* ---------------------------------------------------------------------- */

    /* ----------------------  Scheduling relevant  ------------------------- */

    /** Here is the entry point to start the scheduler. */
    virtual void scheduleActions();

    virtual bool schedule(ProcessModel& pm, Resources& resources, Schedule& schedule);

    /** Find predecessors with the same tool group for every node in the graph. */
    virtual void findPredecessorsSameTG();

    /** Returns at most n priorized bottlenecks which correspond to the currently 
     *  considered node of the search tree. */
    virtual int bottleneckTG(const int& n);

    /** Insert the arcs corresponding to the operation processing sequences
     *  on the given machine/machine group into the graph of the process
     *  model corresponding to the given node of the search tree. */
    virtual void insertTGSelection(const int id);

    /** Remove the arcs corresponding to the operation processing sequences
     *  on the given machine/machine group from the graph of the process model
     *  model corresponding to the given node of the search tree. */
    virtual void removeTGSelection(const int id);

    /** Reoptimization in the current node of the search tree. */
    virtual void reoptimize(const int last_bottleneck_id);

    /** Simple SBH reoptimization in the current node of the search tree. */
    virtual void reoptimizeSimple(const int last_bottleneck_id);

    /** Reoptimization using the neighborhoods of Dauzere-Peres and Paulli
     *  for the case of parallel machines. */
    virtual void reoptimizePM(const int last_bottleneck_id);

    /** Calculate the head of the specified operation, i.e., the longest path 
     *  from the start node to the current operation. */
    void updateOperationHeads(const QList<ListDigraph::Node>& opnodes);

    /** Calculate the tails of the specified operation, i.e., makespan minus 
     *  the longest path from the operation to the k-th terminal node. */
    void updateOperationTails(const QList<ListDigraph::Node>& opnodes);

    /** Update operation tails for the specified process modes and the 
     *  specified nodes. */
    void updateOperationTails(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Schedule the tool group considering the current node of the search tree. */
    void scheduleTG(const int tgid);

    /** Schedule the tool group considering the current node of the search tree
     *  during the reoptimization process. */
    void reoptScheduleTG(const int tgid);

    /** Calculate local objective for the operations of the specified tool group 
     *  for the current node of the search tree. */
    virtual double calculateLocalObj(const int tgid);

    /** Calculate the general objective for the given node of the search tree. 
     *  Note: all operations must be scheduled. */
    virtual double calculateObj();

    /* ---------------------------------------------------------------------- */

    /* -----------  Preserving and restoring schedule data  ----------------- */

    /** Preserve the current ready times of the given operations. */
    void preserveOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved ready times of the given operations. */
    void restoreOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current due dates of the given operations. */
    void preserveOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved due dates of the given operations. */
    void restoreOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current processing times of the given operations. */
    void preserveOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved processing times of the given operations. */
    void restoreOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current start times of the given operations. */
    void preserveOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved start times of the given operations. */
    void restoreOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current machine assignment of the given operations. */
    void preserveOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved machine assignment of the given operations. */
    void restoreOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    /** Preserve the current schedule state of the given operations. */
    void preserveOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);
    /** Restore the last preserved schedule state of the given operations. */
    void restoreOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes);

    void restoreInitialR(QList<ListDigraph::Node> &opnodes);

	/** Removes the current selection. */
	virtual void removeSelection(const int& tgID);
	
	/** Inserts a new selection into the pm. */
	virtual void insertSelection(const int& tgID);
	
	/** Reoptimize a specified machine group */
	void reoptimizeTG(const int& curTGID);
	
    /* ---------------------------------------------------------------------- */

    /** ---------------------  DEBUG  ----------------------------------- */
    void debugCheckReachability(const int& mid, ProcessModel& pm);

    bool reachable(ProcessModel& pm, const ListDigraph::Node& s, const ListDigraph::Node& t);
    /* ---------------------------------------------------------------------- */

private:

};

#endif	/* SBHSCHEDULER_H */

