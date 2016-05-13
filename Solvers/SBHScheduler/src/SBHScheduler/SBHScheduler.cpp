/* 
 * File:   SBHScheduler.cpp
 * Author: DrSobik
 * 
 * Created on January 17, 2012, 1:15 PM
 */

#include "SBHScheduler.h"

SBHScheduler::SBHScheduler() : dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {

}

SBHScheduler::SBHScheduler(SBHScheduler& orig) : LSScheduler(orig), IterativeAlg(), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	this->tgscheduler = (TGScheduler*) orig.tgscheduler->clone();

	this->tgscheduler->sbhscheduler = this;

	//TG2Selection = orig.TG2Selection;
}

SBHScheduler::~SBHScheduler() {

}

Clonable* SBHScheduler::clone() {
	return new SBHScheduler(*this);
}

void SBHScheduler::scheduleActions() {

	//Debugger::info << "SBHScheduler::scheduleActions : Initializing..." << ENDL;
	init();
	//Debugger::info << "SBHScheduler::scheduleActions : Done initializing." << ENDL;

	//Debugger::info << "SBHScheduler::scheduleActions : Running..." << ENDL;
	run();
	//Debugger::info << "SBHScheduler::scheduleActions : Done." << ENDL;

}

bool SBHScheduler::schedule(ProcessModel& pm, Resources& resources, Schedule& schedule) {
	QTextStream out(stdout);

	bool schedstatus = false;

	// Set local pointers for convenience
	this->pm = pm;
	this->rc = resources;
	this->sched = &schedule;

	//out << "SBHScheduler::schedule : PM :" << endl;
	//out << pm << endl;
	//getchar();

	// Initialize the scheduler
	init();

	// Run the scheduling algorithm
	run();

	// Prepare the resulting schedule
	schedstatus = true;
	return schedstatus;
}

void SBHScheduler::init() {
	QTextStream out(stdout);

	// Delete the arcs
	/*
	QList<ListDigraph::Arc> arcs;
	for (QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node > > >::iterator iter = TG2Arcs.begin(); iter != TG2Arcs.end(); iter++) {
		// Iterate over the arcs of each machine group
		for (int ca = 0; ca < iter.value().size(); ca++) {
			// Get the arc
			arcs = pm.arcs(iter.value()[ca].first, iter.value()[ca].second);

			// Delete the current arc from the graph
			for (int i = 0; i < arcs.size(); i++) {
				if (!pm.conjunctive[arcs[i]]) {
					pm.graph.erase(arcs[i]);
				}
			}
		}
	}
	 */

	clear();

	// Select the corresponding initial weights for every arc (average processing time of the source operation)
	// These weights will be updated as soon as the corresponding operations are scheduled on the defined machines
	// IMPORTANT: the lengths of the arcs are set to be negative, since the longest path problems are reduced to the shortest path problems and Bellman-Ford algorithm is applied (polynomial time complexity).
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (pm.ops[pm.graph.source(ait)]->ID <= 0) { // Fictive 
			pm.p[ait] = 0.0;
		} else {
			pm.p[ait] = -rc(pm.ops[pm.graph.source(ait)]->toolID)./*shortestProcTime(pm.ops[pm.graph.source(ait)]); //*/expectedProcTime(pm.ops[pm.graph.source(ait)]);
		}

		pm.ops[pm.graph.source(ait)]->p(-(pm.p[ait]));
	}

	//###########################  DEBUG  ######################################
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency during SBH initialization..." << endl;
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			if (pm.ops[nit]->p() != -pm.p[oait]) {
				out << "op ID = " << pm.ops[nit]->ID << endl;
				out << pm << endl;
				Debugger::err << "SBHScheduler::init : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	out << "Done checking consistency during SBH initialization." << endl;
	//##########################################################################

	// Consider the ready times for the initial nodes
	/*
	 QQueue<ListDigraph::Node> q;
	 ListDigraph::Node cur_node;

	 q.enqueue(pm.head);
	 while (q.size() > 0) {
		 cur_node = q.dequeue();
		 if (pm.ops[cur_node]->ID <= 0) {
			 for (ListDigraph::OutArcIt oait(pm.graph, cur_node); oait != INVALID; ++oait) {
				 q.enqueue(pm.graph.target(oait));
			 }
		 } else {
			 for (ListDigraph::InArcIt iait(pm.graph, cur_node); iait != INVALID; ++iait) {
				 pm.p[iait] = -pm.ops[cur_node]->ir();
			 }
		 }
	 }
	 */

	// Initialize the list of the terminal nodes and get the terminal due dates and the weights
	for (ListDigraph::InArcIt iait(pm.graph, pm.tail); iait != INVALID; ++iait) {
		terminals.append(pm.graph.source(iait));
		terminalopid2d[pm.ops[terminals.last()]->ID] = pm.ops[terminals.last()]->d();
		terminalopid2w[pm.ops[terminals.last()]->ID] = pm.ops[terminals.last()]->w();
	}

	// Initialize the set of unscheduled machines
	for (int i = 0; i < rc.tools.size(); i++) {
		M.insert(rc.tools[i]->ID);
	}

	// Initialize the mapping of the tool group onto the corresponding set of nodes
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		if (pm.ops[nit]->ID >= 0) {
			TG2Nodes[pm.ops[nit]->toolID].append(nit);
		}
	}

	// Topological ordering of the process model
	topolOrdering = pm.topolSort();

}

void SBHScheduler::clear() {
	// Clear the containers for preserved data
	_opid2preservedr.clear();
	_opid2preservedd.clear();
	_opid2preserveds.clear();
	_opid2preservedp.clear();
	_opid2preservedm.clear();
	_opid2preservedr.squeeze();
	_opid2preservedd.squeeze();
	_opid2preserveds.squeeze();
	_opid2preservedp.squeeze();
	_opid2preservedm.squeeze();

	// Initialize the list of the terminal nodes and get the terminal due dates and the weights
	terminals.clear();
	terminalopid2d.clear();
	terminalopid2w.clear();
	terminalopid2d.squeeze();
	terminalopid2w.squeeze();

	// Initialize the list of bottlenecks
	btnseq.clear();

	// Scheduled and unscheduled tool groups corresponding to the nodes of the search tree
	M0.clear();

	M.clear();

	// Initialize the mapping of the tool group onto the corresponding set of nodes
	TG2Nodes.clear();
	TG2Nodes.squeeze();

	// Delete the selection of the nodes, which the arcs are created of.
	TG2Selection.clear();
	TG2Selection.squeeze();

	TG2Arcs.clear();
	TG2Arcs.squeeze();

	dloc.clear();
	dloc.squeeze();
}

void SBHScheduler::preprocessingActions() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// Set the expected processing times
	double ept = 0.0;
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ept = rc(pm.ops[nit]->toolID).expectedProcTime(pm.ops[nit]);

		// Set the processing time
		pm.ops[nit]->p(ept);

		// Update the arcs
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			pm.p[oait] = -ept;
		}
	}

	//out << pm << endl;
	//getchar();

	// Get the topological ordering of the graph
	topolOrdering = pm.topolSort();

	// Update the ready times of the operations
	pm.updateHeads(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	//double smallestOrderS = Math::MAX_DOUBLE;
	//double smallestOrderD = Math::MAX_DOUBLE;
	//double orderTimeInt = 0.0;
	double smallestD = Math::MAX_DOUBLE;
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	QList<ListDigraph::Node> terminals = pm.terminals();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		lenRemain[nit] = 0;
	}

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		//smallestOrderS = Math::MAX_DOUBLE;
		//smallestOrderD = Math::MAX_DOUBLE;

		lenRemain[terminals[i]] = 1;
		pRemain[terminals[i]] = 0.0;
		dOrd[terminals[i]] = pm.ops[terminals[i]]->d();

		for (ListDigraph::InArcIt iait(pm.graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm.graph.source(iait);
			//stack.push(pm.graph.source(iait));
			q.enqueue(curpred);

			lenRemain[curpred] = 2;
			pRemain[curpred] = pm.ops[curpred]->p() + 0.0; // 0.0 - for the terminal node
			dOrd[curpred] = pm.ops[terminals[i]]->d();

			//Debugger::info << "Terminal : " << pm.ops[terminals[i]]->ID << ENDL;
			//Debugger::info << pm.ops[curpred]->ID << " : " << d[curpred] << ENDL;
			//getchar();
		}

		ordNodes.clear();
		while (/*!stack.empty()*/!q.empty()) {
			//curnode = stack.pop();
			curnode = q.dequeue();

			// Save the node 
			ordNodes.append(curnode);

			// Find the smallest wished start time of all successors
			double ss = Math::MAX_DOUBLE;
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				cursucc = pm.graph.target(oait);
				double k = 1.0; // As proposed by Vepsalainen and Morton
				ss = Math::min(ss, pm.ops[cursucc]->d() - k * pm.ops[cursucc]->p());
			}

			// Set the found time as the due date for the current node
			smallestD = Math::min(smallestD, ss);
			pm.ops[curnode]->d(ss);

			// Find the largest number of the operations to be processed after the current node (including the current node)
			ListDigraph::Node longerSucc = INVALID;
			double maxP = -1.0;
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				if (pRemain[pm.graph.target(oait)] > maxP) {
					maxP = pRemain[pm.graph.target(oait)];
					longerSucc = pm.graph.target(oait);
				}
			}
			//Debugger::info << "Found : " << pm.ops[longerSucc]->ID << ENDL;
			//getchar();
			if (longerSucc == INVALID) {
				Debugger::err << "ATCANScheduler::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
			}
			pRemain[curnode] = pRemain[longerSucc] + pm.ops[curnode]->p();
			lenRemain[curnode] = lenRemain[longerSucc] + 1;
			dOrd[curnode] = dOrd[longerSucc]; // Due date of the order

			//Debugger::info << pm.ops[longerSucc]->ID << " : " << d[longerSucc] << ENDL;
			//getchar();

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				curpred = pm.graph.source(iait);

				// Push the current predecessor into the queue
				q.enqueue(curpred);
			}
		}

		//getchar();
	}

	//Debugger::info << "SmallestD : " << smallestD << ENDL;

	//out << pm << endl;
	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//    out << "pRemain(" << pm.ops[nit]->ID << ") = " << pRemain[nit] << endl;
	//}
	//getchar();

	// Set the due dates based on the heads
	for (int i = 0; i < topolOrdering.size(); i++) {
		if (!terminals.contains(topolOrdering[i])) {
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 0.0 * smallestD);
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001);
			pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001, pm.ops[topolOrdering[i]]->r() + pm.ops[topolOrdering[i]]->p()));
			//pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001, pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 1.0 * smallestD));
			//d[topolOrdering[i]] -= 1.0 * smallestD;
		}
	}

	// Initialize the start times of the operations
	pm.updateStartTimes(topolOrdering);


	// Estimate a criticality measure for each machine group
	for (int i = 0; i < rc.tools.size(); i++) {

		ToolGroup& curTG = *(rc.tools[i]);

		if (curTG.ID == 0) {
			tgID2Criticality[curTG.ID] = 0;
			continue;
		}

		QList<ListDigraph::Node>& curNodes = TG2Nodes[curTG.ID];
		double totalP = 0.0;
		double m = (double) curTG.machines().size();

		// Get the total expected processing time of all operations on the machine group
		for (int j = 0; j < curNodes.size(); j++) {
			ListDigraph::Node curNode = curNodes[j];
			Operation& curOp = *(pm.ops[curNode]);

			totalP += curOp.p();
		}

		// Estimate the criticality as in Pinedo FFs
		tgID2Criticality[curTG.ID] = totalP / m;

		out << "Criticality of " << curTG.ID << " is " << tgID2Criticality[curTG.ID] << endl;
	}

}

void SBHScheduler::stepActions() {
	QTextStream out(stdout);

	/*
	// Estimate the longest paths between the nodes and the terminals
	QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > longestPaths = pm.longestPathsLen(); // The longest paths in the graph

	locD.clear();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		for (int i = 0; i < terminals.size(); i++) {

			ListDigraph::Node curTerm = terminals[i];

			locD[nit][curTerm] = pm.ops[curTerm]->d() - longestPaths[nit][curTerm];

		}

	}
	 */

	// Solve the subproblems for the unassigned tool groups and define the bottlenecks. Use the PM assigned to the curnode.
	out << "Searching bottlenecks..." << endl;
	int bottleneck = bottleneckTG(1);
	out << "Found bottlenecks." << endl;

	// Set of scheduled/unscheduled tool groups
	M0.insert(bottleneck);
	M.remove(bottleneck);
	btnseq << bottleneck;

	out << "Current bottleneck sequence : ";
	for (int i = 0; i < btnseq.size(); i++) {
		out << btnseq[i] << ",";
	}
	out << endl;

	out << "Before inserting bottleneck: " << bottleneck << " TWT (partial schedule, full recalculation) : " << TWT()(pm) << endl;
	out << "Arcs in the bottleneck's selection: " << TG2Selection[bottleneck].selection.size() << endl;

	/*
#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHScheduler::stepActions : Selection arcs are invalid before inserting the bottleneck!!!" << ENDL;
			}

		}
	}
#endif
	 */

	// Insert the selection for the selected bottleneck	
	insertTGSelection(bottleneck);

	/*	
	 * // Only relevant for classical reoptimization since the selections are updated there!!!
	#ifdef DEBUG
		for (int i = 0; i < btnseq.size(); i++) {
			int curTGID = btnseq[i];
			// Check correctness of the arcs
			QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
			for (int j = 0; j < curSelectionArcs.size(); j++) {
				ListDigraph::Arc curArc = curSelectionArcs[j];

				if (!pm.graph.valid(curArc)) {
					out << "Current TG : " << curTGID << endl;
					Debugger::err << "SBHScheduler::stepActions : Selection arcs are invalid after inserting the bottleneck!!!" << ENDL;
				}

			}
		}
	#endif
	 */

	// Recalculate heads and start times for the considered process model
	QList<ListDigraph::Node> topSort = pm.topolSort();
	pm.updateHeads(topSort);
	pm.updateStartTimes(topSort);

	out << "After inserting bottleneck: " << bottleneck << " TWT (partial schedule) : " << calculateObj() << endl;


	out << "After inserting bottleneck: " << bottleneck << " TWT (partial schedule, full recalculation) : " << TWT()(pm) << endl;

	// Perform reoptimization
	reoptimize(bottleneck);

	return;
}

void SBHScheduler::findPredecessorsSameTG() {
	QTextStream out(stdout);

	/*
	// Find operation predecessors for all nodes based on the topological ordering
	node2predST.clear();
	node2predST[topolOrdering[0]].clear();
	node2predST[topolOrdering[0]].append(QList<ListDigraph::Node>());
	node2predST[topolOrdering[0]].last().clear();
	ListDigraph::Node prevnode;
	ListDigraph::Node curnode;
	for (int i = 1; i < topolOrdering.size(); i++) {
		//Debugger::info << (int) pm.ops[topolOrdering[i]]->ID << ENDL;
		// Iterate over all the direct predecessors of the node
		curnode = topolOrdering[i];
		node2predST[curnode].clear();
		for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
			prevnode = pm.graph.source(iait);

			// Inherit all arrays
			//out << "Inheriting arrays of : " << pm.ops[prevnode]->ID << endl;
			for (int j = 0; j < node2predST[prevnode].size(); j++) {
				node2predST[curnode].append(node2predST[prevnode][j]);
				node2predST[curnode].last().append(prevnode);
			}

			// Appen the previous node to the arrays of this node
			if (node2predST[curnode].size() == 0) node2predST[curnode].append(QList<ListDigraph::Node>());

			for (int j = 0; j < node2predST[curnode].size(); j++) {
				node2predST[curnode][j].append(prevnode);
			}

			// TO-DO : Think on the possibility of cutting the previous node
		}

		//out << "Tool of the current node : " << (int) pm.ops[curnode]->toolID << endl;
		//out << "Predecessor tools : " << endl;
		//for (int j = 0; j < node2predST[curnode].size(); j++) {
		//    for (int k = 0; k < node2predST[curnode][j].size(); k++) {
		//	out << pm.ops[node2predST[curnode][j][k]]->toolID << ",";
		//    }
		//}
		//out << endl;
		//getchar();

	}

	QHash<int, ListDigraph::Node> nodeID2node;
	nodeID2node.clear();
	for (int i = 0; i < topolOrdering.size(); i++) {
		nodeID2node[pm.ops[topolOrdering[i]]->ID] = topolOrdering[i];
	}

	// Now cut everything in order to obtain only (semi-)direct predecessors
	for (int i = 1; i < topolOrdering.size(); i++) {
		curnode = topolOrdering[i];

		// For the current node sort all arrays by their length
		QMultiMap<int, QList<ListDigraph::Node>* > len2array;

		for (int j = 0; j < node2predST[curnode].size(); j++) {
			// Find index of the last element of the same size
			int k = -1;
			for (k = node2predST[curnode][j].size() - 1; k >= 0; k--) {
				if (pm.ops[node2predST[curnode][j][k]]->toolID == pm.ops[curnode]->toolID) break;
			}
			if (k >= 0) {
				len2array.insertMulti(k, &(node2predST[curnode][j]));
			}
		}

		QSet<int> dirpreds; // Set of direct predecessors (IDs)

		QMultiMap<int, QList<ListDigraph::Node>* >::iterator iter;
		QMultiMap<int, QList<ListDigraph::Node>* >::iterator rbegin = len2array.end();
		rbegin--;
		QMultiMap<int, QList<ListDigraph::Node>* >::iterator rend = len2array.begin();
		rend--;
		for (iter = rbegin; iter != rend; iter--) {
			// Get the last element of the type of curnode
			if (!dirpreds.contains(pm.ops[iter.value()->at(iter.key())]->ID)) {
				dirpreds.insert(pm.ops[iter.value()->at(iter.key())]->ID);
				break;
			}

			//iter.value()->clear();
		}

		//Debugger::info << (int) dirpreds.size() << ENDL;
		//getchar();

		node2predST[curnode].clear();
		node2predST[curnode].append(QList<ListDigraph::Node>());
		node2predST[curnode][0].clear();
		// Restore the nodes from the IDs
		for (QSet<int>::iterator iter = dirpreds.begin(); iter != dirpreds.end(); iter++) {
			node2predST[curnode][0].append(nodeID2node[*iter]);
		}
	}
	 */

	// Preform depth-first search for the graph of the pm
	ListDigraph::NodeMap<int> v(pm.graph);
	ListDigraph::NodeMap<int> l(pm.graph);

	QStack<ListDigraph::Node> s;
	QStack<ListDigraph::Node> path;
	ListDigraph::Node curnode;
	ListDigraph::Node sucnode;
	ListDigraph::Node prednode;

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		v[nit] = -1;
		l[nit] = -1;
	}

	s.push(pm.head);
	v[pm.head] = -1;
	l[pm.head] = -1;

	int step = 0;

	while (!s.isEmpty()) {
		curnode = s.pop();

		// Update the visiting time
		if (v[curnode] < 0) { // This node has not been visited yet. His successors have not been visited.
			v[curnode] = step;

			// Add this node to the path
			path.push(curnode);

			// Push the successors of the node
			int nsuc = 0;
			bool allpredvis = true;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				allpredvis = allpredvis && v[pm.graph.source(iait)] > 0;
			}

			if (allpredvis) {

				for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
					sucnode = pm.graph.target(oait);
					s.push(sucnode);
					nsuc++;
				}

				// If there are no successors then move backwards by the path and set the leave times until a node is found with not all successors nodes visited
				if (nsuc == 0) {
					bool allsucvis = true;
					do {
						curnode = path.pop();

						l[curnode] = step;

						allsucvis = true;
						for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
							allsucvis = allsucvis && v[pm.graph.target(oait)] > 0;
						}

						step++;
					} while (allsucvis || path.isEmpty());

					if (!allsucvis) {
						path.push(curnode);
					}

				} else {
					step++;
				}
			} else {
				bool allsucvis = true;
				do {
					curnode = path.pop();

					l[curnode] = step;

					allsucvis = true;
					for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
						allsucvis = allsucvis && v[pm.graph.target(oait)] > 0;
					}

					step++;
				} while (allsucvis || path.isEmpty());

				if (!allsucvis) {
					path.push(curnode);
				}
			}
		} else { // This node has already been visited, as well as his successors
			l[curnode] = step;
		}
	}

	// Build the predecessor sets based on the topological ordering
	node2predST.clear();
	QMap<ListDigraph::Node, QSet<int> > node2predIDs;
	node2predIDs.clear();
	QList<ListDigraph::Node> tmptopord;
	for (int i = 0; i < topolOrdering.size(); i++) {
		curnode = topolOrdering[i];
		node2predIDs[curnode].clear();

		// Copy only relevant nodes. Identify potential predecessors (direct and indirect)
		tmptopord.clear();
		for (int j = 0; j < i; j++) {
			if (pm.ops[topolOrdering[j]]->toolID == pm.ops[curnode]->toolID) {
				if (!(v[curnode] > l[topolOrdering[j]] || v[topolOrdering[j]] > l[curnode])) {
					tmptopord.append(topolOrdering[j]);
				}
			}
		}

		/*
		// Take one node from the back and eliminate its predecessors
		QList<ListDigraph::Node> tmptopord1;

		int szbefore;
		do {
			szbefore = tmptopord.size();
			tmptopord1.clear();
			for (int j = 0; j < tmptopord.size() - 1; j++) {
				if (v[curnode] > l[tmptopord[j]] || v[tmptopord[j]] > l[curnode]) { // Not predecessor of the last element
					tmptopord1.append(tmptopord[j]);
				}
				tmptopord1.append(tmptopord.last());
			}
			tmptopord = tmptopord1;
		} while (tmptopord.size() < szbefore);
		 */

		// Now all the predecessors are selected (direct)
		node2predST[curnode].clear();
		node2predST[curnode].append(tmptopord);
	}

}

