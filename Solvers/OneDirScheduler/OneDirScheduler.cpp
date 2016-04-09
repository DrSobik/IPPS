/* 
 * File:   OneDirScheduler.cpp
 * Author: DrSobik
 * 
 * Created on August 26, 2011, 10:24 AM
 */

#include "OneDirScheduler.h"

OneDirScheduler::OneDirScheduler() {
}

OneDirScheduler::~OneDirScheduler() {
}

bool OneDirScheduler::schedule(ProcessModel &pm, Resources &rc, Schedule &schedule) {
	Debugger::info << "Building schedule using simple one-directional approach." << ENDL;

	/** Algorithm:
	 * 
	 * Lv - set of head scheduled operations;
	 * Lr - set of tail scheduled operations;
	 * Av - set of additionally head-schedulable operations;
	 * Ar - set of additianally tail-schedulable operations;
	 *  
	 *  1. Lv = Lr = 0; 
	 *	   Av = first available operations of the graph;
	 *     Ar = first available operations in the reverse graph.
	 * 
	 *  2. While  Av + Ar != 0
	 * 
	 *  2.a) select operation from Av according to some priority rule;
	 *  2.b) schedule the selected operation as soon as possible;
	 *  2.c) include the operation into Lv and exclude it from Av
	 *  2.d) select next available operation(s) which are not in Ar + Lr and insert them into Av
	 * 
	 *  2.e) select operation from Ar according to some priority rule;
	 *  2.f) schedule the selected operation as late as possible before all of the operations from Lr;
	 *  2.g) include the operation into Lr and exclude it from Ar;
	 *  2.h) select next available operation(s) (in inverse graph) which are not in Av + Lv and insert them into Ar.
	 *  
	 *	3. Shift left the start times of the operations from Lr so that they start immediately after latest from Lv finishes.
	 *  
	 * */

	QTextStream out(stdout);

	QList<ListDigraph::Node> Lv;
	QList<ListDigraph::Node> Lr;
	QList<ListDigraph::Node> Av;
	QList<ListDigraph::Node> Ar;
	QList<ListDigraph::Node> Avnext;
	QList<ListDigraph::Node> Lravail; // Currently available nodes from Lr

	QMap<ListDigraph::Node, Machine*> nodemachLr; // Assignments of the operations from Lr to the machines
	QHash<int, double> backmach; // Machines for backward operation scheduling <machine ID, time for operation finish>
	QMap<ListDigraph::Node, Machine*> nodemach;

	for (int i = 0; i < rc.machines().size(); i++) {
		backmach[rc.machines()[i]->ID] = 10000000;
	}

	ListDigraph::NodeMap<bool> nodesscheduled(pm.graph, false); // Scheduled nodes
	bool nodeavail; // Used for checking whether some node is available fro scheduling
	ListDigraph::Node curnode;

	Lv.clear();
	Lr.clear();
	Av.clear();
	Ar.clear();

	// Initialize the sets of operations Av
	for (ListDigraph::OutArcIt gait(pm.graph, pm.head); gait != INVALID; ++gait) {
		for (ListDigraph::OutArcIt lait(pm.graph, pm.graph.target(gait)); lait != INVALID; ++lait) {
			Av.append(pm.graph.target(lait));
		}
	}

	//out << "Operations of Av:" << endl;
	//for (int i = 0; i < Av.size(); i++) {
	//	out << Av[i] << endl;
	//}

	// Initialize the sets of operations Ar
	for (ListDigraph::InArcIt gait(pm.graph, pm.tail); gait != INVALID; ++gait) {
		for (ListDigraph::InArcIt lait(pm.graph, pm.graph.source(gait)); lait != INVALID; ++lait) {
			Ar.append(pm.graph.source(lait));
			//Ar.append(pm.graph.source(gait));
		}
	}



	//out << "Operations of Ar:" << endl;
	//for (int i = 0; i < Ar.size(); i++) {
	//	out << *pm.ops[Ar[i]] << endl;
	//}

	//getchar();

	NodeWeightComparatorGreater nwcg(&pm);

	// Run the loop
	while (Av.size() + Ar.size() > 0) {

		// Scheduling one operation from Av
		Avnext.clear();

		//out << "Scheduling operations" << endl;
		//for (int i = 0; i < Av.size(); i++) {
		//	out << pm.ops[Av[i]]->ID <<" ";
		//}
		//out<<endl;
		if (Av.size() > 0) {
			// Sort the nodes of Av
			qSort(Av.begin(), Av.end(), nwcg);

			curnode = Av[0];

			// Schedule the first operation (as soon as possible)
			//Machine &m = rc(pm.ops[Av[0]]->toolID).nextAvailable();

			// Select the fastest available machine from the corresponding tool group
			//Machine &m = rc(pm.ops[Av[0]]->toolID).fastestAvailable(pm.ops[Av[0]]->r(), pm.ops[Av[0]]->type);

			// Select the machine from the corresponding tool group to finish the operation the earliest
			Machine &m = rc(pm.ops[curnode]->toolID).earliestToFinish(pm.ops[curnode]);

			m << pm.ops[curnode];
			nodesscheduled[curnode] = true;
			nodemach[curnode] = &m;

			// Include the operation into Lv
			Lv.append(curnode);

			// Iterate through all child nodes of the current node
			for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
				if (!Lr.contains(pm.graph.target(oait)) && !Ar.contains(pm.graph.target(oait))) {
					// Check availability
					nodeavail = true;
					for (ListDigraph::InArcIt iait(pm.graph, pm.graph.target(oait)); iait != INVALID; ++iait) {
						nodeavail = nodeavail && nodesscheduled[pm.graph.source(iait)];
					}
					if (nodeavail) {
						//out << "Now available : " << pm.graph.id(cursucc) << endl;
						// Update ready time of the newly enabled node
						pm.ops[pm.graph.target(oait)]->r(0.0);
						for (ListDigraph::InArcIt init(pm.graph, pm.graph.target(oait)); init != INVALID; ++init) {
							pm.ops[pm.graph.target(oait)]->r(Math::max(pm.ops[pm.graph.target(oait)]->r(), pm.ops[pm.graph.source(init)]->c()));
						}

						//if (!nodesscheduled[pm.graph.target(oait)]) {
						Av/*next*/.append(pm.graph.target(oait));
						//}
					}
				}
			}

			// Exclude the operation from Av
			Av.removeAt(0);
		}
		//getchar();

		// Collect nodes in Lr
		if (Ar.size() > 0) {

			// Sort the nodes of Ar
			qSort(Ar.begin(), Ar.end(), nwcg);

			curnode = Ar.last();
			nodesscheduled[curnode] = false;

			// Include the operation into Lr
			Lr.prepend(curnode);

			// Iterate through all parent nodes of the current node
			for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
				if (!Lv.contains(pm.graph.source(iait)) && !Av.contains(pm.graph.source(iait))) {
					Ar.prepend(pm.graph.source(iait));
				}
			}

			// Exclude the operation from Av
			Ar.removeLast();
		}

		//out << "Av size = " << Av.size() << endl;
	}

	// Now the nodes from Lv are scheduled and the nodes from Lr are collected but not assigned to the machines

	// Initialize the resources and schedule the operations from Lr on the empty machines
	rc.init();

	QList<ListDigraph::Node> keys;

	out << "Scheduling nodes from Lr ..." << endl;
	do {

		// Collect currently available (immediately schedulable) nodes from Lr
		Lravail.clear();
		for (int i = 0; i < Lr.size(); i++) {
			for (ListDigraph::InArcIt iait(pm.graph, Lr[i]); iait != INVALID; ++iait) {
				if (Lv.contains(pm.graph.source(iait)) && nodesscheduled[pm.graph.source(iait)]) {
					Lravail.append(Lr[i]);
					//Lv.append(Lr[i]);
					//Lr.removeOne(nodemachLr.keys()[i]);
					break;
				}
			}
		}

		//Sort the nodes in Lravail
		qSort(Lravail.begin(), Lravail.end(), nwcg);

		for (int i = Lravail.size() - 1; i >= 0; i--) {
			Lv.append(Lravail[i]);
			Lr.removeOne(Lravail[i]);
		}

		//out << "Lravail operations before ready times update:" << endl;
		//for (int i = 0; i < Lravail.size(); i++) {
		//	out << *pm.ops[Lravail[i]] << endl;
		//}
		//getchar();

		// Update the ready time of the operations in Lravail and schedule them
		for (int i = 0; i < Lravail.size(); i++) {
			pm.ops[Lravail[i]]->r(0.0);
			for (ListDigraph::InArcIt iait(pm.graph, Lravail[i]); iait != INVALID; ++iait) {
				pm.ops[Lravail[i]]->r(Math::max(pm.ops[Lravail[i]]->r(), pm.ops[pm.graph.source(iait)]->c()));
			}

			// Schedule the available nodes
			Machine &m = rc(pm.ops[Lravail[i]]->toolID).earliestToFinish(pm.ops[Lravail[i]]);

			m << pm.ops[Lravail[i]];

			nodesscheduled[Lravail[i]] = true;
			nodemach[Lravail[i]] = &m;
		}

		//out << "Lravail operations after ready times update:" << endl;
		//for (int i = 0; i < Lravail.size(); i++) {
		//	out << *pm.ops[Lravail[i]] << endl;
		//}
		//getchar();
	} while (Lravail.size() > 0);
	out << "Done scheduling nodes from Lr." << endl;


	// Mark all nodes as unscheduled and reschedule the whole set of nodes under consideration of the machine assignment
	rc.init();
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		nodesscheduled[nit] = false;
	}


	int nschednodes = 0;
	nodesscheduled[pm.head] = true;
	bool avail;
	for (ListDigraph::OutArcIt oait(pm.graph, pm.head); oait != INVALID; ++oait) {
		nodesscheduled[pm.graph.target(oait)] = true;
	}

	while (nschednodes < Lv.size()) {
		qSort(Lv.begin(), Lv.end(), nwcg);
		// Reschedule all of the nodes
		for (int i = 0; i < Lv.size(); i++) {
			avail = true;
			for (ListDigraph::InArcIt iait(pm.graph, Lv[i]); iait != INVALID; ++iait) {
				avail = avail && nodesscheduled[pm.graph.source(iait)];
			}
			if (avail && !nodesscheduled[Lv[i]]) {
				*nodemach[Lv[i]] << pm.ops[Lv[i]];
				nodesscheduled[Lv[i]] = true;
				nschednodes++;

				// Update ready times of the successive nodes
				for (ListDigraph::OutArcIt oait(pm.graph, Lv[i]); oait != INVALID; ++oait) {
					pm.ops[pm.graph.target(oait)]->r(Math::max(pm.ops[pm.graph.target(oait)]->r(), pm.ops[Lv[i]]->c()));
				}
			}
		}
	}

	//out << "Resources after scheduling" << endl;
	//out << rc << endl;

	// Iterate over all nodes directly preceding the tail and
	//Debugger::info << "Collecting schedule data ..." << ENDL;
	schedule.objective = 0.0;
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

	//Debugger::info << "Done collecting schedule data." << ENDL;

	return true;


	// Run BFS on the reverse graph to set due date for the operations

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




	return true;
}