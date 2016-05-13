/* 
 * File:   VNSScheduler.cpp
 * Author: DrSobik
 * 
 * Created on June 13, 2014, 6:15 PM
 */

#include "VNSScheduler.h"

VNSScheduler::VNSScheduler() {

	cs.setParent((Scheduler*)this);
	ls.setParent((Scheduler*)this);

	timeBasedAcceptance = false;
}

VNSScheduler::VNSScheduler(VNSScheduler& orig) : Scheduler(orig), IterativeAlg(orig), cs(orig.cs), ls(orig.ls) {

	cs.setParent((Scheduler*)this);
	ls.setParent((Scheduler*)this);

	timeBasedAcceptance = orig.timeBasedAcceptance;
	
}

VNSScheduler::~VNSScheduler() {
}

void VNSScheduler::init() {
	cs.init();
	ls.init();

	// Initialize the iterative process
	IterativeAlg::init();

	// Initialize own variables
	kCur = 1;
	kStep = 1;
	kMax = -1;

	smR = 2;

	curMachID2opNodes.clear();
	prevMachID2opNodes.clear();
	bestMachID2opNodes.clear();

	prevObj = Math::MAX_DOUBLE;
	curObj = Math::MAX_DOUBLE;
	bestObj = Math::MAX_DOUBLE;

	predecessors.clear();
	successors.clear();
}

Clonable* VNSScheduler::clone() {
	return new VNSScheduler(*this);
}

void VNSScheduler::preservePM() {

	origArcs.clear();
	origConj.clear();
	origLen.clear();
	origOpers.clear();

	// Preserve original arcs
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {

		origArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(ait), pm.graph.target(ait));
		origConj << pm.conjunctive[ait];
		origLen << pm.p[ait];

	}

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		Operation curOp;
		curOp.copy(*(pm.ops[nit]));
		origOpers.insert(nit, curOp);
	}

}

void VNSScheduler::restorePM() {

	if (origArcs.size() == 0 && origOpers.size() == 0) return;

	// Remove all arcs
	QList<ListDigraph::Arc> arcsToRem;

	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {
		arcsToRem << ait;
	}

	for (int i = 0; i < arcsToRem.size(); i++) {
		pm.graph.erase(arcsToRem[i]);
	}

	// Restore the original operation
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
		pm.ops[nit]->copy(origOpers.value(nit));
	}

	// Restore the original arcs
	for (int i = 0; i < origArcs.size(); i++) {
		ListDigraph::Arc curArc = pm.graph.addArc(origArcs[i].first, origArcs[i].second);
		pm.conjunctive[curArc] = origConj[i];
		pm.p[curArc] = origLen[i];
	}

	origArcs.clear();
	origConj.clear();
	origLen.clear();
	origOpers.clear();

}

void VNSScheduler::scheduleActions() {
	// Parse options
	if (options.contains("LS_CHK_COR")) {
		if (options["LS_CHK_COR"] == "true") {
			ls.checkCorrectness(true);
			//cout << "Checking correctness" << endl;
		} else {
			ls.checkCorrectness(false);
			//cout << "NOT checking correctness" << endl;
		}
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : LS_CHK_COR not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_ITER")) {
		ls.maxIter(options["LS_MAX_ITER"].toInt());
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_TIME_MS")) {
		ls.maxTimeMs(options["LS_MAX_TIME_MS"].toInt());
	}

	// VNSG parameters
	if (options.contains("VNSG_MAX_ITER")) {
		maxIter(options["VNSG_MAX_ITER"].toInt());
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : VNSG_MAX_ITER not specified!" << ENDL;
	}

	if (options.contains("VNSG_MAX_ITER_DECL")) {
		maxIterDecl(options["VNSG_MAX_ITER_DECL"].toInt());
	}

	if (options.contains("VNSG_MAX_TIME_MS")) {
		maxTimeMs(options["VNSG_MAX_TIME_MS"].toInt());
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : VNSG_MAX_TIME_MS not specified!" << ENDL;
	}
	
	if (options.contains("VNSG_TIME_BASED_ACCEPTANCE")) {
		if (options["VNSG_TIME_BASED_ACCEPTANCE"] == "true") {
			setTimeBasedAcceptance(true);
			//cout << "Checking correctness" << endl;
		} else {
			setTimeBasedAcceptance(false);
			//cout << "NOT checking correctness" << endl;
		}
	} else {
		Debugger::err << "CombinedSchedulerLS::scheduleActions : VNSG_TIME_BASED_ACCEPTANCE not specified!" << ENDL;
	}
	
	if (options.contains("VNSG_K_MAX")) {
		kMax = options["VNSG_K_MAX"].toInt();
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : VNSG_K_MAX not specified!" << ENDL;
	}

	if (options.contains("VNSG_K_STEP")) {
		kStep = options["VNSG_K_STEP"].toInt();
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : VNSG_K_STEP not specified!" << ENDL;
	}

	if (options.contains("VNSG_SMR")) {
		smR = options["VNSG_SMR"].toInt();
	} else {
		Debugger::err << "VNSScheduler::scheduleActions : VNSG_SMR not specified!" << ENDL;
	}

	if (options.contains("LS_CRIT_NODES_UPDATE_FREQ")) {
		ls.setCritNodesUpdateFreq(options["LS_CRIT_NODES_UPDATE_FREQ"].toInt());
	} else {
		Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_CRIT_NODES_UPDATE_FREQ not specified!" << ENDL;
	}
	
	if (options.contains("LS_BEST_POS_TO_MOVE")) {
		if (options["LS_BEST_POS_TO_MOVE"] == "true") {
			ls.setBestPosToMove(true);
			//cout << "Checking correctness" << endl;
		} else {
			ls.setBestPosToMove(false);
			//cout << "NOT checking correctness" << endl;
		}
	} else {
		Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_BEST_POS_TO_MOVE not specified!" << ENDL;
	}
	
	// Run the Combined scheduler
	cs.pm = pm;
	cs.rc = rc;
	cs.sched = sched;
	cs.options = options;

	cs.schedule();

	// Get the best PM found by the CombinedScheduler
	pm = cs.bestPM();

	//	getchar();

	int iterCtr = 0;
	for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
		iterCtr += cs.bestPerformingStat[i];
	}

	//	ls.setPM(&pm);
	//	ls.debugCheckPMCorrectness("After initial solution");

	// Set the objective values
	curObj = (*obj)(pm);
	prevObj = curObj;
	bestObj = curObj;

	// Get the solution representation from PM
	solutionFromPM(pm);

	// Set this solution to be the best and the previous
	prevMachID2opNodes = curMachID2opNodes;
	bestMachID2opNodes = curMachID2opNodes;

	// Now we have an initial schedule => run the VNS algorithm

	IterativeAlg::run();

}

