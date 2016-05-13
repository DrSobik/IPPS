/* 
 * File:   TGVNSScheduler.cpp
 * Author: DrSobik
 * 
 * Created on February 27, 2012, 4:40 PM
 */

#include "TGVNSScheduler.h"

/**  ************************  TGSchedulerLS  *****************************  **/

TGSchedulerLS::TGSchedulerLS() : iniScheduler(NULL) {
	init();
}

TGSchedulerLS::TGSchedulerLS(TGSchedulerLS& other) : TGScheduler(other), ls(other.ls) {

	this->iniScheduler = (TGScheduler*) other.iniScheduler->clone();

}

TGSchedulerLS::~TGSchedulerLS() {
	if (iniScheduler != NULL) delete iniScheduler;
}

void TGSchedulerLS::init() { }

void TGSchedulerLS::clear() {
	if (iniScheduler != NULL) delete iniScheduler;

	iniScheduler = NULL;
}

Clonable* TGSchedulerLS::clone() {
	return new TGSchedulerLS(*this);
}

void TGSchedulerLS::run() {

	// Mark the nodes which can be moved during the local search
	QMap < ListDigraph::Node, bool> nodeMovable;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm->ops[curNode]);

		if (curOp.toolID == tg->ID) { // This node can be moved

			nodeMovable[curNode] = true;

		} else { // This node can not be moved

			nodeMovable[curNode] = false;

		}

	}

	// Get the initial solution for the subproblem
	initialAssignment(*pm, *tg, *opnodes);

	Debugger::info << ENDL << "TGSchedulerLS::run : Initial objective by TGSchedulerLS (partial schedule, TWT (exp. LB), without assignment) : " << TWT()(*pm) << ENDL;

	TGSelection& selection = (TGSelection&) * tgselection;

#ifdef DEBUG

	insertSelection(selection);
	Debugger::info << "TGSchedulerLS::run : Initial objective in tgselection.localobj : " << tgselection->localobj << ENDL;
	Debugger::info << "TGSchedulerLS::run : Global objective after initial assignment (recalculated, TWT, with selection, before LS) : " << TWT()(*pm) << ENDL;
	removeSelection();

#endif 

	// Save the state of the PM (without the initial selection!!!)
	QMap<ListDigraph::Node, Operation> savedOps; // Saved operation data
	//QList<QPair<ListDigraph::Node, ListDigraph::Node> > savedArcs; // Arcs of the graph which were saved previously
	QList<double> savedP; // Arc lengths of the previously saved arcs
	//QList<bool> savedConjunctive; // Indicator of conjunctiveness of the previously saved arcs
	QMap < ListDigraph::Arc, bool> savedConjunctive; // Used to momorize which arc is 

	// Preserve the information about the operations
	savedOps.clear();
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) * pm->ops[curNode];

		savedOps[curNode] = curOp;
	}

	// Preserve the information about the arcs for the current state of the PM
	//savedArcs.clear();
	savedP.clear();
	//savedConjunctive.clear();
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) { // IMPORTANT!!! DO NOT TOUCH THE ARCS WHICH ARE ALREADY IN THE PM, OTHERWISE DELETING AND RESTORING THEM WILL LEAD TO INVALID ARCS IN THE PM -> PROBLEMS!!!
		ListDigraph::Arc curArc = ait;
		//ListDigraph::Node curStartNode = pm->graph.source(curArc);
		//ListDigraph::Node curEndNode = pm->graph.target(curArc);

		//savedArcs.append(QPair<ListDigraph::Node, ListDigraph::Node>(curStartNode, curEndNode));

		savedP.append(pm->p[curArc]);

		//savedConjunctive.append(pm->conjunctive[curArc]);

		savedConjunctive[curArc] = pm->conjunctive[curArc];

		// Mark all arcs of the current PM as conjunctive so that the LS doesn't modify them preserving their validness
		pm->conjunctive[curArc] = true;
	}


	// Perform scheduling
	insertSelection(selection); // Inserting the INITIAL selection of the initial solution in order to improve it later (IT WILL BE MODIFIED THROUGH THE LS)

	// Create the resources for the local search
	Resources rc;

	rc.ID = 1;
	rc << new ToolGroup(*tg);

	ls.setPM(pm); // IMPORTANT!!! The PM will change during the LS - > the information should be preserved
	ls.setResources(&rc);
	ls.setMovableNodes(nodeMovable);
	ls.checkCorrectness(false);
	//ls.maxIter(2000); // Is defined by SBH_TG_LS_MAX_ITER
	//ls.maxTimeMs(60 * 1000);

	ls.run();

	Debugger::info << "TGSchedulerLS::run : Global objective in PM after the LS (recalculated, TWT, state after LS) : " << TWT()(*pm) << ENDL;

	// Update the selection according to the new solution -> the old selection is not appropriate
	getTGSelection();

	// Restore the state of the PM as before the LS (without the current selection!!!)

	// Remove all disjunctive arcs in the graph dealing ONLY with this machine group
	QList<ListDigraph::Arc> arcsToRem;
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		ListDigraph::Node curStartNode = pm->graph.source(ait);
		Operation& curStartOp = (Operation&) *(pm->ops[curStartNode]);

		if (!pm->conjunctive[ait] && curStartOp.toolID == tg->ID) arcsToRem.append(curArc);
	}

	for (int i = 0; i < arcsToRem.size(); i++) {
		ListDigraph::Arc curArc = arcsToRem[i];
		pm->graph.erase(curArc);
	}

	// Restore the information about the operations
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		*(pm->ops[curNode]) = savedOps[curNode];
	}

	// Restore the lengths of all arcs
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		double curP = pm->ops[curNode]->p();
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			pm->p[curArc] = -curP;
		}
	}

	// Restore the conjunctiveness of the arcs
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		pm->conjunctive[curArc] = savedConjunctive[curArc];
	}


	Debugger::info << "TGSchedulerLS::run : Global objective in PM after the LS (recalculated, TWT, no selection, after LS) : " << TWT()(*pm) << ENDL;
	insertSelection(selection);
	Debugger::info << "TGSchedulerLS::run : Global objective in PM after the LS (recalculated, TWT, with selection, after LS) : " << TWT()(*pm) << ENDL;
	removeSelection();

	// Update the objective in the selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	Debugger::info << "TGSchedulerLS::run : Global objective in tgselection after the LS : " << tgselection->localobj << ENDL;

	Debugger::info << "TGSchedulerLS::run : Global objective without the selection -> PM will be used in other SSPs : " << TWT()(*pm) << ENDL;

}

void TGSchedulerLS::schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection) {
	if (opnodes.size() == 0) return;

	init();

	iniScheduler->node2predST = this->node2predST;

	TGScheduler::schedule(pm, tg, opnodes, terminals, dloc, tgselection);
}

/** Apply ATC rule in order to get the initial assignment for VNS. */
void TGSchedulerLS::initialAssignment(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes) {
	// Preserve the initial process model
	//sbhscheduler->preserveOpSchedState(pm, opnodes);

	QTextStream out(stdout);

	QList<ListDigraph::Node> ts = pm.topolSort();

	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *pm.ops[curNode];
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm.conjunctive[curArc];
		arc2PrevP[curArc] = pm.p[curArc];
	}


	//Debugger::info << "Running the iniScheduler scheduler..." << ENDL;
	iniScheduler->schedule(pm, tg, opnodes, *terminals, *dloc, *tgselection);
	//Debugger::info << "Done running the iniScheduler scheduler." << ENDL;  	


	// The selection is found automatically

	int n_selection_nodes = tgselection->opNode2SchedOps.size();
	//machID2opnodes.clear();
	//machID2opnodes = assignmentFromSelection(*tgselection);

	/*
	for (int i = 0; i < tgselection->selection.size(); i++) {
		//Debugger::info << "(" << pm.ops[tgselection->selection[i].first]->ID << ";" << pm.ops[tgselection->selection[i].second]->ID << ")" << ENDL;
		if (!machID2opnodes[pm.ops[tgselection->selection[i].first]->machID].contains(tgselection->selection[i].first)) {
			n_selection_nodes++;
			machID2opnodes[pm.ops[tgselection->selection[i].first]->machID].append(tgselection->selection[i].first);

			//Debugger::warn << pm.ops[tgselection->selection[i].first]->ID << ENDL;
		}

		if (!machID2opnodes[pm.ops[tgselection->selection[i].second]->machID].contains(tgselection->selection[i].second)) {
			n_selection_nodes++;
			machID2opnodes[pm.ops[tgselection->selection[i].second]->machID].append(tgselection->selection[i].second);

			//Debugger::warn << pm.ops[tgselection->selection[i].second]->ID << ENDL;
		}
	}
	 */

	if (n_selection_nodes != opnodes.size()) {
		Debugger::warn << " Problem during the initial assignment! " << ENDL;
		Debugger::warn << " Present nodes: " << opnodes.size() << ENDL;
		for (int i = 0; i < opnodes.size(); i++) {
			Debugger::warn << pm.ops[opnodes[i]]->ID << ENDL;
		}
		Debugger::warn << " Assigned nodes: " << n_selection_nodes << ENDL;
		Debugger::err << "Failure during the initial assignment!" << ENDL;
	}


	// DEBUG

	insertSelection(*tgselection);
	cout << "TGVNSScheduler::initialAssignment : TWT before restoring the PM's state : " << TWT()(pm) << endl;
	removeSelection();

	// END DEBUG


	// Restore the operation data
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		*pm.ops[curNode] = node2PrevOper[curNode];
	}

	// Restore the arcs data
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm.conjunctive[curArc] = arc2PrevConj[curArc];
		pm.p[curArc] = arc2PrevP[curArc];
	}



	// DEBUG

	insertSelection(*tgselection);
	cout << "TGVNSScheduler::initialAssignment : TWT after restoring the PM's state and inserting the selection : " << TWT()(pm) << endl;
	removeSelection();
	cout << "TGVNSScheduler::initialAssignment : TWT after removing the selection : " << TWT()(pm) << endl;

	// END DEBUG



	// Restore the initial scheduling state of the affected operations
	//sbhscheduler->restoreOpSchedState(pm, opnodes);
}

double TGSchedulerLS::localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node> &, QHash<int, QList<double> > &) {
	QTextStream out(stdout);

	// Insert the current selection for the tool group
	//double localobj = 0.0;

	//SBHTWTLocalObj lobj;
	UTWT utwt;
	double res = utwt(pm, opnodes, locD); //localobj = lobj(pm, opnodes, *tgselection, terminals, dloc);

	TWT twt;

	// IMPORTANT!!! It is assumed that the correct selection has already been found

	TGSelection& selection = (TGSelection&) tgselection;

	//int curTGID = selection.tgID;
	//cout << "TGWEODScheduler::localObj : Calculating objective for TG : " << curTGID << endl;
	//cout << "Arcs in the selection : " << selection.selection.size() << endl;

	insertSelection(selection);

	//out << pm << endl;

	res = twt(pm); //*/ utwt(pm, opnodes, locD);

	removeSelection();

	//cout << "TWT : " << res << endl;

	return res;
}

/**  **********************************************************************  **/

/**  ************************  TGVNSScheduler  ****************************  **/

TGVNSScheduler::TGVNSScheduler() {
	init();
}

TGVNSScheduler::TGVNSScheduler(TGVNSScheduler& orig) : TGScheduler(orig), IterativeAlg(orig) {
	this->kmax = orig.kmax;
	this->k = orig.k;

	this->iniScheduler = (TGScheduler*) orig.iniScheduler->clone();
}

TGVNSScheduler::~TGVNSScheduler() {
	if (prev_nodes != NULL) delete prev_nodes;
}

Clonable* TGVNSScheduler::clone() {
	return new TGVNSScheduler(*this);
}

void TGVNSScheduler::init() {
	prev_obj = Math::MAX_DOUBLE;
	cur_obj = Math::MAX_DOUBLE;
	best_obj = Math::MAX_DOUBLE;

	prev_nodes = new QList<ListDigraph::Node>;

	// Set the current degree of the neighborhood
	k = 1;

	kmax = 5;

	machID2opnodes.clear();

	tabus.clear();

}

void TGVNSScheduler::stepActions() {
	//Debugger::info << "Performing step actions ..." << ENDL;
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

	QTextStream out(stdout);

	// Preserve the current sequences of operations on the machines
	prev_machID2opnodes = machID2opnodes;

	// Preserve the current tool group selection before the move
	prev_tgselection = *tgselection;

	// Remove the selection from the graph
	removeSelection();

	// Perform the shaking step of the algorithm
	//Debugger::info << "Shaking ..." << ENDL;
	shake();
	//Debugger::info << "Done shaking." << ENDL;

	// Schedule the newly found sequence of the operations using "list scheduling"
	//Debugger::info << "Scheduling the current operation sequences ..." << ENDL;
	scheduleCurOpSeq();

	insertSelection(*tgselection);
	out << "TGVNSScheduler::stepActions : TWT before the local search : " << TWT()(*pm) << endl;
	removeSelection();


	//Debugger::info << "Done scheduling the current operation sequences." << ENDL;

	//getchar();
	//Debugger::info << "Done running tool group scheduling algorithm." << ENDL;

	// IMPORTANT!!! The selection for the new schedule of the tool group must be inserted into the graph in order to evaluate the solution

	// Perform local search in hope to find some better selection
	//Debugger::info << "Performing VNS local search ..." << ENDL;
	if (iter() % 1 == 0) {
		localSearchLS();
		//localSearch();
		//localSearchSimple();
	}
	//Debugger::info << "Done performing VNS local search." << ENDL;
}

void TGVNSScheduler::assessActions() {
	prev_obj = cur_obj;

	cur_obj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	tgselection->localobj = cur_obj;

	//Debugger::info << "Assessing: cur_obj= " << cur_obj << " prev_obj= " << prev_obj << ENDL;
	//getchar();
}

bool TGVNSScheduler::acceptCondition() {

	if (cur_obj < prev_obj) {

		acceptedWorse = false;

		return true;

	} else {

		if (cur_obj <= 1.05 * best_obj) {

			if (Rand::rnd<double>() <= 0.05) {

				acceptedWorse = true;

				return true;

			} else {

				acceptedWorse = false;

				return false;

			}

		} else {

			acceptedWorse = false;

			return false;

		}

	}

}

void TGVNSScheduler::acceptActions() {
	if (!acceptedWorse) {
		Debugger::info << "Iter : " << (int) iter() << " : ";
		Debugger::info << "TGVNSScheduler::acceptActions : Accepting solution with objective : " << cur_obj << ENDL;
		Debugger::info << "TGVNSScheduler::acceptActions : Best solution with objective : " << best_obj << ENDL;
		//Debugger::info << "TGVNSScheduler::acceptActions : Previous objective : " << prev_obj << ENDL;
		//getchar();

		// Update the assignment
		machID2opnodes = assignmentFromSelection(*tgselection);

		// Save the best assignment
		bestID2opnodes = machID2opnodes;

		bestTGSelection = *tgselection;

		best_obj = cur_obj;

	} else { // Accepted a worse solution

		Debugger::info << "Iter : " << (int) iter() << " : ";
		Debugger::info << "TGVNSScheduler::acceptActions : Accepting a WORSE solution with objective : " << cur_obj << ENDL;
		Debugger::info << "TGVNSScheduler::acceptActions : Best solution with objective : " << best_obj << ENDL;
		//Debugger::info << "TGVNSScheduler::acceptActions : Previous objective : " << prev_obj << ENDL;
		//getchar();

		// Update the assignment

		machID2opnodes = assignmentFromSelection(*tgselection);

		// Restore the best found objective
		cur_obj = best_obj;

	}

	k = 1;

	// IMPORTANT!!! Selection is already preserved

	//Debugger::info << "Assessing: cur_obj= " << cur_obj << ENDL;
	//getchar();

	// Restore the initial values of the ready times of the nodes
	//restoreRs();
}

void TGVNSScheduler::declineActions() {
	Debugger::info << "TGVNSScheduler::declineActions : Declining solution with objective : " << cur_obj << ENDL;
	Debugger::info << "TGVNSScheduler::declineActions : Previous objective : " << prev_obj << ENDL;
	Debugger::info << "TGVNSScheduler::declineActions : Best solution with objective : " << best_obj << ENDL;
	//getchar();

	// Set the previous objective
	cur_obj = prev_obj;

	// Restore the previous sequence of the operations on the machines
	machID2opnodes = prev_machID2opnodes;

	// Restore the initial ready times of the nodes
	//restoreRs();

	// Update k
	k = k + 1;
	if (k > kmax) k = Math::max(kmax / 2, 1); //(1 + kmax) / 2;

	// IMPORTANT!!! Restore the previous selection
	*tgselection = prev_tgselection;
}

bool TGVNSScheduler::stopCondition() {
	//Debugger::info << "TGVNSScheduler:: stopCondition()"<<ENDL;
	return (Math::cmp(cur_obj, 0.0, 0.0001) == 0) || IterativeAlg::stopCondition();
}

void TGVNSScheduler::stopActions() {
	//Debugger::info << "TGVNSScheduler::stopActions()" << ENDL;
}

void TGVNSScheduler::preprocessingActions() {
	// Preserve the initial values of the ready times
	//preserveRs();
}

