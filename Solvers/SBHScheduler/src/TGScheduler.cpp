/* 
 * File:   TGScheduler.cpp
 * Author: DrSobik
 * 
 * Created on January 30, 2012, 9:53 AM
 */

#include <QtCore/qqueue.h>
#include <QtCore/qpair.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdatetime.h>

#include "TGScheduler.h"

#include "SBHScheduler.h"

/* -------------------  Tool group scheduler  ------------------------------- */

TGScheduler::TGScheduler() {
}

TGScheduler::TGScheduler(TGScheduler& orig) : Clonable() {
	pm = orig.pm;

	tg = orig.tg;
	dloc = orig.dloc;
	sbhscheduler = orig.sbhscheduler;

	terminals = orig.terminals;
	tgselection = orig.tgselection;

	opnodes = orig.opnodes;
	node2predST = orig.node2predST;
	locD = orig.locD;

	predecessors = orig.predecessors;
	successors = orig.successors;

}

TGScheduler::~TGScheduler() {
}

Clonable* TGScheduler::clone() {
	return new TGScheduler(*this);
}

void TGScheduler::schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection) {
	/** Algorithm:
	 * 
	 * 1. Find the precedence constraints for the operation nodes to be scheduled:
	 * 1.1 For every node:
	 * 1.1.1 Iterate over all possible predecessors of the node until for every 
	 *		 path from the node we find another node of the same tool group or
	 *		 the start node. Thus, we find all direct predecessors 
	 *		 (or semi-direct predecessors) for every node.
	 * 2. Schedule the operations taking into account the calculated precedence
	 *	  constraints.
	 * 
	 */

	// IMPORTANT!!! Preserve the process model before the scheduling
	//Debugger::info << "TGScheduler::schedule : Preserving the PM..." << ENDL;

	QTime tgeneral;
	QTime tsucpred;
	//QTime tsaverestore;

	//int msgeneral;
	//int mssucpred;
	//int mssaverestore = 0;;

	tgeneral.start();

	//tsaverestore.start();
	//	pm.save();
	//mssaverestore+= tsaverestore.elapsed();
	//Debugger::info << "TGScheduler::schedule : Done preserving the PM." << ENDL;

	// Initialize the local pointers
	this->pm = &pm;
	this->tg = &tg;
	this->opnodes = &opnodes;
	this->terminals = &terminals;
	this->dloc = &dloc;
	this->tgselection = &tgselection;

	// Define the precedence constraints (inclusive DPC) for every node, which has to be scheduled
	//Debugger::info << "Searching precedence constraints ..." << ENDL;
	//Debugger::info << "Number of nodes: " << opnodes.size() << ENDL;

	predecessors.clear();
	successors.clear();

	tsucpred.start();

	QTextStream out(stdout);

	for (int i = 0; i < opnodes.size(); i++) {
		//Debugger::info << "Searching node predecessors ..." << ENDL;
		findNodePredecessors(pm, tg.ID, opnodes[i]);
		//Debugger::info << "Done searching node predecessors." << ENDL;
		//Debugger::info << "Searching node successors ..." << ENDL;
		findNodeSuccessors(pm, tg.ID, opnodes[i]);
		//Debugger::info << "Done searching node successors." << ENDL;

		/*
		out << "Direct predecessors of node : " << this->pm->ops[opnodes[i]]->ID << endl;
		out << node2predST->value(opnodes[i]).size() << endl;
		for (int j = 0; j < node2predST->value(opnodes[i])[0].size(); j++) {
			out << this->pm->ops[node2predST->value(opnodes[i])[0][j]]->ID << ",";
		}
		out << endl;

		out << "All predecessors of node :" << endl;
		for (int j = 0; j < predecessors[opnodes[i]].size(); j++) {
			out << this->pm->ops[predecessors[opnodes[i]][j]]->ID << ",";
		}
		out << endl;
		getchar();
		 */
	}

	//mssucpred = tsucpred.elapsed();


	//Debugger::info << "Done searching precedence constraints." << ENDL;

	// Schedule the operations on the tool group under consideration of the found precedence constraints
	//Debugger::info << "Scheduling on the tool group ..." << ENDL;
	run();

	// IMPORTANT!!! Restore the process model after scheduling
	//Debugger::info << "TGScheduler::schedule : Restoring the PM..." << ENDL;
	//tsaverestore.start();
	//	pm.restore();
	//mssaverestore+= tsaverestore.elapsed();

	//msgeneral = tgeneral.elapsed();

	//Debugger::info << "TGScheduler::schedule : Ratio of suc/pred time to general time for subproblem solving : " << double(mssucpred) / double(msgeneral) << ENDL;
	//Debugger::info << "TGScheduler::schedule : Ratio of save/restore time to general time for subproblem solving : " << double(mssaverestore) / double(msgeneral) << ENDL;
	//getchar();

	//Debugger::info << "TGScheduler::schedule : Done restoring the PM." << ENDL;


	//Debugger::info << "Done scheduling on the tool group." << ENDL;
}

void TGScheduler::findNodePredecessors(ProcessModel &pm, const int tgid, ListDigraph::Node &opnode) {
	QTextStream out(stdout);

	QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
	ListDigraph::Node curnode;
	ListDigraph::NodeMap<bool> q_contains(pm.graph, false); // Consider revising for optimization

	// Initialize the queue with the opnodes' predecessors
	for (ListDigraph::InArcIt iait(pm.graph, opnode); iait != INVALID; ++iait) {
		q.enqueue(pm.graph.source(iait));
	}

	while (!q.empty()) {
		curnode = q.dequeue();

		// Iterate over the graph predecessors of the node. 
		// If the node belongs to the same tool group then add it to the predecessors, otherwise enqueue its predecessors for further processing
		if (pm.ops[curnode]->toolID == tgid && pm.ops[curnode]->ID > 0) {
			if (!predecessors[opnode].contains(curnode)) {
				predecessors[opnode].append(curnode);
			}
		} else {
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (!q_contains[pm.graph.source(iait)]) {
					q.enqueue(pm.graph.source(iait));
					q_contains[pm.graph.source(iait)] = true;
				}
			}
		}
	}

	// ########################  DEBUG  ########################################

	// Check whether there are no duplicate nodes between predecessors and successors

	/*
	for (int j = 0; j < predecessors[opnode].size(); j++) {
		if (predecessors[opnode].count(predecessors[opnode][j]) > 1) {
			out << "Node: " << *pm.ops[opnode] << endl;
			out << "Duplicate: " << pm.ops[predecessors[opnode][j]]->ID << endl;
			for (int k = 0; k < predecessors[opnode].size(); k++) {
				out << pm.ops[predecessors[opnode][k]]->ID << ", ";
			}
			out << endl;
			Debugger::err << "TGScheduler::findNodePredecessors : Duplicate nodes between predecessors!" << ENDL;
		}
	}
	 */

	// #########################################################################
}

void TGScheduler::findNodeSuccessors(ProcessModel &pm, const int tgid, ListDigraph::Node &opnode) {
	QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
	ListDigraph::Node curnode;
	ListDigraph::NodeMap<bool> q_contains(pm.graph, false); // Consider revising for optimization

	// Initialize the queue with the opnodes' successors
	for (ListDigraph::OutArcIt oait(pm.graph, opnode); oait != INVALID; ++oait) {
		q.enqueue(pm.graph.target(oait));
		q_contains[pm.graph.target(oait)] = true;
	}

	while (!q.empty()) {
		curnode = q.dequeue();

		//Debugger::info << "Queue size: " << q.size() << ENDL;

		// Iterate over the graph successors of the node. 
		// If the node belongs to the same tool group then add it to the precedences, otherwise enqueue its successors for further processing
		if (pm.ops[curnode]->toolID == tgid && pm.ops[curnode]->ID > 0) {
			if (!successors[opnode].contains(curnode)) {
				successors[opnode].append(curnode);
			}
		} else {
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				if (!q_contains[pm.graph.target(oait)]) {
					q.enqueue(pm.graph.target(oait));
					q_contains[pm.graph.target(oait)] = true;
				}
			}
		}
	}
}

void TGScheduler::run() {
	Debugger::err << "TGScheduler::run() : not implemented!" << ENDL;
}

void TGScheduler::getTGSelection() {
	QTextStream out(stdout);

	//out << "TGScheduler::getTGSelection : Running... " << endl;
	//out << "TGScheduler::getTGSelection : TWT : " << TWT()(*pm) << endl;

#ifdef DEBUG

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm->graph.target(curArc);

			if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 1) {
				Debugger::err << "TGScheduler::getTGSelection : Too many disjunctive arcs between the nodes!!!" << ENDL;
			}
		}

	}