int SBHScheduler::bottleneckTG(const int&) {
	/** Algorithm:
	 * 
	 * Precondition: Tool groups corresponding to the search tree node must be 
	 *				 scheduled in the given sequence. The state of the graph
	 *				 corresponding to the node must be restored.
	 * 
	 * 1. Iterate over all unscheduled tool groups
	 * 2. For every tool group formulate the corresponding subproblem:
	 * 2.1. update the corresponding ready times and due dates of the corresponding operations,
	 * 2.2. schedule the formulated supproblem under consideration of the DPC,
	 * 3. Select at most n bottlenecks. If the bottlenecks can not be distinguished
	 *	  by the means of the local objective function then the one with the 
	 *    greatest remaining processing time has a higher priority. 
	 * 4. Shift the operations of the already scheduled tool groups in view of 
	 *    the bottleneck to ensure feasibility of the solution (rcrc).
	 * 5. Return the found bottlenecks.
	 * 
	 *  */

	QTextStream out(stdout);

	// Find topological ordering of the graph
	topolOrdering = pm.topolSort();

//	int nbottlenecks = n;
	int res;

	// TO-DO: Restore the state of the graph corresponding to the current node of the search tree


	//Debugger::info << "Scheduling the unscheduled tool groups ... " << ENDL;

	//out << "Solving subproblems for unscheduled tool groups ..." << endl;

	//out << "Graph before preserving : " << endl << pm[node] << endl;

	//    foreach(const int &tgid, M) {
	//out << "Preserving state of the operations for tg : " << tgid << endl;
	//        preserveOpSchedState(pm, TG2Nodes[tgid]);
	//out << "Preserved state for : " << tgid << endl;
	//out << "Preserved state of the operations." << endl;
	//    }

	//QTime tupdate;
	//int msupdate;

	//tupdate.start();
	pm.updateHeads(topolOrdering); // TODO : Optimize using the topological ordering
	pm.updateStartTimes(topolOrdering); // TODO : Optimize using the topological ordering
	//msupdate = tupdate.elapsed();
	//Debugger::info << "SBHScheduler::bottleneckTG : Updating time (ms) "<<msupdate<<ENDL;
	//getchar();

//	foreach(const int &tgid, M) {

		// Iterate over the operations assigned to this tool group
		//opnodes = toolid2nodes[tgid];
		//for (int i = 0; i < opnodes.size(); i++) {
		// Update the head of the operation
		//Debugger::info << "Updating operation heads and tails for " << toolid2nodes[tgid].size() << " nodes" << ENDL;
		//Debugger::info << "The number of arcs in graph: " << countArcs(pm.graph) << ENDL;
		//Debugger::info << "Updating operation heads ..." << ENDL;
		//updateOperationHeads(toolid2nodes[tgid]);

		//out << "Recalculating heads and start times..." << endl;
		//	pm.updateHeads(); // TODO : Optimize using the topological ordering
		//	pm.updateStartTimes(); // TODO : Optimize using the topological ordering
		//out << "Recalculated heads and start times..." << endl;
		//Debugger::info << "Done updating operation heads." << ENDL;

		// Update the tails of the operation
		//Debugger::info << "Updating operation tails ..." << ENDL;
		//out << "Updating operation tails..." << endl;
		//		updateOperationTails(pm, TG2Nodes[tgid]); // TODO : Optimize using the topological ordering
		//out << "Updated operation tails." << endl;
		//Debugger::info << "Done updating operation tails." << ENDL;
		//}

//	}

	/** Find predecessors. */
	findPredecessorsSameTG();

//	foreach(const int &tgid, M) {
		// Schedule the operations on the current tool group
		//Debugger::info << "Scheduling the unscheduled tool group " << tgid << " ... " << ENDL;
		//out << pm << endl;
		//		scheduleTG(tgid);
		//Debugger::info << "Done scheduling the tool group." << ENDL;
//	}

	//out << "Solved subproblems for unscheduled tool groups." << endl << endl;

	//Debugger::info << "Done scheduling the unscheduled tool groups." << ENDL;
	//getchar();

	// Select the bottleneck tool groups
	QMultiMap<double, int> locobj2tgid;

	foreach(const int &tgid, M) {

		//out << "Local objective for tool group with ID = " << tgid << " is :  " << TG2Selection[tgid].localobj << endl;

		locobj2tgid.insert(TG2Selection[tgid].localobj, tgid);
	}
	out << endl;


	QList<int> bottleneckids;

	QMultiMap<double, int>::iterator rbegin = locobj2tgid.end();
	rbegin--;
	QMultiMap<double, int>::iterator rend = locobj2tgid.begin();
	rend--;


	for (QMultiMap<double, int>::iterator iter = rbegin; iter != rend; iter--) {
		//if (nbottlenecks == bottleneckids.size()) break;

		bottleneckids.append(iter.value());
	}


	// Clear the selections for the other tool groups which are in M
	//res = bottleneckids[Rand::rndInt(0, bottleneckids.size() - 1)];
	res = bottleneckids[Rand::rnd<Math::uint32>(0, bottleneckids.size() - 1)];

	// Select the bottleneck with the highest criticality measure
	double curCrit = -1.0;
	for (int i = 0; i < bottleneckids.size(); i++) {
		int curTGID = bottleneckids[i];
		if (tgID2Criticality[curTGID] > curCrit) {
			curCrit = tgID2Criticality[curTGID];
			res = curTGID;
		}
	}

	scheduleTG(res);

	//out << "Found bottleneck tg : " << res << endl;
	//getchar();

	foreach(const int &tgid, M) {
		if (tgid != res) {
			TG2Selection.remove(tgid);
		}
	}

	// Unset the machine assignment for the operations from the other TGs not in M
	// Assign the expected processing times for the outgoing arcs

	//    foreach(const int &tgid, M) {
	//        if (tgid != res) {
	//            //out << "Restoring tg: " << tgid << endl;
	//            restoreOpSchedState(pm, TG2Nodes[tgid]);
	//        } else {

	//        }
	//    }


	//out << "Graph after restoring : " << endl << pm[node] << endl;

	//pm.updateHeads();
	//pm.updateStartTimes();

	return res;
}

void SBHScheduler::scheduleTG(const int tgid) {
	QTextStream out(stdout);

	out << "SBHScheduler::scheduleTG : Trying to schedule machine group : " << tgid << endl;

	if (tgid == 0) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	if (((rc) (tgid)).types.size() == 1 && ((rc) (tgid)).types.contains(0)) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	//out << "Getting terminals ..." << endl;
	QList<ListDigraph::Node> terminals = pm.terminals();
	//out << "Got terminals." << endl;

	//TGScheduler *tgscheduler;

	/*
	tgscheduler = new TGVNSScheduler1;
	((TGVNSScheduler1*) tgscheduler)->maxIter(0000);
	((TGVNSScheduler1*) tgscheduler)->maxIterDecl(3000);
	((TGVNSScheduler*) tgscheduler)->maxTimeMs(30000);
	((TGVNSScheduler1*) tgscheduler)->sbhscheduler = this;
	 */


	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency before scheduling the tool group..." << endl;
	for (ListDigraph::NodeIt nit(pm[node]->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm[node]->graph, nit); oait != INVALID; ++oait) {
			if (pm[node]->ops[nit]->p() != -pm[node]->p[oait]) {
				out << "op ID = " << pm[node]->ops[nit]->ID << endl;
				out << pm[node] << endl;
				Debugger::err << "SBHScheduler::streeScheduleTG : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	//###########################  DEBUG  ######################################
	//out << "Nodes to for scheduling on the TG : " << tgid << endl;
	//for (int i = 0; i < streeTG2Nodes[tgid].size(); i++) {
	//out << pm.ops[streeTG2Nodes[tgid][i]] << endl;
	//}
	//getchar();
	//##########################################################################


	//out << "Running the tool group scheduler..." << endl;

	tgscheduler->node2predST = &node2predST;

	tgscheduler->locD = locD; // Set the local due dates of the operations

	TG2Selection[tgid].tgID = tgid;

#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHScheduler::bottleneckTG : Selection arcs are invalid before scheduling the unscheduled machine groups!!!" << ENDL;
			}

		}
	}
#endif	

	tgscheduler->schedule(pm, (rc) (tgid), TG2Nodes[tgid], terminals, dloc, TG2Selection[tgid]);
	//out << "Done running the scheduler." << endl;

	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency after scheduling the tool group..." << endl;
	for (ListDigraph::NodeIt nit(pm[node]->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm[node]->graph, nit); oait != INVALID; ++oait) {
			if (pm[node]->ops[nit]->p() != -pm[node]->p[oait]) {
				out << "op ID = " << pm[node]->ops[nit]->ID << endl;
				out << pm[node] << endl;
				Debugger::err << "SBHScheduler::streeScheduleTG : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */

	/*
	out << "SBHScheduler::streeScheduleTG : Selection:" << endl;
	for (int i = 0; i < streeTG2Selection[node][tgid].selection.size(); i++) {
		out << pm.ops[streeTG2Selection[node][tgid].selection[i].first] << endl;
	}

	double pt;
	ListDigraph::Node s;
	out << "SBHScheduler::streeScheduleTG : Checking processing times..." << endl;
	for (int i = 0; i < streeTG2Selection[node][tgid].selection.size(); i++) {
		s = streeTG2Selection[node][tgid].selection[i].first;
		if (pm[node]->ops[s]->machID >= 0) {
			pt = ((rc)(tgid, pm[node]->ops[s]->machID)).procTime(pm[node]->ops[s]);

			if (pm[node]->ops[s]->p() != pt) {
				//out << pm << endl;
				out << "pt = " << pt << endl;
				out << "p = " << pm[node]->ops[s]->p() << endl;
				out << pm[node]->ops[s] << endl;
				Debugger::err << "Something is wrong with the processing time for " << pm[node]->ops[s]->ID << ENDL;
			}
		}
	}
	out << "SBHScheduler::streeScheduleTG : Done checking processing times." << endl;
	 */
	//##########################################################################

	//delete tgscheduler;

#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHScheduler::bottleneckTG : Selection arcs are invalid after scheduling the unscheduled machine groups!!!" << ENDL;
			}

		}
	}
#endif

	out << "SBHScheduler::scheduleTG : Scheduled TG with ID = " << tgid << " with local objective : " << TG2Selection[tgid].localobj << endl << endl << endl;
	//getchar();
}

void SBHScheduler::reoptScheduleTG(const int tgid) {
	if (((rc) (tgid)).types.size() == 1 && ((rc) (tgid)).types.contains(0)) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	QList<ListDigraph::Node> terminals = pm.terminals();

	TGScheduler *tgscheduler;
	//if (streeInitSolMode) {
	tgscheduler = new TGVNSScheduler; //*/ TGFIFOScheduler;
	((TGVNSScheduler*) tgscheduler)->maxIterDecl(0);
	((TGVNSScheduler*) tgscheduler)->sbhscheduler = this;
	//} else {
	//tgscheduler = new TGATCScheduler;
	//}

	tgscheduler->node2predST = &node2predST;

	tgscheduler->schedule(pm, (rc) (tgid), TG2Nodes[tgid], terminals, dloc, TG2Selection[tgid]);

	delete tgscheduler;
}

void SBHScheduler::insertTGSelection(const int tgid) {
	QTextStream out(stdout);


	insertSelection(tgid);


	return;
}

void SBHScheduler::removeTGSelection(const int tgid) {

	removeSelection(tgid);

	return;
}

double SBHScheduler::calculateLocalObj(const int tgid) {
	// Calculate the local objective for the current selection
	//SBHTWTLocalObj lobj;
	UTWT utwt;

	QList<ListDigraph::Node> terminals = pm.terminals();

	double res = utwt(pm, TG2Nodes[tgid], locD); //lobj(pm, TG2Nodes[tgid], TG2Selection[tgid], terminals, dloc);

	return res;
}

double SBHScheduler::calculateObj() {
	TWT twt;

	return twt(pm, terminals);
}

void SBHScheduler::updateOperationHeads(const QList<ListDigraph::Node>& opnodes) {
	/**Algorithm: (IMPORTANT: arc lengths are negative)
	 * 
	 * 1. Run the Bellman-Ford algorithm on the graph with the negative 
	 *    arc weights and find the shortest path from the source to the given
	 *	  operation.
	 * 
	 * 2. Set the negated found length of the path as the ready time of the operation. 
	 * 
	 */

	//Debugger::info << "Updating operation heads ..." << ENDL;

	//QTextStream out(stdout);
	//out << pm << endl;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm.graph, pm.p);

	bf.init();
	bf.addSource(pm.head);
	//Debugger::info << "Running the BF algorithm..."<<ENDL;
	bf.start();
	//Debugger::info << "Done running the BF algorithm."<<ENDL;


	// #### IMPORTANT  #### Loca ready times of the operations must be updated, but the initial ready times must be considered
	// Update the ready time of the operation
	for (int i = 0; i < opnodes.size(); i++) {
#ifdef DEBUG
		if (!bf.reached(opnodes[i])) {
			Debugger::err << "SBHScheduler::updateOperationHead : Operation ID= " << pm.ops[opnodes[i]]->OID << ":" << pm.ops[opnodes[i]]->ID << " can not be reached from the root node " << pm.ops[pm.head]->OID << ":" << pm.ops[pm.head]->ID << "!" << ENDL;
		}
#endif
		pm.ops[opnodes[i]]->r(-bf.dist(opnodes[i]));
		//out << "The found length: " << pm.ops[opnodes[i]]->r() << endl;
	}
	//getchar();

	//Debugger::info << "Done updating operation heads." << ENDL;
	//out << pm << endl;
	//getchar();
}

void SBHScheduler::updateOperationTails(ProcessModel& pm, QList<ListDigraph::Node>& opnodes) {
	// Set the due dates of the operations proportionally to their processing times


	return;


	//Debugger::info << "Updating operation tails ..." << ENDL;

	double cmax;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm.graph, pm.p);

	QList<ListDigraph::Node> terminals = pm.terminals();

	for (int j = 0; j < opnodes.size(); j++) {

		bf.run(opnodes[j]);

		dloc[pm.ops[opnodes[j]]->ID].clear();
		for (int i = 0; i < terminals.size(); i++) {
			// Update the i-th local due date equal to the longest path from the operation to the corresponding terminal
			if (!bf.reached(terminals[i])) {
				//Debugger::err << "SBHScheduler::updateOperationTail : Terminal " << pm.ops[terminals[i]]->OID << ":" << pm.ops[terminals[i]]->ID << " can not be reached from the operation ID= " << pm.ops[opnodes[j]]->OID << ":" << pm.ops[opnodes[j]]->ID << " !" << ENDL;
				dloc[pm.ops[opnodes[j]]->ID].append(double(Math::MAX_DOUBLE));
			} else {
				// The longest path from the current operation node to the current terminal
				dloc[pm.ops[opnodes[j]]->ID].append(-bf.dist(terminals[i]));
			}
		}
	}
	// Find the makespan
	bf.init();
	bf.addSource(pm.head);
	bf.start();

	double ept = 0.0;

	for (int j = 0; j < opnodes.size(); j++) {
		// Define the processing time depending on whether the operation is scheduled or not
		if (pm.ops[opnodes[j]]->machID > 0) { // Exact processing time
			ept = pm.ops[opnodes[j]]->p();
		} else { // Expected processing time
			ept = (rc) (pm.ops[opnodes[j]]->toolID).shortestProcTime(pm.ops[opnodes[j]]); //expectedProcTime(pm.ops[opnodes[j]]);
		}

		// Set the local due dates
		for (int i = 0; i < terminals.size(); i++) {
			cmax = -bf.dist(terminals[i]);
			if (dloc[pm.ops[opnodes[j]]->ID][i] < Math::MAX_DOUBLE) {
				dloc[pm.ops[opnodes[j]]->ID][i] = Math::max(cmax, terminalopid2d[pm.ops[terminals[i]]->ID]) - dloc[pm.ops[opnodes[j]]->ID][i] + ept;
#ifdef DEBUG
				if (dloc[pm.ops[opnodes[j]]->ID][i] < 0.0) {
					Debugger::eDebug("Local due date < 0!");
				}
#endif

			}
		}
	}
	//Debugger::info << "Done updating operation tails." << ENDL;
}

void SBHScheduler::reoptimize(const int last_bottleneck_id) {

	//if (last_bottleneck_id == 0) return;

	QTextStream out(stdout);

	// Simple reoptimization strategy
	if (options["SBH_REOPT_TYPE"] == "LS") { // Local search reoptimization

		reoptimizePM(last_bottleneck_id);

	} else if (options["SBH_REOPT_TYPE"] == "STD") { // Classical reoptimization

		reoptimizeSimple(last_bottleneck_id);

	} else if (options["SBH_REOPT_TYPE"] == "NONE") { // No reoptimization is performed

		// No reoptimization is performed

	} else {

		Debugger::err << "SBHScheduler::reoptimize : SBH_REOPT_TYPE not specified/correct !!!" << ENDL;

	}



	return;

	/*
	
	// IMPORTANT!!! Synchronize the list of arcs corresponding to the tool groups! The local search may change the selection!

	// Synchronize the arcs for the scheduled tool groups

	TG2Arcs.clear();
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (!pm.conjunctive[ait]) {
			//out << "SBHScheduler::reoptimize : Synchronizing arc : " << pm.ops[pm.graph.source(ait)]->ID << "->" << pm.ops[pm.graph.target(ait)]->ID << endl;
			TG2Arcs[pm.ops[pm.graph.source(ait)]->toolID].append(QPair<ListDigraph::Node, ListDigraph::Node > (pm.graph.source(ait), pm.graph.target(ait)));
		}
	}

	// TODO : Update the selections

	out << "---" << endl;
	//out << "Found objective of the partial schedule after reoptimization : " << streeCalculateObj(node) << endl << endl << endl;
	// Reoptimization strategy based on the neighborhoods of Mastrolilli/Gambardella and Mati/Perez/Lahloy
	if (M.size() == 0) {
		out << "Scheduled all tool groups." << endl;
		out << "Found objective of the full schedule after reoptimization : " << calculateObj() << endl;
		//getchar();
	}
	 */

}

void SBHScheduler::reoptimizeSimple(const int last_bottleneck_id) {

	QTextStream out(stdout);

	out << endl << endl;
	out << "Classical reoptimization ..." << endl;
	out << "Last bottleneck tool group : " << last_bottleneck_id << endl;

	bool solutionImproved = false; // Indicates whether at least one rescheduling improved the solution
	int imprCtr = 0; // Counter for solution improvements
	int machImprCtr = 5;

	double objBeforeReopt = TWT()(pm);
	double bestObj = TWT()(pm);

	do { // Repeat until there are no further solutions

		solutionImproved = false;

		for (int i = 0; i < btnseq.size(); i++) {
			int curTGID = btnseq[i];

			if (curTGID == last_bottleneck_id) continue;

			out << endl << endl << "SBHScheduler::reoptimizeSimple : Reoptimizing machine group : " << curTGID << endl;

			out << "SBHScheduler::reoptimizeSimple : TWT of the partial schedule before the considering the current TG : " << TWT()(pm) << endl;

#ifdef DEBUG			
			// Check correctness of the arcs
			QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
			for (int j = 0; j < curSelectionArcs.size(); j++) {
				ListDigraph::Arc curArc = curSelectionArcs[j];

				if (!pm.graph.valid(curArc)) {
					Debugger::err << "SBHScheduler::reoptimizeSimple : Selection arcs are invalid!!!" << ENDL;
				}

			}
#endif			

			// Preserve the previous selection of the machine group
			TGSelection prevSelection = TG2Selection[curTGID];

			// Remove the actual selection of the machine group
			removeTGSelection(curTGID);

			out << "SBHScheduler::reoptimizeSimple : TWT of the partial schedule after removing the current selection : " << TWT()(pm) << endl;

			// Reoptimize the current machine group, i.e., schedule it again
			scheduleTG(curTGID);

			// Get the actual selection
			TGSelection& curSelection = (TGSelection&) TG2Selection[curTGID];

			// Check whether the current selection is better than the previous one
			if (curSelection.localobj < bestObj) {

				out << "SBHScheduler::reoptimizeSimple : Accepted a better solution with objective : " << curSelection.localobj << endl;

				solutionImproved = true;

				if (curSelection.localobj > objBeforeReopt) {

					out << "TWT before reoptimization : " << objBeforeReopt << endl;
					out << "Accepted solution : " << curSelection.localobj << endl;

					Debugger::err << "SBHScheduler::reoptimizeSimple : Accepted a worse solution!!!" << ENDL;
				}

				bestObj = curSelection.localobj;

			} else { // Restore the previous selection

				out << "SBHScheduler::reoptimizeSimple : Declined a solution with objective : " << curSelection.localobj << endl;

				// Restore the previous selection
				TG2Selection[curTGID] = prevSelection;

			}

			// Insert the selection after the reoptimization into the graph
			insertTGSelection(curTGID);

		}

		imprCtr++;

	} while (solutionImproved && imprCtr < machImprCtr);

	double objAfterReopt = TWT()(pm);

	if (objAfterReopt > objBeforeReopt) {

		out << "TWT before reoptimization : " << objBeforeReopt << endl;
		out << "TWT after reoptimization : " << objAfterReopt << endl;

		Debugger::err << "SBHScheduler::reoptimizeSimple : Reoptimization found a worse solution!!!" << ENDL;
	}

	out << "SBHScheduler::reoptimizeSimple : TWT of the partial schedule after the reoptimization : " << TWT()(pm) << endl;

	//getchar();

	return;








	TGSelection prev_selection;
	QList<double> prev_dloc;

	double cur_part_obj;
	double best_part_obj = Math::MAX_DOUBLE;

	int no_improvement_steps = 0;
	int max_no_improvement_steps = 3;

	M0.remove(last_bottleneck_id);

	pm.updateHeads();
	pm.updateStartTimes();

	// Update the selections for the scheduled tool groups

	foreach(const int &tgid, M0) {
		// Update operations data in the selections
		for (QMap<ListDigraph::Node, Operation>::iterator iter = TG2Selection[tgid].opNode2SchedOps.begin(); iter != TG2Selection[tgid].opNode2SchedOps.end(); iter++) {
			iter.value().copy(*(pm.ops[iter.key()]));
		}

		// Update the local objective
		TG2Selection[tgid].localobj = calculateLocalObj(tgid);
		out << "TG " << tgid << " : " << TG2Selection[tgid].localobj << endl;

	}

	best_part_obj = calculateObj();

	QList<int> btlncks = btnseq;
	btlncks.removeLast();
	int tgid;

	do {
		if (btlncks.size() == 0) break;

		Rand::randPermut(btlncks);

		// Iterate over the sorted scheduled tool groups
		for (int i = 0; i < btlncks.size(); i++) {

			// Set current tool group ID
			tgid = btlncks[i];

			// Preserve the selection for the current tool group
			prev_selection = TG2Selection[tgid];
			prev_dloc = dloc[tgid];

			// Preserve the scheduling state of the tool group
			preserveOpSchedState(pm, TG2Nodes[tgid]);

			// Remove the current selection of the tool group
			out << "SBHScheduler::reoptimizeSimple : Removing selection for TG " << tgid << endl;
			removeTGSelection(tgid);

			// Recalculate heads and start times
			pm.updateHeads();
			pm.updateStartTimes();

			// Recalculate local due dates for the tool group
			//out << "Updating the local due dates..." << endl;
			//out << "Before the update:" << endl;
			//for (int i = 0; i < TG2Nodes[tgid].size(); i++) {
			//out << pm.ops[TG2Nodes[tgid][i]]->ID << " : ";
			//for (int j = 0; j < dloc[pm.ops[TG2Nodes[tgid][i]]->ID].size(); j++) {
			//   out << dloc[pm.ops[TG2Nodes[tgid][i]]->ID][j] << ", ";
			//}
			//out << endl;
			//}
			updateOperationTails(pm, TG2Nodes[tgid]);
			//out << "After the update:" << endl;
			//for (int i = 0; i < TG2Nodes[tgid].size(); i++) {
			//out << pm.ops[TG2Nodes[tgid][i]]->ID << " : ";
			//for (int j = 0; j < dloc[pm.ops[TG2Nodes[tgid][i]]->ID].size(); j++) {
			//   out << dloc[pm.ops[TG2Nodes[tgid][i]]->ID][j] << ", ";
			//}
			//out << endl;
			//}

			//out << "Before rescheduling the tool group : " << tgid << endl;
			//for (int j = 0; j < TG2Nodes[tgid].size(); j++) {
			//out << pm.ops[TG2Nodes[tgid][j]] << endl;
			//out << *(TG2Selection[tgid].opNode2SchedOps[TG2Nodes[tgid][j]]) << endl;

			//}
			//getchar();

			scheduleTG(tgid);

			//out << "TG " << tgid << " : " << TG2Selection[tgid].localobj << endl;
			//out << "Local obj :  " << calculateLocalObj(tgid) << endl;
			//for (QMap<ListDigraph::Node, Operation*>::iterator iter = TG2Selection[tgid].opNode2SchedOps.begin(); iter != TG2Selection[tgid].opNode2SchedOps.end(); iter++) {
			//out << *iter.value() << endl;
			//}
			//getchar();
			//cur_part_obj = calculateObj();
			if (TG2Selection[tgid].localobj < prev_selection.localobj) {
				out << "Updated local obj for tg " << tgid << TG2Selection[tgid].localobj << endl;
			} else {

				TG2Selection[tgid] = prev_selection;
				dloc[tgid] = prev_dloc;

				restoreOpSchedState(pm, TG2Nodes[tgid]);

				//out << "R" << endl;
			}

			// Insert the new selection of the tool group
			//out << "...." << endl;
			insertTGSelection(tgid);
			//out << "....." << endl;

			// Recalculate heads
			pm.updateHeads();

			// Recalculate the completion times of the partial schedule
			pm.updateStartTimes();

			/*
			cur_part_obj = calculateObj();
			if (cur_part_obj > 1.0 * best_part_obj) {
				removeTGSelection(tgid);

				TG2Selection[tgid] = prev_selection;
				dloc[tgid] = prev_dloc;

				restoreOpSchedState(pm, TG2Nodes[tgid]);

				insertTGSelection(tgid);

				pm.updateHeads();
				pm.updateStartTimes();
			}
			 */

		}

		// Recalculate the current TWT for the partial schedule
		cur_part_obj = calculateObj();

		//out << "Currently found objective: " << cur_part_obj << endl;

		if (cur_part_obj < best_part_obj) {
			best_part_obj = cur_part_obj;

			no_improvement_steps = 0;
		} else {
			no_improvement_steps++;
		}

	} while (no_improvement_steps < max_no_improvement_steps);


	M0.insert(last_bottleneck_id);

	pm.updateHeads();
	pm.updateStartTimes();

	out << "Finished classical reoptimization." << endl;
}

void SBHScheduler::reoptimizeTG(const int&) {

}