void TGVNSScheduler::postprocessingActions() {
	//Debugger::info << "TGVNSScheduler::postprocessingActions()" << ENDL;

	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	QTextStream out(stdout);
	out << "Checking consistency before TGVNSScheduler::postprocessingActions..." << endl;
	for (ListDigraph::NodeIt nit(this->pm->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(this->pm->graph, nit); oait != INVALID; ++oait) {
			if (this->pm->ops[nit]->p() != -this->pm->p[oait]) {
				out << "op ID = " << this->pm->ops[nit]->ID << endl;
				out << *this->pm << endl;
				Debugger::err << "TGVNSScheduler::postprocessingActions : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	// Schedule the operation nodes for the correct generation of the selection
	//Debugger::info << "TGVNSScheduler::postprocessingActions() scheduleCurOpSeq..." << ENDL;
	//scheduleCurOpSeq();

	//###########################  DEBUG  ######################################
	// For every node check whether processing times of the operation equals the length of the arc
	/*
	out << "Checking consistency after TGVNSScheduler::postprocessingActions..." << endl;
	for (ListDigraph::NodeIt nit(this->pm->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(this->pm->graph, nit); oait != INVALID; ++oait) {
			if (this->pm->ops[nit]->p() != -this->pm->p[oait]) {
				out << "op ID = " << this->pm->ops[nit]->ID << endl;
				out << *this->pm << endl;
				Debugger::err << "TGVNSScheduler::postprocessingActions : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	//Debugger::info << "TGVNSScheduler::postprocessingActions() DONE scheduleCurOpSeq." << ENDL;


	// Generate the tool group selection
	//getTGSelection();

	// Calculate the local objective for the current selection
	//tgselection->localobj = cur_obj;

	//#####################  DEBUG  ############################################
	/*
	QTextStream out(stdout);

	out << "TGVNSScheduler::postprocessingActions : Selection:" << endl;
	for (int i = 0; i < tgselection->selection.size(); i++) {
		out << *pm->ops[tgselection->selection[i].first] << endl;
	}

	double pt;
	ListDigraph::Node s;
	out << "TGVNSScheduler::postprocessingActions : Checking processing times..." << endl;
	for (int i = 0; i < tgselection->selection.size(); i++) {
		s = tgselection->selection[i].first;
		if (pm->ops[s]->machID >= 0) {
			pt = ((*tg)(pm->ops[s]->machID)).procTime(pm->ops[s]);

			//out << "pt = " << pt << endl;
			//out << "p = " << pm->ops[s]->p() << endl;

			if (pm->ops[s]->p() != pt) {
				//out << *pm << endl;
				out << "pt = " << pt << endl;
				out << "p = " << pm->ops[s]->p() << endl;
				Debugger::err << "Something is wrong with the processing time for " << pm->ops[s]->ID << ENDL;
			}
		}
	}
	out << "TGVNSScheduler::postprocessingActions : Done checking processing times." << endl;
	 */
	//##########################################################################

	//Debugger::info << "Finished postprocessing: cur_obj= " << cur_obj << ENDL;
	//getchar();
}

void TGVNSScheduler::run() {
	// Initialize the assignment of the operations to the machines
	//Debugger::info << "Creating initial assignment ... " << ENDL;
	//Debugger::info << "Tool group ID = " << tg.ID << ENDL;
	//Debugger::info << "Operation nodes: " << opnodes.size() << ENDL;
	// IMPORTANT!!! Initial assignment is created AFTER the precedence constraints have been found

	//initialAssignment(*pm, *tg, *opnodes);

	// WARNING!!! Initial assignment with ATC does not consider precedence constraints correctly for the case of parallel machines.
	// MUST BE FIXED!!!

	initialAssignment(*pm, *tg, *opnodes);

	//tgselection->localobj = Math::MAX_DOUBLE;

	Debugger::info << ENDL << "TGVNSScheduler::run : Initial objective by TGVNSScheduler (partial schedule, TWT (exp. LB), without assignment) : " << TWT()(*pm) << ENDL;

	//cur_obj = localObj(*pm, *opnodes, *terminals, *dloc);
	//if (localObj(*pm, *opnodes, *terminals, *dloc) != tgselection->localobj) {
	//Debugger::err << "TGVNSScheduler::run : objectives mismatch!" << ENDL;
	//}

	//Debugger::info << "Done creating initial assignment. " << ENDL;

	//Debugger::info << "Running the VNS iterative process..."<< ENDL; 

	// IMPORTANT!!! It is assumed that the correct selection has already been found

	TGSelection& selection = (TGSelection&) * tgselection;

	//int curTGID = selection.tgID;
	//cout << "TGWEODScheduler::localObj : Calculating objective for TG : " << curTGID << endl;
	//cout << "Arcs in the selection : " << selection.selection.size() << endl;

	// Insert the selection found after the initial assignment
	insertSelection(selection);
	Debugger::info << "TGVNSScheduler::run : Initial objective in tgselection.localobj : " << tgselection->localobj << ENDL;
	Debugger::info << "TGVNSScheduler::run : Global objective after initial assignment (recalculated, TWT, with selection, before VNS) : " << TWT()(*pm) << ENDL;
	removeSelection();

	//getchar();


	// Save the state of the PM (without the initial selection!!!)
	QMap<ListDigraph::Node, Operation> savedOps; // Saved operation data
	QList<double> savedP; // Arc lengths of the previously saved arcs
	QMap < ListDigraph::Arc, bool> savedConjunctive; // Used to momorize which arc is 

	// Preserve the information about the operations
	savedOps.clear();
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) * pm->ops[curNode];

		savedOps[curNode] = curOp;
	}

	// Preserve the information about the arcs for the current state of the PM
	savedP.clear();
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) { // IMPORTANT!!! DO NOT TOUCH THE ARCS WHICH ARE ALREADY IN THE PM, OTHERWISE DELETING AND RESTORING THEM WILL LEAD TO INVALID ARCS IN THE PM -> PROBLEMS!!!
		ListDigraph::Arc curArc = ait;

		savedP.append(pm->p[curArc]);

		savedConjunctive[curArc] = pm->conjunctive[curArc];

		// Mark all arcs of the current PM as conjunctive so that the LS doesn't modify them preserving their validness
		pm->conjunctive[curArc] = true;
	}


	insertSelection(selection);
	// Set the initial values of cur_obj and prev_obj
	cur_obj = tgselection->localobj;
	prev_obj = tgselection->localobj;
	best_obj = tgselection->localobj;

	bestID2opnodes = machID2opnodes;
	bestTGSelection = *tgselection;

	// Rund the VNS starting with the initial solution
	IterativeAlg::run();

	// Update the selection according to the new solution -> the old selection is not appropriate
	getTGSelection();

	// Restore the state of the PM as before the LS (without the current selection!!!)

	// Remove all disjunctive arcs in the graph dealing ONLY with this machine group
	QList<ListDigraph::Arc> arcsToRem;
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		ListDigraph::Node curStartNode = pm->graph.source(ait);
		Operation& curStartOp = (Operation&) *(pm->ops[curStartNode]);

		if (!pm->conjunctive[ait] && curStartOp.toolID == tg->ID) arcsToRem.append(curArc);
	}

	for (int i = 0; i < arcsToRem.size(); i++) {
		ListDigraph::Arc curArc = arcsToRem[i];
		pm->graph.erase(curArc);
	}

	// Restore the information about the operations
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		*(pm->ops[curNode]) = savedOps[curNode];
	}

	// Restore the lengths of all arcs
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		double curP = pm->ops[curNode]->p();
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			pm->p[curArc] = -curP;
		}
	}

	// Restore the conjunctiveness of the arcs
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		pm->conjunctive[curArc] = savedConjunctive[curArc];
	}

	Debugger::info << "TGVNSScheduler::run : Objective in tgselection.localobj after VNS : " << tgselection->localobj << ENDL << ENDL;

	//getchar();

	*tgselection = bestTGSelection;

	//Debugger::info << "Done running the VNS iterative process."<< ENDL;
}

void TGVNSScheduler::schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection) {
	if (opnodes.size() == 0) return;

	init();

	prev_nodes->reserve(opnodes.size());
	//kmax = opnodes.size() / tg.machines().size(); //*/(opnodes.size() - 1) / 3;

	iniScheduler->node2predST = this->node2predST;

	TGScheduler::schedule(pm, tg, opnodes, terminals, dloc, tgselection);
}

void TGVNSScheduler::scheduleCurOpSeq() {
	QTextStream out(stdout);

	//out << "ScheduleCurOpSeq ..." << endl;

	/*
#ifdef DEBUG

	// Check whether machID2opnodes is feasible to the selection
	for (int i = 0; i < tgselection->selection.size(); i++) {

		ListDigraph::Node curStartNode = tgselection->selection[i].first;
		ListDigraph::Node curEndNode = tgselection->selection[i].second;

		int curMachID = tgselection->opNode2SchedOps[curStartNode].machID;

		QList<ListDigraph::Node>& curMachNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];

		if (!curMachNodes.contains(curStartNode) || !curMachNodes.contains(curEndNode)) {
			Debugger::err << "TGVNSScheduler::scheduleCurOpSeq : Machine assignment is incorrect!!!" << ENDL;
		} else {

			int curStartIdx = curMachNodes.indexOf(curStartNode);
			int curEndIdx = curMachNodes.indexOf(curEndNode);

			if (curStartIdx > curEndIdx) {
				Debugger::err << "TGVNSScheduler::scheduleCurOpSeq : Machine assignment is incorrect (node sequence) !!!" << ENDL;
			}

		}

	}

#endif	
	 */




	QHash<int, QList<ListDigraph::Node> > machID2unschedopnodes = machID2opnodes; // Nodes, which have not been yet sequenced on the machines
	QList<int> mids = tg->mid2idx.keys();

	// For every node calculate the number of found predecessors
	QMap<ListDigraph::Node, int> unsched_pred;

	out << "Size of opnodes : " << opnodes->size() << endl;

	for (int i = 0; i < opnodes->size(); i++) {

		ListDigraph::Node curNode = opnodes->at(i);
		pm->ops[curNode]->machID = -1;
		unsched_pred[curNode] = predecessors[curNode].size();
		//Debugger::info << "Number of unscheduled predecessors: " << unsched_pred[opnodes->at(i)] << ENDL;

	}

	// Mapping of the operation IDs onto the operation nodes
	QHash<int, ListDigraph::Node> opID2Node;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		opID2Node[pm->ops[nit]->ID] = nit;
	}

	QList<ListDigraph::Node> ts = pm->topolSort();

	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	out << "TGVNSScheduler::scheduleCurOpSeq : TWT before scheduling : " << TWT()(*pm) << endl;


	insertSelection(*tgselection);
	out << "TGVNSScheduler::scheduleCurOpSeq : TWT before scheduling (with selection) : " << TWT()(*pm) << endl;
	removeSelection();
	out << "TGVNSScheduler::scheduleCurOpSeq : TWT before scheduling (removed selection) : " << TWT()(*pm) << endl;

	// DEBUG
	/*
	for (QHash<int, QList<ListDigraph::Node> >::iterator iter = machID2opnodes.begin(); iter != machID2opnodes.end(); iter++) {
		int mid = iter.key();
		out << endl;
		out << "Machine " << mid << " has " << machID2opnodes[mid].size() << " nodes " << endl;
		for (int i = 0; i < machID2opnodes[mid].size(); i++) {
			out << pm->ops[machID2opnodes[mid][i]]->ID << ",";
		}
		out << endl;
	}
	 */
	// DEBUG


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


#ifdef DEBUG 
	int tmp = 0;
	for (int i = 0; i < mids.size(); i++) {
		tmp += machID2unschedopnodes[mids[i]].size();

		//for (int j = 0; j < machID2unschedopnodes[mids[i]].size(); j++) {
		//	out << pm->ops[machID2unschedopnodes[mids[i]][j]]->ID << ", ";
		//}
		//out << endl;
	}
	if (tmp != opnodes->size()) {
		Debugger::warn << tmp << " " << opnodes->size() << ENDL;
		//for (int i = 0; i < opnodes->size(); i++) {
		//	out << pm->ops[opnodes->at(i)]->ID << ", ";
		//}
		//out << endl;
		Debugger::eDebug("Too many nodes to schedule!!!");

	}

	if (!dag(pm->graph)) {
		Debugger::eDebug("The graph is not DAG!!!");
	}
#endif

	tg->init();

	QList<ListDigraph::Node> nodesToSchedule; // Nodes which have to be scheduled
	QMap<ListDigraph::Node, int > node2MachID; // Machine IDs where the nodes must be scheduled
	QMap < ListDigraph::Node, bool > node2Scheduled; // Indicates whether the corresponding node has been scheduled

	for (int i = 0; i < mids.size(); i++) {
		// Schedule the first available nodes on the current machine
		int curMachID = mids[i];

		for (int j = 0; j < machID2opnodes[curMachID].size(); j++) {
			ListDigraph::Node curNode = machID2opnodes[curMachID].at(j);

			nodesToSchedule.append(curNode);
			node2MachID[curNode] = curMachID;
		}
	}

	for (int i = 0; i < nodesToSchedule.size(); i++) {
		ListDigraph::Node curNode = nodesToSchedule[i];
		node2Scheduled[curNode] = false;
	}

	for (int i = 0; i < nodesToSchedule.size(); i++) {
		ListDigraph::Node curNode = nodesToSchedule[i];
		unsched_pred[curNode] = predecessors[curNode].size();
	}


	// Iterate over the nodes which have to be scheduled on the predefined machines
	int numScheduledNodes = 0;

	while (numScheduledNodes < nodesToSchedule.size()) {

		QList<ListDigraph::Node> firstAvailNodes; // First available nodes for each machine

		QHash<int, int> machID2NextOpIdx;
		for (int i = 0; i < mids.size(); i++) {
			int curMachID = mids[i];
			QList<ListDigraph::Node>& curMachOpNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];

			for (int j = 0; j < curMachOpNodes.size(); j++) {
				ListDigraph::Node curMachNode = curMachOpNodes[j];
				if (!node2Scheduled[curMachNode]) {
					machID2NextOpIdx[curMachID] = j;
					break;
				} else {
					machID2NextOpIdx[curMachID] = -1;
				}
			}

			if (curMachOpNodes.size() == 0) { // In case there are no nodes on the machine
				machID2NextOpIdx[curMachID] = -1;
			}

		}

		do {

			// Do something to avoid the ties

			// For each machine select the first available operation
			for (int i = 0; i < mids.size(); i++) {
				int curMachID = mids[i];
				int nextOpIdx = machID2NextOpIdx[curMachID];

				if (nextOpIdx == -1) continue; // In case all nodes of the machine are scheduled

				QList<ListDigraph::Node>& curMachOpNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];
				ListDigraph::Node curNextNode = curMachOpNodes[nextOpIdx];

				if (unsched_pred[curNextNode] == 0 && !node2Scheduled[curNextNode]) {
					firstAvailNodes.append(curNextNode);
				}
			}

			if (firstAvailNodes.size() == 0) {
				// Select an arbitrary machine
				int curMachID = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];

				if (machID2opnodes[curMachID].size() == 0) continue; // In case there are no operations for this machine

				// Take the next operation
				machID2NextOpIdx[curMachID]++;

				if (machID2NextOpIdx[curMachID] >= machID2opnodes[curMachID].size()) { // This is the last operation on the machine -> reset to the first one

					QList<ListDigraph::Node>& curMachOpNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];

					for (int j = 0; j < curMachOpNodes.size(); j++) {
						ListDigraph::Node curMachNode = curMachOpNodes[j];
						if (!node2Scheduled[curMachNode]) {
							machID2NextOpIdx[curMachID] = j;
							break;
						} else {
							machID2NextOpIdx[curMachID] = -1;
						}
					}

				}

			} // First available nodes changing

		} while (firstAvailNodes.size() == 0);

#ifdef DEBUG  
		for (int i = 0; i < firstAvailNodes.size(); i++) {
			if (node2Scheduled[firstAvailNodes[i]]) {
				Debugger::err << "TGVNSScheduler::scheduleCurOpSeq : Selected a scheduled node!" << ENDL;
			}
		}
#endif  

		for (int i = 0; i < firstAvailNodes.size(); i++) {

			ListDigraph::Node curNode = firstAvailNodes[i];
			int curMachID = node2MachID[curNode];
			Operation& curOp = (Operation&) * (pm->ops[curNode]);

			if (unsched_pred[curNode] > 0) continue;

			// The machine for scheduling
			Machine& m = (*tg)(curMachID);

			ListDigraph::Node prevMachNode = INVALID;

			if (m.operations.size() == 0) {
				prevMachNode = INVALID;
			} else {
				int lastMachOperID = m.operations.last()->ID;
				prevMachNode = opID2Node[lastMachOperID];
			}

			//out << "TGVNSScheduler::scheduleCurOpSeq : Machine ID : " << curMachID << endl;
			//out << "TGVNSScheduler::scheduleCurOpSeq : Machine : " << m << endl;
			//out << "TGVNSScheduler::scheduleCurOpSeq : Scheduling operation " << curOp << endl;
			//out << "Number of unscheduled predecessors of 917534 id " << unsched_pred[opID2Node[917534]] << endl;
			//getchar();

			// Schedule the node on the machine
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

			// Update the number of unscheduled predecessors for the successors
			for (int j = 0; j < successors[curNode].size(); j++) {
				ListDigraph::Node curSucc = successors[curNode][j];

				unsched_pred[curSucc]--; // Decrease the number of unscheduled predecessors
			}

			// Indicate the node as scheduled
			node2Scheduled[curNode] = true;

			// Start all over again
			//i = -1;

			numScheduledNodes++;

		} // Iterating over the first available nodes

	} // Scheduling nodes

