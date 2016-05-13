/* 
 * File:   ProcessModel.cpp
 * Author: DrSobik
 * 
 * Created on July 7, 2011, 12:36 PM
 */

#include <QtCore/qstack.h>

#include "ProcessModel.h"

#include <QTextStream>
#include <lemon/list_graph.h>
#include <lemon/bits/graph_extender.h>

// QTextStream out1(stdout); // Very bad practice in case of multi-threading

ProcessModel::ProcessModel() : ops(graph), p(graph), conjunctive(graph), savedops(savedGraph), savedp(savedGraph), savedconjunctive(savedGraph) {
	saved = false;

	head = INVALID;
	tail = INVALID;
}

ProcessModel::ProcessModel(const ProcessModel& orig) : ops(graph), p(graph), conjunctive(graph), savedops(savedGraph), savedp(savedGraph), savedconjunctive(savedGraph) {
	*this = orig;
}

ProcessModel::~ProcessModel() {
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		delete this->ops[nit];
	}

	// Clear the saved model if present
	clearSaved();
}

QHash<int, ListDigraph::Node> ProcessModel::opID2Node() {
	QHash<int, ListDigraph::Node> res;

	res.reserve(countNodes(graph));
	// Iterate over all nodes of the graph
	for (ListDigraph::NodeIt it(graph); it != INVALID; ++it) {
		res[ops[it]->ID] = it;
	}

	return res;
}

QList<ListDigraph::Node> ProcessModel::terminals() {
	QList<ListDigraph::Node> res;

	for (ListDigraph::InArcIt iait(graph, tail); iait != INVALID; ++iait) {
		res.append(graph.source(iait));
	}

	return res;
}

void ProcessModel::updateHeads() {

	QList<ListDigraph::Node> ts = topolSort();

	updateHeads(ts);

	return;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(graph, p);

	ListDigraph::Node prodHead = INVALID;
	for (ListDigraph::OutArcIt oait(graph, head); oait != INVALID; ++oait) {
		prodHead = graph.target(oait);
		for (ListDigraph::OutArcIt oaitcur(graph, prodHead); oaitcur != INVALID; ++oaitcur) {
			p[oaitcur] = -ops[prodHead]->ir();
		}
	}

	bf.init();
	bf.addSource(head);
	//Debugger::info << "Running the BF algorithm..."<<ENDL;
	bf.start();
	//Debugger::info << "Done running the BF algorithm."<<ENDL;

	// #### IMPORTANT  #### Local ready times of the operations must be updated, but the initial ready times must be considered
	// Update the ready time of the operation
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
#ifdef DEBUG
		if (!bf.reached(nit)) {
			Debugger::err << "ProcessModel::updateHeads : Operation ID= " << ops[nit]->OID << ":" << ops[nit]->ID << " can not be reached from the root node " << ops[head]->OID << ":" << ops[head]->ID << "!" << ENDL;
		}
#endif
		ops[nit]->r(Math::max(-bf.dist(nit), ops[nit]->ir()));
		//out << "The found length: " << ops[opnodes[i]]->r() << endl;
	}
}

void ProcessModel::updateHeads(QList<ListDigraph::Node>& topolOrdering) {
	/** Set the ready times equal to the initial ready times. */
	for (int i = 0; i < topolOrdering.size(); i++) {
		ops[topolOrdering[i]]->r(ops[topolOrdering[i]]->ir());
	}

	// Walk through the topologically sorted nodes and recalculate the ready times of the corresponding operations
	double curr;
	for (int i = 0; i < topolOrdering.size(); i++) {
		if (topolOrdering[i] == head) {
			ops[topolOrdering[i]]->p(0.0);
			ops[topolOrdering[i]]->r(0.0);
		} else {
			curr = ops[topolOrdering[i]]->ir(); // Ready time should not be less than its initial ready time defined by the input data (can be 0)
			// Iterate over the direct predecessors of the current node
			for (ListDigraph::InArcIt iait(graph, topolOrdering[i]); iait != INVALID; ++iait) {
				curr = Math::max(curr, ops[graph.source(iait)]->r()+(-p[iait])); // Ready time is the maximum between over all predecessors of ready time + processing time
			}
			ops[topolOrdering[i]]->r(curr);
		}
	}
}