void VNSScheduler::preprocessingActions() {

	QTextStream out(stdout);

	//	out << "VNSScheduler::preprocessingActions : Finding predecessors/successors ... " << endl;

	// For each node, find a set of machines able to process it
	//	findNode2MachIDs();

	// Find predecessors and successors
	//	findPredecessorsSameMachs();

	//	findSuccessorsSameMachs();

	//	out << "VNSScheduler::preprocessingActions : Finding predecessors/successors done. " << endl;

}

void VNSScheduler::postprocessingActions() {

	QTextStream out(stdout);

	// Assign the best schedule to the result
	*sched = bestSchedule;

	out << "VNSScheduler::postprocessingActions : Found best solution : " << bestObj << endl;
	//getchar();

}

void VNSScheduler::stepActions() {
	QTextStream out(stdout);

	static int i = 1;

	out << "VNSG : " << i << endl;

	i++;
	//getchar();

	// Preserve the current sequences of operations on the machines
	prevMachID2opNodes = curMachID2opNodes;
	prevObj = curObj;

	// Preserve the PM's state
	preservePM();

	//	ls.setPM(&pm);
	//	ls.debugCheckPMCorrectness("Before shake");

	// Generate new neighboring encoded solution
	if (!dag(pm.graph)) {
		Debugger::err << "NOT DAG BEFORE shaking!!!" << ENDL;
	}
	shake();

	if (!dag(pm.graph)) {
		Debugger::err << "NOT DAG AFTER shaking!!!" << ENDL;
	}

	QList<ListDigraph::Node> ts = pm.topolSort();

	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	//	ls.setPM(&pm);
	//	ls.debugCheckPMCorrectness("After shake");


	// Improve the current PM by the local search
	if (ls.maxIter() > 0) {
		pm.save();

		ls.setObjective(obj);
		ls.setPM(&pm);
		ls.setResources(&rc);
		//ls.setRandGen(&randGen);

		ls.run();

		pm.restore();

		//		ls.debugCheckPMCorrectness("After reoptimizing");
	}

}

void VNSScheduler::assessActions() {
	curObj = (*obj)(pm);
}

bool VNSScheduler::acceptCondition() {

	if (curObj <= bestObj) {

		acceptedWorse = false;

		return true;

	} else {
		
		double expProgress = (timeBasedAcceptance) ? Math::pow((double) timeMs() / double(maxTimeMs()), 1.0) : Math::pow((double) iter() / double(maxIter()), 1.0);
		
		Debugger::info << "Time-based : " << ((timeBasedAcceptance) ? "true" : "false") << ENDL;
		Debugger::info << "expProgress : " << expProgress << ENDL;
		
		if (Rand::rnd<double>(0.0, 1.0) < Math::exp(-(curObj - prevObj) / (1.0 - expProgress))) {

			acceptedWorse = true;

			return true;

		} else {

			acceptedWorse = false;

			return false;

		}
		 //*/

		/*
		if (curObj <= 1.25 * bestObj) {

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
		 //*/

	}

}

void VNSScheduler::acceptActions() {
	if (!acceptedWorse) {
		Debugger::info << "Iter : " << (int) iter() << " : ";
		Debugger::info << "VNSScheduler::acceptActions : Accepting solution with objective : " << curObj << ENDL;
		Debugger::info << "VNSScheduler::acceptActions : Best solution with objective : " << bestObj << ENDL;
		//Debugger::info << "TGVNSScheduler::acceptActions : Previous objective : " << prev_obj << ENDL;

		bestObj = curObj;

		bestMachID2opNodes = curMachID2opNodes;

		// Save the best schedule
		bestSchedule.fromPM(pm, *obj);

	} else { // Accepted a worse solution

		Debugger::info << "Iter : " << (int) iter() << " : ";
		Debugger::info << "VNSScheduler::acceptActions : Accepting a WORSE solution with objective : " << curObj << ENDL;
		Debugger::info << "VNSScheduler::acceptActions : Best solution with objective : " << bestObj << ENDL;
		//Debugger::info << "TGVNSScheduler::acceptActions : Previous objective : " << prev_obj << ENDL;

	}

	kCur = 1;

	solutionFromPM(pm);
}

void VNSScheduler::declineActions() {
	Debugger::info << "VNSScheduler::declineActions : Declining solution with objective : " << curObj << ENDL;
	Debugger::info << "VNSScheduler::declineActions : Previous objective : " << prevObj << ENDL;
	Debugger::info << "VNSScheduler::declineActions : Best solution with objective : " << bestObj << ENDL;

	// Set the previous objective
	curObj = prevObj;

	// Restore the previous sequence of the operations on the machines
	curMachID2opNodes = prevMachID2opNodes;

	// Update k
	kCur = kCur + kStep;
	if (kCur > kMax) kCur = 1; //Math::max(kmax / 2, 1); //(1 + kmax) / 2;

	// IMPORTANT!!! Restore the previous selection which was before the shake
	restorePM();

}

void VNSScheduler::stopActions() {

}

