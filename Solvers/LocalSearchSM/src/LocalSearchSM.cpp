/* 
 * File:   LocalSearch.cpp
 * Author: DrSobik
 * 
 * Created on May 22, 2012, 10:50 AM
 */

#include <QtCore/qtextstream.h>

#include "LocalSearchSM.h"

LocalSearchSM::LocalSearchSM() {
	pm = NULL;

	//lsmode = IMPROV;

	alpha = 0.1;

	nisteps = 0;
}

LocalSearchSM::~LocalSearchSM() {
}

void LocalSearchSM::setPM(ProcessModel *pm) {
	this->pm = pm;

	/*
	QTextStream out(stdout);

	// Debug : For any node check whether there is duplicate outgoing arcs which are conjunctive and disjunctive
	ListDigraph::Node s, t;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		s = nit;
		for (ListDigraph::OutArcIt oait(pm->graph, s); oait != INVALID; ++oait) {
			t = pm->graph.target(oait);

			int duplicate = 0;

			for (ListDigraph::OutArcIt oait1(pm->graph, s); oait1 != INVALID; ++oait1) {
				if (pm->graph.target(oait1) == t) {
					duplicate++;
				}
			}

			if (duplicate > 1) {
				out << "Duplicate arc: (" << pm->ops[s]->ID << " ; " << pm->ops[t]->ID << ")" << endl;
				out << "Conjunctive path exists: " << pm->conPathExists(s, t) << endl;
				out << "Graph with duplicate arcs: " << endl;
				out << *pm << endl;
				Debugger::eDebug("LocalSearch::setPM : The resulting graph contains duplicate arcs!!!");
			}
		}
	}
	 */

	//out << *pm << endl;
	//getchar();

	// Initialize after setting the process model
	init();
}

void LocalSearchSM::init() {
	QTextStream out(stdout);

	if (pm == NULL) {
		Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with NULL process mode!" << ENDL;
	}

	terminals = pm->terminals();

	pm->updateHeads();
	pm->updateStartTimes();

	TWT obj;

	curobjimprov = obj(*pm, terminals);
	prevobjimprov = curobjimprov;
	bestobjimprov = curobjimprov;

	// Preserve the state of the schedule
	pm->save();

	out << "LS (init) : bestobj = " << curobjimprov << endl;

	alpha = 0.1;

	nisteps = 0;

	// Set the optimization mode
	//lsmode = IMPROV;
}