void SBHScheduler::reoptimizePM(const int) {
	QTextStream out(stdout);

	QMap < ListDigraph::Node, bool> node2Movable;
	QMap<int, double> btnkID2Prob;

	for (int i = 0; i < btnseq.size(); i++) {
		int curBtnk = btnseq[i];
		btnkID2Prob[curBtnk] = double(btnseq.size() - i) / double(btnseq.size());
	}

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curNode]);

		if (curOp.toolID > 0 && btnseq.contains(curOp.toolID)) {
			//double rndNum = Rand::rndDouble();
			double rndNum = Rand::rnd<double>();

			if (rndNum <= btnkID2Prob[curOp.toolID]) { // Select with some probability as movable
				node2Movable[curNode] = true;
			} else {
				node2Movable[curNode] = false;
			}

		}

	}

	//out << "Running SBHScheduler::reoptimizePM... " << endl;
	int lsMaxIter = 0;
	int lsMaxTimeMs = 0;

	if (options["SBH_REOPT_LAST_ITER"] == "false") {

		lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt();
		lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt();

	} else if (options["SBH_REOPT_LAST_ITER"] == "true") { // Perform reoptimization only in the last iteration

		if (M.size() <= 0) {

			//ls.maxIter(100000); // Previously : 200000
			//ls.maxTimeMs(30*1000);

			lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt();
			lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt();

		} else {

			lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt() / (M0.size() + M.size());
			lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt() / (M0.size() + M.size());

		}

	} else {

		Debugger::err << "SBHScheduler::reoptimizePM : Not clear in which iteration to perform reoptimization!!!" << ENDL;

	}

	//cpls.maxIter(1000);

	ls.maxIter(lsMaxIter);
	ls.maxTimeMs(lsMaxTimeMs);

	//################  DEBUG  #################
	ls.setScheduledTGs(M0.toList());
	//##########################################

	ls.setPM(&pm);
	//ls.setMovableNodes(node2Movable);
	rc.init();
	ls.setResources(&rc);
	ls.checkCorrectness(false);
	//out << "Running the local search reoptimization ..." << endl;
	if (ls.maxIter() > 0) {
		ls.run();
	}

	// IMPORTANT!!! The selections in case of LS are not updated automatically!!! This may cause some validity problems if they are manipulated later

	return;
}

bool SBHScheduler::stopCondition() {
	bool stop = true;

	stop = M.size() == 0;

	return stop; //M.size() == 0;
}

void SBHScheduler::stopActions() {
	//Debugger::wDebug("SBHScheduler::stopActions not implemented!");
	QTextStream out(stdout);

	out << "SBHScheduler::stopActions : Final results:" << endl;

	out << "SBHScheduler::stopActions : The sequences of bottlenecks: " << endl;

	for (int i = 0; i < btnseq.size(); i++) {
		out << btnseq[i] << " ";
	}
	out << " | TWT = " << calculateObj();
	out << endl;
	out << endl;
	//getchar();

	//out << "Final schedule:" << endl << pm << endl;

	out << endl;
}

void SBHScheduler::postprocessingActions() {
	/** Algorithm:
	 * 
	 * 1. Calculate the total objective and generate the corresponding schedule.
	 * 
	 * 2. Remove the selections of all of the tool groups 
	 *    from the process model.
	 * 
	 * 3. Bring the process model to the initial state (state before 
	 *    the scheduling actions)
	 */

	QTextStream out(stdout);

	//out << "Performing postprocessing ..." << endl;

	sched->fromPM(pm,*obj);

	//Debugger::info << "Done collecting schedule data." << ENDL;
	Debugger::info << "Found objective value : " << sched->objective << ENDL;
	//return;
	//getchar();

	//out << "Postprocessing finished." << endl;
	//getchar();

	// Restore the initial state of the process model
	/*
	QList<ListDigraph::Arc> arcs;
	for (QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node > > >::iterator iter = TG2Arcs.begin(); iter != TG2Arcs.end(); iter++) {
		// Iterate over the arcs of each machine group
		for (int ca = 0; ca < iter.value().size(); ca++) {
			//out << "SBHScheduler::postprocessingActions : Considering to delete the arc : " << pm.ops[iter.value()[ca].first]->ID << "->" << pm.ops[iter.value()[ca].second]->ID << endl;

			// Get the arcs
			arcs = pm.arcs(iter.value()[ca].first, iter.value()[ca].second);

			// Delete the current arc from the graph
			for (int i = 0; i < arcs.size(); i++) {
				if (!pm.conjunctive[arcs[i]]) {
					//out << "SBHScheduler::postprocessingActions : Deleting arc : " << pm.ops[pm.graph.source(arcs[i])]->ID << "->" << pm.ops[pm.graph.target(arcs[i])]->ID << endl;
					pm.graph.erase(arcs[i]);
				}
			}
		}
	}
	TG2Arcs.clear();
	 */
	//out << "SBHScheduler::postprocessingActions : Done erasing SB arcs." << endl;

	// Restore the processing times and the arc lengths in the graph
	/*
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		// Set the ready times to the initial value
		pm.ops[nit]->r(pm.ops[nit]->ir());

		// Set the processing time to zero
		pm.ops[nit]->p(0.0);

		// Initialize the start times
		pm.ops[nit]->s(0.0);

		// Mark the operation as unscheduled
		pm.ops[nit]->machID = -1;

		// Set the length of the outgoing arcs
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[nit]->p();
		}
	}
	 */

	// Clear the scheduler
	clear();

	//out << "SBHScheduler::postprocessingActions : Restored the initial state of the PM." << endl;

	//out << pm << endl;
	//getchar();

}

void SBHScheduler::preserveOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedr[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->r();
	}
}

void SBHScheduler::restoreOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedr.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore r for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->r(_opid2preservedr.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHScheduler::preserveOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedd[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->d();
	}
}

void SBHScheduler::restoreOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedd.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore d for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->d(_opid2preservedd.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHScheduler::preserveOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedp[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->p();

		//Debugger::info << "Preserving (" << pm.ops[opnodes[i]]->ID << "," << pm.ops[opnodes[i]]->p() << ENDL;
	}
}

void SBHScheduler::restoreOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedp.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore p for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->p(_opid2preservedp.value(pm.ops[opnodes[i]]->ID));

			//Debugger::info << "Restoring (" << pm.ops[opnodes[i]]->ID << "," << pm.ops[opnodes[i]]->p() << ENDL;
		}
	}
}

void SBHScheduler::preserveOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preserveds[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->s();
	}
}

void SBHScheduler::restoreOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preserveds.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore s for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->s(_opid2preserveds.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHScheduler::preserveOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedm[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->machID;
	}
}

void SBHScheduler::restoreOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedm.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore m for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->machID = _opid2preservedm.value(pm.ops[opnodes[i]]->ID);
		}
	}
}

void SBHScheduler::preserveOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	preserveOpReadyTimes(pm, opnodes);
	preserveOpDueDates(pm, opnodes);
	preserveOpProcTimes(pm, opnodes);
	preserveOpStartTimes(pm, opnodes);
	preserveOpMachAssignment(pm, opnodes);
}

void SBHScheduler::restoreOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	restoreOpReadyTimes(pm, opnodes);
	restoreOpDueDates(pm, opnodes);
	restoreOpProcTimes(pm, opnodes);
	restoreOpStartTimes(pm, opnodes);
	restoreOpMachAssignment(pm, opnodes);
}

void SBHScheduler::restoreInitialR(QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		pm.ops[opnodes[i]]->r(pm.ops[opnodes[i]]->ir());
	}
}

void SBHScheduler::removeSelection(const int& tgID) {
	// Remove all arcs which have previously been inserted and CHANGE THE OPERATIONS
	QTextStream out(stdout);

	// Operations of the tool group
	QMap<ListDigraph::Node, Operation>& prevNodeOper = (QMap<ListDigraph::Node, Operation>&) (tgID2PrevNodeOper[tgID]);
	QList<ListDigraph::Arc>& selectionArcs = (QList<ListDigraph::Arc>&) (tgID2SelectionArcs[tgID]);

	//out << "SBHScheduler::removeSelection : TG : " << tgID << endl;
	//out << "SBHScheduler::removeSelection : Restoring nodes : " << prevNodeOper.size() << endl;
	//out << "SBHScheduler::removeSelection : Removing arcs : " << selectionArcs.size() << endl;

	if (prevNodeOper.size() == 0) { // In case there is nothing to remove
		selectionArcs.clear();
		return;
	}

	// Restore the operations
	for (QMap<ListDigraph::Node, Operation>::iterator iter = prevNodeOper.begin(); iter != prevNodeOper.end(); iter++) {
		ListDigraph::Node curNode = iter.key();
		Operation& curOper = (Operation&) iter.value();

		// Restore the operation
		*(pm.ops[curNode]) = curOper;

		// Set the correct outgoing arc lengths
		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
			pm.p[oait] = -curOper.p();
		}

	}

	// Remove the selection arcs
	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];

#ifdef DEBUG		
		if (!pm.graph.valid(curArc)) {
			Debugger::err << "SBHScheduler::removeSelection : Trying to remove an invalid arc!!!" << ENDL;
		};
#endif

		// Remove the arc
		pm.graph.erase(curArc);

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	// Clear the previous data
	selectionArcs.clear();
	prevNodeOper.clear();
	tgID2PrevNodeOper.remove(tgID);
	tgID2SelectionArcs.remove(tgID);

	/*
#ifdef DEBUG

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curStartNode]);

		if (curOp.toolID != tgID) continue; // Omit other machine groups

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm.graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm.graph.target(curArc);

			if (!pm.conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
				Debugger::info << "Current TG : " << tgID << ENDL;
				Debugger::err << "SBHScheduler::removeSelection : Too many disjunctive arcs between the nodes after removing the selection!!!" << ENDL;
			}

		}

	}
#endif
	 */

}

void SBHScheduler::insertSelection(const int& tgID) {
	QTextStream out(stdout);
	/*
	#ifdef DEBUG

		// Check whether there are multiple disjunctinve arcs between any two nodes
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
			ListDigraph::Node curStartNode = nit;
			Operation& curOp = (Operation&) *(pm.ops[curStartNode]);

			if (curOp.toolID != tgID) continue; // Omit other machine groups

			QMap<ListDigraph::Node, int> targetNode2Numarcs;

			for (ListDigraph::OutArcIt oait(pm.graph, curStartNode); oait != INVALID; ++oait) {
				ListDigraph::Arc curArc = oait;
				ListDigraph::Node curEndNode = pm.graph.target(curArc);

				if (!pm.conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

				if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
					Debugger::info << "Current TG : " << tgID << ENDL;
					Debugger::err << "SBHScheduler::insertSelection : Too many disjunctive arcs between the nodes before inserting the selection!!!" << ENDL;
				}

			}

		}
	#endif	
	 */

	// Remove the previous selection
	removeSelection(tgID);

	// Operations of the tool group
	QMap<ListDigraph::Node, Operation>& prevNodeOper = (QMap<ListDigraph::Node, Operation>&) (tgID2PrevNodeOper[tgID]);
	QList<ListDigraph::Arc>& selectionArcs = (QList<ListDigraph::Arc>&) (tgID2SelectionArcs[tgID]);
	TGSelection& selection = (TGSelection&) (TG2Selection[tgID]);

	//out << "SBHScheduler::insertSelection : TG : " << tgID << endl;
	//out << "SBHScheduler::insertSelection : Prev. nodes nodes : " << prevNodeOper.size() << endl;
	//out << "SBHScheduler::insertSelection : Prev. arcs arcs : " << selectionArcs.size() << endl;

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsToAdd = selection.selection;
	selectionArcs = pm.addArcs(arcsToAdd);

	QList<ListDigraph::Node> opsToChange = selection.opNode2SchedOps.keys();

	//cout << "Added arcs : " << arcsAdded.size() << endl;

	for (int i = 0; i < opsToChange.size(); i++) { // Insert the nodes to be changed. Important!!! Inserting arcs only is not enough since there can be only one operation on a machine

		ListDigraph::Node curNode = opsToChange[i];

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		// Preserve the previous operation data
		prevNodeOper[curNode] = *(pm.ops[curNode]);

		// Set the new operation data according to the selection
		*(pm.ops[curNode]) = curOper;

		// Iterate over the outgoing arcs of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
			pm.p[oait] = -curOper.p();
		}

	}

	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];
		ListDigraph::Node curNode = pm.graph.source(curArc);

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		pm.conjunctive[curArc] = false;

		pm.p[curArc] = -curOper.p();

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);
}

void SBHScheduler::debugCheckReachability(const int& mid, ProcessModel& pm) {
	QTextStream out(stdout);

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

	QList<ListDigraph::Node> trgmachnodes; // Nodes currently on the target machine
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;
	ListDigraph::NodeMap<bool> scheduled(pm.graph, false);
	ListDigraph::NodeMap<bool> available(pm.graph, false);

	ListDigraph::Node suc;
	ListDigraph::Node sucpred;

	q.enqueue(pm.head);
	scheduled[pm.head] = false;
	available[pm.head] = true;

	// Collect operation sequences on the target machine
	while (q.size() > 0) {
		curnode = q.dequeue();

		if (available[curnode] && !scheduled[curnode]) {
			if ((pm.ops[curnode]->ID > 0) && (pm.ops[curnode]->machID == mid)) {
				trgmachnodes.append(curnode);
			}

			scheduled[curnode] = true;

			// Enqueue the successors
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				suc = pm.graph.target(oait);
				if (!scheduled[suc]) {

					// Update availability

					available[suc] = true;
					for (ListDigraph::InArcIt iait(pm.graph, suc); iait != INVALID; ++iait) {
						sucpred = pm.graph.source(iait);
						if (!scheduled[sucpred]) {
							available[suc] = false;
							break;
						}
					}

					if (available[suc]) {
						q.enqueue(suc);
					}
				}
			}
		} else {
			if (!available[curnode]) {
				q.enqueue(curnode);
			}
		}

	}

	for (int j = 0; j < trgmachnodes.size() - 1; j++) {

		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (trgmachnodes.at(j), trgmachnodes.at(j + 1)));

	}

	if (trgmachnodes.size() > 0) {
		res.prepend(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, trgmachnodes.first()));
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (trgmachnodes.last(), INVALID));
	}

	// In case there are no operations on the target machine
	if (trgmachnodes.size() == 0) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, INVALID));
	}

	// ###################  DEBUG: can be deleted  #################################   

	/*
	out << "operations on machine " << mid << " : " << endl;
	for (int k = 0; k < trgmachnodes.size(); k++) {
		out << pm.ops[trgmachnodes[k]]->ID << ",";
	}

	out << endl << endl;
	 */
	//out << "GBM:" << endl;
	//out << pm << endl;

	for (int j = 0; j < res.size(); j++) {
		//	out << pm.ops[res[i].first]->ID << "->" << pm.ops[res[i].second]->ID << endl;
		if (!reachable(pm, res[j].first, res[j].second)) {
			out << "Not reachable : " << pm.ops[res[j].first]->ID << "->" << pm.ops[res[j].second]->ID << endl;

			out << "operations on machine " << mid << " : " << endl;
			for (int k = 0; k < trgmachnodes.size(); k++) {
				out << pm.ops[trgmachnodes[k]]->ID << ",";
			}

			out << endl << endl;

			out << pm << endl;
			getchar();
		}
	}

	// #############################################################################
}

bool SBHScheduler::reachable(ProcessModel& pm, const ListDigraph::Node& s, const ListDigraph::Node & t) {
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;

	q.enqueue(t);

	if (s == t) return true;

	if (s == INVALID || t == INVALID) return true;

	while (q.size() > 0) {
		curnode = q.dequeue();

		// Iterate over the predecessors
		for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
			if (pm.graph.source(iait) == s) {
				return true;
			} else {
				q.enqueue(pm.graph.source(iait));
			}
		}
	}

	return false;
}



/**  ********************  SBH with VNS  **********************************  **/

SBHSchedulerVNS::SBHSchedulerVNS() : dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {

}

SBHSchedulerVNS::SBHSchedulerVNS(SBHSchedulerVNS& orig) : LSScheduler(orig), IterativeAlg(), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	this->tgscheduler = (TGScheduler*) orig.tgscheduler->clone();

	this->tgscheduler->sbhscheduler = (SBHScheduler*) this;

	//TG2Selection = orig.TG2Selection;
}

SBHSchedulerVNS::~SBHSchedulerVNS() {

}

Clonable* SBHSchedulerVNS::clone() {
	return new SBHSchedulerVNS(*this);
}

void SBHSchedulerVNS::scheduleActions() {

	//Debugger::info << "SBHSchedulerVNS::scheduleActions : Initializing..." << ENDL;
	init();
	//Debugger::info << "SBHSchedulerVNS::scheduleActions : Done initializing." << ENDL;

	//Debugger::info << "SBHSchedulerVNS::scheduleActions : Running..." << ENDL;
	run();
	//Debugger::info << "SBHSchedulerVNS::scheduleActions : Done." << ENDL;

}

bool SBHSchedulerVNS::schedule(ProcessModel& pm, Resources& resources, Schedule& schedule) {
	QTextStream out(stdout);

	bool schedstatus = false;

	// Set local pointers for convenience
	this->pm = pm;
	this->rc = resources;
	this->sched = &schedule;

	//out << "SBHSchedulerVNS::schedule : PM :" << endl;
	//out << pm << endl;
	//getchar();

	// Initialize the scheduler
	init();

	// Run the scheduling algorithm
	run();

	// Prepare the resulting schedule
	schedstatus = true;
	return schedstatus;
}

void SBHSchedulerVNS::init() {
	QTextStream out(stdout);

	// Delete the arcs
	/*
	QList<ListDigraph::Arc> arcs;
	for (QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node > > >::iterator iter = TG2Arcs.begin(); iter != TG2Arcs.end(); iter++) {
		// Iterate over the arcs of each machine group
		for (int ca = 0; ca < iter.value().size(); ca++) {
			// Get the arc
			arcs = pm.arcs(iter.value()[ca].first, iter.value()[ca].second);

			// Delete the current arc from the graph
			for (int i = 0; i < arcs.size(); i++) {
				if (!pm.conjunctive[arcs[i]]) {
					pm.graph.erase(arcs[i]);
				}
			}
		}
	}
	 */

	clear();

	// Select the corresponding initial weights for every arc (average processing time of the source operation)
	// These weights will be updated as soon as the corresponding operations are scheduled on the defined machines
	// IMPORTANT: the lengths of the arcs are set to be negative, since the longest path problems are reduced to the shortest path problems and Bellman-Ford algorithm is applied (polynomial time complexity).
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (pm.ops[pm.graph.source(ait)]->ID <= 0) { // Fictive 
			pm.p[ait] = 0.0;
		} else {
			pm.p[ait] = -rc(pm.ops[pm.graph.source(ait)]->toolID)./*shortestProcTime(pm.ops[pm.graph.source(ait)]); //*/expectedProcTime(pm.ops[pm.graph.source(ait)]);
		}

		pm.ops[pm.graph.source(ait)]->p(-(pm.p[ait]));
	}

	//###########################  DEBUG  ######################################
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency during SBH initialization..." << endl;
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			if (pm.ops[nit]->p() != -pm.p[oait]) {
				out << "op ID = " << pm.ops[nit]->ID << endl;
				out << pm << endl;
				Debugger::err << "SBHSchedulerVNS::init : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	out << "Done checking consistency during SBH initialization." << endl;
	//##########################################################################

	// Consider the ready times for the initial nodes
	/*
	 QQueue<ListDigraph::Node> q;
	 ListDigraph::Node cur_node;

	 q.enqueue(pm.head);
	 while (q.size() > 0) {
		 cur_node = q.dequeue();
		 if (pm.ops[cur_node]->ID <= 0) {
			 for (ListDigraph::OutArcIt oait(pm.graph, cur_node); oait != INVALID; ++oait) {
				 q.enqueue(pm.graph.target(oait));
			 }
		 } else {
			 for (ListDigraph::InArcIt iait(pm.graph, cur_node); iait != INVALID; ++iait) {
				 pm.p[iait] = -pm.ops[cur_node]->ir();
			 }
		 }
	 }
	 */

	// Initialize the list of the terminal nodes and get the terminal due dates and the weights
	for (ListDigraph::InArcIt iait(pm.graph, pm.tail); iait != INVALID; ++iait) {
		terminals.append(pm.graph.source(iait));
		terminalopid2d[pm.ops[terminals.last()]->ID] = pm.ops[terminals.last()]->d();
		terminalopid2w[pm.ops[terminals.last()]->ID] = pm.ops[terminals.last()]->w();
	}

	// Initialize the set of unscheduled machines
	for (int i = 0; i < rc.tools.size(); i++) {
		M.insert(rc.tools[i]->ID);
	}

	// Initialize the mapping of the tool group onto the corresponding set of nodes
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		if (pm.ops[nit]->ID >= 0) {
			TG2Nodes[pm.ops[nit]->toolID].append(nit);
		}
	}

	// Topological ordering of the process model
	topolOrdering = pm.topolSort();

}

void SBHSchedulerVNS::clear() {
	// Clear the containers for preserved data
	_opid2preservedr.clear();
	_opid2preservedd.clear();
	_opid2preserveds.clear();
	_opid2preservedp.clear();
	_opid2preservedm.clear();
	_opid2preservedr.squeeze();
	_opid2preservedd.squeeze();
	_opid2preserveds.squeeze();
	_opid2preservedp.squeeze();
	_opid2preservedm.squeeze();

	// Initialize the list of the terminal nodes and get the terminal due dates and the weights
	terminals.clear();
	terminalopid2d.clear();
	terminalopid2w.clear();
	terminalopid2d.squeeze();
	terminalopid2w.squeeze();

	// Initialize the list of bottlenecks
	btnseq.clear();

	// Scheduled and unscheduled tool groups corresponding to the nodes of the search tree
	M0.clear();

	M.clear();

	// Initialize the mapping of the tool group onto the corresponding set of nodes
	TG2Nodes.clear();
	TG2Nodes.squeeze();

	// Delete the selection of the nodes, which the arcs are created of.
	TG2Selection.clear();
	TG2Selection.squeeze();

	TG2Arcs.clear();
	TG2Arcs.squeeze();

	dloc.clear();
	dloc.squeeze();
}

void SBHSchedulerVNS::preprocessingActions() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// Set the expected processing times
	double ept = 0.0;
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ept = rc(pm.ops[nit]->toolID).expectedProcTime(pm.ops[nit]);

		// Set the processing time
		pm.ops[nit]->p(ept);

		// Update the arcs
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			pm.p[oait] = -ept;
		}
	}

	//out << pm << endl;
	//getchar();

	// Get the topological ordering of the graph
	topolOrdering = pm.topolSort();

	// Update the ready times of the operations
	pm.updateHeads(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	//double smallestOrderS = Math::MAX_DOUBLE;
	//double smallestOrderD = Math::MAX_DOUBLE;
	//double orderTimeInt = 0.0;
	double smallestD = Math::MAX_DOUBLE;
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	QList<ListDigraph::Node> terminals = pm.terminals();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		lenRemain[nit] = 0;
	}

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		//smallestOrderS = Math::MAX_DOUBLE;
		//smallestOrderD = Math::MAX_DOUBLE;

		lenRemain[terminals[i]] = 1;
		pRemain[terminals[i]] = 0.0;
		dOrd[terminals[i]] = pm.ops[terminals[i]]->d();

		for (ListDigraph::InArcIt iait(pm.graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm.graph.source(iait);
			//stack.push(pm.graph.source(iait));
			q.enqueue(curpred);

			lenRemain[curpred] = 2;
			pRemain[curpred] = pm.ops[curpred]->p() + 0.0; // 0.0 - for the terminal node
			dOrd[curpred] = pm.ops[terminals[i]]->d();

			//Debugger::info << "Terminal : " << pm.ops[terminals[i]]->ID << ENDL;
			//Debugger::info << pm.ops[curpred]->ID << " : " << d[curpred] << ENDL;
			//getchar();
		}

		ordNodes.clear();
		while (/*!stack.empty()*/!q.empty()) {
			//curnode = stack.pop();
			curnode = q.dequeue();

			// Save the node 
			ordNodes.append(curnode);

			// Find the smallest wished start time of all successors
			double ss = Math::MAX_DOUBLE;
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				cursucc = pm.graph.target(oait);
				double k = 1.0; // As proposed by Vepsalainen and Morton
				ss = Math::min(ss, pm.ops[cursucc]->d() - k * pm.ops[cursucc]->p());
			}

			// Set the found time as the due date for the current node
			smallestD = Math::min(smallestD, ss);
			pm.ops[curnode]->d(ss);

			// Find the largest number of the operations to be processed after the current node (including the current node)
			ListDigraph::Node longerSucc = INVALID;
			double maxP = -1.0;
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				if (pRemain[pm.graph.target(oait)] > maxP) {
					maxP = pRemain[pm.graph.target(oait)];
					longerSucc = pm.graph.target(oait);
				}
			}
			//Debugger::info << "Found : " << pm.ops[longerSucc]->ID << ENDL;
			//getchar();
			if (longerSucc == INVALID) {
				Debugger::err << "ATCANScheduler::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
			}
			pRemain[curnode] = pRemain[longerSucc] + pm.ops[curnode]->p();
			lenRemain[curnode] = lenRemain[longerSucc] + 1;
			dOrd[curnode] = dOrd[longerSucc]; // Due date of the order

			//Debugger::info << pm.ops[longerSucc]->ID << " : " << d[longerSucc] << ENDL;
			//getchar();

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				curpred = pm.graph.source(iait);

				// Push the current predecessor into the queue
				q.enqueue(curpred);
			}
		}

		//getchar();
	}

	//Debugger::info << "SmallestD : " << smallestD << ENDL;

	//out << pm << endl;
	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//    out << "pRemain(" << pm.ops[nit]->ID << ") = " << pRemain[nit] << endl;
	//}
	//getchar();

	// Set the due dates based on the heads
	for (int i = 0; i < topolOrdering.size(); i++) {
		if (!terminals.contains(topolOrdering[i])) {
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 0.0 * smallestD);
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001);
			pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001, pm.ops[topolOrdering[i]]->r() + pm.ops[topolOrdering[i]]->p()));
			//pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001, pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 1.0 * smallestD));
			//d[topolOrdering[i]] -= 1.0 * smallestD;
		}
	}

	// Initialize the start times of the operations
	pm.updateStartTimes(topolOrdering);


	// Estimate a criticality measure for each machine group
	for (int i = 0; i < rc.tools.size(); i++) {

		ToolGroup& curTG = *(rc.tools[i]);

		if (curTG.ID == 0) {
			tgID2Criticality[curTG.ID] = 0;
			continue;
		}

		QList<ListDigraph::Node>& curNodes = TG2Nodes[curTG.ID];
		double totalP = 0.0;
		double m = (double) curTG.machines().size();

		// Get the total expected processing time of all operations on the machine group
		for (int j = 0; j < curNodes.size(); j++) {
			ListDigraph::Node curNode = curNodes[j];
			Operation& curOp = *(pm.ops[curNode]);

			totalP += curOp.p();
		}

		// Estimate the criticality as in Pinedo FFs
		tgID2Criticality[curTG.ID] = totalP / m;

		out << "Criticality of " << curTG.ID << " is " << tgID2Criticality[curTG.ID] << endl;
	}

}