void VNSScheduler::shake() {
	QTextStream out(stdout);

	//out << "VNSScheduler::shake : Shaking..." << endl;

	double rnd = Rand::rnd<double>();

	if (rnd <= 0.50) {

		// Perform k random swaps of the operation nodes
		randomSwapOperations(kCur);

	} else if (rnd <= 0.90) {

		// Perform k random swaps of the operation nodes
		randomMoveOperations(kCur);

	} else if (rnd <= 0.95) {

		// Perform k random swaps of the operation nodes
		randomSwapOperations(kCur);

		// Perform k random swaps of the operation nodes
		randomMoveOperations(kCur);

	} else {

		// Perform k random swaps of the operation nodes
		randomMoveOperations(kCur);

		// Perform k random swaps of the operation nodes
		randomSwapOperations(kCur);

	}

	//out << "VNSScheduler::shake : Shaking finished." << endl;
}

void VNSScheduler::randomSwapOperations(const int &nOps, const bool& sameMachOnly) {
	QTextStream out(stdout);

	//	out << "Performing swaps ... " << endl;
	//	getchar();

	int swaps = 0;

	int mID1 = 0;
	int mID2 = 0;
	int opIdx1 = 0;
	int opIdx2 = 0;
	int r = smR;

	do {
		//Debugger::info << "Starting the shake loop ..." << ENDL;

		// Select randomly a machine group
		int curTGID = 0;
		do {
			curTGID = rc.tools[Rand::rnd<Math::uint32>(0, rc.tools.size() - 1)]->ID;
		} while (curTGID == 0);

		// The machine IDs within the selected machine group
		QList<int> mIDs = rc(curTGID).mid2idx.keys();

		// Select randomly machine IDs
		//Debugger::info << "Selecting randomly machines ..." << ENDL;
		do { // To select a machine which is not empty
			mID1 = mIDs[Rand::rnd<Math::uint32>(0, mIDs.size() - 1)];
		} while (curMachID2opNodes[mID1].size() == 0);

		// Select randomly one operation on each of the machines m1
		opIdx1 = Rand::rnd<Math::uint32>(0, curMachID2opNodes[mID1].size() - 1);

		ListDigraph::Node nodeToSwap1 = curMachID2opNodes[mID1][opIdx1];
		Operation& opToSwap1 = (Operation&) (*pm.ops[nodeToSwap1]);
		ListDigraph::Node curPred1 = (opIdx1 > 0) ? curMachID2opNodes[mID1][opIdx1 - 1] : INVALID;
		ListDigraph::Node curSucc1 = (opIdx1 < curMachID2opNodes[mID1].size() - 1) ? curMachID2opNodes[mID1][opIdx1 + 1] : INVALID;

		// Select randomly the other machine which can process the selected operation
		// Select a machine able to process the type of the operation
		if (!sameMachOnly) { // The move can be performed between any machines

			do {

				mID2 = mIDs[Rand::rnd<Math::uint32>(0, mIDs.size() - 1)];

			} while (!rc(curTGID, mID2).type2speed.contains(opToSwap1.type) || curMachID2opNodes[mID2].size() == 0);

		} else { // The move is performed on the same machine

			mID2 = mID1;

		}

		// Select randomly one operation on each of the machines m2 in view of r in [max(opidx1-r,0), min(opidx1+r, nmax)]
		int p = opIdx1;

		if (curMachID2opNodes[mID2].size() - 1 < p) { // Do not perform any swap
			continue;
		} else {
			opIdx2 = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, curMachID2opNodes[mID2].size() - 1));
		}

		ListDigraph::Node nodeToSwap2 = curMachID2opNodes[mID2][opIdx2];
		Operation& opToSwap2 = (Operation&) (*pm.ops[nodeToSwap2]);

		// Check whether opToSwap2 can be processed by the second machine
		if (!rc(curTGID, mID1).type2speed.contains(opToSwap2.type)) { // Swap is not possible
			continue;
		}

		ListDigraph::Node curPred2 = (opIdx2 > 0) ? curMachID2opNodes[mID2][opIdx2 - 1] : INVALID;
		ListDigraph::Node curSucc2 = (opIdx2 < curMachID2opNodes[mID2].size() - 1) ? curMachID2opNodes[mID2][opIdx2 + 1] : INVALID;

		// New potential predecessor/successor nodes
		ListDigraph::Node newPred1 = (opIdx2 == 0) ? INVALID : curMachID2opNodes[mID2][opIdx2 - 1]; // New predecessor of nodeToMove1 on mID2
		ListDigraph::Node newSucc1 = (opIdx2 == curMachID2opNodes[mID2].size() - 1) ? INVALID : curMachID2opNodes[mID2][opIdx2 + 1]; // New successor of nodeToMove1 on mID2

		ListDigraph::Node newPred2 = (opIdx1 == 0) ? INVALID : curMachID2opNodes[mID1][opIdx1 - 1]; // New predecessor of nodeToMove2 on mID1
		ListDigraph::Node newSucc2 = (opIdx1 == curMachID2opNodes[mID1].size() - 1) ? INVALID : curMachID2opNodes[mID1][opIdx1 + 1]; // New successor of nodeToMove2 on mID1

		// In case these nodes are neighbors on the same machine
		if (nodeToSwap1 == newPred1) newPred1 = nodeToSwap2;
		if (nodeToSwap1 == newSucc1) newSucc1 = nodeToSwap2;
		if (nodeToSwap2 == newPred2) newPred2 = nodeToSwap1;
		if (nodeToSwap2 == newSucc2) newSucc2 = nodeToSwap1;

		if (nodeToSwap1 == nodeToSwap2) { // No move is possible
			continue;
		}

		// DEBUG
		if (!dag(pm.graph)) {
			Debugger::err << "NOT DAG BEFORE the swap!!!" << ENDL;
		}

		//	ls.setPM(&pm);
		//	ls.debugCheckPMCorrectness("Before swap");

		// END DEBUG


		// Try to perform the move

		// Preserve the original PM

		//	out << endl << endl << "Preserving state ... " << endl;

		//	out << "mID1 : " << mID1 << endl;
		//	out << "opIdx1 : " << opIdx1 << endl;
		//	out << "mID2 : " << mID2 << endl;
		//	out << "opIdx2 : " << opIdx2 << endl;


		//	out << "curPred1 : " << ((curPred1 == INVALID) ? -1 : pm.ops[curPred1]->ID) << endl;
		//	out << "curSucc1 : " << ((curSucc1 == INVALID) ? -1 : pm.ops[curSucc1]->ID) << endl;
		//	out << "nodeToSwap1 : " << pm.ops[nodeToSwap1]->ID << endl;
		//	out << "opToSwap1 : " << *pm.ops[nodeToSwap1] << endl;
		//	out << "newPred1 : " << ((newPred1 == INVALID) ? -1 : pm.ops[newPred1]->ID) << endl;
		//	out << "newSucc1 : " << ((newSucc1 == INVALID) ? -1 : pm.ops[newSucc1]->ID) << endl;

		//	out << "curPred2 : " << ((curPred2 == INVALID) ? -1 : pm.ops[curPred2]->ID) << endl;
		//	out << "curSucc2 : " << ((curSucc2 == INVALID) ? -1 : pm.ops[curSucc2]->ID) << endl;
		//	out << "nodeToSwap2 : " << pm.ops[nodeToSwap2]->ID << endl;
		//	out << "opToSwap2 : " << *pm.ops[nodeToSwap2] << endl;
		//	out << "newPred2 : " << ((newPred2 == INVALID) ? -1 : pm.ops[newPred2]->ID) << endl;
		//	out << "newSucc2 : " << ((newSucc2 == INVALID) ? -1 : pm.ops[newSucc2]->ID) << endl;

		QList<ListDigraph::Arc> arcsToRem; // Arcs which will be removed from the graph to perform the move
		QList<QPair<ListDigraph::Node, ListDigraph::Node> > removedArcs; // Removed arcs
		// Collect the corresponding schedule-based arcs
		for (ListDigraph::InArcIt iait(pm.graph, nodeToSwap1); iait != INVALID; ++iait) {
			if (!pm.conjunctive[iait]) { // This is a schedule-based arc
				if (!arcsToRem.contains(iait)) {
					arcsToRem << iait;
					removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(iait), pm.graph.target(iait));
				}
			}
		}
		for (ListDigraph::OutArcIt oait(pm.graph, nodeToSwap1); oait != INVALID; ++oait) {
			if (!pm.conjunctive[oait]) { // This is a schedule-based arc
				if (!arcsToRem.contains(oait)) {
					arcsToRem << oait;
					removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(oait), pm.graph.target(oait));
				}
			}
		}
		for (ListDigraph::InArcIt iait(pm.graph, nodeToSwap2); iait != INVALID; ++iait) {
			if (!pm.conjunctive[iait]) { // This is a schedule-based arc
				if (!arcsToRem.contains(iait)) {
					arcsToRem << iait;
					removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(iait), pm.graph.target(iait));
				}
			}
		}
		for (ListDigraph::OutArcIt oait(pm.graph, nodeToSwap2); oait != INVALID; ++oait) {
			if (!pm.conjunctive[oait]) { // This is a schedule-based arc
				if (!arcsToRem.contains(oait)) {
					arcsToRem << oait;
					removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(oait), pm.graph.target(oait));
				}
			}
		}

		// DEBUG

		// Check whether the same arc is not removed twice
		//	for (int i = 0; i < removedArcs.size() - 1; i++) {
		//		for (int j = i + 1; j < removedArcs.size(); j++) {
		//			if (removedArcs[i].first == removedArcs[j].first && removedArcs[i].second == removedArcs[j].second) {
		//				Debugger::err << "The same arc is removed twice!!!" << ENDL;
		//			}
		//		}
		//	}

		// END DEBUG

		// Erase the collected arcs
		for (int i = 0; i < arcsToRem.size(); i++) {
			ListDigraph::Arc curArc = arcsToRem[i];
			pm.graph.erase(curArc);

			//			out << "Removing arc : " << pm.ops[removedArcs[i].first]->ID << "->" << pm.ops[removedArcs[i].second]->ID << endl;
		}
		arcsToRem.clear();

		// Insert the schedule-based arcs between the newPred and nodeToSwap, and nodeToMove and newSucc
		ListDigraph::Arc newPredArc1 = ((newPred1 == INVALID) ? INVALID : pm.graph.addArc(newPred1, nodeToSwap1));
		ListDigraph::Arc newSuccArc1 = ((newSucc1 == INVALID) ? INVALID : pm.graph.addArc(nodeToSwap1, newSucc1));
		ListDigraph::Arc newPredArc2 = ((newPred2 == INVALID) ? INVALID : pm.graph.addArc(newPred2, nodeToSwap2));
		ListDigraph::Arc newSuccArc2 = ((newSucc2 == INVALID) ? INVALID : pm.graph.addArc(nodeToSwap2, newSucc2));

		// To avoid duplicate arcs
		if (nodeToSwap1 == newPred2 && nodeToSwap2 == newSucc1) {
			if (newPredArc2 != INVALID) {
				pm.graph.erase(newPredArc2);
				newPredArc2 = INVALID;
			}
		}
		if (nodeToSwap2 == newPred1 && nodeToSwap1 == newSucc2) {
			if (newSuccArc2 != INVALID) {
				pm.graph.erase(newSuccArc2);
				newSuccArc2 = INVALID;
			}
		}

		// Get the new processing time for the operations
		opToSwap1.machID = mID2;
		opToSwap1.p(rc(curTGID, mID2).procTime(&opToSwap1));

		// Update the lengths of the new arcs
		if (newPredArc1 != INVALID) {
			pm.conjunctive[newPredArc1] = false;
			pm.p[newPredArc1] = -pm.ops[newPred1]->p();
		}
		if (newSuccArc1 != INVALID) {
			pm.conjunctive[newSuccArc1] = false;
		}
		for (ListDigraph::OutArcIt oait(pm.graph, nodeToSwap1); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[nodeToSwap1]->p();
		}

		opToSwap2.machID = mID1;
		opToSwap2.p(rc(curTGID, mID1).procTime(&opToSwap2));

		// Update the lengths of the new arcs
		if (newPredArc2 != INVALID) {
			pm.conjunctive[newPredArc2] = false;
			pm.p[newPredArc2] = -pm.ops[newPred2]->p();
		}
		if (newSuccArc2 != INVALID) {
			pm.conjunctive[newSuccArc2] = false;
		}
		for (ListDigraph::OutArcIt oait(pm.graph, nodeToSwap2); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[nodeToSwap2]->p();
		}


		// Check whether the move does not create cycles
		if (isInCycle(nodeToSwap1) || isInCycle(nodeToSwap2)) { // Cycle is created

			// Restore the previous state of the PM 
			//			out << endl << endl << "Restoring..." << endl;

			// Restore the processing time of the moved operations
			opToSwap1.machID = mID1;
			opToSwap1.p(rc(curTGID, mID1).procTime(&opToSwap1));

			//	out << "Restored operation 1 : " << opToSwap1 << endl;

			// Remove the newly inserted arcs
			//			if (curPredSuccArc1 != INVALID) {
			//				out << "Removing arc : " << pm.ops[pm.graph.source(curPredSuccArc)]->ID << "->" << pm.ops[pm.graph.target(curPredSuccArc)]->ID << endl;
			//			}
			if (newPredArc1 != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(newPredArc)]->ID << "->" << pm.ops[pm.graph.target(newPredArc)]->ID << endl;
			}
			if (newSuccArc1 != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(newSuccArc)]->ID << "->" << pm.ops[pm.graph.target(newSuccArc)]->ID << endl;
			}
			//			if (curPredSuccArc1 != INVALID) pm.graph.erase(curPredSuccArc1);
			if (newPredArc1 != INVALID) pm.graph.erase(newPredArc1);
			if (newSuccArc1 != INVALID) pm.graph.erase(newSuccArc1);


			opToSwap2.machID = mID2;
			opToSwap2.p(rc(curTGID, mID2).procTime(&opToSwap2));

			//	out << "Restored operation 2 : " << opToSwap2 << endl;

			// Remove the newly inserted arcs
			//			if (curPredSuccArc2 != INVALID) {
			//				out << "Removing arc : " << pm.ops[pm.graph.source(curPredSuccArc)]->ID << "->" << pm.ops[pm.graph.target(curPredSuccArc)]->ID << endl;
			//			}
			if (newPredArc2 != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(newPredArc)]->ID << "->" << pm.ops[pm.graph.target(newPredArc)]->ID << endl;
			}
			if (newSuccArc2 != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(newSuccArc)]->ID << "->" << pm.ops[pm.graph.target(newSuccArc)]->ID << endl;
			}
			//			if (curPredSuccArc2 != INVALID) pm.graph.erase(curPredSuccArc2);
			if (newPredArc2 != INVALID) pm.graph.erase(newPredArc2);
			if (newSuccArc2 != INVALID) pm.graph.erase(newSuccArc2);


			// Reinsert the deleted arcs and restore their lengths. IMPORTANT!!! The processing time of the moved node is set above
			for (int i = 0; i < removedArcs.size(); i++) {

				//				out << "Inserting arc : " << pm.ops[removedArcs[i].first]->ID << "->" << pm.ops[removedArcs[i].second]->ID << endl;

				ListDigraph::Arc reinsArc = pm.graph.addArc(removedArcs[i].first, removedArcs[i].second);
				pm.conjunctive[reinsArc] = false;
				pm.p[reinsArc] = -pm.ops[removedArcs[i].first]->p();
			}

			// Restore the length of the affected arcs (going out of the swapped nodes)
			for (ListDigraph::OutArcIt oait(pm.graph, nodeToSwap1); oait != INVALID; ++oait) {
				pm.p[oait] = -pm.ops[nodeToSwap1]->p();
			}
			for (ListDigraph::OutArcIt oait(pm.graph, nodeToSwap2); oait != INVALID; ++oait) {
				pm.p[oait] = -pm.ops[nodeToSwap2]->p();
			}

			// DEBUG
			if (!dag(pm.graph)) {
				Debugger::err << " Not DAG after restoring!!! " << ENDL;
			}

			//	ls.setPM(&pm);
			//	ls.debugCheckPMCorrectness("After declining swap");

			// END DEBUG

			//out << "Rolled back swap" << endl;
			//getchar();

		} else {

			solutionFromPM(pm);

			swaps++;

			// DEBUG
			//	ls.setPM(&pm);
			//	ls.debugCheckPMCorrectness("After accepting swap");
			// END DEBUG

			//out << "Performed swap" << endl;
			//getchar();

		}


		//Debugger::info << "Finished the shake loop." << ENDL;

	} while (swaps < nOps);

}