#endif	

	// Clear the previous selection related data
	selectionArcs.clear();
	prevNodeOper.clear();

	QHash<int, ListDigraph::Node> opid2node;

	opid2node.reserve(opnodes->size());
	for (int i = 0; i < opnodes->size(); i++) {
		opid2node[pm->ops[opnodes->at(i)]->ID] = opnodes->at(i);
	}

	QList<Machine*> tgmachines = tg->machines();
	QList<Operation*> machops;
	tgselection->selection.clear();
	tgselection->selectionOpIDs.clear();

	//for (QMap<ListDigraph::Node, Operation>::iterator iter = tgselection->opNode2SchedOps.begin(); iter != tgselection->opNode2SchedOps.end(); iter++) {
	//delete iter.value();
	//}
	tgselection->opNode2SchedOps.clear();
	tgselection->opID2SchedOp.clear();


	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm->ops[curNode]);

		if (curOp.toolID != tg->ID || curOp.ID < 0 || curOp.toolID == 0) continue;

		// Iterate over the disjunctive successors
		int nDisjunctiveOutArcs = 0;
		int nDisjunctiveInArcs = 0;

		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {

			if (!pm->conjunctive[oait]) { // This arc should be in the selection

				nDisjunctiveOutArcs++;

				ListDigraph::Node curSuccNode = pm->graph.target(oait);
				Operation& curSuccOp = (Operation&) *(pm->ops[curSuccNode]);

				if (curSuccOp.toolID != tg->ID) {
					out << "TGID of the successive operation : " << curSuccOp.toolID << endl;
					out << "Current TGID : " << tg->ID << endl;
					Debugger::err << "TGScheduler::getTGSelection : Adding a wrong selection arc!!!" << ENDL;
				}

				tgselection->selection.append(QPair<ListDigraph::Node, ListDigraph::Node>(curNode, curSuccNode));
				tgselection->selectionOpIDs.append(QPair<int, int>(curOp.ID, curSuccOp.ID));

				if (!tgselection->opNode2SchedOps.contains(curNode)) {
					tgselection->opNode2SchedOps.insert(curNode, Operation(curOp));
				}

				if (!tgselection->opNode2SchedOps.contains(curSuccNode)) {
					tgselection->opNode2SchedOps.insert(curSuccNode, Operation(curSuccOp));
				}

				if (!tgselection->opID2SchedOp.contains(curOp.ID)) {
					tgselection->opID2SchedOp.insert(curOp.ID, Operation(curOp));
				}

				if (!tgselection->opID2SchedOp.contains(curSuccOp.ID)) {
					tgselection->opID2SchedOp.insert(curSuccOp.ID, Operation(curSuccOp));
				}

			} // Check disjunctive arc

		} // Iterating over the successors

		for (ListDigraph::InArcIt iait(pm->graph, curNode); iait != INVALID; ++iait) {

			if (!pm->conjunctive[iait]) { // This arc should be in the selection

				nDisjunctiveInArcs++;

			}

		} // Iterating over the predecessors

		if (nDisjunctiveInArcs == 0 && nDisjunctiveOutArcs == 0) { // There are no incoming/outgoing disjunctive arcs -> this operation is single on the machine
			tgselection->opNode2SchedOps.insert(curNode, Operation(curOp));
			tgselection->opID2SchedOp.insert(curOp.ID, Operation(curOp));
		} // Adding the single operation

	} // Iterating over the nodes


	return;
}

double TGScheduler::localObj(ProcessModel& /*pm*/, QList<ListDigraph::Node>& /*opnodes*/, TGSelection& /*tgselection*/, QList<ListDigraph::Node>& /*terminals*/, QHash<int, QList<double> >& /*dloc*/) {
	Debugger::err << "TGScheduler::localObj() : not implemented!" << ENDL;

	//SBHTWTLocalObj lobj;

	//return lobj(pm, opnodes, terminals, dloc);

	return Math::MIN_DOUBLE;
}

void TGScheduler::removeSelection() {
	// Remove all arcs which have previously been inserted and CHANGE THE OPERATIONS

	if (prevNodeOper.size() == 0) { // In case there is nothing to remove
		selectionArcs.clear();
		return;
	}

	// Restore the operations
	for (QMap<ListDigraph::Node, Operation>::iterator iter = prevNodeOper.begin(); iter != prevNodeOper.end(); iter++) {
		ListDigraph::Node curNode = iter.key();
		Operation& curOper = iter.value();

		// Restore the operation
		*(pm->ops[curNode]) = curOper;

		// Set the correct outgoing arc lengths
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			pm->p[oait] = -curOper.p();
		}

	}

	// Remove the selection arcs
	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];

		// Remove the arc
		pm->graph.erase(curArc);

	}

	QList<ListDigraph::Node> ts = pm->topolSort();
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);

	// Clear the previous data
	selectionArcs.clear();
	prevNodeOper.clear();

}

void TGScheduler::insertSelection(TGSelection& selection) {

	// Remove the previous selection
	removeSelection();

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsToAdd = selection.selection;
	selectionArcs = pm->addArcs(arcsToAdd);

	QList<ListDigraph::Node> opsToChange = selection.opNode2SchedOps.keys();

	//cout << "Added arcs : " << arcsAdded.size() << endl;

	for (int i = 0; i < opsToChange.size(); i++) { // Insert the nodes to be changed. Important!!! Inserting arcs only is not enough since there can be only one operation on a machine

		ListDigraph::Node curNode = opsToChange[i];

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		// Preserve the previous operation data
		prevNodeOper[curNode] = *(pm->ops[curNode]);

		// Set the new operation data according to the selection
		*(pm->ops[curNode]) = curOper;

		// Iterate over the outgoing arcs of the current node
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			pm->p[oait] = -curOper.p();
		}

	}

	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];
		ListDigraph::Node curNode = pm->graph.source(curArc);

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		pm->conjunctive[curArc] = false;

		pm->p[curArc] = -curOper.p();

	}

	QList<ListDigraph::Node> ts = pm->topolSort();
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);

}

/* -------------------------------------------------------------------------- */

class NodeReleaseComparatorLess {
public:

	explicit NodeReleaseComparatorLess(ProcessModel *pm) {
		this->pm = pm;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		//if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;
		return pm->ops[n1]->r() < pm->ops[n2]->r();
	}


private:
	ProcessModel *pm;
};

class NodeWeightedReleaseComparatorGreater {
public:

	explicit NodeWeightedReleaseComparatorGreater(ProcessModel *pm) {
		this->pm = pm;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		//if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;

		if (pm->ops[n1]->r() == 0.0) return true;
		if (pm->ops[n2]->r() == 0.0) return false;

		return pm->ops[n1]->w() / pm->ops[n1]->r() > pm->ops[n2]->w() / pm->ops[n2]->r();
	}


private:
	ProcessModel *pm;
};

class NodeDueComparatorLess {
public:

	explicit NodeDueComparatorLess(ProcessModel *pm) {
		this->pm = pm;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		//if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;

		return pm->ops[n1]->d() < pm->ops[n2]->d();
	}


private:
	ProcessModel *pm;
};

class NodeWeightedDueComparatorGreater {
public:

	explicit NodeWeightedDueComparatorGreater(ProcessModel *pm) {
		this->pm = pm;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		//if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;

		if (pm->ops[n1]->d() < 0.0 && pm->ops[n2]->d() < 0.0) return pm->ops[n1]->w() * pm->ops[n1]->d() < pm->ops[n2]->w() * pm->ops[n2]->d();

		if (pm->ops[n1]->d() < 0.0 && pm->ops[n2]->d() == 0.0) return true;
		if (pm->ops[n1]->d() == 0.0 && pm->ops[n2]->d() < 0.0) return false;

		return pm->ops[n1]->w() / pm->ops[n1]->d() > pm->ops[n2]->w() / pm->ops[n2]->d();
	}


private:
	ProcessModel *pm;
};

class NodeUTWTGreater {
public:

	explicit NodeUTWTGreater(ProcessModel* pm, QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> >& locD) {
		this->pm = pm;
		this->locD = &locD;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		UTWT utwt;

		QList<ListDigraph::Node> nl;

		nl.append(n1);
		double utwt1 = utwt(*pm, nl, *locD);

		nl.clear();
		nl.append(n2);
		double utwt2 = utwt(*pm, nl, *locD);

		return utwt1 > utwt2;
	}


private:
	ProcessModel* pm;
	QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> >* locD;
};

class NodeMODLess {
public:

	explicit NodeMODLess(ProcessModel *pm) {
		this->pm = pm;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		//if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;

		double d1 = Math::max(pm->ops[n1]->d(), pm->ops[n1]->r() + pm->ops[n1]->p());
		double d2 = Math::max(pm->ops[n2]->d(), pm->ops[n2]->r() + pm->ops[n2]->p());

		return d1 < d2;
	}


private:
	ProcessModel *pm;
};

class NodeWMODGreater {
public:

	explicit NodeWMODGreater(ProcessModel *pm) {
		this->pm = pm;
	}

	inline bool operator() (const ListDigraph::Node &n1, const ListDigraph::Node &n2) {
		//if (pm->ops[n1]->toolID != pm->ops[n2]->toolID) return true;

		double d1 = Math::max(pm->ops[n1]->d(), pm->ops[n1]->r() + pm->ops[n1]->p());
		double d2 = Math::max(pm->ops[n2]->d(), pm->ops[n2]->r() + pm->ops[n2]->p());
		double w1 = pm->ops[n1]->w();
		double w2 = pm->ops[n2]->w();

		if (d1 < 0.0 && d2 < 0.0) return w1 * d1 < w2 * d2;

		if (d1 < 0.0 && d2 == 0.0) return true;
		if (d1 == 0.0 && d2 < 0.0) return false;

		return w1 / d1 > w2 / d2;
	}


private:
	ProcessModel *pm;
};

/* -------------------  Tool group FIFO scheduler  -------------------------- */

TGFIFOScheduler::TGFIFOScheduler() {

}