void LocalSearchSM::stepActions() {
	/** Algorithm:
	 * 
	 * 1. Get the terminal nodes of the graph.
	 * 2. Select the most critical in some sense terminal node.
	 * 3. Find a critical path to this terminal.
	 * 4. Find an arc to be reverted on the critical path (It may happen that 
	 *    the path contains no schedule-based conjunctive arcs).
	 * 5. Try to invert the arc.
	 * 6. Update ready times and start times of the operations of the graph 
	 *    (those that have been modified).
	 */

	QTextStream out(stdout);

	// Get the terminals
	QList<ListDigraph::Node> terminals = pm->terminals();

	ListDigraph::Node theterminal;
	Path<ListDigraph> cpath;
	ListDigraph::Arc carc;
	int maxcarcreselections = 100;
	int arcreselections = 0;

    //bool contributing = true;

	pm->updateHeads();
	pm->updateStartTimes();

	do {

		//switch (lsmode) {
		//	case IMPROV: theterminal = selectTerminalContrib(terminals);
		//		break;
		//	case INTERM: theterminal = selectTerminalRnd(terminals);
		//		break;
		//}
		//if (Math::rndDouble() > 0.15) {
		//theterminal = selectTerminalContrib(terminals);
		//} else {
		theterminal = selectTerminalContrib(terminals);
		//}

		//	theterminal = selectTerminalRnd(terminals);

		// Select some terminal for the manipulations (based on the contribution of the terminal to the objective)
		//if (arcreselections > maxcarcreselections / 2) {
		//theterminal = selectTerminalRnd(terminals);
		//} else {
		//	theterminal = selectTerminalContrib(terminals);
		//}

		// Find a critical path to the selected terminal
		cpath = longestPath(theterminal);

		// Get the arc to be reverted
		carc = selectArcToRevert(cpath);

		arcreselections++;

		if (arcreselections > maxcarcreselections) {
			//Debugger::warn << "LocalSearch::stepActions : Failed to find reversible schedule-based arcs!" << ENDL;
			//Debugger::warn << "LocalSearch::stepActions : Considering non-contributing nodes arcs!" << ENDL;
            //contributing = false;
			arcreselections = 0;
			//break;
		}

	} while (carc == INVALID);

	if (carc != INVALID) {
		// Reverse the selected arc
		//out << "Graph before reversing an arc:" << endl;
		//out << *pm << endl;
		//pm->updateHeads();
		//pm->updateStartTimes();

		//out << "Reversing arc ..." << endl;
		//out << pm->ops[pm->graph.source(carc)]->OID << ":" << pm->ops[pm->graph.source(carc)]->ID << " -> " << pm->ops[pm->graph.target(carc)]->OID << ":" << pm->ops[pm->graph.target(carc)]->ID << endl;

		if (!dag(pm->graph)) {
			Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains cycles before reverting a critical arc!!!");
		} else {
			//Debugger::info<<"The resulting graph is DAG before reverting a critical arc!!!"<<ENDL;
		}

		/*
		// Debug : For any node check whether there is duplicate outgoing arcs which are conjunctive and disjunctive
		ListDigraph::Node s, t;
		for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
			s = nit;
			for (ListDigraph::OutArcIt oait(pm->graph, s); oait != INVALID; ++oait) {
				t = pm->graph.target(oait);

				int duplicate = 0;

				for (ListDigraph::OutArcIt oait1(pm->graph, s); oait1 != INVALID; ++oait1) {
					if (pm->graph.target(oait1) == t) {
						duplicate++;
					}
				}

				if (duplicate > 1) {
					out << "Duplicate arc: (" << pm->ops[s]->ID << " ; " << pm->ops[t]->ID << ")" << endl;
					out << "Conjunctive path exists: " << pm->conPathExists(s, t) << endl;
					out << "Graph with duplicate arcs: " << endl;
					out << *pm << endl;
					Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains duplicate arcs before reversing an arc!!!");
				}
			}
		}
		 */

		//out << "Critical arc: (" << pm->ops[pm->graph.source(carc)]->ID << " ; " << pm->ops[pm->graph.target(carc)]->ID << ")" << endl;
		//out << "Graph before reversing the critical arc: " << endl;
		//out << *pm << endl;

		reversed = reverseArc(carc);

		/*
		// Debug : For any node check whether there is duplicate outgoing arcs which are conjunctive and disjunctive
		for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
			s = nit;
			for (ListDigraph::OutArcIt oait(pm->graph, s); oait != INVALID; ++oait) {
				t = pm->graph.target(oait);

				int duplicate = 0;

				for (ListDigraph::OutArcIt oait1(pm->graph, s); oait1 != INVALID; ++oait1) {
					if (pm->graph.target(oait1) == t) {
						duplicate++;
					}
				}

				if (duplicate > 1) {
					out << "Duplicate arc: (" << pm->ops[s]->ID << " ; " << pm->ops[t]->ID << ")" << endl;
					out << "Reversed arc: (" << pm->ops[pm->graph.target(reversed)]->ID << " ; " << pm->ops[pm->graph.source(reversed)]->ID << ")" << endl;
					out << "Conjunctive path exists: " << pm->conPathExists(s, t) << endl;
					out << "Graph with duplicate arcs: " << endl;
					out << *pm << endl;
					Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains duplicate arcs after reversing an arc!!!");
				}
			}
		}
		 */

		// Check whether no cycles occur
		if (!dag(pm->graph)) {
			out << "Reversed arc: (" << pm->ops[pm->graph.source(reversed)]->ID << " ; " << pm->ops[pm->graph.target(reversed)]->ID << ")" << endl;
			out << "Graph after reversing the critical arc: " << endl;
			out << *pm << endl;
			out << "Conjunctive path exists: " << pm->conPathExists(pm->graph.target(reversed), pm->graph.source(reversed)) << endl;
			Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains cycles after reverting a critical arc!!!");
		} else {
			//Debugger::info << "The resulting graph is acyclic :)." << ENDL;
		}

		//out << "Done reversing arc." << endl;

		//out << "Graph after reversing an arc:" << endl;
		//out << *pm << endl;

		// Update the ready times and the start times of the operations in the graph
		pm->updateHeads();
		pm->updateStartTimes();
	} else {
		Debugger::err << "LocalSearch::stepActions : Trying to reverse invalid arc!" << ENDL;
	}

	//out << "Finished one step of the local search." << endl;
}