#ifdef DEBUG 
	if (nodesToSchedule.size() != numScheduledNodes) {
		out << "Scheduled nodes : " << numScheduledNodes << endl;
		out << "Should be scheduled : " << nodesToSchedule.size() << endl;
		Debugger::err << "TGVNSScheduler::scheduleCurOpSeq : Scheduled to few nodes!!!" << ENDL;
	}

	/*
	// Compare the initial assignment of the operations and the actual one
	out << "Initial assignment vs actual assignment: " << endl;
	for (int i = 0; i < mids.size(); i++) {
		int curMachID = mids[i];

		Machine& m = (Machine&) (*tg)(curMachID);

		out << "Machine : " << m.ID << endl;

		out << "Initial : ";
		for (int j = 0; j < machID2opnodes[curMachID].size(); j++) {
			out << pm->ops[machID2opnodes[curMachID][j]]->ID << ",";
		}
		out << endl;

		out << "Actual  : ";
		for (int j = 0; j < m.operations.size(); j++) {
			out << m.operations[j]->ID << ",";
		}
		out << endl;


		out << "Opers   : ";
		for (int j = 0; j < nodesToSchedule.size(); j++) {
			ListDigraph::Node curNode = nodesToSchedule[j];

			if (node2MachID[curNode] == curMachID) {
				out << pm->ops[curNode]->ID << ",";
			}
		}
		out << endl;

	 
	//getchar();
	}
	 */
#endif

	// Update heads and tails since the operations have been assigned without considering the release times
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	out << "TGVNSScheduler::scheduleCurOpSeq : TWT AFTER after scheduling : " << TWT()(*pm) << endl;
	//getchar();

	// Generate the tool group selection
	getTGSelection();

	// Get the assignment from the selection
	machID2opnodes = assignmentFromSelection(*tgselection);

	// Restore the PM's state

#ifdef DEBUG
	if (tgselection->selection.size() != insArcs.size()) {
		Debugger::err << "TGVNSScheduler::scheduleCurOpSeq : Wrong number of selection arcs!!!" << ENDL;
	}
#endif 

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

	// Restore the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm->conjunctive[curArc] = arc2PrevConj[curArc];
		pm->p[curArc] = arc2PrevP[curArc];
	}

	// ts is correct since eracing arcs does not violate the correctness of topological ordering
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	//out << "TGVNSScheduler::scheduleCurOpSeq : TWT AFTER restoring the PM : " << TWT()(*pm) << endl;
	//getchar();

	// Calculate the local objective for the current selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	//insertSelection(*tgselection);
	//out << "TGVNSScheduler::scheduleCurOpSeq : TWT AFTER after scheduling and inserting the selection : " << TWT()(*pm) << endl;
	//removeSelection();
	//out << "TGVNSScheduler::scheduleCurOpSeq : TWT AFTER after scheduling and removing the selection : " << TWT()(*pm) << endl;
	//getchar();

	//out << "Done ScheduleCurOpSeq" << endl;
}

void TGVNSScheduler::preserveRs() {
	rs.clear();
	rs.reserve(opnodes->size());

	for (int i = 0; i < opnodes->size(); i++) {

		rs[pm->ops[opnodes->at(i)]->ID] = pm->ops[opnodes->at(i)]->r();
	}
}

void TGVNSScheduler::restoreRs() {
	for (int i = 0; i < opnodes->size(); i++) {

		pm->ops[opnodes->at(i)]->r(rs[pm->ops[opnodes->at(i)]->ID]);
	}
}

void TGVNSScheduler::shake() {
	QTextStream out(stdout);

	out << "TGVNSScheduler::shake : Shaking..." << endl;

	double rnd = Rand::rnd<double>();

	if (rnd <= 0.50) {

		// Perform 3k+1 random swaps of the operation nodes
		randomSwapOperations(1 * (k - 1) + 1);

	} else if (rnd <= 0.90) {

		// Perform 3k+1 random swaps of the operation nodes
		randomMoveOperations(1 * (k - 1) + 1);

	} else if (rnd <= 0.95) {

		// Perform 3k+1 random swaps of the operation nodes
		randomSwapOperations(1 * (k - 1) + 1);

		// Perform 3k+1 random swaps of the operation nodes
		randomMoveOperations(1 * (k - 1) + 1);

	} else {

		// Perform 3k+1 random swaps of the operation nodes
		randomMoveOperations(1 * (k - 1) + 1);

		// Perform 3k+1 random swaps of the operation nodes
		randomSwapOperations(1 * (k - 1) + 1);

	}

	out << "TGVNSScheduler::shake : Shaking finished." << endl;

}

void TGVNSScheduler::randomSwapOperations(const int &nops) {
	/**Algorithm:
	 * 
	 * 1. Select randomly two machines in the tool group.
	 * 2. Select randomly one operation on the selected machines.
	 * 3. Swap the operations on the machines
	 * 4. Recalculate correctly the processing times of the operations and the 
	 *	  start times taking into consideration the precedence constraints.
	 */

	int swaps = 0;

	QList<int> mids = tg->mid2idx.keys();
	int mid1;
	int mid2;
	int opidx1;
	int opidx2;
	int r = 2;

	do {
		//Debugger::info << "Starting the shake loop ..." << ENDL;

		// Select randomly machine IDs
		//Debugger::info << "Selecting randomly machines ..." << ENDL;
		do {
			mid1 = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
			//Debugger::info << mid1 << machID2opnodes[mid1].size() << ENDL;
		} while (machID2opnodes[mid1].size() == 0);
		// Select randomly one operation on each of the machines m1
		opidx1 = Rand::rnd<Math::uint32>(0, machID2opnodes[mid1].size() - 1);

		do {
			mid2 = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
		} while (machID2opnodes[mid2].size() == 0);
		//Debugger::info << "Done selecting machines." << ENDL;
		// Select randomly one operation on each of the machines m2 in view of r in [max(opidx1-r,0), min(opidx1+r, nmax)]

		int p = opidx1;

		if (machID2opnodes[mid2].size() - 1 < p) { // Do not perform any swap
			continue;
		} else {
			opidx2 = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[mid2].size() - 1));
		}

		// Check whether the swap is possible
		if (mid1 == mid2 && opidx1 > opidx2) {
			qSwap(opidx1, opidx2);
		}
		if (swapPossible(mid1, mid2, opidx1, opidx2)) {
			qSwap(machID2opnodes[mid1][opidx1], machID2opnodes[mid2][opidx2]);
			swaps++;
		} else { // Try again
			continue;
		}

		//Debugger::info << "Finished the shake loop." << ENDL;

	} while (swaps < nops);
}

void TGVNSScheduler::randomSwapOperationsSameMachine(const int& mid, const int &) {
	int swaps = 0;

	if (machID2opnodes[mid].size() == 0) return;

	QList<int> mids = tg->mid2idx.keys();
	int opidx1;
	int opidx2;
	int r = 2;

	do {

		// Select randomly one operation on each of the machines m1
		opidx1 = Rand::rnd<Math::uint32>(0, machID2opnodes[mid].size() - 1);

		// Select randomly one operation on each of the machines m2 in view of r in [max(opidx1-r,0), min(opidx1+r, nmax)]
		int p = opidx1;

		opidx2 = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[mid].size() - 1));

		// Check whether the swap is possible
		if (opidx1 > opidx2) {
			qSwap(opidx1, opidx2);
		}
		if (swapPossible(mid, mid, opidx1, opidx2)) {
			qSwap(machID2opnodes[mid][opidx1], machID2opnodes[mid][opidx2]);
			swaps++;
		} else { // Try again
			continue;
		}

	} while (/*swaps < nops*/false);

}

bool TGVNSScheduler::swapPossible(const int &mid1, const int &mid2, const int &opidx1, const int &opidx2) {
	//return true;
	// Filter at least some potentially inefficient moves

	int type1 = pm->ops[machID2opnodes[mid1][opidx1]]->type; // Type of the first operation
	int type2 = pm->ops[machID2opnodes[mid2][opidx2]]->type; // Type of the second operation
	Machine& mach1 = *(tg->_machines[tg->mid2idx[mid1]]);
	Machine& mach2 = *(tg->_machines[tg->mid2idx[mid2]]);
	if (!mach1.type2speed.contains(type2) || !mach2.type2speed.contains(type1)) {
		return false;
	}

	// Check whether it is the same operation
	if (mid1 == mid2 && opidx1 == opidx2) return false;

	// Check whether the precedence constraints on each one of the machines are not violated due to the potential swap
	// The newly inserted node must not contain its predecessors after it and its successors before it

	// Check condition with predecessors
	for (int i = opidx1 + 1; i < machID2opnodes[mid1].size(); i++) {
		if ((mid1 == mid2 && machID2opnodes[mid1][i] != machID2opnodes[mid2][opidx2]) || (mid1 != mid2))
			if (predecessors[machID2opnodes[mid2][opidx2]].contains(machID2opnodes[mid1][i])) {
				return false;
			}
	}

	// Check condition with successors
	for (int i = 0; i < opidx1; i++) {
		if ((mid1 == mid2 && machID2opnodes[mid1][i] != machID2opnodes[mid2][opidx2]) || (mid1 != mid2))
			if (successors[machID2opnodes[mid2][opidx2]].contains(machID2opnodes[mid1][i])) {
				return false;
			}
	}

	// Check condition with predecessors
	for (int i = opidx2 + 1; i < machID2opnodes[mid2].size(); i++) {
		if ((mid1 == mid2 && machID2opnodes[mid2][i] != machID2opnodes[mid1][opidx1]) || (mid1 != mid2))
			if (predecessors[machID2opnodes[mid1][opidx1]].contains(machID2opnodes[mid2][i])) {
				return false;
			}
	}

	// Check condition with successors
	for (int i = 0; i < opidx2; i++) {
		if ((mid1 == mid2 && machID2opnodes[mid2][i] != machID2opnodes[mid1][opidx1]) || (mid1 != mid2))
			if (successors[machID2opnodes[mid1][opidx1]].contains(machID2opnodes[mid2][i])) {
				return false;
			}
	}

	return true;
}

void TGVNSScheduler::randomMoveOperations(const int &nops) {
	/**Algorithm:
	 * 
	 * 1. Select randomly two machines in the tool group.
	 * 2. Select randomly one operation on first machine.
	 * 3. Move the operations if it is correct
	 * 4. Recalculate correctly the processing times of the operations and the 
	 *	  start times taking into consideration the precedence constraints.
	 */

	QTextStream out(stdout);

	int moves = 0;

	QList<int> mids = tg->mid2idx.keys();
	int from_mid;
	int to_mid;
	int from_opidx;
	int to_opidx;
	bool move_possible = false;

	QSet<int> moved_operarions;

	int r = 2;

	do {
		//Debugger::info << "Starting the shake loop ..." << ENDL;

		// Select randomly machine IDs
		//Debugger::info << "Selecting randomly machines ..." << ENDL;
		do {
			from_mid = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
			//Debugger::info << mid1 << machID2opnodes[mid1].size() << ENDL;
		} while (machID2opnodes[from_mid].size() == 0);
		// Select randomly one operation on each of the machines
		from_opidx = Rand::rnd<Math::uint32>(0, machID2opnodes[from_mid].size() - 1);

		to_mid = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
		//Debugger::info << "Done selecting machines." << ENDL;

		// Select randomly one operation on each of the machines in view of r in [max(from_opidx-r,0), min(from_opidx+r, nmax)]
		int p = from_opidx;

		if (machID2opnodes[to_mid].size() - 1 < p) { // Move to the end

			to_opidx = machID2opnodes[to_mid].size();

		} else {

			to_opidx = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[to_mid].size() - 1));

		}


		// Check whether the move is possible
		//Debugger::info << "Checking whether the move is possible... : from_mid=" << from_mid << ", to_mid=" << to_mid << ", oper_id=" << pm->ops[machID2opnodes[from_mid].at(from_opidx)]->ID << ", from_opidx=" << from_opidx << ", to_opidx=" << to_opidx << ENDL;
		move_possible = movePossible(from_mid, to_mid, from_opidx, to_opidx);
		//Debugger::info << "Done checking whether the move is possible:" << ENDL;

		if (move_possible) {

			//qSwap(machID2opnodes[mid1][opidx1], machID2opnodes[mid2][opidx2]);
			//moved_operarions.insert(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID);
			//out << "Moving operation " << pm->ops[machID2opnodes[from_mid].at(from_opidx)]->ID << " from machine " << from_mid << " to machine " << to_mid << endl;
			machID2opnodes[to_mid].insert(to_opidx, machID2opnodes[from_mid].takeAt(from_opidx));

			moves++;

		} else { // Try again

			continue;

		}

		//Debugger::info << "Finished the shake loop." << ENDL;

	} while (moves < nops);
}

void TGVNSScheduler::randomMoveOperationsSameMachine(const int& mid, const int &nops) {
	QTextStream out(stdout);

	if (machID2opnodes[mid].size() == 0) return;

	int moves = 0;

	QList<int> mids = tg->mid2idx.keys();
	int from_opidx;
	int to_opidx;
	bool move_possible = false;

	int r = 2;

	do {

		// Select randomly one operation on each of the machines
		from_opidx = Rand::rnd<Math::uint32>(0, machID2opnodes[mid].size() - 1);

		// Select randomly one operation on each of the machines in view of r in [max(from_opidx-r,0), min(from_opidx+r, nmax)]
		int p = from_opidx;

		to_opidx = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[mid].size() - 1));

		// Check whether the move is possible
		move_possible = movePossible(mid, mid, from_opidx, to_opidx);

		if (move_possible) {

			machID2opnodes[mid].insert(to_opidx, machID2opnodes[mid].takeAt(from_opidx));

			moves++;

		} else { // Try again

			continue;

		}

	} while (moves < nops);

}

bool TGVNSScheduler::movePossible(const int &from_mid, const int &to_mid, const int &from_opidx, const int &to_opidx) {
	//return true;
	// Filter at least some moves which could be ineffective

	//if (machID2opnodes[from_mid].size() == 1) return false; // There was a bug when some machine had no operations assigned to it -> seems to be fixed

	int type = pm->ops[machID2opnodes[from_mid][from_opidx]]->type; // Type of the first operation
	Machine& mach = *(tg->_machines[tg->mid2idx[to_mid]]);
	if (!mach.type2speed.contains(type)) {
		return false;
	}

	/*
			if (tabus.contains(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID)) {
					if (tabus.value(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID).first == to_mid && tabus.value(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID).second == to_opidx) {
							return false;
					} else {
							// Insert the move to the tabu list
							tabus.insert(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID, QPair<int, int>(to_mid, to_opidx));
					}
			} else {
					tabus.insert(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID, QPair<int, int>(to_mid, to_opidx));
			}

	 */

	// Check whether it is the same operation
	if (from_mid == to_mid && from_opidx == to_opidx) return false;

	// Check whether some of the precedence constraints on each one of the machines are not violated due to the potential move
	// The newly inserted node must not contain its predecessors after it and its successors before it

	QList<ListDigraph::Node> nodes;

	// Check condition with predecessors
	//Debugger::info << "1..." << ENDL;
	nodes.clear();
	int istart;
	if (from_mid == to_mid) {
		if (from_opidx < to_opidx) {
			istart = to_opidx + 1;
		} else {
			istart = to_opidx;
		}
	} else {
		istart = to_opidx;
	}
	for (int i = istart; i < machID2opnodes[to_mid].size(); i++) {
		if ((from_mid == to_mid && machID2opnodes[to_mid][i] != machID2opnodes[from_mid][from_opidx]) || (from_mid != to_mid))
			if (predecessors[machID2opnodes[from_mid][from_opidx]].contains(machID2opnodes[to_mid][i])) {
				return false;
			}
	}
	//Debugger::info << "1." << ENDL;

	// Check condition with successors
	//Debugger::info << "2..." << ENDL;
	nodes.clear();
	int iend;
	if (from_mid == to_mid) {
		if (from_opidx < to_opidx) {
			iend = to_opidx + 1;
		} else {
			iend = to_opidx;
		}
	} else {
		iend = to_opidx;
	}
	for (int i = 0; i < iend; i++) {
		if ((from_mid == to_mid && machID2opnodes[to_mid][i] != machID2opnodes[from_mid][from_opidx]) || (from_mid != to_mid))
			if (successors[machID2opnodes[from_mid][from_opidx]].contains(machID2opnodes[to_mid][i])) {
				return false;
			}
	}

	//Debugger::info << "2." << ENDL;

	return true;
}

void TGVNSScheduler::initialAssignmentRND(ProcessModel& /*pm*/, ToolGroup &tg, QList<ListDigraph::Node> &opnodes) {
	QTextStream out(stdout);

	// Randomly assign equal number of operations to each machine
	QList<int> mids = tg.mid2idx.keys();
	int m = 0;

	machID2opnodes.clear();

	for (int i = 0; i < opnodes.size(); i++) {

		// Select randomly the machine ID, which the operation will be assigned to

		m = Rand::rnd<Math::uint32>(0, mids.size() - 1);

		// Assign the operation to the machine
		machID2opnodes[mids[m]].append(opnodes.at(i));
		//out << "Assigned operation: " << pm.ops[machID2opnodes[mids[m]].last()]->ID << endl;
		//getchar();
	}

	/*
	out << "Initial assignment:" << endl;
	for (int i = 0; i < mids.size(); i++) {
			out << "Machine " << mids[i] << ":" << endl;
			for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {
					out << pm.ops[machID2opnodes[mids[i]][j]]->ID << "->";
			}
			out << endl;
	}
	 */
}