TGFIFOScheduler::TGFIFOScheduler(TGFIFOScheduler& orig) : TGScheduler(orig) {

}

TGFIFOScheduler::~TGFIFOScheduler() {

}

Clonable* TGFIFOScheduler::clone() {
	return new TGFIFOScheduler(*this);
}

void TGFIFOScheduler::run() {
	/** Algorithm:
	 * 
	 *	1. For every node:
	 *  1.1. Calculate the number of unscheduled predecessors
	 * 
	 *	2. While not all operation scheduled:
	 *  2.1. Find first unscheduled operation, predecessors of which have 
	 *		 already been scheduled and which is not scheduled itself
	 *  2.2. Update the ready time of the operation as the maximum of its own
	 *		 ready time and the completion times of its predecessors
	 *  2.3. Schedule the operation
	 *  2.4. Subtract 1 from number of unscheduled predecessors for all 
	 *		 successors of the node
	 * 
	 *	3. Generate the tool group selection.
	 * 
	 */

	//QTextStream out(stdout);

	//Debugger::info << "Running tool group scheduling algorithm ..." << ENDL;

	// Sort the available nodes
	//NodeMODLess cmp(pm);
	//NodeWMODGreater cmp(pm);
	//NodeWeightedDueComparatorGreater cmp(pm);
	//NodeUTWTGreater cmp(pm, locD);
	//NodeDueComparatorLess cmp(pm);
	NodeReleaseComparatorLess cmp(pm);
	//NodeWeightedReleaseComparatorGreater cmp(pm);
	//NodeWeightComparatorGreater cmp(&(*pm));
	//NodeWSPTComparatorGreater cmp(&(*pm), &(*rc));
	//NodeExpProcTimeComparatorGreater cmp(&pm, &rc);
	qSort(opnodes->begin(), opnodes->end(), cmp);

	// For every node calculate the number of unscheduled predecessors
	QMap<ListDigraph::Node, int> unsched_pred;

	for (int i = 0; i < opnodes->size(); i++) {
		pm->ops[opnodes->at(i)]->machID = -1;
		unsched_pred[opnodes->at(i)] = predecessors[opnodes->at(i)].size();
	}

	// Schedule all operation one by one updating their availability
	int ops_unscheduled = opnodes->size();

	//Debugger::info << "Scheduling the operations ..." << ENDL;

	QTextStream out(stdout);

	//out << *pm << endl;

	//for (int j = 0; j < opnodes->size(); j++) {
	//	Debugger::info << "opnode (" << pm->ops[opnodes->at(j)]->OID << ":" << pm->ops[opnodes->at(j)]->ID << ")" << ENDL;
	//}
	//getchar();

	QHash<int, ListDigraph::Node> opID2Node;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		opID2Node[pm->ops[nit]->ID] = nit;
	}

	QList<ListDigraph::Node> ts = pm->topolSort();

	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	out << "TGWEODScheduler::run : TWT before scheduling : " << TWT()(*pm) << endl;


	out << "TGWEODScheduler::run : Preserving the PM's state... " << endl;
	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;
	QList<ListDigraph::Arc> insArcs;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *(pm->ops[curNode]);
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm->conjunctive[curArc];
		arc2PrevP[curArc] = pm->p[curArc];
	}





	out << "TGWEODScheduler::run : Initializing the machine group... " << endl;

	out << "TGWEODScheduler::run : Initializing the machine group ID : " << tg->ID << endl;

	tg->init();

	out << "TGWEODScheduler::run : Running the main loop... " << endl;

#ifdef DEBUG

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;
		Operation& curOp = (Operation&) *(pm->ops[curStartNode]);

		if (curOp.toolID != tg->ID) continue; // Omit other machine groups

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm->graph.target(curArc);

			if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
				Debugger::err << "TGWEODScheduler::run : Too many disjunctive arcs between the nodes before running the main loop!!!" << ENDL;
			}

		}

	}
#endif

	out << "TGWEODScheduler::run : Number of opnodes to schedule : " << opnodes->size() << endl;

	QMap < ListDigraph::Node, bool> opnodeScheduled;
	for (int i = 0; i < opnodes->size(); i++) {
		ListDigraph::Node curNode = opnodes->at(i);
		opnodeScheduled[curNode] = false;
	}

	while (ops_unscheduled > 0) {

		// Sort the nodes according to some criterion
		qSort(opnodes->begin(), opnodes->end(), cmp);

		//Debugger::info << "Unscheduled operations:  " << ops_unscheduled << ENDL;

		for (int i = 0; i < opnodes->size(); i++) {

			ListDigraph::Node curNode = opnodes->at(i);
			Operation& curOp = (Operation&) *(pm->ops[curNode]);

			if (curOp.ID > 0 && (curOp.machID < 0) && unsched_pred[curNode] == 0 && !opnodeScheduled[curNode]) {

				//out << "Scheduling operation : " << curOp << endl;

				// IMPORTANT!!! Mark the opnode as scheduled
				opnodeScheduled[curNode] = true;

				// Update the ready time of the operation
				double largestPredC = 0.0;
				for (int j = 0; j < predecessors[curNode].size(); j++) {

					ListDigraph::Node predNode = predecessors[curNode][j];
					Operation& predOp = (Operation&) *(pm->ops[predNode]);

					largestPredC = Math::max(largestPredC, predOp.c());

				}

				pm->ops[curNode]->r(Math::max(curOp.r(), largestPredC));

				// Schedule the operation
				Machine &m = tg->earliestToFinish(&curOp);

				ListDigraph::Node prevMachNode = INVALID;

				if (m.operations.size() == 0) {
					prevMachNode = INVALID;
				} else {
					int lastMachOperID = m.operations.last()->ID;
					prevMachNode = opID2Node[lastMachOperID];
				}

				//out << m << endl;
				m << &curOp;

				// The operation has been scheduled - > reflect this in the PM
				if (prevMachNode == INVALID) { // No arcs are inserted

				} else { // Insert an arc

					ListDigraph::Arc curArc = pm->graph.addArc(prevMachNode, curNode);

					insArcs.append(curArc);

					pm->conjunctive[curArc] = false;
					pm->p[curArc] = -pm->ops[prevMachNode]->p();

					// Update the topological ordering of the graph
					pm->dynUpdateTopolSort(ts, prevMachNode, curNode);

				}

				// Update the lengths of all outgoing arcs of the current node
				for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
					pm->p[oait] = -pm->ops[curNode]->p();
				}

				// Update the release and the start times
				pm->updateHeads(ts);
				pm->updateStartTimes(ts);

				// Subtract 1 from number of unscheduled predecessors for all successors of the node
				for (int j = 0; j < successors[curNode].size(); j++) {
					ListDigraph::Node curSucc = successors[curNode][j];
					unsched_pred[curSucc] -= 1;
				}

				// Decrease the number of unscheduled operations
				ops_unscheduled--;

				//Debugger::info << "Done scheduling unscheduled operation." << ENDL;
				break;

			} // Checking whether the opnode can be scheduled



		} // Iterating over the nodes

#ifdef DEBUG

		// Check whether there are multiple disjunctinve arcs between any two nodes
		for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
			ListDigraph::Node curStartNode = nit;

			QMap<ListDigraph::Node, int> targetNode2Numarcs;

			for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
				ListDigraph::Arc curArc = oait;
				ListDigraph::Node curEndNode = pm->graph.target(curArc);

				if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

				if (targetNode2Numarcs[curEndNode] > 1) {
					Debugger::err << "TGWEODScheduler::run : Too many disjunctive arcs between the nodes before scheduling!!!" << ENDL;
				}
			}

		}
#endif	

	} // Check whether all nodes are scheduled

	//Debugger::info << "Done scheduling the operations." << ENDL;

	pm->updateHeads();
	pm->updateStartTimes();
	out << "TGWEODScheduler::run : TWT AFTER after scheduling : " << TWT()(*pm) << endl;


#ifdef DEBUG

	if (!dag(pm->graph)) {
		Debugger::err << "TGWEODScheduler::run : The graph is not DAG after scheduling!!!" << ENDL;
	}

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm->graph.target(curArc);

			if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 1) {
				Debugger::err << "TGWEODScheduler::run : Too many disjunctive arcs between the nodes after scheduling!!!" << ENDL;
			}
		}

	}