void SBHSchedulerVNS::stepActions() {
	QTextStream out(stdout);

	// Estimate the longest paths between the nodes and the terminals
	QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > longestPaths = pm.longestPathsLen(); // The longest paths in the graph

	locD.clear();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		for (int i = 0; i < terminals.size(); i++) {

			ListDigraph::Node curTerm = terminals[i];

			locD[nit][curTerm] = pm.ops[curTerm]->d() - longestPaths[nit][curTerm];

		}

	}


	// Solve the subproblems for the unassigned tool groups and define the bottlenecks. Use the PM assigned to the curnode.
	out << "Searching bottlenecks..." << endl;
	int bottleneck = bottleneckTG(1);
	out << "Found bottlenecks." << endl;

	// Set of scheduled/unscheduled tool groups
	M0.insert(bottleneck);
	M.remove(bottleneck);
	btnseq << bottleneck;

	out << "Current bottleneck sequence : ";
	for (int i = 0; i < btnseq.size(); i++) {
		out << btnseq[i] << ",";
	}
	out << endl;

	out << "Before inserting bottleneck: " << bottleneck << " TWT (partial schedule, full recalculation) : " << TWT()(pm) << endl;
	out << "Arcs in the bottleneck's selection: " << TG2Selection[bottleneck].selection.size() << endl;

	/*
#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHSchedulerVNS::stepActions : Selection arcs are invalid before inserting the bottleneck!!!" << ENDL;
			}

		}
	}
#endif
	 */

	// Insert the selection for the selected bottleneck	
	insertTGSelection(bottleneck);

	/* Only relevant for classical reoptimization since the selections are updated there	
	#ifdef DEBUG
		for (int i = 0; i < btnseq.size(); i++) {
			int curTGID = btnseq[i];
			// Check correctness of the arcs
			QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
			for (int j = 0; j < curSelectionArcs.size(); j++) {
				ListDigraph::Arc curArc = curSelectionArcs[j];

				if (!pm.graph.valid(curArc)) {
					out << "Current TG : " << curTGID << endl;
					Debugger::err << "SBHSchedulerVNS::stepActions : Selection arcs are invalid after inserting the bottleneck!!!" << ENDL;
				}

			}
		}
	#endif
	 */

	// Recalculate heads and start times for the considered process model
	QList<ListDigraph::Node> topSort = pm.topolSort();
	pm.updateHeads(topSort);
	pm.updateStartTimes(topSort);

	out << "After inserting bottleneck: " << bottleneck << " TWT (partial schedule) : " << calculateObj() << endl;


	out << "After inserting bottleneck: " << bottleneck << " TWT (partial schedule, full recalculation) : " << TWT()(pm) << endl;

	//getchar();

	// Perform reoptimization for the considered process model
	reoptimize(bottleneck);


	//getchar();
	return;
}

void SBHSchedulerVNS::findPredecessorsSameTG() {
	QTextStream out(stdout);

	/*
	// Find operation predecessors for all nodes based on the topological ordering
	node2predST.clear();
	node2predST[topolOrdering[0]].clear();
	node2predST[topolOrdering[0]].append(QList<ListDigraph::Node>());
	node2predST[topolOrdering[0]].last().clear();
	ListDigraph::Node prevnode;
	ListDigraph::Node curnode;
	for (int i = 1; i < topolOrdering.size(); i++) {
		//Debugger::info << (int) pm.ops[topolOrdering[i]]->ID << ENDL;
		// Iterate over all the direct predecessors of the node
		curnode = topolOrdering[i];
		node2predST[curnode].clear();
		for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
			prevnode = pm.graph.source(iait);

			// Inherit all arrays
			//out << "Inheriting arrays of : " << pm.ops[prevnode]->ID << endl;
			for (int j = 0; j < node2predST[prevnode].size(); j++) {
				node2predST[curnode].append(node2predST[prevnode][j]);
				node2predST[curnode].last().append(prevnode);
			}

			// Appen the previous node to the arrays of this node
			if (node2predST[curnode].size() == 0) node2predST[curnode].append(QList<ListDigraph::Node>());

			for (int j = 0; j < node2predST[curnode].size(); j++) {
				node2predST[curnode][j].append(prevnode);
			}

			// TO-DO : Think on the possibility of cutting the previous node
		}

		//out << "Tool of the current node : " << (int) pm.ops[curnode]->toolID << endl;
		//out << "Predecessor tools : " << endl;
		//for (int j = 0; j < node2predST[curnode].size(); j++) {
		//    for (int k = 0; k < node2predST[curnode][j].size(); k++) {
		//	out << pm.ops[node2predST[curnode][j][k]]->toolID << ",";
		//    }
		//}
		//out << endl;
		//getchar();

	}

	QHash<int, ListDigraph::Node> nodeID2node;
	nodeID2node.clear();
	for (int i = 0; i < topolOrdering.size(); i++) {
		nodeID2node[pm.ops[topolOrdering[i]]->ID] = topolOrdering[i];
	}

	// Now cut everything in order to obtain only (semi-)direct predecessors
	for (int i = 1; i < topolOrdering.size(); i++) {
		curnode = topolOrdering[i];

		// For the current node sort all arrays by their length
		QMultiMap<int, QList<ListDigraph::Node>* > len2array;

		for (int j = 0; j < node2predST[curnode].size(); j++) {
			// Find index of the last element of the same size
			int k = -1;
			for (k = node2predST[curnode][j].size() - 1; k >= 0; k--) {
				if (pm.ops[node2predST[curnode][j][k]]->toolID == pm.ops[curnode]->toolID) break;
			}
			if (k >= 0) {
				len2array.insertMulti(k, &(node2predST[curnode][j]));
			}
		}

		QSet<int> dirpreds; // Set of direct predecessors (IDs)

		QMultiMap<int, QList<ListDigraph::Node>* >::iterator iter;
		QMultiMap<int, QList<ListDigraph::Node>* >::iterator rbegin = len2array.end();
		rbegin--;
		QMultiMap<int, QList<ListDigraph::Node>* >::iterator rend = len2array.begin();
		rend--;
		for (iter = rbegin; iter != rend; iter--) {
			// Get the last element of the type of curnode
			if (!dirpreds.contains(pm.ops[iter.value()->at(iter.key())]->ID)) {
				dirpreds.insert(pm.ops[iter.value()->at(iter.key())]->ID);
				break;
			}

			//iter.value()->clear();
		}

		//Debugger::info << (int) dirpreds.size() << ENDL;
		//getchar();

		node2predST[curnode].clear();
		node2predST[curnode].append(QList<ListDigraph::Node>());
		node2predST[curnode][0].clear();
		// Restore the nodes from the IDs
		for (QSet<int>::iterator iter = dirpreds.begin(); iter != dirpreds.end(); iter++) {
			node2predST[curnode][0].append(nodeID2node[*iter]);
		}
	}
	 */

	// Preform depth-first search for the graph of the pm
	ListDigraph::NodeMap<int> v(pm.graph);
	ListDigraph::NodeMap<int> l(pm.graph);

	QStack<ListDigraph::Node> s;
	QStack<ListDigraph::Node> path;
	ListDigraph::Node curnode;
	ListDigraph::Node sucnode;
	ListDigraph::Node prednode;

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		v[nit] = -1;
		l[nit] = -1;
	}

	s.push(pm.head);
	v[pm.head] = -1;
	l[pm.head] = -1;

	int step = 0;

	while (!s.isEmpty()) {
		curnode = s.pop();

		// Update the visiting time
		if (v[curnode] < 0) { // This node has not been visited yet. His successors have not been visited.
			v[curnode] = step;

			// Add this node to the path
			path.push(curnode);

			// Push the successors of the node
			int nsuc = 0;
			bool allpredvis = true;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				allpredvis = allpredvis && v[pm.graph.source(iait)] > 0;
			}

			if (allpredvis) {

				for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
					sucnode = pm.graph.target(oait);
					s.push(sucnode);
					nsuc++;
				}

				// If there are no successors then move backwards by the path and set the leave times until a node is found with not all successors nodes visited
				if (nsuc == 0) {
					bool allsucvis = true;
					do {
						curnode = path.pop();

						l[curnode] = step;

						allsucvis = true;
						for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
							allsucvis = allsucvis && v[pm.graph.target(oait)] > 0;
						}

						step++;
					} while (allsucvis || path.isEmpty());

					if (!allsucvis) {
						path.push(curnode);
					}

				} else {
					step++;
				}
			} else {
				bool allsucvis = true;
				do {
					curnode = path.pop();

					l[curnode] = step;

					allsucvis = true;
					for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
						allsucvis = allsucvis && v[pm.graph.target(oait)] > 0;
					}

					step++;
				} while (allsucvis || path.isEmpty());

				if (!allsucvis) {
					path.push(curnode);
				}
			}
		} else { // This node has already been visited, as well as his successors
			l[curnode] = step;
		}
	}

	// Build the predecessor sets based on the topological ordering
	node2predST.clear();
	QMap<ListDigraph::Node, QSet<int> > node2predIDs;
	node2predIDs.clear();
	QList<ListDigraph::Node> tmptopord;
	for (int i = 0; i < topolOrdering.size(); i++) {
		curnode = topolOrdering[i];
		node2predIDs[curnode].clear();

		// Copy only relevant nodes. Identify potential predecessors (direct and indirect)
		tmptopord.clear();
		for (int j = 0; j < i; j++) {
			if (pm.ops[topolOrdering[j]]->toolID == pm.ops[curnode]->toolID) {
				if (!(v[curnode] > l[topolOrdering[j]] || v[topolOrdering[j]] > l[curnode])) {
					tmptopord.append(topolOrdering[j]);
				}
			}
		}

		/*
		// Take one node from the back and eliminate its predecessors
		QList<ListDigraph::Node> tmptopord1;

		int szbefore;
		do {
			szbefore = tmptopord.size();
			tmptopord1.clear();
			for (int j = 0; j < tmptopord.size() - 1; j++) {
				if (v[curnode] > l[tmptopord[j]] || v[tmptopord[j]] > l[curnode]) { // Not predecessor of the last element
					tmptopord1.append(tmptopord[j]);
				}
				tmptopord1.append(tmptopord.last());
			}
			tmptopord = tmptopord1;
		} while (tmptopord.size() < szbefore);
		 */

		// Now all the predecessors are selected (direct)
		node2predST[curnode].clear();
		node2predST[curnode].append(tmptopord);
	}

}

int SBHSchedulerVNS::bottleneckTG(const int&) {
	/** Algorithm:
	 * 
	 * Precondition: Tool groups corresponding to the search tree node must be 
	 *				 scheduled in the given sequence. The state of the graph
	 *				 corresponding to the node must be restored.
	 * 
	 * 1. Iterate over all unscheduled tool groups
	 * 2. For every tool group formulate the corresponding subproblem:
	 * 2.1. update the corresponding ready times and due dates of the corresponding operations,
	 * 2.2. schedule the formulated supproblem under consideration of the DPC,
	 * 3. Select at most n bottlenecks. If the bottlenecks can not be distinguished
	 *	  by the means of the local objective function then the one with the 
	 *    greatest remaining processing time has a higher priority. 
	 * 4. Shift the operations of the already scheduled tool groups in view of 
	 *    the bottleneck to ensure feasibility of the solution (rcrc).
	 * 5. Return the found bottlenecks.
	 * 
	 *  */

	QTextStream out(stdout);

	// Find topological ordering of the graph
	topolOrdering = pm.topolSort();

//	int nbottlenecks = n;
	int res;

	// TO-DO: Restore the state of the graph corresponding to the current node of the search tree


	//Debugger::info << "Scheduling the unscheduled tool groups ... " << ENDL;

	//out << "Solving subproblems for unscheduled tool groups ..." << endl;

	//out << "Graph before preserving : " << endl << pm[node] << endl;

	//    foreach(const int &tgid, M) {
	//out << "Preserving state of the operations for tg : " << tgid << endl;
	//        preserveOpSchedState(pm, TG2Nodes[tgid]);
	//out << "Preserved state for : " << tgid << endl;
	//out << "Preserved state of the operations." << endl;
	//    }

	//QTime tupdate;
	//int msupdate;

	//tupdate.start();
	pm.updateHeads(topolOrdering); // TODO : Optimize using the topological ordering
	pm.updateStartTimes(topolOrdering); // TODO : Optimize using the topological ordering
	//msupdate = tupdate.elapsed();
	//Debugger::info << "SBHSchedulerVNS::bottleneckTG : Updating time (ms) "<<msupdate<<ENDL;
	//getchar();

//	foreach(const int &tgid, M) {

		// Iterate over the operations assigned to this tool group
		//opnodes = toolid2nodes[tgid];
		//for (int i = 0; i < opnodes.size(); i++) {
		// Update the head of the operation
		//Debugger::info << "Updating operation heads and tails for " << toolid2nodes[tgid].size() << " nodes" << ENDL;
		//Debugger::info << "The number of arcs in graph: " << countArcs(pm.graph) << ENDL;
		//Debugger::info << "Updating operation heads ..." << ENDL;
		//updateOperationHeads(toolid2nodes[tgid]);

		//out << "Recalculating heads and start times..." << endl;
		//	pm.updateHeads(); // TODO : Optimize using the topological ordering
		//	pm.updateStartTimes(); // TODO : Optimize using the topological ordering
		//out << "Recalculated heads and start times..." << endl;
		//Debugger::info << "Done updating operation heads." << ENDL;

		// Update the tails of the operation
		//Debugger::info << "Updating operation tails ..." << ENDL;
		//out << "Updating operation tails..." << endl;
		//		updateOperationTails(pm, TG2Nodes[tgid]); // TODO : Optimize using the topological ordering
		//out << "Updated operation tails." << endl;
		//Debugger::info << "Done updating operation tails." << ENDL;
		//}

//	}

	/** Find predecessors. */
	findPredecessorsSameTG();

//	foreach(const int &tgid, M) {
		// Schedule the operations on the current tool group
		//Debugger::info << "Scheduling the unscheduled tool group " << tgid << " ... " << ENDL;
		//out << pm << endl;
		//		scheduleTG(tgid);
		//Debugger::info << "Done scheduling the tool group." << ENDL;
//	}

	//out << "Solved subproblems for unscheduled tool groups." << endl << endl;

	//Debugger::info << "Done scheduling the unscheduled tool groups." << ENDL;
	//getchar();

	// Select the bottleneck tool groups
	QMultiMap<double, int> locobj2tgid;

	foreach(const int &tgid, M) {

		//out << "Local objective for tool group with ID = " << tgid << " is :  " << TG2Selection[tgid].localobj << endl;

		locobj2tgid.insert(TG2Selection[tgid].localobj, tgid);
	}
	out << endl;


	QList<int> bottleneckids;

	QMultiMap<double, int>::iterator rbegin = locobj2tgid.end();
	rbegin--;
	QMultiMap<double, int>::iterator rend = locobj2tgid.begin();
	rend--;


	for (QMultiMap<double, int>::iterator iter = rbegin; iter != rend; iter--) {
		//if (nbottlenecks == bottleneckids.size()) break;

		bottleneckids.append(iter.value());
	}


	// Clear the selections for the other tool groups which are in M
	//res = bottleneckids[Rand::rndInt(0, bottleneckids.size() - 1)];
	res = bottleneckids[Rand::rnd<Math::uint32>(0, bottleneckids.size() - 1)];

	// Select the bottleneck with the highest criticality measure
	double curCrit = -1.0;
	for (int i = 0; i < bottleneckids.size(); i++) {
		int curTGID = bottleneckids[i];
		if (tgID2Criticality[curTGID] > curCrit) {
			curCrit = tgID2Criticality[curTGID];
			res = curTGID;
		}
	}

	// Schedule the machine selected as the most critical
	scheduleTG(res);

	//res = bottleneckids.first();

	//out << "Found bottleneck tg : " << res << endl;
	//getchar();

	foreach(const int &tgid, M) {
		if (tgid != res) {
			TG2Selection.remove(tgid);
		}
	}

	// Unset the machine assignment for the operations from the other TGs not in M
	// Assign the expected processing times for the outgoing arcs

	//    foreach(const int &tgid, M) {
	//        if (tgid != res) {
	//            //out << "Restoring tg: " << tgid << endl;
	//            restoreOpSchedState(pm, TG2Nodes[tgid]);
	//        } else {

	//        }
	//    }


	//out << "Graph after restoring : " << endl << pm[node] << endl;

	//pm.updateHeads();
	//pm.updateStartTimes();

	return res;
}

void SBHSchedulerVNS::scheduleTG(const int tgid) {
	QTextStream out(stdout);

	out << "SBHSchedulerVNS::scheduleTG : Trying to schedule machine group : " << tgid << endl;

	if (((rc) (tgid)).types.size() == 1 && ((rc) (tgid)).types.contains(0)) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	//out << "Getting terminals ..." << endl;
	QList<ListDigraph::Node> terminals = pm.terminals();
	//out << "Got terminals." << endl;

	//TGScheduler *tgscheduler;

	/*
	tgscheduler = new TGVNSScheduler1;
	((TGVNSScheduler1*) tgscheduler)->maxIter(0000);
	((TGVNSScheduler1*) tgscheduler)->maxIterDecl(3000);
	((TGVNSScheduler*) tgscheduler)->maxTimeMs(30000);
	((TGVNSScheduler1*) tgscheduler)->sbhscheduler = this;
	 */


	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency before scheduling the tool group..." << endl;
	for (ListDigraph::NodeIt nit(pm[node]->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm[node]->graph, nit); oait != INVALID; ++oait) {
			if (pm[node]->ops[nit]->p() != -pm[node]->p[oait]) {
				out << "op ID = " << pm[node]->ops[nit]->ID << endl;
				out << pm[node] << endl;
				Debugger::err << "SBHSchedulerVNS::streeScheduleTG : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	//###########################  DEBUG  ######################################
	//out << "Nodes to for scheduling on the TG : " << tgid << endl;
	//for (int i = 0; i < streeTG2Nodes[tgid].size(); i++) {
	//out << pm.ops[streeTG2Nodes[tgid][i]] << endl;
	//}
	//getchar();
	//##########################################################################


	//out << "Running the tool group scheduler..." << endl;

	tgscheduler->node2predST = &node2predST;

	tgscheduler->locD = locD; // Set the local due dates of the operations

	TG2Selection[tgid].tgID = tgid;

#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHSchedulerVNS::bottleneckTG : Selection arcs are invalid before scheduling the unscheduled machine groups!!!" << ENDL;
			}

		}
	}
#endif	

	tgscheduler->schedule(pm, (rc) (tgid), TG2Nodes[tgid], terminals, dloc, TG2Selection[tgid]);
	//out << "Done running the scheduler." << endl;

	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency after scheduling the tool group..." << endl;
	for (ListDigraph::NodeIt nit(pm[node]->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm[node]->graph, nit); oait != INVALID; ++oait) {
			if (pm[node]->ops[nit]->p() != -pm[node]->p[oait]) {
				out << "op ID = " << pm[node]->ops[nit]->ID << endl;
				out << pm[node] << endl;
				Debugger::err << "SBHSchedulerVNS::streeScheduleTG : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */

	/*
	out << "SBHSchedulerVNS::streeScheduleTG : Selection:" << endl;
	for (int i = 0; i < streeTG2Selection[node][tgid].selection.size(); i++) {
		out << pm.ops[streeTG2Selection[node][tgid].selection[i].first] << endl;
	}

	double pt;
	ListDigraph::Node s;
	out << "SBHSchedulerVNS::streeScheduleTG : Checking processing times..." << endl;
	for (int i = 0; i < streeTG2Selection[node][tgid].selection.size(); i++) {
		s = streeTG2Selection[node][tgid].selection[i].first;
		if (pm[node]->ops[s]->machID >= 0) {
			pt = ((rc)(tgid, pm[node]->ops[s]->machID)).procTime(pm[node]->ops[s]);

			if (pm[node]->ops[s]->p() != pt) {
				//out << pm << endl;
				out << "pt = " << pt << endl;
				out << "p = " << pm[node]->ops[s]->p() << endl;
				out << pm[node]->ops[s] << endl;
				Debugger::err << "Something is wrong with the processing time for " << pm[node]->ops[s]->ID << ENDL;
			}
		}
	}
	out << "SBHSchedulerVNS::streeScheduleTG : Done checking processing times." << endl;
	 */
	//##########################################################################

	//delete tgscheduler;

#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHSchedulerVNS::bottleneckTG : Selection arcs are invalid after scheduling the unscheduled machine groups!!!" << ENDL;
			}

		}
	}
#endif

	out << "SBHSchedulerVNS::scheduleTG : Scheduled TG with ID = " << tgid << " with local objective : " << TG2Selection[tgid].localobj << endl << endl << endl;
	//getchar();
}

void SBHSchedulerVNS::reoptScheduleTG(const int tgid) {
	if (((rc) (tgid)).types.size() == 1 && ((rc) (tgid)).types.contains(0)) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	QList<ListDigraph::Node> terminals = pm.terminals();

	TGScheduler *tgscheduler;
	//if (streeInitSolMode) {
	tgscheduler = new TGVNSScheduler; //*/ TGFIFOScheduler;
	((TGVNSScheduler*) tgscheduler)->maxIterDecl(0);
	((TGVNSScheduler*) tgscheduler)->sbhscheduler = (SBHScheduler*) this;
	//} else {
	//tgscheduler = new TGATCScheduler;
	//}

	tgscheduler->node2predST = &node2predST;

	tgscheduler->schedule(pm, (rc) (tgid), TG2Nodes[tgid], terminals, dloc, TG2Selection[tgid]);

	delete tgscheduler;
}

void SBHSchedulerVNS::insertTGSelection(const int tgid) {
	QTextStream out(stdout);


	insertSelection(tgid);


	return;
}

void SBHSchedulerVNS::removeTGSelection(const int tgid) {

	removeSelection(tgid);

	return;
}

double SBHSchedulerVNS::calculateLocalObj(const int tgid) {
	// Calculate the local objective for the current selection
	//SBHTWTLocalObj lobj;
	UTWT utwt;

	QList<ListDigraph::Node> terminals = pm.terminals();

	double res = utwt(pm, TG2Nodes[tgid], locD); //lobj(pm, TG2Nodes[tgid], TG2Selection[tgid], terminals, dloc);

	return res;
}

double SBHSchedulerVNS::calculateObj() {
	TWT twt;

	return twt(pm, terminals);
}