void ProcessModel::updateStartTimes() {

	QList<ListDigraph::Node> ts = topolSort();

	updateStartTimes(ts);

	return;

	/*
	ListDigraph::NodeMap<int> nodes2pos(graph); // Map of nodes sorted topologically

	// Sort the nodes topologically
	topologicalSort(graph, nodes2pos);

	QMap<int, ListDigraph::Node> pos2nodes; // Topologically sorted nodes 

	for (ListDigraph::NodeMap<int>::MapIt mi(nodes2pos); mi != INVALID; ++mi) {
		pos2nodes[*mi] = mi;
	}

	QList<ListDigraph::Node> tnodes;

	tnodes.clear();
	tnodes.reserve(pos2nodes.size());
	for (QMap<int, ListDigraph::Node>::iterator sti = pos2nodes.begin(); sti != pos2nodes.end(); sti++) {
		tnodes << sti.value();
	}

	// Set the start times equal to the ready times. 
	for (int i = 0; i < tnodes.size(); i++) {
		ops[tnodes[i]]->s(ops[tnodes[i]]->r());
	}

	// Walk through the topologically sorted nodes and recalculate the start times of the corresponding operations
	double curs;
	for (int i = 0; i < tnodes.size(); i++) {
		if (tnodes[i] == head) {
			ops[tnodes[i]]->p(0.0);
			ops[tnodes[i]]->s(0.0);
		} else {
			curs = ops[tnodes[i]]->ir();
			// Iterate over the direct predecessors of the current node
			for (ListDigraph::InArcIt iait(graph, tnodes[i]); iait != INVALID; ++iait) {
				curs = Math::max(curs, ops[graph.source(iait)]->c());
			}
			ops[tnodes[i]]->s(curs);
		}
	}
	 */
}

void ProcessModel::updateStartTimes(QList<ListDigraph::Node>& topolOrdering) {
	// Set the start times equal to the ready times. 
	for (int i = 0; i < topolOrdering.size(); i++) {
		ops[topolOrdering[i]]->s(ops[topolOrdering[i]]->r());
	}

	// Walk through the topologically sorted nodes and recalculate the start times of the corresponding operations
	double curs;
	ListDigraph::Node curNode = INVALID;
	for (int i = 0; i < topolOrdering.size(); i++) {
		curNode = topolOrdering[i];

		if (curNode == head) {
			ops[curNode]->p(0.0);
			ops[curNode]->s(0.0);
		} else {
			curs = ops[curNode]->ir();
			// Iterate over the direct predecessors of the current node
			for (ListDigraph::InArcIt iait(graph, curNode); iait != INVALID; ++iait) {
				curs = Math::max(curs, ops[graph.source(iait)]->c());
			}

			// Take into account the machine's availability time
			curs = Math::max(curs, ops[curNode]->machAvailTime());

			// Set the correct starting time of the operation
			ops[curNode]->s(curs);
		}
	}
}

void ProcessModel::updateHeadsAndStartTimes(QList<ListDigraph::Node>& topolOrdering) {
	/** Set the ready times equal to the initial ready times. */
	for (int i = 0; i < topolOrdering.size(); i++) {
		ops[topolOrdering[i]]->r(ops[topolOrdering[i]]->ir());
	}

	// Walk through the topologically sorted nodes and recalculate the ready times of the corresponding operations
	double curr;
	for (int i = 0; i < topolOrdering.size(); i++) {
		ListDigraph::Node curNode = topolOrdering[i];

		if (curNode == head) {
			ops[curNode]->p(0.0);
			ops[curNode]->r(0.0);
		} else {
			curr = ops[curNode]->ir(); // Ready time should not be less than its initial ready time defined by the input data (can be 0)
			// Iterate over the direct predecessors of the current node
			for (ListDigraph::InArcIt iait(graph, curNode); iait != INVALID; ++iait) {
				curr = Math::max(curr, ops[graph.source(iait)]->c()); // Ready time is the maximum between over all predecessors of ready time + processing time
			}

			// Take into account the machine's availability time
			curr = Math::max(curr, ops[curNode]->machAvailTime());

			ops[curNode]->r(curr); // The earliest time point to start the operation

			// Set the start time
			ops[curNode]->s(curr);
		}
	}
}

