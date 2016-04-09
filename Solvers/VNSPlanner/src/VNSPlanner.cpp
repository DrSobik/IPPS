/* 
 * File:   VNSPlanner.cpp
 * Author: DrSobik
 * 
 * Created on August 17, 2011, 9:52 AM
 */

#include <QTextStream>
#include <QCoreApplication>
//#include <QApplication>

#include "VNSPlanner.h"
//#include "SBHScheduler.h"
//#include "TrivialScheduler.h"

/** ##########################  VNSPlannerStrategy  ######################### */

VNSPlannerStrategy::VNSPlannerStrategy() : PlannerStrategy() {

}

VNSPlannerStrategy::~VNSPlannerStrategy() {

}

bool VNSPlannerStrategy::parseStrategy(const QString&) {
	QTextStream out(stdout);

	plannerProgressIntervals.clear();
	plannerProgressIntervalsBrackets.clear();
	plannerOptions.clear();

	schedulerProgressIntervals.clear();
	schedulerProgressIntervalsBrackets.clear();
	schedulerNames.clear();
	schedulerOptions.clear();

	//	out << "VNSPlannerStrategy::parseStrategy : Parsing strategy :  " << getStrategy() << endl << endl;

	// Find the "PLANNER" part
	QRegExp rePlannerPart("PLANNER\\{(.*)\\}");
	rePlannerPart.setMinimal(true); // Make it non-greedy
	QString strStrategyPlannerPart;

	rePlannerPart.indexIn(getStrategy().remove(" "));
	if (rePlannerPart.captureCount() > 0) { // There are planner options
		strStrategyPlannerPart = rePlannerPart.cap(1);

		//		out << "Captured planner options : " << strStrategyPlannerPart << endl;

		QStringList splitStrategyPlannerPart = strStrategyPlannerPart.split(",", QString::SkipEmptyParts);

		// Parse the percentages and the corresponding options
		for (int i = 0; i < splitStrategyPlannerPart.size(); i++) {

			// Parse the percentages
			QString curPercPart = splitStrategyPlannerPart[i].split(":", QString::SkipEmptyParts)[0];
			QString curOptPart = splitStrategyPlannerPart[i].split(":", QString::SkipEmptyParts)[1];

			QRegExp rePerc("([\\[|\\(])([\\.|\\d]*)\\%\\-([\\.|\\d]*)\\%([\\]|\\)])");
			rePerc.setMinimal(true);
			rePerc.indexIn(curPercPart);

			double curPercStart = rePerc.cap(2).toDouble() / 100.0;
			double curPercEnd = rePerc.cap(3).toDouble() / 100.0;

			QString curPercBracketStart = rePerc.cap(1);
			QString curPercBracketEnd = rePerc.cap(4);

			//			out << "Planner percentage : " << curPercBracketStart << curPercStart << "," << curPercEnd << curPercBracketEnd << endl;

			// Parse all planner adjustments
			QRegExp reOpts("\\((.*)\\)");
			reOpts.setMinimal(true);
			reOpts.indexIn(curOptPart);

			//			out << "Planner options: " << reOpts.cap(1) << endl;
			QStringList splitOpts = reOpts.cap(1).split(";", QString::SkipEmptyParts);
			SchedulerOptions curPlannerOpt;
			for (int j = 0; j < splitOpts.size(); j++) {

				//				out << splitOpts[j].split("=", QString::SkipEmptyParts)[0] << "=" << splitOpts[j].split("=", QString::SkipEmptyParts)[1] << endl;

				curPlannerOpt[splitOpts[j].split("=", QString::SkipEmptyParts)[0]] = splitOpts[j].split("=", QString::SkipEmptyParts)[1];

			}

			plannerProgressIntervals.append(QPair<double, double>(curPercStart, curPercEnd));
			plannerProgressIntervalsBrackets.append(QPair<QString, QString>(curPercBracketStart, curPercBracketEnd));
			plannerOptions.append(curPlannerOpt);

		}

	}
	//	out << endl;

	// Find the "SCHEDULER" part
	QRegExp reSchedulerPart("SCHEDULER\\{(.*)\\}");
	reSchedulerPart.setMinimal(true);
	QString strStrategySchedulerPart; // = getStrategy().section(re);

	reSchedulerPart.indexIn(getStrategy().remove(" "));
	if (reSchedulerPart.captureCount() == 0) {
		Debugger::err << "VNSPlannerStrategy::parseStrategy : Failed to parse strategy, no captured text!!!" << ENDL;
	}
	strStrategySchedulerPart = reSchedulerPart.cap(1); // Captured text in the first parenthesis of the regexps
	//	out << "Captured scheduler options : " << strStrategySchedulerPart << endl;

	// Split the captured text
	QStringList splittedStrategySchedulerParts = strStrategySchedulerPart.split(",", QString::SkipEmptyParts);

	for (int i = 0; i < splittedStrategySchedulerParts.size(); i++) {

		QStringList listStrPercSched = splittedStrategySchedulerParts[i].split(":", QString::SkipEmptyParts);

		//		out << "Percentage: " << listStrPercSched[0] << endl;
		//		out << "Schedulers: " << listStrPercSched[1] << endl;

		// Analyze the percentage for the current scheduler
		QRegExp percEx("([\\[|\\(])([\\.|\\d]*)\\%\\-([\\.|\\d]*)\\%([\\]|\\)])");
		percEx.setMinimal(true);
		percEx.indexIn(listStrPercSched[0]);

		QString strOpenBracket = percEx.cap(1);
		QString strCloseBracket = percEx.cap(4);
		QString strPercStart = percEx.cap(2);
		QString strPercEnd = percEx.cap(3);

		//		out << "Parsed percentage: " << strOpenBracket << strPercStart << "-" << strPercEnd << strCloseBracket << endl << endl;

		double curStartPerc = strPercStart.toDouble() / 100.0;
		double curEndPerc = strPercEnd.toDouble() / 100.0;

		//		out << "Parsed interval : [" << curStartPerc << "," << curEndPerc << "]" << endl;

		// Parse the scheduler name
		QRegExp schedNameEx("(.*)\\((.*)\\)|(.*)");
		schedNameEx.setMinimal(false);
		schedNameEx.indexIn(listStrPercSched[1]);
		QString curSchedulerName = schedNameEx.cap(1);

		//		out << "Scheduler name : " << curSchedulerName << endl << endl;

		SchedulerOptions curSchedulerOptions;

		if (schedNameEx.captureCount() > 1) { // There are options of the algorithm
			QString curSchedOpt = schedNameEx.cap(2);

			//			out << "Parsed options: " << endl;

			QStringList listSplittedSchedOpt = curSchedOpt.split(";", QString::SkipEmptyParts);

			for (int j = 0; j < listSplittedSchedOpt.size(); j++) {

				curSchedulerOptions[listSplittedSchedOpt[j].split("=", QString::SkipEmptyParts)[0]] = listSplittedSchedOpt[j].split("=", QString::SkipEmptyParts)[1];
				//				out << listSplittedSchedOpt[j].split("=", QString::SkipEmptyParts)[0] << " = " << curSchedulerOptions[listSplittedSchedOpt[j].split("=", QString::SkipEmptyParts)[0]] << endl;

			}

		}
		//		out << endl;

		// Add the parsed data
		schedulerProgressIntervals.append(QPair<double, double>(curStartPerc, curEndPerc));
		schedulerProgressIntervalsBrackets.append(QPair<QString, QString>(strOpenBracket, strCloseBracket));
		schedulerNames.append(curSchedulerName);
		schedulerOptions.append(curSchedulerOptions);

	}

	//getchar();
	return true;
}

VNSPlannerStrategy* VNSPlannerStrategy::clone() {
	return new VNSPlannerStrategy(*this);
}

/** ######################################################################### */

/** ##########################  VNSPlanner  ################################# */

VNSPlanner::VNSPlanner() {
	solInitType = VNSPlanner::SOL_INIT_RND;
	ns = VNSPlanner::NS_N3;

	// Initialize the kStep
	kStep = 1;

	// Initialize the itmKStep
	itmKStep = 1;

	// Initialize the kContribStep
	kContribStep = 1;

	// Set the maximal amount of the new plans to generate at once
	maxNewPlans = 1;

	scheduler = NULL;

	// Connect signals and slots
	connect(this, SIGNAL(sigAssessPlan(const int&)), this, SLOT(slotAssessPlan(const int&)));
	connect(this, SIGNAL(sigPlanAssessFinished(const int&)), this, SLOT(slotPlanAssessFinished(const int&)));
	connect(this, SIGNAL(sigAllPlansAssessed()), this, SLOT(slotAllPlansAssessed()));
	connect(this, SIGNAL(sigInitSched()), this, SLOT(slotInitSched()));

	connect(this, SIGNAL(sigCompleteStepFinished()), this, SLOT(slotIterationFinished()));

}

VNSPlanner::~VNSPlanner() {

}