void SBHSchedulerVNS::updateOperationHeads(const QList<ListDigraph::Node>& opnodes) {
	/**Algorithm: (IMPORTANT: arc lengths are negative)
	 * 
	 * 1. Run the Bellman-Ford algorithm on the graph with the negative 
	 *    arc weights and find the shortest path from the source to the given
	 *	  operation.
	 * 
	 * 2. Set the negated found length of the path as the ready time of the operation. 
	 * 
	 */

	//Debugger::info << "Updating operation heads ..." << ENDL;

	//QTextStream out(stdout);
	//out << pm << endl;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm.graph, pm.p);

	bf.init();
	bf.addSource(pm.head);
	//Debugger::info << "Running the BF algorithm..."<<ENDL;
	bf.start();
	//Debugger::info << "Done running the BF algorithm."<<ENDL;


	// #### IMPORTANT  #### Loca ready times of the operations must be updated, but the initial ready times must be considered
	// Update the ready time of the operation
	for (int i = 0; i < opnodes.size(); i++) {
#ifdef DEBUG
		if (!bf.reached(opnodes[i])) {
			Debugger::err << "SBHSchedulerVNS::updateOperationHead : Operation ID= " << pm.ops[opnodes[i]]->OID << ":" << pm.ops[opnodes[i]]->ID << " can not be reached from the root node " << pm.ops[pm.head]->OID << ":" << pm.ops[pm.head]->ID << "!" << ENDL;
		}
#endif
		pm.ops[opnodes[i]]->r(-bf.dist(opnodes[i]));
		//out << "The found length: " << pm.ops[opnodes[i]]->r() << endl;
	}
	//getchar();

	//Debugger::info << "Done updating operation heads." << ENDL;
	//out << pm << endl;
	//getchar();
}

void SBHSchedulerVNS::updateOperationTails(ProcessModel& pm, QList<ListDigraph::Node>& opnodes) {
	// Set the due dates of the operations proportionally to their processing times


	return;


	//Debugger::info << "Updating operation tails ..." << ENDL;

	double cmax;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm.graph, pm.p);

	QList<ListDigraph::Node> terminals = pm.terminals();

	for (int j = 0; j < opnodes.size(); j++) {

		bf.run(opnodes[j]);

		dloc[pm.ops[opnodes[j]]->ID].clear();
		for (int i = 0; i < terminals.size(); i++) {
			// Update the i-th local due date equal to the longest path from the operation to the corresponding terminal
			if (!bf.reached(terminals[i])) {
				//Debugger::err << "SBHSchedulerVNS::updateOperationTail : Terminal " << pm.ops[terminals[i]]->OID << ":" << pm.ops[terminals[i]]->ID << " can not be reached from the operation ID= " << pm.ops[opnodes[j]]->OID << ":" << pm.ops[opnodes[j]]->ID << " !" << ENDL;
				dloc[pm.ops[opnodes[j]]->ID].append(double(Math::MAX_DOUBLE));
			} else {
				// The longest path from the current operation node to the current terminal
				dloc[pm.ops[opnodes[j]]->ID].append(-bf.dist(terminals[i]));
			}
		}
	}
	// Find the makespan
	bf.init();
	bf.addSource(pm.head);
	bf.start();

	double ept = 0.0;

	for (int j = 0; j < opnodes.size(); j++) {
		// Define the processing time depending on whether the operation is scheduled or not
		if (pm.ops[opnodes[j]]->machID > 0) { // Exact processing time
			ept = pm.ops[opnodes[j]]->p();
		} else { // Expected processing time
			ept = (rc) (pm.ops[opnodes[j]]->toolID).shortestProcTime(pm.ops[opnodes[j]]); //expectedProcTime(pm.ops[opnodes[j]]);
		}

		// Set the local due dates
		for (int i = 0; i < terminals.size(); i++) {
			cmax = -bf.dist(terminals[i]);
			if (dloc[pm.ops[opnodes[j]]->ID][i] < Math::MAX_DOUBLE) {
				dloc[pm.ops[opnodes[j]]->ID][i] = Math::max(cmax, terminalopid2d[pm.ops[terminals[i]]->ID]) - dloc[pm.ops[opnodes[j]]->ID][i] + ept;
#ifdef DEBUG
				if (dloc[pm.ops[opnodes[j]]->ID][i] < 0.0) {
					Debugger::eDebug("Local due date < 0!");
				}
#endif

			}
		}
	}
	//Debugger::info << "Done updating operation tails." << ENDL;
}

void SBHSchedulerVNS::reoptimize(const int last_bottleneck_id) {
	QTextStream out(stdout);

	// Simple reoptimization strategy
	if (options["SBH_REOPT_TYPE"] == "LS") { // Local search reoptimization

		reoptimizePM(last_bottleneck_id);

	} else if (options["SBH_REOPT_TYPE"] == "STD") { // Classical reoptimization

		reoptimizeSimple(last_bottleneck_id);

	} else if (options["SBH_REOPT_TYPE"] == "NONE") { // No reoptimization is performed

		// No reoptimization is performed

	} else {

		Debugger::err << "SBHSchedulerVNS::reoptimize : SBH_REOPT_TYPE not specified/correct !!!" << ENDL;

	}

	return;

	// IMPORTANT!!! Synchronize the list of arcs corresponding to the tool groups! The local search may change the selection!

	// Synchronize the arcs for the scheduled tool groups

	TG2Arcs.clear();
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (!pm.conjunctive[ait]) {
			//out << "SBHSchedulerVNS::reoptimize : Synchronizing arc : " << pm.ops[pm.graph.source(ait)]->ID << "->" << pm.ops[pm.graph.target(ait)]->ID << endl;
			TG2Arcs[pm.ops[pm.graph.source(ait)]->toolID].append(QPair<ListDigraph::Node, ListDigraph::Node > (pm.graph.source(ait), pm.graph.target(ait)));
		}
	}

	// TODO : Update the selections

	out << "---" << endl;
	//out << "Found objective of the partial schedule after reoptimization : " << streeCalculateObj(node) << endl << endl << endl;
	// Reoptimization strategy based on the neighborhoods of Mastrolilli/Gambardella and Mati/Perez/Lahloy
	if (M.size() == 0) {
		out << "Scheduled all tool groups." << endl;
		out << "Found objective of the full schedule after reoptimization : " << calculateObj() << endl;
		//getchar();
	}

}

void SBHSchedulerVNS::reoptimizeSimple(const int last_bottleneck_id) {

	QTextStream out(stdout);

	out << endl << endl;
	out << "Classical reoptimization ..." << endl;
	out << "Last bottleneck tool group : " << last_bottleneck_id << endl;

	bool solutionImproved = false; // Indicates whether at least one rescheduling improved the solution

	double objBeforeReopt = TWT()(pm);
	double bestObj = TWT()(pm);

	do { // Repeat until there are no further solutions

		solutionImproved = false;

		for (int i = 0; i < btnseq.size(); i++) {
			//int curTGID = btnseq[Rand::rndInt(0, btnseq.size() - 1)];
			int curTGID = btnseq[Rand::rnd<Math::uint32>(0, btnseq.size() - 1)];

			if (curTGID == last_bottleneck_id) continue;

			out << endl << endl << "SBHSchedulerVNS::reoptimizeSimple : Reoptimizing machine group : " << curTGID << endl;

			out << "SBHSchedulerVNS::reoptimizeSimple : TWT of the partial schedule before the considering the current TG : " << TWT()(pm) << endl;

#ifdef DEBUG			
			// Check correctness of the arcs
			QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
			for (int j = 0; j < curSelectionArcs.size(); j++) {
				ListDigraph::Arc curArc = curSelectionArcs[j];

				if (!pm.graph.valid(curArc)) {
					Debugger::err << "SBHSchedulerVNS::reoptimizeSimple : Selection arcs are invalid!!!" << ENDL;
				}

			}
#endif			

			// Preserve the previous selection of the machine group
			TGSelection prevSelection = TG2Selection[curTGID];

			// Remove the actual selection of the machine group
			removeTGSelection(curTGID);

			out << "SBHSchedulerVNS::reoptimizeSimple : TWT of the partial schedule after removing the current selection : " << TWT()(pm) << endl;

			// Reoptimize the current machine group, i.e., schedule it again
			scheduleTG(curTGID);

			// Get the actual selection
			TGSelection& curSelection = (TGSelection&) TG2Selection[curTGID];

			// Check whether the current selection is better than the previous one
			if (curSelection.localobj < bestObj) {

				out << "SBHSchedulerVNS::reoptimizeSimple : Accepted a better solution with objective : " << curSelection.localobj << endl;

				solutionImproved = true;

				if (curSelection.localobj > objBeforeReopt) {

					out << "TWT before reoptimization : " << objBeforeReopt << endl;
					out << "Accepted solution : " << curSelection.localobj << endl;

					Debugger::err << "SBHSchedulerVNS::reoptimizeSimple : Accepted a worse solution!!!" << ENDL;
				}

				bestObj = curSelection.localobj;

			} else { // Restore the previous selection

				out << "SBHSchedulerVNS::reoptimizeSimple : Declined a solution with objective : " << curSelection.localobj << endl;

				// Restore the previous selection
				TG2Selection[curTGID] = prevSelection;

			}

			// Insert the selection after the reoptimization into the graph
			insertTGSelection(curTGID);

		}

	} while (solutionImproved);

	double objAfterReopt = TWT()(pm);

	if (objAfterReopt > objBeforeReopt) {

		out << "TWT before reoptimization : " << objBeforeReopt << endl;
		out << "TWT after reoptimization : " << objAfterReopt << endl;

		Debugger::err << "SBHSchedulerVNS::reoptimizeSimple : Reoptimization found a worse solution!!!" << ENDL;
	}

	out << "SBHSchedulerVNS::reoptimizeSimple : TWT of the partial schedule after the reoptimization : " << TWT()(pm) << endl;

	//getchar();

	return;








	TGSelection prev_selection;
	QList<double> prev_dloc;

	double cur_part_obj;
	double best_part_obj = Math::MAX_DOUBLE;

	int no_improvement_steps = 0;
	int max_no_improvement_steps = 3;

	M0.remove(last_bottleneck_id);

	pm.updateHeads();
	pm.updateStartTimes();

	// Update the selections for the scheduled tool groups

	foreach(const int &tgid, M0) {
		// Update operations data in the selections
		for (QMap<ListDigraph::Node, Operation>::iterator iter = TG2Selection[tgid].opNode2SchedOps.begin(); iter != TG2Selection[tgid].opNode2SchedOps.end(); iter++) {
			iter.value().copy(*(pm.ops[iter.key()]));
		}

		// Update the local objective
		TG2Selection[tgid].localobj = calculateLocalObj(tgid);
		out << "TG " << tgid << " : " << TG2Selection[tgid].localobj << endl;

	}

	best_part_obj = calculateObj();

	QList<int> btlncks = btnseq;
	btlncks.removeLast();
	int tgid;

	do {
		if (btlncks.size() == 0) break;

		Rand::randPermut(btlncks);

		// Iterate over the sorted scheduled tool groups
		for (int i = 0; i < btlncks.size(); i++) {

			// Set current tool group ID
			tgid = btlncks[i];

			// Preserve the selection for the current tool group
			prev_selection = TG2Selection[tgid];
			prev_dloc = dloc[tgid];

			// Preserve the scheduling state of the tool group
			preserveOpSchedState(pm, TG2Nodes[tgid]);

			// Remove the current selection of the tool group
			out << "SBHSchedulerVNS::reoptimizeSimple : Removing selection for TG " << tgid << endl;
			removeTGSelection(tgid);

			// Recalculate heads and start times
			pm.updateHeads();
			pm.updateStartTimes();

			// Recalculate local due dates for the tool group
			//out << "Updating the local due dates..." << endl;
			//out << "Before the update:" << endl;
			//for (int i = 0; i < TG2Nodes[tgid].size(); i++) {
			//out << pm.ops[TG2Nodes[tgid][i]]->ID << " : ";
			//for (int j = 0; j < dloc[pm.ops[TG2Nodes[tgid][i]]->ID].size(); j++) {
			//   out << dloc[pm.ops[TG2Nodes[tgid][i]]->ID][j] << ", ";
			//}
			//out << endl;
			//}
			updateOperationTails(pm, TG2Nodes[tgid]);
			//out << "After the update:" << endl;
			//for (int i = 0; i < TG2Nodes[tgid].size(); i++) {
			//out << pm.ops[TG2Nodes[tgid][i]]->ID << " : ";
			//for (int j = 0; j < dloc[pm.ops[TG2Nodes[tgid][i]]->ID].size(); j++) {
			//   out << dloc[pm.ops[TG2Nodes[tgid][i]]->ID][j] << ", ";
			//}
			//out << endl;
			//}

			//out << "Before rescheduling the tool group : " << tgid << endl;
			//for (int j = 0; j < TG2Nodes[tgid].size(); j++) {
			//out << pm.ops[TG2Nodes[tgid][j]] << endl;
			//out << *(TG2Selection[tgid].opNode2SchedOps[TG2Nodes[tgid][j]]) << endl;

			//}
			//getchar();

			scheduleTG(tgid);

			//out << "TG " << tgid << " : " << TG2Selection[tgid].localobj << endl;
			//out << "Local obj :  " << calculateLocalObj(tgid) << endl;
			//for (QMap<ListDigraph::Node, Operation*>::iterator iter = TG2Selection[tgid].opNode2SchedOps.begin(); iter != TG2Selection[tgid].opNode2SchedOps.end(); iter++) {
			//out << *iter.value() << endl;
			//}
			//getchar();
			//cur_part_obj = calculateObj();
			if (TG2Selection[tgid].localobj < prev_selection.localobj) {
				out << "Updated local obj for tg " << tgid << TG2Selection[tgid].localobj << endl;
			} else {

				TG2Selection[tgid] = prev_selection;
				dloc[tgid] = prev_dloc;

				restoreOpSchedState(pm, TG2Nodes[tgid]);

				//out << "R" << endl;
			}

			// Insert the new selection of the tool group
			//out << "...." << endl;
			insertTGSelection(tgid);
			//out << "....." << endl;

			// Recalculate heads
			pm.updateHeads();

			// Recalculate the completion times of the partial schedule
			pm.updateStartTimes();

			/*
			cur_part_obj = calculateObj();
			if (cur_part_obj > 1.0 * best_part_obj) {
				removeTGSelection(tgid);

				TG2Selection[tgid] = prev_selection;
				dloc[tgid] = prev_dloc;

				restoreOpSchedState(pm, TG2Nodes[tgid]);

				insertTGSelection(tgid);

				pm.updateHeads();
				pm.updateStartTimes();
			}
			 */

		}

		// Recalculate the current TWT for the partial schedule
		cur_part_obj = calculateObj();

		//out << "Currently found objective: " << cur_part_obj << endl;

		if (cur_part_obj < best_part_obj) {
			best_part_obj = cur_part_obj;

			no_improvement_steps = 0;
		} else {
			no_improvement_steps++;
		}

	} while (no_improvement_steps < max_no_improvement_steps);


	M0.insert(last_bottleneck_id);

	pm.updateHeads();
	pm.updateStartTimes();

	out << "Finished classical reoptimization." << endl;
}

void SBHSchedulerVNS::reoptimizeTG(const int&) {

}

void SBHSchedulerVNS::reoptimizePM(const int) {
	QTextStream out(stdout);

	QMap < ListDigraph::Node, bool> node2Movable;
	QMap<int, double> btnkID2Prob;

	for (int i = 0; i < btnseq.size(); i++) {
		int curBtnk = btnseq[i];
		btnkID2Prob[curBtnk] = double(btnseq.size() - i) / double(btnseq.size());
	}

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curNode]);

		if (curOp.toolID > 0 && btnseq.contains(curOp.toolID)) {
			//double rndNum = Rand::rndDouble();
			double rndNum = Rand::rnd<double>();

			if (rndNum <= btnkID2Prob[curOp.toolID]) { // Select with some probability as movable
				node2Movable[curNode] = true;
			} else {
				node2Movable[curNode] = false;
			}

		}

	}

	//out << "Running SBHSchedulerVNS::reoptimizePM... " << endl;
	int lsMaxIter = 0;
	int lsMaxTimeMs = 0;

	if (options["SBH_REOPT_LAST_ITER"] == "false") {

		lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt();
		lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt();

	} else if (options["SBH_REOPT_LAST_ITER"] == "true") { // Perform reoptimization only in the last iteration

		if (M.size() <= 0) {

			//ls.maxIter(100000); // Previously : 200000
			//ls.maxTimeMs(30*1000);

			lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt();
			lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt();

		} else {

			lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt() / (M0.size() + M.size());
			lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt() / (M0.size() + M.size());

		}

	} else {

		Debugger::err << "SBHSchedulerVNS::reoptimizePM : Not clear in which iteration to perform reoptimization!!!" << ENDL;

	}

	ls.maxIter(lsMaxIter);
	ls.maxTimeMs(lsMaxTimeMs);

	//################  DEBUG  #################
	ls.setScheduledTGs(M0.toList());
	//##########################################

	ls.setPM(&pm);
	//ls.setMovableNodes(node2Movable);
	rc.init();
	ls.setResources(&rc);
	ls.checkCorrectness(false);
	//out << "Running the local search reoptimization ..." << endl;
	ls.run();

	// IMPORTANT!!! The selections in case of LS are not updated automatically!!! This may cause some validity problems if they are manipulated later

	return;
}

bool SBHSchedulerVNS::stopCondition() {
	bool stop = true;

	stop = M.size() == 0;

	return stop; //M.size() == 0;
}

void SBHSchedulerVNS::stopActions() {
	//Debugger::wDebug("SBHSchedulerVNS::stopActions not implemented!");
	QTextStream out(stdout);

	out << "SBHSchedulerVNS::stopActions : Final results:" << endl;

	out << "SBHSchedulerVNS::stopActions : The sequences of bottlenecks: " << endl;

	for (int i = 0; i < btnseq.size(); i++) {
		out << btnseq[i] << " ";
	}
	out << " | TWT = " << calculateObj();
	out << endl;
	out << endl;
	//getchar();

	//out << "Final schedule:" << endl << pm << endl;

	out << endl;
}

void SBHSchedulerVNS::postprocessingActions() {
	/** Algorithm:
	 * 
	 * 1. Calculate the total objective and generate the corresponding schedule.
	 * 
	 * 2. Remove the selections of all of the tool groups 
	 *    from the process model.
	 * 
	 * 3. Bring the process model to the initial state (state before 
	 *    the scheduling actions)
	 */

	QTextStream out(stdout);

	//out << "Performing postprocessing ..." << endl;

	sched->fromPM(pm, *obj);

	//Debugger::info << "Done collecting schedule data." << ENDL;
	Debugger::info << "Found objective value : " << sched->objective << ENDL;
	//return;
	//getchar();

	//out << "Postprocessing finished." << endl;
	//getchar();

	// Restore the initial state of the process model
	/*
	QList<ListDigraph::Arc> arcs;
	for (QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node > > >::iterator iter = TG2Arcs.begin(); iter != TG2Arcs.end(); iter++) {
		// Iterate over the arcs of each machine group
		for (int ca = 0; ca < iter.value().size(); ca++) {
			//out << "SBHSchedulerVNS::postprocessingActions : Considering to delete the arc : " << pm.ops[iter.value()[ca].first]->ID << "->" << pm.ops[iter.value()[ca].second]->ID << endl;

			// Get the arcs
			arcs = pm.arcs(iter.value()[ca].first, iter.value()[ca].second);

			// Delete the current arc from the graph
			for (int i = 0; i < arcs.size(); i++) {
				if (!pm.conjunctive[arcs[i]]) {
					//out << "SBHSchedulerVNS::postprocessingActions : Deleting arc : " << pm.ops[pm.graph.source(arcs[i])]->ID << "->" << pm.ops[pm.graph.target(arcs[i])]->ID << endl;
					pm.graph.erase(arcs[i]);
				}
			}
		}
	}
	TG2Arcs.clear();
	 */
	//out << "SBHSchedulerVNS::postprocessingActions : Done erasing SB arcs." << endl;

	// Restore the processing times and the arc lengths in the graph
	/*
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		// Set the ready times to the initial value
		pm.ops[nit]->r(pm.ops[nit]->ir());

		// Set the processing time to zero
		pm.ops[nit]->p(0.0);

		// Initialize the start times
		pm.ops[nit]->s(0.0);

		// Mark the operation as unscheduled
		pm.ops[nit]->machID = -1;

		// Set the length of the outgoing arcs
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[nit]->p();
		}
	}
	 */

	// Clear the scheduler
	clear();

	//out << "SBHSchedulerVNS::postprocessingActions : Restored the initial state of the PM." << endl;

	//out << pm << endl;
	//getchar();

}

void SBHSchedulerVNS::preserveOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedr[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->r();
	}
}

void SBHSchedulerVNS::restoreOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedr.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore r for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->r(_opid2preservedr.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHSchedulerVNS::preserveOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedd[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->d();
	}
}

void SBHSchedulerVNS::restoreOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedd.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore d for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->d(_opid2preservedd.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHSchedulerVNS::preserveOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedp[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->p();

		//Debugger::info << "Preserving (" << pm.ops[opnodes[i]]->ID << "," << pm.ops[opnodes[i]]->p() << ENDL;
	}
}

void SBHSchedulerVNS::restoreOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedp.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore p for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->p(_opid2preservedp.value(pm.ops[opnodes[i]]->ID));

			//Debugger::info << "Restoring (" << pm.ops[opnodes[i]]->ID << "," << pm.ops[opnodes[i]]->p() << ENDL;
		}
	}
}

void SBHSchedulerVNS::preserveOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preserveds[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->s();
	}
}

void SBHSchedulerVNS::restoreOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preserveds.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore s for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->s(_opid2preserveds.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHSchedulerVNS::preserveOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedm[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->machID;
	}
}

void SBHSchedulerVNS::restoreOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedm.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore m for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->machID = _opid2preservedm.value(pm.ops[opnodes[i]]->ID);
		}
	}
}

void SBHSchedulerVNS::preserveOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	preserveOpReadyTimes(pm, opnodes);
	preserveOpDueDates(pm, opnodes);
	preserveOpProcTimes(pm, opnodes);
	preserveOpStartTimes(pm, opnodes);
	preserveOpMachAssignment(pm, opnodes);
}

void SBHSchedulerVNS::restoreOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	restoreOpReadyTimes(pm, opnodes);
	restoreOpDueDates(pm, opnodes);
	restoreOpProcTimes(pm, opnodes);
	restoreOpStartTimes(pm, opnodes);
	restoreOpMachAssignment(pm, opnodes);
}

void SBHSchedulerVNS::restoreInitialR(QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		pm.ops[opnodes[i]]->r(pm.ops[opnodes[i]]->ir());
	}
}

void SBHSchedulerVNS::removeSelection(const int& tgID) {
	// Remove all arcs which have previously been inserted and CHANGE THE OPERATIONS
	QTextStream out(stdout);

	// Operations of the tool group
	QMap<ListDigraph::Node, Operation>& prevNodeOper = (QMap<ListDigraph::Node, Operation>&) (tgID2PrevNodeOper[tgID]);
	QList<ListDigraph::Arc>& selectionArcs = (QList<ListDigraph::Arc>&) (tgID2SelectionArcs[tgID]);

	//out << "SBHSchedulerVNS::removeSelection : TG : " << tgID << endl;
	//out << "SBHSchedulerVNS::removeSelection : Restoring nodes : " << prevNodeOper.size() << endl;
	//out << "SBHSchedulerVNS::removeSelection : Removing arcs : " << selectionArcs.size() << endl;

	if (prevNodeOper.size() == 0) { // In case there is nothing to remove
		selectionArcs.clear();
		return;
	}

	// Restore the operations
	for (QMap<ListDigraph::Node, Operation>::iterator iter = prevNodeOper.begin(); iter != prevNodeOper.end(); iter++) {
		ListDigraph::Node curNode = iter.key();
		Operation& curOper = (Operation&) iter.value();

		// Restore the operation
		*(pm.ops[curNode]) = curOper;

		// Set the correct outgoing arc lengths
		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
			pm.p[oait] = -curOper.p();
		}

	}

	// Remove the selection arcs
	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];

#ifdef DEBUG		
		if (!pm.graph.valid(curArc)) {
			Debugger::err << "SBHSchedulerVNS::removeSelection : Trying to remove an invalid arc!!!" << ENDL;
		};
#endif

		// Remove the arc
		pm.graph.erase(curArc);

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	// Clear the previous data
	selectionArcs.clear();
	prevNodeOper.clear();
	tgID2PrevNodeOper.remove(tgID);
	tgID2SelectionArcs.remove(tgID);

	/*
#ifdef DEBUG

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curStartNode]);

		if (curOp.toolID != tgID) continue; // Omit other machine groups

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm.graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm.graph.target(curArc);

			if (!pm.conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
				Debugger::info << "Current TG : " << tgID << ENDL;
				Debugger::err << "SBHSchedulerVNS::removeSelection : Too many disjunctive arcs between the nodes after removing the selection!!!" << ENDL;
			}

		}

	}
#endif
	 */

}