void TGVNSScheduler::initialAssignment(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes) {
	// Preserve the initial process model
	//sbhscheduler->preserveOpSchedState(pm, opnodes);

	QTextStream out(stdout);

	QList<ListDigraph::Node> ts = pm.topolSort();

	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *pm.ops[curNode];
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm.conjunctive[curArc];
		arc2PrevP[curArc] = pm.p[curArc];
	}


	//Debugger::info << "Running the iniScheduler scheduler..." << ENDL;
	iniScheduler->schedule(pm, tg, opnodes, *terminals, *dloc, *tgselection);
	//Debugger::info << "Done running the iniScheduler scheduler." << ENDL;  	


	// The selection is found automatically

	int n_selection_nodes = tgselection->opNode2SchedOps.size();
	machID2opnodes.clear();
	out << "TGVNSScheduler::initialAssignment : Creating assignment from selection..." << endl;
	machID2opnodes = assignmentFromSelection(*tgselection);
	out << "TGVNSScheduler::initialAssignment : Created assignment from selection." << endl;

	/*
	for (int i = 0; i < tgselection->selection.size(); i++) {
		//Debugger::info << "(" << pm.ops[tgselection->selection[i].first]->ID << ";" << pm.ops[tgselection->selection[i].second]->ID << ")" << ENDL;
		if (!machID2opnodes[pm.ops[tgselection->selection[i].first]->machID].contains(tgselection->selection[i].first)) {
			n_selection_nodes++;
			machID2opnodes[pm.ops[tgselection->selection[i].first]->machID].append(tgselection->selection[i].first);

			//Debugger::warn << pm.ops[tgselection->selection[i].first]->ID << ENDL;
		}

		if (!machID2opnodes[pm.ops[tgselection->selection[i].second]->machID].contains(tgselection->selection[i].second)) {
			n_selection_nodes++;
			machID2opnodes[pm.ops[tgselection->selection[i].second]->machID].append(tgselection->selection[i].second);

			//Debugger::warn << pm.ops[tgselection->selection[i].second]->ID << ENDL;
		}
	}
	 */

	if (n_selection_nodes != opnodes.size()) {
		Debugger::warn << " Problem during the initial assignment! " << ENDL;
		Debugger::warn << " Present nodes: " << opnodes.size() << ENDL;
		for (int i = 0; i < opnodes.size(); i++) {
			Debugger::warn << pm.ops[opnodes[i]]->ID << ENDL;
		}
		Debugger::warn << " Assigned nodes: " << n_selection_nodes << ENDL;
		Debugger::err << "Failure during the initial assignment!" << ENDL;
	}


	// DEBUG

	insertSelection(*tgselection);
	cout << "TGVNSScheduler::initialAssignment : TWT before restoring the PM's state : " << TWT()(pm) << endl;
	removeSelection();

	// END DEBUG


	// Restore the operation data
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		*pm.ops[curNode] = node2PrevOper[curNode];
	}

	// Restore the arcs data
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm.conjunctive[curArc] = arc2PrevConj[curArc];
		pm.p[curArc] = arc2PrevP[curArc];
	}



	// DEBUG

	insertSelection(*tgselection);
	cout << "TGVNSScheduler::initialAssignment : TWT after restoring the PM's state and inserting the selection : " << TWT()(pm) << endl;
	removeSelection();
	cout << "TGVNSScheduler::initialAssignment : TWT after removing the selection : " << TWT()(pm) << endl;

	// END DEBUG



	// Restore the initial scheduling state of the affected operations
	//sbhscheduler->restoreOpSchedState(pm, opnodes);
}

void TGVNSScheduler::localSearch() {
	QTextStream out(stdout);
	//return;
	/** Algorithm:
	 * 
	 * 1. For every fixed operation try to swap it with all other operations
	 *  
	 */

	//Debugger::info << "Entered local search..." << ENDL;

	for (QMap<ListDigraph::Node, Operation>::const_iterator iter = tgselection->opNode2SchedOps.begin(); iter != tgselection->opNode2SchedOps.end(); iter++) {
		if (Math::abs((Math::intUNI)iter.value().ID) > 1000000000) {
			out << iter.value() << endl;
			//Debugger::err << "TGVNSScheduler::localSearch : Build invalid selection!" << ENDL;
		}
	}

	TGSelection besttgselection = *tgselection;
	TGSelection prevTGSelection = *tgselection;
	QHash<int, QList<ListDigraph::Node> > bestMachID2opnodes = machID2opnodes;
	QHash<int, QList<ListDigraph::Node> > prevMachID2opnodes = machID2opnodes;

	double bestobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
	double curobj;

	out << "TGVNSScheduler::localSearch : Starting with objective : " << bestobj << endl;
	out << "TGVNSScheduler::localSearch : Current best found objective : " << cur_obj << endl;
	getchar();

	QList<int> mids = machID2opnodes.keys();


	//for (int l = 0; l < opnodes->size(); l++) {

	/*

	for (int i = 0; i < mids.size(); i++) {
		for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {
			for (int othi = i; othi < mids.size(); othi++) {
				for (int othj = j + 1; othj < machID2opnodes[mids[othi]].size(); othj++) {
					// Try to swap the two operations
					if (swapPossible(mids[i], mids[othi], j, othj)) {
						qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
						// Accept or decline
						scheduleCurOpSeq();
						getTGSelection();
						curobj = localObj(*pm, *opnodes, *terminals, *dloc);
						tgselection->localobj = curobj;

						if (curobj < bestobj) {
							bestobj = curobj;
							besttgselection = *tgselection;
						} else {// Swap back
							qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
	 *tgselection = besttgselection;
						}
					}
				}
			}
		}

	}

	 */

	//return;
	//}

	bool move_possible = false;

	//for (int l = 0; l < opnodes->size(); l++) {

	// IMPORTANT: BUG - sizes in the loops change dynamically (seems to be solved).

	//Debugger::info << "1..." << ENDL;

	int ctr = 0;

	for (int i = 0; i < mids.size(); i++) {

		for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {

			for (int othi = i; othi < mids.size(); othi++) {

				for (int othj = j + 1; othj < machID2opnodes[mids[othi]].size(); othj++) {

					prevTGSelection = *tgselection;
					prevMachID2opnodes = machID2opnodes;

					// Avoid moving from the empty machine
					if (machID2opnodes[mids[i]].size() == 0) continue;
					// Try to move the operation
					//Debugger::info << "Checking whether the move is possible... : from_mid=" << mids[i] << ", to_mid=" << mids[othi] << ", from_opidx=" << j << ", to_opidx=" << othj << " from_m.size=" << machID2opnodes[mids[i]].size() << ", to_m.size=" << machID2opnodes[mids[othi]].size() << ENDL;
					move_possible = movePossible(mids[i], mids[othi], j, othj);
					//Debugger::info << "Done checking whether the move is possible." << ENDL;

					//Debugger::info << "3..." << ENDL;
					if (move_possible /*&& ctr < 100000000000000000*/) {

						ctr++;

						//Debugger::info << "3.1 ..." << ENDL;
						machID2opnodes[mids[othi]].insert(othj, machID2opnodes[mids[i]].takeAt(j));

						//qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
						// Accept or decline
						//Debugger::info << "3.1." << ENDL;
						//Debugger::info << "3.2 ..." << ENDL;
						scheduleCurOpSeq();
						//Debugger::info << "3.2." << ENDL;
						curobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

						//Debugger::info << "3.3 ..." << ENDL;
						if (curobj < bestobj) {

							bestMachID2opnodes = machID2opnodes;
							besttgselection = *tgselection;

							out << "TGVNSScheduler::localSearch : Improved solution : " << bestobj << endl;
							getchar();

							bestobj = curobj;

							if (j + 1 >= machID2opnodes[mids[i]].size()) {

								j = machID2opnodes[mids[i]].size() - 1;
								othj = j + 1;

							} else {

								//othj--;

							}

							besttgselection = *tgselection;

						} else {// Swap back

							machID2opnodes = prevMachID2opnodes;

							*tgselection = prevTGSelection;

						}
						//Debugger::info << "3.3." << ENDL;
					} // Performing a move
					//Debugger::info << "3." << ENDL;
				} // Iterating over operations j

			} // Iterating over machines j

		} // Iterating over operations i

	} // Iterating over machines i

	//Debugger::info << "1." << ENDL;

	//}

	*tgselection = besttgselection;
	machID2opnodes = bestMachID2opnodes;

	//Debugger::info << "Finished local search." << ENDL;

}

void TGVNSScheduler::localSearchSimple() {
	QTextStream out(stdout);
	//return;
	/** Algorithm:
	 * 
	 * 1. For every fixed operation try to swap it with all other operations
	 *  
	 */

	//Debugger::info << "Entered local search..." << ENDL;

	for (QMap<ListDigraph::Node, Operation>::const_iterator iter = tgselection->opNode2SchedOps.begin(); iter != tgselection->opNode2SchedOps.end(); iter++) {
		if (Math::abs((Math::intUNI)iter.value().ID) > 1000000000) {
			out << iter.value() << endl;
			//Debugger::err << "TGVNSScheduler::localSearch : Build invalid selection!" << ENDL;
		}
	}

	TGSelection besttgselection = *tgselection;
	TGSelection prevTGSelection = *tgselection;
	QHash<int, QList<ListDigraph::Node> > bestMachID2opnodes = machID2opnodes;
	QHash<int, QList<ListDigraph::Node> > prevMachID2opnodes = machID2opnodes;

	double bestobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
	double curobj;

	out << "TGVNSScheduler::localSearchSimple : Starting with objective : " << bestobj << endl;
	out << "TGVNSScheduler::localSearchSimple : Current best found objective : " << cur_obj << endl;
	//getchar();

	QList<int> mids = machID2opnodes.keys();

	/*

	for (int i = 0; i < mids.size(); i++) {
		for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {
			for (int othi = i; othi < mids.size(); othi++) {
				for (int othj = j + 1; othj < machID2opnodes[mids[othi]].size(); othj++) {
					// Try to swap the two operations
					if (swapPossible(mids[i], mids[othi], j, othj)) {
						qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
						// Accept or decline
						scheduleCurOpSeq();
						getTGSelection();
						curobj = localObj(*pm, *opnodes, *terminals, *dloc);
						tgselection->localobj = curobj;

						if (curobj < bestobj) {
							bestobj = curobj;
							besttgselection = *tgselection;
						} else {// Swap back
							qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
	 *tgselection = besttgselection;
						}
					}
				}
			}
		}

	}

	 */

	for (int i = 0; i < mids.size(); i++) {

		int ctr = 0;

		int curMID = mids[i];

		prevTGSelection = *tgselection;
		prevMachID2opnodes = machID2opnodes;

		// Avoid moving from the empty machine
		if (machID2opnodes[curMID].size() == 0) continue;

		while (ctr < 1000) {

			ctr++;

			double rnd = Rand::rnd<double>();
			if (rnd <= 0.5) {
				randomSwapOperationsSameMachine(curMID, k);
			} else {
				randomMoveOperationsSameMachine(curMID, k);
			}

			scheduleCurOpSeq();

			curobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
			tgselection->localobj = curobj;

			if (curobj < bestobj) { // Save the current better solution

				out << "TGVNSScheduler::localSearchSimple : Solution improved : " << curobj << endl;

				getchar();

				bestobj = curobj;

				bestMachID2opnodes = machID2opnodes;

				besttgselection = *tgselection;

			} else { // Restore

				//out << "TGVNSScheduler::localSearchSimple : Solution declined : " << curobj << endl;

				machID2opnodes = prevMachID2opnodes;

				*tgselection = prevTGSelection;

			}

		}

	}

	machID2opnodes = bestMachID2opnodes;
	*tgselection = besttgselection;

	out << "TGVNSScheduler::localSearchSimple : Finished! " << endl;
	getchar();

}

void TGVNSScheduler::localSearchLS() {
	LocalSearchPM ls;

	// Mark the nodes which can be moved during the local search
	QMap < ListDigraph::Node, bool> nodeMovable;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm->ops[curNode]);

		if (curOp.toolID == tg->ID) { // This node can be moved

			nodeMovable[curNode] = true;

		} else { // This node can not be moved

			nodeMovable[curNode] = false;

		}

	}

	Debugger::info << ENDL << "TGVNSScheduler::localSearchLS : Initial objective by TGVNSScheduler (partial schedule, TWT (exp. LB), without assignment) : " << TWT()(*pm) << ENDL;

	TGSelection& selection = (TGSelection&) * tgselection;

#ifdef DEBUG

	insertSelection(selection);
	Debugger::info << "TGVNSScheduler::localSearchLS : Initial objective in tgselection.localobj : " << tgselection->localobj << ENDL;
	Debugger::info << "TGVNSScheduler::localSearchLS : Global objective  (recalculated, TWT, with selection, before LS) : " << TWT()(*pm) << ENDL;
	removeSelection();

#endif 

	// Save the state of the PM (without the initial selection!!!)
	QMap<ListDigraph::Node, Operation> savedOps; // Saved operation data
	//QList<QPair<ListDigraph::Node, ListDigraph::Node> > savedArcs; // Arcs of the graph which were saved previously
	QList<double> savedP; // Arc lengths of the previously saved arcs
	//QList<bool> savedConjunctive; // Indicator of conjunctiveness of the previously saved arcs
	QMap < ListDigraph::Arc, bool> savedConjunctive; // Used to momorize which arc is 

	// Preserve the information about the operations
	savedOps.clear();
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) * pm->ops[curNode];

		savedOps[curNode] = curOp;
	}

	// Preserve the information about the arcs for the current state of the PM
	//savedArcs.clear();
	savedP.clear();
	//savedConjunctive.clear();
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) { // IMPORTANT!!! DO NOT TOUCH THE ARCS WHICH ARE ALREADY IN THE PM, OTHERWISE DELETING AND RESTORING THEM WILL LEAD TO INVALID ARCS IN THE PM -> PROBLEMS!!!
		ListDigraph::Arc curArc = ait;
		//ListDigraph::Node curStartNode = pm->graph.source(curArc);
		//ListDigraph::Node curEndNode = pm->graph.target(curArc);

		//savedArcs.append(QPair<ListDigraph::Node, ListDigraph::Node>(curStartNode, curEndNode));

		savedP.append(pm->p[curArc]);

		//savedConjunctive.append(pm->conjunctive[curArc]);

		savedConjunctive[curArc] = pm->conjunctive[curArc];

		// Mark all arcs of the current PM as conjunctive so that the LS doesn't modify them preserving their validness
		pm->conjunctive[curArc] = true;
	}


	// Perform scheduling
	insertSelection(selection); // Inserting the INITIAL selection of the initial solution in order to improve it later (IT WILL BE MODIFIED THROUGH THE LS)

	// Create the resources for the local search
	Resources rc;

	rc.ID = 1;
	rc << new ToolGroup(*tg);

	//ls.setRandGen(new RandGenMT(Rand::rndSeed()));
	ls.setRandGens(new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()), new Common::Rand::MT19937<double>(Rand::rndSeed()));
	ls.setObjective(this->sbhscheduler->obj->clone());
	ls.setPM(pm); // IMPORTANT!!! The PM will change during the LS - > the information should be preserved
	ls.setResources(&rc);
	ls.setMovableNodes(nodeMovable);
	ls.checkCorrectness(false);
	int lsIter = 4000; //10000; //*/Math::max(Math::round(4000.0 * (double(iter()) / double(maxIter()))), Math::round(4000.0 * (double(_curtime.elapsed()) / double(maxTimeMs()))));
	ls.maxIter(lsIter);
	//ls.maxTimeMs(100);

	cout << "k = " << k << endl;

	ls.run();

	Debugger::info << "TGVNSScheduler::localSearchLS : Global objective in PM after the LS (recalculated, TWT, state after LS) : " << TWT()(*pm) << ENDL;

	// Update the selection according to the new solution -> the old selection is not appropriate
	getTGSelection();

	// Restore the state of the PM as before the LS (without the current selection!!!)

	// Remove all disjunctive arcs in the graph dealing ONLY with this machine group
	QList<ListDigraph::Arc> arcsToRem;
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		ListDigraph::Node curStartNode = pm->graph.source(ait);
		Operation& curStartOp = (Operation&) *(pm->ops[curStartNode]);

		if (!pm->conjunctive[ait] && curStartOp.toolID == tg->ID) arcsToRem.append(curArc);
	}

	for (int i = 0; i < arcsToRem.size(); i++) {
		ListDigraph::Arc curArc = arcsToRem[i];
		pm->graph.erase(curArc);
	}

	// Restore the information about the operations
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		*(pm->ops[curNode]) = savedOps[curNode];
	}

	// Restore the lengths of all arcs
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		double curP = pm->ops[curNode]->p();
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			pm->p[curArc] = -curP;
		}
	}

	// Restore the conjunctiveness of the arcs
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		pm->conjunctive[curArc] = savedConjunctive[curArc];
	}


	Debugger::info << "TGVNSScheduler::localSearchLS : Global objective in PM after the LS (recalculated, TWT, no selection, after LS) : " << TWT()(*pm) << ENDL;
	insertSelection(selection);
	Debugger::info << "TGVNSScheduler::localSearchLS : Global objective in PM after the LS (recalculated, TWT, with selection, after LS) : " << TWT()(*pm) << ENDL;
	removeSelection();

	// Update the objective in the selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	Debugger::info << "TGVNSScheduler::localSearchLS : Global objective in tgselection after the LS : " << tgselection->localobj << ENDL;

	Debugger::info << "TGVNSScheduler::localSearchLS : Global objective without the selection -> PM will be used in other SSPs : " << TWT()(*pm) << ENDL;

}

double TGVNSScheduler::localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node>&, QHash<int, QList<double> >&) {
	QTextStream out(stdout);

	// Insert the current selection for the tool group
	//double localobj = 0.0;

	//SBHTWTLocalObj lobj;
	UTWT utwt;
	double res = utwt(pm, opnodes, locD); //localobj = lobj(pm, opnodes, *tgselection, terminals, dloc);

	TWT twt;

	// IMPORTANT!!! It is assumed that the correct selection has already been found

	TGSelection& selection = (TGSelection&) tgselection;

	//int curTGID = selection.tgID;
	//cout << "TGWEODScheduler::localObj : Calculating objective for TG : " << curTGID << endl;
	//cout << "Arcs in the selection : " << selection.selection.size() << endl;

	insertSelection(selection);

	//out << pm << endl;

	res = twt(pm); //*/ utwt(pm, opnodes, locD);

	removeSelection();

	//cout << "TWT : " << res << endl;

	return res;
}