void VNSScheduler::randomMoveOperations(const int &nOps, const bool& sameMachOnly) {

	QTextStream out(stdout);

	int moves = 0;

	int from_mID = 0;
	int to_mID = 0;
	int from_opIdx = 0;
	int to_opIdx = 0;

	int r = smR;

	do {
		//Debugger::info << "Starting the shake loop ..." << ENDL;

		int curTGID = 0;
		do {
			curTGID = rc.tools[Rand::rnd<Math::uint32>(0, rc.tools.size() - 1)]->ID;
		} while (curTGID == 0);

		// The machine IDs within the selected machine group
		QList<int> mIDs = rc(curTGID).mid2idx.keys();

		// Select randomly machine IDs
		do {

			from_mID = mIDs[Rand::rnd<Math::uint32>(0, mIDs.size() - 1)];

		} while (curMachID2opNodes[from_mID].size() == 0);
		// Select randomly one operation on each of the machines
		from_opIdx = Rand::rnd<Math::uint32>(0, curMachID2opNodes[from_mID].size() - 1);

		ListDigraph::Node nodeToMove = curMachID2opNodes[from_mID][from_opIdx];
		Operation& opToMove = (Operation&) *(pm.ops[nodeToMove]);
		ListDigraph::Node curPred = (from_opIdx > 0) ? curMachID2opNodes[from_mID][from_opIdx - 1] : INVALID;
		ListDigraph::Node curSucc = (from_opIdx < curMachID2opNodes[from_mID].size() - 1) ? curMachID2opNodes[from_mID][from_opIdx + 1] : INVALID;

		// Select a machine able to process the type of the operation
		if (!sameMachOnly) { // The move can be performed between any machines

			do {

				to_mID = mIDs[Rand::rnd<Math::uint32>(0, mIDs.size() - 1)];

			} while (!rc(curTGID, to_mID).type2speed.contains(opToMove.type));

		} else { // The move is performed on the same machine

			to_mID = from_mID;

		}

		// Select randomly one operation on each of the machines in view of r in [max(from_opidx-r,0), min(from_opidx+r, nmax)]
		int p = from_opIdx;

		if (curMachID2opNodes[to_mID].size() - 1 < p) { // Move to the end

			to_opIdx = curMachID2opNodes[to_mID].size();

		} else {

			to_opIdx = Rand::rnd<Math::uint32>(Math::max(p - r, 0), Math::min(p + r, curMachID2opNodes[to_mID].size() - 1));

		}

		// New potential predecessor/successor nodes
		ListDigraph::Node newPred = (to_opIdx == 0) ? INVALID : curMachID2opNodes[to_mID][to_opIdx - 1];
		ListDigraph::Node newSucc = (to_opIdx == curMachID2opNodes[to_mID].size()) ? INVALID : curMachID2opNodes[to_mID][to_opIdx];

		if (curMachID2opNodes[to_mID].size() == 0) {
			newPred = INVALID;
			newSucc = INVALID;
		}

		if (nodeToMove == newPred || nodeToMove == newSucc) { // No move is possible
			continue;
		}


		// DEBUG
		if (!dag(pm.graph)) {
			Debugger::err << "NOT DAG BEFORE the move!!!" << ENDL;
		}
		// END DEBUG


		// Try to perform the move

		// Preserve the original PM

		//		out << endl << endl << "Preserving state ... " << endl;

		//		out << "curPred : " << ((curPred == INVALID) ? -1 : pm.ops[curPred]->ID) << endl;
		//		out << "curSucc : " << ((curSucc == INVALID) ? -1 : pm.ops[curSucc]->ID) << endl;
		//		out << "nodeToMove : " << pm.ops[nodeToMove]->ID << endl;
		//		out << "newPred : " << ((newPred == INVALID) ? -1 : pm.ops[newPred]->ID) << endl;
		//		out << "newSucc : " << ((newSucc == INVALID) ? -1 : pm.ops[newSucc]->ID) << endl;

		QList<ListDigraph::Arc> arcsToRem; // Arcs which will be removed from the graph to perform the move
		QList<QPair<ListDigraph::Node, ListDigraph::Node> > removedArcs; // Removed arcs
		// Collect the corresponding schedule-based arcs
		for (ListDigraph::InArcIt iait(pm.graph, nodeToMove); iait != INVALID; ++iait) {
			if (!pm.conjunctive[iait]) { // This is a schedule-based arc
				arcsToRem << iait;
				removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(iait), pm.graph.target(iait));
			}
		}
		for (ListDigraph::OutArcIt oait(pm.graph, nodeToMove); oait != INVALID; ++oait) {
			if (!pm.conjunctive[oait]) { // This is a schedule-based arc
				arcsToRem << oait;
				removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(oait), pm.graph.target(oait));
			}
		}

		// Collect the schedule based arc(s) between the potential predecessor and successor
		if (newPred != INVALID) {
			for (ListDigraph::OutArcIt oait(pm.graph, newPred); oait != INVALID; ++oait) {
				if (!pm.conjunctive[oait]) { // This is a schedule-based arc
					arcsToRem << oait;
					removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(oait), pm.graph.target(oait));
				}
			}
		}
		if (newSucc != INVALID) {
			for (ListDigraph::InArcIt iait(pm.graph, newSucc); iait != INVALID; ++iait) {
				if (!pm.conjunctive[iait]) { // This is a schedule-based arc
					if (!arcsToRem.contains(iait)) {
						arcsToRem << iait;
						removedArcs << QPair<ListDigraph::Node, ListDigraph::Node>(pm.graph.source(iait), pm.graph.target(iait));
					}
				}
			}
		}

		// Erase the collected arcs
		for (int i = 0; i < arcsToRem.size(); i++) {
			ListDigraph::Arc curArc = arcsToRem[i];
			pm.graph.erase(curArc);

			//			out << "Removing arc : " << pm.ops[removedArcs[i].first]->ID << "->" << pm.ops[removedArcs[i].second]->ID << endl;
		}
		arcsToRem.clear();

		// Insert an arc between the current predecessor and successor
		ListDigraph::Arc curPredSuccArc = (curPred != INVALID && curSucc != INVALID) ? pm.graph.addArc(curPred, curSucc) : INVALID;
		if (curPredSuccArc != INVALID) {
			pm.conjunctive[curPredSuccArc] = false;
			pm.p[curPredSuccArc] = -pm.ops[curPred]->p();
		}
		if (curPred != INVALID && curSucc != INVALID) {
			//			out << "Inserting arc : " << pm.ops[pm.graph.source(curPredSuccArc)]->ID << "->" << pm.ops[pm.graph.target(curPredSuccArc)]->ID << endl;
		}

		// Insert the schedule-based arcs between the newPred and nodeToMove, and nodeToMove and newSucc
		ListDigraph::Arc newPredArc = ((newPred == INVALID || newPred == nodeToMove) ? INVALID : pm.graph.addArc(newPred, nodeToMove));
		ListDigraph::Arc newSuccArc = ((newSucc == INVALID || newSucc == nodeToMove) ? INVALID : pm.graph.addArc(nodeToMove, newSucc));



		if (newPredArc != INVALID) {
			//			out << "Inserting arc : " << pm.ops[pm.graph.source(newPredArc)]->ID << "->" << pm.ops[pm.graph.target(newPredArc)]->ID << endl;
		}
		if (newSuccArc != INVALID) {
			//			out << "Inserting arc : " << pm.ops[pm.graph.source(newSuccArc)]->ID << "->" << pm.ops[pm.graph.target(newSuccArc)]->ID << endl;
		}



		// Get the new processing time for the operation
		opToMove.machID = to_mID;
		opToMove.p(rc(curTGID, to_mID).procTime(&opToMove));

		// Update the lengths of the new arcs
		if (newPredArc != INVALID) {
			pm.conjunctive[newPredArc] = false;
			pm.p[newPredArc] = -pm.ops[newPred]->p();
		}
		if (newSuccArc != INVALID) {
			pm.conjunctive[newSuccArc] = false;
		}
		for (ListDigraph::OutArcIt oait(pm.graph, nodeToMove); oait != INVALID; ++oait) {
			pm.p[oait] = -pm.ops[nodeToMove]->p();
		}

		// Check whether the move does not create cycles
		if (isInCycle(nodeToMove)) { // Cycle is created

			// Restore the previous state of the PM 
			//			out << endl << endl << "Restoring..." << endl;

			// Restore the processing time of the moved operation
			opToMove.machID = from_mID;
			opToMove.p(rc(curTGID, from_mID).procTime(&opToMove));

			// Remove the newly inserted arcs
			if (curPredSuccArc != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(curPredSuccArc)]->ID << "->" << pm.ops[pm.graph.target(curPredSuccArc)]->ID << endl;
			}
			if (newPredArc != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(newPredArc)]->ID << "->" << pm.ops[pm.graph.target(newPredArc)]->ID << endl;
			}
			if (newSuccArc != INVALID) {
				//				out << "Removing arc : " << pm.ops[pm.graph.source(newSuccArc)]->ID << "->" << pm.ops[pm.graph.target(newSuccArc)]->ID << endl;
			}
			if (curPredSuccArc != INVALID) pm.graph.erase(curPredSuccArc);
			if (newPredArc != INVALID) pm.graph.erase(newPredArc);
			if (newSuccArc != INVALID) pm.graph.erase(newSuccArc);

			// Reinsert the deleted arcs and restore their lengths. IMPORTANT!!! The processing time of the moved node is set above
			for (int i = 0; i < removedArcs.size(); i++) {

				//				out << "Inserting arc : " << pm.ops[removedArcs[i].first]->ID << "->" << pm.ops[removedArcs[i].second]->ID << endl;

				ListDigraph::Arc reinsArc = pm.graph.addArc(removedArcs[i].first, removedArcs[i].second);
				pm.conjunctive[reinsArc] = false;
				pm.p[reinsArc] = -pm.ops[removedArcs[i].first]->p();
			}
			
			// Restore the length of all arcs going out of nodeToMove
			for (ListDigraph::OutArcIt oait(pm.graph, nodeToMove); oait != INVALID; ++oait) {
				pm.p[oait] = -pm.ops[nodeToMove]->p();
			}


			// DEBUG
			if (!dag(pm.graph)) {
				Debugger::err << " Not DAG after restoring!!! " << ENDL;
			}

			//	ls.setPM(&pm);
			//	ls.debugCheckPMCorrectness("After declining move");

			// END DEBUG

		} else {

			solutionFromPM(pm);

			moves++;

			// DEBUG
			//	ls.setPM(&pm);
			//	ls.debugCheckPMCorrectness("After accepting move");
			// END DEBUG

		}

		//Debugger::info << "Finished the shake loop." << ENDL;

		//getchar();

	} while (moves < nOps);
}