void LocalSearchSM::assessActions() {
	QTextStream out(stdout);

	//out << "Assessing the step of the LS..." << endl;
	curobjimprov = objimprov(*pm, pm->terminals());


	/*
	if (lsmode == INTERM) {
		curobjinterm = objinterm(*pm, pm->terminals());
	}
	 */


	//out << "Done assessing the step of the LS." << endl;
}

bool LocalSearchSM::acceptCondition() {
	// With probability alpha we accept the the worser solution
	//double alpha = 0.05;


	//if (iter() > 0) alpha = 0.05;
	//if (iter() > 30000) alpha = 0.04;


	if (iter() == 50000) alpha = 0.05;
	if (iter() == 100000) alpha = 0.05;
	if (iter() == 150000) alpha = 0.05;
	if (iter() == 200000) alpha = 0.05;
	if (iter() == 250000) alpha = 0.05;

	//if (nisteps / 10000 == 1) alpha = 0.05;
	//if (nisteps / 10000 == 2) alpha = 0.1;
	//if (nisteps / 10000 == 3) alpha = 0.2;

	//alpha = 0.1;



	//if (lsmode == IMPROV) {
	if (curobjimprov <= prevobjimprov /*bestobjimprov*/) {
		acceptedworse = false;
		return true;
	} else {
		//if (Rand::rndDouble(0.0, 1.0) < alpha) {
		if (Rand::rnd<double>(0.0, 1.0) < alpha) {
			acceptedworse = true;
			return true;
		} else {
			acceptedworse = false;
			return false;
		}
	}
	//}


	/*
	if (lsmode == INTERM) {
		if (curobjimprov <= bestobjimprov && curobjinterm < bestobjinterm) {
			return true;
		} else {
			return false;
		}
	}
	 */

}

void LocalSearchSM::acceptActions() {
	QTextStream out(stdout);

	if (acceptedworse || curobjimprov == bestobjimprov) {
		nisteps++;
	} else {
		if (curobjimprov < bestobjimprov) {
			nisteps = 0;
		}
	}

	//if (lsmode == IMPROV) {
	// If a better solution is found then the state of the process model should be preserved!!!
	if (curobjimprov <= bestobjimprov) {
		bestobjimprov = curobjimprov;

		// Preserve the state of the process model
		pm->save();

		out << "LS (" << iter() << ") : bestobj = " << bestobjimprov << endl;
	}

	prevobjimprov = curobjimprov;
	//}

	/*	
	if (lsmode == INTERM) {
		// If the primary criterion is improved => set improvement search mode
		if (curobjimprov < bestobjimprov) {

			bestobjimprov = curobjimprov;

			// Preserve the state of the process model
			pm->save();

			out << "LS (" << iter() << ") : bestobj = " << bestobjimprov << ",  bestsecondobj = " << bestobjinterm << endl;

			prevobjimprov = curobjimprov;

			// Switch back to the IMPROV mode
			lsmode = IMPROV;
			out << "Switched to IMPROV mode" << endl;

		} else { // Accept the new solution with the improved secondary objective
			// Preserve the state of the process model
			pm->save();

			out << "LS (" << iter() << ") : bestobj = " << bestobjimprov << ",  bestsecondobj = " << bestobjinterm << endl;

			bestobjinterm = curobjinterm;
		}
	}
	 */


	//out << "The step has been accepted." << endl;
}

