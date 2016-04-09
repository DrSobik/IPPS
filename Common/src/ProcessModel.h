/* 
 * File:   ProcessModel.h
 * Author: DrSobik
 *
 * Created on July 7, 2011, 12:36 PM
 * 
 * Description: ProcessModel class represents the process model. It contains
 *				information about all of the operation that have to be processed
 *				as well as information about precedence constraints between the
 *				operations.
 * 
 * Contained data:
 * 
 *				graph	-	contains the structure of the graph to model the 
 *							precedence constraints between the operations.
 * 
 *				ops		-	map of the nodes in the graph onto the set of
 *							the corresponding operations pointers.
 * 
 *				p		-	processing times of the operations. Defined after 
 *							assigning the operations (source node of the map) to
 *							the concrete machine in the production system. 
 *							Depends on the operation and on the machine.
 * 
 *				conjunctive
 *						-	an arc map with boolean values defining whether an 
 *							arc is a conjunctive arc of the graph. If "false" 
 *							then the arc is schedule-based
 */

#ifndef PROCESSMODEL_H
#define	PROCESSMODEL_H

#include <QTextStream>

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>
#include <lemon/bellman_ford.h>

#include <QQueue>

#include "Operation.h"
#include "Resources.h"

using namespace lemon;

class ProcessModel {
public:
    ListDigraph graph;

    ListDigraph::Node head;
    ListDigraph::Node tail;

    ListDigraph::NodeMap<Operation* > ops;
    ListDigraph::ArcMap<double> p;

    ListDigraph::ArcMap<bool> conjunctive;

private:
    bool saved;

    ListDigraph savedGraph;
    ListDigraph::Node savedHead;
    ListDigraph::Node savedTail;

    ListDigraph::NodeMap<Operation* > savedops;
    ListDigraph::ArcMap<double> savedp;

    ListDigraph::ArcMap<bool> savedconjunctive;


    QMap<ListDigraph::Node, Operation> savedOps; // Saved operation data
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > savedArcs; // Arcs of the graph which were saved previously
    QList<double> savedP; // Arc lengths of the previously saved arcs
    QList<bool> savedConjunctive; // Indicator of conjunctiveness of the previously saved arcs
	
public:

    ProcessModel();
    ProcessModel(const ProcessModel& orig);
    virtual ~ProcessModel();

    /** Mapping of operations IDs onto the set of nodes in the graph. */
    QHash<int, ListDigraph::Node> opID2Node();

    /** List of terminal nodes of the graph. */
    QList<ListDigraph::Node> terminals();

    /** Update heads of the operations in the graph. NOTE : Full update. */
    void updateHeads();

    /** Update heads of the given nodes. The nodes are presumed to be topologically sorted!!! */
    void updateHeads(QList<ListDigraph::Node>& topolOrdering);

    /** Update start times of the operations in the graph. NOTE : Full update. */
    void updateStartTimes();

    /** Update start times of the given nodes. The nodes are presumed to be topologically sorted!!! */
    void updateStartTimes(QList<ListDigraph::Node>& topolOrdering);

	/** 
	 * A function which updates the heads of the operations as the earliest time points to start operations.
	 * Based on the topological ordering of the graph
	 *  */
	
	void updateHeadsAndStartTimes(QList<ListDigraph::Node>& topolOrdering);
	
	/** 
	 * 
	 *	For each node in the topological ordering (starting from position startPos), update the direct predecessors which are on the critical path to this node.
	 *  
	 *	IMPORTANT!!! The nodes are assumed to be topologically ordered!!!
	 *  
	 *	IMPORTANT!!! If the node is already assigned to some machine and the availability time of the machine it larger than the 
	 *	largest completion time of the preceding nodes then none of them is situated on the critical path, since the CP is defined 
	 *  by the machine availability time. If all machines are available at time 0 or the operation is not assigned to any machine 
	 *  then the critical path is defined by the longest path in sense of arc lengths in the graph.
	 *	
	 *	IMPORTANT!!! It is assumed that the heads and the start times of the operations are correct (updated by updateHeads and updateStartTimes)
	 *	
	 *	 */
	QHash<int, QList<int> > updateCPPred(QList<ListDigraph::Node>& topolOrdering, const int& startPos = 0);
	
    /** Update the topological ordering in case arc (i,k) is inserted. IMPORTANT!!! The given topological ordering must be correct!!!*/
    void dynUpdateTopolSort(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &k);