void VNSScheduler::solutionFromPM(ProcessModel& pm) {

	curMachID2opNodes.clear();

	// Get the topological ordering of the nodes
	QList<ListDigraph::Node> ts = pm.topolSort();

	// Get the solution

	for (int i = 0; i < ts.size(); i++) {

		ListDigraph::Node& curNode = ts[i];

		Operation& curOp = *(pm.ops[curNode]);

		if (curOp.machID > 0) {

			curMachID2opNodes[curOp.machID] << curNode;

		}

	}

}

void VNSScheduler::findPredecessorsSameMachs() {

	// For each node, find the set of (semi-)direct predecessors which can be processed by the same machine(s)
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		ListDigraph::Node opNode = nit;
		Operation& curOp = *(pm.ops[opNode]);

		if (curOp.type <= 0) {
			continue;
		}

		QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
		ListDigraph::Node curNode;
		ListDigraph::NodeMap<bool> q_contains(pm.graph, false); // Consider revising for optimization

		// Initialize the queue with the opnodes' predecessors
		for (ListDigraph::InArcIt iait(pm.graph, opNode); iait != INVALID; ++iait) {

			q.enqueue(pm.graph.source(iait));

		}

		while (!q.empty()) {

			curNode = q.dequeue();

			// Iterate over the graph predecessors of the node. 
			// If the node belongs to the same machine(s) then add it to the predecessors, otherwise enqueue its predecessors for further processing
			if (!node2MachIDs[opNode].intersect(node2MachIDs[curNode]).empty() && pm.ops[curNode]->ID > 0) {
				if (!predecessors[opNode].contains(curNode)) {
					predecessors[opNode].append(curNode);
				}
			} else {
				for (ListDigraph::InArcIt iait(pm.graph, curNode); iait != INVALID; ++iait) {

					if (!q_contains[pm.graph.source(iait)]) {
						q.enqueue(pm.graph.source(iait));
						q_contains[pm.graph.source(iait)] = true;
					}

				}
			}

		} // while

	} // Iterating over searching the predecessors

}

