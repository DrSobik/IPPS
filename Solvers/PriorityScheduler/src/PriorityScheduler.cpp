/* 
 * File:   TrivialScheduler.cpp
 * Author: DrSobik
 * 
 * Created on July 21, 2011, 9:59 AM
 */

#include <QMap>
#include <QtCore/qtextstream.h>
#include <stdexcept>
#include <QtCore/qdatetime.h>
#include <QStringList>
#include <lemon/list_graph.h>

//#include "TrivialScheduler.h"
#include "PriorityScheduler.h"
#include "Objective.h"

TrivialScheduler::TrivialScheduler() { }

TrivialScheduler::~TrivialScheduler() { }

bool TrivialScheduler::schedule(ProcessModel &pm, Resources &rc, Schedule &schedule) {
	Debugger::info << "Building schedule using trivial assignment." << ENDL;

	/** Algorithm:
	 *  1. Perform topological sort in the graph of the process model.
	 *  2. One by one select the sorted AVAILABLE operations and assign them to 
	 *     the first available machine of the corresponding tool group. After 
	 *     assignment of one operation update the ready times of the following 
	 *     operations.
	 *	3. Collect the relevant data and calculate the objective data. */

	ListDigraph::NodeMap<int> tnodes(pm.graph); // Map of nodes sorted topologically
	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int nscheduled = 0;

	QTextStream out(stdout);

	//Debugger::info << "Resources before scheduling:" << ENDL;
	//out << rc;


	//Debugger::info << "Sorting the graph topologically ..." << ENDL;
	topologicalSort(pm.graph, tnodes);
	//Debugger::info << "Done sorting the graph topologically." << ENDL;

	QMap<int, ListDigraph::Node> stnodes; // Topologically sorted nodes 

	for (ListDigraph::NodeMap<int>::MapIt mi(tnodes); mi != INVALID; ++mi) {
		stnodes[*mi] = mi;
		//cout <<*mi<<" node id="<<pm.graph.id(stnodes[*mi])<<endl;
	}

	//for (QMap<int, ListDigraph::Node>::iterator sti = stnodes.begin(); sti != stnodes.end(); sti++) {
	//	cout << sti.key() << " node id=" << pm.graph.id(*sti) << endl;
	//}

	// Set the head node available
	nodesavail[pm.head] = true;

	//cout << "Currently available nodes:"<<endl;


	// Estimate due dates for all operations based on the due date of the 
	// last operation for each product.

	//Bfs<ListDigraph> bfs(pm.graph);
	//bfs.init();
	//bfs.addSource(pm.head);
	ListDigraph::Node curnode;

	//while (!bfs.emptyQueue()) {
	//	curnode = bfs.processNextNode();
	//	out << "Processing node with id= " << pm.graph.id(curnode) << " ord id=" << pm.ops[curnode]->OID <<
	//			" id=" << pm.ops[curnode]->ID << " type=" << pm.ops[curnode]->type << endl;
	//}

	Operation *so;
	Operation *to;
	ReverseDigraph<ListDigraph> rg(pm.graph);
	Bfs<ReverseDigraph<ListDigraph> > bfsr(rg);
	bfsr.init();
	bfsr.addSource(pm.tail);

	//out << "Running BFS algorithm on the reverse graph ..." << endl;
	while (!bfsr.emptyQueue()) {
		curnode = bfsr.processNextNode();
		if (pm.ops[curnode]->ID < 0) continue;

		//out << "Processing node with id= " << rg.id(curnode) << " " << *(pm.ops[curnode]) << endl;

		//out << "Next available nodes:" << endl;
		for (ReverseDigraph<ListDigraph>::OutArcIt it(rg, curnode); it != INVALID; ++it) {
			// Update the due dates of the reverse target nodes
			// Rev. target d == rev. source d. - rev. source longest processing time
			so = pm.ops[rg.source(it)];
			to = pm.ops[rg.target(it)];
			to->d(so->d() - rc(so->toolID).slowestMachine(so->type).procTime(so));

			//out << "Node with id= " << rg.id(rg.target(it)) << " " << *(pm.ops[rg.target(it)]) << endl;
		}

	}
	so = to = NULL;
	//out << "Done running BFS algorithm on the reverse graph." << endl;


	QVector<ListDigraph::Node> anodes;
	anodes.reserve(stnodes.size());

	QVector<ListDigraph::Node> atcanodes; // Available nodes sorted with ATC
	atcanodes.reserve(stnodes.size());

	while (!nodesscheduled[pm.tail]) {

		// One iteration of the algorithm

		// Collect nodes available for scheduling
		anodes.resize(0);
		for (QMap<int, ListDigraph::Node>::iterator sti = stnodes.begin(); sti != stnodes.end(); sti++) {
			if (nodesavail[*sti] && !nodesscheduled[*sti]) {
				anodes.append(*sti);
			}
		}


		/*
		// Sort available nodes with ATC
		atcanodes = anodes;
		int atcidxstart = 0;
		int atcidxend = atcanodes.size();
		int atccuridx = 0;
		double curI = 0.0;
		double prevI = 0.0;
		double k = 2.3;
		double pavg;
		int curtool;

		while (atcidxstart < atcidxend) {
						curtool = pm.ops[atcanodes[atcidxstart]]->toolID;

						//if (atcanodes.size() == 1) {
						//	atccuridx = 0;
						//	break;
						//}

						// Calculate rest processing time
						pavg = 0.0;
						int natcops = 0;
						for (int i = atcidxstart; i < atcidxend; i++) {
										if (pm.ops[atcanodes[i]]->toolID != curtool) continue;
										pavg += rc(pm.ops[atcanodes[i]]->toolID).expectedProcTime(pm.ops[atcanodes[i]]);
										natcops++;
						}
						pavg /= double(natcops);

						// Find index of the node with the biggest value of ATC

						prevI = 0.0;
						for (int i = atcidxstart; i < atcidxend; i++) {
										if (pm.ops[atcanodes[i]]->toolID != curtool) continue;
										double t = rc(pm.ops[atcanodes[i]]->toolID).nextAvailable().time(); //rc(pm.ops[atcanodes[i]]->toolID).fastestAvailable(pm.ops[atcanodes[i]]->r(),pm.ops[atcanodes[i]]->type).time();
										curI = pm.ops[atcanodes[i]]->w() / rc(pm.ops[atcanodes[i]]->toolID).expectedProcTime(pm.ops[atcanodes[i]])
		 * Math::exp(-1.0 / (k * pavg) * Math::max(0.0, pm.ops[atcanodes[i]]->d() - rc(pm.ops[atcanodes[i]]->toolID).expectedProcTime(pm.ops[atcanodes[i]]) - t));

										//out << "Current ATC index= " << curI << endl;
										if (curI > prevI) {
														prevI = curI;
														atccuridx = i;
										}
						}

						// Swap the nodes with indices atccuridx and atcidxstart
						if (atccuridx != atcidxstart)
										qSwap(atcanodes[atccuridx], atcanodes[atcidxstart]);


						atcidxstart++;
		}
		 */


		//out << "Size of anodes: " << anodes.size() << endl;
		//out << "Size of atcanodes: " << atcanodes.size() << endl;

		// Sort the available nodes by their weights descending
		NodeWeightComparatorGreater nwcg(&pm);
		NodeWeightDueComparatorGreater nwdcg(&pm);
		NodeWSPTComparatorGreater nwsptcg(&pm, &rc);
		NodeExpProcTimeComparatorGreater neptcg(&pm, &rc);
		qSort(anodes.begin(), anodes.end(), nwcg);
		//anodes = atcanodes;

		//out << endl << "Sorted operations' types:" << endl;
		//for (int i = 0; i < anodes.size(); i++) {
		//	out << pm.ops[anodes[i]]->type << ",";
		//}
		//out << endl;

		// Schedule the available operations
		for (int i = 0; i < anodes.size(); i++) {
			// Select the first available machine from the corresponding tool group
			//Machine &m = rc(pm.ops[anodes[i]]->toolID).nextAvailable();

			// Select the fastest available machine from the corresponding tool group
			//Machine &m = rc(pm.ops[anodes[i]]->toolID).fastestAvailable(pm.ops[anodes[i]]->r(), pm.ops[anodes[i]]->type);

			// Select the machine from the corresponding tool group to finish the operation the earliest
			Machine &m = rc(pm.ops[anodes[i]]->toolID).earliestToFinish(pm.ops[anodes[i]]);

			//pm.ops[anodes[i]]->write(out);

			//out << m << endl;

			if (!m.type2speed.contains(pm.ops[anodes[i]]->type)) continue;

			m << pm.ops[anodes[i]];

			nodesavail[anodes[i]] = false;
			nodesscheduled[anodes[i]] = true;
			nscheduled++;

			// Set the length of the corresponding arc in the graph
			for (ListDigraph::OutArcIt oait(pm.graph, anodes[i]); oait != INVALID; ++oait) {
				pm.p[oait] = pm.ops[anodes[i]]->p();
			}

			//break;
			//out << m << endl;
			//out << rc(pm.ops[anodes[i]]->toolID) << endl;
			//getchar();
		}


		// Mark the operations as "not available" and set the successive operations
		// as "available" if and only if all of its predecessors are scheduled. 
		// Update the ready times of the new "available" operations.

		for (int i = 0; i < anodes.size(); i++) {
			nodesavail[anodes[i]] = false;

			if (!nodesscheduled[anodes[i]]) continue;

			// Mark all direct successors of this node as "available" if and only if 
			// all of the direct predecessors of the successor have been scheduled.
			ListDigraph::Node cursucc;
			for (ListDigraph::OutArcIt it(pm.graph, anodes[i]); it != INVALID; ++it) {
				cursucc = pm.graph.target(it);
				nodesavail[cursucc] = true;
				// Check whether all direct predecessors of cursucc are scheduled.
				for (ListDigraph::InArcIt init(pm.graph, cursucc); init != INVALID; ++init) {
					nodesavail[cursucc] = nodesavail[cursucc] && nodesscheduled[pm.graph.source(init)];
				}

				if (nodesavail[cursucc]) {
					//out << "Now available : " << pm.graph.id(cursucc) << endl;
					// Update ready time of the newly enabled node
					pm.ops[cursucc]->r(0.0);
					for (ListDigraph::InArcIt init(pm.graph, cursucc); init != INVALID; ++init) {
						pm.ops[cursucc]->r(Math::max(pm.ops[cursucc]->r(), pm.ops[pm.graph.source(init)]->c()));
					}

				} else {
					//out << "Node " << pm.graph.id(cursucc) << " is not available!" << endl;
				}
			}

		}

	}

	//Debugger::info << "Resources after scheduling:" << ENDL << ENDL;
	//out << rc;

	// Iterate over all nodes directly preceding the tail and
	//Debugger::info << "Collecting schedule data ..." << ENDL;
	schedule.objective = 0.0;
	//for (ListDigraph::InArcIt init(pm.graph, pm.tail); init != INVALID; ++init) {
	//	schedule.TWT += pm.ops[pm.graph.source(init)]->wT();
	//}

	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (pm.graph.target(ait) == pm.tail) {
			if (pm.ops[pm.graph.source(ait)]->ID <= 0) {
				for (ListDigraph::InArcIt iait(pm.graph, pm.graph.source(ait)); iait != INVALID; ++iait) {
					schedule.objective += pm.ops[pm.graph.source(iait)]->wT();
				}
			} else {
				schedule.objective += pm.ops[pm.graph.source(ait)]->wT();
			}
		}
	}

	/* // Cmax
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
					if (pm.graph.target(ait) == pm.tail) {
									if (pm.ops[pm.graph.source(ait)]->ID < 0) {
													for (ListDigraph::InArcIt iait(pm.graph, pm.graph.source(ait)); iait != INVALID; ++iait) {
																	if (schedule.objective < pm.ops[pm.graph.source(iait)]->c())
																					schedule.objective = pm.ops[pm.graph.source(iait)]->c();
													}
									} else {
													if (schedule.objective < pm.ops[pm.graph.source(ait)]->c())
																	schedule.objective = pm.ops[pm.graph.source(ait)]->c();
									}
					}
	}
	 */

	//Debugger::info << "Done collecting schedule data." << ENDL;

	out << pm << endl;
	out << rc << endl;

	return true;
}

/** ******************** Trivial balance scheduler ************************* **/

TrivialBalanceScheduler::TrivialBalanceScheduler() { }

TrivialBalanceScheduler::~TrivialBalanceScheduler() { }