QHash<int, QList<ListDigraph::Node> > TGVNSScheduler::assignmentFromSelection(TGSelection & selection) {
	QTextStream out(stdout);

	QHash<int, QList<ListDigraph::Node> > res;

	QList<ListDigraph::Node> tsNodes = selection.opNode2SchedOps.keys();

	QHash<int, QSet<int> > tsNodeID2PredIDs; // Predecessor nodes for each of the tsNodes

	QHash<int, ListDigraph::Node> nodeID2Node;

	for (int i = 0; i < tsNodes.size(); i++) {
		ListDigraph::Node curTSNode = tsNodes[i];
		int curNodeID = ListDigraph::id(curTSNode);

		tsNodeID2PredIDs[curNodeID] = QSet<int>();

		tsNodeID2PredIDs[curNodeID].clear();

		nodeID2Node[curNodeID] = curTSNode;
	}

	out << "TGVNSScheduler::assignmentFromSelection : Sorting the ts nodes... " << endl;

	// Iterate over the precedence constraints and collect the predecessors for each node
	for (int i = 0; i < selection.selection.size(); i++) {

		ListDigraph::Node curStartNode = selection.selection[i].first;
		ListDigraph::Node curEndNode = selection.selection[i].second;

		int curStartNodeID = ListDigraph::id(curStartNode);
		int curEndNodeID = ListDigraph::id(curEndNode);

		tsNodeID2PredIDs[curEndNodeID].insert(curStartNodeID);

	}

	QList<ListDigraph::Node> newTSNodes;
	QSet<int> sortedNodeIDs; // IDs of the nodes which have already been sorted
	sortedNodeIDs.clear();

	newTSNodes.reserve(tsNodes.size());
	for (int i = 0; i < tsNodes.size(); i++) {
		ListDigraph::Node curNode = tsNodes[i];
		int curNodeID = ListDigraph::id(curNode);

		// Remove the sorted elements from the predecessors set of the current node
		tsNodeID2PredIDs[curNodeID] = tsNodeID2PredIDs[curNodeID].subtract(sortedNodeIDs);

		// If there are no predecessors then move the node to the result
		if (tsNodeID2PredIDs[curNodeID].isEmpty()) {
			newTSNodes << curNode;
			sortedNodeIDs.insert(curNodeID);

			tsNodes.removeAt(i);

			i = -1;
		}

	}

	// Copy the new sorted TS Nodes
	tsNodes = newTSNodes;

	out << "TGVNSScheduler::assignmentFromSelection : Sorted the ts nodes. " << endl;

#ifdef DEBUG 

	for (int i = 0; i < selection.selection.size(); i++) {

		ListDigraph::Node curStartNode = selection.selection[i].first;
		ListDigraph::Node curEndNode = selection.selection[i].second;

		int curStartNodeIdx = tsNodes.indexOf(curStartNode);
		int curEndNodeIdx = tsNodes.indexOf(curEndNode);

		if (curStartNodeIdx > curEndNodeIdx) { // Precedence violated - > swap the nodes
			out << "Idx start : " << curStartNodeIdx << endl;
			out << "Idx end : " << curEndNodeIdx << endl;
			getchar();
			Debugger::err << "TGVNSScheduler::assignmentFromSelection : Precedence violated!!!" << ENDL;
		}

	}

#endif 

	// Update the assignment
	for (int i = 0; i < tsNodes.size(); i++) {

		ListDigraph::Node curNode = tsNodes[i];
		int curMachID = selection.opNode2SchedOps[curNode].machID;

		res[curMachID].append(curNode);

		//out << "Assigning operation " << selection.opNode2SchedOps[curNode] << " to machine " << curMachID << endl;

	}


	return res;
}
/*
QHash<int, QList<ListDigraph::Node> > TGVNSScheduler::OLDassignmentFromSelection(TGSelection & selection) {
	QTextStream out(stdout);

	QHash<int, QList<ListDigraph::Node> > res;

	QList<ListDigraph::Node> tsNodes = selection.opNode2SchedOps.keys();

	out << "TGVNSScheduler::assignmentFromSelection : Sorting the ts nodes... " << endl;

	QMap<ListDigraph::Node, int> nodeID2Idx;

	nodeID2Idx.clear();
	for (int i = 0; i < tsNodes.size(); i++) {
		ListDigraph::Node curNode = tsNodes[i];
		nodeID2Idx[curNode] = i;
	}

	// Sort the tsNodes so that no selection constraint is violated
	for (int i = 0; i < selection.selection.size(); i++) {

		ListDigraph::Node curStartNode = selection.selection[i].first;
		ListDigraph::Node curEndNode = selection.selection[i].second;

		//int curStartNodeIdx = tsNodes.indexOf(curStartNode);
		//int curEndNodeIdx = tsNodes.indexOf(curEndNode);

		int curStartNodeIdx = nodeID2Idx[curStartNode];
		int curEndNodeIdx = nodeID2Idx[curEndNode];

		if (curStartNodeIdx > curEndNodeIdx) { // Precedence violated - > swap the nodes
			tsNodes.move(curStartNodeIdx, curEndNodeIdx);

			//out << "Moving from " << curStartNodeIdx << " to " << curEndNodeIdx << endl;

			i = -1; // Since i will be increased at the end of the loop

			// Update the indices
			for (int i = 0; i < tsNodes.size(); i++) {
				ListDigraph::Node curNode = tsNodes[i];
				nodeID2Idx[curNode] = i;
			}
		}

	}

	out << "TGVNSScheduler::assignmentFromSelection : Sorted the ts nodes. " << endl;

#ifdef DEBUG	

	for (int i = 0; i < selection.selection.size(); i++) {

		ListDigraph::Node curStartNode = selection.selection[i].first;
		ListDigraph::Node curEndNode = selection.selection[i].second;

		int curStartNodeIdx = tsNodes.indexOf(curStartNode);
		int curEndNodeIdx = tsNodes.indexOf(curEndNode);

		if (curStartNodeIdx > curEndNodeIdx) { // Precedence violated - > swap the nodes
			out << "Idx start : " << curStartNodeIdx << endl;
			out << "Idx end : " << curEndNodeIdx << endl;
			Debugger::err << "TGVNSScheduler::assignmentFromSelection : Precedence violated!!!" << ENDL;
		}

	}

#endif	

	// Update the assignment
	for (int i = 0; i < tsNodes.size(); i++) {

		ListDigraph::Node curNode = tsNodes[i];
		int curMachID = selection.opNode2SchedOps[curNode].machID;

		res[curMachID].append(curNode);

		//out << "Assigning operation " << selection.opNode2SchedOps[curNode] << " to machine " << curMachID << endl;

	}


	return res;
}
 */
/**  **********************************************************************  **/

/**  ************************  TGVNSSchedulerPinSin  **********************  **/

TGVNSSchedulerPinSin::TGVNSSchedulerPinSin() {
	init();
}

TGVNSSchedulerPinSin::TGVNSSchedulerPinSin(TGVNSSchedulerPinSin& orig) : TGScheduler(orig), IterativeAlg(orig) {
	this->kmax = orig.kmax;
	this->k = orig.k;

	this->iniScheduler = (TGScheduler*) orig.iniScheduler->clone();

	this->locD = orig.locD;
}

TGVNSSchedulerPinSin::~TGVNSSchedulerPinSin() {
	if (prev_nodes != NULL) delete prev_nodes;
}

Clonable* TGVNSSchedulerPinSin::clone() {
	return new TGVNSSchedulerPinSin(*this);
}

void TGVNSSchedulerPinSin::init() {
	prev_obj = Math::MAX_DOUBLE;
	cur_obj = Math::MAX_DOUBLE;
	best_obj = Math::MAX_DOUBLE;

	prev_nodes = new QList<ListDigraph::Node>;

	// Set the current degree of the neighborhood
	k = 1;

	kmax = 5;

	machID2opnodes.clear();

	tabus.clear();

}

void TGVNSSchedulerPinSin::stepActions() {
	//Debugger::info << "Performing step actions ..." << ENDL;
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

	QTextStream out(stdout);

	// Preserve the current sequences of operations on the machines
	prev_machID2opnodes = machID2opnodes;

	// Preserve the current tool group selection before the move
	prev_tgselection = *tgselection;

	// Remove the selection from the graph
	removeSelection();

	// Perform the shaking step of the algorithm
	//Debugger::info << "Shaking ..." << ENDL;
	shake();
	//Debugger::info << "Done shaking." << ENDL;

	// Schedule the newly found sequence of the operations using "list scheduling"
	//Debugger::info << "Scheduling the current operation sequences ..." << ENDL;
	scheduleCurOpSeq();

	//insertSelection(*tgselection);
	//out << "TGVNSSchedulerPinSin::stepActions : UTWT before the local search : " << UTWT()(*pm, *opnodes, locD) << endl;
	//out << "TGVNSSchedulerPinSin::stepActions : TWT before the local search : " << TWT()(*pm) << endl;
	//removeSelection();


	//Debugger::info << "Done scheduling the current operation sequences." << ENDL;

	//getchar();
	//Debugger::info << "Done running tool group scheduling algorithm." << ENDL;

	// IMPORTANT!!! The selection for the new schedule of the tool group must be inserted into the graph in order to evaluate the solution

	// Perform local search in hope to find some better selection
	//Debugger::info << "Performing VNS local search ..." << ENDL;
	if (iter() % 1 == 0) {
		localSearchLS();
		//localSearch();
		//localSearchSimple();
	}
	//Debugger::info << "Done performing VNS local search." << ENDL;
}

void TGVNSSchedulerPinSin::assessActions() {
	prev_obj = cur_obj;

	cur_obj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	tgselection->localobj = cur_obj;

	//Debugger::info << "Assessing: cur_obj= " << cur_obj << " prev_obj= " << prev_obj << ENDL;
	//getchar();
}

bool TGVNSSchedulerPinSin::acceptCondition() {

	if (cur_obj < prev_obj) {

		acceptedWorse = false;

		return true;

	} else {

		if (cur_obj <= 1.05 * best_obj) {

			if (Rand::rnd<double>() <= 0.05) {

				acceptedWorse = true;

				return true;

			} else {

				acceptedWorse = false;

				return false;

			}

		} else {

			acceptedWorse = false;

			return false;

		}

	}

}

void TGVNSSchedulerPinSin::acceptActions() {
	QTextStream out(stdout);

	if (!acceptedWorse) {

		/*
		insertSelection(*tgselection);
		out << "TGVNSSchedulerPinSin::acceptActions : Accepted a better solution with UTWT : " << UTWT()(*pm, *opnodes, locD) << endl;
		out << "TGVNSSchedulerPinSin::acceptActions : Accepted a better solution with TWT : " << TWT()(*pm) << endl;
		removeSelection();
		getchar();
		 */

		Debugger::info << "Iter : " << (int) iter() << " : ";
		Debugger::info << "TGVNSSchedulerPinSin::acceptActions : Accepting solution with objective : " << cur_obj << ENDL;
		Debugger::info << "TGVNSSchedulerPinSin::acceptActions : Best solution with objective : " << best_obj << ENDL;
		//Debugger::info << "TGVNSSchedulerPinSin::acceptActions : Previous objective : " << prev_obj << ENDL;
		//getchar();

		// Update the assignment
		machID2opnodes = assignmentFromSelection(*tgselection);

		// Save the best assignment
		bestID2opnodes = machID2opnodes;

		bestTGSelection = *tgselection;

		best_obj = cur_obj;

	} else { // Accepted a worse solution

		Debugger::info << "Iter : " << (int) iter() << " : ";
		Debugger::info << "TGVNSSchedulerPinSin::acceptActions : Accepting a WORSE solution with objective : " << cur_obj << ENDL;
		Debugger::info << "TGVNSSchedulerPinSin::acceptActions : Best solution with objective : " << best_obj << ENDL;
		//Debugger::info << "TGVNSSchedulerPinSin::acceptActions : Previous objective : " << prev_obj << ENDL;
		//getchar();

		// Update the assignment

		machID2opnodes = assignmentFromSelection(*tgselection);

		// Restore the best found objective
		cur_obj = best_obj;

	}

	k = 1;

	// IMPORTANT!!! Selection is already preserved

	//Debugger::info << "Assessing: cur_obj= " << cur_obj << ENDL;
	//getchar();

	// Restore the initial values of the ready times of the nodes
	//restoreRs();
}

void TGVNSSchedulerPinSin::declineActions() {
	Debugger::info << "TGVNSSchedulerPinSin::declineActions : Declining solution with objective : " << cur_obj << ENDL;
	Debugger::info << "TGVNSSchedulerPinSin::declineActions : Previous objective : " << prev_obj << ENDL;
	Debugger::info << "TGVNSSchedulerPinSin::declineActions : Best solution with objective : " << best_obj << ENDL;
	//getchar();

	// Set the previous objective
	cur_obj = prev_obj;

	// Restore the previous sequence of the operations on the machines
	machID2opnodes = prev_machID2opnodes;

	// Restore the initial ready times of the nodes
	//restoreRs();

	// Update k
	k = k + 1;
	if (k > kmax) k = Math::max(kmax / 2, 1); //(1 + kmax) / 2;

	// IMPORTANT!!! Restore the previous selection
	*tgselection = prev_tgselection;
}

bool TGVNSSchedulerPinSin::stopCondition() {
	//Debugger::info << "TGVNSSchedulerPinSin:: stopCondition()"<<ENDL;
	return (Math::cmp(cur_obj, 0.0, 0.0001) == 0) || IterativeAlg::stopCondition();
}

void TGVNSSchedulerPinSin::stopActions() {
	//Debugger::info << "TGVNSSchedulerPinSin::stopActions()" << ENDL;
}

void TGVNSSchedulerPinSin::preprocessingActions() {
	// Preserve the initial values of the ready times
	//preserveRs();
}

void TGVNSSchedulerPinSin::postprocessingActions() {
	//Debugger::info << "TGVNSSchedulerPinSin::postprocessingActions()" << ENDL;

	//###########################  DEBUG  ######################################
	/*
	// For every node check whether processing times of the operation equals the length of the arc
	QTextStream out(stdout);
	out << "Checking consistency before TGVNSSchedulerPinSin::postprocessingActions..." << endl;
	for (ListDigraph::NodeIt nit(this->pm->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(this->pm->graph, nit); oait != INVALID; ++oait) {
			if (this->pm->ops[nit]->p() != -this->pm->p[oait]) {
				out << "op ID = " << this->pm->ops[nit]->ID << endl;
				out << *this->pm << endl;
				Debugger::err << "TGVNSSchedulerPinSin::postprocessingActions : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	// Schedule the operation nodes for the correct generation of the selection
	//Debugger::info << "TGVNSSchedulerPinSin::postprocessingActions() scheduleCurOpSeq..." << ENDL;
	//scheduleCurOpSeq();

	//###########################  DEBUG  ######################################
	// For every node check whether processing times of the operation equals the length of the arc
	/*
	out << "Checking consistency after TGVNSSchedulerPinSin::postprocessingActions..." << endl;
	for (ListDigraph::NodeIt nit(this->pm->graph); nit != INVALID; ++nit) {
		for (ListDigraph::OutArcIt oait(this->pm->graph, nit); oait != INVALID; ++oait) {
			if (this->pm->ops[nit]->p() != -this->pm->p[oait]) {
				out << "op ID = " << this->pm->ops[nit]->ID << endl;
				out << *this->pm << endl;
				Debugger::err << "TGVNSSchedulerPinSin::postprocessingActions : processing time does not equal the arc length!!! " << ENDL;
			}
		}
	}
	 */
	//##########################################################################

	//Debugger::info << "TGVNSSchedulerPinSin::postprocessingActions() DONE scheduleCurOpSeq." << ENDL;


	// Generate the tool group selection
	//getTGSelection();

	// Calculate the local objective for the current selection
	//tgselection->localobj = cur_obj;

	//#####################  DEBUG  ############################################
	/*
	QTextStream out(stdout);

	out << "TGVNSSchedulerPinSin::postprocessingActions : Selection:" << endl;
	for (int i = 0; i < tgselection->selection.size(); i++) {
		out << *pm->ops[tgselection->selection[i].first] << endl;
	}

	double pt;
	ListDigraph::Node s;
	out << "TGVNSSchedulerPinSin::postprocessingActions : Checking processing times..." << endl;
	for (int i = 0; i < tgselection->selection.size(); i++) {
		s = tgselection->selection[i].first;
		if (pm->ops[s]->machID >= 0) {
			pt = ((*tg)(pm->ops[s]->machID)).procTime(pm->ops[s]);

			//out << "pt = " << pt << endl;
			//out << "p = " << pm->ops[s]->p() << endl;

			if (pm->ops[s]->p() != pt) {
				//out << *pm << endl;
				out << "pt = " << pt << endl;
				out << "p = " << pm->ops[s]->p() << endl;
				Debugger::err << "Something is wrong with the processing time for " << pm->ops[s]->ID << ENDL;
			}
		}
	}
	out << "TGVNSSchedulerPinSin::postprocessingActions : Done checking processing times." << endl;
	 */
	//##########################################################################

	//Debugger::info << "Finished postprocessing: cur_obj= " << cur_obj << ENDL;
	//getchar();
}