void VNSScheduler::findSuccessorsSameMachs() {

	// For each node, find the set of (semi-)direct predecessors which can be processed by the same machine(s)
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		ListDigraph::Node opNode = nit;
		Operation& curOp = *(pm.ops[opNode]);

		if (curOp.type <= 0) {
			continue;
		}

		QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
		ListDigraph::Node curNode;
		ListDigraph::NodeMap<bool> q_contains(pm.graph, false); // Consider revising for optimization

		// Initialize the queue with the opNodes' successors
		for (ListDigraph::OutArcIt oait(pm.graph, opNode); oait != INVALID; ++oait) {

			q.enqueue(pm.graph.target(oait));
			q_contains[pm.graph.target(oait)] = true;

		}

		while (!q.empty()) {
			curNode = q.dequeue();

			//Debugger::info << "Queue size: " << q.size() << ENDL;

			// Iterate over the graph successors of the node. 
			// If the node belongs to the same machine(s) then add it to the precedences, otherwise enqueue its successors for further processing
			if (!node2MachIDs[opNode].intersect(node2MachIDs[curNode]).empty() && pm.ops[curNode]->ID > 0) {
				if (!successors[opNode].contains(curNode)) {
					successors[opNode].append(curNode);
				}
			} else {
				for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {

					if (!q_contains[pm.graph.target(oait)]) {
						q.enqueue(pm.graph.target(oait));
						q_contains[pm.graph.target(oait)] = true;
					}

				}
			}

		} // while

	} // Finding successors

}