bool TrivialBalanceScheduler::schedule(ProcessModel &pm, Resources &rc, Schedule &schedule) {
	Debugger::info << "Building schedule using trivial assignment." << ENDL;

	/** Algorithm:
	 *  1. Perform topological sort in the graph of the process model.
	 *  2. Select the first available machine, which is able to process at least one
	 *	   of the available operations.
	 *  3. Find the available operation, which can be processed on the machine,
	 *     with the largest processing time.
	 *  4. Schedule the selected operation on the selected machine
	 
	 *	5. Collect the relevant data and calculate the objective data. */

	ListDigraph::NodeMap<int> tnodes(pm.graph); // Map of nodes sorted topologically
	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int nscheduled = 0;

	QTextStream out(stdout);

	//Debugger::info << "Resources before scheduling:" << ENDL;
	//out << rc;


	//Debugger::info << "Sorting the graph topologically ..." << ENDL;
	topologicalSort(pm.graph, tnodes);
	//Debugger::info << "Done sorting the graph topologically." << ENDL;

	QMap<int, ListDigraph::Node> stnodes; // Topologically sorted nodes 

	for (ListDigraph::NodeMap<int>::MapIt mi(tnodes); mi != INVALID; ++mi) {
		stnodes[*mi] = mi;
		//cout <<*mi<<" node id="<<pm.graph.id(stnodes[*mi])<<endl;
	}

	//for (QMap<int, ListDigraph::Node>::iterator sti = stnodes.begin(); sti != stnodes.end(); sti++) {
	//	cout << sti.key() << " node id=" << pm.graph.id(*sti) << endl;
	//}

	// Set the head node available
	nodesavail[pm.head] = true;

	//cout << "Currently available nodes:"<<endl;


	// Estimate due dates for all operations based on the due date of the 
	// last operation for each product.

	//Bfs<ListDigraph> bfs(pm.graph);
	//bfs.init();
	//bfs.addSource(pm.head);
	ListDigraph::Node curnode;

	//while (!bfs.emptyQueue()) {
	//	curnode = bfs.processNextNode();
	//	out << "Processing node with id= " << pm.graph.id(curnode) << " ord id=" << pm.ops[curnode]->OID <<
	//			" id=" << pm.ops[curnode]->ID << " type=" << pm.ops[curnode]->type << endl;
	//}

	Operation *so;
	Operation *to;
	ReverseDigraph<ListDigraph> rg(pm.graph);
	Bfs<ReverseDigraph<ListDigraph> > bfsr(rg);
	bfsr.init();
	bfsr.addSource(pm.tail);

	//out << "Running BFS algorithm on the reverse graph ..." << endl;
	while (!bfsr.emptyQueue()) {
		curnode = bfsr.processNextNode();
		if (pm.ops[curnode]->ID < 0) continue;

		//out << "Processing node with id= " << rg.id(curnode) << " " << *(pm.ops[curnode]) << endl;

		//out << "Next available nodes:" << endl;
		for (ReverseDigraph<ListDigraph>::OutArcIt it(rg, curnode); it != INVALID; ++it) {
			// Update the due dates of the reverse target nodes
			// Rev. target d == rev. source d. - rev. source longest processing time
			so = pm.ops[rg.source(it)];
			to = pm.ops[rg.target(it)];
			to->d(so->d() - rc(so->toolID).slowestMachine(so->type).procTime(so));

			//out << "Node with id= " << rg.id(rg.target(it)) << " " << *(pm.ops[rg.target(it)]) << endl;
		}

	}
	so = to = NULL;
	//out << "Done running BFS algorithm on the reverse graph." << endl;


	QVector<ListDigraph::Node> anodes;
	anodes.reserve(stnodes.size());

	QVector<ListDigraph::Node> atcanodes; // Available nodes sorted with ATC
	atcanodes.reserve(stnodes.size());

	while (!nodesscheduled[pm.tail]) {

		// One iteration of the algorithm

		// Collect nodes available for scheduling
		anodes.resize(0);
		for (QMap<int, ListDigraph::Node>::iterator sti = stnodes.begin(); sti != stnodes.end(); sti++) {
			if (nodesavail[*sti] && !nodesscheduled[*sti]) {
				anodes.append(*sti);
			}
		}

		// Select the first available machine, for which there are available operations
		QMap<double, Machine*> avail_mahines;
		QList<Machine*> all_machines = rc.machines();

		// Iterate over machines
		for (int i = 0; i < all_machines.size(); i++) {
			// Iterate over the nodes
			for (int j = 0; j < anodes.size(); j++) {
				if (all_machines[i]->type2speed.contains(pm.ops[anodes[j]]->type)) {
					avail_mahines[all_machines[i]->time()] = all_machines[i];
					break;
				}
			}
		}

		// Select the first relevant available machine
		Machine &m = *(avail_mahines.begin().value());

		// Get the relevant operation with the biggest processing time
		QMap<double, ListDigraph::Node> rel_ops;
		rel_ops.clear();
		for (int j = 0; j < anodes.size(); j++) {
			if (m.type2speed.contains(pm.ops[anodes[j]]->type)) {
				rel_ops[pm.ops[anodes[j]]->d()] = anodes[j];

				//out << m << endl;
				//out << *(pm.ops[anodes[j]]) << endl;
				//getchar();
			}
		}

		//out << "Size of rel_ops = " << rel_ops.size() << endl;
		ListDigraph::Node cur_node = rel_ops.begin().value();

		//out << m << endl;
		//out << *(pm.ops[cur_node]) << endl;

		// Schedule the selected operation on the machine
		m << pm.ops[cur_node];

		nodesavail[cur_node] = false;
		nodesscheduled[cur_node] = true;
		nscheduled++;

		// Set the length of the corresponding arc in the graph
		for (ListDigraph::OutArcIt oait(pm.graph, cur_node); oait != INVALID; ++oait) {
			pm.p[oait] = pm.ops[cur_node]->p();
		}

		// Mark the operations as "not available" and set the successive operations
		// as "available" if and only if all of its predecessors are scheduled. 
		// Update the ready times of the new "available" operations.

		// Mark all direct successors of this node as "available" if and only if 
		// all of the direct predecessors of the successor have been scheduled.
		ListDigraph::Node cursucc;
		for (ListDigraph::OutArcIt it(pm.graph, cur_node); it != INVALID; ++it) {
			cursucc = pm.graph.target(it);
			nodesavail[cursucc] = true;
			// Check whether all direct predecessors of cursucc are scheduled.
			for (ListDigraph::InArcIt init(pm.graph, cursucc); init != INVALID; ++init) {
				nodesavail[cursucc] = nodesavail[cursucc] && nodesscheduled[pm.graph.source(init)];
			}

			if (nodesavail[cursucc]) {
				//out << "Now available : " << pm.graph.id(cursucc) << endl;
				// Update ready time of the newly enabled node
				pm.ops[cursucc]->r(0.0);
				for (ListDigraph::InArcIt init(pm.graph, cursucc); init != INVALID; ++init) {
					pm.ops[cursucc]->r(Math::max(pm.ops[cursucc]->r(), pm.ops[pm.graph.source(init)]->c()));
				}

			} else {
				//out << "Node " << pm.graph.id(cursucc) << " is not available!" << endl;
			}
		}


		//getchar();
	}

	//Debugger::info << "Resources after scheduling:" << ENDL << ENDL;
	//out << rc;

	// Iterate over all nodes directly preceding the tail and
	//Debugger::info << "Collecting schedule data ..." << ENDL;
	schedule.objective = 0.0;
	//for (ListDigraph::InArcIt init(pm.graph, pm.tail); init != INVALID; ++init) {
	//	schedule.TWT += pm.ops[pm.graph.source(init)]->wT();
	//}

	//TWT
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		if (pm.graph.target(ait) == pm.tail) {
			if (pm.ops[pm.graph.source(ait)]->ID < 0) {
				for (ListDigraph::InArcIt iait(pm.graph, pm.graph.source(ait)); iait != INVALID; ++iait) {
					schedule.objective += pm.ops[pm.graph.source(iait)]->wT();
				}
			} else {
				schedule.objective += pm.ops[pm.graph.source(ait)]->wT();
			}
		}
	}


	/* Cmax
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
					if (pm.graph.target(ait) == pm.tail) {
									if (pm.ops[pm.graph.source(ait)]->ID < 0) {
													for (ListDigraph::InArcIt iait(pm.graph, pm.graph.source(ait)); iait != INVALID; ++iait) {
																	if (schedule.objective < pm.ops[pm.graph.source(iait)]->c())
																					schedule.objective = pm.ops[pm.graph.source(iait)]->c();
													}
									} else {
													if (schedule.objective < pm.ops[pm.graph.source(ait)]->c())
																	schedule.objective = pm.ops[pm.graph.source(ait)]->c();
									}
					}
	}
	 */

	//Debugger::info << "Done collecting schedule data." << ENDL;

	//out << pm << endl;
	//out << rc << endl;

	return true;
}

/** ***********************  Priority scheduler  ******************************** */

PriorityScheduler::PriorityScheduler() {
	//pm = NULL;
	//rc = NULL;
	sched = NULL;
	smallestD = 0.0;

	totalW = 0.0;
	totalD = 0.0;
}

PriorityScheduler::PriorityScheduler(PriorityScheduler& orig) : /*LS*/Scheduler(orig) {
	//pm = orig.pm;
	//rc = orig.rc;
	//sched = orig.sched;

	topolOrdering = orig.topolOrdering;
	availIDs = orig.availIDs;
	schedIDs = orig.schedIDs;

	opID2Node = pm.opID2Node();

	smallestD = orig.smallestD;

}

PriorityScheduler::~PriorityScheduler() {
	//Debugger::info << "PriorityScheduler::~PriorityScheduler" << ENDL;
}

void PriorityScheduler::preparePM() {
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

	QList<ListDigraph::Node> terminals = pm.terminals();

	// Get the topological ordering of the graph
	topolOrdering = pm.topolSort();

	// Update the ready times of the operations
	pm.updateHeads(topolOrdering);

	// Initialize the start times of the operations
	pm.updateStartTimes(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	double smallestOrderS = Math::MAX_DOUBLE;
	double smallestOrderD = Math::MAX_DOUBLE;
	//double orderTimeInt = 0.0;
	smallestD = Math::MAX_DOUBLE;
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		smallestOrderS = Math::MAX_DOUBLE;
		smallestOrderD = Math::MAX_DOUBLE;

		for (ListDigraph::InArcIt iait(pm.graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm.graph.source(iait);
			//stack.push(pm.graph.source(iait));
			q.enqueue(curpred);
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

			smallestOrderD = Math::min(smallestOrderD, ss);
			smallestOrderS = Math::min(smallestOrderS, smallestD - pm.ops[curnode]->p());

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				curpred = pm.graph.source(iait);

				// Push the current predecessor into the stack
				//stack.push(curpred);
				q.enqueue(curpred);
			}
		}

		// Get the time interval for the order. The order should be completed within this time interval
		//orderTimeInt = pm.ops[terminals[i]]->d() - smallestOrderS;

		// For the saved nodes set the due dates proportionally to their processing times
		//double p = 0.0;
		//double dop = 0.0;
		//double dord = 0.0;
		//double rop = 0.0;
		//for (int j = 0; j < ordNodes.size(); j++) {
		//	rop = pm.ops[ordNodes[j]]->r();
		//	dop = pm.ops[ordNodes[j]]->d();
		//	p = pm.ops[ordNodes[j]]->p();
		//	dord = pm.ops[terminals[i]]->d();
		//pm.ops[ordNodes[j]]->d(rop + p / (rop + dord - dop + p) * dord);
		//}
	}

	//out << pm << endl;
	//out << "smallestD = " << smallestD << endl;
	//getchar();

	// Set the due dates based on the heads
	for (int i = 0; i < topolOrdering.size(); i++) {
		if (!terminals.contains(topolOrdering[i])) {
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->r() + 3.0 * pm.ops[topolOrdering[i]]->p() - 0.0 * smallestD);
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001);
			pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001, pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 1.0 * smallestD));
		}
	}

	//out << pm << endl;
	//getchar();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		totalW += pm.ops[nit]->w();
		totalD += pm.ops[nit]->d();
	}

}

void PriorityScheduler::restorePM() {

	// Return the process model to the state before the scheduling
	//pm.clearSchedRelData();
	//QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();
}

void PriorityScheduler::init() {
	// Preserve the state of the process model
	pm.save();

	// Prepare the graph
	preparePM();

	// Clear the selection arcs
	//selectionArcs.clear();

	// Clear the nodes
	availIDs.clear();
	schedIDs.clear();

	// opID2Node
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		opID2Node[pm.ops[nit]->ID] = nit;
	}
}

void PriorityScheduler::prepareSched() {
	QTextStream out(stdout);

	sched->fromPM(pm, *obj);

	//out << "####################  PriorityScheduler::prepareSched  ############" << endl;
	//out << "FF = " << FF() << "  TWT = " << sched->objective << endl;
	//out << "###################################################################" << endl;

	//getchar();

}

Clonable* PriorityScheduler::clone() {
	return new PriorityScheduler(*this);
}