QHash<int, QList<int> > ProcessModel::updateCPPred(QList<ListDigraph::Node>& topolOrdering, const int& startPos) {
	/**
	 * 
	 * 1. Consider each node. For the current node:
	 * 1.1. For the considered node find its (semi-)direct predecessors which have the largest completion time
	 * 1.2. It the largest completion time is less than the start time of the current operation -> this operation has no critical predecessors and is delayed by the machine availability
	 * 1.3. If the largest completion time equals the start time of the operation -> the corresponding predecessors are critical
	 * 
	 */

	QHash<int, QList<int> > opID2CPPredOpIDs;

	for (int i = startPos; i < topolOrdering.size(); i++) { // Iterating over the topologically ordered nodes

		ListDigraph::Node curNode = topolOrdering[i];
		Operation& curOp = *(ops[curNode]);

		double maxPredC = -1.0;

		// Find the largest predecessor completion time
		for (ListDigraph::InArcIt iait(graph, curNode); iait != INVALID; ++iait) { // Iterate over the (semi-)direct predecessors of the node

			ListDigraph::Node curPredNode = graph.source(iait);
			Operation& curPredOp = *(ops[curPredNode]);

			if (maxPredC < curPredOp.c()) { // Found a predecessor with a larger completion time
				maxPredC = curPredOp.c();
			}

		}

		// Check the maxPredC/s relation
		if (Math::cmp(maxPredC, curOp.s()) == -1) { // This node has no critical predecessors and is delayed by its machine or its initial release time
			opID2CPPredOpIDs[curOp.ID].clear();
			continue;
		}

		// Find all (semi-)direct predecessors with the largest completion time
		QList<int> critPredNodeIDs;
		for (ListDigraph::InArcIt iait(graph, curNode); iait != INVALID; ++iait) { // Iterate over the (semi-)direct predecessors of the node

			ListDigraph::Node curPredNode = graph.source(iait);
			Operation& curPredOp = *(ops[curPredNode]);

			if (maxPredC == curPredOp.c()) { // This is a critical predecessor

				critPredNodeIDs.append(curPredOp.ID);

			}

		} // Searching for the critical predecessors

		opID2CPPredOpIDs[curOp.ID] = critPredNodeIDs;


	} // Iterating over the topologically ordered nodes

	// Return the result
	return opID2CPPredOpIDs;

}

