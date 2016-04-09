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
#include "CombinedScheduler.h"
#include "Objective.h"

/** *********************** Combined scheduler ***************************** **/

CombinedScheduler::CombinedScheduler() {
	schedulers.clear();
}

CombinedScheduler::CombinedScheduler(CombinedScheduler & orig) : Scheduler(orig) {
	/*
	for (int i = 0; i < schedulers.size(); i++) {
			delete schedulers[i];
	}

	schedulers.clear();
	 */

	//Debugger::info << "CombinedScheduler::CombinedScheduler(CombinedScheduler & orig) ...  " << ENDL;

	clear();

	// Copy the scheduling rules
	for (int i = 0; i < orig.schedulers.size(); i++) {
		schedulers.append((Scheduler*) (orig.schedulers[i]->clone()));
	}

	bestPerformingStat = orig.bestPerformingStat;
	lastBestSchedulerIdx = orig.lastBestSchedulerIdx;
	_bestPM = orig._bestPM;

	// IMPORTANT!!! Objective cloning is done in the Scheduler!!!
	//this->obj = orig.obj->clone();
}

CombinedScheduler::~CombinedScheduler() {
	Debugger::info << "CombinedScheduler::~CombinedScheduler : Started" << ENDL;

	/*
	for (int i = 0; i < schedulers.size(); i++) {
			//delete schedulers[i];
	}

	schedulers.clear();
	 */

	clear();

	Debugger::info << "CombinedScheduler::~CombinedScheduler : Finished" << ENDL;
}

void CombinedScheduler::init() {
	QTextStream out(stdout);
	//out << "CombinedScheduler::init :  Running in thread : " << this->thread() << endl;
	//getchar();

	// Clear the statistics
	bestPerformingStat.clear();
	for (int i = 0; i < schedulers.size(); i++) {
		bestPerformingStat.append(0);
	}

	lastBestSchedulerIdx = -1;

}

void CombinedScheduler::clear() { // Does not touch the base class Scheduler -> only clears the data relevant for this scheduler

	for (int i = 0; i < schedulers.size(); i++) {
		delete schedulers[i];
	}

	schedulers.clear();

	bestPerformingStat.clear();

	//if (obj != NULL) {
	//	delete obj;
	//  obj = NULL;
	//}
}

Clonable * CombinedScheduler::clone() {
	return new CombinedScheduler(*this);
}

void CombinedScheduler::scheduleActions() {
	QTextStream out(stdout);

	// Preserve the state of the resources
	Resources rcInit = rc;

	// Preserve the PM
	ProcessModel pmInit = pm;

	// Initialize the best PM
	_bestPM = pm;

	// Best found schedule
	Schedule bestSched;

	bestSched.init();

	//out << "CombinedScheduler::scheduleActions : " << pm << endl;
	//out << "CombinedScheduler::scheduleActions : best PM : " << _bestPM << endl;

	//out << "Running scheduler with ID = " << ID << endl;

	QSet<int> allowedSchedIDs;

	if (!options.contains("CS_ALLOWED_SCHEDULERS")) {
		Debugger::err << "CombinedScheduler::scheduleActions : No allowed schedulers specified!" << ENDL;
	}

	QStringList schedIDsList = options["CS_ALLOWED_SCHEDULERS"].split("&");

	for (int i = 0; i < schedIDsList.size(); i++) {
		allowedSchedIDs.insert(schedIDsList[i].toInt());
		//cout << schedIDsList[i].toStdString();
	}
	//cout << endl;

	for (int i = 0; i < schedulers.size(); i++) {
		if (!allowedSchedIDs.contains(schedulers[i]->ID)) continue; // Skip the scheduler

		//out << "Setting pm "<< endl;
		//out << "Subscheduler ID = " << schedulers[i]->ID << endl;
		schedulers[i]->pm = pmInit;
		schedulers[i]->rc = rcInit;
		schedulers[i]->sched = sched;
		schedulers[i]->options = options;
		//out << "pm set. "<< endl;

		schedulers[i]->schedule();

		if (bestSched.objective >= sched->objective) {
			bestSched = *sched;

			lastBestSchedulerIdx = i;

			// Preserve the best PM
			_bestPM = schedulers[i]->pm;
		}

	}

	// Update the statistics
	bestPerformingStat[lastBestSchedulerIdx]++;

	*sched = bestSched;

	out << "CombinedScheduler::scheduleActions : Best TWT =  " << sched->objective << endl;
	out << "CombinedScheduler::scheduleActions : Best performing scheduler " << schedulers[lastBestSchedulerIdx]->ID << endl;
	out << "CombinedScheduler::scheduleActions :  Flow factor of the best schedule : " << schedulers[lastBestSchedulerIdx]->flowFactor() << endl;

	//TWT twt;
	//out << "CombinedScheduler::scheduleActions : Recalculated best TWT =  " << twt(_bestPM) << endl;

	out << "Performance statistics : " << endl;
	for (int i = 0; i < bestPerformingStat.size(); i++) {
		out << bestPerformingStat[i] << " ";
	}
	out << endl;

}