void PriorityScheduler::scheduleActions() {

	//QTextStream out(stdout);
	QTime total;
	//int totelapsed = 0;
	//QTime partial;
	//int partelapsedms = 0;

	/** Algorithm:
	 *  0. Prepare the AVAILABLE operations.
	 *  1. One loop:
	 *	1.1. Select the AVAILABLE operations. Update their ready times and the start times.
	 *	1.2. Select the operation with the highest 1/p or w/p from the AVAILABLE.
	 *	1.3. Schedule the selected operation.
	 *	1.4. Update the AVAILABLE operations excluding the SCHEDULED
	 */

	// Initialize the scheduler
	init();

	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int n = countNodes(pm.graph);
	int nsched = 0;

	// Prepare the available operations
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		nodesavail[nit] = false;
		nodesscheduled[nit] = false;
		opID2Node[pm.ops[nit]->ID] = nit;
	}
	availIDs.clear();
	schedIDs.clear();
	nodesavail[pm.head] = true;
	availIDs.insert(pm.ops[pm.head]->ID);

	total.start();

	/*
	// Update the ready times and the start times of the available BUT NOT SCHEDULED operations
	//out << "WSPTScheduler::schedule : Updating ready times for the available operations..." << endl;
	for (int i = 0; i < topolOrdering.size(); i++) {
			if (nodesavail[topolOrdering[i]]&& !nodesscheduled[topolOrdering[i]]) {
					// Update the ready time for the operation
					ListDigraph::Node curnode = topolOrdering[i];
					ListDigraph::Node prednode;
					double maxr = pm.ops[curnode]->ir();
					for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
							prednode = pm.graph.source(iait);
							if (!nodesavail[prednode] || !nodesscheduled[prednode]) {
									Debugger::err << "PriorityScheduler::schedule : Node of an available node is either not available or not scheduled!" << ENDL;
							}

							// In this case ready times are used to avoid the operations to be scheduled too early (without considering the precedence constraints)
							maxr = Math::max(maxr, pm.ops[prednode]->c()); // The ready time is defined by the completion times of the predecessors since they all are SCHEDULED
					}
					pm.ops[curnode]->r(maxr);
					pm.ops[curnode]->s(maxr); // The operation is AVAILABLE but NOT SCHEDULED => its start time will be defined in the future;
			}
	}
	 */

	// Perform the scheduling
	while (nsched < n) {

		// Select the available node with the highest priority index
		ListDigraph::Node bestnode = INVALID;
		double bestprior = Math::MIN_DOUBLE;
		double curprior = -1.0;

		foreach(const int& curID, availIDs) {
			ListDigraph::Node curnode = opID2Node[curID];

			// Update the ready time and the start time of the current node
			double maxr = 0.0;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (maxr < pm.ops[pm.graph.source(iait)]->c()) {
					maxr = pm.ops[pm.graph.source(iait)]->c();
				}
			}
			pm.ops[curnode]->r(Math::max(pm.ops[curnode]->ir(), maxr));
			pm.ops[curnode]->s(pm.ops[curnode]->r());

			// Get the priority
			curprior = priority(curnode);

			if (bestprior < curprior) {
				bestnode = curnode;
				bestprior = curprior;
			}
		}

		if (bestnode == INVALID) {
			Debugger::err << "PriorityScheduler::schedule : opID == -1!!!" << ENDL;
		}

		// Schedule the selected operation
		Machine &m = rc(pm.ops[bestnode]->toolID).earliestToFinish(pm.ops[bestnode]);

		ListDigraph::Node prevOpNode;
		if (m.operations.size() == 0) {
			prevOpNode = INVALID;
		} else {
			prevOpNode = opID2Node[m.operations.last()->ID];
		}

		m << pm.ops[bestnode];

		// Add an arc into the graph which represents the scheduling decision : The arc connects this operation and the previous operation on the machine
		if (prevOpNode != INVALID) {
			ListDigraph::Arc newArc = pm.graph.addArc(prevOpNode, bestnode);

			pm.p[newArc] = -pm.ops[prevOpNode]->p();

			// Update the topological orderings
			//partial.start();
			//pm.dynUpdateTopolSort(topolOrdering, prevOpNode, bestnode);
			//topolOrdering = pm.topolSort();
			//partelapsedms += partial.elapsed();

		}

		// Update the outgoing arcs for the best node and the ready times for the direct successors
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[bestnode]->p();

			// The heads are updated right before the node evealuation
			/*
			ListDigraph::Node curSucc = pm.graph.target(oait);
			pm.ops[curSucc]->r(0.0);

			for (ListDigraph::InArcIt iait(pm.graph, curSucc); iait != INVALID; ++iait) {
					ListDigraph::Node curSuccPred = pm.graph.source(iait);
					pm.ops[curSucc]->r(Math::max(pm.ops[curSucc]->r(), pm.ops[curSuccPred]->c()));
			}

			// IMPORTANT!!! Update the start time, since decreasing the ready time does not cause decreasing of the start time
			pm.ops[curSucc]->s(pm.ops[curSucc]->r());
			 */
		}

		// Exclude the scheduled operation
		nsched++;
		nodesscheduled[bestnode] = true;
		availIDs.remove(pm.ops[bestnode]->ID);
		schedIDs.insert(pm.ops[bestnode]->ID);

		totalW -= pm.ops[bestnode]->w();
		totalD -= pm.ops[bestnode]->d();

		// Update the set of the available operations : all successors of the scheduled operations with all predecessors scheduled
		ListDigraph::Node cursuc;
		// Check the direct successors of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			cursuc = pm.graph.target(oait);

			// Check whether all predecessors of the cursuc have been scheduled
			bool allpredsched = true;
			for (ListDigraph::InArcIt iait(pm.graph, cursuc); iait != INVALID; ++iait) {
				allpredsched = allpredsched && nodesscheduled[pm.graph.source(iait)];
			}

			// If all predecessor are scheduled then this node is newly available
			if (allpredsched) {
				nodesavail[cursuc] = true;
				availIDs.insert(pm.ops[cursuc]->ID);
			}
		}

	}

	//out << pm <<endl;
	//getchar();

	// Run the local search
	/*
	ls.maxIter(0);
	ls.checkCorectness(false);
	if (ls.maxIter() > 0) {
			pm.save();

			ls.setPM(&pm);
			ls.setResources(&rc);
			ls.run();

			pm.restore();
	}
	 */

	//totelapsed = total.elapsed();

	//out << pm << endl;
	//getchar();
	//out << "Elapsed : " << totelapsed << endl;
	//out << "Partial time percentage : " << double(partelapsedms) / double(totelapsed) << endl;

	// Save the schedule found so far
	//pm.save();

	// Prepare the schedule
	prepareSched();

	//return true;
}

double PriorityScheduler::priority(const ListDigraph::Node&) {
	Debugger::err << "PriorityScheduler::priority : Not implemented!!!" << ENDL;
	return 0.0;
}

/** ***********************  RND scheduler  ******************************** */

RNDScheduler::RNDScheduler() { }

RNDScheduler::RNDScheduler(RNDScheduler& orig) : PriorityScheduler(orig) { }

RNDScheduler::~RNDScheduler() {
	//Debugger::info << "RNDScheduler::~RNDScheduler" << ENDL;
}

double RNDScheduler::priority(const ListDigraph::Node&) {
	return Rand::rnd<double>();
}

Clonable* RNDScheduler::clone() {
	return new RNDScheduler(*this);
}

/** ***********************  FIFO scheduler  ******************************** */

WFIFOScheduler::WFIFOScheduler() {
	_weightedFIFO = true;
}

WFIFOScheduler::WFIFOScheduler(WFIFOScheduler& orig) : PriorityScheduler(orig) {
	_weightedFIFO = orig._weightedFIFO;
}

WFIFOScheduler::~WFIFOScheduler() {
	//Debugger::info << "WFIFOScheduler::~WFIFOScheduler" << ENDL;
}

Clonable* WFIFOScheduler::clone() {
	return new WFIFOScheduler(*this);
}

double WFIFOScheduler::priority(const ListDigraph::Node& node) {
	double prio = 0.0;

	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;
	if (pm.ops[node]->r() <= 0.00000001) return Math::MAX_DOUBLE;

	prio = 1.0 / pm.ops[node]->r();

	if (_weightedFIFO) {
		prio *= pm.ops[node]->w();
	}

	return prio; // The lover the index the higher the priority is
}

/** ***********************  Weight scheduler  ******************************** */

WScheduler::WScheduler() { }

WScheduler::WScheduler(WScheduler& orig) : PriorityScheduler(orig) { }

WScheduler::~WScheduler() { }

Clonable* WScheduler::clone() {
	return new WScheduler(*this);
}

double WScheduler::priority(const ListDigraph::Node& node) {
	return pm.ops[node]->w();
}

/** ***********************  Weighted tardiness scheduler  ******************************** */

WTScheduler::WTScheduler() {
	_weightedT = true;
}

WTScheduler::WTScheduler(WTScheduler& orig) : PriorityScheduler(orig) {
	_weightedT = orig._weightedT;
}

WTScheduler::~WTScheduler() { }

Clonable* WTScheduler::clone() {
	return new WTScheduler(*this);
}

double WTScheduler::priority(const ListDigraph::Node& node) {
	double res = 0.0;
	double ect = 0.0; // Expected completion time of the operation if it is scheduled as the next one

	Machine& m = rc(pm.ops[node]->toolID).earliestToFinish(pm.ops[node]);

	ect = Math::max(m.time(), pm.ops[node]->r()) + m.procTime(pm.ops[node]);

	if (ect <= 0.00001) return Math::MAX_DOUBLE;

	res = Math::max(ect - pm.ops[node]->d(), 0.0); // Tardiness of the operation

	if (_weightedT) {
		res *= pm.ops[node]->w();
	}

	return res;
}

/** ******************** WSPT scheduler ************************* **/

WSPTScheduler::WSPTScheduler() {
	weightedSPT(true);
}

WSPTScheduler::WSPTScheduler(WSPTScheduler& orig) : PriorityScheduler(orig) {
	weightedSPT(orig._weightedSPT);
}

WSPTScheduler::~WSPTScheduler() {
	//Debugger::info << "WSPTScheduler::~WSPTScheduler" << ENDL;
}

Clonable* WSPTScheduler::clone() {
	return new WSPTScheduler(*this);
}

double WSPTScheduler::priority(const ListDigraph::Node& node) {
	double curprior = 0.0;

	if (pm.ops[node]->ID >= 0) {
		curprior = 1.0 / rc(pm.ops[node]->toolID).expectedProcTime(pm.ops[node]);
		if (_weightedSPT) {
			curprior *= pm.ops[node]->w();
		}
	} else {
		curprior = Math::MAX_DOUBLE;
	}

	return curprior;
}

/** ******************** EOD scheduler ************************* **/

EODScheduler::EODScheduler() { }

EODScheduler::EODScheduler(EODScheduler& orig) : PriorityScheduler(orig) { }

EODScheduler::~EODScheduler() {
	//Debugger::info << "EODScheduler::~EODScheduler" << ENDL;
}

Clonable* EODScheduler::clone() {
	return new EODScheduler(*this);
}

void EODScheduler::preparePM() {
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

	QList<ListDigraph::Node> terminals = pm.terminals();

	// Get the topological ordering of the graph
	topolOrdering = pm.topolSort();

	// Update the ready times of the operations
	pm.updateHeads(topolOrdering);

	// Initialize the start times of the operations
	pm.updateStartTimes(topolOrdering);

	// Set local due dates for all operations based on the due dates of the terminal nodes
	double smallestOrderS = Math::MAX_DOUBLE;
	double smallestOrderD = Math::MAX_DOUBLE;
	//double orderTimeInt = 0.0;
	smallestD = Math::MAX_DOUBLE;
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

	for (int i = 0; i < terminals.size(); i++) {
		//QStack<ListDigraph::Node> stack;
		QQueue<ListDigraph::Node> q;
		ListDigraph::Node curnode = INVALID;
		ListDigraph::Node curpred = INVALID;
		ListDigraph::Node cursucc = INVALID;

		smallestOrderS = Math::MAX_DOUBLE;
		smallestOrderD = Math::MAX_DOUBLE;

		for (ListDigraph::InArcIt iait(pm.graph, terminals[i]); iait != INVALID; ++iait) {
			curpred = pm.graph.source(iait);
			//stack.push(pm.graph.source(iait));
			q.enqueue(curpred);
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

			smallestOrderD = Math::min(smallestOrderD, ss);
			smallestOrderS = Math::min(smallestOrderS, smallestD - pm.ops[curnode]->p());

			// Consider the direct predecessors
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				curpred = pm.graph.source(iait);

				// Push the current predecessor into the stack
				//stack.push(curpred);
				q.enqueue(curpred);
			}
		}

		// Get the time interval for the order. The order should be completed within this time interval
		//orderTimeInt = pm.ops[terminals[i]]->d() - smallestOrderS;

		// For the saved nodes set the due dates proportionally to their processing times
		/*
		double p = 0.0;
		double dop = 0.0;
		double dord = 0.0;
		double rop = 0.0;
		for (int j = 0; j < ordNodes.size(); j++) {
				rop = pm.ops[ordNodes[j]]->r();
				dop = pm.ops[ordNodes[j]]->d();
				p = pm.ops[ordNodes[j]]->p();
				dord = pm.ops[terminals[i]]->d();
				//pm.ops[ordNodes[j]]->d(rop + p / (rop + dord - dop + p) * dord);
		}
		 */
	}

	//out << pm << endl;
	out << "smallestD = " << smallestD << endl;
	//getchar();

	// Set the due dates based on the heads
	for (int i = 0; i < topolOrdering.size(); i++) {
		if (!terminals.contains(topolOrdering[i])) {
			//pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->r() + 3.0 * pm.ops[topolOrdering[i]]->p() - 0.0 * smallestD);
			pm.ops[topolOrdering[i]]->d(pm.ops[topolOrdering[i]]->d() - 1.0 * Math::min(0.0, smallestD) + 0.00000001);
			//pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001, pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 1.0 * smallestD));
		}
	}

	//out << pm << endl;
	//getchar();

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		totalW += pm.ops[nit]->w();
		totalD += pm.ops[nit]->d();
	}

}

double EODScheduler::priority(const ListDigraph::Node& node) {
	//Debugger::info << "WSDDScheduler::priority..." << ENDL;

	double curprior = 0.0;

	if (pm.ops[node]->p() <= 0.000001) return Math::MAX_DOUBLE;


	if (pm.ops[node]->ID >= 0) {

		curprior = 1.0 / pm.ops[node]->d();

	} else {
		curprior = Math::MAX_DOUBLE;
	}

	return curprior;
}

/** ******************** WODD scheduler ************************* **/

WEODScheduler::WEODScheduler() {
	weightedEOD(true);
}

WEODScheduler::WEODScheduler(WEODScheduler& orig) : PriorityScheduler(orig) {
	weightedEOD(orig._weightedEOD);
}

WEODScheduler::~WEODScheduler() {
	//Debugger::info << "WEODScheduler::~WEODScheduler" << ENDL;
}

Clonable* WEODScheduler::clone() {
	return new WEODScheduler(*this);
}

double WEODScheduler::priority(const ListDigraph::Node& node) {
	//Debugger::info << "WSDDScheduler::priority..." << ENDL;

	double curprior = 0.0;

	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;


	if (pm.ops[node]->ID >= 0) {

		if (pm.ops[node]->p() > 0.0) {
			curprior = 1.0 / pm.ops[node]->d(); //*/ 1.0 / ((pm.ops[node]->r() + pm.ops[node]->p() + pm.ops[node]->d()) / 2.0); // The node with the smallest d will have the highest priority

		} else {
			return Math::MAX_DOUBLE;
		}
		//}

		if (_weightedEOD) {
			curprior *= pm.ops[node]->w();
		}
	} else {
		curprior = Math::MAX_DOUBLE;
	}

	return curprior;
}

/** ************************** WEDD scheduler ****************************** **/

WEDDScheduler::WEDDScheduler() {
	weightedEDD(true);
}

WEDDScheduler::WEDDScheduler(WEDDScheduler& orig) : PriorityScheduler(orig) {
	weightedEDD(orig._weightedEDD);
}

WEDDScheduler::~WEDDScheduler() {
	//Debugger::info << "WEDDScheduler::~WEDDScheduler" << ENDL;
}

Clonable* WEDDScheduler::clone() {
	return new WEDDScheduler(*this);
}

void WEDDScheduler::preparePM() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// For each operation, set its due date to the due date of the corresponding order
	QList<ListDigraph::Node> terminals = pm.terminals();
	QHash<int, double> ordID2OrdD; // Due dates of the orders

	// Get the due dates of the orders
	for (int i = 0; i < terminals.size(); i++) {
		ListDigraph::Node curNode = terminals[i];
		int curOrdID = pm.ops[curNode]->OID;
		double curD = pm.ops[curNode]->d();

		ordID2OrdD[curOrdID] = curD;

	}

	// Iterate over all graph nodes setting the due dates
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		Operation& curOp = (Operation&) * pm.ops[nit];
		int curOrdID = curOp.OID;
		double curD = ordID2OrdD[curOrdID];

		// Set the due date of the operation to be equal to the due date of its job
		curOp.d(curD);
	}

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		totalW += pm.ops[nit]->w();
		totalD += pm.ops[nit]->d();
	}
}