void ProcessModel::dynUpdateTopolSort(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &k) {
	QTextStream out(stdout);

	/** Algorithm:
	 * 
	 * 1. Find the positions of i and k
	 * 2. IF posi < posk => no changes to the topological sorting needs to be performed. Return.
	 * 3. IF posi > posk => reorder the nodes. The affected region is [posi, posk]. Return.
	 */

	int posi = -1;
	int posk = -1;

	if (k == INVALID) {
		posk = (int) Math::MAX_INTUNI;
	} else {
		posk = topolOrdering.indexOf(k);
	}

	if (i == INVALID) {
		posi = 0;
	} else {
		posi = topolOrdering.indexOf(i);
	}

	if (posi < posk) { // No changes to perform
		return;
	}

	// #####################  DEBUG  ###########################################

	/*
	out << "Before DTO:" << endl;
	out << "posj = " << posj << " ID = " << ((j == INVALID) ? -1 : pm->ops[j]->ID) << endl;
	out << "posi = " << posi << " ID = " << ((i == INVALID) ? -1 : pm->ops[i]->ID) << endl;
	out << "posk = " << posk << " ID = " << ((k == INVALID) ? -1 : pm->ops[k]->ID) << endl;

	for (int l = 0; l < topolOrdering.size(); l++) {
		out << pm->ops[topolOrdering[l]]->ID << " ";
	}
	out << endl;

	//getchar();
	 */

	// #########################################################################

	if (posi == posk) {
		out << "posi = " << posi << " ID = " << ((i == INVALID) ? -1 : this->ops[i]->ID) << endl;
		out << "posk = " << posk << " ID = " << ((k == INVALID) ? -1 : this->ops[k]->ID) << endl;

		for (int l = 0; l < topolOrdering.size(); l++) {
			out << this->ops[topolOrdering[l]]->ID << " ";
		}
		out << endl;

		Debugger::err << "ProcessModel::dynUpdateTopolSort : posi == posk which is impossible!!!" << ENDL;
	}

	// Find the affected region
	int arbegin = -1;
	int arend = -1;
	ListDigraph::Node arstartnode = INVALID;
	ListDigraph::Node arendnode = INVALID;

	if (posi > posk) {
		arbegin = posk;
		arend = posi;
		arstartnode = k;
		arendnode = i;
	}

	// #####################  DEBUG  ###########################################
	/*
	out << "arbegin = " << arbegin << endl;
	out << "arend = " << arend << endl;
	out << "arstartnode = " << pm->ops[arstartnode]->ID << endl;
	out << "arendnode = " << pm->ops[arendnode]->ID << endl;
	 */
	// #########################################################################

	// Update the affected region

	// The nodes of the affected region
	QList<ListDigraph::Node> ar = topolOrdering.mid(arbegin, arend - arbegin + 1);
	QList<bool> visited;
	visited.reserve(ar.size());
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;
	ListDigraph::Node tmpnode;
	int tmpidx;
	//QList<int> deltaBIdx;

	// #####################  DEBUG  ###########################################

	/*
	out << "ar:" << endl;
	for (int l = 0; l < ar.size(); l++) {
		out << pm->ops[ar[l]]->ID << " ";
	}
	out << endl;
	 */

	// #########################################################################

	// Find nodes which are contained in ar and are reachable from arstartnode
	//out << "Finding deltaF..." << endl;
	QList<ListDigraph::Node> deltaF;

	deltaF.reserve(ar.size());

	for (int l = 0; l < ar.size(); l++) {
		visited.append(false);
	}

	q.clear();
	q.enqueue(arstartnode);

	deltaF.append(arstartnode);
	while (q.size() != 0) {
		curnode = q.dequeue();

		// Check the successors of the current node
		for (ListDigraph::OutArcIt oait(this->graph, curnode); oait != INVALID; ++oait) {
			tmpnode = this->graph.target(oait);

			tmpidx = ar.indexOf(tmpnode);

			if (tmpidx >= 0 && !visited[tmpidx]) { // This successor is within the affected region
				q.enqueue(tmpnode);
				visited[tmpidx] = true;

				// Add the node to the deltaF
				deltaF.append(tmpnode);

			}

		}
	}

	//out << "Found deltaF." << endl;

	//######################  DEBUG  ###########################################
	/*
	out << "deltaF:" << endl;
	for (int l = 0; l < deltaF.size(); l++) {
		out << pm->ops[deltaF[l]]->ID << " ";
	}
	out << endl;
	 */
	//##########################################################################

	// IMPORTANT!!! Actually deltaB is not needed! If we find deltaF and move it to the end of the affected region then the elements
	// of deltaB preserve their initial positions and are placed directly before the elements of deltaF. Thus, the backward arc becomes a forward one
	/*
	// Find the nodes which are in ar and are BACKWARD reachable from arendnode
	QList<ListDigraph::Node> deltaB;

	deltaB.reserve(ar.size());

	for (int l = 0; l < visited.size(); l++) {
		visited[l] = false;
	}

	q.clear();
	q.enqueue(arendnode);

	deltaB.prepend(arendnode);
	deltaBIdx.prepend(ar.size() - 1);

	visited.clear();
	for (int l = 0; l < ar.size(); l++) {
		visited.append(false);
	}
	while (q.size() != 0) {
		curnode = q.dequeue();

		// Check the predecessors of the current node
		for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
			tmpnode = pm->graph.source(iait);

			tmpidx = ar.indexOf(tmpnode);

			if (tmpidx >= 0 && !visited[tmpidx]) { // This successor is within the affected region
				q.enqueue(tmpnode);
				visited[tmpidx] = true;

				// Add the node to the deltaF
				deltaB.prepend(tmpnode); // IMPORTANT!!! PREpend!
				deltaBIdx.prepend(tmpidx);
			}

		}
	}
	 */

	// Move elements of deltaB to the left and the elements of deltaF to the right until the backward ark does not disappear
	//int posB = 0;
	//out << "Shifting deltaF to the right..." << endl;
	int posF = ar.size() - 1;

	// Move elements in deltaF to the right
	while (!deltaF.isEmpty()) {
		// Find the first element in ar starting from posB that is in deltaB
		tmpidx = -1;
		for (int l = posF; l >= 0; l--) {
			if (deltaF.contains(ar[l])) {
				tmpidx = l;
				break;
			}
		}

		if (tmpidx == -1) {
			Debugger::err << "ProcessModel::dynUpdateTopolSort : tmpidx = -1 while shifting deltaF. Probably the graph is NOT DAG! " << ENDL;
		}

		// Erase this element from deltaF
		deltaF.removeOne(ar[tmpidx]);

		// Move this element to the left
		ar.move(tmpidx, posF);
		posF--;
	}
	//out << "Shifted deltaF to the right." << endl;

	// Moving elements of deltaB is not necessary, since they are automatically found before any element of deltaF, since these were moved to the right

	/*
	// Move elements in deltaB to the left so that the last element of deltaB is on the position posF (right before elements of deltaF)
	while (!deltaB.isEmpty()) {
		// Find the first element in ar starting from posB that is in deltaB
		tmpidx = -1;
		for (int l = posB; l < ar.size(); l++) {
			if (deltaB.contains(ar[l])) {
				tmpidx = l;
				break;
			}
		}

		// Erase this element from deltaB
		deltaB.removeOne(ar[tmpidx]);

		// Move this element to the left
		ar.move(tmpidx, posB);
		posB++;
	}
	 */


	// Modify the final topological ordering
	for (int l = 0; l < ar.size(); l++) {
		topolOrdering[arbegin + l] = ar[l];
	}

	//######################  DEBUG  ###########################################

	/*
	out << "After DTO:" << endl;
	out << "posj = " << posj << " ID = " << ((j == INVALID) ? -1 : pm->ops[j]->ID) << endl;
	out << "posi = " << posi << " ID = " << ((i == INVALID) ? -1 : pm->ops[i]->ID) << endl;
	out << "posk = " << posk << " ID = " << ((k == INVALID) ? -1 : pm->ops[k]->ID) << endl;

	out << "ar later:" << endl;
	for (int l = 0; l < ar.size(); l++) {
		out << pm->ops[ar[l]]->ID << " ";
	}
	out << endl;

	//out << "deltaB:" << endl;
	//for (int l = 0; l < deltaB.size(); l++) {
	//out << pm->ops[deltaB[l]]->ID << " ";
	//}
	//out << endl;

	out << "deltaF:" << endl;
	for (int l = 0; l < deltaF.size(); l++) {
		out << pm->ops[deltaF[l]]->ID << " ";
	}
	out << endl;

	for (int l = 0; l < topolOrdering.size(); l++) {
		out << pm->ops[topolOrdering[l]]->ID << " ";
	}
	out << endl;
	 */

	// Check the correctness of the topological sorting

	/*
	QList<ListDigraph::Node> list;
	for (int i = 0; i < topolOrdering.size() - 1; i++) {
		for (int j = i + 1; j < topolOrdering.size(); j++) {
			list.clear();
			list.append(topolOrdering[j]);
			if (reachableFrom(list).contains(topolOrdering[i])) {
				out << *this << endl;
				out << this->ops[topolOrdering[j]]->ID << " -> " << this->ops[topolOrdering[i]]->ID << endl;
				Debugger::err << "Topological sorting is not correct after DTO!!!" << ENDL;
			}
		}
	}
	 */


	//getchar();

	//##########################################################################   
}