void VNSPlanner::init() {
	QTextStream out(stdout);

	// Accepting worse solutions
	acceptedWorse = false;
	worseAcceptRate = 0.1;
	maxIterDeclToAccWorse = 20;

	// #############  DEBUG PURPOSES ONLY => should later be deleted  ##########
	//    Debugger::wDebug("Fix the BOMs for scheduling");

	//    bestBOPIDs[0] = 65540;
	//    bestBOPIDs[1] = 131079;

	//    getchar();
	// ##############################################	

	// Initialize kmax
	kmax = pmm->ordman->availProducts().size(); //bestBOPIDs.size();

	// Initialize the current degree of the global neighborhoods
	k = 1;

	// Initialize VNS parameters for the N2-neighborhood
	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		prodID2itmK[pmm->prodman->products[i]->ID] = 1;
		//prodID2itmKmax[pmm->prodman->products[i]->ID] = pmm->prodman->products[i]->bops[pmm->prodman->products[i]->bopid2idx[pmm->prodman->products[i]->bopID]]->nItemTypes();
		prodID2itmKmax[pmm->prodman->products[i]->ID] = pmm->prodman->products[i]->bops[pmm->prodman->products[i]->bopid2idx[pmm->prodman->products[i]->bopID]]->nItems();
	}

	// Initialize kContribMax
	kContribMax = pmm->ordman->availProducts().size(); //bestBOPIDs.size();

	// Initialize kContrib
	kContrib = 1;

	// Initialize the kContrib probability scaling power
	kContribPow = 2.0;

	// Initialize the expected processing of the products and their deviations
	prodID2expCost.clear();
	prodID2minExpCostDev.clear();
	prodID2maxExpCostDev.clear();
	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		prodID2minExpCostDev[pmm->prodman->products[i]->ID] = 1.0;
		prodID2maxExpCostDev[pmm->prodman->products[i]->ID] = Math::MAX_DOUBLE;

		// Calculate the expected processing costs for each product based on its BOMs
		prodID2expCost[pmm->prodman->products[i]->ID] = pmm->prodman->products[i]->expCost(*rc);
		//Debugger::info << pmm->prodman->products[i]->expCost(*rc) << ENDL;
		//getchar();
	}

	// Select the initial set of BOP IDs
	curPlan = initialSolution();

	// Set the current plan and the schedule as the best
	bestPlan = curPlan;

	for (int i = 0; i < schedAlgs.size(); i++) {
		delete schedAlgs[i];
	}
	schedAlgs.clear();

	// Create the scheduling threads
	for (int i = 0; i < schedThreads.size(); i++) {
		schedThreads[i]->exit(0);
		schedThreads[i]->wait();
		delete schedThreads[i];
	}
	schedThreads.clear();
	for (int i = 0; i < maxNewPlans; i++) {
		schedThreads.append(new SchedThread(this));
	}

	if (scheduler->obj == NULL) {
		Debugger::err << "VNSPlanner::init : NULL objective in the scheduler!!!" << ENDL;
	}

	for (int i = 0; i < maxNewPlans; i++) {
		schedAlgs.append((Scheduler*) scheduler->clone());

		schedAlgs[i]->ID = i;

		// Adjust the scheduling algorithm (LS)
		//((LSScheduler*) schedAlgs.last())->ls.maxIter(0);
		//((LSScheduler*) schedAlgs.last())->ls.checkCorectness(true);

		// Move the scheduling algorithm to its own thread
		schedAlgs[i]->moveToThread(schedThreads[i]);
		//schedAlgs.last()->setParent(schedThreads[i]);		

		// Connect the signals and the slots
		connect(schedThreads[i], SIGNAL(sigRunScheduler()), schedAlgs[i], SLOT(slotSchedule()));
		connect(schedAlgs[i], SIGNAL(sigFinished(const int&)), this, SIGNAL(sigPlanAssessFinished(const int&)));

		// Start the scheduler thread
		schedThreads[i]->start();

		out << "Created schedAlg " << schedAlgs.last()->ID << " addr. " << schedAlgs[i] << endl;
		//getchar();
	}

	// Emit a signal to start initial scheduling
	emit sigInitSched();
}

void VNSPlanner::clear() {
	QTextStream out(stdout);

	prodID2expCost.clear();
	prodID2maxExpCostDev.clear();
	prodID2minExpCostDev.clear();

	prodID2itmK.clear();
	prodID2itmKmax.clear();

	bestPlan.clear();
	bestSched.clear();
	curPlan.clear();
	curSched.clear();
	prevPlan.clear();
	prevSched.clear();

	maxNewPlans = 0;
	newPlans.clear();
	newScheds.clear();

	out << "VNSPlanner::clear : Deleting the scheduler ... " << endl;

	if (scheduler != NULL) {
		delete scheduler;

		scheduler = NULL;
	}

	out << "VNSPlanner::clear : Done deleting the scheduler. " << endl;

	out << "VNSPlanner::clear : Deleting the schedAlgs ... " << endl;
	for (int i = 0; i < schedAlgs.size(); i++) {
		// IMPORTANT!!! They live in another threads
		//out << "Deleting schedAlg with idx " << i << endl;

		disconnect(schedThreads[i], SIGNAL(sigRunScheduler()), schedAlgs[i], SLOT(slotSchedule()));
		disconnect(schedAlgs[i], SIGNAL(sigFinished(const int&)), this, SIGNAL(sigPlanAssessFinished(const int&)));

		//schedAlgs[i]->moveToThread(QApplication::instance()->thread());

		//out << "Deleting schedAlg " << schedAlgs[i]->ID << " addr. " << schedAlgs[i] << endl;

		delete schedAlgs[i];

		//getchar();
	}
	schedAlgs.clear();
	out << "VNSPlanner::clear : Done deleting the schedAlgs. " << endl;

	//getchar();
	out << "VNSPlanner::clear : stopping the schedThreads ... " << endl;
	for (int i = 0; i < schedThreads.size(); i++) {
		//out << "Exiting schedThread " << i << endl;
		schedThreads[i]->exit(0);
		schedThreads[i]->wait(); // Wait until the schedThread finishes exits its event loop
	}
	out << "VNSPlanner::clear : Done stopping the schedThreads. " << endl;

	out << "VNSPlanner::clear : Deleting the schedThreads ... " << endl;
	for (int i = 0; i < schedThreads.size(); i++) {
		//out << "Deleting schedThread " << i << endl;
		delete schedThreads[i];
	}
	schedThreads.clear();
	out << "VNSPlanner::clear : Done deleting the schedThreads. " << endl;

	//getchar();

	assessedNewPlansIdx.clear();

	acceptedWorse = false;
	worseAcceptRate = 0.0;
	maxIterDeclToAccWorse = -1;

	kmax = -1;
	k = -1;
	kStep = -1;

	kContrib = -1;
	kContribStep = -1;
	kContribMax = -1;
	kContribPow = 0.0;

	itmKStep = -1;

	schedOptions.clear();

	solInitType = VNSPlanner::SOL_INIT_RND;
	ns = VNSPlanner::NS_N3;

	// Initialize the kStep
	kStep = 1;

	// Initialize the itmKStep
	itmKStep = 1;

	// Initialize the kContribStep
	kContribStep = 1;

	// Set the maximal amount of the new plans to generate at once
	maxNewPlans = 1;

}

void VNSPlanner::stepActions() {

	worseAcceptRate = 0.15 * Math::exp(1.0 - Math::pow(double(_maxiter) / double(_maxiter - iter() + 0.00000001), 3.0));

	//Debugger::info << "######################################################### " << ENDL;
	//Debugger::info << "######################################################### " << ENDL;
	//Debugger::info << "######################################################### " << ENDL;
	//Debugger::info << "######################################################### " << ENDL;
	//Debugger::info << "######################################################### " << ENDL;
	//Debugger::info << "VNSPlanner::stepActions: Starting iteration " << iter() << ENDL;
	//Debugger::info << "######################################################### " << ENDL;
	//getchar();

	// Preserve the current best solution
	prevPlan = curPlan;
	prevSched = curSched;

	// Perform shaking
	shake();

	// Perform local search
	localSearch();

	//Debugger::info << "VNSPlanner::stepActions : Finished step actions. Emitting sigStepFinished..." << ENDL;
	//getchar();

	emit sigStepFinished();

}