CombinedScheduler & CombinedScheduler::operator<<(Scheduler * sch) {
	// IMPORTANT!!! Clone the input scheduler and do not pass it as a pointer!!!
	schedulers.append((Scheduler*) (sch->clone()));

	init();

	return *this;
}

Scheduler * CombinedScheduler::lastBestScheduler() {
	if (lastBestSchedulerIdx == -1) {
		Debugger::err << "CombinedScheduler::lastBestScheduler : Failer to find the last best performing scheduler!" << ENDL;
	}

	return schedulers[lastBestSchedulerIdx];
}

ProcessModel & CombinedScheduler::bestPM() {
	return _bestPM;
}

void CombinedScheduler::setObjective(ScalarObjective& newObj) {

	// Initialize the base class
	Scheduler::setObjective(newObj);

	// Set the objectives for all subschedulers
	for (int i = 0; i < schedulers.size(); i++) {

		Scheduler& curScheduler = (Scheduler&) * schedulers[i];

		curScheduler.setObjective(newObj);

	}

}


/** ************************************************************************ **/

/** *********************** Combined scheduler with LS ********************* **/

CombinedSchedulerLS::CombinedSchedulerLS() {

	cs.setParent(this);
	ls.setParent(this);

}

CombinedSchedulerLS::CombinedSchedulerLS(CombinedSchedulerLS & orig) : Scheduler(orig), cs(orig.cs), ls(orig.ls) {

	//Debugger::info << "CombinedSchedulerLS::CombinedSchedulerLS(CombinedSchedulerLS & orig)..." << ENDL;

	cs.setParent(this);
	ls.setParent(this);

	obj = orig.obj->clone();

}

CombinedSchedulerLS::~CombinedSchedulerLS() {

	Debugger::info << "CombinedSchedulerLS::~CombinedSchedulerLS" << ENDL;

}

void CombinedSchedulerLS::init() {
	cs.init();
	ls.init();
}

Clonable * CombinedSchedulerLS::clone() {
	//Debugger::info << "CombinedSchedulerLS::clone : Cloning..." << ENDL;
	return new CombinedSchedulerLS(*this);
}

void CombinedSchedulerLS::scheduleActions() {
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
		Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_CHK_COR not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_ITER")) {
		ls.maxIter(options["LS_MAX_ITER"].toInt());
	} else {
		Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_TIME_MS")) {
		ls.maxTimeMs(options["LS_MAX_TIME_MS"].toInt());
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

	int iterCtr = 0;
	for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
		iterCtr += cs.bestPerformingStat[i];
	}

	//RandGenMT randGen(Math::rndSeed());

	//QTextStream out(stdout);
	//out << "Current seed : " << Math::rndSeed() << endl;
	//getchar();

	//if (iterCtr > 400) {
	// Improve the PM by the local search
	//ls.maxIter(0);
	//ls.checkCorectness(false);
	if (ls.maxIter() > 0) {
		pm.save();

		ls.setObjective(obj);
		ls.setPM(&pm);
		ls.setResources(&rc);
		//ls.setRandGen(&randGen);

		ls.run();

		pm.restore();
	}
	//}

	// Prepare the schedule
	sched->fromPM(pm, *obj);

	//getchar();

	/*
	// TESTING : Try to leave the selections of only one machine group and check the TWT

	QTextStream out(stdout);
	pm.updateHeads();
	pm.updateStartTimes();
	ls.debugCheckPMCorrectness("");
	out << "TWT of the full schedule is " << TWT()(pm) << endl;

	int curTG = 10;

	QList<ListDigraph::Arc> arcsToRem;

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curNode]);

		if (curOp.toolID != curTG) { // Mark the disjunctive arcs for removal and restore the expected processing time

			curOp.p(rc(curOp.toolID).expectedProcTime(&curOp));

			for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {

				ListDigraph::Arc curArc = oait;

				pm.p[curArc] = -curOp.p();

				if (!pm.conjunctive[curArc]) arcsToRem.append(curArc);

			}

		}

	}

	// Remove the arcs
	for (int i = 0; i < arcsToRem.size(); i++) {
		pm.graph.erase(arcsToRem[i]);
	}

	pm.updateHeads();
	pm.updateStartTimes();

	out << "TWT of the partial schedule with machine group " << curTG << " is " << TWT()(pm) << endl;
	getchar();
	
	// TESTING
	 */

}

/** ************************************************************************ **/