ProcessModel& ProcessModel::operator=(const ProcessModel &other) {
	ListDigraph::NodeMap<ListDigraph::Node> noderef(other.graph);

	if (other.head == INVALID || other.tail == INVALID) { // The other is invalid => clear this one
		clear();
		//Debugger::warn << "ProcessModel::operator= : The other PM is not valid!" << ENDL;
		return *this;
	}

	// Delete the saved stuff
	clearSaved();

	// Delete the previous operations
	clearCurrent();

	// Copy nodes and arc map of the process model
	digraphCopy(other.graph, this->graph).node(other.head, this->head).node(other.tail, this->tail).nodeRef(noderef).arcMap(other.p, this->p).arcMap(other.conjunctive, this->conjunctive).run();

	// Create and copy the operations
	for (ListDigraph::NodeIt nit(other.graph); nit != INVALID; ++nit) {
		this->ops[noderef[nit]] = new Operation(*other.ops[nit]);
	}

	return *this;
}

void ProcessModel::save() {
	//out1 << "Saving PM ..." << endl;

	// Delete the previous operations
	clearSaved();

	// Mark the PM to be saved
	saved = true;

	// #######  TESTING
	// Preserve the information about the operations
	savedOps.clear();
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) * ops[curNode];

		savedOps[curNode] = curOp;
	}

	// Preserve the information about the arcs
	savedArcs.clear();
	savedP.clear();
	savedConjunctive.clear();
	for (ListDigraph::ArcIt ait(graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		ListDigraph::Node curStartNode = graph.source(curArc);
		ListDigraph::Node curEndNode = graph.target(curArc);

		savedArcs.append(QPair<ListDigraph::Node, ListDigraph::Node>(curStartNode, curEndNode));

		savedP.append(p[curArc]);

		savedConjunctive.append(conjunctive[curArc]);
	}

	// ################

	return;

	// Copy the current graph into the saved graph
	//out1 << "Copying the current graph..." << endl;
	ListDigraph::NodeMap<ListDigraph::Node> noderef(graph);
	digraphCopy(graph, this->savedGraph).node(head, this->savedHead).node(tail, savedTail).nodeRef(noderef).arcMap(p, savedp).arcMap(conjunctive, savedconjunctive).run();
	//out1 << "Done copying the current graph." << endl;

	// Create and copy the operations
	//out1 << "Copying the current operations into the saved operations ..."<<endl;
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		this->savedops[noderef[nit]] = new Operation(*ops[nit]);
	}
	//out1 << "Done copying the current operations into the saved operations."<<endl;

	/*
	// Save the operations
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		savedOps[ops[nit]->ID] = new Operation(*ops[nit]);
	}

	// Save the arcs and the assigned data
	for (ListDigraph::ArcIt ait(graph); ait != INVALID; ++ait) {
		savedArcs.append(QPair<ListDigraph::Node, ListDigraph::Node > (graph.source(ait), graph.target(ait)));
		savedP.append(p[ait]);
		savedConjunctive.append(conjunctive[ait]);
	}
	 */

	//out1 << "Saved PM: " << *this << endl;
}