double WEDDScheduler::priority(const ListDigraph::Node& node) {
	//Debugger::info << "WSDDScheduler::priority..." << ENDL;

	double curprior = 0.0;

	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;

	if (pm.ops[node]->ID >= 0) {

		if (pm.ops[node]->p() > 0.0) {

			curprior = 1.0 / pm.ops[node]->d(); // The node with the smallest d will have the highest priority

		} else {

			return Math::MAX_DOUBLE;

		}

		if (_weightedEDD) {

			curprior *= pm.ops[node]->w();

		}

	} else {

		curprior = Math::MAX_DOUBLE;

	}

	return curprior;
}

/** ************************************************************************ **/

/** ******************** WEDD2 scheduler ************************* **/

WEDD2Scheduler::WEDD2Scheduler() {
	weightedEDD(true);
}

WEDD2Scheduler::WEDD2Scheduler(WEDD2Scheduler& orig) : PriorityScheduler(orig) {
	weightedEDD(orig._weightedEDD);
}

WEDD2Scheduler::~WEDD2Scheduler() {
	//Debugger::info << "WEDD2Scheduler::~WEDD2Scheduler" << ENDL;
}

Clonable* WEDD2Scheduler::clone() {
	return new WEDD2Scheduler(*this);
}

double WEDD2Scheduler::priority(const ListDigraph::Node& node) {
	//Debugger::info << "WSDDScheduler::priority..." << ENDL;

	double curprior = 0.0;

	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;


	if (pm.ops[node]->ID >= 0) {

		if (pm.ops[node]->p() > 0.0) {
			curprior = 1.0 / pm.ops[node]->d(); //*/ 1.0 / ((pm.ops[node]->r() + pm.ops[node]->p() + pm.ops[node]->d()) / 2.0); // The node with the smallest d will have the highest priority

		} else {
			return Math::MAX_DOUBLE;
		}
		//}

		if (_weightedEDD) {
			curprior *= Math::pow(pm.ops[node]->w(), (Math::intUNI) 2); //pm.ops[node]->w();
		}
	} else {
		curprior = Math::MAX_DOUBLE;
	}

	return curprior;
}

/** *************************** WMOD scheduler ***************************** **/

WMODScheduler::WMODScheduler() {
	weightedMOD(true);
}

WMODScheduler::WMODScheduler(WMODScheduler& orig) : PriorityScheduler(orig) {
	weightedMOD(orig._weightedMOD);
}

WMODScheduler::~WMODScheduler() {
	//Debugger::info << "WMODScheduler::~WMODScheduler" << ENDL;
}

Clonable* WMODScheduler::clone() {
	return new WMODScheduler(*this);
}

double WMODScheduler::priority(const ListDigraph::Node& node) {
	//Debugger::info << "WSDDScheduler::priority..." << ENDL;

	double curprior = 0.0;

	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;

	if (pm.ops[node]->ID >= 0) {

		Machine& m = rc(pm.ops[node]->toolID).earliestToFinish(pm.ops[node]);

		double t = m.time();

		double p = /*pm.ops[node]->p(); //*/m.procTime(pm.ops[node]);

		if (pm.ops[node]->p() > 0.0) {
			curprior = 1.0 / Math::max(pm.ops[node]->d(), Math::max(pm.ops[node]->r(), 0.0 * t) + p); // The node with the smallest d will have the highest priority
		} else {
			return Math::MAX_DOUBLE;
		}

		if (_weightedMOD) {
			curprior *= pm.ops[node]->w();
		}
	} else {
		curprior = Math::MAX_DOUBLE;
	}

	return curprior;
}

/** ************************************************************************ **/

/** *************************** WMOD scheduler ***************************** **/

WMDDScheduler::WMDDScheduler() : dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	weightedMDD(true);
}

WMDDScheduler::WMDDScheduler(WMDDScheduler& orig) : PriorityScheduler(orig), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	weightedMDD(orig._weightedMDD);
}

WMDDScheduler::~WMDDScheduler() {
	//Debugger::info << "WMDDScheduler::~WMDDScheduler" << ENDL;
}

Clonable* WMDDScheduler::clone() {
	return new WMDDScheduler(*this);
}

void WMDDScheduler::preparePM() {
	QTextStream out(stdout);
	//out << *pm << endl;
	//getchar();

	// For each operation, set its due date to the due date of the corresponding order
	QList<ListDigraph::Node> terminals = pm.terminals();
	QHash<int, double> ordID2OrdD; // Due dates of the orders

	// Get the due dates of the orders
	for (int i = 0; i < terminals.size(); i++) {
		ListDigraph::Node curNode = terminals[i];
		int curOrdID = pm.ops[curNode]->OID;
		double curD = pm.ops[curNode]->d();

		ordID2OrdD[curOrdID] = curD;

	}

	// Iterate over all graph nodes setting the due dates
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		Operation& curOp = (Operation&) * pm.ops[nit];
		int curOrdID = curOp.OID;
		double curD = ordID2OrdD[curOrdID];

		// Set the due date of the operation to be equal to the due date of its job
		curOp.d(curD);
	}

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		totalW += pm.ops[nit]->w();
		totalD += pm.ops[nit]->d();
	}

	// Imported from ATC in purpose to calculate the remaining processing times for all operations

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
	smallestD = Math::MAX_DOUBLE;
	QList<ListDigraph::Node> ordNodes; // Nodes in the current order

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
			//            pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.000000001, pm.ops[topolOrdering[i]]->r() + pm.ops[topolOrdering[i]]->p()));
			//pm.ops[topolOrdering[i]]->d(Math::max(pm.ops[topolOrdering[i]]->d() - 1.0 * smallestD + 0.00000001, pm.ops[topolOrdering[i]]->r() + 1.0 * pm.ops[topolOrdering[i]]->p() - 1.0 * smallestD));
			//d[topolOrdering[i]] -= 1.0 * smallestD;
		}
	}

	// Initialize the start times of the operations
	pm.updateStartTimes(topolOrdering);
}

double WMDDScheduler::priority(const ListDigraph::Node& node) {
	//Debugger::info << "WSDDScheduler::priority..." << ENDL;

	double curprior = 0.0;

	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;

	if (pm.ops[node]->ID >= 0) {

		double t = rc(pm.ops[node]->toolID).earliestToFinish(pm.ops[node]).time();

		if (pm.ops[node]->p() > 0.0) {

			//curprior = 1.0 / Math::max(pm.ops[node]->d(), Math::max(pm.ops[node]->r(), t) + pm.ops[node]->p()); // The node with the smallest d will have the highest priority
			//curprior = 1.0 / Math::max(pm.ops[node]->d() - Math::max(pm.ops[node]->r(), t), pm.ops[node]->p()); // The node with the smallest d will have the highest priority
			curprior = 1.0 / Math::max(pm.ops[node]->d(), Math::max(pm.ops[node]->r(), t) + pRemain[node]);

		} else {

			return Math::MAX_DOUBLE;

		}

		if (_weightedMDD) {

			curprior *= pm.ops[node]->w();

		}

	} else {

		curprior = Math::MAX_DOUBLE;

	}

	return curprior;
}

/** ************************************************************************ **/

/** *********************  Machine utilization scheduler  ******************* */

MUBScheduler::MUBScheduler() { }

MUBScheduler::MUBScheduler(MUBScheduler& orig) : PriorityScheduler(orig) { }

MUBScheduler::~MUBScheduler() { }

Clonable* MUBScheduler::clone() {
	return new MUBScheduler(*this);
}

double MUBScheduler::priority(const ListDigraph::Node& node) {
	// The lower utilization the tool group for this operation has the higher is the priority
	double tgutil = rc(pm.ops[node]->toolID).avgUtil();

	if (tgutil == 0.0) return Math::MAX_DOUBLE;

	return 1.0 / tgutil;
}

/** ***********************  Machine time balancing scheduler  ******************************** */

MTBScheduler::MTBScheduler() { }

MTBScheduler::MTBScheduler(MTBScheduler& orig) : PriorityScheduler(orig) { }

MTBScheduler::~MTBScheduler() { }

Clonable* MTBScheduler::clone() {
	return new MTBScheduler(*this);
}

double MTBScheduler::priority(const ListDigraph::Node& node) {
	// The earlier the tool group for this operation can be available the higher is the priority
	double earliesavail = rc(pm.ops[node]->toolID).nextAvailable().time();

	if (earliesavail == 0.0) return Math::MAX_DOUBLE;

	return 1.0 /*pm.ops[node]->w()*/ / earliesavail;
}

/** ******************** ATC scheduler ************************* **/

ATCANScheduler::ATCANScheduler() : cs(NULL), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	kappaOptim(true);
	considerSucc(true);

	kappa = 0.3;

	// Setup the combined scheduler

	cs = new CombinedScheduler;

	// IMPORTANT!!! The objective function for the schedulers is not defined here -> will be set later externally

	Scheduler* sch = 0;

	sch = new WFIFOScheduler;
	sch->ID = 1;
	//	sch->obj = obj->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(false);
	*cs << sch;
	delete sch;

	sch = new WFIFOScheduler;
	sch->ID = 2;
	//	sch->obj = obj->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(true);
	*cs << sch;
	delete sch;

	sch = new WEODScheduler;
	sch->ID = 3;
	//	sch->obj = obj->clone();
	((WEODScheduler*) sch)->weightedEOD(false);
	*cs << sch;
	delete sch;

	sch = new WEODScheduler;
	sch->ID = 4;
	//	sch->obj = obj->clone();
	((WEODScheduler*) sch)->weightedEOD(true);
	*cs << sch;
	delete sch;

	sch = new WMDDScheduler;
	sch->ID = 5;
	//	sch->obj = obj->clone();
	((WMDDScheduler*) sch)->weightedMDD(false);
	*cs << sch;
	delete sch;

	sch = new WMDDScheduler;
	sch->ID = 6;
	//	sch->obj = obj->clone();
	((WMDDScheduler*) sch)->weightedMDD(true);
	*cs << sch;
	delete sch;

	sch = NULL;

	cs->options = options;
	cs->options["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

}

ATCANScheduler::ATCANScheduler(ATCANScheduler& orig) : PriorityScheduler(orig), cs(NULL), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	kappaOptim(orig.kappaoptim);
	considerSucc(orig.considersucc);

	kappa = orig.kappa;

	cs = new CombinedScheduler(*orig.cs); // Clone the combined scheduler
}

ATCANScheduler::~ATCANScheduler() {
	//Debugger::info << "ATCANScheduler::~ATCANScheduler : Started" << ENDL;

	// Delete the CS object
	if (cs != NULL) {
		delete cs;
		cs = NULL;
	}

	//Debugger::info << "ATCANScheduler::~ATCANScheduler : Finished" << ENDL;
}

Clonable* ATCANScheduler::clone() {
	return new ATCANScheduler(*this);
}

void ATCANScheduler::preparePM() {
	//PriorityScheduler::preparePM();
	//return;

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
	smallestD = Math::MAX_DOUBLE;
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

}

double ATCANScheduler::priority(const ListDigraph::Node & node) {
	//Debugger::err << "Priority ATC" << ENDL;

	double curprior = 0.0;

	//if (pm.ops[node]->ID >= 0) {

	curprior = I(kappaPAvg, kappaRPAvg, node);

	//} else {
	//    curprior = Math::MAX_DOUBLE;
	//}

	return curprior;
}

double ATCANScheduler::I(const double &kappapavg, const double& /*kappaRpAvg*/, const ListDigraph::Node & node) {
	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;

	Machine &m = rc(pm.ops[node]->toolID)./*nextAvailable(pm.ops[node]->type); //*/earliestToFinish(pm.ops[node]);
	double t = m.time(); // Time the machine becomes available
	//double h = (pRemain[node] > 0.0) ? (dOrd[node] - smallestD - Math::max(pm.ops[node]->r(), t)) / pRemain[node] : 0.0;
	double p = m.procTime(pm.ops[node]);
	double r = pm.ops[node]->r();
	double w = pm.ops[node]->w();

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

	//ff = 2.0;

	double d = dOrd[node] + p /*ff * pm.ops[node]->p()*/ - ff * (pRemain[node]);

	/*
	cout << " Order " << pm.ops[node]->OID << endl;
	cout << " dOrd = " << dOrd[node] << endl;
	cout << " d : " << d << endl;
	cout << " p : " << p << endl;
	cout << " pRemain : " << pRemain[node] << endl;
	cout << " ff : " << ff << endl;
	cout << " ff*pRemain : " << ff * pRemain[node] << endl;
	 */

	d = Math::max(d, Math::max(r, t) + p);

	//cout << " dmod : " << d << endl;
	//getchar();

	double slack = (d - p) + /*r - Math::max(r,t);//*/ + Math::max(pm.ops[node]->ir() - t, 0.0); // / ff;

	double exppow = -(Math::max(slack, 0.0)) / kappapavg; //kpavg;
	double exppowr = -(Math::max(/*(1.0 + ff) **/ r - t, 0.0)) / kappaRPAvg; //kR_pAvg; //*/kappapavg;

	/*
	Debugger::info << "Operation : " << pm.ops[node]->ID << ENDL;
	Debugger::info << "Priority : " << pm.ops[node]->w() / p * Math::exp(exppow) << ENDL;
	Debugger::info << "k = " << kappa << ENDL;
	Debugger::info << "pavg = " << p_avg << ENDL;
	Debugger::info << "kpavg = " << kappapavg << ENDL;
	Debugger::info << "slack = " << slack << ENDL;
	Debugger::info << "pow = " << exppow << " e^pow = " << Math::exp(exppow) << ENDL;
	
	getchar();
	 */

	//p = pRemain[node];

	//if (r <= 0.00000001) return pm.ops[node]->w() / p;

	return w / p * Math::exp(exppow) * Math::exp(exppowr);

}