void TGVNSSchedulerPinSin::run() {
	// Initialize the assignment of the operations to the machines
	//Debugger::info << "Creating initial assignment ... " << ENDL;
	//Debugger::info << "Tool group ID = " << tg.ID << ENDL;
	//Debugger::info << "Operation nodes: " << opnodes.size() << ENDL;
	// IMPORTANT!!! Initial assignment is created AFTER the precedence constraints have been found

	//initialAssignment(*pm, *tg, *opnodes);

	// WARNING!!! Initial assignment with ATC does not consider precedence constraints correctly for the case of parallel machines.
	// MUST BE FIXED!!!

	initialAssignment(*pm, *tg, *opnodes);

	//tgselection->localobj = Math::MAX_DOUBLE;

	Debugger::info << ENDL << "TGVNSSchedulerPinSin::run : Initial objective by TGVNSSchedulerPinSin (partial schedule, TWT (exp. LB), without assignment) : " << TWT()(*pm) << ENDL;

	//cur_obj = localObj(*pm, *opnodes, *terminals, *dloc);
	//if (localObj(*pm, *opnodes, *terminals, *dloc) != tgselection->localobj) {
	//Debugger::err << "TGVNSSchedulerPinSin::run : objectives mismatch!" << ENDL;
	//}

	//Debugger::info << "Done creating initial assignment. " << ENDL;

	//Debugger::info << "Running the VNS iterative process..."<< ENDL; 

	// IMPORTANT!!! It is assumed that the correct selection has already been found

	TGSelection& selection = (TGSelection&) * tgselection;

	//int curTGID = selection.tgID;
	//cout << "TGWEODScheduler::localObj : Calculating objective for TG : " << curTGID << endl;
	//cout << "Arcs in the selection : " << selection.selection.size() << endl;

	// Insert the selection found after the initial assignment
	insertSelection(selection);
	Debugger::info << "TGVNSSchedulerPinSin::run : Initial objective in tgselection.localobj : " << tgselection->localobj << ENDL;
	Debugger::info << "TGVNSSchedulerPinSin::run : Global objective after initial assignment (recalculated, TWT, with selection, before VNS) : " << TWT()(*pm) << ENDL;
	removeSelection();

	//getchar();


	// Save the state of the PM (without the initial selection!!!)
	QMap<ListDigraph::Node, Operation> savedOps; // Saved operation data
	QList<double> savedP; // Arc lengths of the previously saved arcs
	QMap < ListDigraph::Arc, bool> savedConjunctive; // Used to momorize which arc is 

	// Preserve the information about the operations
	savedOps.clear();
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) * pm->ops[curNode];

		savedOps[curNode] = curOp;
	}

	// Preserve the information about the arcs for the current state of the PM
	savedP.clear();
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) { // IMPORTANT!!! DO NOT TOUCH THE ARCS WHICH ARE ALREADY IN THE PM, OTHERWISE DELETING AND RESTORING THEM WILL LEAD TO INVALID ARCS IN THE PM -> PROBLEMS!!!
		ListDigraph::Arc curArc = ait;

		savedP.append(pm->p[curArc]);

		savedConjunctive[curArc] = pm->conjunctive[curArc];

		// Mark all arcs of the current PM as conjunctive so that the LS doesn't modify them preserving their validness
		pm->conjunctive[curArc] = true;
	}


	insertSelection(selection);
	// Set the initial values of cur_obj and prev_obj
	cur_obj = tgselection->localobj;
	prev_obj = tgselection->localobj;
	best_obj = tgselection->localobj;

	bestID2opnodes = machID2opnodes;
	bestTGSelection = *tgselection;

	// Rund the VNS starting with the initial solution
	IterativeAlg::run();

	// Update the selection according to the new solution -> the old selection is not appropriate
	getTGSelection();

	// Restore the state of the PM as before the LS (without the current selection!!!)

	// Remove all disjunctive arcs in the graph dealing ONLY with this machine group
	QList<ListDigraph::Arc> arcsToRem;
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		ListDigraph::Node curStartNode = pm->graph.source(ait);
		Operation& curStartOp = (Operation&) *(pm->ops[curStartNode]);

		if (!pm->conjunctive[ait] && curStartOp.toolID == tg->ID) arcsToRem.append(curArc);
	}

	for (int i = 0; i < arcsToRem.size(); i++) {
		ListDigraph::Arc curArc = arcsToRem[i];
		pm->graph.erase(curArc);
	}

	// Restore the information about the operations
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		*(pm->ops[curNode]) = savedOps[curNode];
	}

	// Restore the lengths of all arcs
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		double curP = pm->ops[curNode]->p();
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			pm->p[curArc] = -curP;
		}
	}

	// Restore the conjunctiveness of the arcs
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		pm->conjunctive[curArc] = savedConjunctive[curArc];
	}

	Debugger::info << "TGVNSSchedulerPinSin::run : Objective in tgselection.localobj after VNS : " << tgselection->localobj << ENDL << ENDL;

	//getchar();

	*tgselection = bestTGSelection;

	//Debugger::info << "Done running the VNS iterative process."<< ENDL;
}

void TGVNSSchedulerPinSin::schedule(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes, QList<ListDigraph::Node> & terminals, QHash<int, QList<double> > &dloc, TGSelection &tgselection) {
	if (opnodes.size() == 0) return;

	init();

	prev_nodes->reserve(opnodes.size());
	//kmax = opnodes.size() / tg.machines().size(); //*/(opnodes.size() - 1) / 3;

	iniScheduler->node2predST = this->node2predST;

	TGScheduler::schedule(pm, tg, opnodes, terminals, dloc, tgselection);
}

void TGVNSSchedulerPinSin::scheduleCurOpSeq() {
	QTextStream out(stdout);

	//out << "ScheduleCurOpSeq ..." << endl;

	/*
#ifdef DEBUG

	// Check whether machID2opnodes is feasible to the selection
	for (int i = 0; i < tgselection->selection.size(); i++) {

		ListDigraph::Node curStartNode = tgselection->selection[i].first;
		ListDigraph::Node curEndNode = tgselection->selection[i].second;

		int curMachID = tgselection->opNode2SchedOps[curStartNode].machID;

		QList<ListDigraph::Node>& curMachNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];

		if (!curMachNodes.contains(curStartNode) || !curMachNodes.contains(curEndNode)) {
			Debugger::err << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Machine assignment is incorrect!!!" << ENDL;
		} else {

			int curStartIdx = curMachNodes.indexOf(curStartNode);
			int curEndIdx = curMachNodes.indexOf(curEndNode);

			if (curStartIdx > curEndIdx) {
				Debugger::err << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Machine assignment is incorrect (node sequence) !!!" << ENDL;
			}

		}

	}

#endif	
	 */




	QHash<int, QList<ListDigraph::Node> > machID2unschedopnodes = machID2opnodes; // Nodes, which have not been yet sequenced on the machines
	QList<int> mids = tg->mid2idx.keys();

	// For every node calculate the number of found predecessors
	QMap<ListDigraph::Node, int> unsched_pred;

	out << "Size of opnodes : " << opnodes->size() << endl;

	for (int i = 0; i < opnodes->size(); i++) {

		ListDigraph::Node curNode = opnodes->at(i);
		pm->ops[curNode]->machID = -1;
		unsched_pred[curNode] = predecessors[curNode].size();
		//Debugger::info << "Number of unscheduled predecessors: " << unsched_pred[opnodes->at(i)] << ENDL;

	}

	// Mapping of the operation IDs onto the operation nodes
	QHash<int, ListDigraph::Node> opID2Node;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		opID2Node[pm->ops[nit]->ID] = nit;
	}

	QList<ListDigraph::Node> ts = pm->topolSort();

	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT before scheduling : " << TWT()(*pm) << endl;


	insertSelection(*tgselection);
	out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT before scheduling (with selection) : " << TWT()(*pm) << endl;
	removeSelection();
	out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT before scheduling (removed selection) : " << TWT()(*pm) << endl;

	// DEBUG
	/*
	for (QHash<int, QList<ListDigraph::Node> >::iterator iter = machID2opnodes.begin(); iter != machID2opnodes.end(); iter++) {
		int mid = iter.key();
		out << endl;
		out << "Machine " << mid << " has " << machID2opnodes[mid].size() << " nodes " << endl;
		for (int i = 0; i < machID2opnodes[mid].size(); i++) {
			out << pm->ops[machID2opnodes[mid][i]]->ID << ",";
		}
		out << endl;
	}
	 */
	// DEBUG


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


#ifdef DEBUG 
	int tmp = 0;
	for (int i = 0; i < mids.size(); i++) {
		tmp += machID2unschedopnodes[mids[i]].size();

		//for (int j = 0; j < machID2unschedopnodes[mids[i]].size(); j++) {
		//	out << pm->ops[machID2unschedopnodes[mids[i]][j]]->ID << ", ";
		//}
		//out << endl;
	}
	if (tmp != opnodes->size()) {
		Debugger::warn << tmp << " " << opnodes->size() << ENDL;
		//for (int i = 0; i < opnodes->size(); i++) {
		//	out << pm->ops[opnodes->at(i)]->ID << ", ";
		//}
		//out << endl;
		Debugger::eDebug("Too many nodes to schedule!!!");

	}

	if (!dag(pm->graph)) {
		Debugger::eDebug("The graph is not DAG!!!");
	}
#endif

	tg->init();

	QList<ListDigraph::Node> nodesToSchedule; // Nodes which have to be scheduled
	QMap<ListDigraph::Node, int > node2MachID; // Machine IDs where the nodes must be scheduled
	QMap < ListDigraph::Node, bool > node2Scheduled; // Indicates whether the corresponding node has been scheduled

	for (int i = 0; i < mids.size(); i++) {
		// Schedule the first available nodes on the current machine
		int curMachID = mids[i];

		for (int j = 0; j < machID2opnodes[curMachID].size(); j++) {
			ListDigraph::Node curNode = machID2opnodes[curMachID].at(j);

			nodesToSchedule.append(curNode);
			node2MachID[curNode] = curMachID;
		}
	}

	for (int i = 0; i < nodesToSchedule.size(); i++) {
		ListDigraph::Node curNode = nodesToSchedule[i];
		node2Scheduled[curNode] = false;
	}

	for (int i = 0; i < nodesToSchedule.size(); i++) {
		ListDigraph::Node curNode = nodesToSchedule[i];
		unsched_pred[curNode] = predecessors[curNode].size();
	}


	// Iterate over the nodes which have to be scheduled on the predefined machines
	int numScheduledNodes = 0;

	while (numScheduledNodes < nodesToSchedule.size()) {

		QList<ListDigraph::Node> firstAvailNodes; // First available nodes for each machine

		QHash<int, int> machID2NextOpIdx;
		for (int i = 0; i < mids.size(); i++) {
			int curMachID = mids[i];
			QList<ListDigraph::Node>& curMachOpNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];

			for (int j = 0; j < curMachOpNodes.size(); j++) {
				ListDigraph::Node curMachNode = curMachOpNodes[j];
				if (!node2Scheduled[curMachNode]) {
					machID2NextOpIdx[curMachID] = j;
					break;
				} else {
					machID2NextOpIdx[curMachID] = -1;
				}
			}

			if (curMachOpNodes.size() == 0) { // In case there are no nodes on the machine
				machID2NextOpIdx[curMachID] = -1;
			}

		}

		do {

			// Do something to avoid the ties

			// For each machine select the first available operation
			for (int i = 0; i < mids.size(); i++) {
				int curMachID = mids[i];
				int nextOpIdx = machID2NextOpIdx[curMachID];

				if (nextOpIdx == -1) continue; // In case all nodes of the machine are scheduled

				QList<ListDigraph::Node>& curMachOpNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];
				ListDigraph::Node curNextNode = curMachOpNodes[nextOpIdx];

				if (unsched_pred[curNextNode] == 0 && !node2Scheduled[curNextNode]) {
					firstAvailNodes.append(curNextNode);
				}
			}

			if (firstAvailNodes.size() == 0) {
				// Select an arbitrary machine
				int curMachID = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];

				if (machID2opnodes[curMachID].size() == 0) continue; // In case there are no operations for this machine

				// Take the next operation
				machID2NextOpIdx[curMachID]++;

				if (machID2NextOpIdx[curMachID] >= machID2opnodes[curMachID].size()) { // This is the last operation on the machine -> reset to the first one

					QList<ListDigraph::Node>& curMachOpNodes = (QList<ListDigraph::Node>&) machID2opnodes[curMachID];

					for (int j = 0; j < curMachOpNodes.size(); j++) {
						ListDigraph::Node curMachNode = curMachOpNodes[j];
						if (!node2Scheduled[curMachNode]) {
							machID2NextOpIdx[curMachID] = j;
							break;
						} else {
							machID2NextOpIdx[curMachID] = -1;
						}
					}

				}

			} // First available nodes changing

		} while (firstAvailNodes.size() == 0);

#ifdef DEBUG  
		for (int i = 0; i < firstAvailNodes.size(); i++) {
			if (node2Scheduled[firstAvailNodes[i]]) {
				Debugger::err << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Selected a scheduled node!" << ENDL;
			}
		}
#endif  

		for (int i = 0; i < firstAvailNodes.size(); i++) {

			ListDigraph::Node curNode = firstAvailNodes[i];
			int curMachID = node2MachID[curNode];
			Operation& curOp = (Operation&) * (pm->ops[curNode]);

			if (unsched_pred[curNode] > 0) continue;

			// The machine for scheduling
			Machine& m = (*tg)(curMachID);

			ListDigraph::Node prevMachNode = INVALID;

			if (m.operations.size() == 0) {
				prevMachNode = INVALID;
			} else {
				int lastMachOperID = m.operations.last()->ID;
				prevMachNode = opID2Node[lastMachOperID];
			}

			//out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Machine ID : " << curMachID << endl;
			//out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Machine : " << m << endl;
			//out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Scheduling operation " << curOp << endl;
			//out << "Number of unscheduled predecessors of 917534 id " << unsched_pred[opID2Node[917534]] << endl;
			//getchar();

			// Schedule the node on the machine
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

			// Update the number of unscheduled predecessors for the successors
			for (int j = 0; j < successors[curNode].size(); j++) {
				ListDigraph::Node curSucc = successors[curNode][j];

				unsched_pred[curSucc]--; // Decrease the number of unscheduled predecessors
			}

			// Indicate the node as scheduled
			node2Scheduled[curNode] = true;

			// Start all over again
			//i = -1;

			numScheduledNodes++;

		} // Iterating over the first available nodes

	} // Scheduling nodes

#ifdef DEBUG 
	if (nodesToSchedule.size() != numScheduledNodes) {
		out << "Scheduled nodes : " << numScheduledNodes << endl;
		out << "Should be scheduled : " << nodesToSchedule.size() << endl;
		Debugger::err << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Scheduled to few nodes!!!" << ENDL;
	}

	/*
	// Compare the initial assignment of the operations and the actual one
	out << "Initial assignment vs actual assignment: " << endl;
	for (int i = 0; i < mids.size(); i++) {
		int curMachID = mids[i];

		Machine& m = (Machine&) (*tg)(curMachID);

		out << "Machine : " << m.ID << endl;

		out << "Initial : ";
		for (int j = 0; j < machID2opnodes[curMachID].size(); j++) {
			out << pm->ops[machID2opnodes[curMachID][j]]->ID << ",";
		}
		out << endl;

		out << "Actual  : ";
		for (int j = 0; j < m.operations.size(); j++) {
			out << m.operations[j]->ID << ",";
		}
		out << endl;


		out << "Opers   : ";
		for (int j = 0; j < nodesToSchedule.size(); j++) {
			ListDigraph::Node curNode = nodesToSchedule[j];

			if (node2MachID[curNode] == curMachID) {
				out << pm->ops[curNode]->ID << ",";
			}
		}
		out << endl;

	 
	//getchar();
	}
	 */
#endif

	// Update heads and tails since the operations have been assigned without considering the release times
	pm->updateHeads();
	pm->updateStartTimes();
	out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT AFTER after scheduling : " << TWT()(*pm) << endl;
	//getchar();

	// Generate the tool group selection
	getTGSelection();

	// Get the assignment from the selection
	machID2opnodes = assignmentFromSelection(*tgselection);

	// Restore the PM's state

#ifdef DEBUG
	if (tgselection->selection.size() != insArcs.size()) {
		Debugger::err << "TGVNSSchedulerPinSin::scheduleCurOpSeq : Wrong number of selection arcs!!!" << ENDL;
	}
#endif 

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

	// Restore the arcs data
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm->conjunctive[curArc] = arc2PrevConj[curArc];
		pm->p[curArc] = arc2PrevP[curArc];
	}

	// ts is correct since eracing arcs does not violate the correctness of topological ordering
	pm->updateHeads(ts);
	pm->updateStartTimes(ts);
	//out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT AFTER restoring the PM : " << TWT()(*pm) << endl;
	//getchar();

	// Calculate the local objective for the current selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	//insertSelection(*tgselection);
	//out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT AFTER after scheduling and inserting the selection : " << TWT()(*pm) << endl;
	//removeSelection();
	//out << "TGVNSSchedulerPinSin::scheduleCurOpSeq : TWT AFTER after scheduling and removing the selection : " << TWT()(*pm) << endl;
	//getchar();

	//out << "Done ScheduleCurOpSeq" << endl;
}

void TGVNSSchedulerPinSin::preserveRs() {
	rs.clear();
	rs.reserve(opnodes->size());

	for (int i = 0; i < opnodes->size(); i++) {

		rs[pm->ops[opnodes->at(i)]->ID] = pm->ops[opnodes->at(i)]->r();
	}
}

void TGVNSSchedulerPinSin::restoreRs() {
	for (int i = 0; i < opnodes->size(); i++) {

		pm->ops[opnodes->at(i)]->r(rs[pm->ops[opnodes->at(i)]->ID]);
	}
}