void ProcessModel::restore() {
	if (!saved) {
		Debugger::err << "ProcessModel::restore : Trying to restore an empty PM!" << ENDL;
		return;
	}

	if (saved) {

		// clearCurrent();

		// #######  TESTING
		// Remove all disjunctive arcs in the graph
		QList<ListDigraph::Arc> arcsToRem;
		for (ListDigraph::ArcIt ait(graph); ait != INVALID; ++ait) {
			ListDigraph::Arc curArc = ait;

			if (!conjunctive[ait]) arcsToRem.append(curArc);
		}

		for (int i = 0; i < arcsToRem.size(); i++) {
			ListDigraph::Arc curArc = arcsToRem[i];
			graph.erase(curArc);
		}

		// Restore the information about the operations
		for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
			ListDigraph::Node curNode = nit;
			*(ops[curNode]) = savedOps[curNode];
		}

		// Restore the DISJUNCTIVE arcs and the information about the arcs
		for (int i = 0; i < savedArcs.size(); i++) {
			if (!savedConjunctive[i]) { // This arc is disjunctive

				ListDigraph::Node curStartNode = savedArcs[i].first;
				ListDigraph::Node curEndNode = savedArcs[i].second;
				ListDigraph::Arc curArc = graph.addArc(curStartNode, curEndNode);

				p[curArc] = savedP[i];

				conjunctive[curArc] = savedConjunctive[i];
			}

		}

		// Restore the lengths of all arcs
		for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
			ListDigraph::Node curNode = nit;
			double curP = ops[curNode]->p();
			for (ListDigraph::OutArcIt oait(graph, curNode); oait != INVALID; ++oait) {
				ListDigraph::Arc curArc = oait;
				p[curArc] = -curP;
			}
		}

		// ################

		return;

		// Copy the saved graph into the current one
		ListDigraph::NodeMap<ListDigraph::Node> noderef(savedGraph);
		digraphCopy(savedGraph, graph).node(savedHead, head).node(savedTail, tail).nodeRef(noderef).arcMap(savedp, p).arcMap(savedconjunctive, conjunctive).run();

		// Create and copy the operations
		for (ListDigraph::NodeIt nit(savedGraph); nit != INVALID; ++nit) {
			this->ops[noderef[nit]] = new Operation(*savedops[nit]);
		}

		/*
		// Restore the operations
		for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		 *ops[nit] = *savedOps[ops[nit]->ID];
		}

		// Restore the arcs
		for (ListDigraph::ArcIt ait(graph); ait != INVALID; ++ait) {
			arcsdel.append(ait);
		}

		for (int i = 0; i < arcsdel.size(); i++) {
			graph.erase(arcsdel[i]);
		}

		for (int i = 0; i < savedArcs.size(); i++) {
			arc = graph.addArc(savedArcs[i].first, savedArcs[i].second);
			p[arc] = savedP[i];
			conjunctive[arc] = savedConjunctive[i];
		}
		 */
	}

	//out1 << "Restored PM: " << *this << endl;
}

void ProcessModel::clear() {
	clearSaved();
	clearCurrent();
}

void ProcessModel::clearCurrent() {
	// Delete the operations
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		delete ops[nit];
	}

	// Clear the saved graph
	graph.clear();

	head = INVALID;
	tail = INVALID;
}