#endif	

	QSet<int> scheduledOperations1;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		if (pm->ops[nit]->machID > 0 && pm->ops[nit]->toolID == tg->ID) {
			scheduledOperations1.insert(pm->ops[nit]->ID);
		}
	}

	//getchar();

	//out << *pm << endl;
	//getchar();

	// Generate the tool group selection
	getTGSelection();

	out << "Arcs should be in the current selection : " << insArcs.size() << endl;
	out << "Arcs actually in the current selection : " << tgselection->selection.size() << endl;
	out << "TGWEODScheduler::run : Nodes in the new selection : " << tgselection->opID2SchedOp.size() << endl;

	// Restore the PM's state

	//	if (tgselection->selection.size() != insArcs.size()) {
	//		out << "Arcs in the selection : " << tgselection->selection.size() << endl;
	//		out << "Arcs inserted previously : " << insArcs.size() << endl;
	//		Debugger::err << "TGWEODScheduler::run : Wrong number of selection arcs!!!" << ENDL;
	//	}

	// Remove the selection arcs
	for (int i = 0; i < insArcs.size(); i++) {
		ListDigraph::Arc curArc = insArcs[i];
		pm->graph.erase(curArc);
	}

	// Restore the operation data
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		*(pm->ops[curNode]) = node2PrevOper[curNode];
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm->conjunctive[curArc] = arc2PrevConj[curArc];
		pm->p[curArc] = arc2PrevP[curArc];
	}






	pm->updateHeads();
	pm->updateStartTimes();
	out << "TGWEODScheduler::run : TWT AFTER restoring the PM : " << TWT()(*pm) << endl;
	//getchar();


	insertSelection(*tgselection);

	QSet<int> scheduledOperations2;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		if (pm->ops[nit]->machID > 0 && pm->ops[nit]->toolID == tg->ID) {
			scheduledOperations2.insert(pm->ops[nit]->ID);
		}
	}

	QList<int> diff = (scheduledOperations1 - scheduledOperations2).toList();

	//out << *pm << endl;

	out << scheduledOperations1.size() << " , " << scheduledOperations2.size() << endl;
	for (int i = 0; i < diff.size(); i++) {
		out << diff[i] << endl;
	}

	//getchar();
	out << "TGWEODScheduler::run : TWT after getting the new selection : " << TWT()(*pm) << endl;
	removeSelection();

	// Calculate the local objective for the current selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
	//Debugger::info << "Done running tool group scheduling algorithm." << ENDL;
}

double TGFIFOScheduler::localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection&, QList<ListDigraph::Node>&, QHash<int, QList<double> >&) {

	UTWT utwt;

	double res = utwt(pm, opnodes, locD);

	return res;
}

/* -------------------------------------------------------------------------- */

/* -------------------  Tool group WEOD scheduler  -------------------------- */

TGWEODScheduler::TGWEODScheduler() {

}

TGWEODScheduler::TGWEODScheduler(TGWEODScheduler& orig) : TGScheduler(orig) {

}

TGWEODScheduler::~TGWEODScheduler() {

}

Clonable* TGWEODScheduler::clone() {
	return new TGWEODScheduler(*this);
}

void TGWEODScheduler::run() {
	/** Algorithm:
	 * 
	 *	1. For every node:
	 *  1.1. Calculate the number of unscheduled predecessors
	 * 
	 *	2. While not all operation scheduled:
	 *  2.1. Find first unscheduled operation, predecessors of which have 
	 *		 already been scheduled and which is not scheduled itself
	 *  2.2. Update the ready time of the operation as the maximum of its own
	 *		 ready time and the completion times of its predecessors
	 *  2.3. Schedule the operation
	 *  2.4. Subtract 1 from number of unscheduled predecessors for all 
	 *		 successors of the node
	 * 
	 *	3. Generate the tool group selection.
	 * 
	 */

	//QTextStream out(stdout);

	//Debugger::info << "Running tool group scheduling algorithm ..." << ENDL;

	// Sort the available nodes
	//NodeMODLess cmp(pm);
	//NodeWMODGreater cmp(pm);
	NodeWeightedDueComparatorGreater cmp(pm);
	//NodeUTWTGreater cmp(pm, locD);
	//NodeDueComparatorLess cmp(pm);
	//NodeReleaseComparatorLess cmp(pm);
	//NodeWeightedReleaseComparatorGreater cmp(pm);
	//NodeWeightComparatorGreater cmp(&(*pm));
	//NodeWSPTComparatorGreater cmp(&(*pm), &(*rc));
	//NodeExpProcTimeComparatorGreater cmp(&pm, &rc);
	qSort(opnodes->begin(), opnodes->end(), cmp);

	// For every node calculate the number of unscheduled predecessors
	QMap<ListDigraph::Node, int> unsched_pred;

	for (int i = 0; i < opnodes->size(); i++) {
		pm->ops[opnodes->at(i)]->machID = -1;
		unsched_pred[opnodes->at(i)] = predecessors[opnodes->at(i)].size();
	}

	// Schedule all operation one by one updating their availability
	int ops_unscheduled = opnodes->size();

	//Debugger::info << "Scheduling the operations ..." << ENDL;

	QTextStream out(stdout);

	//out << *pm << endl;

	//for (int j = 0; j < opnodes->size(); j++) {
	//	Debugger::info << "opnode (" << pm->ops[opnodes->at(j)]->OID << ":" << pm->ops[opnodes->at(j)]->ID << ")" << ENDL;
	//}
	//getchar();

	QHash<int, ListDigraph::Node> opID2Node;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		opID2Node[pm->ops[nit]->ID] = nit;
	}

	QList<ListDigraph::Node> ts = pm->topolSort();

	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	out << "TGWEODScheduler::run : TWT before scheduling : " << TWT()(*pm) << endl;


	out << "TGWEODScheduler::run : Preserving the PM's state... " << endl;
	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;
	QList<ListDigraph::Arc> insArcs;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *(pm->ops[curNode]);
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm->conjunctive[curArc];
		arc2PrevP[curArc] = pm->p[curArc];
	}





	out << "TGWEODScheduler::run : Initializing the machine group... " << endl;

	out << "TGWEODScheduler::run : Initializing the machine group ID : " << tg->ID << endl;

	tg->init();

	out << "TGWEODScheduler::run : Running the main loop... " << endl;

#ifdef DEBUG

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;
		Operation& curOp = (Operation&) *(pm->ops[curStartNode]);

		if (curOp.toolID != tg->ID) continue; // Omit other machine groups

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm->graph.target(curArc);

			if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
				Debugger::err << "TGWEODScheduler::run : Too many disjunctive arcs between the nodes before running the main loop!!!" << ENDL;
			}

		}

	}
#endif

	out << "TGWEODScheduler::run : Number of opnodes to schedule : " << opnodes->size() << endl;

	QList<bool> opnodeScheduled;
	for (int i = 0; i < opnodes->size(); i++) {
		opnodeScheduled.append(false);
	}

	while (ops_unscheduled > 0) {

		//Debugger::info << "Unscheduled operations:  " << ops_unscheduled << ENDL;

		/*
		if (ops_unscheduled == 15) {
				for (int j = 0; j < opnodes->size(); j++) {
						Debugger::info << "opnode (" << pm->ops[opnodes->at(j)]->OID << ":" << pm->ops[opnodes->at(j)]->ID << ")" << ENDL;
						Debugger::info << "machID = " << pm->ops[opnodes->at(j)]->machID << ENDL;
						for (int k = 0; k < predecessors[opnodes->at(j)].size(); k++) {
								Debugger::info << "predecessor (" << pm->ops[predecessors[opnodes->at(j)][k]]->OID << ":" << pm->ops[predecessors[opnodes->at(j)][k]]->ID << ")" << ENDL;
						}
				}
		}
		 */

		for (int i = 0; i < opnodes->size(); i++) {

			ListDigraph::Node curNode = opnodes->at(i);
			Operation& curOp = (Operation&) *(pm->ops[curNode]);

			if (curOp.ID > 0 && (curOp.machID < 0) && unsched_pred[curNode] == 0 && !opnodeScheduled[i]) {

				//out << "Scheduling operation : " << curOp << endl;

				// IMPORTANT!!! Mark the opnode as scheduled
				opnodeScheduled[i] = true;

				// Update the ready time of the operation
				double largestPredC = 0.0;
				for (int j = 0; j < predecessors[curNode].size(); j++) {

					ListDigraph::Node predNode = predecessors[curNode][j];
					Operation& predOp = (Operation&) *(pm->ops[predNode]);

					largestPredC = Math::max(largestPredC, predOp.c());

				}

				pm->ops[curNode]->r(Math::max(curOp.r(), largestPredC));

				// Schedule the operation
				Machine &m = tg->earliestToFinish(&curOp);

				ListDigraph::Node prevMachNode = INVALID;

				if (m.operations.size() == 0) {
					prevMachNode = INVALID;
				} else {
					int lastMachOperID = m.operations.last()->ID;
					prevMachNode = opID2Node[lastMachOperID];
				}

				//out << m << endl;
				m << &curOp;

				// The operation has been scheduled - > reflect this in the PM
				if (prevMachNode == INVALID) { // No arcs are inserted

				} else { // Insert an arc

					ListDigraph::Arc curArc = pm->graph.addArc(prevMachNode, curNode);

					insArcs.append(curArc);

					pm->conjunctive[curArc] = false;
					pm->p[curArc] = -pm->ops[prevMachNode]->p();

					// Update the topological ordering of the graph
					pm->dynUpdateTopolSort(ts, prevMachNode, curNode);

				}

				// Update the lengths of all outgoing arcs of the current node
				for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
					pm->p[oait] = -pm->ops[curNode]->p();
				}

				// Update the release and the start times
				pm->updateHeads(ts);
				pm->updateStartTimes(ts);

				// Subtract 1 from number of unscheduled predecessors for all successors of the node
				for (int j = 0; j < successors[curNode].size(); j++) {
					ListDigraph::Node curSucc = successors[curNode][j];
					unsched_pred[curSucc] -= 1;
				}

				// Decrease the number of unscheduled operations
				ops_unscheduled--;

				//Debugger::info << "Done scheduling unscheduled operation." << ENDL;
				break;

			} // Checking whether the opnode can be scheduled

		} // Iterating over the nodes