void ATCANScheduler::scheduleActions() {
	QTextStream out(stdout);

	// Best found PM
	ProcessModel bestPM;

	// Preserve the state of the resources
	Resources rcInit = rc;

	/*
	
	CombinedScheduler cs;

	cs.pm = pm;
	cs.rc = rc;
	cs.sched = sched;

	Scheduler* sch = 0;

	sch = new WFIFOScheduler;
	sch->ID = 1;
	sch->obj = obj->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(false);
	cs << sch;
	delete sch;

	sch = new WFIFOScheduler;
	sch->ID = 2;
	sch->obj = obj->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(true);
	cs << sch;
	delete sch;

	sch = new WEODScheduler;
	sch->ID = 3;
	sch->obj = obj->clone();
	((WEODScheduler*) sch)->weightedEOD(false);
	cs << sch;
	delete sch;

	sch = new WEODScheduler;
	sch->ID = 4;
	sch->obj = obj->clone();
	((WEODScheduler*) sch)->weightedEOD(true);
	cs << sch;
	delete sch;

	sch = new WMDDScheduler;
	sch->ID = 5;
	sch->obj = obj->clone();
	((WMDDScheduler*) sch)->weightedMDD(false);
	cs << sch;
	delete sch;

	sch = new WMDDScheduler;
	sch->ID = 6;
	sch->obj = obj->clone();
	((WMDDScheduler*) sch)->weightedMDD(true);
	cs << sch;
	delete sch;

	sch = 0;

	cs.options = options;
	cs.options["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

	cs.schedule();

	 */

	cs->pm = pm;
	cs->rc = rc;
	cs->sched = sched;

	cs->options = options;
	cs->options["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

	cs->schedule();

	out << "Objective of the CombinedScheduler : " << sched->objective << endl;

	// Get an estimation of the flow factor
	ff = cs->lastBestScheduler()->flowFactor();

	out << "Flow factor estimation : " << ff << endl;
	//getchar();

	// Here kappa optimization is performed

	if (!kappaoptim) {
		//PriorityScheduler::scheduleActions();
		schedAct();
	} else {

		QList<double> kappaGrid;
		QList<double> kappaGridBckp;
		QList<double> kappaRGrid;
		QList<double> FFGrid;

		//kappaGrid << 0.01 << 0.011 << 0.012 << 0.013 << 0.014 << 0.015 << 0.016 << 0.017 << 0.018 << 0.019 << 0.02 << 0.021 << 0.022 << 0.023 << 0.024 << 0.025 << 0.026 << 0.027 << 0.028 << 0.029 << 0.03 << 0.04 << 0.05 << 0.05 << 0.06 << 0.07 << 0.08 << 0.09 << 0.1 << 0.25 << 0.5 << 0.75 << 1.0 << 1.25 << 1.5 << 1.75 << 2.0 << 2.5 << 3.0 << 3.5 << 4.0 << 4.5 << 5.0 << 5.5 << 6.0 << 6.5 << 10000000000000.0; // << 7.0 << 7.5 << 8.0 << 8.5 << 9.0 << 10.0 << 15.0 << 20.0 << 50.0 << 100.0 << 500.0 << 1000.0; //<< 0.01 << 0.1 << 0.2 << 0.6 << 0.8 << 1.0 << 1.2 << 1.4 << 1.6 << 1.8 << 2.0 << 2.4 << 2.8 << 3.2 << 3.6 << 4.0 << 4.4 << 4.8 << 5.2;

		kappaGrid << 0.01 << 0.011 << 0.012 << 0.013 << 0.014 << 0.015 << 0.016 << 0.017 << 0.018 << 0.019 << 0.02 << 0.021 << 0.022 << 0.023 << 0.024 << 0.025 << 0.026 << 0.027 << 0.028 << 0.029 << 0.03 << 0.04 << 0.05 << 0.05 << 0.06 << 0.07 << 0.08 << 0.09 << 0.1 << 0.25 << 0.5 << 1.0 << 1.5 << 2.0 << 2.5 << 3.0 << 3.5 << 4.0 << 4.5 << 5.0 << 5.5 << 6.0; // << 7.0 << 7.5 << 8.0 << 8.5 << 9.0 << 10.0 << 15.0 << 20.0 << 50.0 << 100.0 << 500.0 << 1000.0; //<< 0.01 << 0.1 << 0.2 << 0.6 << 0.8 << 1.0 << 1.2 << 1.4 << 1.6 << 1.8 << 2.0 << 2.4 << 2.8 << 3.2 << 3.6 << 4.0 << 4.4 << 4.8 << 5.2;


		kappaGridBckp = kappaGrid;

		kappaRGrid << 0.001 << 0.01 << 0.04 << 0.07 << 0.125 << 0.2 << 0.25 << 0.4 << 0.8 << 1.2 << 100000000.0; //<< 0.001 << 0.0025 << 0.004 << 0.005 << 0.025 << 0.04 << 0.05 << 0.25 << 0.4 << 0.6 << 0.8 << 1.0 << 1.2 << 100000000.0; // << 1.4 << 1.6 << 1.8 << 2.0 << 3.0 << 4.0 << 5.0 << 7.0 << 10.0;
		FFGrid << 1.0; // << 1.5 << 2.0 << 2.5 << 3.0 << 4.0 << 5.0; //<< -2.0 << -1.0 << -0.5 << 0.0 << 0.5 << 0.75 << 1.0 << 1.5 << 2.0 << 2.5 << 3.0;

		//int totalParComb = kappaGrid.size() * kappaRGrid.size() * FFGrid.size();
		int curParCombCtr = 1;

		Schedule bestsched;

		bestsched.init();
		bestsched.objective = Math::MAX_DOUBLE;

		double bestFF = ff;
		double bestKappa = Math::MIN_DOUBLE;
		double bestKappaR = Math::MIN_DOUBLE;

		for (int iFF = 0; iFF < FFGrid.size(); iFF++) {
			//ff = FFGrid[iFF];
			//out << "ATCANScheduler::scheduleActions : FF = " << FF <<endl;

			for (int iKR = 0; iKR < kappaRGrid.size(); iKR++) {
				kappaR = kappaRGrid[iKR];

				kappaGrid = kappaGridBckp;

				for (int iK = 0; iK < kappaGrid.size(); iK++) {
					kappa = kappaGrid[iK];



					// Perform scheduling for the current parameter combination

					// Delete the scheduling relevant data from the process model
					pm.clearSchedRelData();

					// Restore the state of the resources
					rc = rcInit;

					// Perform scheduling
					schedAct();

					if (bestsched.objective > sched->objective) {
						bestsched = *sched;

						bestKappa = kappa;
						bestKappaR = kappaR;
						bestFF = flowFactor(); //FF;
						//ff = bestFF;

						out << "ATCANScheduler::scheduleActions : Found a better TWT : " << bestsched.objective << endl;
						out << "ATCANScheduler::scheduleActions : Found a better TWT : kappa : " << bestKappa << endl;
						out << "ATCANScheduler::scheduleActions : Found a better TWT : kappaR : " << bestKappaR << endl;

						bestPM = pm; // Save the best PM

						//TWT twt;
						//	out << "Recalculated TWT : " << TWT()(bestPM) << endl;
						//getchar();

						// Create an additional grid

						/*
						double offset = 0.0001;

						QList<double> newKappaGrid;
						for (int i = iK; i < Math::min(iK + 50, kappaGrid.size()); i++) {
							newKappaGrid.append(kappaGrid[iK]);
						}

						while (offset < 0.01) {

							if (kappa - offset > 0) newKappaGrid << kappa - offset;
							newKappaGrid << kappa + offset;

							offset += 0.0001;
						}
						kappaGrid = newKappaGrid;
						iK = -1;
						iKR = -1;
						 */


					}

					//out << "ATC completed combinations : " << QString::number(double(curParCombCtr) / double(totalParComb) *100.0, 'f', 1) << " % " << endl;
					curParCombCtr++;

				}

			}

		}

		//TWT twt;
		//out << "Recalculated TWT of the best PM : " << twt(bestPM) << endl;

		// Store the best solution
		pm = bestPM;
		*sched = bestsched;

		// Restore the best parameters
		kappa = bestKappa;
		kappaR = bestKappaR;
		ff = bestFF;

		/* Commented out because of the mismatch between the TWT value returned by the ATC-rule and the initial TWT for the LS : 
		 * probably, the commented recalculation provides other solution.
         
		  sched = bestsched;

		  out << "Best FF = " << bestFF << endl;
		  out << "Best kappa = " << bestKappa << endl;
		  out << "Best kappaR = " << bestKappaR << endl;

		  // Run the scheduler with the best found parameter values
		  kappa = bestKappa;
		  kappaR = bestKappaR;
		  ff = bestFF;

		  // Delete the scheduling relevant data from the process model
		  pm.clearSchedRelData();
                
		  // Restore the state of the resources
		  rc = rcInit;

		  // Perform scheduling
		  schedAct();

		  // Prepare the schedule
		  //prepareSched();

		  if (bestsched.objective < sched->objective) {
		 *sched = bestsched;
		  }

		  //out << "Objective of the best schedule : " << bestsched.objective << endl;
		  //out << "Objective of the current schedule : " << sched->objective << endl;
         
		 */

	}

}

void ATCANScheduler::schedAct() {

	QTextStream out(stdout);
	QTime total;
	//int totelapsed = 0;
	//QTime partial;
	//int partelapsedms = 0;

	/** Algorithm:
	 *  0. Prepare the AVAILABLE operations.
	 *  1. One loop:
	 *	1.1. Select the AVAILABLE operations. Update their ready times and the start times.
	 *	1.2. Select the operation with the highest 1/p or w/p from the AVAILABLE.
	 *	1.3. Schedule the selected operation.
	 *	1.4. Update the AVAILABLE operations excluding the SCHEDULED
	 */

	// Initialize the scheduler
	init();

	//out << pm << endl;
	//getchar();

	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//out << lenRemain[nit] << endl;
	//}
	//getchar();

	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int n = countNodes(pm.graph);
	int nsched = 0;

	// Prepare the available operations
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		nodesavail[nit] = false;
		nodesscheduled[nit] = false;
		//opID2Node[pm.ops[nit]->ID] = nit;
	}
	availIDs.clear();
	schedIDs.clear();
	nodesavail[pm.head] = true;
	availIDs.insert(pm.ops[pm.head]->ID);

	totalPRemain = 0.0;
	for (ListDigraph::OutArcIt oait(pm.graph, pm.head); oait != INVALID; ++oait) {
		totalPRemain += pRemain[pm.graph.target(oait)];
	}

	total.start();

	// Perform the scheduling
	while (nsched < n) {

		// Select the available node with the highest priority index
		ListDigraph::Node bestnode = INVALID;
		double bestprior = Math::MIN_DOUBLE;
		double curprior = -1.0;

		//out << topolOrdering.size() << endl;

		QHash<int, double> toolID2pAvg; // Average processing time of the available operations of the tool 
		QHash<int, int> toolID2ctr; // Counter of the operations for the tool group

		toolID2pAvg.clear();
		toolID2ctr.clear();

		// Initialize
		//QList<int> availIDsList = availIDs.toList();
		//Math::sort(availIDsList);
		//int curID = 0;

		//        foreach(const int& curID, availIDs) {
		//for (int i = 0; i < availIDsList.size(); i++) {
		//curID = availIDsList[i];

		//            toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] = 0.0;
		//            toolID2ctr[pm.ops[opID2Node[curID]]->toolID] = 0;

		//        }
		//}

		//out << endl << endl;
		//for (int i = 0; i < availIDsList.size(); i++) {
		//out << availIDsList[i] << " : " << *pm.ops[opID2Node[availIDsList[i]]] << endl;
		//out << toolID2pAvg[pm.ops[opID2Node[curids[i]]]->toolID] << endl;
		//}

		// For the available operations, calculate the average remaining processing times of the jobs waiting in the queue of each tool group

		p_avg = 0.0;
		int pAvgCtr = 0;

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			//            toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += pRemain[opID2Node[curID]]; // Remaining processing time of the job starting from the current operation
			//            toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;

			p_avg += pRemain[opID2Node[curID]];
			pAvgCtr++;

			//out << toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] << endl;
		}

		if (pAvgCtr > 0) p_avg /= double (pAvgCtr);

		//if (pAvgCtr > 0) p_avg = totalPRemain / 70.0;

		kappaPAvg = kappa * p_avg;
		kappaRPAvg = kappaR * p_avg;

		/*
		// Calculate the sums       
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
				if (nodesavail[nit] && !nodesscheduled[nit]) {
						toolID2pAvg[pm.ops[nit]->toolID] += pm.ops[nit]->p();
						toolID2ctr[pm.ops[nit]->toolID] += 1;
				}
		}
		 */

		//foreach(const int& curID, availIDs) {
		//    toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += pm.ops[opID2Node[curID]]->p();
		//    toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;
		//}

		//        for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
		//            if (toolID2ctr[iter.key()] > 0) {
		//                toolID2pAvg[iter.key()] /= double(toolID2ctr[iter.key()]);
		//            }
		//        }

		// Find the machine group with the highest utilization
		//        double curUtil = -1.0;
		//        int curToolID = -1;
		//        for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
		//            if (rc(iter.key()).avgUtil() > curUtil) {
		//                curToolID = iter.key();
		//                curUtil = rc(iter.key()).avgUtil();
		//            }
		//        }
		//        if (curToolID == -1) {
		//            Debugger::err << "ATCANScheduler::schedAct : Failed to find a tool group with the highest utilization!" << ENDL;
		//        } else {
		//            //Debugger::info << curUtil << ENDL;
		//        }

		//Debugger::info << "Currently available operations : " << ENDL;

		//foreach(const int& curID, availIDs) {
		//    Debugger::info << curID << ENDL;
		//}

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			ListDigraph::Node curnode = opID2Node[curID];

			// Update the ready time and the start time of the current node
			double maxr = 0.0;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (maxr < pm.ops[pm.graph.source(iait)]->c()) {
					maxr = pm.ops[pm.graph.source(iait)]->c();
				}
			}
			pm.ops[curnode]->r(Math::max(pm.ops[curnode]->ir(), maxr));
			pm.ops[curnode]->s(pm.ops[curnode]->r());

			// Set p_avg
			//p_avg = toolID2pAvg[pm.ops[curnode]->toolID];

			//            kappaPAvg = kappa * p_avg;
			//            kappaRPAvg = kappaR * p_avg;

			// Get the priority
			curprior = priority(curnode);

			// Consider only the currently selected machine group
			//if (pm.ops[curnode]->toolID == curToolID) {
			//if (curUtil > 0.9) {
			if (pm.ops[curnode]->toolID == 8 || pm.ops[curnode]->toolID == 9 || pm.ops[curnode]->toolID == 10) {
				//curprior *= 100000000000.0;
			}
			//out << curToolID << endl;
			//}
			//}

			//out << "Priority for " << availIDsList[i] << " is " << curprior << endl;
			//out << "p_avg is " << p_avg << endl;
			//out << "dOrd is " << dOrd[curnode] << endl;

			if (bestprior < curprior) {
				bestnode = curnode;
				bestprior = curprior;
			}

		}

		if (bestnode == INVALID) {
			Debugger::err << "PriorityScheduler::schedule : opID == -1!!!" << ENDL;
		}

		// Schedule the selected operation
		Machine &m = rc(pm.ops[bestnode]->toolID)./*nextAvailable(pm.ops[bestnode]->type); //*/earliestToFinish(pm.ops[bestnode]);

		ListDigraph::Node prevOpNode;
		if (m.operations.size() == 0) {
			prevOpNode = INVALID;
		} else {
			prevOpNode = opID2Node[m.operations.last()->ID];
		}

		m << pm.ops[bestnode];

		// Add an arc into the graph which represents the scheduling decision : The arc connects this operation and the previous operation on the machine
		if (prevOpNode != INVALID) {
			ListDigraph::Arc newArc = pm.graph.addArc(prevOpNode, bestnode);
			//selectionArcs.append(newArc);
			pm.p[newArc] = -pm.ops[prevOpNode]->p();

		}
		// Update the outgoing arcs for the best node and the ready times for the direct successors
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[bestnode]->p();

			/*
			ListDigraph::Node curSucc = pm.graph.target(oait);
			pm.ops[curSucc]->r(0.0);

			for (ListDigraph::InArcIt iait(pm.graph, curSucc); iait != INVALID; ++iait) {
					ListDigraph::Node curSuccPred = pm.graph.source(iait);
					pm.ops[curSucc]->r(Math::max(pm.ops[curSucc]->r(), pm.ops[curSuccPred]->c()));
			}

			// IMPORTANT!!! Update the start time, since decreasing the ready time does not cause decreasing of the start time
			pm.ops[curSucc]->s(pm.ops[curSucc]->r());
			 */
		}

		// Exclude the scheduled operation
		nsched++;
		nodesscheduled[bestnode] = true;
		availIDs.remove(pm.ops[bestnode]->ID);
		schedIDs.insert(pm.ops[bestnode]->ID);

		// Update the total remaining processing time
		totalPRemain -= pm.ops[bestnode]->p();

		// Update the set of the available operations : all successors of the scheduled operations with all predecessors scheduled
		ListDigraph::Node cursuc;
		// Check the direct successors of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			cursuc = pm.graph.target(oait);

			// Check whether all predecessors of the cursuc have been scheduled
			bool allpredsched = true;
			for (ListDigraph::InArcIt iait(pm.graph, cursuc); iait != INVALID; ++iait) {
				allpredsched = allpredsched && nodesscheduled[pm.graph.source(iait)];
			}

			// If all predecessor are scheduled then this node is newly available
			if (allpredsched) {
				nodesavail[cursuc] = true;
				availIDs.insert(pm.ops[cursuc]->ID);
			}
		}

	}

	// Run the local search
	/*
	ls.checkCorectness(true);
	ls.maxIter(0);
	if (ls.maxIter() > 0) {
			pm.save();

			ls.setPM(&pm);
			ls.setResources(&rc);
			ls.run();

			pm.restore();
	}
	 */

	//totelapsed = total.elapsed();

	//out << "Elapsed : " << totelapsed << endl;
	//out << "Partial time percentage : " << double(partelapsedms) / double(totelapsed) << endl;

	// Save the schedule found so far
	//pm.save();

	// Prepare the schedule
	prepareSched();
}