void SBHSchedulerVNS::insertSelection(const int& tgID) {
	QTextStream out(stdout);
	/*
	#ifdef DEBUG

		// Check whether there are multiple disjunctinve arcs between any two nodes
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
			ListDigraph::Node curStartNode = nit;
			Operation& curOp = (Operation&) *(pm.ops[curStartNode]);

			if (curOp.toolID != tgID) continue; // Omit other machine groups

			QMap<ListDigraph::Node, int> targetNode2Numarcs;

			for (ListDigraph::OutArcIt oait(pm.graph, curStartNode); oait != INVALID; ++oait) {
				ListDigraph::Arc curArc = oait;
				ListDigraph::Node curEndNode = pm.graph.target(curArc);

				if (!pm.conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

				if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
					Debugger::info << "Current TG : " << tgID << ENDL;
					Debugger::err << "SBHSchedulerVNS::insertSelection : Too many disjunctive arcs between the nodes before inserting the selection!!!" << ENDL;
				}

			}

		}
	#endif	
	 */

	// Remove the previous selection
	removeSelection(tgID);

	// Operations of the tool group
	QMap<ListDigraph::Node, Operation>& prevNodeOper = (QMap<ListDigraph::Node, Operation>&) (tgID2PrevNodeOper[tgID]);
	QList<ListDigraph::Arc>& selectionArcs = (QList<ListDigraph::Arc>&) (tgID2SelectionArcs[tgID]);
	TGSelection& selection = (TGSelection&) (TG2Selection[tgID]);

	//out << "SBHSchedulerVNS::insertSelection : TG : " << tgID << endl;
	//out << "SBHSchedulerVNS::insertSelection : Prev. nodes nodes : " << prevNodeOper.size() << endl;
	//out << "SBHSchedulerVNS::insertSelection : Prev. arcs arcs : " << selectionArcs.size() << endl;

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsToAdd = selection.selection;
	selectionArcs = pm.addArcs(arcsToAdd);

	QList<ListDigraph::Node> opsToChange = selection.opNode2SchedOps.keys();

	//cout << "Added arcs : " << arcsAdded.size() << endl;

	for (int i = 0; i < opsToChange.size(); i++) { // Insert the nodes to be changed. Important!!! Inserting arcs only is not enough since there can be only one operation on a machine

		ListDigraph::Node curNode = opsToChange[i];

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		// Preserve the previous operation data
		prevNodeOper[curNode] = *(pm.ops[curNode]);

		// Set the new operation data according to the selection
		*(pm.ops[curNode]) = curOper;

		// Iterate over the outgoing arcs of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
			pm.p[oait] = -curOper.p();
		}

	}

	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];
		ListDigraph::Node curNode = pm.graph.source(curArc);

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		pm.conjunctive[curArc] = false;

		pm.p[curArc] = -curOper.p();

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);
}

void SBHSchedulerVNS::debugCheckReachability(const int& mid, ProcessModel& pm) {
	QTextStream out(stdout);

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

	QList<ListDigraph::Node> trgmachnodes; // Nodes currently on the target machine
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;
	ListDigraph::NodeMap<bool> scheduled(pm.graph, false);
	ListDigraph::NodeMap<bool> available(pm.graph, false);

	ListDigraph::Node suc;
	ListDigraph::Node sucpred;

	q.enqueue(pm.head);
	scheduled[pm.head] = false;
	available[pm.head] = true;

	// Collect operation sequences on the target machine
	while (q.size() > 0) {
		curnode = q.dequeue();

		if (available[curnode] && !scheduled[curnode]) {
			if ((pm.ops[curnode]->ID > 0) && (pm.ops[curnode]->machID == mid)) {
				trgmachnodes.append(curnode);
			}

			scheduled[curnode] = true;

			// Enqueue the successors
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				suc = pm.graph.target(oait);
				if (!scheduled[suc]) {

					// Update availability

					available[suc] = true;
					for (ListDigraph::InArcIt iait(pm.graph, suc); iait != INVALID; ++iait) {
						sucpred = pm.graph.source(iait);
						if (!scheduled[sucpred]) {
							available[suc] = false;
							break;
						}
					}

					if (available[suc]) {
						q.enqueue(suc);
					}
				}
			}
		} else {
			if (!available[curnode]) {
				q.enqueue(curnode);
			}
		}

	}

	for (int j = 0; j < trgmachnodes.size() - 1; j++) {

		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (trgmachnodes.at(j), trgmachnodes.at(j + 1)));

	}

	if (trgmachnodes.size() > 0) {
		res.prepend(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, trgmachnodes.first()));
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (trgmachnodes.last(), INVALID));
	}

	// In case there are no operations on the target machine
	if (trgmachnodes.size() == 0) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, INVALID));
	}

	// ###################  DEBUG: can be deleted  #################################   

	/*
	out << "operations on machine " << mid << " : " << endl;
	for (int k = 0; k < trgmachnodes.size(); k++) {
		out << pm.ops[trgmachnodes[k]]->ID << ",";
	}

	out << endl << endl;
	 */
	//out << "GBM:" << endl;
	//out << pm << endl;

	for (int j = 0; j < res.size(); j++) {
		//	out << pm.ops[res[i].first]->ID << "->" << pm.ops[res[i].second]->ID << endl;
		if (!reachable(pm, res[j].first, res[j].second)) {
			out << "Not reachable : " << pm.ops[res[j].first]->ID << "->" << pm.ops[res[j].second]->ID << endl;

			out << "operations on machine " << mid << " : " << endl;
			for (int k = 0; k < trgmachnodes.size(); k++) {
				out << pm.ops[trgmachnodes[k]]->ID << ",";
			}

			out << endl << endl;

			out << pm << endl;
			getchar();
		}
	}

	// #############################################################################
}

bool SBHSchedulerVNS::reachable(ProcessModel& pm, const ListDigraph::Node& s, const ListDigraph::Node & t) {
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;

	q.enqueue(t);

	if (s == t) return true;

	if (s == INVALID || t == INVALID) return true;

	while (q.size() > 0) {
		curnode = q.dequeue();

		// Iterate over the predecessors
		for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
			if (pm.graph.source(iait) == s) {
				return true;
			} else {
				q.enqueue(pm.graph.source(iait));
			}
		}
	}

	return false;
}

/**  **********************************************************************  **/


/**  ***************  SBH as in Pinedo-Singer  ****************************  **/

SBHSchedulerPinSin::SBHSchedulerPinSin() : dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {

}

SBHSchedulerPinSin::SBHSchedulerPinSin(SBHSchedulerPinSin& orig) : LSScheduler(orig), IterativeAlg(), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	this->tgscheduler = (TGScheduler*) orig.tgscheduler->clone();

	this->tgscheduler->sbhscheduler = (SBHScheduler*) this;

	//TG2Selection = orig.TG2Selection;
}

SBHSchedulerPinSin::~SBHSchedulerPinSin() {

}

Clonable* SBHSchedulerPinSin::clone() {
	return new SBHSchedulerPinSin(*this);
}

void SBHSchedulerPinSin::scheduleActions() {

	//Debugger::info << "SBHSchedulerPinSin::scheduleActions : Initializing..." << ENDL;
	init();
	//Debugger::info << "SBHSchedulerPinSin::scheduleActions : Done initializing." << ENDL;

	//Debugger::info << "SBHSchedulerPinSin::scheduleActions : Running..." << ENDL;
	run();
	//Debugger::info << "SBHSchedulerPinSin::scheduleActions : Done." << ENDL;

}

bool SBHSchedulerPinSin::schedule(ProcessModel& pm, Resources& resources, Schedule& schedule) {
	QTextStream out(stdout);

	bool schedstatus = false;

	// Set local pointers for convenience
	this->pm = pm;
	this->rc = resources;
	this->sched = &schedule;

	//out << "SBHSchedulerPinSin::schedule : PM :" << endl;
	//out << pm << endl;
	//getchar();

	// Initialize the scheduler
	init();

	// Run the scheduling algorithm
	run();

	// Prepare the resulting schedule
	schedstatus = true;
	return schedstatus;
}

void SBHSchedulerPinSin::init() {
	QTextStream out(stdout);

	// Delete the arcs
	/*
	QList<ListDigraph::Arc> arcs;
	for (QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node > > >::iterator iter = TG2Arcs.begin(); iter != TG2Arcs.end(); iter++) {
		// Iterate over the arcs of each machine group
		for (int ca = 0; ca < iter.value().size(); ca++) {
			// Get the arc
			arcs = pm.arcs(iter.value()[ca].first, iter.value()[ca].second);

			// Delete the current arc from the graph
			for (int i = 0; i < arcs.size(); i++) {
				if (!pm.conjunctive[arcs[i]]) {
					pm.graph.erase(arcs[i]);
				}
			}
		}
	}
	 */

	clear();

	// Select the corresponding initial weights for every arc (average processing time of the source operation)
	// These weights will be updated as soon as the corresponding operations are scheduled on the defined machines
	// IMPORTANT: the lengths of the arcs are set to be negative, since the longest path problems are reduced to the shortest path problems and Bellman-Ford algorithm is applied (polynomial time complexity).
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (pm.ops[pm.graph.source(ait)]->ID <= 0) { // Fictive 
			pm.p[ait] = 0.0;
		} else {
			pm.p[ait] = -rc(pm.ops[pm.graph.source(ait)]->toolID)./*shortestProcTime(pm.ops[pm.graph.source(ait)]); //*/expectedProcTime(pm.ops[pm.graph.source(ait)]);
		}

		pm.ops[pm.graph.source(ait)]->p(-(pm.p[ait]));
	}

	//###########################  DEBUG  ######################################
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency during SBH initialization..." << endl;
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			if (pm.ops[nit]->p() != -pm.p[oait]) {
				out << "op ID = " << pm.ops[nit]->ID << endl;
				out << pm << endl;
				Debugger::err << "SBHSchedulerPinSin::init : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	out << "Done checking consistency during SBH initialization." << endl;
	//##########################################################################

	// Consider the ready times for the initial nodes
	/*
	 QQueue<ListDigraph::Node> q;
	 ListDigraph::Node cur_node;

	 q.enqueue(pm.head);
	 while (q.size() > 0) {
		 cur_node = q.dequeue();
		 if (pm.ops[cur_node]->ID <= 0) {
			 for (ListDigraph::OutArcIt oait(pm.graph, cur_node); oait != INVALID; ++oait) {
				 q.enqueue(pm.graph.target(oait));
			 }
		 } else {
			 for (ListDigraph::InArcIt iait(pm.graph, cur_node); iait != INVALID; ++iait) {
				 pm.p[iait] = -pm.ops[cur_node]->ir();
			 }
		 }
	 }
	 */

	// Initialize the list of the terminal nodes and get the terminal due dates and the weights
	for (ListDigraph::InArcIt iait(pm.graph, pm.tail); iait != INVALID; ++iait) {
		terminals.append(pm.graph.source(iait));
		terminalopid2d[pm.ops[terminals.last()]->ID] = pm.ops[terminals.last()]->d();
		terminalopid2w[pm.ops[terminals.last()]->ID] = pm.ops[terminals.last()]->w();
	}

	// Initialize the set of unscheduled machines
	for (int i = 0; i < rc.tools.size(); i++) {
		M.insert(rc.tools[i]->ID);
	}

	// Initialize the mapping of the tool group onto the corresponding set of nodes
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		if (pm.ops[nit]->ID >= 0) {
			TG2Nodes[pm.ops[nit]->toolID].append(nit);
		}
	}

	// Topological ordering of the process model
	topolOrdering = pm.topolSort();

}

void SBHSchedulerPinSin::clear() {
	// Clear the containers for preserved data
	_opid2preservedr.clear();
	_opid2preservedd.clear();
	_opid2preserveds.clear();
	_opid2preservedp.clear();
	_opid2preservedm.clear();
	_opid2preservedr.squeeze();
	_opid2preservedd.squeeze();
	_opid2preserveds.squeeze();
	_opid2preservedp.squeeze();
	_opid2preservedm.squeeze();

	// Initialize the list of the terminal nodes and get the terminal due dates and the weights
	terminals.clear();
	terminalopid2d.clear();
	terminalopid2w.clear();
	terminalopid2d.squeeze();
	terminalopid2w.squeeze();

	// Initialize the list of bottlenecks
	btnseq.clear();

	// Scheduled and unscheduled tool groups corresponding to the nodes of the search tree
	M0.clear();

	M.clear();

	// Initialize the mapping of the tool group onto the corresponding set of nodes
	TG2Nodes.clear();
	TG2Nodes.squeeze();

	// Delete the selection of the nodes, which the arcs are created of.
	TG2Selection.clear();
	TG2Selection.squeeze();

	TG2Arcs.clear();
	TG2Arcs.squeeze();

	dloc.clear();
	dloc.squeeze();
}

void SBHSchedulerPinSin::preprocessingActions() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// Set the expected processing times
	double ept = 0.0;
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ept = rc(pm.ops[nit]->toolID).expectedProcTime(pm.ops[nit]);

		// Set the processing time
		pm.ops[nit]->p(ept);

		// Update the arcs
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			pm.p[oait] = -ept;
		}
	}

	//out << pm << endl;
	//getchar();

	// Get the topological ordering of the graph
	topolOrdering = pm.topolSort();

	// Update the ready times of the operations
	pm.updateHeads(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	//double smallestOrderS = Math::MAX_DOUBLE;
	//double smallestOrderD = Math::MAX_DOUBLE;
	//double orderTimeInt = 0.0;
	double smallestD = Math::MAX_DOUBLE;
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	QList<ListDigraph::Node> terminals = pm.terminals();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		lenRemain[nit] = 0;
	}

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		//smallestOrderS = Math::MAX_DOUBLE;
		//smallestOrderD = Math::MAX_DOUBLE;

		lenRemain[terminals[i]] = 1;
		pRemain[terminals[i]] = 0.0;
		dOrd[terminals[i]] = pm.ops[terminals[i]]->d();

		for (ListDigraph::InArcIt iait(pm.graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm.graph.source(iait);
			//stack.push(pm.graph.source(iait));
			q.enqueue(curpred);

			lenRemain[curpred] = 2;
			pRemain[curpred] = pm.ops[curpred]->p() + 0.0; // 0.0 - for the terminal node
			dOrd[curpred] = pm.ops[terminals[i]]->d();

			//Debugger::info << "Terminal : " << pm.ops[terminals[i]]->ID << ENDL;
			//Debugger::info << pm.ops[curpred]->ID << " : " << d[curpred] << ENDL;
			//getchar();
		}

		ordNodes.clear();
		while (/*!stack.empty()*/!q.empty()) {
			//curnode = stack.pop();
			curnode = q.dequeue();

			// Save the node 
			ordNodes.append(curnode);

			// Find the smallest wished start time of all successors
			double ss = Math::MAX_DOUBLE;
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				cursucc = pm.graph.target(oait);
				double k = 1.0; // As proposed by Vepsalainen and Morton
				ss = Math::min(ss, pm.ops[cursucc]->d() - k * pm.ops[cursucc]->p());
			}

			// Set the found time as the due date for the current node
			smallestD = Math::min(smallestD, ss);
			pm.ops[curnode]->d(ss);

			// Find the largest number of the operations to be processed after the current node (including the current node)
			ListDigraph::Node longerSucc = INVALID;
			double maxP = -1.0;
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				if (pRemain[pm.graph.target(oait)] > maxP) {
					maxP = pRemain[pm.graph.target(oait)];
					longerSucc = pm.graph.target(oait);
				}
			}
			//Debugger::info << "Found : " << pm.ops[longerSucc]->ID << ENDL;
			//getchar();
			if (longerSucc == INVALID) {
				Debugger::err << "ATCANScheduler::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
			}
			pRemain[curnode] = pRemain[longerSucc] + pm.ops[curnode]->p();
			lenRemain[curnode] = lenRemain[longerSucc] + 1;
			dOrd[curnode] = dOrd[longerSucc]; // Due date of the order

			//Debugger::info << pm.ops[longerSucc]->ID << " : " << d[longerSucc] << ENDL;
			//getchar();

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				curpred = pm.graph.source(iait);

				// Push the current predecessor into the queue
				q.enqueue(curpred);
			}
		}

		//getchar();
	}

	//Debugger::info << "SmallestD : " << smallestD << ENDL;

	//out << pm << endl;
	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//    out << "pRemain(" << pm.ops[nit]->ID << ") = " << pRemain[nit] << endl;
	//}
	//getchar();

	// Set the due dates based on the heads
	for (int i = 0; i < topolOrdering.size(); i++) {
		if (!terminals.contains(topolOrdering[i])) {
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 0.0 * smallestD);
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001);
			pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001, pm.ops[topolOrdering[i]]->r() + pm.ops[topolOrdering[i]]->p()));
			//pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001, pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 1.0 * smallestD));
			//d[topolOrdering[i]] -= 1.0 * smallestD;
		}
	}

	// Initialize the start times of the operations
	pm.updateStartTimes(topolOrdering);


	// Estimate a criticality measure for each machine group
	for (int i = 0; i < rc.tools.size(); i++) {

		ToolGroup& curTG = *(rc.tools[i]);

		if (curTG.ID == 0) {
			tgID2Criticality[curTG.ID] = 0;
			continue;
		}

		QList<ListDigraph::Node>& curNodes = TG2Nodes[curTG.ID];
		double totalP = 0.0;
		double m = (double) curTG.machines().size();

		// Get the total expected processing time of all operations on the machine group
		for (int j = 0; j < curNodes.size(); j++) {
			ListDigraph::Node curNode = curNodes[j];
			Operation& curOp = *(pm.ops[curNode]);

			totalP += curOp.p();
		}

		// Estimate the criticality as in Pinedo FFs
		tgID2Criticality[curTG.ID] = totalP / m;

		out << "Criticality of " << curTG.ID << " is " << tgID2Criticality[curTG.ID] << endl;
	}

}

void SBHSchedulerPinSin::stepActions() {
	QTextStream out(stdout);

	// Estimate the longest paths between the nodes and the terminals
	QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> > longestPaths = pm.longestPathsLen(); // The longest paths in the graph

	locD.clear();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		for (int i = 0; i < terminals.size(); i++) {

			ListDigraph::Node curTerm = terminals[i];

			locD[nit][curTerm] = pm.ops[curTerm]->d() - longestPaths[nit][curTerm];

		}

	}


	// Solve the subproblems for the unassigned tool groups and define the bottlenecks. Use the PM assigned to the curnode.
	out << "Searching bottlenecks..." << endl;
	int bottleneck = bottleneckTG(1);
	out << "Found bottlenecks." << endl;

	// Set of scheduled/unscheduled tool groups
	M0.insert(bottleneck);
	M.remove(bottleneck);
	btnseq << bottleneck;

	out << "Current bottleneck sequence : ";
	for (int i = 0; i < btnseq.size(); i++) {
		out << btnseq[i] << ",";
	}
	out << endl;

	out << "Before inserting bottleneck: " << bottleneck << " TWT (partial schedule, full recalculation) : " << TWT()(pm) << endl;
	out << "Arcs in the bottleneck's selection: " << TG2Selection[bottleneck].selection.size() << endl;

	/*
#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHSchedulerPinSin::stepActions : Selection arcs are invalid before inserting the bottleneck!!!" << ENDL;
			}

		}
	}
#endif
	 */

	// Insert the selection for the selected bottleneck	
	insertTGSelection(bottleneck);

	/* Only relevant for classical reoptimization since the selections are updated there	
	#ifdef DEBUG
		for (int i = 0; i < btnseq.size(); i++) {
			int curTGID = btnseq[i];
			// Check correctness of the arcs
			QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
			for (int j = 0; j < curSelectionArcs.size(); j++) {
				ListDigraph::Arc curArc = curSelectionArcs[j];

				if (!pm.graph.valid(curArc)) {
					out << "Current TG : " << curTGID << endl;
					Debugger::err << "SBHSchedulerPinSin::stepActions : Selection arcs are invalid after inserting the bottleneck!!!" << ENDL;
				}

			}
		}
	#endif
	 */

	// Recalculate heads and start times for the considered process model
	QList<ListDigraph::Node> topSort = pm.topolSort();
	pm.updateHeads(topSort);
	pm.updateStartTimes(topSort);

	out << "After inserting bottleneck: " << bottleneck << " TWT (partial schedule) : " << calculateObj() << endl;


	out << "After inserting bottleneck: " << bottleneck << " TWT (partial schedule, full recalculation) : " << TWT()(pm) << endl;

	//getchar();

	// Perform reoptimization for the considered process model
	reoptimize(bottleneck);


	//getchar();
	return;
}

void SBHSchedulerPinSin::findPredecessorsSameTG() {
	QTextStream out(stdout);

	/*
	// Find operation predecessors for all nodes based on the topological ordering
	node2predST.clear();
	node2predST[topolOrdering[0]].clear();
	node2predST[topolOrdering[0]].append(QList<ListDigraph::Node>());
	node2predST[topolOrdering[0]].last().clear();
	ListDigraph::Node prevnode;
	ListDigraph::Node curnode;
	for (int i = 1; i < topolOrdering.size(); i++) {
		//Debugger::info << (int) pm.ops[topolOrdering[i]]->ID << ENDL;
		// Iterate over all the direct predecessors of the node
		curnode = topolOrdering[i];
		node2predST[curnode].clear();
		for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
			prevnode = pm.graph.source(iait);

			// Inherit all arrays
			//out << "Inheriting arrays of : " << pm.ops[prevnode]->ID << endl;
			for (int j = 0; j < node2predST[prevnode].size(); j++) {
				node2predST[curnode].append(node2predST[prevnode][j]);
				node2predST[curnode].last().append(prevnode);
			}

			// Appen the previous node to the arrays of this node
			if (node2predST[curnode].size() == 0) node2predST[curnode].append(QList<ListDigraph::Node>());

			for (int j = 0; j < node2predST[curnode].size(); j++) {
				node2predST[curnode][j].append(prevnode);
			}

			// TO-DO : Think on the possibility of cutting the previous node
		}

		//out << "Tool of the current node : " << (int) pm.ops[curnode]->toolID << endl;
		//out << "Predecessor tools : " << endl;
		//for (int j = 0; j < node2predST[curnode].size(); j++) {
		//    for (int k = 0; k < node2predST[curnode][j].size(); k++) {
		//	out << pm.ops[node2predST[curnode][j][k]]->toolID << ",";
		//    }
		//}
		//out << endl;
		//getchar();

	}

	QHash<int, ListDigraph::Node> nodeID2node;
	nodeID2node.clear();
	for (int i = 0; i < topolOrdering.size(); i++) {
		nodeID2node[pm.ops[topolOrdering[i]]->ID] = topolOrdering[i];
	}

	// Now cut everything in order to obtain only (semi-)direct predecessors
	for (int i = 1; i < topolOrdering.size(); i++) {
		curnode = topolOrdering[i];

		// For the current node sort all arrays by their length
		QMultiMap<int, QList<ListDigraph::Node>* > len2array;

		for (int j = 0; j < node2predST[curnode].size(); j++) {
			// Find index of the last element of the same size
			int k = -1;
			for (k = node2predST[curnode][j].size() - 1; k >= 0; k--) {
				if (pm.ops[node2predST[curnode][j][k]]->toolID == pm.ops[curnode]->toolID) break;
			}
			if (k >= 0) {
				len2array.insertMulti(k, &(node2predST[curnode][j]));
			}
		}

		QSet<int> dirpreds; // Set of direct predecessors (IDs)

		QMultiMap<int, QList<ListDigraph::Node>* >::iterator iter;
		QMultiMap<int, QList<ListDigraph::Node>* >::iterator rbegin = len2array.end();
		rbegin--;
		QMultiMap<int, QList<ListDigraph::Node>* >::iterator rend = len2array.begin();
		rend--;
		for (iter = rbegin; iter != rend; iter--) {
			// Get the last element of the type of curnode
			if (!dirpreds.contains(pm.ops[iter.value()->at(iter.key())]->ID)) {
				dirpreds.insert(pm.ops[iter.value()->at(iter.key())]->ID);
				break;
			}

			//iter.value()->clear();
		}

		//Debugger::info << (int) dirpreds.size() << ENDL;
		//getchar();

		node2predST[curnode].clear();
		node2predST[curnode].append(QList<ListDigraph::Node>());
		node2predST[curnode][0].clear();
		// Restore the nodes from the IDs
		for (QSet<int>::iterator iter = dirpreds.begin(); iter != dirpreds.end(); iter++) {
			node2predST[curnode][0].append(nodeID2node[*iter]);
		}
	}
	 */

	// Preform depth-first search for the graph of the pm
	ListDigraph::NodeMap<int> v(pm.graph);
	ListDigraph::NodeMap<int> l(pm.graph);

	QStack<ListDigraph::Node> s;
	QStack<ListDigraph::Node> path;
	ListDigraph::Node curnode;
	ListDigraph::Node sucnode;
	ListDigraph::Node prednode;

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		v[nit] = -1;
		l[nit] = -1;
	}

	s.push(pm.head);
	v[pm.head] = -1;
	l[pm.head] = -1;

	int step = 0;

	while (!s.isEmpty()) {
		curnode = s.pop();

		// Update the visiting time
		if (v[curnode] < 0) { // This node has not been visited yet. His successors have not been visited.
			v[curnode] = step;

			// Add this node to the path
			path.push(curnode);

			// Push the successors of the node
			int nsuc = 0;
			bool allpredvis = true;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				allpredvis = allpredvis && v[pm.graph.source(iait)] > 0;
			}

			if (allpredvis) {

				for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
					sucnode = pm.graph.target(oait);
					s.push(sucnode);
					nsuc++;
				}

				// If there are no successors then move backwards by the path and set the leave times until a node is found with not all successors nodes visited
				if (nsuc == 0) {
					bool allsucvis = true;
					do {
						curnode = path.pop();

						l[curnode] = step;

						allsucvis = true;
						for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
							allsucvis = allsucvis && v[pm.graph.target(oait)] > 0;
						}

						step++;
					} while (allsucvis || path.isEmpty());

					if (!allsucvis) {
						path.push(curnode);
					}

				} else {
					step++;
				}
			} else {
				bool allsucvis = true;
				do {
					curnode = path.pop();

					l[curnode] = step;

					allsucvis = true;
					for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
						allsucvis = allsucvis && v[pm.graph.target(oait)] > 0;
					}

					step++;
				} while (allsucvis || path.isEmpty());

				if (!allsucvis) {
					path.push(curnode);
				}
			}
		} else { // This node has already been visited, as well as his successors
			l[curnode] = step;
		}
	}

	// Build the predecessor sets based on the topological ordering
	node2predST.clear();
	QMap<ListDigraph::Node, QSet<int> > node2predIDs;
	node2predIDs.clear();
	QList<ListDigraph::Node> tmptopord;
	for (int i = 0; i < topolOrdering.size(); i++) {
		curnode = topolOrdering[i];
		node2predIDs[curnode].clear();

		// Copy only relevant nodes. Identify potential predecessors (direct and indirect)
		tmptopord.clear();
		for (int j = 0; j < i; j++) {
			if (pm.ops[topolOrdering[j]]->toolID == pm.ops[curnode]->toolID) {
				if (!(v[curnode] > l[topolOrdering[j]] || v[topolOrdering[j]] > l[curnode])) {
					tmptopord.append(topolOrdering[j]);
				}
			}
		}

		/*
		// Take one node from the back and eliminate its predecessors
		QList<ListDigraph::Node> tmptopord1;

		int szbefore;
		do {
			szbefore = tmptopord.size();
			tmptopord1.clear();
			for (int j = 0; j < tmptopord.size() - 1; j++) {
				if (v[curnode] > l[tmptopord[j]] || v[tmptopord[j]] > l[curnode]) { // Not predecessor of the last element
					tmptopord1.append(tmptopord[j]);
				}
				tmptopord1.append(tmptopord.last());
			}
			tmptopord = tmptopord1;
		} while (tmptopord.size() < szbefore);
		 */

		// Now all the predecessors are selected (direct)
		node2predST[curnode].clear();
		node2predST[curnode].append(tmptopord);
	}

}