#ifdef DEBUG

		// Check whether there are multiple disjunctinve arcs between any two nodes
		for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
			ListDigraph::Node curStartNode = nit;

			QMap<ListDigraph::Node, int> targetNode2Numarcs;

			for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
				ListDigraph::Arc curArc = oait;
				ListDigraph::Node curEndNode = pm->graph.target(curArc);

				if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

				if (targetNode2Numarcs[curEndNode] > 1) {
					Debugger::err << "TGWEODScheduler::run : Too many disjunctive arcs between the nodes before scheduling!!!" << ENDL;
				}
			}

		}
#endif	

	} // Check whether all nodes are scheduled

	//Debugger::info << "Done scheduling the operations." << ENDL;

	pm->updateHeads();
	pm->updateStartTimes();
	out << "TGWEODScheduler::run : TWT AFTER after scheduling : " << TWT()(*pm) << endl;


#ifdef DEBUG

	if (!dag(pm->graph)) {
		Debugger::err << "TGWEODScheduler::run : The graph is not DAG after scheduling!!!" << ENDL;
	}

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm->graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm->graph.target(curArc);

			if (!pm->conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 1) {
				Debugger::err << "TGWEODScheduler::run : Too many disjunctive arcs between the nodes after scheduling!!!" << ENDL;
			}
		}

	}
#endif	

	QSet<int> scheduledOperations1;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		if (pm->ops[nit]->machID > 0 && pm->ops[nit]->toolID == tg->ID) {
			scheduledOperations1.insert(pm->ops[nit]->ID);
		}
	}

	//getchar();

	//out << *pm << endl;
	//getchar();

	// Generate the tool group selection
	getTGSelection();

	out << "Arcs should be in the current selection : " << insArcs.size() << endl;
	out << "Arcs actually in the current selection : " << tgselection->selection.size() << endl;
	out << "TGWEODScheduler::run : Nodes in the new selection : " << tgselection->opID2SchedOp.size() << endl;

	// Restore the PM's state

	//	if (tgselection->selection.size() != insArcs.size()) {
	//		out << "Arcs in the selection : " << tgselection->selection.size() << endl;
	//		out << "Arcs inserted previously : " << insArcs.size() << endl;
	//		Debugger::err << "TGWEODScheduler::run : Wrong number of selection arcs!!!" << ENDL;
	//	}

	// Remove the selection arcs
	for (int i = 0; i < insArcs.size(); i++) {
		ListDigraph::Arc curArc = insArcs[i];
		pm->graph.erase(curArc);
	}

	// Restore the operation data
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		*(pm->ops[curNode]) = node2PrevOper[curNode];
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm->conjunctive[curArc] = arc2PrevConj[curArc];
		pm->p[curArc] = arc2PrevP[curArc];
	}






	pm->updateHeads();
	pm->updateStartTimes();
	out << "TGWEODScheduler::run : TWT AFTER restoring the PM : " << TWT()(*pm) << endl;
	//getchar();


	insertSelection(*tgselection);

	QSet<int> scheduledOperations2;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		if (pm->ops[nit]->machID > 0 && pm->ops[nit]->toolID == tg->ID) {
			scheduledOperations2.insert(pm->ops[nit]->ID);
		}
	}

	QList<int> diff = (scheduledOperations1 - scheduledOperations2).toList();

	//out << *pm << endl;

	out << scheduledOperations1.size() << " , " << scheduledOperations2.size() << endl;
	for (int i = 0; i < diff.size(); i++) {
		out << diff[i] << endl;
	}

	//getchar();
	out << "TGWEODScheduler::run : TWT after getting the new selection : " << TWT()(*pm) << endl;
	removeSelection();

	// Calculate the local objective for the current selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
	//Debugger::info << "Done running tool group scheduling algorithm." << ENDL;
}

double TGWEODScheduler::localObj(ProcessModel &pm, QList<ListDigraph::Node> &, TGSelection& selection, QList<ListDigraph::Node>&, QHash<int, QList<double> >&) {
	QTextStream out(stdout);

	UTWT utwt;
	TWT twt;

	double res = 0.0;

	int curTGID = selection.tgID;
	cout << "TGWEODScheduler::localObj : Calculating objective for TG : " << curTGID << endl;
	cout << "Arcs in the selection : " << selection.selection.size() << endl;

	insertSelection(selection);

	//out << pm << endl;

	res = twt(pm); //*/ utwt(pm, opnodes, locD);

	removeSelection();

	cout << "TWT : " << res << endl;

	//getchar();

	return res;
}

/* -------------------------------------------------------------------------- */

/* ---------------  Tool group Extended ATC scheduler  ---------------------- */

TGATCScheduler::TGATCScheduler() {
	kappa = 2.0;
	kstart = 0.01;
	kfinish = 4.0;
	kstep = 0.2;
	kappaoptim = true;
}

TGATCScheduler::TGATCScheduler(TGATCScheduler& orig) : TGScheduler(orig) {
	this->kappa = orig.kappa;
	this->kstart = orig.kstart;
	this->kfinish = orig.kfinish;
	this->kstep = orig.kstep;
	this->kappaoptim = orig.kappaoptim;

	this->dOrd = orig.dOrd;
	this->pRemain = orig.pRemain;
	this->lenRemain = orig.lenRemain;
	
	this->ff = orig.ff;
}

TGATCScheduler::~TGATCScheduler() {

}

Clonable* TGATCScheduler::clone() {
	return new TGATCScheduler(*this);
}

void TGATCScheduler::clear() {

	dOrd.clear();
	pRemain.clear();
	lenRemain.clear();

	kappa = kstart;

	topolOrdering.clear();

}

void TGATCScheduler::preparePM() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// Get the topological ordering of the graph
	topolOrdering = pm->topolSort();

	// Update the ready times of the operations
	pm->updateHeads(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	QList<ListDigraph::Node> terminals = pm->terminals();

	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		lenRemain[nit] = 0;
	}

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		lenRemain[terminals[i]] = 1;
		pRemain[terminals[i]] = 0.0;
		dOrd[terminals[i]] = pm->ops[terminals[i]]->d();

		for (ListDigraph::InArcIt iait(pm->graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm->graph.source(iait);
			q.enqueue(curpred);

			lenRemain[curpred] = 2;
			pRemain[curpred] = pm->ops[curpred]->p() + 0.0; // 0.0 - for the terminal node
			dOrd[curpred] = pm->ops[terminals[i]]->d();
		}

		ordNodes.clear();
		QSet<int> insertedNodeISs;
		while (!q.empty()) {
			curnode = q.dequeue();

			insertedNodeISs.remove(pm->graph.id(curnode));

			// Save the node 
			ordNodes.append(curnode);

			// Find the largest number of the operations to be processed after the current node (including the current node)
			ListDigraph::Node longerSucc = INVALID;
			double maxP = -1.0;
			for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
				if (pRemain[pm->graph.target(oait)] > maxP) {
					maxP = pRemain[pm->graph.target(oait)];
					longerSucc = pm->graph.target(oait);
				}
			}
			//Debugger::info << "Found : " << pm.ops[longerSucc]->ID << ENDL;
			//getchar();
			if (longerSucc == INVALID) {
				Debugger::err << "TGATCScheduler::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
			}
			pRemain[curnode] = pRemain[longerSucc] + pm->ops[curnode]->p();
			lenRemain[curnode] = lenRemain[longerSucc] + 1;
			dOrd[curnode] = dOrd[longerSucc]; // Due date of the order

			//Debugger::info << pm.ops[longerSucc]->ID << " : " << d[longerSucc] << ENDL;
			//getchar();

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
				curpred = pm->graph.source(iait);

				// Push the current predecessor into the queue, provided it is not in there already
				if (!insertedNodeISs.contains(pm->graph.id(curnode))) {

					q.enqueue(curpred);

					insertedNodeISs.insert(pm->graph.id(curnode));
				}

			}

		}

		//getchar();
	}

	// Initialize the start times of the operations
	pm->updateStartTimes(topolOrdering);

}