void LocalSearchSM::declineActions() {
	QTextStream out(stdout);

	//TL objinterm;

	//if (acceptedworse) {
	//	prevobjimprov = curobjimprov;
	//}

	//switch (lsmode) {

	//case IMPROV:
	//{
	//	if (iterDecl() > 10000) { // Local minimum of the primary criterion found
	//		lsmode = INTERM;
	//		out << "Switched to INTERM mode" << endl;

	//		pm->restore(); // Restore the local optimum graph

	//		bestobjinterm = objinterm(*pm, pm->terminals());
	//		curobjinterm = bestobjinterm;

	//	} else {
	reversed = reverseArc(reversed);

	pm->updateHeads();
	pm->updateStartTimes();
	//	}
	//};
	//	break;



	//case INTERM:
	//{

	//	reversed = reverseArc(reversed);

	//	pm->updateHeads();
	//	pm->updateStartTimes();

	//};
	//break;


	//}


	// Preform diversification

	//if (lsmode == INTERM) {


	//if (iter() > 250000) {
	//	alpha = Math::min(alpha + 0.000001, 0.1);
	//out << "Alpha = " << alpha << endl;
	//}

	nisteps++;

	if (nisteps > 100000) {
		out << "Diversifying (nisteps)..." << endl;
		diversify();
	}

	if (((iter() + 1) % 50000 == 0)) {
		out << "Diversifying..." << endl;
		//out << "Declined at iteration : " << iterDecl() << endl;
		diversify();

		//alpha = 0.07;
		//curobjinterm = objinterm(*pm, terminals);
		//prevobjinterm = curobjinterm;

		//lsmode = IMPROV;
		//}
	}

}

bool LocalSearchSM::stopCondition() {
	return (curobjimprov <= objimprov.LB(*pm)) || IterativeAlg::stopCondition();
}

void LocalSearchSM::stopActions() {
	QTextStream out(stdout);
	//Debugger::info << "LocalSearch::stopActions : Found local optimum with objective " << curobj << ENDL;

	//out << "                  " << endl;
	//getchar();
}

void LocalSearchSM::preprocessingActions() {

}

void LocalSearchSM::postprocessingActions() {
	// Restore the state corresponding to the best found value of the objective
	pm->restore();

}

Path<ListDigraph> LocalSearchSM::longestPath(const ListDigraph::Node & node) {
	Path<ListDigraph> res;

	BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm->graph, pm->p);

	bf.init();
	bf.addSource(pm->head);
	//Debugger::info << "Running the BF algorithm..."<<ENDL;
	bf.start();
	//Debugger::info << "Done running the BF algorithm."<<ENDL;

#ifdef DEBUG
	if (!bf.reached(node)) {
		Debugger::err << "LocalSearch::longestPath : Operation ID= " << pm->ops[node]->OID << ":" << pm->ops[node]->ID << " can not be reached from the root node " << pm->ops[pm->head]->OID << ":" << pm->ops[pm->head]->ID << "!" << ENDL;
	}
#endif

	res = bf.path(node);
	return res;

}

ListDigraph::Arc LocalSearchSM::selectArcToRevert(const Path<ListDigraph> &cpath) {
	// Select only those arcs which are schedule based and NOT conjunctive

	int n = cpath.length();

	QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs
	for (int i = 0; i < n; i++) {
		if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based
			schedbased.append(cpath.nth(i));
		}
	}

	if (schedbased.size() == 0) return INVALID;

	//return schedbased[Rand::rndInt(0, schedbased.size() - 1)];
	return schedbased[Rand::rnd<Math::uint32>(0, schedbased.size() - 1)];
}