void TGVNSSchedulerPinSin::shake() {
	QTextStream out(stdout);

	double rnd = Rand::rnd<double>();

	if (rnd <= 0.50) {

		// Perform 3k+1 random swaps of the operation nodes
		randomSwapOperations(1 * (k - 1) + 1);

	} else if (rnd <= 0.90) {

		// Perform 3k+1 random swaps of the operation nodes
		randomMoveOperations(1 * (k - 1) + 1);

	} else if (rnd <= 0.95) {

		// Perform 3k+1 random swaps of the operation nodes
		randomSwapOperations(1 * (k - 1) + 1);

		// Perform 3k+1 random swaps of the operation nodes
		randomMoveOperations(1 * (k - 1) + 1);

	} else {

		// Perform 3k+1 random swaps of the operation nodes
		randomMoveOperations(1 * (k - 1) + 1);

		// Perform 3k+1 random swaps of the operation nodes
		randomSwapOperations(1 * (k - 1) + 1);

	}

}

void TGVNSSchedulerPinSin::randomSwapOperations(const int &nops) {
	/**Algorithm:
	 * 
	 * 1. Select randomly two machines in the tool group.
	 * 2. Select randomly one operation on the selected machines.
	 * 3. Swap the operations on the machines
	 * 4. Recalculate correctly the processing times of the operations and the 
	 *	  start times taking into consideration the precedence constraints.
	 */

	int swaps = 0;

	QList<int> mids = tg->mid2idx.keys();
	int mid1;
	int mid2;
	int opidx1;
	int opidx2;
	int r = 2;

	do {
		//Debugger::info << "Starting the shake loop ..." << ENDL;

		// Select randomly machine IDs
		//Debugger::info << "Selecting randomly machines ..." << ENDL;
		do {
			mid1 = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
			//Debugger::info << mid1 << machID2opnodes[mid1].size() << ENDL;
		} while (machID2opnodes[mid1].size() == 0);
		// Select randomly one operation on each of the machines m1
		opidx1 = Rand::rnd<Math::uint32>(0, machID2opnodes[mid1].size() - 1);

		do {
			mid2 = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
		} while (machID2opnodes[mid2].size() == 0);
		//Debugger::info << "Done selecting machines." << ENDL;
		// Select randomly one operation on each of the machines m2 in view of r in [max(opidx1-r,0), min(opidx1+r, nmax)]

		int p = opidx1;

		if (machID2opnodes[mid2].size() - 1 < p) { // Do not perform any swap
			continue;
		} else {
			opidx2 = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[mid2].size() - 1));
		}

		// Check whether the swap is possible
		if (mid1 == mid2 && opidx1 > opidx2) {
			qSwap(opidx1, opidx2);
		}
		if (swapPossible(mid1, mid2, opidx1, opidx2)) {
			qSwap(machID2opnodes[mid1][opidx1], machID2opnodes[mid2][opidx2]);
			swaps++;
		} else { // Try again
			continue;
		}

		//Debugger::info << "Finished the shake loop." << ENDL;

	} while (swaps < nops);
}

void TGVNSSchedulerPinSin::randomSwapOperationsSameMachine(const int& mid, const int &) {
	int swaps = 0;

	if (machID2opnodes[mid].size() == 0) return;

	QList<int> mids = tg->mid2idx.keys();
	int opidx1;
	int opidx2;
	int r = 2;

	do {

		// Select randomly one operation on each of the machines m1
		opidx1 = Rand::rnd<Math::uint32>(0, machID2opnodes[mid].size() - 1);

		// Select randomly one operation on each of the machines m2 in view of r in [max(opidx1-r,0), min(opidx1+r, nmax)]
		int p = opidx1;

		opidx2 = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[mid].size() - 1));

		// Check whether the swap is possible
		if (opidx1 > opidx2) {
			qSwap(opidx1, opidx2);
		}
		if (swapPossible(mid, mid, opidx1, opidx2)) {
			qSwap(machID2opnodes[mid][opidx1], machID2opnodes[mid][opidx2]);
			swaps++;
		} else { // Try again
			continue;
		}

	} while (/*swaps < nops*/false);

}

bool TGVNSSchedulerPinSin::swapPossible(const int &mid1, const int &mid2, const int &opidx1, const int &opidx2) {
	//return true;
	// Filter at least some potentially inefficient moves

	int type1 = pm->ops[machID2opnodes[mid1][opidx1]]->type; // Type of the first operation
	int type2 = pm->ops[machID2opnodes[mid2][opidx2]]->type; // Type of the second operation
	Machine& mach1 = *(tg->_machines[tg->mid2idx[mid1]]);
	Machine& mach2 = *(tg->_machines[tg->mid2idx[mid2]]);
	if (!mach1.type2speed.contains(type2) || !mach2.type2speed.contains(type1)) {
		return false;
	}

	// Check whether it is the same operation
	if (mid1 == mid2 && opidx1 == opidx2) return false;

	// Check whether the precedence constraints on each one of the machines are not violated due to the potential swap
	// The newly inserted node must not contain its predecessors after it and its successors before it

	// Check condition with predecessors
	for (int i = opidx1 + 1; i < machID2opnodes[mid1].size(); i++) {
		if ((mid1 == mid2 && machID2opnodes[mid1][i] != machID2opnodes[mid2][opidx2]) || (mid1 != mid2))
			if (predecessors[machID2opnodes[mid2][opidx2]].contains(machID2opnodes[mid1][i])) {
				return false;
			}
	}

	// Check condition with successors
	for (int i = 0; i < opidx1; i++) {
		if ((mid1 == mid2 && machID2opnodes[mid1][i] != machID2opnodes[mid2][opidx2]) || (mid1 != mid2))
			if (successors[machID2opnodes[mid2][opidx2]].contains(machID2opnodes[mid1][i])) {
				return false;
			}
	}

	// Check condition with predecessors
	for (int i = opidx2 + 1; i < machID2opnodes[mid2].size(); i++) {
		if ((mid1 == mid2 && machID2opnodes[mid2][i] != machID2opnodes[mid1][opidx1]) || (mid1 != mid2))
			if (predecessors[machID2opnodes[mid1][opidx1]].contains(machID2opnodes[mid2][i])) {
				return false;
			}
	}

	// Check condition with successors
	for (int i = 0; i < opidx2; i++) {
		if ((mid1 == mid2 && machID2opnodes[mid2][i] != machID2opnodes[mid1][opidx1]) || (mid1 != mid2))
			if (successors[machID2opnodes[mid1][opidx1]].contains(machID2opnodes[mid2][i])) {
				return false;
			}
	}

	return true;
}

void TGVNSSchedulerPinSin::randomMoveOperations(const int &nops) {
	/**Algorithm:
	 * 
	 * 1. Select randomly two machines in the tool group.
	 * 2. Select randomly one operation on first machine.
	 * 3. Move the operations if it is correct
	 * 4. Recalculate correctly the processing times of the operations and the 
	 *	  start times taking into consideration the precedence constraints.
	 */

	QTextStream out(stdout);

	int moves = 0;

	QList<int> mids = tg->mid2idx.keys();
	int from_mid;
	int to_mid;
	int from_opidx;
	int to_opidx;
	bool move_possible = false;

	QSet<int> moved_operarions;

	int r = 2;

	do {
		//Debugger::info << "Starting the shake loop ..." << ENDL;

		// Select randomly machine IDs
		//Debugger::info << "Selecting randomly machines ..." << ENDL;
		do {
			from_mid = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
			//Debugger::info << mid1 << machID2opnodes[mid1].size() << ENDL;
		} while (machID2opnodes[from_mid].size() == 0);
		// Select randomly one operation on each of the machines
		from_opidx = Rand::rnd<Math::uint32>(0, machID2opnodes[from_mid].size() - 1);

		to_mid = mids[Rand::rnd<Math::uint32>(0, mids.size() - 1)];
		//Debugger::info << "Done selecting machines." << ENDL;

		// Select randomly one operation on each of the machines in view of r in [max(from_opidx-r,0), min(from_opidx+r, nmax)]
		int p = from_opidx;

		if (machID2opnodes[to_mid].size() - 1 < p) { // Move to the end

			to_opidx = machID2opnodes[to_mid].size();

		} else {

			to_opidx = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[to_mid].size() - 1));

		}


		// Check whether the move is possible
		//Debugger::info << "Checking whether the move is possible... : from_mid=" << from_mid << ", to_mid=" << to_mid << ", oper_id=" << pm->ops[machID2opnodes[from_mid].at(from_opidx)]->ID << ", from_opidx=" << from_opidx << ", to_opidx=" << to_opidx << ENDL;
		move_possible = movePossible(from_mid, to_mid, from_opidx, to_opidx);
		//Debugger::info << "Done checking whether the move is possible:" << ENDL;

		if (move_possible) {

			//qSwap(machID2opnodes[mid1][opidx1], machID2opnodes[mid2][opidx2]);
			//moved_operarions.insert(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID);
			//out << "Moving operation " << pm->ops[machID2opnodes[from_mid].at(from_opidx)]->ID << " from machine " << from_mid << " to machine " << to_mid << endl;
			machID2opnodes[to_mid].insert(to_opidx, machID2opnodes[from_mid].takeAt(from_opidx));

			moves++;

		} else { // Try again

			continue;

		}

		//Debugger::info << "Finished the shake loop." << ENDL;

	} while (moves < nops);
}

void TGVNSSchedulerPinSin::randomMoveOperationsSameMachine(const int& mid, const int &nops) {
	QTextStream out(stdout);

	if (machID2opnodes[mid].size() == 0) return;

	int moves = 0;

	QList<int> mids = tg->mid2idx.keys();
	int from_opidx;
	int to_opidx;
	bool move_possible = false;

	int r = 2;

	do {

		// Select randomly one operation on each of the machines
		from_opidx = Rand::rnd<Math::uint32>(0, machID2opnodes[mid].size() - 1);

		// Select randomly one operation on each of the machines in view of r in [max(from_opidx-r,0), min(from_opidx+r, nmax)]
		int p = from_opidx;

		to_opidx = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, machID2opnodes[mid].size() - 1));

		// Check whether the move is possible
		move_possible = movePossible(mid, mid, from_opidx, to_opidx);

		if (move_possible) {

			machID2opnodes[mid].insert(to_opidx, machID2opnodes[mid].takeAt(from_opidx));

			moves++;

		} else { // Try again

			continue;

		}

	} while (moves < nops);

}

bool TGVNSSchedulerPinSin::movePossible(const int &from_mid, const int &to_mid, const int &from_opidx, const int &to_opidx) {
	//return true;
	// Filter at least some moves which could be ineffective

	//if (machID2opnodes[from_mid].size() == 1) return false; // There was a bug when some machine had no operations assigned to it -> seems to be fixed

	int type = pm->ops[machID2opnodes[from_mid][from_opidx]]->type; // Type of the first operation
	Machine& mach = *(tg->_machines[tg->mid2idx[to_mid]]);
	if (!mach.type2speed.contains(type)) {
		return false;
	}

	/*
			if (tabus.contains(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID)) {
					if (tabus.value(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID).first == to_mid && tabus.value(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID).second == to_opidx) {
							return false;
					} else {
							// Insert the move to the tabu list
							tabus.insert(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID, QPair<int, int>(to_mid, to_opidx));
					}
			} else {
					tabus.insert(pm->ops[machID2opnodes[from_mid][from_opidx]]->ID, QPair<int, int>(to_mid, to_opidx));
			}

	 */

	// Check whether it is the same operation
	if (from_mid == to_mid && from_opidx == to_opidx) return false;

	// Check whether some of the precedence constraints on each one of the machines are not violated due to the potential move
	// The newly inserted node must not contain its predecessors after it and its successors before it

	QList<ListDigraph::Node> nodes;

	// Check condition with predecessors
	//Debugger::info << "1..." << ENDL;
	nodes.clear();
	int istart;
	if (from_mid == to_mid) {
		if (from_opidx < to_opidx) {
			istart = to_opidx + 1;
		} else {
			istart = to_opidx;
		}
	} else {
		istart = to_opidx;
	}
	for (int i = istart; i < machID2opnodes[to_mid].size(); i++) {
		if ((from_mid == to_mid && machID2opnodes[to_mid][i] != machID2opnodes[from_mid][from_opidx]) || (from_mid != to_mid))
			if (predecessors[machID2opnodes[from_mid][from_opidx]].contains(machID2opnodes[to_mid][i])) {
				return false;
			}
	}
	//Debugger::info << "1." << ENDL;

	// Check condition with successors
	//Debugger::info << "2..." << ENDL;
	nodes.clear();
	int iend;
	if (from_mid == to_mid) {
		if (from_opidx < to_opidx) {
			iend = to_opidx + 1;
		} else {
			iend = to_opidx;
		}
	} else {
		iend = to_opidx;
	}
	for (int i = 0; i < iend; i++) {
		if ((from_mid == to_mid && machID2opnodes[to_mid][i] != machID2opnodes[from_mid][from_opidx]) || (from_mid != to_mid))
			if (successors[machID2opnodes[from_mid][from_opidx]].contains(machID2opnodes[to_mid][i])) {
				return false;
			}
	}

	//Debugger::info << "2." << ENDL;

	return true;
}

void TGVNSSchedulerPinSin::initialAssignmentRND(ProcessModel& /*pm*/, ToolGroup &tg, QList<ListDigraph::Node> &opnodes) {
	QTextStream out(stdout);

	// Randomly assign equal number of operations to each machine
	QList<int> mids = tg.mid2idx.keys();
	int m = 0;

	machID2opnodes.clear();

	for (int i = 0; i < opnodes.size(); i++) {

		// Select randomly the machine ID, which the operation will be assigned to

		m = Rand::rnd<Math::uint32>(0, mids.size() - 1);

		// Assign the operation to the machine
		machID2opnodes[mids[m]].append(opnodes.at(i));
		//out << "Assigned operation: " << pm.ops[machID2opnodes[mids[m]].last()]->ID << endl;
		//getchar();
	}

	/*
	out << "Initial assignment:" << endl;
	for (int i = 0; i < mids.size(); i++) {
			out << "Machine " << mids[i] << ":" << endl;
			for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {
					out << pm.ops[machID2opnodes[mids[i]][j]]->ID << "->";
			}
			out << endl;
	}
	 */
}

void TGVNSSchedulerPinSin::initialAssignment(ProcessModel &pm, ToolGroup &tg, QList<ListDigraph::Node> &opnodes) {
	// Preserve the initial process model
	//sbhscheduler->preserveOpSchedState(pm, opnodes);

	QTextStream out(stdout);

	QList<ListDigraph::Node> ts = pm.topolSort();

	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	QMap<ListDigraph::Node, Operation> node2PrevOper;
	QMap < ListDigraph::Arc, bool> arc2PrevConj;
	QMap<ListDigraph::Arc, double> arc2PrevP;

	// Preserve the operation data
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		node2PrevOper[curNode] = *pm.ops[curNode];
	}

	// Preserve the arcs data
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		arc2PrevConj[curArc] = pm.conjunctive[curArc];
		arc2PrevP[curArc] = pm.p[curArc];
	}


	//Debugger::info << "Running the iniScheduler scheduler..." << ENDL;
	iniScheduler->schedule(pm, tg, opnodes, *terminals, *dloc, *tgselection);
	//Debugger::info << "Done running the iniScheduler scheduler." << ENDL;  	


	// The selection is found automatically

	int n_selection_nodes = tgselection->opNode2SchedOps.size();
	machID2opnodes.clear();
	machID2opnodes = assignmentFromSelection(*tgselection);

	/*
	for (int i = 0; i < tgselection->selection.size(); i++) {
		//Debugger::info << "(" << pm.ops[tgselection->selection[i].first]->ID << ";" << pm.ops[tgselection->selection[i].second]->ID << ")" << ENDL;
		if (!machID2opnodes[pm.ops[tgselection->selection[i].first]->machID].contains(tgselection->selection[i].first)) {
			n_selection_nodes++;
			machID2opnodes[pm.ops[tgselection->selection[i].first]->machID].append(tgselection->selection[i].first);

			//Debugger::warn << pm.ops[tgselection->selection[i].first]->ID << ENDL;
		}

		if (!machID2opnodes[pm.ops[tgselection->selection[i].second]->machID].contains(tgselection->selection[i].second)) {
			n_selection_nodes++;
			machID2opnodes[pm.ops[tgselection->selection[i].second]->machID].append(tgselection->selection[i].second);

			//Debugger::warn << pm.ops[tgselection->selection[i].second]->ID << ENDL;
		}
	}
	 */

	if (n_selection_nodes != opnodes.size()) {
		Debugger::warn << " Problem during the initial assignment! " << ENDL;
		Debugger::warn << " Present nodes: " << opnodes.size() << ENDL;
		for (int i = 0; i < opnodes.size(); i++) {
			Debugger::warn << pm.ops[opnodes[i]]->ID << ENDL;
		}
		Debugger::warn << " Assigned nodes: " << n_selection_nodes << ENDL;
		Debugger::err << "Failure during the initial assignment!" << ENDL;
	}


	// DEBUG

	insertSelection(*tgselection);
	cout << "TGVNSSchedulerPinSin::initialAssignment : TWT before restoring the PM's state : " << TWT()(pm) << endl;
	removeSelection();

	// END DEBUG


	// Restore the operation data
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;

		*pm.ops[curNode] = node2PrevOper[curNode];
	}

	// Restore the arcs data
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;

		pm.conjunctive[curArc] = arc2PrevConj[curArc];
		pm.p[curArc] = arc2PrevP[curArc];
	}



	// DEBUG

	insertSelection(*tgselection);
	cout << "TGVNSSchedulerPinSin::initialAssignment : TWT after restoring the PM's state and inserting the selection : " << TWT()(pm) << endl;
	removeSelection();
	cout << "TGVNSSchedulerPinSin::initialAssignment : TWT after removing the selection : " << TWT()(pm) << endl;

	// END DEBUG



	// Restore the initial scheduling state of the affected operations
	//sbhscheduler->restoreOpSchedState(pm, opnodes);
}