/** *********************** Combined scheduler with modified LS ************ **/

CombinedSchedulerModLS::CombinedSchedulerModLS() {
 }

CombinedSchedulerModLS::CombinedSchedulerModLS(CombinedSchedulerModLS & orig) : Scheduler(orig), cs(orig.cs), ls(orig.ls) {
 }

CombinedSchedulerModLS::~CombinedSchedulerModLS() {
 }

void CombinedSchedulerModLS::init() {
	cs.init();
	ls.init();
}

Clonable * CombinedSchedulerModLS::clone() {
	return new CombinedSchedulerModLS(*this);
}

void CombinedSchedulerModLS::scheduleActions() {
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
		Debugger::err << "CombinedSchedulerModLS::scheduleActions : LS_CHK_COR not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_ITER")) {
		ls.maxIter(options["LS_MAX_ITER"].toInt());
	} else {
		Debugger::err << "CombinedSchedulerModLS::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_TIME_MS")) {
		ls.maxTimeMs(options["LS_MAX_TIME_MS"].toInt());
	}

	// Run the Combined scheduler
	cs.pm = pm;
	cs.rc = rc;
	cs.sched = sched;
	cs.options = options;

	cs.schedule();

	// Get the best PM found by the CombinedScheduler
	pm = cs.bestPM();

	int iterCtr = 0;
	for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
		iterCtr += cs.bestPerformingStat[i];
	}

	//if (iterCtr > 400) {
	// Improve the PM by the local search
	//ls.maxIter(0);
	//ls.checkCorectness(false);
	if (ls.maxIter() > 0) {
		pm.save();

		ls.setPM(&pm);
		ls.setResources(&rc);
		ls.run();

		pm.restore();
	}
	//}

	// Prepare the schedule
	sched->fromPM(pm, *obj);


	/*
	// TESTING : Try to leave the selections of only one machine group and check the TWT

	QTextStream out(stdout);
	pm.updateHeads();
	pm.updateStartTimes();
	ls.debugCheckPMCorrectness("");
	out << "TWT of the full schedule is " << TWT()(pm) << endl;

	int curTG = 10;

	QList<ListDigraph::Arc> arcsToRem;

	for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {

		ListDigraph::Node curNode = nit;
		Operation& curOp = (Operation&) *(pm.ops[curNode]);

		if (curOp.toolID != curTG) { // Mark the disjunctive arcs for removal and restore the expected processing time

			curOp.p(rc(curOp.toolID).expectedProcTime(&curOp));

			for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {

				ListDigraph::Arc curArc = oait;

				pm.p[curArc] = -curOp.p();

				if (!pm.conjunctive[curArc]) arcsToRem.append(curArc);

			}

		}

	}

	// Remove the arcs
	for (int i = 0; i < arcsToRem.size(); i++) {
		pm.graph.erase(arcsToRem[i]);
	}

	pm.updateHeads();
	pm.updateStartTimes();

	out << "TWT of the partial schedule with machine group " << curTG << " is " << TWT()(pm) << endl;
	getchar();
	
	// TESTING
	 */

}

/** ************************************************************************ **/

/** *********************** Combined scheduler with LSCP ********************* **/

CombinedSchedulerLSCP::CombinedSchedulerLSCP() {
 }

CombinedSchedulerLSCP::CombinedSchedulerLSCP(CombinedSchedulerLSCP & orig) : Scheduler(orig), cs(orig.cs), ls(orig.ls) {
 }

CombinedSchedulerLSCP::~CombinedSchedulerLSCP() {
 }

void CombinedSchedulerLSCP::init() {
	cs.init();
	ls.init();
}

Clonable * CombinedSchedulerLSCP::clone() {
	return new CombinedSchedulerLSCP(*this);
}

void CombinedSchedulerLSCP::scheduleActions() {
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
		Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_CHK_COR not specified!" << ENDL;
	}

	if (options.contains("LS_MAX_ITER")) {
		ls.maxIter(options["LS_MAX_ITER"].toInt());
	} else {
		Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
	}

	// Run the Combined scheduler
	cs.pm = pm;
	cs.rc = rc;
	cs.sched = sched;
	cs.options = options;

	cs.schedule();

	// Get the best PM found by the CombinedScheduler
	pm = cs.bestPM();

	int iterCtr = 0;
	for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
		iterCtr += cs.bestPerformingStat[i];
	}

	//if (iterCtr > 400) {
	// Improve the PM by the local search
	//ls.maxIter(0);
	//ls.checkCorectness(false);
	if (ls.maxIter() > 0) {
		pm.save();

		ls.setPM(&pm);
		ls.setResources(&rc);
		ls.run();

		pm.restore();
	}
	//}

	// Prepare the schedule
	sched->fromPM(pm, *obj);

}