void TGATCScheduler::run() {
	/** Algorithm:
	 * 
	 *	1. For every node:
	 *  1.1. Calculate the number of unscheduled predecessors
	 * 
	 *  2. For different values of parameter kappa:
	 *	2. While not all operation scheduled:
	 *  2.1. Find first unscheduled operation, predecessors of which have 
	 *		 already been scheduled and which is not scheduled itself
	 *  2.2. Update the ready time of the operation as the maximum of its own
	 *		 ready time and the completion times of its predecessors
	 *  2.3. Schedule the operation
	 *  2.4. Subtract 1 from number of unscheduled predecessors for all 
	 *		 successors of the node
	 * 
	 *	3. Generate the tool group selection.
	 * 
	 */

	QTextStream out(stdout);

	out << "TGATCScheduler::run..." << endl;

	clear();

#ifdef DEBUG	
	if (!dag(pm->graph)) {
		Debugger::err << "TGATCScheduler::run : Cycles in the graph!!!" << ENDL;
	}
#endif	

	preparePM();

	out << "TGATCScheduler:: Prepared the PM " << endl;

	QHash<int, ListDigraph::Node> opID2Node;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		opID2Node[pm->ops[nit]->ID] = nit;
	}

	TGSelection prevtgselection;
	TGSelection besttgselection;

	if (kappaoptim) {
		kappa = kstart;
	}

	double best_loc_obj = Math::MAX_DOUBLE;
	int best_kappa = kappa;

	// Topological ordering of the graph
	QList<ListDigraph::Node> ts = pm->topolSort();

	// The actual state of the PM
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);

	// Save the state of the graph
	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;
	QList<ListDigraph::Arc> insArcs;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *(pm->ops[curNode]);
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm->conjunctive[curArc];
		arc2PrevP[curArc] = pm->p[curArc];
	}

	out << "TGATCScheduler::run : TWT before scheduling the TG (partial schedule) : " << TWT()(*pm) << endl;

	do {

		ts = pm->topolSort();

		// For every node calculate the number of predecessors
		QMap<ListDigraph::Node, int> unsched_pred;

		for (int i = 0; i < opnodes->size(); i++) {

			ListDigraph::Node curNode = opnodes->at(i);

			pm->ops[curNode]->machID = -1;
			//out << "TGATCScheduler::run : Unscheduling operation " << pm->ops[opnodes->at(i)]->ID << endl;

			unsched_pred[curNode] = predecessors[curNode].size();
		}

		// Schedule all operation one by one updating their availability
		int ops_unscheduled = opnodes->size();

		tg->init();

		out << "TGATCScheduler::run : Scheduling operations ... " << endl;
		
		while (ops_unscheduled > 0) {

			// Iterate over the unassigned nodes and select the one with the greatest priority index
			ListDigraph::Node next_node = INVALID;
			double prio = -1.0;
			double p_avg = 0.0;
			int n_unassigned_avail = 0;

			// Recalculate p_avg: average processing time for the not assigned but already available operations
			for (int i = 0; i < opnodes->size(); i++) {

				ListDigraph::Node curNode = opnodes->at(i);
				Operation& curOp = *(pm->ops[curNode]);

				// Is the node not assigned?

				if (curOp.ID > 0 && curOp.machID < 0 && unsched_pred[curNode] == 0) { // This operation is not assigned to any machine

					Machine& curMach = tg->earliestToFinish(&curOp);
					double curP = curMach.procTime(&curOp);

					p_avg += curP; //pRemain[curNode];

					n_unassigned_avail++;

				} // Accounting the unscheduled operations

			} // Iterating over nodes
			p_avg /= double(n_unassigned_avail);

			for (int i = 0; i < opnodes->size(); i++) {

				ListDigraph::Node curNode = opnodes->at(i);
				Operation& curOp = *(pm->ops[curNode]);

				// Is the node not assigned and available for scheduling
				if (curOp.ID > 0 && curOp.machID < 0 && unsched_pred[curNode] == 0) { // Unassigned available node

					//out << pm->ops[opnodes->at(i)]->ID << " : " << I(tg->earliestToFinish(pm->ops[opnodes->at(i)]).time(), kappa, p_avg, opnodes->at(i)) << endl;

					// Is the priority index of the node greater?

					double curPrioVal = I(tg->earliestToFinish(&curOp).time(), kappa, p_avg, curNode);

					if (prio < curPrioVal) {

						prio = curPrioVal;

						next_node = curNode;

					}

				} // Taking into account the unassigned available nodes

			} // Iterating over the nodes

			if (next_node == INVALID) {
				out << "Operations unscheduled : " << ops_unscheduled << endl;
				int ctr = 0;
				for (int i = 0; i < opnodes->size(); i++) {
					if (pm->ops[opnodes->at(i)]->ID > 0 && (pm->ops[opnodes->at(i)]->machID < 0)) {
						ctr++;
						out << ctr << " : " << *pm->ops[opnodes->at(i)] << endl;
						out << "Unscheduled predecessors : " << unsched_pred[opnodes->at(i)] << endl;

						for (int j = 0; j < predecessors[opnodes->at(i)].size(); j++) {
							if (pm->ops[predecessors[opnodes->at(i)][j]]->machID >= 0) {
								out << *pm->ops[predecessors[opnodes->at(i)][j]] << endl;
								Debugger::err << "Scheduled predecessor is marked as unscheduled!" << ENDL;
							}
						}
					}
				}
				Debugger::err << "TGATCScheduler::run : No node for scheduling found although unscheduled operations are still remaining!" << ENDL;
			}

			//out << "TGATCScheduler::run : Trying to schedule operation " << *pm->ops[next_node] << endl;

			Operation& curOp = *(pm->ops[next_node]);

			// Schedule the operation
			Machine &m = tg->earliestToFinish(&curOp);

			ListDigraph::Node prevMachNode = INVALID; // The last node on the assigned to the machine

			if (m.operations.size() == 0) {
				prevMachNode = INVALID;
			} else {
				int lastMachOperID = m.operations.last()->ID;
				prevMachNode = opID2Node[lastMachOperID];
			}

			if (curOp.machID > 0) {
				out << *pm << endl;
				Debugger::err << "TGATCScheduler::run : Trying to schedule already scheduled operation : " << curOp.ID << " !!!" << ENDL;
			}

			//out << m << endl;
			m << &curOp;

			// The operation has been scheduled - > reflect this in the PM
			if (prevMachNode == INVALID) { // No arcs are inserted

			} else { // Insert an arc

				ListDigraph::Arc curArc = pm->graph.addArc(prevMachNode, next_node);

				insArcs.append(curArc);

				pm->conjunctive[curArc] = false;
				pm->p[curArc] = -pm->ops[prevMachNode]->p();

				// Update the topological ordering of the graph
				pm->dynUpdateTopolSort(ts, prevMachNode, next_node);

			}

			// Update the lengths of all outgoing arcs of the current node
			for (ListDigraph::OutArcIt oait(pm->graph, next_node); oait != INVALID; ++oait) {
				pm->p[oait] = -pm->ops[next_node]->p();
			}

			// Update the release and the start times
			pm->updateHeads(ts);
			pm->updateStartTimes(ts);

			// Subtract 1 from number of unscheduled predecessors for all successors of the node
			for (int j = 0; j < successors[next_node].size(); j++) {
				ListDigraph::Node curSucc = successors[next_node][j];
				unsched_pred[curSucc] -= 1;
			}

			// Decrease the number of unscheduled operations
			ops_unscheduled--;
		}

		out << "TGATCScheduler::run : Scheduling operations done. " << endl;
		
		// Preserve the TG selection
		prevtgselection = *tgselection;
		getTGSelection();
		tgselection->localobj = TWT()(*pm);

		//out << "TGATCScheduler::run : Current selection objective : " << tgselection->localobj << endl;

		// Check if the local objective has the best value with the current parameter kappa
		if (best_loc_obj > tgselection->localobj/*localObj(*pm, *opnodes, *tgselection, *terminals, *dloc)*/) {

			best_loc_obj = tgselection->localobj; //localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

			best_kappa = kappa;

			besttgselection = *tgselection;

			out << "TGATCScheduler::run : A better selection objective : " << tgselection->localobj << " (k = " << kappa << " )" << endl;
		} else {
			*tgselection = prevtgselection;
		}

		// Restore the initial state of the graph before the scheduling of the machine group

		// Remove the selection arcs
		for (int i = 0; i < insArcs.size(); i++) {
			ListDigraph::Arc curArc = insArcs[i];
			pm->graph.erase(curArc);
		}
		insArcs.clear();

		// Restore the operation data
		for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
			ListDigraph::Node curNode = nit;

			*(pm->ops[curNode]) = node2PrevOper[curNode];
		}

		// Preserve the arcs data
		for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
			ListDigraph::Arc curArc = ait;

			pm->conjunctive[curArc] = arc2PrevConj[curArc];
			pm->p[curArc] = arc2PrevP[curArc];
		}


		pm->updateHeads();
		pm->updateStartTimes();
		//out << "TGATCScheduler::run : TWT AFTER restoring the PM : " << TWT()(*pm) << endl;


		if (!kappaoptim) {
			break;
		} else { // Increase kappa
			kappa += kstep;
		}

	} while (kappa < kfinish);

	kappa = best_kappa;

	*tgselection = besttgselection;

#ifdef DEBUG
	insertSelection(*tgselection);
	out << "TGATCScheduler::run : TWT Final (partial schedule) : " << TWT()(*pm) << endl;
	removeSelection();
#endif	

	out << "Finished TGATCScheduler::run." << endl;

}