void VNSPlanner::assessActions() {
	QTextStream out(stdout);

	//out << "VNSPlanner::assessActions : Assessing the generated plans ... " << endl;
	//getchar();

	// Assess each of the generated new plans

	// Clear the set of the already assessed plans
	assessedNewPlansIdx.clear();

	for (int i = 0; i < newPlans.size(); i++) {
		emit sigAssessPlan(i);
	}

	return;


	// Restore the state of the products based on the current plan
	pmm->prodman->bopsFromPlan(curPlan);

	// Update the global process model according to the current state
	pmm->updatePM();

	/** Initialize the resources for the new clean scheduling. */
	rc->init();

	// Assign operations of the current global PM to the resources.
	//Debugger::info << "Planner::assess : Assessing graph with number of nodes:" << countNodes(pmm->pm.graph) << ENDL;
	for (ListDigraph::NodeIt nit(pmm->pm.graph); nit != INVALID; ++nit) {
		if (!rc->assign(pmm->pm.ops[nit])) {
			out << "VNSPlanner::assessActions : Assigning operation: " << *pmm->pm.ops[nit] << endl;
			Debugger::err << "VNSPlanner::assessActions : Failed to assign operation to resource!" << ENDL;
		}
	}

	//out << "VNSPlanner::assessActions : Assigned operations to resources." << endl;
	//out << pmm->pm << endl;

	int strategy = 0;

	if (strategy == 0) { // Apply no zero strategy
		//((SBHScheduler*) scheduler)->ls.maxIter(2000);
		//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
	}

	if (strategy == 1) { // Apply the first VNS search strategy

		if (_curtime.elapsed() < Math::round(0.6 * double(_maxtimems))/*iter() < 70*//*25*/) {
			//((SBHScheduler*) scheduler)->ls->maxIter(5);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(false);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		}

		if (iter() >= 25 && iter() < 50) {
			//((SBHScheduler*) scheduler)->ls->maxIter(20);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(false);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		}

		if (iter() >= 50 && iter() < 75) {
			//((SBHScheduler*) scheduler)->ls->maxIter(200);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(false);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		}

		if (iter() >= 75 && iter() < 90) {
			//((SBHScheduler*) scheduler)->ls->maxIter(200);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		}

		if (_curtime.elapsed() >= Math::round(0.6 * double(_maxtimems))/*iter() >= 70 && iter() < 100*/) {
			//((SBHScheduler*) scheduler)->ls->maxIter(2000);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		}

		if (iter() >= 100) {
			//((SBHScheduler*) scheduler)->ls->maxIter(2000);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		}
	}

	if (strategy == 2) { // Apply the second VNS search strategy

		if (iter() % 10 == 0) { // Solve the job shop with good quality
			if (iter() > 5) {
				//((SBHScheduler*) scheduler)->ls->maxIter(2000);
				//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
			}
		} else { // Do not solve the job shop with good quality
			//((SBHScheduler*) scheduler)->ls->maxIter(5);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(false);
			//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		}
	}

	strategy = -6;

	// Restore the currently best found plan and solve the corresponding model with the higher quality
	if (iter() == _maxiter / 2) {

	}

	if (strategy < 0) {
		//prioscheduler->pm = pmm->pm;
		//prioscheduler->rc = *rc;
		//prioscheduler->sched = schedule;
		//prioscheduler->schedule();
	}

	// This is the best schedule found by the simple scheduler
	pmm->pm.restore();

	// Run the local search over the schedule obtained by some other method
	if (iter() > _maxiter / 2) {

		// Try run the local search
//		LocalSearchPM lspm;

//		lspm.setPM(&(pmm->pm));
//		lspm.setResources(rc);

//		lspm.maxIter(2000);
		//out << "Running the local search reoptimization ..." << endl;
//		lspm.run();
	}

	// This is the best schedule found by the LS
	pmm->pm.restore();

	if (iter() > _maxiter - 10) {
		Debugger::info << "Running SBH + LS" << ENDL;
		//schedule->fromPM(pmm->pm);
		//Debugger::info << "Current obj : " << schedule->objective << ENDL;
		//getchar();

		pmm->pm.clearSchedRelData();
		//((SBHScheduler*) scheduler)->ls->maxIter(2000);
		//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//scheduler->schedule(pmm->pm, *rc, *schedule);

		Debugger::info << "Done running SBH + LS" << ENDL;
		pmm->pm.restore();
		//schedule->fromPM(pmm->pm);
		//Debugger::info << "Current obj : " << schedule->objective << ENDL;
		//getchar();
	}

	// Restore the best scheduler
	pmm->pm.restore();

	// Prepare the schedule based on the PM
	//schedule->fromPM(pmm->pm);

	// Now the process model can be cleaned
	pmm->pm.clearSchedRelData();

	out << "Average TG utilization : " << endl;
	for (int i = 0; i < rc->tools.size(); i++) {
		out << rc->tools[i]->ID << ":" << rc->tools[i]->avgUtil() << ", ";
	}
	out << endl;


	// Output of the current BOP IDs
	out << "VNSPlanner::assessActions : Iteration " << this->iter() << endl;
	out << "VNSPlanner::assessActions : Current BOP IDs: (";
	for (int i = 0; i < pmm->prodman->products.size() - 1; i++) {

		out << pmm->prodman->products[i]->bopID << ",";
	}
	out << pmm->prodman->products.last()->bopID << ")" << endl;

	//Debugger::info << "VNSPlanner::assessActions : Best found objective value : " << bestObj << ENDL;
	//Debugger::info << "VNSPlanner::assessActions : Current objective value : " << schedule->objective << ENDL;

	/*
	for (QHash<int, double>::iterator iter = schedule->ordID2objContrib.begin(); iter != schedule->ordID2objContrib.end(); iter++) {
			out << "Order: " << iter.key() << ", Contrib: " << iter.value() << endl;
	}
	getchar();
	 */

	//getchar();
}

bool VNSPlanner::acceptCondition() {
	bool accept = false;
	int maxiterdecl = maxIterDeclToAccWorse;
	double maxworserate = worseAcceptRate;

	acceptedWorse = false;

	if (curSched.objective < bestSched.objective) {
		accept = true;
		acceptedWorse = false;
	} else {
		if (iterDecl() >= maxiterdecl && curSched.objective < (1.0 + maxworserate) * bestSched.objective) { // Accept worse with some probability
			accept = true;
			acceptedWorse = true;
		} else {
			accept = false;
			acceptedWorse = false;
		}
	}

	emit sigAcceptConditionFinished(accept);

	return accept;
}

void VNSPlanner::acceptActions() {
	if (acceptedWorse) {
		Debugger::warn << "VNSPlanner::acceptActions : Accepted worse solution with TWT = " << curSched.objective << " ! " << ENDL;

		// Do not save the accepted plan as the best one

	} else {
		Debugger::info << "VNSPlanner::acceptActions : Accepted solution with TWT = " << curSched.objective << ENDL;

		// Save the accepted plan as the best one
		bestPlan = curPlan;
		bestSched = curSched;
	}

	// Update k
	k = 1;

	// Update k for items
	for (int i = 0; i < pmm->prodman->products.size(); i++) {

		prodID2itmK[pmm->prodman->products[i]->ID] = 1;
	}

	// Update kContrib
	kContrib = 1;


	slotUpdateProtocol();

	emit sigAcceptFinished();

	//QTextStream out(stdout);
	//out << "Resource assignment:" << endl;
	//out << *rc << endl;
}

void VNSPlanner::declineActions() {
	Debugger::info << "VNSPlanner::declineActions : Declined solution with TWT = " << curSched.objective << ENDL;

	// Back to the previous solution
	curPlan = prevPlan;
	curSched = prevSched;

	pmm->prodman->bopsFromPlan(curPlan);

	// Clean the resources
	rc->init();

	// Update k
	k = k + kStep;
	if (k > kmax && k < kmax + kStep) {
		k = kmax;
	} else {
		if (k >= kmax + kStep) {
			k = 1;
		}
	}

	// Update k and kmax for items (neighborhood N2)
	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		// Update the max number of item types since the bopIDs are restored 
		prodID2itmKmax[pmm->prodman->products[i]->ID] = pmm->prodman->products[i]->bops[pmm->prodman->products[i]->bopid2idx[pmm->prodman->products[i]->bopID]]->nItemTypes();
	}

	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		prodID2itmK[pmm->prodman->products[i]->ID] += itmKStep;

		if (prodID2itmK[pmm->prodman->products[i]->ID] > prodID2itmKmax[pmm->prodman->products[i]->ID] && prodID2itmK[pmm->prodman->products[i]->ID] < prodID2itmKmax[pmm->prodman->products[i]->ID] + itmKStep) {
			prodID2itmK[pmm->prodman->products[i]->ID] = prodID2itmKmax[pmm->prodman->products[i]->ID];
		} else {
			if (prodID2itmK[pmm->prodman->products[i]->ID] >= prodID2itmKmax[pmm->prodman->products[i]->ID] + itmKStep) {
				prodID2itmK[pmm->prodman->products[i]->ID] = 1;
			}
		}
	}

	// Update kContib (neighborhood N3);
	kContrib = kContrib + kContribStep;
	if (kContrib > kContribMax && kContrib < kContribMax + kContribStep) {
		kContrib = kContribMax;
	} else {
		if (kContrib >= kContribMax + kContribStep) {

			kContrib = 1;
		}
	}

	slotUpdateProtocol();

	emit sigDeclineFinished();

	//Debugger::info << "VNSPlanner::declineActions : Done." << ENDL;
}

bool VNSPlanner::stopCondition() {

	return Planner::stopCondition();
}

void VNSPlanner::stopActions() {
}

void VNSPlanner::preprocessingActions() {
	QTextStream out(stdout);

	// Write the protocol

	QDomDocument run_start_doc("run start");
	QDomElement run_start_elem = run_start_doc.createElement("start");
	//QDomElement obj_elem = newsol_doc.createElement("obj");

	run_start_doc.appendChild(run_start_elem);
	//run_end_elem.setAttribute("iterations", iter());
	run_start_elem.setAttribute("timestamp", QTime::currentTime().toString());
	//run_end_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	//newsol_elem.appendChild(obj_elem);

	protocol << run_start_doc;

	protocol.save();

}

void VNSPlanner::postprocessingActions() {
	Debugger::info << "VNSPlanner::postprocessingActions : Postprocessing the results" << ENDL;

	QTextStream out(stdout);

	out << "VNSPlanner::postprocessingActions : Final set of BOPs is: " << endl;

	out << "(";
	for (int i = 0; i < pmm->prodman->products.size() - 1; i++) {
		//out << (pmm->prodman->products[i]->bopID/* << 16 >> 16*/) << ",";

		out << bestPlan.prodID2BOPID[pmm->prodman->products[i]->ID] << ",";
	}
	out << bestPlan.prodID2BOPID[pmm->prodman->products.last()->ID] << ")" << endl;

	out << "VNSPlanner::postprocessingActions : Final schedule : " << endl << bestSched << endl;
	out << "VNSPlanner::postprocessingActions : Final PM : " << endl << bestSched.pm << endl;
	out << "VNSPlanner::postprocessingActions : Final TWT = " << bestSched.objective << endl;

	out << "VNSPlanner::postprocessingActions : Number of sequential iterations without improvements: " << iterDecl() << endl;
	out << "VNSPlanner::postprocessingActions : Total iterations performed: " << iter() << endl;

	//out << "Final resource assignment:" << endl;
	//out << *rc << endl;

	// Write the protocol
	QDomDocument run_end_doc("run summary");
	QDomElement run_end_elem = run_end_doc.createElement("finish");
	//QDomElement obj_elem = newsol_doc.createElement("obj");

	QTime elapsed_hrs;
	elapsed_hrs.setHMS(0, 0, 0, 0);
	elapsed_hrs = elapsed_hrs.addMSecs(_curtime.elapsed());

	run_end_doc.appendChild(run_end_elem);
	run_end_elem.setAttribute("iterations", (int) iter());
	run_end_elem.setAttribute("timestamp", QTime::currentTime().toString());
	run_end_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	run_end_elem.setAttribute("elapsed_hrsm", elapsed_hrs.toString("HH:mm:ss:zzz"));
	//newsol_elem.appendChild(obj_elem);

	protocol << run_end_doc;

	/*
	QDomNodeList solNodes = protocol.elementsByTagName("solution");
	double bestObj = Math::MAX_DOUBLE;
	int bestSolIdx = -1;
	for (int i = 0 ; i < solNodes.size() ; i++){
			out << solNodes.at(i).toElement().attribute("iteration") << endl;
			out << solNodes.at(i).toElement().elementsByTagName("solution").at(0).childNodes().at(0).toElement().text() << endl;
	}
	 */

	protocol.save();
}

void VNSPlanner::shake() {

	//double alpha = 0.8;

	//if (iter() > 1000) alpha = 0.2;

	newPlans.clear();
	newScheds.clear();

	for (int i = 0; i < maxNewPlans; i++) {

		Plan newPlan;

		switch (ns) {
			case NS_N1:
			{
				newPlan = N1(curPlan);
			}
				break;

			case NS_N2:
			{
				newPlan = N2(curPlan);
			}
				break;

			case NS_N3:
			{
				newPlan = N3(curPlan);
			}
				break;

			case NS_N2N1:
			{
				newPlan = N1(curPlan); // Generate a new plan
				newPlan = N2(newPlan); // Change the routes in the new plan
			}
				break;

			case NS_N2N3:
			{
				newPlan = N3(curPlan);
				newPlan = N2(newPlan);
			}
				break;

			case NS_N3N1:
			{
				newPlan = N1(curPlan);
				newPlan = N3(newPlan);
			}
				break;

			case NS_N1N3:
			{
				newPlan = N3(curPlan);
				newPlan = N1(newPlan);
			}
				break;

			case NS_N2N3N1:
			{
				newPlan = N1(curPlan);
				newPlan = N3(newPlan);
				newPlan = N2(newPlan);
			}
				break;

			case NS_N2N1N3:
			{
				newPlan = N3(curPlan);
				newPlan = N1(newPlan);
				newPlan = N2(newPlan);
			}
				break;

			case NS_N2N1PN3:
			{
				if (Rand::rnd<double>() < 0.5) {
					newPlan = N3(curPlan);
				} else {
					newPlan = N1(curPlan);
				}
				newPlan = N2(newPlan);
			}
				break;

			case NS_PN2PN1PN3:
			{
				double rd = Rand::rnd<double>();

				if (rd < 0.33) {
					newPlan = N1(curPlan);
				} else if (rd < 0.67) {
					newPlan = N3(curPlan);
				} else {
					newPlan = N2(curPlan);
				}

			}
				break;

			case NS_N4:
			{
				newPlan = N4(curPlan);
			}
				break;

			case NS_N5:
			{
				newPlan = N5(curPlan);
			}
				break;

			default:
			{

				Debugger::err << "VNSPlanner::shake : Incorrect main NS structure!" << ENDL;
			}
				break;
		}

		newPlans.append(newPlan);

		newScheds.append(Schedule());
	}

	// Update the global PM
	//pmm->updatePM();
}

Plan VNSPlanner::N1(const Plan& curplan) {
	/**
	 * Algorithm:
	 *
	 * 1. Perform exactly k random perturbations of the vector of BOP IDs, i.e. 
	 *	  take it from the Nk.
	 * 2. Update the global PM according to the newly selected BOP IDs. 
	 * 
	 */

	//Debugger::info << "VNSPlanner::N1 : running ..." << ENDL;

	QList<int> prodstochange;
	QSet<int> prodscontained;
	int rndidx;

	prodstochange.clear();

	// Define randomly the products whose BOP ID are to be changed (the products without orders are excluded)
	QList<int> availprodidcs;
	QSet<int> avaiprodtypes = pmm->ordman->availProducts();

	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		if (avaiprodtypes.contains(pmm->prodman->products[i]->type)) {
			availprodidcs.append(i);
		}
	}

	for (int i = 0; i < k; i++) {
		do {

			rndidx = Rand::rnd<Math::uint32>(0, availprodidcs.size()/*pmm->prodman->products.size()*/ - 1);

		} while (prodscontained.contains(rndidx));

		prodscontained.insert(rndidx);
		prodstochange.append(availprodidcs[rndidx]);
	}

	// Restore the state of the products based on the current plan
	pmm->prodman->bopsFromPlan(curplan);

	// Generate a plan corresponding to the current state of the products
	Plan res;

	res = curplan;

	// For each product to be changed select randomly one of its available BOP IDs
	Product* curprod; // Pointer to the current product
	for (int i = 0; i < k; i++) {

		curprod = pmm->prodman->products[prodstochange[i]];

		// Select a random BOP
		res.prodID2BOPID[curprod->ID] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; // curBOPIDs[prodstochange[i]] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; //pmm->prodman->products[prodstochange[i]]->decompbopids[setidx][elidx];
		// Set the selected BOP ID as the current
		curprod->bopID = res.prodID2BOPID[curprod->ID]; //curBOPIDs[prodstochange[i]];

		// Set the corresponding routes
		res.prodID2ItemID2RouteIdx[curprod->ID] = curprod->bopByID(curprod->bopID)->itemRouteIdx();

		// IMPORTANT!!! Leave the item type route indices as they were before the modification

		// When changing the product's BOP the corresponding VNS parameters should be updated
		prodID2itmK[curprod->ID] = 1;
		//prodID2itmKmax[curprod->ID] = curprod->bopByID(curprod->bopID)->nItemTypes();
		prodID2itmKmax[curprod->ID] = curprod->bopByID(curprod->bopID)->nItems();

	}

	// Return the newly generated plan
	return res;

	//Debugger::info << "VNSPlanner::N1 : done." << ENDL;
}

Plan VNSPlanner::N2(const Plan& curplan) {

	QTextStream out(stdout);
	//Debugger::info << "VNSPlanner::N2 : running ..." << ENDL;

	QList<int> itmstochange;
	QSet<int> itmscontained;
	int rndidx;

	itmstochange.clear();

	QList<int> prodstochange;
	QSet<int> prodscontained;

	prodstochange.clear();
	prodscontained.clear();

	// Define randomly the products whose BOP ID are to be changed
	//Debugger::info << "VNSPlanner::N2 : Defining products for change ..." << ENDL;
	// Define randomly the products whose BOP ID are to be changed (the products without orders are excluded)
	QList<int> availprodidcs;
	QSet<int> avaiprodtypes = pmm->ordman->availProducts();

	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		if (avaiprodtypes.contains(pmm->prodman->products[i]->type)) {
			availprodidcs.append(i);
		}
	}

	for (int i = 0; i < k; i++) {
		do {

			rndidx = Rand::rnd<Math::uint32>(0, availprodidcs.size()/*pmm->prodman->products.size()*/ - 1);

		} while (prodscontained.contains(rndidx));

		prodscontained.insert(rndidx);
		prodstochange.append(availprodidcs[rndidx]);
	}

	//Debugger::info << "VNSPlanner::N2 : Done ..." << ENDL;

	// Restore the state of the products based on the current plan
	pmm->prodman->bopsFromPlan(curplan);

	// Iterate over all the products to change
	for (int i = 0; i < prodstochange.size(); i++) {

		int bopid = pmm->prodman->products[prodstochange[i]]->bopID;
		int bopidx = pmm->prodman->products[prodstochange[i]]->bopid2idx[bopid];

		//QHash<int, QList<Route*> > itmtype2routes = pmm->prodman->products[prodstochange[i]]->bops[bopidx]->itemRoutes();
		//QHash<int, int> itmtype2routeidx = pmm->prodman->products[prodstochange[i]]->bops[bopidx]->itemRouteIdx();
		//QList<int> itmtypes = itmtype2routes.keys(); // Item types in the current BOP with the current BOM
		QHash<int, QList<Route*> > itmID2routes = pmm->prodman->products[prodstochange[i]]->bops[bopidx]->itemRoutes();
		QHash<int, int> itmID2routeidx = pmm->prodman->products[prodstochange[i]]->bops[bopidx]->itemRouteIdx();
		QList<int> itmIDs = itmID2routeidx.keys(); // Item types in the current BOP with the current BOM

		// Define the items which have to be changed in the current product
		//Debugger::info << "VNSPlanner::N2 : Defining items for change in product " << pmm->prodman->products[prodstochange[i]]->ID << ENDL;
		//for (int j = 0; j < itmtypes.size(); j++) {
		//    out << itmtypes[j] << ",";
		//}
		//out << endl;
		//out << "Changing " << prodID2itmK[pmm->prodman->products[prodstochange[i]]->ID] << " item types..." << endl;
		itmscontained.clear();
		itmstochange.clear();
		for (int j = 0; j < prodID2itmK[pmm->prodman->products[prodstochange[i]]->ID]; j++) {
			do {

				rndidx = Rand::rnd<Math::uint32>(0, itmIDs.size() - 1);

			} while (itmscontained.contains(itmIDs[rndidx]));

			itmscontained.insert(itmIDs[rndidx]);
			itmstochange.append(itmIDs[rndidx]);
		}
		//Debugger::info << "VNSPlanner::N2 : Done. " << ENDL;

		// Change randomly the routes of the selected items
		for (int j = 0; j < itmstochange.size(); j++) {
			//out << "Changing route for item type " << itmstochange[j] << endl;
			if (!itmID2routeidx.contains(itmstochange[j])) {
				Debugger::err << "VNSPlanner::N2 : Element does not exist!" << ENDL;
			}

			itmID2routeidx[itmstochange[j]] = Rand::rnd<Math::uint32>(0, itmID2routes[itmstochange[j]].size() - 1);

			//Debugger::info << itmtype2routeidx[itmstochange[j]] << ENDL;
			//Debugger::info << prodID2itmKmax[pmm->prodman->products[i]->ID] << ENDL;
		}

		// Set the new item type routes
		pmm->prodman->products[prodstochange[i]]->bops[bopidx]->setItemRouteIdx(itmID2routeidx);

	}

	// Return the final plan
	Plan res;
	res = pmm->prodman->bops2Plan();

	// Return the newly generated plan

	return res;

	//Debugger::info << "VNSPlanner::N2 : done." << ENDL;
	//getchar();
}

Plan VNSPlanner::N3(const Plan& curplan) {
	/**
	 * Algorithm:
	 *
	 * 1. Perform exactly kConrib random perturbations of the vector of BOP IDs. 
	 *	  Only kContrib most contributing products should be selected 
	 *	  
	 * 2. Update the global PM according to the newly selected BOP IDs. 
	 * 
	 */

	kContribPow = 2.0 * (1.0 - double(iter()) / double(_maxiter));

	//Debugger::info << "VNSPlanner::N1 : running ..." << ENDL;
	double totalContrib = 0.0; // Total contribution of the orders corresponding to all products

	// For every product calculate its contribution to the scheduling criterion
	QList<double> prodContrib;
	QMultiMap<double, int> contrib2prodIdx;
	QList<Product*> products = pmm->prodman->productsList();
	QList<Order*> orders;

	prodContrib.clear();
	contrib2prodIdx.clear();

	// Define randomly the products whose BOP ID are to be changed (the products without orders are excluded)
	QSet<int> availprodtypes = pmm->ordman->availProducts();

	for (int i = 0; i < products.size(); i++) {
		if (availprodtypes.contains(products[i]->type)) { // Filter the products which have no orders

			prodContrib.append(0.0);

			orders = pmm->ordman->ordersByType(products[i]->type);

			for (int j = 0; j < orders.size(); j++) {
				//if (!schedule->ordID2objContrib.contains(orders[j]->ID)) {
				//Debugger::err << "Orders do not contribute!" << ENDL;
				//}
				prodContrib.last() += Math::pow(curSched.ordID2objContrib[orders[j]->ID], kContribPow);
				//totalContrib += Math::pow(schedule->ordID2objContrib[orders[j]->ID], kContribPow);
				//Debugger::info << orders[j]->ID << " : " << schedule->ordID2objContrib[orders[j]->ID] << ENDL;
			}

			contrib2prodIdx.insertMulti(prodContrib.last(), i);
		} else {

		}
	}


	QList<int> prodstochange;
	prodstochange.clear();
	//Debugger::info << "VNSPlanner::N3 : Searching products to change..." << ENDL;
	while (prodstochange.size() < kContrib) {
		// Recalculate the total contribution and the intervals for the products which haven't been selected so far
		QList<QPair<double, double> > interval;
		double istart = 0.0;
		double iend = 0.0;
		totalContrib = 0.0;
		for (QMultiMap<double, int>::iterator iter = contrib2prodIdx.begin(); iter != contrib2prodIdx.end(); iter++) {
			iend = istart + iter.key();

			totalContrib += Math::pow(iter.key(), kContribPow);

			interval.append(QPair<double, double>(istart, iend));

			istart = iend;
		}

		// Generate the new arbitrary point
		double curPoint = Rand::rnd<double>(0.0, totalContrib);

		// Find the interval containing the point
		int i = 0;
		for (QMultiMap<double, int>::iterator iter = contrib2prodIdx.begin(); iter != contrib2prodIdx.end(); iter++) {
			if (interval[i].first < curPoint && curPoint <= interval[i].second) { // Found the interval
				// This product will be changed
				prodstochange.append(iter.value());

				//Eliminate the element
				contrib2prodIdx.erase(iter);

				i = contrib2prodIdx.size() - 1;
				break; // From for()
			}

			i++;
		}

		// If all the rest available products have no contribution then select the orders randomly
		if (i == contrib2prodIdx.size() && prodstochange.size() < kContrib) {
			double prob = 1.0 / double(contrib2prodIdx.size());
			while (prodstochange.size() < kContrib) {
				for (QMultiMap<double, int>::iterator iter = contrib2prodIdx.begin(); iter != contrib2prodIdx.end(); iter++) {
					if (Rand::rnd<double>() <= prob) { // Found the interval
						// This product will be changed
						prodstochange.append(iter.value());

						//Eliminate the element
						contrib2prodIdx.erase(iter);
						break; // From for()
					}
				}
			}
		} // if() for selecting orders with zero contributions

	} // Main while()

	//Debugger::info << "VNSPlanner::N3 : Done searching products to change." << ENDL;

	//Debugger::info << prodstochange.size() << ENDL;
	//for (int i = 0; i < prodstochange.size(); i++) {
	//Debugger::info << prodstochange[i] << ENDL;
	//}

	// Restore the state of the products based on the current plan
	pmm->prodman->bopsFromPlan(curplan);

	// Generate a plan corresponding to the current state of the products
	Plan res;
	res = curplan;

	// For each product to be changed select randomly one of its available BOP IDs
	Product* curprod; // Pointer to the current product
	for (int i = 0; i < kContrib; i++) {

		curprod = pmm->prodman->products[prodstochange[i]];

		// Select a random BOP
		res.prodID2BOPID[curprod->ID] = curprod->selectBOPByRank()->ID; // curBOPIDs[prodstochange[i]] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; //pmm->prodman->products[prodstochange[i]]->decompbopids[setidx][elidx];
		// Set the selected BOP ID as the current
		curprod->bopID = res.prodID2BOPID[curprod->ID]; //curBOPIDs[prodstochange[i]];

		// Set the corresponding routes
		res.prodID2ItemID2RouteIdx[curprod->ID] = curprod->bopByID(curprod->bopID)->itemRouteIdx();

		// IMPORTANT!!! Leave the item type route indices as they were before the modification

		// When changing the product's BOP the corresponding VNS parameters should be updated
		prodID2itmK[curprod->ID] = 1;
		//prodID2itmKmax[curprod->ID] = curprod->bopByID(curprod->bopID)->nItemTypes();
		prodID2itmKmax[curprod->ID] = curprod->bopByID(curprod->bopID)->nItems();

	}

	// Return the newly generated plan
	return res;

	//Debugger::info << "VNSPlanner::N3 : done." << ENDL;
}

Plan VNSPlanner::N4(const Plan& curplan) {
	return curplan; // Since no modification for the case of different item routes performed : I had no time to do it for this function

	/**
	 * Algorithm:
	 *
	 * 1. Perform exactly k random perturbations of the vector of BOP IDs, i.e. 
	 *	  take it from the Nk.
	 * 2. For the affected BOMs change the routes of some items subset randomly.
	 * 3. Update the global PM according to the newly selected BOP IDs. 
	 * 
	 */

	//Debugger::info << "VNSPlanner::N4 : running ..." << ENDL;

	QList<int> prodstochange;
	QSet<int> prodscontained;
	int rndidx;

	prodstochange.clear();

	// Define randomly the products whose BOP ID are to be changed (the products without orders are excluded)
	QList<int> availprodidcs;
	QSet<int> avaiprodtypes = pmm->ordman->availProducts();

	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		if (avaiprodtypes.contains(pmm->prodman->products[i]->type)) {
			availprodidcs.append(i);
		}
	}

	for (int i = 0; i < k; i++) {
		do {

			rndidx = Rand::rnd<Math::uint32>(0, availprodidcs.size()/*pmm->prodman->products.size()*/ - 1);

		} while (prodscontained.contains(rndidx));

		prodscontained.insert(rndidx);
		prodstochange.append(availprodidcs[rndidx]);
	}

	// Restore the state of the products based on the current plan
	pmm->prodman->bopsFromPlan(curplan);

	// Generate a plan corresponding to the current state of the products
	Plan res;

	res = curplan;

	// For each product to be changed select randomly one of its available BOP IDs
	Product* curprod; // Pointer to the current product
	for (int i = 0; i < k; i++) {

		curprod = pmm->prodman->products[prodstochange[i]];

		// Select a random BOP
		res.prodID2BOPID[curprod->ID] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; // curBOPIDs[prodstochange[i]] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; //pmm->prodman->products[prodstochange[i]]->decompbopids[setidx][elidx];
		// Set the selected BOP ID as the current one
		curprod->bopID = res.prodID2BOPID[curprod->ID]; //curBOPIDs[prodstochange[i]];


		// Get the item indices within the current BOM
		QHash<int, int> itemRouteIndices = curprod->bopByID(curprod->bopID)->itemRouteIdx();

		// Select not more than itemK items to change
		int nItemTypes2Change = Math::min(itemRouteIndices.size(), prodID2itmK[curprod->ID]);

		QList<int> selectedItemTypes = itemRouteIndices.keys();
		while (nItemTypes2Change > 0) {
			selectedItemTypes.removeAt(Rand::rnd<Math::uint32>(0, selectedItemTypes.size() - 1));
			nItemTypes2Change--;
		}

		//Debugger::info << selectedItemTypes.size() << ENDL;

		for (int j = 0; j < selectedItemTypes.size(); j++) {
			itemRouteIndices[selectedItemTypes[j]] = Rand::rnd<Math::uint32>(0, curprod->bopByID(curprod->bopID)->itemTypeRoutes()[selectedItemTypes[j]].size() - 1);
		}

		// Set the corresponding routes
		res.prodID2ItemID2RouteIdx[curprod->ID] = itemRouteIndices; //curprod->bopByID(curprod->bopID)->itemRouteIdx();


		// When changing the product's BOP the corresponding VNS parameters should be updated
		prodID2itmK[curprod->ID] = 1;
		prodID2itmKmax[curprod->ID] = curprod->bopByID(curprod->bopID)->nItemTypes();

	}

	// Return the newly generated plan
	return res;

	//Debugger::info << "VNSPlanner::N4 : done." << ENDL;
}

Plan VNSPlanner::N5(const Plan& curplan) {
	return curplan; // Since no modification for the case of different item routes performed : I had no time to do it for this function

	/**
	 * Algorithm:
	 *
	 * 1. Perform exactly k random perturbations of the vector of BOP IDs, i.e. 
	 *	  take it from the Nk.
	 * 2. For the NON-affected BOMs change the routes of some items subset randomly.
	 * 3. Update the global PM according to the newly selected BOP IDs. 
	 * 
	 */

	//Debugger::info << "VNSPlanner::N4 : running ..." << ENDL;

	QList<int> prodstochange;
	QSet<int> prodscontained;
	int rndidx;

	prodstochange.clear();

	// Define randomly the products whose BOP ID are to be changed (the products without orders are excluded)
	QList<int> availprodidcs;
	QSet<int> avaiprodtypes = pmm->ordman->availProducts();

	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		if (avaiprodtypes.contains(pmm->prodman->products[i]->type)) {
			availprodidcs.append(i);
		}
	}

	for (int i = 0; i < k; i++) {
		do {

			rndidx = Rand::rnd<Math::uint32>(0, availprodidcs.size()/*pmm->prodman->products.size()*/ - 1);

		} while (prodscontained.contains(rndidx));

		prodscontained.insert(rndidx);
		prodstochange.append(availprodidcs[rndidx]);
	}

	// Restore the state of the products based on the current plan
	pmm->prodman->bopsFromPlan(curplan);

	// Generate a plan corresponding to the current state of the products
	Plan res;

	res = curplan;

	// For each product to be changed select randomly one of its available BOP IDs
	Product* curprod; // Pointer to the current product
	for (int i = 0; i < k; i++) {

		curprod = pmm->prodman->products[prodstochange[i]];

		// Select a random BOP
		res.prodID2BOPID[curprod->ID] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; // curBOPIDs[prodstochange[i]] = curprod->bops[Rand::rnd<Math::uint32>(0, curprod->bops.size() - 1)]->ID; //pmm->prodman->products[prodstochange[i]]->decompbopids[setidx][elidx];
		// Set the selected BOP ID as the current one
		curprod->bopID = res.prodID2BOPID[curprod->ID]; //curBOPIDs[prodstochange[i]];

		// Set the corresponding routes
		res.prodID2ItemID2RouteIdx[curprod->ID] = curprod->bopByID(curprod->bopID)->itemRouteIdx();

		// When changing the product's BOP the corresponding VNS parameters should be updated
		prodID2itmK[curprod->ID] = 1;
		prodID2itmKmax[curprod->ID] = curprod->bopByID(curprod->bopID)->nItemTypes();

	}

	// Change the routes only for the non-affected products
	for (int i = 0; i < availprodidcs.size(); i++) {
		if (!prodstochange.contains(availprodidcs[i])) {
			curprod = pmm->prodman->products[availprodidcs[i]];
		} else {
			continue;
		}

		// Get the item indices within the current BOM
		QHash<int, int> itemRouteIndices = curprod->bopByID(curprod->bopID)->itemRouteIdx();

		// Select not more than itemK items to change
		int nItemTypes2Change = Math::min(itemRouteIndices.size(), prodID2itmK[curprod->ID]);

		QList<int> selectedItemTypes = itemRouteIndices.keys();
		while (nItemTypes2Change > 0) {
			selectedItemTypes.removeAt(Rand::rnd<Math::uint32>(0, selectedItemTypes.size() - 1));
			nItemTypes2Change--;
		}

		//Debugger::info << selectedItemTypes.size() << ENDL;

		for (int j = 0; j < selectedItemTypes.size(); j++) {
			itemRouteIndices[selectedItemTypes[j]] = Rand::rnd<Math::uint32>(0, curprod->bopByID(curprod->bopID)->itemTypeRoutes()[selectedItemTypes[j]].size() - 1);
		}

		// Set the corresponding routes
		res.prodID2ItemID2RouteIdx[curprod->ID] = itemRouteIndices;

	}

	// Return the newly generated plan
	return res;

	//Debugger::info << "VNSPlanner::N4 : done." << ENDL;
}

void VNSPlanner::localSearch() {

}

Plan VNSPlanner::initialSolution() {
	//int j;
	QVector<int> curBOPIDs(pmm->prodman->products.size());

	switch (solInitType) {
		case SOL_INIT_RND:
		{
			for (int i = 0; i < curBOPIDs.size(); i++) {
				//j = Rand::rnd<Math::uint32>(0, pmm->prodman->products[i]->bops.size() - 1); //Rand::rnd<Math::uint32>(0, pmm->prodman->products[i]->decompbopids.size() - 1); //Rand::rnd<Math::uint32>(0, prodBOPIDs[i].size() - 1);
				curBOPIDs[i] = pmm->prodman->products[i]->rndBOP()->ID; //pmm->prodman->products[i]->bops[j]->ID; //pmm->prodman->products[i]->decompbopids[j][Rand::rnd<Math::uint32>(0, pmm->prodman->products[i]->decompbopids[j].size() - 1)];
			}
		}
			break;

		case SOL_INIT_RANK:
		{
			for (int i = 0; i < curBOPIDs.size(); i++) {
				//j = Rand::rnd<Math::uint32>(0, pmm->prodman->products[i]->bops.size() - 1); //Rand::rnd<Math::uint32>(0, pmm->prodman->products[i]->decompbopids.size() - 1); //Rand::rnd<Math::uint32>(0, prodBOPIDs[i].size() - 1);
				curBOPIDs[i] = pmm->prodman->products[i]->bestBOPByRank()->ID; //pmm->prodman->products[i]->bops[j]->ID; //pmm->prodman->products[i]->decompbopids[j][Rand::rnd<Math::uint32>(0, pmm->prodman->products[i]->decompbopids[j].size() - 1)];
			}
		}
			break;

		default:
		{
			Debugger::err << "VNSPlanner::initialSolution : No initialization rule specified!!!" << ENDL;
		}
			break;
	}

	/** Set the initial BOP IDs for each product. */
	for (int i = 0; i < curBOPIDs.size(); i++) {
		pmm->prodman->products[i]->bopID = curBOPIDs[i];
	}

	Plan initPlan;

	// Collect the current plan and set it as the best one
	initPlan = pmm->prodman->bops2Plan();

	return initPlan;

}

void VNSPlanner::slotAssessPlan(const int& idx) {
	QTextStream out(stdout);

	// Eliminate non-relevant scheduler options
	schedOptions.clear();

	// Set the default adjustments (they will be modified later)
	schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
	schedOptions["LS_CHK_COR"] = "false";
	schedOptions["LS_MAX_ITER"] = "0";
	schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "0";
	schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

	// Current iteration and max iterations
	int currentIter = iter();
	int maximumIter = maxIter();
	double curIterFloat = double(currentIter) / double(maximumIter);
	//	out << "Curiter float : " << curIterFloat << endl;

	int curIntIdx = -1; // Index of the current progress interval

	// Find the progress in context of the planner
	QVector<QPair<double, double> > curPlanProgressIntervals = strategy.getPlannerProgressIntervals();
	curIntIdx = -1; // Index of the current progress interval

	for (int i = 0; i < curPlanProgressIntervals.size(); i++) {
		QString curStartBracket = strategy.getPlannerProgressIntervalsBrackets()[i].first;
		QString curEndBracket = strategy.getPlannerProgressIntervalsBrackets()[i].second;

		double curIntervalStartFloat = curPlanProgressIntervals[i].first;
		double curIntervalEndFloat = curPlanProgressIntervals[i].second;

		//		out << "Checking interval : " << curStartBracket << curIntervalStartFloat << " , " << curIntervalEndFloat << curEndBracket << endl;

		if (curStartBracket == QString("[") && curEndBracket == QString("]")) {
			if (curIntervalStartFloat <= curIterFloat && curIterFloat <= curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

		if (curStartBracket == QString("[") && curEndBracket == QString(")")) {
			if (curIntervalStartFloat <= curIterFloat && curIterFloat < curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

		if (curStartBracket == QString("(") && curEndBracket == QString("]")) {
			if (curIntervalStartFloat < curIterFloat && curIterFloat <= curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

		if (curStartBracket == QString("(") && curEndBracket == QString(")")) {
			if (curIntervalStartFloat < curIterFloat && curIterFloat < curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

	}

	if (curIntIdx == -1) {
		Debugger::err << "VNSPlanner::slotAssessPlan : Failed to find the iteration interval for the planner!!!" << ENDL;
	}

	// Get the adjustments of the planner
	SchedulerOptions curPlannerOptions = strategy.getPlannerOptions()[curIntIdx];

	//	out << "Interval : " << curIntIdx << endl;

	// Set the actual scheduler options
	for (SchedulerOptions::iterator iter = curPlannerOptions.begin(); iter != curPlannerOptions.end(); iter++) {
		schedOptions[iter.key()] = iter.value();
	}



	// Find the progress interval which the current iteration belongs to (for the scheduler)
	QVector<QPair<double, double> > curSchedProgressIntervals = strategy.getSchedulerProgressIntervals();
	curIntIdx = -1; // Index of the current progress interval


	for (int i = 0; i < curSchedProgressIntervals.size(); i++) {
		QString curStartBracket = strategy.getSchedulerProgressIntervalsBrackets()[i].first;
		QString curEndBracket = strategy.getSchedulerProgressIntervalsBrackets()[i].second;

		double curIntervalStartFloat = curSchedProgressIntervals[i].first;
		double curIntervalEndFloat = curSchedProgressIntervals[i].second;

		//		out << "Checking interval : " << curStartBracket << curIntervalStartFloat << " , " << curIntervalEndFloat << curEndBracket << endl;

		if (curStartBracket == QString("[") && curEndBracket == QString("]")) {
			if (curIntervalStartFloat <= curIterFloat && curIterFloat <= curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

		if (curStartBracket == QString("[") && curEndBracket == QString(")")) {
			if (curIntervalStartFloat <= curIterFloat && curIterFloat < curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

		if (curStartBracket == QString("(") && curEndBracket == QString("]")) {
			if (curIntervalStartFloat < curIterFloat && curIterFloat <= curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

		if (curStartBracket == QString("(") && curEndBracket == QString(")")) {
			if (curIntervalStartFloat < curIterFloat && curIterFloat < curIntervalEndFloat) {
				curIntIdx = i;
				break;
			}
		}

	}

	if (curIntIdx == -1) {
		Debugger::err << "VNSPlanner::slotAssessPlan : Failed to find the iteration interval!!!" << ENDL;
	}

	// Get the name of the current scheduler to use and its adjustments
	QString curSchedulerName = strategy.getSchedulerNames()[curIntIdx];
	SchedulerOptions curSchedulerOptions = strategy.getSchedulerOptions()[curIntIdx];

	//	out << "Interval : " << curIntIdx << endl;
	//out << "Running scheduler : " << curSchedulerName << endl;

	// Set the actual scheduler options
	for (SchedulerOptions::iterator iter = curSchedulerOptions.begin(); iter != curSchedulerOptions.end(); iter++) {
		schedOptions[iter.key()] = iter.value();
	}

	//out << "Scheduler options : " << endl << schedOptions << endl;

	//out << endl << "VNSPlanner::slotAssessPlan " << idx << endl << endl;
	//getchar();

	//schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";
	//schedOptions["LS_MAX_ITER"] = "50";

	//	if (iter() > maxIter() / 10 * 9 || _curtime.elapsed() > maxTimeMs() / 2) {
	//		schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";
	//	}
	//
	//	if (iter() > maxIter() - 1) { // Last iteration of the planning algorithm
	//		schedOptions["LS_MAX_ITER"] = schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"]; //"20000";
	//
	//		schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";
	//
	//		if (idx == 0) newPlans[idx] = bestPlan; // One of the optimized plans in the last iteration should be the best found so far
	//	}

	if (schedOptions["PLANNER_BEST_PLAN"] == "true") { // Select currently the best plan for scheduling
		if (idx == 0) newPlans[idx] = bestPlan;

		out << "Selected the best plan for scheduling." << endl;
	}


	Plan curplan = newPlans[idx];

	pmm->prodman->bopsFromPlan(curplan);

	// Update the global process model according to the current state
	ProcessModel planPM = pmm->plan2PM(curplan);

	/** Initialize the resources for the new clean scheduling. */
	rc->init();

	// Assign operations of the current global PM to the resources.
	for (ListDigraph::NodeIt nit(planPM.graph); nit != INVALID; ++nit) {
		if (!rc->assign(planPM.ops[nit])) {

			out << "VNSPlanner::assessPlan : Assigning operation: " << *pmm->pm.ops[nit] << endl;
			Debugger::err << "VNSPlanner::assessPlan : Failed to assign operation to resource!" << ENDL;
		}
	}

	Resources newrc;
	newrc = *rc;
	newrc.init();

	// Set the objective
	//newScheds[idx] << schedAlgs[idx]->obj;

	// DEBUG
	//schedOptions["LS_MAX_ITER"] = "4";

	// IMPORTANT!!! Set the ID of this scheduler as the index corresponding to the current plan
	schedAlgs[idx]->ID = idx;
	schedAlgs[idx]->pm = planPM;
	schedAlgs[idx]->rc = newrc;
	schedAlgs[idx]->sched = &newScheds[idx];
	schedAlgs[idx]->options = schedOptions;

	// Now the process model can be cleaned
	planPM.clearSchedRelData();

	// Run the scheduler in a separate thread
	//schedThreads[idx]->start(QThread::TimeCriticalPriority);
	schedThreads[idx]->runScheduler();

}

void VNSPlanner::slotPlanAssessFinished(const int& idx) {
	//Debugger::info << "VNSPlanner::slotPlanAssessFinished : Finished thread " << idx << ENDL;
	//Debugger::info << "VNSPlanner::slotPlanAssessFinished : Current objective : " << newScheds[idx].objective << ENDL;
	QTextStream out(stdout);

	//out << endl << "VNSPlanner::slotPlanAssessFinished " << idx << endl << endl;

	//	out << "Average TG utilization : " << endl;
	//	for (int i = 0; i < rc->tools.size(); i++) {
	//		out << schedAlgs[idx]->rc.tools[i]->ID << ":" << schedAlgs[idx]->rc.tools[i]->avgUtil() << ", ";
	//	}
	//	out << endl;

	//schedThreads[idx]->quit();
	//schedThreads[idx]->wait();

	if (assessedNewPlansIdx.size() == 0) {
		curSched = newScheds[idx];
		curPlan = newPlans[idx];
	} else {
		if (curSched.objective > newScheds[idx].objective) {
			curSched = newScheds[idx];
			curPlan = newPlans[idx];
		}
	}

	assessedNewPlansIdx.insert(idx);

	if (assessedNewPlansIdx.size() == maxNewPlans) { // All the plans have been assessed

		//out << endl << "VNSPlanner::slotPlanAssessFinished : Assessed ALL Plans!" << endl;
		//getchar();

		emit sigAllPlansAssessed();

	}

}

void VNSPlanner::slotAllPlansAssessed() {
	QTextStream out(stdout);

	// Output of the current BOP IDs
	out << "VNSPlanner::slotAllPlansAssessed : Iteration " << this->iter() << endl;
	out << "VNSPlanner::slotAllPlansAssessed : Current BOP IDs: (";
	for (int i = 0; i < pmm->prodman->products.size() - 1; i++) {

		out << pmm->prodman->products[i]->bopID << ",";
	}
	out << pmm->prodman->products.last()->bopID << ")" << endl;

	Debugger::info << "VNSPlanner::slotAllPlansAssessed : Best found objective value : " << bestSched.objective << ENDL;
	Debugger::info << "VNSPlanner::slotAllPlansAssessed : Current objective value : " << curSched.objective << ENDL;

	/** Notify that the assess actions have been finished. */

	emit sigAssessFinished();
}

void VNSPlanner::slotInitSched() {
	QTextStream out(stdout);

	// Update the global process model according to the current state
	ProcessModel planPM = pmm->plan2PM(curPlan);

	/** Initialize the resources for the new clean scheduling. */
	rc->init();

	// Assign operations of the current global PM to the resources.
	for (ListDigraph::NodeIt nit(planPM.graph); nit != INVALID; ++nit) {
		if (!rc->assign(planPM.ops[nit])) {

			out << "VNSPlanner::assessPlan : Assigning operation: " << *pmm->pm.ops[nit] << endl;
			Debugger::err << "VNSPlanner::assessPlan : Failed to assign operation to resource!" << ENDL;
		}
	}

	Resources newrc;
	newrc = *rc;
	newrc.init();

	//curSched << schedAlgs[0]->obj;

	schedAlgs[0]->ID = 0;
	schedAlgs[0]->pm = planPM;
	schedAlgs[0]->rc = newrc;
	schedAlgs[0]->sched = &curSched;
	schedAlgs[0]->options = schedOptions;

	//schedAlgs[0]->ls.maxIter(0);
	//schedAlgs[0]->ls.checkCorectness(false);

	//prioscheduler->schedule();

	// Now the process model can be cleaned
	planPM.clearSchedRelData();

	disconnect(schedAlgs[0], SIGNAL(sigFinished(const int&)), this, SIGNAL(sigPlanAssessFinished(const int&)));
	connect(schedAlgs[0], SIGNAL(sigFinished()), this, SLOT(slotInitSchedFinished()));
	//connect(initSchedThread, SIGNAL(started()), prioscheduler, SLOT(slotSchedule()));
	//connect(initSchedThread, SIGNAL(finished()), this, SLOT(slotInitSchedFinished()));
	//initSchedThread->start();

	//out << "VNSPlanner::slotInitSched : Running the initial scheduler ... " << endl;

	schedAlgs[0]->slotSchedule();
}

void VNSPlanner::slotInitSchedFinished() {
	QTextStream out(stdout);

	//out << "VNSPlanner::slotInitSchedFinished : Finished the initial scheduler ... " << endl;

	// Delete the initalization thread
	//delete initSchedThread;

	disconnect(schedAlgs[0], SIGNAL(sigFinished()), this, SLOT(slotInitSchedFinished()));
	connect(schedAlgs[0], SIGNAL(sigFinished(const int&)), this, SIGNAL(sigPlanAssessFinished(const int&)));

	// Prepare the schedule based on the PM
	//curSched.fromPM(((Scheduler*) schedAlgs[0])->pm);
	// The schedule is already prepared by the algorithm

	// Set the current scheduler as the best one
	bestSched = curSched;

	for (int i = 0; i < pmm->prodman->products.size(); i++) {
		out << curPlan.prodID2BOPID[pmm->prodman->products[i]->ID] << ",";
	}
	out << endl;

	// Initialize the initial best objective value
	Debugger::info << "#########################################################" << ENDL;
	Debugger::info << "VNSPlanner::init : bestObj = " << bestSched.objective << ENDL;
	Debugger::info << "#########################################################" << ENDL;

	//out << schedAlgs[0]->pm << endl;
	//getchar();

	slotUpdateProtocol();

	emit sigInitFinished();
}

void VNSPlanner::slotFinished() {
	Debugger::info << "VNSPlanner::slotFinished : Finishing..." << ENDL;
	for (int i = 0; i < schedThreads.size(); i++) {
		schedThreads[i]->exit(0);
		schedThreads[i]->wait();
		delete schedThreads[i];
	}

	for (int i = 0; i < schedAlgs.size(); i++) {
		delete schedAlgs[i];
	}

	QTextStream out(stdout);
	out << "Emitting sigCompletelyFinished() ... " << endl;
	emit sigCompletelyFinished();
	out << "Done emitting sigCompletelyFinished() " << endl;

	Debugger::info << "VNSPlanner::slotFinished : Finished!" << ENDL;
}

void VNSPlanner::slotIterationFinished() {
	QTextStream out(stdout);

	//Debugger::info << "#########################################################" << ENDL;
	//Debugger::info << "VNSPlanner::slotIterationFinished : Finished iteration " << (int) iter() << ENDL;
	//Debugger::info << "#########################################################" << ENDL;
	//Debugger::info << "#########################################################" << ENDL;
	//Debugger::info << "#########################################################" << ENDL;
	//Debugger::info << "#########################################################" << ENDL;
	//Debugger::info << "#########################################################" << ENDL;
	//Debugger::info << "#########################################################" << ENDL;
	//getchar();
}

void VNSPlanner::slotUpdateProtocol() {
	// Update the protocol
	QTextStream out(stdout);
	QDomDocument newsol_doc("new solution");
	QDomElement newsol_elem = newsol_doc.createElement("solution");
	QDomElement obj_elem = newsol_doc.createElement("obj");

	out << " VNSPlanner::slotUpdateProtocol : Updating the protocol " << endl;

	QTime elapsed_hrs;
	elapsed_hrs.setHMS(0, 0, 0, 0);
	elapsed_hrs = elapsed_hrs.addMSecs(_curtime.elapsed());
	newsol_doc.appendChild(newsol_elem);
	newsol_elem.setAttribute("iteration", (int) iter());
	newsol_elem.setAttribute("timestamp", QTime::currentTime().toString());
	newsol_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	newsol_elem.setAttribute("elapsed_hrsm", elapsed_hrs.toString("HH:mm:ss:zzz"));
	newsol_elem.appendChild(obj_elem);

	obj_elem.appendChild(newsol_doc.createTextNode(QString::number(bestSched.objective)));

	protocol << newsol_doc;

	out << " VNSPlanner::slotUpdateProtocol : Done updating the protocol " << endl;

	//protocol.save();
}

VNSPlanner& VNSPlanner::operator<<(VNSPlannerStrategy& strategy) {

	this->strategy = strategy;

	return *this;
}

/** ######################################################################### */