void ATCANScheduler::schedAct1() {

	QTextStream out(stdout);
	QTime total;
	//int totelapsed = 0;
	//QTime partial;
	//int partelapsedms = 0;

	/** Algorithm:
	 *  0. Prepare the AVAILABLE operations.
	 *  1. One loop:
	 *	1.1. Select the AVAILABLE operations. Update their ready times and the start times.
	 *	1.2. Select the operation with the highest 1/p or w/p from the AVAILABLE.
	 *	1.3. Schedule the selected operation.
	 *	1.4. Update the AVAILABLE operations excluding the SCHEDULED
	 */

	// Initialize the scheduler
	init();

	//out << pm << endl;

	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//out << lenRemain[nit] << endl;
	//}
	//getchar();

	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int n = countNodes(pm.graph);
	int nsched = 0;

	// Prepare the available operations
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		nodesavail[nit] = false;
		nodesscheduled[nit] = false;
		//opID2Node[pm.ops[nit]->ID] = nit;
	}
	availIDs.clear();
	schedIDs.clear();
	nodesavail[pm.head] = true;
	availIDs.insert(pm.ops[pm.head]->ID);

	total.start();

	// Perform the scheduling
	while (nsched < n) {

		// Select the available node with the highest priority index
		ListDigraph::Node bestnode = INVALID;
		double bestprior = Math::MIN_DOUBLE;
		double curprior = -1.0;

		//out << topolOrdering.size() << endl;

		QHash<int, double> toolID2pAvg; // Average processing time of the available operations of the tool 
		QHash<int, int> toolID2ctr; // Counter of the operations for the tool group

		toolID2pAvg.clear();
		toolID2ctr.clear();

		// Initialize
		//QList<int> availIDsList = availIDs.toList();
		//Math::sort(availIDsList);
		//int curID = 0;

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] = 0.0;
			toolID2ctr[pm.ops[opID2Node[curID]]->toolID] = 0;

		}
		//}

		//out << endl << endl;
		//for (int i = 0; i < availIDsList.size(); i++) {
		//out << availIDsList[i] << " : " << *pm.ops[opID2Node[availIDsList[i]]] << endl;
		//out << toolID2pAvg[pm.ops[opID2Node[curids[i]]]->toolID] << endl;
		//}

		// For the available operations, calculate the average remaining processing times of the jobs waiting in the queue of each tool group

		QHash<int, double> ordID2largestPRemain;
		ordID2largestPRemain.clear();

		foreach(const int& curID, availIDs) {
			ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID] = 0.0;
		}

		foreach(const int& curID, availIDs) {
			if (ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID] < pRemain[opID2Node[curID]]) {
				ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID] = pRemain[opID2Node[curID]];
			}
		}

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID]; //pRemain[opID2Node[curID]]; // Remaining processing time of the job starting from the current operation
			toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;

			//out << toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] << endl;
		}

		/*
		// Calculate the sums       
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
				if (nodesavail[nit] && !nodesscheduled[nit]) {
						toolID2pAvg[pm.ops[nit]->toolID] += pm.ops[nit]->p();
						toolID2ctr[pm.ops[nit]->toolID] += 1;
				}
		}
		 */

		//foreach(const int& curID, availIDs) {
		//    toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += pm.ops[opID2Node[curID]]->p();
		//    toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;
		//}

		for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
			if (toolID2ctr[iter.key()] > 0) {
				toolID2pAvg[iter.key()] /= double(toolID2ctr[iter.key()]);
			}
		}

		// Find the machine group with the highest utilization
		double curUtil = -1.0;
		int curToolID = -1;
		for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
			if (rc(iter.key()).avgUtil() > curUtil) {
				curToolID = iter.key();
				curUtil = rc(iter.key()).avgUtil();
			}
		}
		if (curToolID == -1) {
			Debugger::err << "ATCANScheduler::schedAct : Failed to find a tool group with the highest utilization!" << ENDL;
		} else {
			//Debugger::info << curUtil << ENDL;
		}

		//Debugger::info << "Currently available operations : " << ENDL;

		//foreach(const int& curID, availIDs) {
		//    Debugger::info << curID << ENDL;
		//}

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			ListDigraph::Node curnode = opID2Node[curID];

			// Update the ready time and the start time of the current node
			double maxr = 0.0;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (maxr < pm.ops[pm.graph.source(iait)]->c()) {
					maxr = pm.ops[pm.graph.source(iait)]->c();
				}
			}
			pm.ops[curnode]->r(Math::max(pm.ops[curnode]->ir(), maxr));
			pm.ops[curnode]->s(pm.ops[curnode]->r());

			// Set p_avg
			p_avg = toolID2pAvg[pm.ops[curnode]->toolID];
			kappaPAvg = kappa * p_avg;
			kappaRPAvg = kappaR * p_avg;

			// Get the priority
			curprior = priority(curnode);

			// Consider only the currently selected machine group
			//if (pm.ops[curnode]->toolID == curToolID) {
			//if (curUtil > 0.9) {
			if (pm.ops[curnode]->toolID == 8 || pm.ops[curnode]->toolID == 9 || pm.ops[curnode]->toolID == 10) {
				//curprior *= 100000000000.0;
			}
			//out << curToolID << endl;
			//}
			//}

			//out << "Priority for " << availIDsList[i] << " is " << curprior << endl;
			//out << "p_avg is " << p_avg << endl;
			//out << "dOrd is " << dOrd[curnode] << endl;

			if (bestprior < curprior) {
				bestnode = curnode;
				bestprior = curprior;
			}

		}

		if (bestnode == INVALID) {
			Debugger::err << "PriorityScheduler::schedule : opID == -1!!!" << ENDL;
		}

		// Schedule the selected operation
		Machine &m = rc(pm.ops[bestnode]->toolID)./*nextAvailable(pm.ops[bestnode]->type); //*/earliestToFinish(pm.ops[bestnode]);

		ListDigraph::Node prevOpNode;
		if (m.operations.size() == 0) {
			prevOpNode = INVALID;
		} else {
			prevOpNode = opID2Node[m.operations.last()->ID];
		}

		m << pm.ops[bestnode];

		// Add an arc into the graph which represents the scheduling decision : The arc connects this operation and the previous operation on the machine
		if (prevOpNode != INVALID) {
			ListDigraph::Arc newArc = pm.graph.addArc(prevOpNode, bestnode);
			//selectionArcs.append(newArc);
			pm.p[newArc] = -pm.ops[prevOpNode]->p();

		}
		// Update the outgoing arcs for the best node and the ready times for the direct successors
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[bestnode]->p();

			/*
			ListDigraph::Node curSucc = pm.graph.target(oait);
			pm.ops[curSucc]->r(0.0);

			for (ListDigraph::InArcIt iait(pm.graph, curSucc); iait != INVALID; ++iait) {
					ListDigraph::Node curSuccPred = pm.graph.source(iait);
					pm.ops[curSucc]->r(Math::max(pm.ops[curSucc]->r(), pm.ops[curSuccPred]->c()));
			}

			// IMPORTANT!!! Update the start time, since decreasing the ready time does not cause decreasing of the start time
			pm.ops[curSucc]->s(pm.ops[curSucc]->r());
			 */
		}

		// Exclude the scheduled operation
		nsched++;
		nodesscheduled[bestnode] = true;
		availIDs.remove(pm.ops[bestnode]->ID);
		schedIDs.insert(pm.ops[bestnode]->ID);


		// Update the set of the available operations : all successors of the scheduled operations with all predecessors scheduled
		ListDigraph::Node cursuc;
		// Check the direct successors of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			cursuc = pm.graph.target(oait);

			// Check whether all predecessors of the cursuc have been scheduled
			bool allpredsched = true;
			for (ListDigraph::InArcIt iait(pm.graph, cursuc); iait != INVALID; ++iait) {
				allpredsched = allpredsched && nodesscheduled[pm.graph.source(iait)];
			}

			// If all predecessor are scheduled then this node is newly available
			if (allpredsched) {
				nodesavail[cursuc] = true;
				availIDs.insert(pm.ops[cursuc]->ID);
			}
		}

	}

	// Run the local search
	/*
	ls.checkCorectness(true);
	ls.maxIter(0);
	if (ls.maxIter() > 0) {
			pm.save();

			ls.setPM(&pm);
			ls.setResources(&rc);
			ls.run();

			pm.restore();
	}
	 */

	//totelapsed = total.elapsed();

	//out << "Elapsed : " << totelapsed << endl;
	//out << "Partial time percentage : " << double(partelapsedms) / double(totelapsed) << endl;

	// Save the schedule found so far
	//pm.save();

	// Prepare the schedule
	prepareSched();
}

void ATCANScheduler::setObjective(ScalarObjective& newObj) {

	// Initialize the base class
	PriorityScheduler::setObjective(newObj);

	// Set the objective for the CS
	cs->setObjective(newObj);

}

/** ******************** ATC scheduler TEST ************************* **/

ATCSchedulerTest::ATCSchedulerTest() : dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	kappaOptim(true);
	considerSucc(true);

	kappa = 0.3;
}

ATCSchedulerTest::ATCSchedulerTest(ATCSchedulerTest& orig) : PriorityScheduler(orig), dOrd(pm.graph), pRemain(pm.graph), lenRemain(pm.graph) {
	kappaOptim(orig.kappaoptim);
	considerSucc(orig.considersucc);

	kappa = orig.kappa;
}

ATCSchedulerTest::~ATCSchedulerTest() { }

Clonable* ATCSchedulerTest::clone() {
	return new ATCSchedulerTest(*this);
}

void ATCSchedulerTest::preparePM() {
	//PriorityScheduler::preparePM();
	//return;

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
	smallestD = Math::MAX_DOUBLE;
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
				Debugger::err << "ATCSchedulerTest::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
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

}

double ATCSchedulerTest::priority(const ListDigraph::Node & node) {
	//Debugger::err << "Priority ATC" << ENDL;

	double curprior = 0.0;

	//if (pm.ops[node]->ID >= 0) {

	curprior = I(kappaPAvg, kappaRPAvg, node);

	//} else {
	//    curprior = Math::MAX_DOUBLE;
	//}

	return curprior;
}