ListDigraph::Arc LocalSearchSM::reverseArc(const ListDigraph::Arc & carc) {
	// Revert the arc in the graph. If the arc is critical there should appear no cycles

	/* IMPORTANT!!! When reversing some critical arc it may appear that 
	 * schedule-based arcs will be implied for nodes between which a conjunctive 
	 * path of non-schedule-based arcs exists. This may lead to trivial cycles
	 * during the following steps when such arc is recognized to be critical. 
	 * Checking whether there exists a conjunctive path between two nodes can be 
	 * efficiently defined at the beginning of the local search process. However,
	 * if it is assumed that there are no zero-weighted arcs in the graph then it
	 * is enough to avoid arc duplication (where one arc is conjunctive and the
	 * parallel one is schedule-based).
	 *  */

	/** Algorithm:
	 * 
	 * Assuming that an arc (u,v) is being reverted
	 * 
	 * 1. Get the incoming schedule-based arcs for u.
	 * 1.1. Set the target node of the found arcs to be v.
	 * 1.2. The length of the arcs are not changed, provided no initial ready 
	 *		times are considered.
	 * 
	 * 2. Get the outgoing schedule-based arcs for v.
	 * 2.1. Set u to be the source node of the found arcs
	 * 2.2. Update the lengths of the arcs.
	 * 
	 * 3. Revert the arc (u,v) and update its processing time
	 *  
	 */

	ListDigraph::Node u = pm->graph.source(carc);
	ListDigraph::Node v = pm->graph.target(carc);

	QList<ListDigraph::Arc> uinarcs;
	QList<ListDigraph::Arc> voutarcs;

	// Find incoming schedule-based arcs for u
	for (ListDigraph::InArcIt iait(pm->graph, u); iait != INVALID; ++iait) {
		if (!pm->conjunctive[iait]) { // The arc is schedule-based
			uinarcs.append(iait);
		}
	}

	// Find outgoing schedule-based arcs for v
	for (ListDigraph::OutArcIt oait(pm->graph, v); oait != INVALID; ++oait) {
		if (!pm->conjunctive[oait]) { // The arc is schedule-based
			voutarcs.append(oait);
		}
	}

	// Change the target nodes of the arcs that come into u. Do not touch the lengths
	for (int i = 0; i < uinarcs.size(); i++) {
		if (!pm->conPathExists(pm->graph.source(uinarcs[i]), v)) { // Insert a schedule-based arc only if there is no conjunctive path between the nodes
			pm->graph.changeTarget(uinarcs[i], v);
		} else {
			pm->graph.erase(uinarcs[i]);
		}
	}

	// Change the source nodes of the arcs that go out from v. The length of the arcs must equal to the processing times of the operations
	for (int i = 0; i < voutarcs.size(); i++) {
		if (!pm->conPathExists(u, pm->graph.target(voutarcs[i]))) { // Insert a schedule-based arc only if there is no conjunctive path between the nodes
			// Change source node of the arc
			pm->graph.changeSource(voutarcs[i], u);
			// Update length of the arc
			pm->p[voutarcs[i]] = -pm->ops[u]->p();
		} else {
			pm->graph.erase(voutarcs[i]);
		}
	}

	// Revert the arc (u,v)
	pm->graph.reverseArc(carc);

	// Update the length of the reversed arc (processing time of operation v)
	pm->p[carc] = -pm->ops[v]->p();

	// Return the reversed arc
	return carc;
}