void VNSScheduler::findNode2MachIDs() {

	// Assign a set of machines, able to process the corresponding operation, to each node
	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		ListDigraph::Node curNode = nit;
		Operation& curOp = *(pm.ops[curNode]);

		if (curOp.type <= 0) {
			continue;
		}

		// Machines able to process this operation
		QList<Machine*> curMachs = rc(curOp.toolID).machines(curOp.type);

		QSet<int> curMachIDs; // Set of machine IDs to process this operation
		for (int i = 0; i < curMachs.size(); i++) {

			Machine& curMach = *(curMachs[i]);

			curMachIDs << curMach.ID;
		}

		node2MachIDs[curNode] = curMachIDs;

	}

}

QList<ListDigraph::Node> VNSScheduler::findPredecessorsTargetMach(const int& mID, const ListDigraph::Node& newPred, const ListDigraph::Node& node) {

	QList<ListDigraph::Node> res;

	QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
	ListDigraph::Node curNode;
	ListDigraph::NodeMap<bool> q_contains(pm.graph, false); // Consider revising for optimization

	// Initialize the queue with the opnodes' predecessors
	for (ListDigraph::InArcIt iait(pm.graph, node); iait != INVALID; ++iait) {
		if (pm.conjunctive[iait]) { // The same effect as removing the machine predecessor
			q.enqueue(pm.graph.source(iait));
			q_contains[pm.graph.source(iait)] = false;
		}
	}
	if (newPred != INVALID) {
		q.enqueue(newPred);
		q_contains[newPred] = true;
		for (ListDigraph::InArcIt iait(pm.graph, newPred); iait != INVALID; ++iait) {
			q.enqueue(pm.graph.source(iait));
			q_contains[pm.graph.source(iait)] = true;
		}
	}

	while (!q.empty()) {

		curNode = q.dequeue();

		// Iterate over the graph predecessors of the node. 
		// If the node belongs to the specified machine then add it to the predecessors, otherwise enqueue its predecessors for further processing
		if (pm.ops[curNode]->machID == mID && pm.ops[curNode]->ID > 0) {
			if (!res.contains(curNode)) {
				res.append(curNode);
			}
		}


		for (ListDigraph::InArcIt iait(pm.graph, curNode); iait != INVALID; ++iait) {

			if (!q_contains[pm.graph.source(iait)]) {
				q.enqueue(pm.graph.source(iait));
				q_contains[pm.graph.source(iait)] = true;
			}

		}

	} // while

	return res;

}