double TGATCScheduler::I(const double &t, const double &kappa, const double &p_avg, const ListDigraph::Node &node) {

	ListDigraph::Node curNode = node;
	Operation& curOp = *(pm->ops[curNode]);

	Machine &m = tg->earliestToFinish(&curOp);
	double curTime = m.time(); // Time the machine becomes available
	//double h = (pRemain[node] > 0.0) ? (dOrd[node] - smallestD - Math::max(pm.ops[node]->r(), t)) / pRemain[node] : 0.0;
	double p = m.procTime(&curOp);

	//pm.ops[node]->p(m.procTime(pm.ops[node]));

	//double h = pm.ops[node]->p() / pRemain[node];

	//Debugger::info << "d = " << dOrd[node] << ENDL;
	//Debugger::info << dOrd[node] - smallestD - pRemain[node] << ENDL;
	//if (lenRemain[node] == 2) {
	//    Debugger::info << "lenRemain = " << lenRemain[node] << ENDL;
	//}
	//getchar();

	//pm.ops[node]->d(Math::max(d[node], pm.ops[node]->r() + h * pm.ops[node]->p()));

	//p_avg = pRemain[node] / double(lenRemain[node]);
	//double kpavg = kappapavg; //p_avg*kappa;
	//double kR_pAvg = kappaR*p_avg;
	//FF = 1.5; //kappa; //1.1;

	//double exppow = -Math::max(Math::max(pm.ops[node]->r() + FF * pm.ops[node]->p(), dOrd[node] - FF * pRemain[node] + p) - p - t, 0.0) / kappapavg;

	//double slack = dOrd[node] - t - p - FF * (pRemain[node] - pm.ops[node]->p());

	//ff = 1.5;

	ff = 1.38;
	
	//cout << "FF : " << ff << endl;
	//getchar();

	double d = dOrd[curNode] + p /*ff * pm.ops[node]->p()*/ - ff * (pRemain[node]);

	d = Math::max(d, Math::max(curOp.r(), curTime) + p);

	double slack = (d - p) + /*curOp.r() - Math::max(curOp.r(), curTime); //*/+ Math::max(curOp.r() - curTime, 0.0); // / ff;

	double exppow = -Math::max(slack, 0.0) / (kappa * p_avg); //kpavg;
	//double exppowr = -(Math::max((1.0 + FF) * pm.ops[node]->r() - t, 0.0)) / kappaRpAvg; //kR_pAvg; //kappapavg;

	/*
	Debugger::info << "Operation : " << pm.ops[node]->ID << ENDL;
	Debugger::info << "Priority : " << pm.ops[node]->w() / p * Math::exp(exppow) << ENDL;
	Debugger::info << "k = " << kappa << ENDL;
	Debugger::info << "pavg = " << p_avg << ENDL;
	Debugger::info << "kpavg = " << kpavg << ENDL;
	Debugger::info << "slack = " << slack << ENDL;
	Debugger::info << "pow = " << exppow << " e^pow = " << Math::exp(exppow) << ENDL;
	 */
	//getchar();

	//p = pRemain[node];

	return curOp.w() / p * Math::exp(exppow); // * Math::exp(exppowr);



	double res = 0.0;
	//int n = terminals->size();

	if (pm->ops[node]->p() <= 0.0000001) {
		return Math::MAX_DOUBLE;
	}

	double kappapavg = kappa*p_avg;

	//for (int k = 0; k < n; k++) {
	//res += pm->ops[terminals->at(k)]->w() / pm->ops[node]->p() *
	//	Math::exp(-((*dloc)[pm->ops[node]->ID][k] - pm->ops[node]->p() + Math::max(0.0, pm->ops[node]->r() - t)) / (kappapavg));

	res = pm->ops[node]->w() / pm->ops[node]->p() *
			Math::exp(-(pm->ops[node]->d() - pm->ops[node]->p() + Math::max(0.0, pm->ops[node]->r() - t)) / (kappapavg));
	//}

	//res = pm->ops[node]->w() / (pm->ops[node]->r());

	return res;
}

double TGATCScheduler::localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection&, QList<ListDigraph::Node>&, QHash<int, QList<double> >&) {
	//Debugger::err << "TGScheduler::localObj() : not implemented!" << ENDL;

	//SBHTWTLocalObj lobj;
	UTWT utwt;

	double res = utwt(pm, opnodes, locD); // lobj(pm, opnodes, tgselection, terminals, dloc);

	return res;
}

/* -------------------------------------------------------------------------- */

/* --------  Tool group Extended ATC scheduler as in Pinedo-Singer ---------- */

TGATCSchedulerPinSin::TGATCSchedulerPinSin() {
	kappa = 2.0;
	kstart = 0.01;
	kfinish = 4.0;
	kstep = 0.2;
	kappaoptim = true;

	locD.clear();
}

TGATCSchedulerPinSin::TGATCSchedulerPinSin(TGATCSchedulerPinSin& orig) : TGScheduler(orig) {
	this->kappa = orig.kappa;
	this->kstart = orig.kstart;
	this->kfinish = orig.kfinish;
	this->kstep = orig.kstep;
	this->kappaoptim = orig.kappaoptim;

	this->dOrd = orig.dOrd;
	this->pRemain = orig.pRemain;
	this->lenRemain = orig.lenRemain;

	this->locD = orig.locD;
}

TGATCSchedulerPinSin::~TGATCSchedulerPinSin() {

}

Clonable* TGATCSchedulerPinSin::clone() {
	return new TGATCSchedulerPinSin(*this);
}

void TGATCSchedulerPinSin::clear() {

	dOrd.clear();
	pRemain.clear();
	lenRemain.clear();

	kappa = kstart;

	topolOrdering.clear();

	locD.clear();
}

void TGATCSchedulerPinSin::preparePM() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// Get the topological ordering of the graph
	topolOrdering = pm->topolSort();

	// Update the ready times of the operations
	pm->updateHeads(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	QList<ListDigraph::Node> terminals = pm->terminals();

	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		lenRemain[nit] = 0;
	}

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		lenRemain[terminals[i]] = 1;
		pRemain[terminals[i]] = 0.0;
		dOrd[terminals[i]] = pm->ops[terminals[i]]->d();

		for (ListDigraph::InArcIt iait(pm->graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm->graph.source(iait);
			q.enqueue(curpred);

			lenRemain[curpred] = 2;
			pRemain[curpred] = pm->ops[curpred]->p() + 0.0; // 0.0 - for the terminal node
			dOrd[curpred] = pm->ops[terminals[i]]->d();
		}

		ordNodes.clear();
		QSet<int> insertedNodeISs;
		while (!q.empty()) {
			curnode = q.dequeue();

			insertedNodeISs.remove(pm->graph.id(curnode));

			// Save the node 
			ordNodes.append(curnode);

			// Find the largest number of the operations to be processed after the current node (including the current node)
			ListDigraph::Node longerSucc = INVALID;
			double maxP = -1.0;
			for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
				if (pRemain[pm->graph.target(oait)] > maxP) {
					maxP = pRemain[pm->graph.target(oait)];
					longerSucc = pm->graph.target(oait);
				}
			}
			//Debugger::info << "Found : " << pm.ops[longerSucc]->ID << ENDL;
			//getchar();
			if (longerSucc == INVALID) {
				Debugger::err << "TGATCSchedulerPinSin::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
			}
			pRemain[curnode] = pRemain[longerSucc] + pm->ops[curnode]->p();
			lenRemain[curnode] = lenRemain[longerSucc] + 1;
			dOrd[curnode] = dOrd[longerSucc]; // Due date of the order

			//Debugger::info << pm.ops[longerSucc]->ID << " : " << d[longerSucc] << ENDL;
			//getchar();

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
				curpred = pm->graph.source(iait);

				// Push the current predecessor into the queue, provided it is not in there already
				if (!insertedNodeISs.contains(pm->graph.id(curnode))) {

					q.enqueue(curpred);

					insertedNodeISs.insert(pm->graph.id(curnode));
				}

			}

		}

		//getchar();
	}

	// Initialize the start times of the operations
	pm->updateStartTimes(topolOrdering);

}