int SBHSchedulerPinSin::bottleneckTG(const int&) {
	/** Algorithm:
	 * 
	 * Precondition: Tool groups corresponding to the search tree node must be 
	 *				 scheduled in the given sequence. The state of the graph
	 *				 corresponding to the node must be restored.
	 * 
	 * 1. Iterate over all unscheduled tool groups
	 * 2. For every tool group formulate the corresponding subproblem:
	 * 2.1. update the corresponding ready times and due dates of the corresponding operations,
	 * 2.2. schedule the formulated supproblem under consideration of the DPC,
	 * 3. Select at most n bottlenecks. If the bottlenecks can not be distinguished
	 *	  by the means of the local objective function then the one with the 
	 *    greatest remaining processing time has a higher priority. 
	 * 4. Shift the operations of the already scheduled tool groups in view of 
	 *    the bottleneck to ensure feasibility of the solution (rcrc).
	 * 5. Return the found bottlenecks.
	 * 
	 *  */

	QTextStream out(stdout);

	// Find topological ordering of the graph
	topolOrdering = pm.topolSort();

//	int nbottlenecks = n;
	int res;

	// TO-DO: Restore the state of the graph corresponding to the current node of the search tree


	//Debugger::info << "Scheduling the unscheduled tool groups ... " << ENDL;

	//out << "Solving subproblems for unscheduled tool groups ..." << endl;

	//out << "Graph before preserving : " << endl << pm[node] << endl;

	//    foreach(const int &tgid, M) {
	//out << "Preserving state of the operations for tg : " << tgid << endl;
	//        preserveOpSchedState(pm, TG2Nodes[tgid]);
	//out << "Preserved state for : " << tgid << endl;
	//out << "Preserved state of the operations." << endl;
	//    }

	//QTime tupdate;
	//int msupdate;

	//tupdate.start();
	pm.updateHeads(topolOrdering); // TODO : Optimize using the topological ordering
	pm.updateStartTimes(topolOrdering); // TODO : Optimize using the topological ordering
	//msupdate = tupdate.elapsed();
	//Debugger::info << "SBHSchedulerPinSin::bottleneckTG : Updating time (ms) "<<msupdate<<ENDL;
	//getchar();

//	foreach(const int &tgid, M) {

		// Iterate over the operations assigned to this tool group
		//opnodes = toolid2nodes[tgid];
		//for (int i = 0; i < opnodes.size(); i++) {
		// Update the head of the operation
		//Debugger::info << "Updating operation heads and tails for " << toolid2nodes[tgid].size() << " nodes" << ENDL;
		//Debugger::info << "The number of arcs in graph: " << countArcs(pm.graph) << ENDL;
		//Debugger::info << "Updating operation heads ..." << ENDL;
		//updateOperationHeads(toolid2nodes[tgid]);

		//out << "Recalculating heads and start times..." << endl;
		//	pm.updateHeads(); // TODO : Optimize using the topological ordering
		//	pm.updateStartTimes(); // TODO : Optimize using the topological ordering
		//out << "Recalculated heads and start times..." << endl;
		//Debugger::info << "Done updating operation heads." << ENDL;

		// Update the tails of the operation
		//Debugger::info << "Updating operation tails ..." << ENDL;
		//out << "Updating operation tails..." << endl;
		//		updateOperationTails(pm, TG2Nodes[tgid]); // TODO : Optimize using the topological ordering
		//out << "Updated operation tails." << endl;
		//Debugger::info << "Done updating operation tails." << ENDL;
		//}

//	}

	/** Find predecessors. */
	findPredecessorsSameTG();

//	foreach(const int &tgid, M) {
		// Schedule the operations on the current tool group
		//Debugger::info << "Scheduling the unscheduled tool group " << tgid << " ... " << ENDL;
		//out << pm << endl;
		//		scheduleTG(tgid);
		//Debugger::info << "Done scheduling the tool group." << ENDL;
//	}

	//out << "Solved subproblems for unscheduled tool groups." << endl << endl;

	//Debugger::info << "Done scheduling the unscheduled tool groups." << ENDL;
	//getchar();

	// Select the bottleneck tool groups
	QMultiMap<double, int> locobj2tgid;

	foreach(const int &tgid, M) {

		//out << "Local objective for tool group with ID = " << tgid << " is :  " << TG2Selection[tgid].localobj << endl;

		locobj2tgid.insert(TG2Selection[tgid].localobj, tgid);
	}
	out << endl;


	QList<int> bottleneckids;

	QMultiMap<double, int>::iterator rbegin = locobj2tgid.end();
	rbegin--;
	QMultiMap<double, int>::iterator rend = locobj2tgid.begin();
	rend--;


	for (QMultiMap<double, int>::iterator iter = rbegin; iter != rend; iter--) {
		//if (nbottlenecks == bottleneckids.size()) break;

		bottleneckids.append(iter.value());
	}


	// Clear the selections for the other tool groups which are in M
	//res = bottleneckids[Rand::rndInt(0, bottleneckids.size() - 1)];
	res = bottleneckids[Rand::rnd<Math::uint32>(0, bottleneckids.size() - 1)];

	// Select the bottleneck with the highest criticality measure
	double curCrit = -1.0;
	for (int i = 0; i < bottleneckids.size(); i++) {
		int curTGID = bottleneckids[i];
		if (tgID2Criticality[curTGID] > curCrit) {
			curCrit = tgID2Criticality[curTGID];
			res = curTGID;
		}
	}

	// Schedule the machine selected as the most critical
	scheduleTG(res);

	//res = bottleneckids.first();

	//out << "Found bottleneck tg : " << res << endl;
	//getchar();

	foreach(const int &tgid, M) {
		if (tgid != res) {
			TG2Selection.remove(tgid);
		}
	}

	// Unset the machine assignment for the operations from the other TGs not in M
	// Assign the expected processing times for the outgoing arcs

	//    foreach(const int &tgid, M) {
	//        if (tgid != res) {
	//            //out << "Restoring tg: " << tgid << endl;
	//            restoreOpSchedState(pm, TG2Nodes[tgid]);
	//        } else {

	//        }
	//    }


	//out << "Graph after restoring : " << endl << pm[node] << endl;

	//pm.updateHeads();
	//pm.updateStartTimes();

	return res;
}

void SBHSchedulerPinSin::scheduleTG(const int tgid) {
	QTextStream out(stdout);

	out << "SBHSchedulerPinSin::scheduleTG : Trying to schedule machine group : " << tgid << endl;

	if (((rc) (tgid)).types.size() == 1 && ((rc) (tgid)).types.contains(0)) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	//out << "Getting terminals ..." << endl;
	QList<ListDigraph::Node> terminals = pm.terminals();
	//out << "Got terminals." << endl;

	//TGScheduler *tgscheduler;

	/*
	tgscheduler = new TGVNSScheduler1;
	((TGVNSScheduler1*) tgscheduler)->maxIter(0000);
	((TGVNSScheduler1*) tgscheduler)->maxIterDecl(3000);
	((TGVNSScheduler*) tgscheduler)->maxTimeMs(30000);
	((TGVNSScheduler1*) tgscheduler)->sbhscheduler = this;
	 */


	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency before scheduling the tool group..." << endl;
	for (ListDigraph::NodeIt nit(pm[node]->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm[node]->graph, nit); oait != INVALID; ++oait) {
			if (pm[node]->ops[nit]->p() != -pm[node]->p[oait]) {
				out << "op ID = " << pm[node]->ops[nit]->ID << endl;
				out << pm[node] << endl;
				Debugger::err << "SBHSchedulerPinSin::streeScheduleTG : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	//###########################  DEBUG  ######################################
	//out << "Nodes to for scheduling on the TG : " << tgid << endl;
	//for (int i = 0; i < streeTG2Nodes[tgid].size(); i++) {
	//out << pm.ops[streeTG2Nodes[tgid][i]] << endl;
	//}
	//getchar();
	//##########################################################################


	//out << "Running the tool group scheduler..." << endl;

	tgscheduler->node2predST = &node2predST;

	tgscheduler->locD = locD; // Set the local due dates of the operations

	TG2Selection[tgid].tgID = tgid;

#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHSchedulerPinSin::bottleneckTG : Selection arcs are invalid before scheduling the unscheduled machine groups!!!" << ENDL;
			}

		}
	}
#endif	

	tgscheduler->schedule(pm, (rc) (tgid), TG2Nodes[tgid], terminals, dloc, TG2Selection[tgid]);
	//out << "Done running the scheduler." << endl;

	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	out << "Checking consistency after scheduling the tool group..." << endl;
	for (ListDigraph::NodeIt nit(pm[node]->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(pm[node]->graph, nit); oait != INVALID; ++oait) {
			if (pm[node]->ops[nit]->p() != -pm[node]->p[oait]) {
				out << "op ID = " << pm[node]->ops[nit]->ID << endl;
				out << pm[node] << endl;
				Debugger::err << "SBHSchedulerPinSin::streeScheduleTG : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */

	/*
	out << "SBHSchedulerPinSin::streeScheduleTG : Selection:" << endl;
	for (int i = 0; i < streeTG2Selection[node][tgid].selection.size(); i++) {
		out << pm.ops[streeTG2Selection[node][tgid].selection[i].first] << endl;
	}

	double pt;
	ListDigraph::Node s;
	out << "SBHSchedulerPinSin::streeScheduleTG : Checking processing times..." << endl;
	for (int i = 0; i < streeTG2Selection[node][tgid].selection.size(); i++) {
		s = streeTG2Selection[node][tgid].selection[i].first;
		if (pm[node]->ops[s]->machID >= 0) {
			pt = ((rc)(tgid, pm[node]->ops[s]->machID)).procTime(pm[node]->ops[s]);

			if (pm[node]->ops[s]->p() != pt) {
				//out << pm << endl;
				out << "pt = " << pt << endl;
				out << "p = " << pm[node]->ops[s]->p() << endl;
				out << pm[node]->ops[s] << endl;
				Debugger::err << "Something is wrong with the processing time for " << pm[node]->ops[s]->ID << ENDL;
			}
		}
	}
	out << "SBHSchedulerPinSin::streeScheduleTG : Done checking processing times." << endl;
	 */
	//##########################################################################

	//delete tgscheduler;

#ifdef DEBUG
	for (int i = 0; i < btnseq.size(); i++) {
		int curTGID = btnseq[i];
		// Check correctness of the arcs
		QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
		for (int j = 0; j < curSelectionArcs.size(); j++) {
			ListDigraph::Arc curArc = curSelectionArcs[j];

			if (!pm.graph.valid(curArc)) {
				out << "Current TG : " << curTGID << endl;
				Debugger::err << "SBHSchedulerPinSin::bottleneckTG : Selection arcs are invalid after scheduling the unscheduled machine groups!!!" << ENDL;
			}

		}
	}
#endif

	out << "SBHSchedulerPinSin::scheduleTG : Scheduled TG with ID = " << tgid << " with local objective : " << TG2Selection[tgid].localobj << endl << endl << endl;
	getchar();
}

void SBHSchedulerPinSin::reoptScheduleTG(const int tgid) {
	if (((rc) (tgid)).types.size() == 1 && ((rc) (tgid)).types.contains(0)) {
		TG2Selection[tgid].localobj = -1E-300;
		return;
	}

	QList<ListDigraph::Node> terminals = pm.terminals();

	TGScheduler *tgscheduler;
	//if (streeInitSolMode) {
	tgscheduler = new TGVNSScheduler; //*/ TGFIFOScheduler;
	((TGVNSScheduler*) tgscheduler)->maxIterDecl(0);
	((TGVNSScheduler*) tgscheduler)->sbhscheduler = (SBHScheduler*) this;
	//} else {
	//tgscheduler = new TGATCScheduler;
	//}

	tgscheduler->node2predST = &node2predST;

	tgscheduler->schedule(pm, (rc) (tgid), TG2Nodes[tgid], terminals, dloc, TG2Selection[tgid]);

	delete tgscheduler;
}

void SBHSchedulerPinSin::insertTGSelection(const int tgid) {
	QTextStream out(stdout);


	insertSelection(tgid);


	return;
}

void SBHSchedulerPinSin::removeTGSelection(const int tgid) {

	removeSelection(tgid);

	return;
}

double SBHSchedulerPinSin::calculateLocalObj(const int tgid) {
	// Calculate the local objective for the current selection
	//SBHTWTLocalObj lobj;
	UTWT utwt;

	QList<ListDigraph::Node> terminals = pm.terminals();

	double res = utwt(pm, TG2Nodes[tgid], locD); //lobj(pm, TG2Nodes[tgid], TG2Selection[tgid], terminals, dloc);

	return res;
}

double SBHSchedulerPinSin::calculateObj() {
	TWT twt;

	return twt(pm, terminals);
}

void SBHSchedulerPinSin::updateOperationHeads(const QList<ListDigraph::Node>& opnodes) {
	/**Algorithm: (IMPORTANT: arc lengths are negative)
	 * 
	 * 1. Run the Bellman-Ford algorithm on the graph with the negative 
	 *    arc weights and find the shortest path from the source to the given
	 *	  operation.
	 * 
	 * 2. Set the negated found length of the path as the ready time of the operation. 
	 * 
	 */

	//Debugger::info << "Updating operation heads ..." << ENDL;

	//QTextStream out(stdout);
	//out << pm << endl;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm.graph, pm.p);

	bf.init();
	bf.addSource(pm.head);
	//Debugger::info << "Running the BF algorithm..."<<ENDL;
	bf.start();
	//Debugger::info << "Done running the BF algorithm."<<ENDL;


	// #### IMPORTANT  #### Loca ready times of the operations must be updated, but the initial ready times must be considered
	// Update the ready time of the operation
	for (int i = 0; i < opnodes.size(); i++) {
#ifdef DEBUG
		if (!bf.reached(opnodes[i])) {
			Debugger::err << "SBHSchedulerPinSin::updateOperationHead : Operation ID= " << pm.ops[opnodes[i]]->OID << ":" << pm.ops[opnodes[i]]->ID << " can not be reached from the root node " << pm.ops[pm.head]->OID << ":" << pm.ops[pm.head]->ID << "!" << ENDL;
		}
#endif
		pm.ops[opnodes[i]]->r(-bf.dist(opnodes[i]));
		//out << "The found length: " << pm.ops[opnodes[i]]->r() << endl;
	}
	//getchar();

	//Debugger::info << "Done updating operation heads." << ENDL;
	//out << pm << endl;
	//getchar();
}

void SBHSchedulerPinSin::updateOperationTails(ProcessModel& pm, QList<ListDigraph::Node>& opnodes) {
	// Set the due dates of the operations proportionally to their processing times


	return;


	//Debugger::info << "Updating operation tails ..." << ENDL;

	double cmax;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm.graph, pm.p);

	QList<ListDigraph::Node> terminals = pm.terminals();

	for (int j = 0; j < opnodes.size(); j++) {

		bf.run(opnodes[j]);

		dloc[pm.ops[opnodes[j]]->ID].clear();
		for (int i = 0; i < terminals.size(); i++) {
			// Update the i-th local due date equal to the longest path from the operation to the corresponding terminal
			if (!bf.reached(terminals[i])) {
				//Debugger::err << "SBHSchedulerPinSin::updateOperationTail : Terminal " << pm.ops[terminals[i]]->OID << ":" << pm.ops[terminals[i]]->ID << " can not be reached from the operation ID= " << pm.ops[opnodes[j]]->OID << ":" << pm.ops[opnodes[j]]->ID << " !" << ENDL;
				dloc[pm.ops[opnodes[j]]->ID].append(double(Math::MAX_DOUBLE));
			} else {
				// The longest path from the current operation node to the current terminal
				dloc[pm.ops[opnodes[j]]->ID].append(-bf.dist(terminals[i]));
			}
		}
	}
	// Find the makespan
	bf.init();
	bf.addSource(pm.head);
	bf.start();

	double ept = 0.0;

	for (int j = 0; j < opnodes.size(); j++) {
		// Define the processing time depending on whether the operation is scheduled or not
		if (pm.ops[opnodes[j]]->machID > 0) { // Exact processing time
			ept = pm.ops[opnodes[j]]->p();
		} else { // Expected processing time
			ept = (rc) (pm.ops[opnodes[j]]->toolID).shortestProcTime(pm.ops[opnodes[j]]); //expectedProcTime(pm.ops[opnodes[j]]);
		}

		// Set the local due dates
		for (int i = 0; i < terminals.size(); i++) {
			cmax = -bf.dist(terminals[i]);
			if (dloc[pm.ops[opnodes[j]]->ID][i] < Math::MAX_DOUBLE) {
				dloc[pm.ops[opnodes[j]]->ID][i] = Math::max(cmax, terminalopid2d[pm.ops[terminals[i]]->ID]) - dloc[pm.ops[opnodes[j]]->ID][i] + ept;
#ifdef DEBUG
				if (dloc[pm.ops[opnodes[j]]->ID][i] < 0.0) {
					Debugger::eDebug("Local due date < 0!");
				}
#endif

			}
		}
	}
	//Debugger::info << "Done updating operation tails." << ENDL;
}

void SBHSchedulerPinSin::reoptimize(const int last_bottleneck_id) {
	QTextStream out(stdout);

	// Simple reoptimization strategy
	if (options["SBH_REOPT_TYPE"] == "LS") { // Local search reoptimization

		reoptimizePM(last_bottleneck_id);

	} else if (options["SBH_REOPT_TYPE"] == "STD") { // Classical reoptimization

		reoptimizeSimple(last_bottleneck_id);

	} else if (options["SBH_REOPT_TYPE"] == "NONE") { // No reoptimization is performed

		// No reoptimization is performed

	} else {

		Debugger::err << "SBHSchedulerPinSin::reoptimize : SBH_REOPT_TYPE not specified/correct !!!" << ENDL;

	}

	return;

	// IMPORTANT!!! Synchronize the list of arcs corresponding to the tool groups! The local search may change the selection!

	// Synchronize the arcs for the scheduled tool groups

	TG2Arcs.clear();
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (!pm.conjunctive[ait]) {
			//out << "SBHSchedulerPinSin::reoptimize : Synchronizing arc : " << pm.ops[pm.graph.source(ait)]->ID << "->" << pm.ops[pm.graph.target(ait)]->ID << endl;
			TG2Arcs[pm.ops[pm.graph.source(ait)]->toolID].append(QPair<ListDigraph::Node, ListDigraph::Node > (pm.graph.source(ait), pm.graph.target(ait)));
		}
	}

	// TODO : Update the selections

	out << "---" << endl;
	//out << "Found objective of the partial schedule after reoptimization : " << streeCalculateObj(node) << endl << endl << endl;
	// Reoptimization strategy based on the neighborhoods of Mastrolilli/Gambardella and Mati/Perez/Lahloy
	if (M.size() == 0) {
		out << "Scheduled all tool groups." << endl;
		out << "Found objective of the full schedule after reoptimization : " << calculateObj() << endl;
		//getchar();
	}

}