ListDigraph::Node LocalSearchSM::selectTerminalContrib(QList<ListDigraph::Node> &terminals) {
	// The probability that some terminal will be selected should be proportional to its contribution to the objective of the partial schedule 
	/** Algorithm:
	 * 
	 * 1. Calculate the summ of the weighted tardinesses of the terminal nodes.
	 * 2. For every terminal node assign a subinterval which corresponds to 
	 *    the contribution of the terminal to the objective of the partial 
	 *    schedule.
	 * 3. Choose an arbitrary number from [0, TWT] and find the subinterval 
	 *    which contains this number. Return the corresponding terminal node.
	 * 
	 */

	QList<QPair<QPair<double, double>, ListDigraph::Node > > interval2node;
	double totalobj = 0.0;
	double istart;
	double iend;
	ListDigraph::Node res = INVALID;

	for (int i = 0; i < terminals.size(); i++) {
		istart = totalobj;
		totalobj += 1.0; //*/pm->ops[terminals[i]]->wT();
		iend = totalobj;

		interval2node.append(QPair<QPair<double, double>, ListDigraph::Node > (QPair<double, double>(istart, iend), terminals[i]));
	}

	// Choose an arbitrary number
	//double arbnum = Rand::rndDouble(0.0, totalobj);
	double arbnum = Rand::rnd<double>(0.0, totalobj);

	// Find an interval that contains the generated number
	for (int i = 0; i < interval2node.size(); i++) {
		if (interval2node[i].first.first <= arbnum && arbnum <= interval2node[i].first.second) {
			res = interval2node[i].second;
			break;
		}
	}

	return res;
}

ListDigraph::Node LocalSearchSM::selectTerminalRnd(QList<ListDigraph::Node> &terminals) {
	//return terminals[Rand::rndInt(0, terminals.size() - 1)];
	return terminals[Rand::rnd<Math::uint32>(0, terminals.size() - 1)];
}

void LocalSearchSM::diversify() {
	/** Algorithm:
	 * 
	 * 1. Select the random number of arcs to be reversed
	 * 
	 * 2. Reverse randomly the selected number of critical arcs 
	 * 
	 */

	pm->restore();
	pm->updateHeads();
	pm->updateStartTimes();
	curobjimprov = objimprov(*pm, pm->terminals());

	//int narcs2reverse = Rand::rndInt(5, 8);
	int narcs2reverse = Rand::rnd<Math::uint32>(5, 8);
	int narcsreversed = 0;

	// Get the terminals
	QList<ListDigraph::Node> terminals = pm->terminals();

	ListDigraph::Node theterminal;
	Path<ListDigraph> cpath;
	ListDigraph::Arc carc;
	int maxcarcreselections = 10000;
	int arcreselections = 0;

	do {

		do {
			// Select some terminal for the manipulations (based on the contribution of the terminal to the objective)

			theterminal = selectTerminalRnd(terminals);

			// Find a critical path to the selected terminal
			cpath = longestPath(theterminal);

			// Get the arc to be reverted
			carc = selectArcToRevert(cpath);

			arcreselections++;

			if (arcreselections > maxcarcreselections) {
				Debugger::err << "LocalSearch::diversify : Failed to find reversible schedule-based arcs!" << ENDL;
			}

		} while (carc == INVALID);

		// Reverse the selected arc
		reverseArc(carc);

		// Update the ready times and the start times of the operations in the graph
		pm->updateHeads();
		pm->updateStartTimes();

		narcsreversed++;

		if (objimprov(*pm, pm->terminals()) < bestobjimprov) {
			pm->save();
			curobjimprov = objimprov(*pm, pm->terminals());
			prevobjimprov = curobjimprov;
			bestobjimprov = curobjimprov;
			nisteps = 0;
			break;
		}

	} while (/*objimprov(*pm, pm->terminals()) > bestobjimprov &&*/ narcsreversed < narcs2reverse);

	//if (objimprov(*pm, pm->terminals()) < bestobjimprov) {
	//pm->save();
	curobjimprov = objimprov(*pm, pm->terminals());
	prevobjimprov = curobjimprov - 0.00000000001;
	nisteps = 0;
	//bestobjimprov = Math::MAX_DOUBLE;
	//prevobjimprov = Math::MAX_DOUBLE;
	//}


}