double ATCSchedulerTest::I(const double &kappapavg, const double& /*kappaRpAvg*/, const ListDigraph::Node & node) {
	if (pm.ops[node]->p() <= 0.00000001) return Math::MAX_DOUBLE;

	Machine &m = rc(pm.ops[node]->toolID)./*nextAvailable(pm.ops[node]->type); //*/earliestToFinish(pm.ops[node]);
	double t = m.time(); // Time the machine becomes available
	//double h = (pRemain[node] > 0.0) ? (dOrd[node] - smallestD - Math::max(pm.ops[node]->r(), t)) / pRemain[node] : 0.0;
	double p = m.procTime(pm.ops[node]);
	double r = pm.ops[node]->r();
	double w = pm.ops[node]->w();

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

	double d = dOrd[node] + p /*ff * pm.ops[node]->p()*/ - ff * (pRemain[node]);

	/*
	cout << " Order " << pm.ops[node]->OID << endl;
	cout << " dOrd = " << dOrd[node] << endl;
	cout << " d : " << d << endl;
	cout << " p : " << p << endl;
	cout << " pRemain : " << pRemain[node] << endl;
	cout << " ff : " << ff << endl;
	cout << " ff*pRemain : " << ff * pRemain[node] << endl;
	 */

	d = Math::max(d, Math::max(r, t) + p);

	//cout << " dmod : " << d << endl;
	//getchar();

	double slack = (d - p) + r - t; // / ff;

	double exppow = -Math::max(slack, 0.0) / kappapavg; //kpavg;
	double exppowr = -(Math::max(/*(1.0 + ff) **/ r - t, 0.0)) / kappaRPAvg; //kR_pAvg; //*/kappapavg;

	/*
	Debugger::info << "Operation : " << pm.ops[node]->ID << ENDL;
	Debugger::info << "Priority : " << pm.ops[node]->w() / p * Math::exp(exppow) << ENDL;
	Debugger::info << "k = " << kappa << ENDL;
	Debugger::info << "pavg = " << p_avg << ENDL;
	Debugger::info << "kpavg = " << kappapavg << ENDL;
	Debugger::info << "slack = " << slack << ENDL;
	Debugger::info << "pow = " << exppow << " e^pow = " << Math::exp(exppow) << ENDL;
	
	getchar();
	 */

	//p = pRemain[node];

	//if (r <= 0.00000001) return pm.ops[node]->w() / p;

	return w / p * Math::exp(exppow) * Math::exp(exppowr);

}

void ATCSchedulerTest::scheduleActions() {
	QTextStream out(stdout);

	// Best found PM
	ProcessModel bestPM;

	// Preserve the state of the resources
	Resources rcInit = rc;

	CombinedScheduler cs;

	cs.pm = pm;
	cs.rc = rc;
	cs.sched = sched;

	Scheduler* sch = 0;

	sch = new WFIFOScheduler;
	sch->ID = 1;
	((WFIFOScheduler*) sch)->weightedFIFO(false);
	cs << sch;
	delete sch;

	sch = new WFIFOScheduler;
	sch->ID = 2;
	((WFIFOScheduler*) sch)->weightedFIFO(true);
	cs << sch;
	delete sch;

	sch = new WEODScheduler;
	sch->ID = 3;
	((WEODScheduler*) sch)->weightedEOD(false);
	cs << sch;
	delete sch;

	sch = new WEODScheduler;
	sch->ID = 4;
	((WEODScheduler*) sch)->weightedEOD(true);
	cs << sch;
	delete sch;

	sch = new WMDDScheduler;
	sch->ID = 5;
	((WMDDScheduler*) sch)->weightedMDD(false);
	cs << sch;
	delete sch;

	sch = new WMDDScheduler;
	sch->ID = 6;
	((WMDDScheduler*) sch)->weightedMDD(true);
	cs << sch;
	delete sch;

	sch = 0;

	cs.options = options;
	cs.options["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

	cs.schedule();

	out << "Objective of the CombinedScheduler : " << sched->objective << endl;

	// Get an estimation of the flow factor
	ff = cs.lastBestScheduler()->flowFactor();
	out << "Flow factor estimation : " << ff << endl;
	//getchar();

	// Here kappa optimization is performed

	if (!kappaoptim) {
		//PriorityScheduler::scheduleActions();
		schedAct();
	} else {

		QList<double> kappaGrid;
		QList<double> kappaGridBckp;
		QList<double> kappaRGrid;
		QList<double> FFGrid;

		kappaGrid << 0.01 << 0.011 << 0.012 << 0.013 << 0.014 << 0.015 << 0.016 << 0.017 << 0.018 << 0.019 << 0.02 << 0.021 << 0.022 << 0.023 << 0.024 << 0.025 << 0.026 << 0.027 << 0.028 << 0.029 << 0.03 << 0.04 << 0.05 << 0.05 << 0.06 << 0.07 << 0.08 << 0.09 << 0.1 << 0.25 << 0.5 << 0.75 << 1.0 << 1.25 << 1.5 << 1.75 << 2.0 << 2.5 << 3.0 << 3.5 << 4.0 << 4.5 << 5.0 << 5.5 << 6.0 << 6.5 << 10000000000000.0; // << 7.0 << 7.5 << 8.0 << 8.5 << 9.0 << 10.0 << 15.0 << 20.0 << 50.0 << 100.0 << 500.0 << 1000.0; //<< 0.01 << 0.1 << 0.2 << 0.6 << 0.8 << 1.0 << 1.2 << 1.4 << 1.6 << 1.8 << 2.0 << 2.4 << 2.8 << 3.2 << 3.6 << 4.0 << 4.4 << 4.8 << 5.2;

		kappaGridBckp = kappaGrid;

		kappaRGrid /*<< 10000000000.0; //*/ << 0.001 << 0.01 << 0.04 << 0.07 << 0.125 << 0.2 << 0.25 << 0.4 << 0.8 << 1.2 << 100000000.0; //<< 0.001 << 0.0025 << 0.004 << 0.005 << 0.025 << 0.04 << 0.05 << 0.25 << 0.4 << 0.6 << 0.8 << 1.0 << 1.2 << 100000000.0; // << 1.4 << 1.6 << 1.8 << 2.0 << 3.0 << 4.0 << 5.0 << 7.0 << 10.0;
		FFGrid << 1.0; // << 1.5 << 2.0 << 2.5 << 3.0 << 4.0 << 5.0; //<< -2.0 << -1.0 << -0.5 << 0.0 << 0.5 << 0.75 << 1.0 << 1.5 << 2.0 << 2.5 << 3.0;

		//int totalParComb = kappaGrid.size() * kappaRGrid.size() * FFGrid.size();
		int curParCombCtr = 1;

		Schedule bestsched;

		bestsched.init();
		bestsched.objective = Math::MAX_DOUBLE;

		double bestFF = ff;
		double bestKappa = Math::MIN_DOUBLE;
		double bestKappaR = Math::MIN_DOUBLE;

		for (int iFF = 0; iFF < FFGrid.size(); iFF++) {
			//FF = FFGrid[iFF];
			//out << "ATCSchedulerTest::scheduleActions : FF = " << FF <<endl;

			for (int iKR = 0; iKR < kappaRGrid.size(); iKR++) {
				kappaR = kappaRGrid[iKR];

				kappaGrid = kappaGridBckp;

				for (int iK = 0; iK < kappaGrid.size(); iK++) {
					kappa = kappaGrid[iK];



					// Perform scheduling for the current parameter combination

					// Delete the scheduling relevant data from the process model
					pm.clearSchedRelData();

					// Restore the state of the resources
					rc = rcInit;

					// Perform scheduling
					schedAct();

					if (bestsched.objective > sched->objective) {
						bestsched = *sched;

						bestKappa = kappa;
						bestKappaR = kappaR;
						bestFF = flowFactor(); //FF;
						//ff = bestFF;

						out << "ATCSchedulerTest::scheduleActions : Found a better TWT : " << bestsched.objective << endl;
						out << "ATCSchedulerTest::scheduleActions : Found a better TWT : kappa : " << bestKappa << endl;
						out << "ATCSchedulerTest::scheduleActions : Found a better TWT : kappaR : " << bestKappaR << endl;

						bestPM = pm; // Save the best PM

						//TWT twt;
						out << "Recalculated TWT : " << TWT()(bestPM) << endl;
						//getchar();

						// Create an additional grid

						/*
						double offset = 0.0001;

						QList<double> newKappaGrid;
						for (int i = iK; i < Math::min(iK + 50, kappaGrid.size()); i++) {
							newKappaGrid.append(kappaGrid[iK]);
						}

						while (offset < 0.01) {

							if (kappa - offset > 0) newKappaGrid << kappa - offset;
							newKappaGrid << kappa + offset;

							offset += 0.0001;
						}
						kappaGrid = newKappaGrid;
						iK = -1;
						iKR = -1;
						 */


					}

					//out << "ATC completed combinations : " << QString::number(double(curParCombCtr) / double(totalParComb) *100.0, 'f', 1) << " % " << endl;
					curParCombCtr++;

				}

			}

		}

		//TWT twt;
		//out << "Recalculated TWT of the best PM : " << twt(bestPM) << endl;

		// Store the best solution
		pm = bestPM;
		*sched = bestsched;

		// Restore the best parameters
		kappa = bestKappa;
		kappaR = bestKappaR;
		ff = bestFF;

		/* Commented out because of the mismatch between the TWT value returned by the ATC-rule and the initial TWT for the LS : 
		 * probably, the commented recalculation provides other solution.
         
		  sched = bestsched;

		  out << "Best FF = " << bestFF << endl;
		  out << "Best kappa = " << bestKappa << endl;
		  out << "Best kappaR = " << bestKappaR << endl;

		  // Run the scheduler with the best found parameter values
		  kappa = bestKappa;
		  kappaR = bestKappaR;
		  ff = bestFF;

		  // Delete the scheduling relevant data from the process model
		  pm.clearSchedRelData();
                
		  // Restore the state of the resources
		  rc = rcInit;

		  // Perform scheduling
		  schedAct();

		  // Prepare the schedule
		  //prepareSched();

		  if (bestsched.objective < sched->objective) {
		 *sched = bestsched;
		  }

		  //out << "Objective of the best schedule : " << bestsched.objective << endl;
		  //out << "Objective of the current schedule : " << sched->objective << endl;
         
		 */

	}

}

void ATCSchedulerTest::schedAct() {

	//QTextStream out(stdout);
	//QTime total;
	//int totelapsed = 0;
	//QTime partial;
	//int partelapsedms = 0;

	/** Algorithm:
	 *  0. Prepare the AVAILABLE operations.
	 *  1. One loop:
	 *	1.1. Select the AVAILABLE operations. Update their ready times and the start times.
	 *	1.2. Select the operation with the highest 1/p or w/p from the AVAILABLE.
	 *	1.3. Schedule the selected operation.
	 *	1.4. Update the AVAILABLE operations excluding the SCHEDULED
	 */

	// Initialize the scheduler
	init();

	//out << pm << endl;
	//getchar();

	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//out << lenRemain[nit] << endl;
	//}
	//getchar();

	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int n = countNodes(pm.graph);
	int nsched = 0;

	// Prepare the available operations
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		nodesavail[nit] = false;
		nodesscheduled[nit] = false;
		//opID2Node[pm.ops[nit]->ID] = nit;
	}
	availIDs.clear();
	schedIDs.clear();
	nodesavail[pm.head] = true;
	availIDs.insert(pm.ops[pm.head]->ID);

	totalPRemain = 0.0;
	for (ListDigraph::OutArcIt oait(pm.graph, pm.head); oait != INVALID; ++oait) {
		totalPRemain += pRemain[pm.graph.target(oait)];
	}

	//total.start();

	// Perform the scheduling
	while (nsched < n) {

		// Select the available node with the highest priority index
		ListDigraph::Node bestnode = INVALID;
		double bestprior = Math::MIN_DOUBLE;
		double curprior = -1.0;

		//out << topolOrdering.size() << endl;

		QHash<int, double> toolID2pAvg; // Average processing time of the available operations of the tool 
		QHash<int, int> toolID2ctr; // Counter of the operations for the tool group

		toolID2pAvg.clear();
		toolID2ctr.clear();

		// Initialize
		//QList<int> availIDsList = availIDs.toList();
		//Math::sort(availIDsList);
		//int curID = 0;

		//        foreach(const int& curID, availIDs) {
		//for (int i = 0; i < availIDsList.size(); i++) {
		//curID = availIDsList[i];

		//            toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] = 0.0;
		//            toolID2ctr[pm.ops[opID2Node[curID]]->toolID] = 0;

		//        }
		//}

		//out << endl << endl;
		//for (int i = 0; i < availIDsList.size(); i++) {
		//out << availIDsList[i] << " : " << *pm.ops[opID2Node[availIDsList[i]]] << endl;
		//out << toolID2pAvg[pm.ops[opID2Node[curids[i]]]->toolID] << endl;
		//}

		// For the available operations, calculate the average remaining processing times of the jobs waiting in the queue of each tool group

		p_avg = 0.0;
		int pAvgCtr = 0;

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			//            toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += pRemain[opID2Node[curID]]; // Remaining processing time of the job starting from the current operation
			//            toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;

			p_avg += pRemain[opID2Node[curID]];
			pAvgCtr++;

			//out << toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] << endl;
		}

		if (pAvgCtr > 0) p_avg /= double (pAvgCtr);

		//if (pAvgCtr > 0) p_avg = totalPRemain / 70.0;

		kappaPAvg = kappa * p_avg;
		kappaRPAvg = kappaR * p_avg;

		/*
		// Calculate the sums       
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
				if (nodesavail[nit] && !nodesscheduled[nit]) {
						toolID2pAvg[pm.ops[nit]->toolID] += pm.ops[nit]->p();
						toolID2ctr[pm.ops[nit]->toolID] += 1;
				}
		}
		 */

		//foreach(const int& curID, availIDs) {
		//    toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += pm.ops[opID2Node[curID]]->p();
		//    toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;
		//}

		//        for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
		//            if (toolID2ctr[iter.key()] > 0) {
		//                toolID2pAvg[iter.key()] /= double(toolID2ctr[iter.key()]);
		//            }
		//        }

		// Find the machine group with the highest utilization
		//        double curUtil = -1.0;
		//        int curToolID = -1;
		//        for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
		//            if (rc(iter.key()).avgUtil() > curUtil) {
		//                curToolID = iter.key();
		//                curUtil = rc(iter.key()).avgUtil();
		//            }
		//        }
		//        if (curToolID == -1) {
		//            Debugger::err << "ATCSchedulerTest::schedAct : Failed to find a tool group with the highest utilization!" << ENDL;
		//        } else {
		//            //Debugger::info << curUtil << ENDL;
		//        }

		//Debugger::info << "Currently available operations : " << ENDL;

		//foreach(const int& curID, availIDs) {
		//    Debugger::info << curID << ENDL;
		//}

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			ListDigraph::Node curnode = opID2Node[curID];

			// Update the ready time and the start time of the current node
			double maxr = 0.0;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (maxr < pm.ops[pm.graph.source(iait)]->c()) {
					maxr = pm.ops[pm.graph.source(iait)]->c();
				}
			}
			pm.ops[curnode]->r(Math::max(pm.ops[curnode]->ir(), maxr));
			pm.ops[curnode]->s(pm.ops[curnode]->r());

			// Set p_avg
			//p_avg = toolID2pAvg[pm.ops[curnode]->toolID];

			//            kappaPAvg = kappa * p_avg;
			//            kappaRPAvg = kappaR * p_avg;

			// Get the priority
			curprior = priority(curnode);

			// Consider only the currently selected machine group
			//if (pm.ops[curnode]->toolID == curToolID) {
			//if (curUtil > 0.9) {
			if (pm.ops[curnode]->toolID == 8 || pm.ops[curnode]->toolID == 9 || pm.ops[curnode]->toolID == 10) {
				//curprior *= 100000000000.0;
			}
			//out << curToolID << endl;
			//}
			//}

			//out << "Priority for " << availIDsList[i] << " is " << curprior << endl;
			//out << "p_avg is " << p_avg << endl;
			//out << "dOrd is " << dOrd[curnode] << endl;

			if (bestprior < curprior) {
				bestnode = curnode;
				bestprior = curprior;
			}

		}

		if (bestnode == INVALID) {
			Debugger::err << "PriorityScheduler::schedule : opID == -1!!!" << ENDL;
		}

		// Schedule the selected operation
		Machine &m = rc(pm.ops[bestnode]->toolID)./*nextAvailable(pm.ops[bestnode]->type); //*/earliestToFinish(pm.ops[bestnode]);

		ListDigraph::Node prevOpNode;
		if (m.operations.size() == 0) {
			prevOpNode = INVALID;
		} else {
			prevOpNode = opID2Node[m.operations.last()->ID];
		}

		m << pm.ops[bestnode];

		// Add an arc into the graph which represents the scheduling decision : The arc connects this operation and the previous operation on the machine
		if (prevOpNode != INVALID) {
			ListDigraph::Arc newArc = pm.graph.addArc(prevOpNode, bestnode);
			//selectionArcs.append(newArc);
			pm.p[newArc] = -pm.ops[prevOpNode]->p();

		}
		// Update the outgoing arcs for the best node and the ready times for the direct successors
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[bestnode]->p();

			/*
			ListDigraph::Node curSucc = pm.graph.target(oait);
			pm.ops[curSucc]->r(0.0);

			for (ListDigraph::InArcIt iait(pm.graph, curSucc); iait != INVALID; ++iait) {
					ListDigraph::Node curSuccPred = pm.graph.source(iait);
					pm.ops[curSucc]->r(Math::max(pm.ops[curSucc]->r(), pm.ops[curSuccPred]->c()));
			}

			// IMPORTANT!!! Update the start time, since decreasing the ready time does not cause decreasing of the start time
			pm.ops[curSucc]->s(pm.ops[curSucc]->r());
			 */
		}

		// Exclude the scheduled operation
		nsched++;
		nodesscheduled[bestnode] = true;
		availIDs.remove(pm.ops[bestnode]->ID);
		schedIDs.insert(pm.ops[bestnode]->ID);

		// Update the total remaining processing time
		totalPRemain -= pm.ops[bestnode]->p();

		// Update the set of the available operations : all successors of the scheduled operations with all predecessors scheduled
		ListDigraph::Node cursuc;
		// Check the direct successors of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			cursuc = pm.graph.target(oait);

			// Check whether all predecessors of the cursuc have been scheduled
			bool allpredsched = true;
			for (ListDigraph::InArcIt iait(pm.graph, cursuc); iait != INVALID; ++iait) {
				allpredsched = allpredsched && nodesscheduled[pm.graph.source(iait)];
			}

			// If all predecessor are scheduled then this node is newly available
			if (allpredsched) {
				nodesavail[cursuc] = true;
				availIDs.insert(pm.ops[cursuc]->ID);
			}
		}

	}

	// Run the local search
	/*
	ls.checkCorectness(true);
	ls.maxIter(0);
	if (ls.maxIter() > 0) {
			pm.save();

			ls.setPM(&pm);
			ls.setResources(&rc);
			ls.run();

			pm.restore();
	}
	 */

	//totelapsed = total.elapsed();

	//out << "Elapsed : " << totelapsed << endl;
	//out << "Partial time percentage : " << double(partelapsedms) / double(totelapsed) << endl;

	// Save the schedule found so far
	//pm.save();

	// Prepare the schedule
	prepareSched();
}