void SBHSchedulerPinSin::reoptimizeSimple(const int last_bottleneck_id) {

	QTextStream out(stdout);

	out << endl << endl;
	out << "Classical reoptimization ..." << endl;
	out << "Last bottleneck tool group : " << last_bottleneck_id << endl;

	bool solutionImproved = false; // Indicates whether at least one rescheduling improved the solution

	double objBeforeReopt = TWT()(pm);
	double bestObj = TWT()(pm);

	do { // Repeat until there are no further solutions

		solutionImproved = false;

		for (int i = 0; i < btnseq.size(); i++) {
			//int curTGID = btnseq[Rand::rndInt(0, btnseq.size() - 1)];
			int curTGID = btnseq[Rand::rnd<Math::uint32>(0, btnseq.size() - 1)];

			if (curTGID == last_bottleneck_id) continue;

			out << endl << endl << "SBHSchedulerPinSin::reoptimizeSimple : Reoptimizing machine group : " << curTGID << endl;

			out << "SBHSchedulerPinSin::reoptimizeSimple : TWT of the partial schedule before the considering the current TG : " << TWT()(pm) << endl;

#ifdef DEBUG			
			// Check correctness of the arcs
			QList<ListDigraph::Arc>& curSelectionArcs = tgID2SelectionArcs[curTGID];
			for (int j = 0; j < curSelectionArcs.size(); j++) {
				ListDigraph::Arc curArc = curSelectionArcs[j];

				if (!pm.graph.valid(curArc)) {
					Debugger::err << "SBHSchedulerPinSin::reoptimizeSimple : Selection arcs are invalid!!!" << ENDL;
				}

			}
#endif			

			// Preserve the previous selection of the machine group
			TGSelection prevSelection = TG2Selection[curTGID];

			// Remove the actual selection of the machine group
			removeTGSelection(curTGID);

			out << "SBHSchedulerPinSin::reoptimizeSimple : TWT of the partial schedule after removing the current selection : " << TWT()(pm) << endl;

			// Reoptimize the current machine group, i.e., schedule it again
			scheduleTG(curTGID);

			// Get the actual selection
			TGSelection& curSelection = (TGSelection&) TG2Selection[curTGID];

			// Check whether the current selection is better than the previous one
			if (curSelection.localobj < bestObj) {

				out << "SBHSchedulerPinSin::reoptimizeSimple : Accepted a better solution with objective : " << curSelection.localobj << endl;

				solutionImproved = true;

				if (curSelection.localobj > objBeforeReopt) {

					out << "TWT before reoptimization : " << objBeforeReopt << endl;
					out << "Accepted solution : " << curSelection.localobj << endl;

					Debugger::err << "SBHSchedulerPinSin::reoptimizeSimple : Accepted a worse solution!!!" << ENDL;
				}

				bestObj = curSelection.localobj;

			} else { // Restore the previous selection

				out << "SBHSchedulerPinSin::reoptimizeSimple : Declined a solution with objective : " << curSelection.localobj << endl;

				// Restore the previous selection
				TG2Selection[curTGID] = prevSelection;

			}

			// Insert the selection after the reoptimization into the graph
			insertTGSelection(curTGID);

		}

	} while (solutionImproved);

	double objAfterReopt = TWT()(pm);

	if (objAfterReopt > objBeforeReopt) {

		out << "TWT before reoptimization : " << objBeforeReopt << endl;
		out << "TWT after reoptimization : " << objAfterReopt << endl;

		Debugger::err << "SBHSchedulerPinSin::reoptimizeSimple : Reoptimization found a worse solution!!!" << ENDL;
	}

	out << "SBHSchedulerPinSin::reoptimizeSimple : TWT of the partial schedule after the reoptimization : " << TWT()(pm) << endl;

	//getchar();

	return;








	TGSelection prev_selection;
	QList<double> prev_dloc;

	double cur_part_obj;
	double best_part_obj = Math::MAX_DOUBLE;

	int no_improvement_steps = 0;
	int max_no_improvement_steps = 3;

	M0.remove(last_bottleneck_id);

	pm.updateHeads();
	pm.updateStartTimes();

	// Update the selections for the scheduled tool groups

	foreach(const int &tgid, M0) {
		// Update operations data in the selections
		for (QMap<ListDigraph::Node, Operation>::iterator iter = TG2Selection[tgid].opNode2SchedOps.begin(); iter != TG2Selection[tgid].opNode2SchedOps.end(); iter++) {
			iter.value().copy(*(pm.ops[iter.key()]));
		}

		// Update the local objective
		TG2Selection[tgid].localobj = calculateLocalObj(tgid);
		out << "TG " << tgid << " : " << TG2Selection[tgid].localobj << endl;

	}

	best_part_obj = calculateObj();

	QList<int> btlncks = btnseq;
	btlncks.removeLast();
	int tgid;

	do {
		if (btlncks.size() == 0) break;

		Rand::randPermut(btlncks);

		// Iterate over the sorted scheduled tool groups
		for (int i = 0; i < btlncks.size(); i++) {

			// Set current tool group ID
			tgid = btlncks[i];

			// Preserve the selection for the current tool group
			prev_selection = TG2Selection[tgid];
			prev_dloc = dloc[tgid];

			// Preserve the scheduling state of the tool group
			preserveOpSchedState(pm, TG2Nodes[tgid]);

			// Remove the current selection of the tool group
			out << "SBHSchedulerPinSin::reoptimizeSimple : Removing selection for TG " << tgid << endl;
			removeTGSelection(tgid);

			// Recalculate heads and start times
			pm.updateHeads();
			pm.updateStartTimes();

			// Recalculate local due dates for the tool group
			//out << "Updating the local due dates..." << endl;
			//out << "Before the update:" << endl;
			//for (int i = 0; i < TG2Nodes[tgid].size(); i++) {
			//out << pm.ops[TG2Nodes[tgid][i]]->ID << " : ";
			//for (int j = 0; j < dloc[pm.ops[TG2Nodes[tgid][i]]->ID].size(); j++) {
			//   out << dloc[pm.ops[TG2Nodes[tgid][i]]->ID][j] << ", ";
			//}
			//out << endl;
			//}
			updateOperationTails(pm, TG2Nodes[tgid]);
			//out << "After the update:" << endl;
			//for (int i = 0; i < TG2Nodes[tgid].size(); i++) {
			//out << pm.ops[TG2Nodes[tgid][i]]->ID << " : ";
			//for (int j = 0; j < dloc[pm.ops[TG2Nodes[tgid][i]]->ID].size(); j++) {
			//   out << dloc[pm.ops[TG2Nodes[tgid][i]]->ID][j] << ", ";
			//}
			//out << endl;
			//}

			//out << "Before rescheduling the tool group : " << tgid << endl;
			//for (int j = 0; j < TG2Nodes[tgid].size(); j++) {
			//out << pm.ops[TG2Nodes[tgid][j]] << endl;
			//out << *(TG2Selection[tgid].opNode2SchedOps[TG2Nodes[tgid][j]]) << endl;

			//}
			//getchar();

			scheduleTG(tgid);

			//out << "TG " << tgid << " : " << TG2Selection[tgid].localobj << endl;
			//out << "Local obj :  " << calculateLocalObj(tgid) << endl;
			//for (QMap<ListDigraph::Node, Operation*>::iterator iter = TG2Selection[tgid].opNode2SchedOps.begin(); iter != TG2Selection[tgid].opNode2SchedOps.end(); iter++) {
			//out << *iter.value() << endl;
			//}
			//getchar();
			//cur_part_obj = calculateObj();
			if (TG2Selection[tgid].localobj < prev_selection.localobj) {
				out << "Updated local obj for tg " << tgid << TG2Selection[tgid].localobj << endl;
			} else {

				TG2Selection[tgid] = prev_selection;
				dloc[tgid] = prev_dloc;

				restoreOpSchedState(pm, TG2Nodes[tgid]);

				//out << "R" << endl;
			}

			// Insert the new selection of the tool group
			//out << "...." << endl;
			insertTGSelection(tgid);
			//out << "....." << endl;

			// Recalculate heads
			pm.updateHeads();

			// Recalculate the completion times of the partial schedule
			pm.updateStartTimes();

			/*
			cur_part_obj = calculateObj();
			if (cur_part_obj > 1.0 * best_part_obj) {
				removeTGSelection(tgid);

				TG2Selection[tgid] = prev_selection;
				dloc[tgid] = prev_dloc;

				restoreOpSchedState(pm, TG2Nodes[tgid]);

				insertTGSelection(tgid);

				pm.updateHeads();
				pm.updateStartTimes();
			}
			 */

		}

		// Recalculate the current TWT for the partial schedule
		cur_part_obj = calculateObj();

		//out << "Currently found objective: " << cur_part_obj << endl;

		if (cur_part_obj < best_part_obj) {
			best_part_obj = cur_part_obj;

			no_improvement_steps = 0;
		} else {
			no_improvement_steps++;
		}

	} while (no_improvement_steps < max_no_improvement_steps);


	M0.insert(last_bottleneck_id);

	pm.updateHeads();
	pm.updateStartTimes();

	out << "Finished classical reoptimization." << endl;
}

void SBHSchedulerPinSin::reoptimizeTG(const int&) {

}

void SBHSchedulerPinSin::reoptimizePM(const int) {
	QTextStream out(stdout);

	QMap < ListDigraph::Node, bool> node2Movable;
	QMap<int, double> btnkID2Prob;

	for (int i = 0; i < btnseq.size(); i++) {
		int curBtnk = btnseq[i];
		btnkID2Prob[curBtnk] = double(btnseq.size() - i) / double(btnseq.size());
	}

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curNode]);

		if (curOp.toolID > 0 && btnseq.contains(curOp.toolID)) {
			//double rndNum = Rand::rndDouble();
			double rndNum = Rand::rnd<double>();

			if (rndNum <= btnkID2Prob[curOp.toolID]) { // Select with some probability as movable
				node2Movable[curNode] = true;
			} else {
				node2Movable[curNode] = false;
			}

		}

	}

	//out << "Running SBHSchedulerPinSin::reoptimizePM... " << endl;
	int lsMaxIter = 0;
	int lsMaxTimeMs = 0;

	if (options["SBH_REOPT_LAST_ITER"] == "false") {

		lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt();
		lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt();

	} else if (options["SBH_REOPT_LAST_ITER"] == "true") { // Perform reoptimization only in the last iteration

		if (M.size() <= 0) {

			//ls.maxIter(100000); // Previously : 200000
			//ls.maxTimeMs(30*1000);

			lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt();
			lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt();

		} else {

			lsMaxIter = options["SBH_REOPT_LS_MAX_ITER"].toInt() / (M0.size() + M.size());
			lsMaxTimeMs = options["SBH_REOPT_LS_MAX_TIME_MS"].toInt() / (M0.size() + M.size());

		}

	} else {

		Debugger::err << "SBHSchedulerPinSin::reoptimizePM : Not clear in which iteration to perform reoptimization!!!" << ENDL;

	}

	ls.maxIter(lsMaxIter);
	ls.maxTimeMs(lsMaxTimeMs);

	//################  DEBUG  #################
	ls.setScheduledTGs(M0.toList());
	//##########################################

	ls.setPM(&pm);
	//ls.setMovableNodes(node2Movable);
	rc.init();
	ls.setResources(&rc);
	ls.checkCorrectness(false);
	//out << "Running the local search reoptimization ..." << endl;
	ls.run();

	// IMPORTANT!!! The selections in case of LS are not updated automatically!!! This may cause some validity problems if they are manipulated later

	return;
}

bool SBHSchedulerPinSin::stopCondition() {
	bool stop = true;

	stop = M.size() == 0;

	return stop; //M.size() == 0;
}

void SBHSchedulerPinSin::stopActions() {
	//Debugger::wDebug("SBHSchedulerPinSin::stopActions not implemented!");
	QTextStream out(stdout);

	out << "SBHSchedulerPinSin::stopActions : Final results:" << endl;

	out << "SBHSchedulerPinSin::stopActions : The sequences of bottlenecks: " << endl;

	for (int i = 0; i < btnseq.size(); i++) {
		out << btnseq[i] << " ";
	}
	out << " | TWT = " << calculateObj();
	out << endl;
	out << endl;
	//getchar();

	//out << "Final schedule:" << endl << pm << endl;

	out << endl;
}

void SBHSchedulerPinSin::postprocessingActions() {
	/** Algorithm:
	 * 
	 * 1. Calculate the total objective and generate the corresponding schedule.
	 * 
	 * 2. Remove the selections of all of the tool groups 
	 *    from the process model.
	 * 
	 * 3. Bring the process model to the initial state (state before 
	 *    the scheduling actions)
	 */

	QTextStream out(stdout);

	//out << "Performing postprocessing ..." << endl;

	sched->fromPM(pm, *obj);

	//Debugger::info << "Done collecting schedule data." << ENDL;
	Debugger::info << "Found objective value : " << sched->objective << ENDL;
	//return;
	//getchar();

	//out << "Postprocessing finished." << endl;
	//getchar();

	// Restore the initial state of the process model
	/*
	QList<ListDigraph::Arc> arcs;
	for (QHash<int, QVector<QPair<ListDigraph::Node, ListDigraph::Node > > >::iterator iter = TG2Arcs.begin(); iter != TG2Arcs.end(); iter++) {
		// Iterate over the arcs of each machine group
		for (int ca = 0; ca < iter.value().size(); ca++) {
			//out << "SBHSchedulerPinSin::postprocessingActions : Considering to delete the arc : " << pm.ops[iter.value()[ca].first]->ID << "->" << pm.ops[iter.value()[ca].second]->ID << endl;

			// Get the arcs
			arcs = pm.arcs(iter.value()[ca].first, iter.value()[ca].second);

			// Delete the current arc from the graph
			for (int i = 0; i < arcs.size(); i++) {
				if (!pm.conjunctive[arcs[i]]) {
					//out << "SBHSchedulerPinSin::postprocessingActions : Deleting arc : " << pm.ops[pm.graph.source(arcs[i])]->ID << "->" << pm.ops[pm.graph.target(arcs[i])]->ID << endl;
					pm.graph.erase(arcs[i]);
				}
			}
		}
	}
	TG2Arcs.clear();
	 */
	//out << "SBHSchedulerPinSin::postprocessingActions : Done erasing SB arcs." << endl;

	// Restore the processing times and the arc lengths in the graph
	/*
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		// Set the ready times to the initial value
		pm.ops[nit]->r(pm.ops[nit]->ir());

		// Set the processing time to zero
		pm.ops[nit]->p(0.0);

		// Initialize the start times
		pm.ops[nit]->s(0.0);

		// Mark the operation as unscheduled
		pm.ops[nit]->machID = -1;

		// Set the length of the outgoing arcs
		for (ListDigraph::OutArcIt oait(pm.graph, nit); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[nit]->p();
		}
	}
	 */

	// Clear the scheduler
	clear();

	//out << "SBHSchedulerPinSin::postprocessingActions : Restored the initial state of the PM." << endl;

	//out << pm << endl;
	//getchar();

}

void SBHSchedulerPinSin::preserveOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedr[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->r();
	}
}

void SBHSchedulerPinSin::restoreOpReadyTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedr.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore r for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->r(_opid2preservedr.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHSchedulerPinSin::preserveOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedd[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->d();
	}
}

void SBHSchedulerPinSin::restoreOpDueDates(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedd.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore d for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->d(_opid2preservedd.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHSchedulerPinSin::preserveOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedp[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->p();

		//Debugger::info << "Preserving (" << pm.ops[opnodes[i]]->ID << "," << pm.ops[opnodes[i]]->p() << ENDL;
	}
}

void SBHSchedulerPinSin::restoreOpProcTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedp.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore p for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->p(_opid2preservedp.value(pm.ops[opnodes[i]]->ID));

			//Debugger::info << "Restoring (" << pm.ops[opnodes[i]]->ID << "," << pm.ops[opnodes[i]]->p() << ENDL;
		}
	}
}

void SBHSchedulerPinSin::preserveOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preserveds[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->s();
	}
}

void SBHSchedulerPinSin::restoreOpStartTimes(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preserveds.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore s for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->s(_opid2preserveds.value(pm.ops[opnodes[i]]->ID));
		}
	}
}

void SBHSchedulerPinSin::preserveOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		_opid2preservedm[pm.ops[opnodes[i]]->ID] = pm.ops[opnodes[i]]->machID;
	}
}

void SBHSchedulerPinSin::restoreOpMachAssignment(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		if (!(_opid2preservedm.contains(pm.ops[opnodes[i]]->ID))) {
			Debugger::err << "Failed to restore m for operation ID=" << pm.ops[opnodes[i]]->ID << ENDL;
		} else {
			pm.ops[opnodes[i]]->machID = _opid2preservedm.value(pm.ops[opnodes[i]]->ID);
		}
	}
}

void SBHSchedulerPinSin::preserveOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	preserveOpReadyTimes(pm, opnodes);
	preserveOpDueDates(pm, opnodes);
	preserveOpProcTimes(pm, opnodes);
	preserveOpStartTimes(pm, opnodes);
	preserveOpMachAssignment(pm, opnodes);
}

void SBHSchedulerPinSin::restoreOpSchedState(ProcessModel& pm, QList<ListDigraph::Node> &opnodes) {
	restoreOpReadyTimes(pm, opnodes);
	restoreOpDueDates(pm, opnodes);
	restoreOpProcTimes(pm, opnodes);
	restoreOpStartTimes(pm, opnodes);
	restoreOpMachAssignment(pm, opnodes);
}

void SBHSchedulerPinSin::restoreInitialR(QList<ListDigraph::Node> &opnodes) {
	int sz = opnodes.size();

	for (int i = 0; i < sz; i++) {
		pm.ops[opnodes[i]]->r(pm.ops[opnodes[i]]->ir());
	}
}

void SBHSchedulerPinSin::removeSelection(const int& tgID) {
	// Remove all arcs which have previously been inserted and CHANGE THE OPERATIONS
	QTextStream out(stdout);

	// Operations of the tool group
	QMap<ListDigraph::Node, Operation>& prevNodeOper = (QMap<ListDigraph::Node, Operation>&) (tgID2PrevNodeOper[tgID]);
	QList<ListDigraph::Arc>& selectionArcs = (QList<ListDigraph::Arc>&) (tgID2SelectionArcs[tgID]);

	//out << "SBHSchedulerPinSin::removeSelection : TG : " << tgID << endl;
	//out << "SBHSchedulerPinSin::removeSelection : Restoring nodes : " << prevNodeOper.size() << endl;
	//out << "SBHSchedulerPinSin::removeSelection : Removing arcs : " << selectionArcs.size() << endl;

	if (prevNodeOper.size() == 0) { // In case there is nothing to remove
		selectionArcs.clear();
		return;
	}

	// Restore the operations
	for (QMap<ListDigraph::Node, Operation>::iterator iter = prevNodeOper.begin(); iter != prevNodeOper.end(); iter++) {
		ListDigraph::Node curNode = iter.key();
		Operation& curOper = (Operation&) iter.value();

		// Restore the operation
		*(pm.ops[curNode]) = curOper;

		// Set the correct outgoing arc lengths
		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
			pm.p[oait] = -curOper.p();
		}

	}

	// Remove the selection arcs
	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];

#ifdef DEBUG		
		if (!pm.graph.valid(curArc)) {
			Debugger::err << "SBHSchedulerPinSin::removeSelection : Trying to remove an invalid arc!!!" << ENDL;
		};
#endif

		// Remove the arc
		pm.graph.erase(curArc);

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	// Clear the previous data
	selectionArcs.clear();
	prevNodeOper.clear();
	tgID2PrevNodeOper.remove(tgID);
	tgID2SelectionArcs.remove(tgID);

	/*
#ifdef DEBUG

	// Check whether there are multiple disjunctinve arcs between any two nodes
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curStartNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curStartNode]);

		if (curOp.toolID != tgID) continue; // Omit other machine groups

		QMap<ListDigraph::Node, int> targetNode2Numarcs;

		for (ListDigraph::OutArcIt oait(pm.graph, curStartNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			ListDigraph::Node curEndNode = pm.graph.target(curArc);

			if (!pm.conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

			if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
				Debugger::info << "Current TG : " << tgID << ENDL;
				Debugger::err << "SBHSchedulerPinSin::removeSelection : Too many disjunctive arcs between the nodes after removing the selection!!!" << ENDL;
			}

		}

	}
#endif
	 */

}

void SBHSchedulerPinSin::insertSelection(const int& tgID) {
	QTextStream out(stdout);
	/*
	#ifdef DEBUG

		// Check whether there are multiple disjunctinve arcs between any two nodes
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
			ListDigraph::Node curStartNode = nit;
			Operation& curOp = (Operation&) *(pm.ops[curStartNode]);

			if (curOp.toolID != tgID) continue; // Omit other machine groups

			QMap<ListDigraph::Node, int> targetNode2Numarcs;

			for (ListDigraph::OutArcIt oait(pm.graph, curStartNode); oait != INVALID; ++oait) {
				ListDigraph::Arc curArc = oait;
				ListDigraph::Node curEndNode = pm.graph.target(curArc);

				if (!pm.conjunctive[curArc]) targetNode2Numarcs[curEndNode]++;

				if (targetNode2Numarcs[curEndNode] > 0) { // > 0 since there should be no arcs for the machine group at the moment
					Debugger::info << "Current TG : " << tgID << ENDL;
					Debugger::err << "SBHSchedulerPinSin::insertSelection : Too many disjunctive arcs between the nodes before inserting the selection!!!" << ENDL;
				}

			}

		}
	#endif	
	 */

	// Remove the previous selection
	removeSelection(tgID);

	// Operations of the tool group
	QMap<ListDigraph::Node, Operation>& prevNodeOper = (QMap<ListDigraph::Node, Operation>&) (tgID2PrevNodeOper[tgID]);
	QList<ListDigraph::Arc>& selectionArcs = (QList<ListDigraph::Arc>&) (tgID2SelectionArcs[tgID]);
	TGSelection& selection = (TGSelection&) (TG2Selection[tgID]);

	//out << "SBHSchedulerPinSin::insertSelection : TG : " << tgID << endl;
	//out << "SBHSchedulerPinSin::insertSelection : Prev. nodes nodes : " << prevNodeOper.size() << endl;
	//out << "SBHSchedulerPinSin::insertSelection : Prev. arcs arcs : " << selectionArcs.size() << endl;

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsToAdd = selection.selection;
	selectionArcs = pm.addArcs(arcsToAdd);

	QList<ListDigraph::Node> opsToChange = selection.opNode2SchedOps.keys();

	//cout << "Added arcs : " << arcsAdded.size() << endl;

	for (int i = 0; i < opsToChange.size(); i++) { // Insert the nodes to be changed. Important!!! Inserting arcs only is not enough since there can be only one operation on a machine

		ListDigraph::Node curNode = opsToChange[i];

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		// Preserve the previous operation data
		prevNodeOper[curNode] = *(pm.ops[curNode]);

		// Set the new operation data according to the selection
		*(pm.ops[curNode]) = curOper;

		// Iterate over the outgoing arcs of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
			pm.p[oait] = -curOper.p();
		}

	}

	for (int i = 0; i < selectionArcs.size(); i++) {
		ListDigraph::Arc curArc = selectionArcs[i];
		ListDigraph::Node curNode = pm.graph.source(curArc);

		Operation& curOper = (Operation&) selection.opNode2SchedOps[curNode];

		pm.conjunctive[curArc] = false;

		pm.p[curArc] = -curOper.p();

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);
}

void SBHSchedulerPinSin::debugCheckReachability(const int& mid, ProcessModel& pm) {
	QTextStream out(stdout);

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

	QList<ListDigraph::Node> trgmachnodes; // Nodes currently on the target machine
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;
	ListDigraph::NodeMap<bool> scheduled(pm.graph, false);
	ListDigraph::NodeMap<bool> available(pm.graph, false);

	ListDigraph::Node suc;
	ListDigraph::Node sucpred;

	q.enqueue(pm.head);
	scheduled[pm.head] = false;
	available[pm.head] = true;

	// Collect operation sequences on the target machine
	while (q.size() > 0) {
		curnode = q.dequeue();

		if (available[curnode] && !scheduled[curnode]) {
			if ((pm.ops[curnode]->ID > 0) && (pm.ops[curnode]->machID == mid)) {
				trgmachnodes.append(curnode);
			}

			scheduled[curnode] = true;

			// Enqueue the successors
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				suc = pm.graph.target(oait);
				if (!scheduled[suc]) {

					// Update availability

					available[suc] = true;
					for (ListDigraph::InArcIt iait(pm.graph, suc); iait != INVALID; ++iait) {
						sucpred = pm.graph.source(iait);
						if (!scheduled[sucpred]) {
							available[suc] = false;
							break;
						}
					}

					if (available[suc]) {
						q.enqueue(suc);
					}
				}
			}
		} else {
			if (!available[curnode]) {
				q.enqueue(curnode);
			}
		}

	}

	for (int j = 0; j < trgmachnodes.size() - 1; j++) {

		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (trgmachnodes.at(j), trgmachnodes.at(j + 1)));

	}

	if (trgmachnodes.size() > 0) {
		res.prepend(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, trgmachnodes.first()));
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (trgmachnodes.last(), INVALID));
	}

	// In case there are no operations on the target machine
	if (trgmachnodes.size() == 0) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, INVALID));
	}

	// ###################  DEBUG: can be deleted  #################################   

	/*
	out << "operations on machine " << mid << " : " << endl;
	for (int k = 0; k < trgmachnodes.size(); k++) {
		out << pm.ops[trgmachnodes[k]]->ID << ",";
	}

	out << endl << endl;
	 */
	//out << "GBM:" << endl;
	//out << pm << endl;

	for (int j = 0; j < res.size(); j++) {
		//	out << pm.ops[res[i].first]->ID << "->" << pm.ops[res[i].second]->ID << endl;
		if (!reachable(pm, res[j].first, res[j].second)) {
			out << "Not reachable : " << pm.ops[res[j].first]->ID << "->" << pm.ops[res[j].second]->ID << endl;

			out << "operations on machine " << mid << " : " << endl;
			for (int k = 0; k < trgmachnodes.size(); k++) {
				out << pm.ops[trgmachnodes[k]]->ID << ",";
			}

			out << endl << endl;

			out << pm << endl;
			getchar();
		}
	}

	// #############################################################################
}

bool SBHSchedulerPinSin::reachable(ProcessModel& pm, const ListDigraph::Node& s, const ListDigraph::Node & t) {
	QQueue<ListDigraph::Node> q;
	ListDigraph::Node curnode;

	q.enqueue(t);

	if (s == t) return true;

	if (s == INVALID || t == INVALID) return true;

	while (q.size() > 0) {
		curnode = q.dequeue();

		// Iterate over the predecessors
		for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
			if (pm.graph.source(iait) == s) {
				return true;
			} else {
				q.enqueue(pm.graph.source(iait));
			}
		}
	}

	return false;
}

/**  **********************************************************************  **/