void TGATCSchedulerPinSin::run() {
	/** Algorithm:
	 * 
	 *	1. For every node:
	 *  1.1. Calculate the number of unscheduled predecessors
	 * 
	 *  2. For different values of parameter kappa:
	 *	2. While not all operation scheduled:
	 *  2.1. Find first unscheduled operation, predecessors of which have 
	 *		 already been scheduled and which is not scheduled itself
	 *  2.2. Update the ready time of the operation as the maximum of its own
	 *		 ready time and the completion times of its predecessors
	 *  2.3. Schedule the operation
	 *  2.4. Subtract 1 from number of unscheduled predecessors for all 
	 *		 successors of the node
	 * 
	 *	3. Generate the tool group selection.
	 * 
	 */

	QTextStream out(stdout);

	out << "TGATCSchedulerPinSin::run..." << endl;

	clear();

#ifdef DEBUG	
	if (!dag(pm->graph)) {
		Debugger::err << "TGATCSchedulerPinSin::run : Cycles in the graph!!!" << ENDL;
	}
#endif	

	preparePM();

	out << "TGATCSchedulerPinSin:: Prepared the PM " << endl;

	QHash<int, ListDigraph::Node> opID2Node;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		opID2Node[pm->ops[nit]->ID] = nit;
	}

	TGSelection prevtgselection;
	TGSelection besttgselection;

	if (kappaoptim) {
		kappa = kstart;
	}

	double best_loc_obj = Math::MAX_DOUBLE;
	int best_kappa = kappa;

	// Topological ordering of the graph
	QList<ListDigraph::Node> ts = pm->topolSort();

	// The actual state of the PM
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);

	// Save the state of the graph
	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;
	QList<ListDigraph::Arc> insArcs;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *(pm->ops[curNode]);
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm->conjunctive[curArc];
		arc2PrevP[curArc] = pm->p[curArc];
	}

	out << "TGATCSchedulerPinSin::run : UTWT before scheduling the TG (partial schedule) : " << UTWT()(*pm, *opnodes, locD) << endl;

	do {

		ts = pm->topolSort();

		// For every node calculate the number of predecessors
		QMap<ListDigraph::Node, int> unsched_pred;

		for (int i = 0; i < opnodes->size(); i++) {

			ListDigraph::Node curNode = opnodes->at(i);

			pm->ops[curNode]->machID = -1;
			//out << "TGATCSchedulerPinSin::run : Unscheduling operation " << pm->ops[opnodes->at(i)]->ID << endl;

			unsched_pred[curNode] = predecessors[curNode].size();
		}

		// Schedule all operation one by one updating their availability
		int ops_unscheduled = opnodes->size();

		tg->init();

		while (ops_unscheduled > 0) {

			// Iterate over the unassigned nodes and select the one with the greatest priority index
			ListDigraph::Node next_node = INVALID;
			double prio = -1.0;
			double p_avg = 0.0;
			int n_unassigned_avail = 0;

			// Recalculate p_avg: average processing time for the not assigned but already available operations
			for (int i = 0; i < opnodes->size(); i++) {

				ListDigraph::Node curNode = opnodes->at(i);
				Operation& curOp = *(pm->ops[curNode]);

				// Is the node not assigned?

				if (curOp.ID > 0 && curOp.machID < 0 && unsched_pred[curNode] == 0) { // This operation is not assigned to any machine

					Machine& curMach = tg->earliestToFinish(&curOp);
					double curP = curMach.procTime(&curOp);

					p_avg += curP; //pRemain[curNode];

					n_unassigned_avail++;

				} // Accounting the unscheduled operations

			} // Iterating over nodes
			p_avg /= double(n_unassigned_avail);

			for (int i = 0; i < opnodes->size(); i++) {

				ListDigraph::Node curNode = opnodes->at(i);
				Operation& curOp = *(pm->ops[curNode]);

				// Is the node not assigned and available for scheduling
				if (curOp.ID > 0 && curOp.machID < 0 && unsched_pred[curNode] == 0) { // Unassigned available node

					//out << pm->ops[opnodes->at(i)]->ID << " : " << I(tg->earliestToFinish(pm->ops[opnodes->at(i)]).time(), kappa, p_avg, opnodes->at(i)) << endl;

					// Is the priority index of the node greater?

					double curPrioVal = I(tg->earliestToFinish(&curOp).time(), kappa, p_avg, curNode);

					if (prio < curPrioVal) {

						prio = curPrioVal;

						next_node = curNode;

					}

				} // Taking into account the unassigned available nodes

			} // Iterating over the nodes

			if (next_node == INVALID) {
				out << "Operations unscheduled : " << ops_unscheduled << endl;
				int ctr = 0;
				for (int i = 0; i < opnodes->size(); i++) {
					if (pm->ops[opnodes->at(i)]->ID > 0 && (pm->ops[opnodes->at(i)]->machID < 0)) {
						ctr++;
						out << ctr << " : " << *pm->ops[opnodes->at(i)] << endl;
						out << "Unscheduled predecessors : " << unsched_pred[opnodes->at(i)] << endl;

						for (int j = 0; j < predecessors[opnodes->at(i)].size(); j++) {
							if (pm->ops[predecessors[opnodes->at(i)][j]]->machID >= 0) {
								out << *pm->ops[predecessors[opnodes->at(i)][j]] << endl;
								Debugger::err << "Scheduled predecessor is marked as unscheduled!" << ENDL;
							}
						}
					}
				}
				Debugger::err << "TGATCSchedulerPinSin::run : No node for scheduling found although unscheduled operations are still remaining!" << ENDL;
			}

			//out << "TGATCSchedulerPinSin::run : Trying to schedule operation " << *pm->ops[next_node] << endl;

			Operation& curOp = *(pm->ops[next_node]);

			// Schedule the operation
			Machine &m = tg->earliestToFinish(&curOp);

			ListDigraph::Node prevMachNode = INVALID; // The last node on the assigned to the machine

			if (m.operations.size() == 0) {
				prevMachNode = INVALID;
			} else {
				int lastMachOperID = m.operations.last()->ID;
				prevMachNode = opID2Node[lastMachOperID];
			}

			if (curOp.machID > 0) {
				out << *pm << endl;
				Debugger::err << "TGATCSchedulerPinSin::run : Trying to schedule already scheduled operation : " << curOp.ID << " !!!" << ENDL;
			}

			//out << m << endl;
			m << &curOp;

			// The operation has been scheduled - > reflect this in the PM
			if (prevMachNode == INVALID) { // No arcs are inserted

			} else { // Insert an arc

				ListDigraph::Arc curArc = pm->graph.addArc(prevMachNode, next_node);

				insArcs.append(curArc);

				pm->conjunctive[curArc] = false;
				pm->p[curArc] = -pm->ops[prevMachNode]->p();

				// Update the topological ordering of the graph
				pm->dynUpdateTopolSort(ts, prevMachNode, next_node);

			}

			// Update the lengths of all outgoing arcs of the current node
			for (ListDigraph::OutArcIt oait(pm->graph, next_node); oait != INVALID; ++oait) {
				pm->p[oait] = -pm->ops[next_node]->p();
			}

			// Update the release and the start times
			pm->updateHeads(ts);
			pm->updateStartTimes(ts);

			// Subtract 1 from number of unscheduled predecessors for all successors of the node
			for (int j = 0; j < successors[next_node].size(); j++) {
				ListDigraph::Node curSucc = successors[next_node][j];
				unsched_pred[curSucc] -= 1;
			}

			// Decrease the number of unscheduled operations
			ops_unscheduled--;
		}

		// Preserve the TG selection
		prevtgselection = *tgselection;
		getTGSelection();
		tgselection->localobj = UTWT()(*pm, *opnodes, locD);

		//out << "TGATCSchedulerPinSin::run : Current selection objective : " << tgselection->localobj << endl;

		// Check if the local objective has the best value with the current parameter kappa
		if (best_loc_obj > tgselection->localobj/*localObj(*pm, *opnodes, *tgselection, *terminals, *dloc)*/) {

			best_loc_obj = tgselection->localobj; //localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

			best_kappa = kappa;

			besttgselection = *tgselection;

			out << "TGATCSchedulerPinSin::run : A better selection objective : " << tgselection->localobj << " (k = " << kappa << " )" << endl;
		} else {
			*tgselection = prevtgselection;
		}

		// Restore the initial state of the graph before the scheduling of the machine group

		// Remove the selection arcs
		for (int i = 0; i < insArcs.size(); i++) {
			ListDigraph::Arc curArc = insArcs[i];
			pm->graph.erase(curArc);
		}
		insArcs.clear();

		// Restore the operation data
		for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
			ListDigraph::Node curNode = nit;

			*(pm->ops[curNode]) = node2PrevOper[curNode];
		}

		// Preserve the arcs data
		for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
			ListDigraph::Arc curArc = ait;

			pm->conjunctive[curArc] = arc2PrevConj[curArc];
			pm->p[curArc] = arc2PrevP[curArc];
		}


		pm->updateHeads();
		pm->updateStartTimes();
		//out << "TGATCSchedulerPinSin::run : TWT AFTER restoring the PM : " << TWT()(*pm) << endl;


		if (!kappaoptim) {
			break;
		} else { // Increase kappa
			kappa += kstep;
		}

	} while (kappa < kfinish);

	kappa = best_kappa;

	*tgselection = besttgselection;

#ifdef DEBUG
	insertSelection(*tgselection);
	out << "TGATCSchedulerPinSin::run : UTWT Final (partial schedule) : " << UTWT()(*pm, *opnodes, locD) << endl;
	out << "TGATCSchedulerPinSin::run : TWT Final (partial schedule) : " << TWT()(*pm) << endl;
	removeSelection();
#endif	

	out << "TGATCSchedulerPinSin::run : Finished ." << endl;
	

}

double TGATCSchedulerPinSin::I(const double &, const double &kappa, const double &p_avg, const ListDigraph::Node &node) {

	ListDigraph::Node curNode = node;
	Operation& curOp = *(pm->ops[curNode]);

	Machine &m = tg->earliestToFinish(&curOp);
	double curTime = m.time(); // Time the machine becomes available
	//double h = (pRemain[node] > 0.0) ? (dOrd[node] - smallestD - Math::max(pm.ops[node]->r(), t)) / pRemain[node] : 0.0;
	double p = m.procTime(&curOp);

	ff = 1.0;

	//double d = dOrd[curNode] + p /*ff * pm.ops[node]->p()*/ - ff * (pRemain[node]);

	//d = Math::max(d, Math::max(curOp.r(), curTime) + p);
	QList<ListDigraph::Node> terminals = pm->terminals();

	double res = 0.0;

	for (int k = 0; k < terminals.size(); k++) {
		ListDigraph::Node curTerm = terminals[k];
		ListDigraph::Node curNode = node;

		double d = locD[curNode][curTerm];

		double slack = (d - p) + Math::max(curOp.r() - curTime, 0.0); // / ff;

		double exppow = -Math::max(slack, 0.0) / (kappa * p_avg); //kpavg;

		double w = pm->ops[curTerm]->w();

		res += w / p * Math::exp(exppow);

	}

	return res;
}

double TGATCSchedulerPinSin::localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection&, QList<ListDigraph::Node>&, QHash<int, QList<double> >&) {
	//Debugger::err << "TGScheduler::localObj() : not implemented!" << ENDL;

	//SBHTWTLocalObj lobj;
	UTWT utwt;

	double res = utwt(pm, opnodes, locD); // lobj(pm, opnodes, tgselection, terminals, dloc);

	return res;
}

/* -------------------------------------------------------------------------- */