void ATCSchedulerTest::schedAct1() {

	QTextStream out(stdout);
	QTime total;
	//int totelapsed = 0;
	//QTime partial;
	//int partelapsedms = 0;

	/** Algorithm:
	 *  0. Prepare the AVAILABLE operations.
	 *  1. One loop:
	 *	1.1. Select the AVAILABLE operations. Update their ready times and the start times.
	 *	1.2. Select the operation with the highest 1/p or w/p from the AVAILABLE.
	 *	1.3. Schedule the selected operation.
	 *	1.4. Update the AVAILABLE operations excluding the SCHEDULED
	 */

	// Initialize the scheduler
	init();

	//out << pm << endl;

	//for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
	//out << lenRemain[nit] << endl;
	//}
	//getchar();

	ListDigraph::NodeMap<bool> nodesavail(pm.graph, false); // Available for scheduling nodes
	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	int n = countNodes(pm.graph);
	int nsched = 0;

	// Prepare the available operations
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		nodesavail[nit] = false;
		nodesscheduled[nit] = false;
		//opID2Node[pm.ops[nit]->ID] = nit;
	}
	availIDs.clear();
	schedIDs.clear();
	nodesavail[pm.head] = true;
	availIDs.insert(pm.ops[pm.head]->ID);

	total.start();

	// Perform the scheduling
	while (nsched < n) {

		// Select the available node with the highest priority index
		ListDigraph::Node bestnode = INVALID;
		double bestprior = Math::MIN_DOUBLE;
		double curprior = -1.0;

		//out << topolOrdering.size() << endl;

		QHash<int, double> toolID2pAvg; // Average processing time of the available operations of the tool 
		QHash<int, int> toolID2ctr; // Counter of the operations for the tool group

		toolID2pAvg.clear();
		toolID2ctr.clear();

		// Initialize
		//QList<int> availIDsList = availIDs.toList();
		//Math::sort(availIDsList);
		//int curID = 0;

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] = 0.0;
			toolID2ctr[pm.ops[opID2Node[curID]]->toolID] = 0;

		}
		//}

		//out << endl << endl;
		//for (int i = 0; i < availIDsList.size(); i++) {
		//out << availIDsList[i] << " : " << *pm.ops[opID2Node[availIDsList[i]]] << endl;
		//out << toolID2pAvg[pm.ops[opID2Node[curids[i]]]->toolID] << endl;
		//}

		// For the available operations, calculate the average remaining processing times of the jobs waiting in the queue of each tool group

		QHash<int, double> ordID2largestPRemain;
		ordID2largestPRemain.clear();

		foreach(const int& curID, availIDs) {
			ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID] = 0.0;
		}

		foreach(const int& curID, availIDs) {
			if (ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID] < pRemain[opID2Node[curID]]) {
				ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID] = pRemain[opID2Node[curID]];
			}
		}

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += ordID2largestPRemain[pm.ops[opID2Node[curID]]->OID]; //pRemain[opID2Node[curID]]; // Remaining processing time of the job starting from the current operation
			toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;

			//out << toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] << endl;
		}

		/*
		// Calculate the sums       
		for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
				if (nodesavail[nit] && !nodesscheduled[nit]) {
						toolID2pAvg[pm.ops[nit]->toolID] += pm.ops[nit]->p();
						toolID2ctr[pm.ops[nit]->toolID] += 1;
				}
		}
		 */

		//foreach(const int& curID, availIDs) {
		//    toolID2pAvg[pm.ops[opID2Node[curID]]->toolID] += pm.ops[opID2Node[curID]]->p();
		//    toolID2ctr[pm.ops[opID2Node[curID]]->toolID] += 1;
		//}

		for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
			if (toolID2ctr[iter.key()] > 0) {
				toolID2pAvg[iter.key()] /= double(toolID2ctr[iter.key()]);
			}
		}

		// Find the machine group with the highest utilization
		double curUtil = -1.0;
		int curToolID = -1;
		for (QHash<int, double>::iterator iter = toolID2pAvg.begin(); iter != toolID2pAvg.end(); iter++) {
			if (rc(iter.key()).avgUtil() > curUtil) {
				curToolID = iter.key();
				curUtil = rc(iter.key()).avgUtil();
			}
		}
		if (curToolID == -1) {
			Debugger::err << "ATCSchedulerTest::schedAct : Failed to find a tool group with the highest utilization!" << ENDL;
		} else {
			//Debugger::info << curUtil << ENDL;
		}

		//Debugger::info << "Currently available operations : " << ENDL;

		//foreach(const int& curID, availIDs) {
		//    Debugger::info << curID << ENDL;
		//}

		foreach(const int& curID, availIDs) {
			//for (int i = 0; i < availIDsList.size(); i++) {
			//curID = availIDsList[i];

			ListDigraph::Node curnode = opID2Node[curID];

			// Update the ready time and the start time of the current node
			double maxr = 0.0;
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (maxr < pm.ops[pm.graph.source(iait)]->c()) {
					maxr = pm.ops[pm.graph.source(iait)]->c();
				}
			}
			pm.ops[curnode]->r(Math::max(pm.ops[curnode]->ir(), maxr));
			pm.ops[curnode]->s(pm.ops[curnode]->r());

			// Set p_avg
			p_avg = toolID2pAvg[pm.ops[curnode]->toolID];
			kappaPAvg = kappa * p_avg;
			kappaRPAvg = kappaR * p_avg;

			// Get the priority
			curprior = priority(curnode);

			// Consider only the currently selected machine group
			//if (pm.ops[curnode]->toolID == curToolID) {
			//if (curUtil > 0.9) {
			if (pm.ops[curnode]->toolID == 8 || pm.ops[curnode]->toolID == 9 || pm.ops[curnode]->toolID == 10) {
				//curprior *= 100000000000.0;
			}
			//out << curToolID << endl;
			//}
			//}

			//out << "Priority for " << availIDsList[i] << " is " << curprior << endl;
			//out << "p_avg is " << p_avg << endl;
			//out << "dOrd is " << dOrd[curnode] << endl;

			if (bestprior < curprior) {
				bestnode = curnode;
				bestprior = curprior;
			}

		}

		if (bestnode == INVALID) {
			Debugger::err << "PriorityScheduler::schedule : opID == -1!!!" << ENDL;
		}

		// Schedule the selected operation
		Machine &m = rc(pm.ops[bestnode]->toolID)./*nextAvailable(pm.ops[bestnode]->type); //*/earliestToFinish(pm.ops[bestnode]);

		ListDigraph::Node prevOpNode;
		if (m.operations.size() == 0) {
			prevOpNode = INVALID;
		} else {
			prevOpNode = opID2Node[m.operations.last()->ID];
		}

		m << pm.ops[bestnode];

		// Add an arc into the graph which represents the scheduling decision : The arc connects this operation and the previous operation on the machine
		if (prevOpNode != INVALID) {
			ListDigraph::Arc newArc = pm.graph.addArc(prevOpNode, bestnode);
			//selectionArcs.append(newArc);
			pm.p[newArc] = -pm.ops[prevOpNode]->p();

		}
		// Update the outgoing arcs for the best node and the ready times for the direct successors
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[bestnode]->p();

			/*
			ListDigraph::Node curSucc = pm.graph.target(oait);
			pm.ops[curSucc]->r(0.0);

			for (ListDigraph::InArcIt iait(pm.graph, curSucc); iait != INVALID; ++iait) {
					ListDigraph::Node curSuccPred = pm.graph.source(iait);
					pm.ops[curSucc]->r(Math::max(pm.ops[curSucc]->r(), pm.ops[curSuccPred]->c()));
			}

			// IMPORTANT!!! Update the start time, since decreasing the ready time does not cause decreasing of the start time
			pm.ops[curSucc]->s(pm.ops[curSucc]->r());
			 */
		}

		// Exclude the scheduled operation
		nsched++;
		nodesscheduled[bestnode] = true;
		availIDs.remove(pm.ops[bestnode]->ID);
		schedIDs.insert(pm.ops[bestnode]->ID);


		// Update the set of the available operations : all successors of the scheduled operations with all predecessors scheduled
		ListDigraph::Node cursuc;
		// Check the direct successors of the current node
		for (ListDigraph::OutArcIt oait(pm.graph, bestnode); oait != INVALID; ++oait) {
			cursuc = pm.graph.target(oait);

			// Check whether all predecessors of the cursuc have been scheduled
			bool allpredsched = true;
			for (ListDigraph::InArcIt iait(pm.graph, cursuc); iait != INVALID; ++iait) {
				allpredsched = allpredsched && nodesscheduled[pm.graph.source(iait)];
			}

			// If all predecessor are scheduled then this node is newly available
			if (allpredsched) {
				nodesavail[cursuc] = true;
				availIDs.insert(pm.ops[cursuc]->ID);
			}
		}

	}

	// Run the local search
	/*
	ls.checkCorectness(true);
	ls.maxIter(0);
	if (ls.maxIter() > 0) {
			pm.save();

			ls.setPM(&pm);
			ls.setResources(&rc);
			ls.run();

			pm.restore();
	}
	 */

	//totelapsed = total.elapsed();

	//out << "Elapsed : " << totelapsed << endl;
	//out << "Partial time percentage : " << double(partelapsedms) / double(totelapsed) << endl;

	// Save the schedule found so far
	//pm.save();

	// Prepare the schedule
	prepareSched();
}