    /** Copy the whole process model. */
    ProcessModel& operator=(const ProcessModel &other);

    /** Preserve the full state of the process model. The previous state will
     *  be destroyed or overwritten. */
    virtual void save();

    /** Restores the last saved state of the process model. */
    virtual void restore();

    /** Clear both the saved and the current data. IMPORTANT!!! This clears the graph as well! */
    virtual void clear();

    /** Clear only the current data. This clears the graph as well! */
    virtual void clearCurrent();

    /** Clear the saved data. This clears the graph as well! */
    virtual void clearSaved();

    /** Clear the scheduling relevant data. */
    virtual void clearSchedRelData();

    /** Check whether there is a conjunctive path from s to t in the graph of the process model. */
    bool conPathExists(const ListDigraph::Node &s, const ListDigraph::Node &t);

    /** List of nodes which are reachable from the set of given nodes. */
    QList<ListDigraph::Node> reachableFrom(const QList<ListDigraph::Node>& s);

    /** List of topologically sorted nodes which are reachable from the set of
     *  given nodes. */
    QList<ListDigraph::Node> topolSortReachableFrom(const QList<ListDigraph::Node>& s);

    /** General topological ordering of the graph. */
    QList<ListDigraph::Node> topolSort();

    /** Get the list of arcs starting at s and finishing at t. */
    QList<ListDigraph::Arc> arcs(const ListDigraph::Node& s, const ListDigraph::Node& t);

    /** Add arcs to the PM. Note : Can be useful for DTO. */
    QList<ListDigraph::Arc> addArcs(const QList<QPair<ListDigraph::Node, ListDigraph::Node> >& arcNodes);
    
    /** Remove arcs from the PM. Returns the number of removed arcs. */
    void removeArcs(const QList<ListDigraph::Arc>& arcsRem);

    /** Length of the longest paths between the nodes in the graph. Returns the start node -> end node, and the longest path's length.*/
    QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > longestPathsLen();

    /** Write the process model to the stream. */
    friend QTextStream& operator<<(QTextStream &out, ProcessModel&pm);

private:

};

class NodeWeightComparatorGreater {
public:

    explicit NodeWeightComparatorGreater(ProcessModel *pm) {
        this->pm = pm;
    }

    inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
        //if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;
        return pm->ops[n1]->w() > pm->ops[n2]->w();
    }


private:
    ProcessModel *pm;
};

class NodeWeightDueComparatorGreater {
public:

    explicit NodeWeightDueComparatorGreater(ProcessModel *pm) {
        this->pm = pm;
    }

    inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
        //if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;
        double left;
        double right;
        if (pm->ops[n1]->d() > 0) left = pm->ops[n1]->w() / pm->ops[n1]->d();
        else
            left = pm->ops[n1]->w() * Math::abs(pm->ops[n1]->d());

        if (pm->ops[n2]->d() > 0) right = pm->ops[n2]->w() / pm->ops[n2]->d();
        else
            right = pm->ops[n2]->w() * Math::abs(pm->ops[n2]->d());

        return left > right;
    }


private:
    ProcessModel *pm;
};

class NodeWSPTComparatorGreater {
public:

    explicit NodeWSPTComparatorGreater(ProcessModel *pm, Resources *rc) {
        this->pm = pm;
        this->rc = rc;
    }

    inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
        //if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;
        return pm->ops[n1]->w() / ((*rc)(pm->ops[n1]->toolID)).expectedProcTime(pm->ops[n1]) > pm->ops[n2]->w() / ((*rc)(pm->ops[n2]->toolID)).expectedProcTime(pm->ops[n2]);
    }


private:
    ProcessModel *pm;
    Resources *rc;
};

class NodeExpProcTimeComparatorGreater {
public:

    explicit NodeExpProcTimeComparatorGreater(ProcessModel *pm, Resources *rc) {
        this->pm = pm;
        this->rc = rc;
    }

    inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
        //if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;
        return ((*rc)(pm->ops[n1]->toolID)).expectedProcTime(pm->ops[n1]) > ((*rc)(pm->ops[n2]->toolID)).expectedProcTime(pm->ops[n2]);
    }


private:
    ProcessModel *pm;
    Resources *rc;
};

#endif	/* PROCESSMODEL_H */