void TGVNSSchedulerPinSin::localSearch() {
	QTextStream out(stdout);
	//return;
	/** Algorithm:
	 * 
	 * 1. For every fixed operation try to swap it with all other operations
	 *  
	 */

	//Debugger::info << "Entered local search..." << ENDL;

	for (QMap<ListDigraph::Node, Operation>::const_iterator iter = tgselection->opNode2SchedOps.begin(); iter != tgselection->opNode2SchedOps.end(); iter++) {
		if (Math::abs((Math::intUNI)iter.value().ID) > 1000000000) {
			out << iter.value() << endl;
			//Debugger::err << "TGVNSSchedulerPinSin::localSearch : Build invalid selection!" << ENDL;
		}
	}

	TGSelection besttgselection = *tgselection;
	TGSelection prevTGSelection = *tgselection;
	QHash<int, QList<ListDigraph::Node> > bestMachID2opnodes = machID2opnodes;
	QHash<int, QList<ListDigraph::Node> > prevMachID2opnodes = machID2opnodes;

	double bestobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
	double curobj;

	out << "TGVNSSchedulerPinSin::localSearch : Starting with objective : " << bestobj << endl;
	out << "TGVNSSchedulerPinSin::localSearch : Current best found objective : " << cur_obj << endl;
	getchar();

	QList<int> mids = machID2opnodes.keys();


	//for (int l = 0; l < opnodes->size(); l++) {

	/*

	for (int i = 0; i < mids.size(); i++) {
		for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {
			for (int othi = i; othi < mids.size(); othi++) {
				for (int othj = j + 1; othj < machID2opnodes[mids[othi]].size(); othj++) {
					// Try to swap the two operations
					if (swapPossible(mids[i], mids[othi], j, othj)) {
						qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
						// Accept or decline
						scheduleCurOpSeq();
						getTGSelection();
						curobj = localObj(*pm, *opnodes, *terminals, *dloc);
						tgselection->localobj = curobj;

						if (curobj < bestobj) {
							bestobj = curobj;
							besttgselection = *tgselection;
						} else {// Swap back
							qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
	 *tgselection = besttgselection;
						}
					}
				}
			}
		}

	}

	 */

	//return;
	//}

	bool move_possible = false;

	//for (int l = 0; l < opnodes->size(); l++) {

	// IMPORTANT: BUG - sizes in the loops change dynamically (seems to be solved).

	//Debugger::info << "1..." << ENDL;

	int ctr = 0;

	for (int i = 0; i < mids.size(); i++) {

		for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {

			for (int othi = i; othi < mids.size(); othi++) {

				for (int othj = j + 1; othj < machID2opnodes[mids[othi]].size(); othj++) {

					prevTGSelection = *tgselection;
					prevMachID2opnodes = machID2opnodes;

					// Avoid moving from the empty machine
					if (machID2opnodes[mids[i]].size() == 0) continue;
					// Try to move the operation
					//Debugger::info << "Checking whether the move is possible... : from_mid=" << mids[i] << ", to_mid=" << mids[othi] << ", from_opidx=" << j << ", to_opidx=" << othj << " from_m.size=" << machID2opnodes[mids[i]].size() << ", to_m.size=" << machID2opnodes[mids[othi]].size() << ENDL;
					move_possible = movePossible(mids[i], mids[othi], j, othj);
					//Debugger::info << "Done checking whether the move is possible." << ENDL;

					//Debugger::info << "3..." << ENDL;
					if (move_possible /*&& ctr < 100000000000000000*/) {

						ctr++;

						//Debugger::info << "3.1 ..." << ENDL;
						machID2opnodes[mids[othi]].insert(othj, machID2opnodes[mids[i]].takeAt(j));

						//qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
						// Accept or decline
						//Debugger::info << "3.1." << ENDL;
						//Debugger::info << "3.2 ..." << ENDL;
						scheduleCurOpSeq();
						//Debugger::info << "3.2." << ENDL;
						curobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

						//Debugger::info << "3.3 ..." << ENDL;
						if (curobj < bestobj) {

							bestMachID2opnodes = machID2opnodes;
							besttgselection = *tgselection;

							out << "TGVNSSchedulerPinSin::localSearch : Improved solution : " << bestobj << endl;
							getchar();

							bestobj = curobj;

							if (j + 1 >= machID2opnodes[mids[i]].size()) {

								j = machID2opnodes[mids[i]].size() - 1;
								othj = j + 1;

							} else {

								//othj--;

							}

							besttgselection = *tgselection;

						} else {// Swap back

							machID2opnodes = prevMachID2opnodes;

							*tgselection = prevTGSelection;

						}
						//Debugger::info << "3.3." << ENDL;
					} // Performing a move
					//Debugger::info << "3." << ENDL;
				} // Iterating over operations j

			} // Iterating over machines j

		} // Iterating over operations i

	} // Iterating over machines i

	//Debugger::info << "1." << ENDL;

	//}

	*tgselection = besttgselection;
	machID2opnodes = bestMachID2opnodes;

	//Debugger::info << "Finished local search." << ENDL;

}

void TGVNSSchedulerPinSin::localSearchSimple() {
	QTextStream out(stdout);
	//return;
	/** Algorithm:
	 * 
	 * 1. For every fixed operation try to swap it with all other operations
	 *  
	 */

	//Debugger::info << "Entered local search..." << ENDL;

	for (QMap<ListDigraph::Node, Operation>::const_iterator iter = tgselection->opNode2SchedOps.begin(); iter != tgselection->opNode2SchedOps.end(); iter++) {
		if (Math::abs((Math::intUNI)iter.value().ID) > 1000000000) {
			out << iter.value() << endl;
			//Debugger::err << "TGVNSSchedulerPinSin::localSearch : Build invalid selection!" << ENDL;
		}
	}

	TGSelection besttgselection = *tgselection;
	TGSelection prevTGSelection = *tgselection;
	QHash<int, QList<ListDigraph::Node> > bestMachID2opnodes = machID2opnodes;
	QHash<int, QList<ListDigraph::Node> > prevMachID2opnodes = machID2opnodes;

	double bestobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
	double curobj;

	out << "TGVNSSchedulerPinSin::localSearchSimple : Starting with objective : " << bestobj << endl;
	out << "TGVNSSchedulerPinSin::localSearchSimple : Current best found objective : " << cur_obj << endl;
	//getchar();

	QList<int> mids = machID2opnodes.keys();

	/*

	for (int i = 0; i < mids.size(); i++) {
		for (int j = 0; j < machID2opnodes[mids[i]].size(); j++) {
			for (int othi = i; othi < mids.size(); othi++) {
				for (int othj = j + 1; othj < machID2opnodes[mids[othi]].size(); othj++) {
					// Try to swap the two operations
					if (swapPossible(mids[i], mids[othi], j, othj)) {
						qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
						// Accept or decline
						scheduleCurOpSeq();
						getTGSelection();
						curobj = localObj(*pm, *opnodes, *terminals, *dloc);
						tgselection->localobj = curobj;

						if (curobj < bestobj) {
							bestobj = curobj;
							besttgselection = *tgselection;
						} else {// Swap back
							qSwap(machID2opnodes[mids[i]][j], machID2opnodes[mids[othi]][othj]);
	 *tgselection = besttgselection;
						}
					}
				}
			}
		}

	}

	 */

	for (int i = 0; i < mids.size(); i++) {

		int ctr = 0;

		int curMID = mids[i];

		prevTGSelection = *tgselection;
		prevMachID2opnodes = machID2opnodes;

		// Avoid moving from the empty machine
		if (machID2opnodes[curMID].size() == 0) continue;

		while (ctr < 1000) {

			ctr++;

			double rnd = Rand::rnd<double>();
			if (rnd <= 0.5) {
				randomSwapOperationsSameMachine(curMID, k);
			} else {
				randomMoveOperationsSameMachine(curMID, k);
			}

			scheduleCurOpSeq();

			curobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);
			tgselection->localobj = curobj;

			if (curobj < bestobj) { // Save the current better solution

				out << "TGVNSSchedulerPinSin::localSearchSimple : Solution improved : " << curobj << endl;

				getchar();

				bestobj = curobj;

				bestMachID2opnodes = machID2opnodes;

				besttgselection = *tgselection;

			} else { // Restore

				//out << "TGVNSSchedulerPinSin::localSearchSimple : Solution declined : " << curobj << endl;

				machID2opnodes = prevMachID2opnodes;

				*tgselection = prevTGSelection;

			}

		}

	}

	machID2opnodes = bestMachID2opnodes;
	*tgselection = besttgselection;

	out << "TGVNSSchedulerPinSin::localSearchSimple : Finished! " << endl;
	getchar();

}

void TGVNSSchedulerPinSin::localSearchLS() {
	return;

	LocalSearchPM ls;

	// Mark the nodes which can be moved during the local search
	QMap < ListDigraph::Node, bool> nodeMovable;
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm->ops[curNode]);

		if (curOp.toolID == tg->ID) { // This node can be moved

			nodeMovable[curNode] = true;

		} else { // This node can not be moved

			nodeMovable[curNode] = false;

		}

	}

	Debugger::info << ENDL << "TGVNSSchedulerPinSin::localSearchLS : Initial objective by TGVNSSchedulerPinSin (partial schedule, TWT (exp. LB), without assignment) : " << TWT()(*pm) << ENDL;

	TGSelection& selection = (TGSelection&) * tgselection;

#ifdef DEBUG

	insertSelection(selection);
	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Initial objective in tgselection.localobj : " << tgselection->localobj << ENDL;
	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Global objective  (recalculated, TWT, with selection, before LS) : " << TWT()(*pm) << ENDL;
	removeSelection();

#endif 

	// Save the state of the PM (without the initial selection!!!)
	QMap<ListDigraph::Node, Operation> savedOps; // Saved operation data
	//QList<QPair<ListDigraph::Node, ListDigraph::Node> > savedArcs; // Arcs of the graph which were saved previously
	QList<double> savedP; // Arc lengths of the previously saved arcs
	//QList<bool> savedConjunctive; // Indicator of conjunctiveness of the previously saved arcs
	QMap < ListDigraph::Arc, bool> savedConjunctive; // Used to momorize which arc is 

	// Preserve the information about the operations
	savedOps.clear();
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) * pm->ops[curNode];

		savedOps[curNode] = curOp;
	}

	// Preserve the information about the arcs for the current state of the PM
	//savedArcs.clear();
	savedP.clear();
	//savedConjunctive.clear();
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) { // IMPORTANT!!! DO NOT TOUCH THE ARCS WHICH ARE ALREADY IN THE PM, OTHERWISE DELETING AND RESTORING THEM WILL LEAD TO INVALID ARCS IN THE PM -> PROBLEMS!!!
		ListDigraph::Arc curArc = ait;
		//ListDigraph::Node curStartNode = pm->graph.source(curArc);
		//ListDigraph::Node curEndNode = pm->graph.target(curArc);

		//savedArcs.append(QPair<ListDigraph::Node, ListDigraph::Node>(curStartNode, curEndNode));

		savedP.append(pm->p[curArc]);

		//savedConjunctive.append(pm->conjunctive[curArc]);

		savedConjunctive[curArc] = pm->conjunctive[curArc];

		// Mark all arcs of the current PM as conjunctive so that the LS doesn't modify them preserving their validness
		pm->conjunctive[curArc] = true;
	}


	// Perform scheduling
	insertSelection(selection); // Inserting the INITIAL selection of the initial solution in order to improve it later (IT WILL BE MODIFIED THROUGH THE LS)

	// Create the resources for the local search
	Resources rc;

	rc.ID = 1;
	rc << new ToolGroup(*tg);

	ls.setPM(pm); // IMPORTANT!!! The PM will change during the LS - > the information should be preserved
	ls.setResources(&rc);
	ls.setMovableNodes(nodeMovable);
	ls.checkCorrectness(false);
	int lsIter = Math::max(Math::round(4000.0 * (double(iter()) / double(maxIter()))), Math::round(4000.0 * (double(_curtime.elapsed()) / double(maxTimeMs()))));
	ls.maxIter(lsIter);
	//ls.maxTimeMs(100);

	cout << "k = " << k << endl;

	ls.run();

	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Global objective in PM after the LS (recalculated, TWT, state after LS) : " << TWT()(*pm) << ENDL;

	// Update the selection according to the new solution -> the old selection is not appropriate
	getTGSelection();

	// Restore the state of the PM as before the LS (without the current selection!!!)

	// Remove all disjunctive arcs in the graph dealing ONLY with this machine group
	QList<ListDigraph::Arc> arcsToRem;
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		ListDigraph::Node curStartNode = pm->graph.source(ait);
		Operation& curStartOp = (Operation&) *(pm->ops[curStartNode]);

		if (!pm->conjunctive[ait] && curStartOp.toolID == tg->ID) arcsToRem.append(curArc);
	}

	for (int i = 0; i < arcsToRem.size(); i++) {
		ListDigraph::Arc curArc = arcsToRem[i];
		pm->graph.erase(curArc);
	}

	// Restore the information about the operations
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		*(pm->ops[curNode]) = savedOps[curNode];
	}

	// Restore the lengths of all arcs
	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
		ListDigraph::Node curNode = nit;
		double curP = pm->ops[curNode]->p();
		for (ListDigraph::OutArcIt oait(pm->graph, curNode); oait != INVALID; ++oait) {
			ListDigraph::Arc curArc = oait;
			pm->p[curArc] = -curP;
		}
	}

	// Restore the conjunctiveness of the arcs
	for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
		ListDigraph::Arc curArc = ait;
		pm->conjunctive[curArc] = savedConjunctive[curArc];
	}


	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Global objective in PM after the LS (recalculated, TWT, no selection, after LS) : " << TWT()(*pm) << ENDL;
	insertSelection(selection);
	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Global objective in PM after the LS (recalculated, TWT, with selection, after LS) : " << TWT()(*pm) << ENDL;
	removeSelection();

	// Update the objective in the selection
	tgselection->localobj = localObj(*pm, *opnodes, *tgselection, *terminals, *dloc);

	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Global objective in tgselection after the LS : " << tgselection->localobj << ENDL;

	Debugger::info << "TGVNSSchedulerPinSin::localSearchLS : Global objective without the selection -> PM will be used in other SSPs : " << TWT()(*pm) << ENDL;

}

double TGVNSSchedulerPinSin::localObj(ProcessModel &pm, QList<ListDigraph::Node> &opnodes, TGSelection& tgselection, QList<ListDigraph::Node>&, QHash<int, QList<double> >&) {
	QTextStream out(stdout);

	// IMPORTANT!!! It is assumed that the correct selection has already been found

	TGSelection& selection = (TGSelection&) tgselection;

	//int curTGID = selection.tgID;
	//cout << "TGWEODScheduler::localObj : Calculating objective for TG : " << curTGID << endl;
	//cout << "Arcs in the selection : " << selection.selection.size() << endl;

	insertSelection(selection);

	//out << pm << endl;

	double res = /*TWT()(pm); //*/ UTWT()(pm, opnodes, locD);

	removeSelection();

	//cout << "TWT : " << res << endl;

	return res;
}

QHash<int, QList<ListDigraph::Node> > TGVNSSchedulerPinSin::assignmentFromSelection(TGSelection & selection) {
	QTextStream out(stdout);

	QHash<int, QList<ListDigraph::Node> > res;

	QList<ListDigraph::Node> tsNodes = selection.opNode2SchedOps.keys();

	// Sort the tsNodes so that no selection constraint is violated
	for (int i = 0; i < selection.selection.size(); i++) {

		ListDigraph::Node curStartNode = selection.selection[i].first;
		ListDigraph::Node curEndNode = selection.selection[i].second;

		int curStartNodeIdx = tsNodes.indexOf(curStartNode);
		int curEndNodeIdx = tsNodes.indexOf(curEndNode);

		if (curStartNodeIdx > curEndNodeIdx) { // Precedence violated - > swap the nodes
			tsNodes.move(curStartNodeIdx, curEndNodeIdx);

			//out << "Moving from " << curStartNodeIdx << " to " << curEndNodeIdx << endl;

			i = -1; // Since i will be increased at the end of the loop
		}

	}


#ifdef DEBUG 

	for (int i = 0; i < selection.selection.size(); i++) {

		ListDigraph::Node curStartNode = selection.selection[i].first;
		ListDigraph::Node curEndNode = selection.selection[i].second;

		int curStartNodeIdx = tsNodes.indexOf(curStartNode);
		int curEndNodeIdx = tsNodes.indexOf(curEndNode);

		if (curStartNodeIdx > curEndNodeIdx) { // Precedence violated - > swap the nodes
			out << "Idx start : " << curStartNodeIdx << endl;
			out << "Idx end : " << curEndNodeIdx << endl;
			Debugger::err << "TGVNSSchedulerPinSin::assignmentFromSelection : Precedence violated!!!" << ENDL;
		}

	}

#endif 

	// Update the assignment
	for (int i = 0; i < tsNodes.size(); i++) {

		ListDigraph::Node curNode = tsNodes[i];
		int curMachID = selection.opNode2SchedOps[curNode].machID;

		res[curMachID].append(curNode);

		//out << "Assigning operation " << selection.opNode2SchedOps[curNode] << " to machine " << curMachID << endl;

	}


	return res;
}

/**  **********************************************************************  **/