void ProcessModel::clearSaved() {

	if (saved) {

		// TESTING
		savedOps.clear();
		savedArcs.clear();
		savedP.clear();
		savedConjunctive.clear();
		// TESTING

		saved = false;

		return;

		// Delete the saved operations
		for (ListDigraph::NodeIt nit(savedGraph); nit != INVALID; ++nit) {
			delete savedops[nit];
		}

		// Clear the saved graph
		savedGraph.clear();

		savedHead = INVALID;
		savedTail = INVALID;

		/*
		for (QHash<int, Operation* >::iterator iter = savedOps.begin(); iter != savedOps.end(); iter++) {
			delete iter.value();
		}

		savedOps.clear();

		savedArcs.clear();
		savedP.clear();
		savedConjunctive.clear();
		 */

	}

}

void ProcessModel::clearSchedRelData() {

	// Collect the selection arcs
	QList<ListDigraph::Arc> selectionArcs;

	selectionArcs.clear();
	for (ListDigraph::ArcIt ait(this->graph); ait != INVALID; ++ait) {
		if (!this->conjunctive[ait]) {
			selectionArcs.append(ait);
		}
		this->p[ait] = 0.0;
	}

	// Clear the scheduling relevant data
	for (ListDigraph::NodeIt nit(this->graph); nit != INVALID; ++nit) {
		this->ops[nit]->machID = -1;
		this->ops[nit]->machAvailTime(0.0);
		this->ops[nit]->r(this->ops[nit]->ir());
		this->ops[nit]->s(this->ops[nit]->r());
		this->ops[nit]->p(0.0);
	}

	// Clear the selection arcs
	for (int i = 0; i < selectionArcs.size(); i++) {
		this->graph.erase(selectionArcs[i]);
	}
}

bool ProcessModel::conPathExists(const ListDigraph::Node &s, const ListDigraph::Node & t) {
	QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
	ListDigraph::Node curnode;
	ListDigraph::NodeMap<bool> q_contains(graph, false); // Consider revising for optimization

	// Initialize the queue with the s' successors
	for (ListDigraph::OutArcIt oait(graph, s); oait != INVALID; ++oait) {
		// Consider only the conjunctive arcs
		if (conjunctive[oait]) {
			q.enqueue(graph.target(oait));
			q_contains[graph.target(oait)] = true;
		}
	}

	while (!q.empty()) {
		curnode = q.dequeue();

		//Debugger::info << "Queue size: " << q.size() << ENDL;

		// Iterate over the graph successors of the node which are reachable over the conjunctive arcs. 
		if (curnode == t) {
			return true;
		} else {
			for (ListDigraph::OutArcIt oait(graph, curnode); oait != INVALID; ++oait) {
				if (conjunctive[oait]) {
					if (!q_contains[graph.target(oait)]) {
						q.enqueue(graph.target(oait));
						q_contains[graph.target(oait)] = true;
					}
				}
			}
		}
	}

	return false;
}

QList<ListDigraph::Node> ProcessModel::reachableFrom(const QList<ListDigraph::Node>& s) {
	ListDigraph::NodeMap<bool > nodevisited(graph, false);
	QList<ListDigraph::Node> res;
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;
	ListDigraph::Node targetnode;

	q.reserve(countNodes(graph));
	res.reserve(countNodes(graph));

	for (int i = 0; i < s.size(); i++) {
		if (s[i] != INVALID) {
			q.append(s[i]);
		}
	}

	while (!q.empty()) {
		curnode = q.dequeue();

		if (!nodevisited[curnode]) {
			nodevisited[curnode] = true;
			res.append(curnode);

			// Check the successors
			for (ListDigraph::OutArcIt oait(graph, curnode); oait != INVALID; ++oait) {
				targetnode = graph.target(oait);
				if (!nodevisited[targetnode]) {
					q.enqueue(targetnode);
				}
			}
		}

	}

	return res;
}

QList<ListDigraph::Node> ProcessModel::topolSortReachableFrom(const QList<ListDigraph::Node>& s) {
	ListDigraph::NodeMap<bool > nodevisited(graph, false);
	QList<ListDigraph::Node> res;
	QStack<ListDigraph::Node> stack;
	ListDigraph::Node curnode;
	ListDigraph::Node pnode;
	ListDigraph::Node snode;
	QList<ListDigraph::Node> reachable = reachableFrom(s);

	// Reserve memory
	res.reserve(countNodes(graph));
	stack.reserve(countNodes(graph));

	for (int i = 0; i < s.size(); i++) {
		if (s[i] != INVALID) {
			stack.push(s[i]);
		}
	}

	bool psched;
	while (!stack.empty()) {
		curnode = stack.pop();

		if (!nodevisited[curnode]) { // This node has not been visited yet

			// Check whether all predecessors in reachable are scheduled
			psched = true;
			for (ListDigraph::InArcIt iait(graph, curnode); iait != INVALID; ++iait) {
				pnode = graph.source(iait);
				if (reachable.contains(pnode)) { // Consider only nodes which can be reached from s
					if (!nodevisited[pnode]) {
						psched = false;
						break;
					}
				}
			}

			if (psched) { // All predecessors have been visited
				res.append(curnode);
				nodevisited[curnode] = true;

				// Push the succeeding nodes
				for (ListDigraph::OutArcIt oait(graph, curnode); oait != INVALID; ++oait) {
					snode = graph.target(oait);
					if (!nodevisited[snode]) {
						stack.push(snode);
					}
				}
			} else {
				stack.prepend(curnode);
			}

		} // Else ignore the visited node
	}

	return res;
}

