/* 
 * File:   BiDirScheduler.cpp
 * Author: DrSobik
 * 
 * Created on August 22, 2011, 3:47 PM
 */

#include "BiDirScheduler.h"

BiDirScheduler::BiDirScheduler() {
}

BiDirScheduler::~BiDirScheduler() {
}

bool BiDirScheduler::schedule(ProcessModel &pm, Resources &rc, Schedule &schedule) {
	Debugger::info << "Building schedule using simple bidirectional approach." << ENDL;

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

		//Av = Avnext;

		// Scheduling one operation from Ar
		if (Ar.size() > 0) {
			// Sort the nodes of Ar
			qSort(Ar.begin(), Ar.end(), nwcg);

			curnode = Ar.last();

			// Schedule the operation with as late as possible before all operations from Lr
			// The operation with the smallest priority is selected

			//Machine &m = rc(pm.ops[curnode]->toolID).nextAvailable();

			// Select the fastest available machine from the corresponding tool group
			//Machine &m = rc(pm.ops[curnode]->toolID).fastestAvailable(10000000, pm.ops[curnode]->type);

			// Select the random machine from the corresponding tool group
			//Machine &m = rc(pm.ops[curnode]->toolID).randomMachine();

			// Find machine which could start this operation the latest
			Machine *m = NULL;
			double lateststarttime = 0.0;
			double curstarttime;

			// Iterate over all machines able to process the operation
			QList<Machine*> machines = rc(pm.ops[curnode]->toolID).machines();
			for (int i = 0; i < machines.size(); i++) {
				curstarttime = backmach[machines[i]->ID] - machines[i]->procTime(pm.ops[curnode]);

				if (lateststarttime <= curstarttime) {
					lateststarttime = curstarttime;
					m = machines[i];
				}
			}
			
			backmach[m->ID] = lateststarttime;

			// Select the machine from the corresponding tool group to finish the operation the earliest
			//Machine &m = rc(pm.ops[curnode]->toolID).earliestToFinish(pm.ops[curnode]);

			//m << pm.ops[curnode];
			nodemachLr[curnode] = m;
			//nodesscheduled[curnode] = true;


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

	// Shift left all of the operations from Lr so that the earliest from them is started as soon as possible

	//out << "Operations in nodemachLr: " << endl;
	//for (int i = 0; i < nodemachLr.size(); i++) {
	//	out << *pm.ops[nodemachLr[i].first] << endl;
	//}

	//getchar();


	QList<ListDigraph::Node> keys;
	while (nodemachLr.size() > 0) {

		keys = nodemachLr.keys();

		// Collect currently available (immediately schedulable) nodes from Lr
		Lravail.clear();
		for (int i = 0; i < nodemachLr.size(); i++) {
			for (ListDigraph::InArcIt iait(pm.graph, keys[i]); iait != INVALID; ++iait) {
				if (Lv.contains(pm.graph.source(iait)) && nodesscheduled[pm.graph.source(iait)]) {
					Lravail.append(keys[i]);
					Lv.append(keys[i]);
					//Lr.removeOne(nodemachLr.keys()[i]);
					break;
				}
			}
		}

		//
		//Sort the nodes in Lravail
		qSort(Lravail.begin(), Lravail.end(), nwcg);

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
			*(nodemachLr.value(Lravail[i])) << pm.ops[Lravail[i]];
			nodesscheduled[Lravail[i]] = true;
			nodemachLr.remove(Lravail[i]);
		}

		//out << "Lravail operations after ready times update:" << endl;
		//for (int i = 0; i < Lravail.size(); i++) {
		//	out << *pm.ops[Lravail[i]] << endl;
		//}
		//getchar();
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