QList<ListDigraph::Node> VNSScheduler::findSuccessorsTargetMach(const int& mID, const ListDigraph::Node& newSucc, const ListDigraph::Node& node) {

	QList<ListDigraph::Node> res;

	QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
	ListDigraph::Node curNode;
	ListDigraph::NodeMap<bool> q_contains(pm.graph, false); // Consider revising for optimization

	// Initialize the queue with the opNodes' successors
	for (ListDigraph::OutArcIt oait(pm.graph, node); oait != INVALID; ++oait) {
		if (pm.conjunctive[oait]) { // The same effect as not considering the successor on the current machine
			q.enqueue(pm.graph.target(oait));
			q_contains[pm.graph.target(oait)] = false;
		}

	}
	if (newSucc != INVALID) {
		q.enqueue(newSucc);
		q_contains[newSucc] = true;
		for (ListDigraph::OutArcIt oait(pm.graph, newSucc); oait != INVALID; ++oait) {
			q.enqueue(pm.graph.target(oait));
			q_contains[pm.graph.target(oait)] = true;
		}
	}

	while (!q.empty()) {
		curNode = q.dequeue();

		//Debugger::info << "Queue size: " << q.size() << ENDL;

		// Iterate over the graph successors of the node. 
		// If the node belongs to the same machine(s) then add it to the precedences, otherwise enqueue its successors for further processing
		if (pm.ops[curNode]->machID == mID && pm.ops[curNode]->ID > 0) {
			if (!res.contains(curNode)) {
				res.append(curNode);
			}
		}

		for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {

			if (!q_contains[pm.graph.target(oait)]) {
				q.enqueue(pm.graph.target(oait));
				q_contains[pm.graph.target(oait)] = true;
			}

		}

	} // while

	return res;

}

bool VNSScheduler::isInCycle(const ListDigraph::Node& node) {
	//return !dag(pm.graph);

	QQueue<ListDigraph::Node> q; // Used for later processing of the nodes
	ListDigraph::Node curNode;
	ListDigraph::NodeMap<bool> nodeVisited(pm.graph, false); // Consider revising for optimization

	nodeVisited[node] = false;

	// Initialize the queue with the opNodes' successors
	for (ListDigraph::OutArcIt oait(pm.graph, node); oait != INVALID; ++oait) {

		q.enqueue(pm.graph.target(oait));
		nodeVisited[pm.graph.target(oait)] = true;

	}

	while (!q.empty()) {

		curNode = q.dequeue();

		// Iterate over the graph successors of the node. 
		if (curNode == node) { // We have a cycle

			return true;

		} else {

			for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {

				ListDigraph::Node succNode = pm.graph.target(oait);

				if (!nodeVisited[succNode]) {
					q.enqueue(succNode);
					nodeVisited[succNode] = true;
				}

			}

		}

	} // while

	// No cycle containing the given node detected
	return false;

}