QList<ListDigraph::Node> ProcessModel::topolSort() {
	ListDigraph::NodeMap<int> nodes2pos(graph); // Map of nodes sorted topologically

	// Sort the nodes topologically
	topologicalSort(graph, nodes2pos);

	QMap<int, ListDigraph::Node> pos2nodes; // Topologically sorted nodes 

	for (ListDigraph::NodeMap<int>::MapIt mi(nodes2pos); mi != INVALID; ++mi) {
		pos2nodes[*mi] = mi;
	}

	QList<ListDigraph::Node> tnodes;

	tnodes.clear();
	tnodes.reserve(pos2nodes.size());
	for (QMap<int, ListDigraph::Node>::iterator sti = pos2nodes.begin(); sti != pos2nodes.end(); sti++) {
		tnodes << sti.value();
	}

	return tnodes;
}

QList<ListDigraph::Arc> ProcessModel::arcs(const ListDigraph::Node& s, const ListDigraph::Node & t) {
	QList<ListDigraph::Arc> res;

	for (ListDigraph::OutArcIt oait(graph, s); oait != INVALID; ++oait) {
		if (graph.target(oait) == t) {
			res.append(oait);
		}
	}

	return res;
}

QList<ListDigraph::Arc> ProcessModel::addArcs(const QList<QPair<ListDigraph::Node, ListDigraph::Node> >& arcNodes) {

	QList<ListDigraph::Arc> res;

	for (int i = 0; i < arcNodes.size(); i++) {
		ListDigraph::Node s = arcNodes[i].first;
		ListDigraph::Node t = arcNodes[i].second;

		ListDigraph::Arc a = graph.addArc(s, t);

		res.append(a);
	}

	return res;
}

void ProcessModel::removeArcs(const QList<ListDigraph::Arc>& arcsRem) {

	for (int i = 0; i < arcsRem.size(); i++) {
		ListDigraph::Arc a = arcsRem[i];

		graph.erase(a);
	}

}

QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > ProcessModel::longestPathsLen() {
	/**
	 * Algorithm:
	 * 
	 * 1. Apply the Bellman-Ford algorithm
	 * 
	 */

	Debugger::info << "ProcessModel::longestPathsLen : Searching the longest paths in the graph..." << ENDL;

	QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > res;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(graph, p);

	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) { // Iterate over all nodes in the graph

		ListDigraph::Node curNode = nit;

		// Rund the B-F algorithm
		bf.run(curNode);

		// Collect the result for the current node
		for (ListDigraph::NodeIt nitOther(graph); nitOther != INVALID; ++nitOther) {
			if (bf.reached(nitOther)) {
				res[nit][nitOther] = -bf.dist(nitOther); // Minus, since the arcs have negative weights
			}
		}

	}



	Debugger::info << "ProcessModel::longestPathsLen : Longest paths found." << ENDL;

	return res;
}

QTextStream& operator<<(QTextStream &out, ProcessModel & pm) {
	out << "PM: [" << endl;

	for (ListDigraph::NodeIt ni(pm.graph); ni != INVALID; ++ni) {
		out << *pm.ops[ni] << " &" << pm.ops[ni] << endl;
	}

	for (ListDigraph::ArcIt ai(pm.graph); ai != INVALID; ++ai) {
		out << "(" << pm.ops[pm.graph.source(ai)]->OID << ":" << pm.ops[pm.graph.source(ai)]->ID << "->"
				<< pm.ops[pm.graph.target(ai)]->OID << ":" << pm.ops[pm.graph.target(ai)]->ID << ", l = " << pm.p[ai] << ", "
				<< "con=" << ((pm.conjunctive[ai]) ? "1" : "0") << ")" << endl;
	}

	out << "]";

	return out;
}