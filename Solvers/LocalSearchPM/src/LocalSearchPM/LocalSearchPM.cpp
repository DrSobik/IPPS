/* 
 * File:   LocalSearchPM.cpp
 * Author: DrSobik
 * 
 * Created on September 18, 2012, 12:28 PM
 */

#include "LocalSearchPM.h"
//#include "TGScheduler.h"
//#include "LocalSearchSM.h"


//#define LS_MSG

/**  ************************  LocalSearchPM  ****************************** **/

LocalSearchPM::LocalSearchPM() : intRNG(NULL), floatRNG(NULL), obj(NULL) {
    pm = NULL;

    //lsmode = IMPROV;

    alpha = 0.1;

    nisteps = 0;

    nodeI = INVALID;
    nodeT = INVALID;

    critNodesUpdateFreq = 100;

    checkCorrectness(true);

    bestPosToMove = false;

    // RNG (created and set by default)
    SmartPointer<Common::Rand::MT19937 < Math::uint32 >> intRandGen(new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed())); // The random numbers generator (integers)
    SmartPointer<Common::Rand::MT19937<double>> floatRandGen(new Common::Rand::MT19937<double>(Rand::rndSeed())); // The random numbers generator (floats)

    this->setRandGens(intRandGen.getPointer(), floatRandGen.getPointer());

}

LocalSearchPM::LocalSearchPM(LocalSearchPM& orig) : IterativeAlg(orig), SchedSolver(), intRNG(nullptr), floatRNG(nullptr), obj(nullptr) {

    QTextStream out(stdout);

    out << "LocalSearchPM::LocalSearchPM(LocalSearchPM& orig) : Running..." << endl;

    //Debugger::info << "LocalSearchPM::LocalSearchPM(LocalSearchPM& orig) ... " << ENDL;

    alpha = orig.alpha;

    nisteps = orig.nisteps;

    nodeI = orig.nodeI;
    nodeT = orig.nodeT;

    critNodesUpdateFreq = orig.critNodesUpdateFreq;

    checkCorrectness(orig._check_correctness);

    if (this->obj != nullptr) {
	delete this->obj;

    }

    if (orig.obj == nullptr) {
	Debugger::err << "LocalSearchPM::LocalSearchPM : Trying to clone a NULL objective!!!" << ENDL;
    }

    this->obj = orig.obj->clone();

    if (orig.intRNG.getPointer() == nullptr) {
	Debugger::err << "LocalSearchPM::LocalSearchPM : Trying to clone a NULL intRNG!!!" << ENDL;
    } else {
	this->intRNG.setPointer(orig.intRNG->clone(), true);
    }

    if (orig.floatRNG.getPointer() == nullptr) {
	Debugger::err << "LocalSearchPM::LocalSearchPM : Trying to clone a NULL floatRNG!!!" << ENDL;
    } else {
	this->floatRNG.setPointer(orig.floatRNG->clone(), true);
    }

    this->bestPosToMove = orig.bestPosToMove;

    settings = orig.settings;

    initScheduler.setPointer(orig.initScheduler->clone(), true);

    out << "LocalSearchPM::LocalSearchPM(LocalSearchPM& orig) : Done." << endl;
}

LocalSearchPM::~LocalSearchPM() {
    //Debugger::info << "LocalSearchPM::~LocalSearchPM : Started... " << ENDL;

    this->pm = NULL;
    this->rc = NULL;
    if (this->obj != NULL) {
	delete this->obj;
	this->obj = NULL;
    }

    //    if (this->intRNG != NULL) {
    //        delete this->intRNG;
    //        this->intRNG = NULL;
    //    }
    //
    //    if (this->floatRNG != NULL) {
    //        delete this->floatRNG;
    //        this->floatRNG = NULL;
    //    }

    //Debugger::info << "LocalSearchPM::~LocalSearchPM : Finished! " << ENDL;

}

LocalSearchPM* LocalSearchPM::clone() {
    return new LocalSearchPM(*this);
}

void LocalSearchPM::setPM(ProcessModel *pm) {
    this->pm = pm;

    // Mark all nodes as movable by default
    node2Movable.clear();
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	ListDigraph::Node curNode = nit;

	node2Movable[ListDigraph::id(curNode)] = true;

    }


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

void LocalSearchPM::setResources(Resources *rc) {
    this->rc = rc;

    // #########################    DEBUG    ###################################    
    /*
    out << "Check reachability for every machine during the LS initialization (having set the resources)..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;

    out << "Check whether the processing times of the operations are OK..." << endl;
    double pt;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    if (pm->ops[nit]->machID >= 0) {
		    pt = ((*this->rc)(pm->ops[nit]->toolID, pm->ops[nit]->machID)).procTime(pm->ops[nit]);

		    if (pm->ops[nit]->p() != pt) {
			    out << *pm << endl;
			    out << "pt = " << pt << endl;
			    out << "p = " << pm->ops[nit]->p() << endl;
			    Debugger::err << "Something is wrong with the processing time for " << pm->ops[nit]->ID << ENDL;
		    }
	    }
    }
    out << "The processing times of the operations are OK." << endl;
     */
    // #########################################################################
}

void LocalSearchPM::setObjective(ScalarObjective* obj) {

    if (this->obj != NULL) {
	delete this->obj;
    }

    if (obj == NULL) {
	Debugger::err << "LocalSearchPM::setObjective : Trying to clone a NULL objective!!!" << ENDL;
    }

    this->obj = obj->clone();
}

void LocalSearchPM::setRandGens(Common::Interfaces::RandGen<Math::uint32>* irg, Common::Interfaces::RandGen<double>* frg) {

    this->intRNG.setPointer(irg->clone(), true);

    this->floatRNG.setPointer(frg->clone(), true);
}

void LocalSearchPM::setCritNodesUpdateFreq(const int& freq) {
    critNodesUpdateFreq = freq;
}

void LocalSearchPM::setBestPosToMove(const bool& active) {
    bestPosToMove = active;
}

void LocalSearchPM::parse(const SchedulerOptions& options) {

    QTextStream out(stdout);

    out << "LocalSearchPM::parse : Parsing settings..." << endl;

    //  ###################  Parse settings  ###################################

    settings = options;

    bool considerSingleSettings = true;
    if (settings.container().contains("ALL_SETTINGS")) {

	if (settings["ALL_SETTINGS"].changed()) {

	    out << "LocalSearchPM::parse : ALL_SETTINGS changed!" << endl;

	    //QRegExp settingsRE("([^\\(]+)\\({1,1}(.*)\\){1,1}@([^@^\\(^\\)]+)");
	    QRegularExpression settingsRE;
	    QRegularExpressionMatch match;

	    QString allSettingsStr = settings["ALL_SETTINGS"].get();


	    // LS_CHK_COR
	    settingsRE.setPattern("LS_CHK_COR=([^;]+);{0,1}");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_CHK_COR:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_CHK_COR"] = match.captured(1);
	    }

	    // LS_MAX_ITER
	    settingsRE.setPattern("LS_MAX_ITER=(\\d+);{0,1}");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_MAX_ITER:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_MAX_ITER"] = match.captured(1);
	    }

	    // LS_MAX_TIME_MS
	    settingsRE.setPattern("LS_MAX_TIME_MS=(\\d+);{0,1}");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_MAX_TIME_MS:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_MAX_TIME_MS"] = match.captured(1);
	    }

	    // LS_CRIT_NODES_UPDATE_FREQ
	    settingsRE.setPattern("LS_CRIT_NODES_UPDATE_FREQ=(\\d+);{0,1}");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_CRIT_NODES_UPDATE_FREQ:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_CRIT_NODES_UPDATE_FREQ"] = match.captured(1);
	    }

	    // LS_BEST_POS_TO_MOVE
	    settingsRE.setPattern("LS_BEST_POS_TO_MOVE=([^;]+);{0,1}");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_BEST_POS_TO_MOVE:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_BEST_POS_TO_MOVE"] = match.captured(1);
	    }

	    // LS_PRIMARY_OBJECTIVE
	    settingsRE.setPattern("LS_PRIMARY_OBJECTIVE=([^;]+@[^;]+);{0,1}");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_PRIMARY_OBJECTIVE:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_PRIMARY_OBJECTIVE"] = match.captured(1);
	    }

	    // LS_INIT_SCHEDULER
	    settingsRE.setPattern("LS_INIT_SCHEDULER=([^\\(]+\\(.*\\)@[^@\\(\\),;]+);?");
	    match = settingsRE.match(allSettingsStr);
	    out << "LocalSearchPM::parse : Parsed: LS_INIT_SCHEDULER:" << match.captured(1) << endl;
	    //getchar();
	    if (match.captured(1) != "") {
		settings["LS_INIT_SCHEDULER"] = match.captured(1);
	    }

	    considerSingleSettings = true;

	} else {

	    considerSingleSettings = false;

	}

	considerSingleSettings = true;

    }


    if (considerSingleSettings) {

	// LS_CHK_COR
	if (settings.container().contains("LS_CHK_COR")) {

	    if (settings["LS_CHK_COR"].changed()) {

		out << "LocalSearchPM::parse : LS_CHK_COR changed!" << endl;

		if (settings["LS_CHK_COR"].get() == "true") {
		    this->checkCorrectness(true);
		    //cout << "Checking correctness" << endl;
		} else {
		    this->checkCorrectness(false);
		    //cout << "NOT checking correctness" << endl;
		}

	    }

	} else {
	    throw ErrMsgException<>(std::string("LocalSearchPM::parse : LS_CHK_COR not specified!"));
	}

	// LS_MAX_ITER
	if (settings.container().contains("LS_MAX_ITER")) {
	    if (settings["LS_MAX_ITER"].changed()) {
		out << "LocalSearchPM::parse : LS_MAX_ITER changed!" << endl;
		this->maxIter(settings["LS_MAX_ITER"].get().toInt());
	    }
	} else {
	    throw ErrMsgException<>(std::string("LocalSearchPM::parse : LS_MAX_ITER not specified!"));
	}

	// LS_MAX_TIME_MS
	if (settings.container().contains("LS_MAX_TIME_MS")) {
	    if (settings["LS_MAX_TIME_MS"].changed()) {
		out << "LocalSearchPM::parse : LS_MAX_TIME_MS changed!" << endl;
		this->maxTimeMs(settings["LS_MAX_TIME_MS"].get().toInt());
	    }
	}

	// LS_CRIT_NODES_UPDATE_FREQ
	if (settings.container().contains("LS_CRIT_NODES_UPDATE_FREQ")) {
	    if (settings["LS_CRIT_NODES_UPDATE_FREQ"].changed()) {
		out << "LocalSearchPM::parse : LS_CRIT_NODES_UPDATE_FREQ changed!" << endl;
		this->setCritNodesUpdateFreq(settings["LS_CRIT_NODES_UPDATE_FREQ"].get().toInt());
	    }
	} else {
	    throw ErrMsgException<>(std::string("LocalSearchPM::parse : LS_CRIT_NODES_UPDATE_FREQ not specified!"));
	}

	// LS_BEST_POS_TO_MOVE
	if (settings.container().contains("LS_BEST_POS_TO_MOVE")) {
	    if (settings["LS_BEST_POS_TO_MOVE"].changed()) {
		out << "LocalSearchPM::parse : LS_BEST_POS_TO_MOVE changed!" << endl;
		if (settings["LS_BEST_POS_TO_MOVE"].get() == "true") {
		    this->setBestPosToMove(true);
		    //cout << "Checking correctness" << endl;
		} else {
		    this->setBestPosToMove(false);
		    //cout << "NOT checking correctness" << endl;
		}
	    }
	} else {
	    throw ErrMsgException<>(std::string("LocalSearchPM::parse : LS_BEST_POS_TO_MOVE not specified!"));
	}

	// Parse the objective
	if (settings.container().contains("LS_PRIMARY_OBJECTIVE")) {

	    if (settings["LS_PRIMARY_OBJECTIVE"].changed()) {

		out << "LocalSearchPM::parse : LS_PRIMARY_OBJECTIVE changed!" << endl;

		QString objStr = settings["LS_PRIMARY_OBJECTIVE"].get();

		QRegularExpression curObjRE("(.*)@(.*)");
		QRegularExpressionMatch match = curObjRE.match(objStr);

		QString objLibName = match.captured(2); // Library where the objective is located
		QString objName = match.captured(1); // Objective name

		QLibrary objLib(objLibName);

		out << "LocalSearchPM::parse : objLibName : " << objLibName << endl;
		out << "LocalSearchPM::parse : objName : " << objName << endl;

		// The search algorithm
		Common::Util::DLLCallLoader<ScalarObjective*, QLibrary&, const char*> objLoader;
		SmartPointer<ScalarObjective> curObj;

		try {

		    //curObj = objLoader.load(objLib, QString("new_" + initSchedulerName).toStdString().data());
		    curObj.setPointer(objLoader.load(objLib, QString("new_" + objName).toStdString().data()));

		} catch (...) {

		    out << objLib.fileName() << endl;
		    throw ErrMsgException<>(std::string("LocalSearchPM::parse : Failed to resolve objective!"));

		}

		// Set the objective
		this->setObjective(curObj.getPointer());

	    }

	} else {
	    throw ErrMsgException<>(std::string("LocalSearchPM::parse : LS_PRIMARY_OBJECTIVE not specified!"));
	}

	// Initial scheduler (must not necessarily be defined )
	if (settings.container().contains("LS_INIT_SCHEDULER")) {

	    if (settings["LS_INIT_SCHEDULER"].changed()) {

		out << "LocalSearchPM::parse : LS_INIT_SCHEDULER changed!" << endl;

		QString initSchedStr = settings["LS_INIT_SCHEDULER"].get();

		QRegularExpression curInitSchedulerRE("([^\\(]+)\\({1,1}(.*)\\){1,1}@([^@^\\(^\\)]+)");
		QRegularExpressionMatch match = curInitSchedulerRE.match(initSchedStr);

		QString initSchedulerLibName = match.captured(3); // Library where the scheduler is to be looked for
		QString initSchedulerName = match.captured(1); // Scheduler name
		//QVector<QString> initSchedulerSettings = curInitSchedulerRE.cap(2).split(";").toVector();
		QString initSchedulerParams = match.captured(2);

		QLibrary initSchedulerLib(initSchedulerLibName);

		out << "LocalSearchPM::parse : initSchedulerLibName : " << initSchedulerLibName << endl;
		out << "LocalSearchPM::parse : initSchedulerName : " << initSchedulerName << endl;
		out << "LocalSearchPM::parse : params : " << initSchedulerParams << endl;

		// The search algorithm
		Common::Util::DLLCallLoader<SchedSolver*, QLibrary&, const char*> initSchedulerLoader;
		//SchedSolver* initScheduler = nullptr;
		//SmartPointer<SchedSolver> initScheduler;

		try {

		    //initScheduler = initSchedulerLoader.load(initSchedureLib, QString("new_" + initSchedulerName).toStdString().data());
		    initScheduler.setPointer(initSchedulerLoader.load(initSchedulerLib, QString("new_" + initSchedulerName).toStdString().data()), true);

		} catch (Common::Util::DLLLoadException<Common::Util::DLLResolveLoader<SchedSolver*, QLibrary&, const char*>>&) {

		    out << "LocalSearchPM::parse : Load exception! " << initSchedulerLib.fileName() << endl;
		    getchar();

		} catch (...) {

		    out << initSchedulerLib.fileName() << endl;
		    throw ErrMsgException<>(std::string("LocalSearchPM::parse : Failed to resolve InitScheduler algorithm!"));

		}

		// Let the initScheduler parse its settings
		if (initScheduler.valid()) {

		    SchedulerOptions lsInitSchedSettings;
		    lsInitSchedSettings["ALL_SETTINGS"] = initSchedulerParams;

		    initScheduler->parse(lsInitSchedSettings);

		}

	    }

	}

    }

    //  ########################################################################

    // Set all settings as unchanged
    settings.setChanged(false);

    out << "LocalSearchPM::parse : Parsed all settings." << endl;
    //getchar();

}

Schedule LocalSearchPM::solve(const SchedulingProblem& problem/*, const SchedulerOptions& options*/) {

    QTextStream out(stdout);

    out << "LocalSearchPM::solve : running..." << endl;

    // Defined here since we might run the initial solver
    ProcessModel curPM = (ProcessModel&) problem.pm;
    Resources curRC = (Resources&) problem.rc;
    Schedule sched;

    // Try to run the initial scheduler
    if (initScheduler.valid()) {

	sched = initScheduler->solve(problem);

	// Set initial PM
	curPM = sched.pm;

    }

    out << "LocalSearchPM::solve : Ready to start!" << endl;


    // Run the scheduler
    if (this->maxIter() > 0) {
	//pm->save();
	curPM.save();

	// Run the scheduler
	//		this->setObjective(problem.obj);
	this->setPM(&curPM);
	this->setResources(&curRC);

	this->run();

	//pm->restore();
	curPM.restore();
    }

    // Prepare the schedule
    sched.fromPM(curPM, *obj);

    out << "LocalSearchPM::solve : Finished!" << endl;

    return sched;

}

void LocalSearchPM::init() {
    QTextStream out(stdout);

    IterativeAlg::init();

    //out << "LocalSearchPM::init : Running in thread : " << this->thread() << endl;
    //getchar();

    if (pm == NULL) {
	Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with NULL process model!" << ENDL;
    }

    if (obj == NULL) {
	Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with a NULL objective!" << ENDL;
    }

    if (!intRNG.valid()) {
	Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with a NULL random numbers generator!" << ENDL;
    }

    if (!floatRNG.valid()) {
	Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with a NULL random numbers generator!" << ENDL;
    }

    // Preserve the state of the schedule
    pm->save();

    // Get the terminal nodes
    QList<ListDigraph::Node> terminals;
    terminals = pm->terminals();

    // Find the initial topological ordering of the nodes in the graph
    topolOrdering = pm->topolSort();
    topolITStart = 0;

    pm->updateHeads(topolOrdering);
    pm->updateStartTimes(topolOrdering);

    //TWT obj;

    curobj = (*obj)(*pm);
    prevobj = curobj;
    bestobj = curobj;

    //if (!debugCheckPMCorrectness("LocalSearchPM::init")) {
    //	Debugger::err << "PM is not correct while initializing!" << ENDL;
    //}

    out << "LS (init) : bestobj = " << curobj << endl;

    //out << "RandSeed in the PM: " << randGen->getSeeds()[0] << endl;
    //out << "RandSeed in the PM: " << randGen->getMaxGenInt() << endl;
    //out << "2^32-1: " << (unsigned int) ((((unsigned long int) (2)) << 31) - 1) << endl;

    alpha = 0.1;

    nisteps = 0;

    criticalNodes.clear();

    //out << "LocalSearchPM::init : Updating critical nodes... " << endl;

    //out << *pm << endl;

    updateCriticalNodes();

    //out << "LocalSearchPM::init : Done updating critical nodes. " << endl;

    nodeI = INVALID;
    nodeT = INVALID;

    //########################  DEBUG  #########################################
    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency during the initialization..." << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */

    topSortElapsedMS = 0;
    objElapsedMS = 0;
    updateEvalElapsedMS = 0;
    totalElapsedMS = 0;
    opSelectionElapsedMS = 0;
    potentialPositionsSelectionElapsedMS = 0;
    posSelectionElapsedMS = 0;
    opMoveElapsedMS = 0;
    opMoveBackElapsedMS = 0;
    blocksExecElapsedMS = 0;
    opMovePossibleElapsedMS = 0;
    dynTopSortElapsedMS = 0;
    updateCritNodesElapsedMS = 0;
    longestPathsElapsedMS = 0;

    totalChecksElapsedMS = 0;

    totalTimer.start();
    //##########################################################################
}

void LocalSearchPM::stepActions() {
    //if (iter() % 1 == 0) out << "iter : " << iter() << endl;

    //	blocksExecTimer.start();

    //if (Rand::rndDouble() < 1.99999) {
    transitionPM(); // 1-2 cores makes significant difference. Probably because of the random number generator
    //} else {
    //	transitionCP();
    //}

    //	blocksExecElapsedMS += blocksExecTimer.elapsed();

}

void LocalSearchPM::assessActions() {
    //	blocksExecTimer.start();
    //out << "Assessing the step of the LS..." << endl;

    // Update the ready times and the start times of the operations in the graph
    //pm->updateHeads();
    //pm->updateStartTimes();

    updateEval(nodeI, nodeT);

    //	objTimer.start();
    curobj = (*obj)(*pm);
    //	objElapsedMS += objTimer.elapsed();

    //    if (_check_correctness && !debugCheckPMCorrectness("LocalSearchPM::assessActions")) {
    //        Debugger::err << "PM is not correct after updating!" << ENDL;
    //    }

    //out << "Done assessing the step of the LS." << endl;
    //	blocksExecElapsedMS += blocksExecTimer.elapsed();
}

bool LocalSearchPM::acceptCondition() {
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

    alpha = Math::exp(-(curobj - prevobj) / (1.0 - Math::pow((double) iter() / double(maxIter()), 1.0))); ///*0.2; //*/0.5 * Math::exp(-((double) iter())); // 1.0 - 0.999 * Math::exp(1.0 - ((double) iter()) / double(_maxiter));
    //alpha = 0.001;

    if (curobj <= prevobj /*bestobj*/) {
	acceptedworse = false;
	return true;
    } else {
	if (floatRNG->rnd(0.0, 1.0) < alpha) {
	    acceptedworse = true;
	    return true;
	} else {
	    acceptedworse = false;
	    return false;
	}
    }

}

void LocalSearchPM::acceptActions() {
#ifdef LS_MSG
    QTextStream out(stdout);
#endif 
    //	blocksExecTimer.start();
    //out << "accepted." << endl;


    if (acceptedworse || curobj == bestobj) {
	nisteps++;
    } else {
	if (curobj < bestobj) {
	    nisteps = 0;
	} else {
	    nisteps++;
	}
    }

    if (curobj <= bestobj) {
#ifdef LS_MSG
	if (curobj < bestobj) out << "LSPM (" << iter() << ") : bestobj = " << curobj << endl;
#endif
	bestobj = curobj;

	// Preserve the state of the process model
	pm->save();

	bestobj = curobj;

	//if (curobj < bestobj) updateCriticalNodes(); // Notice : updating critical nodes every time a better solution has been found is REALLY EXPENSIVE.


    }

    prevobj = curobj;


    //out << "The step has been accepted." << endl;
    //	blocksExecElapsedMS += blocksExecTimer.elapsed();
}

void LocalSearchPM::declineActions() {
    //	blocksExecTimer.start();
    //out << "Declining the iteration..." << endl;

    moveBackOper(optomove);
    optomove = INVALID;

    // Restore the previous ready times and start times of the operations
    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Graph after moving BACK the operation " << *pm << endl;
    //out << "PM after restoring : " << *pm << endl;


    //out << "Done declining." << endl;


    // Preform diversification

    //if (iter() > 250000) {
    //	alpha = Math::min(alpha + 0.000001, 0.1);
    //out << "Alpha = " << alpha << endl;
    //}

    nisteps++;


    if (nisteps > 2000) {
	//out << "Diversifying (nisteps)..." << endl;
	diversify();
    }

    //	blocksExecElapsedMS += blocksExecTimer.elapsed();
}

bool LocalSearchPM::stopCondition() {
    return (curobj <= (*obj).LB(*pm)) || IterativeAlg::stopCondition();
}

void LocalSearchPM::stopActions() {
    Debugger::info << "LocalSearchPM::stopActions : Found local optimum with objective " << bestobj << ENDL;
    //getchar();
}

void LocalSearchPM::preprocessingActions() {
    // Check correctness of the PM right before the processing
    //out << "LocalSearchPM::preprocessingActions : Checking PM correctness..." << endl;
    if (_check_correctness) {
	debugCheckPMCorrectness("LocalSearchPM::preprocessingActions");
    }
    //out << "LocalSearchPM::preprocessingActions : PM is correct." << endl;
}

void LocalSearchPM::postprocessingActions() {
    QTextStream out(stdout);
    // Restore the state corresponding to the best found value of the objective
    pm->restore();


    //out << "PM : " << endl << *pm << endl;

    // Check the correctness of the process model
    if (_check_correctness) {
	debugCheckPMCorrectness("LocalSearchPM::postprocessingActions");
    }


    out << "                  " << endl;


    totalElapsedMS = totalTimer.elapsed();

    /*
    out << "Time (ms) for objective estimations : " << objElapsedMS << endl;
    out << "Time (ms) for topological sorting : " << topSortElapsedMS << endl;
    out << "Time (ms) for running the update evaluations : " << updateEvalElapsedMS << endl;
    out << "Time (ms) for selecting operations : " << opSelectionElapsedMS << endl;
    out << "Time (ms) for finding potential moves : " << potentialPositionsSelectionElapsedMS << endl;
    out << "Time (ms) for selecting the insert position : " << posSelectionElapsedMS << endl;
    out << "Time (ms) for moving operation : " << opMoveElapsedMS << endl;
    out << "Time (ms) for moving back operation : " << opMoveBackElapsedMS << endl;
    out << "Time (ms) for estimating feasibility of the moves : " << opMovePossibleElapsedMS << endl;
    out << "Time (ms) for DTO : " << dynTopSortElapsedMS << endl;
    out << "Time (ms) for updating critical nodes : " << updateCritNodesElapsedMS << endl;
    out << "Time (ms) for calculating the longest paths : " << longestPathsElapsedMS << endl;

    out << " -----------------" << endl;
    out << "Time (ms) for executing blocks (1) : " << objElapsedMS +
		    updateEvalElapsedMS + opSelectionElapsedMS + potentialPositionsSelectionElapsedMS + posSelectionElapsedMS +
		    opMoveElapsedMS + opMoveBackElapsedMS << endl;
    out << "Time (ms) for executing blocks (2) : " << blocksExecElapsedMS << endl;
     */
    out << " -----------------" << endl;
    out << "Time (ms) for running the algorithm : " << totalElapsedMS << endl;
    out << " -----------------" << endl;

    out << "Time percentage for correctness checks : " << double(totalChecksElapsedMS) / double(totalElapsedMS) << endl;
    out << " -----------------" << endl;

}

void LocalSearchPM::transitionPM() {
    //QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Get the terminal nodes of the graph.
     * 2. Select the most critical in some sense terminal node.
     * 3. Find a critical path to this terminal.
     * 4. Select an operation from the critical path
     * 5. Select a machine which the operation has to be moved to.
     * 6. Try to move the operation to the selected machine
     */

    // Get the terminals
    //QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath; // Critical path

    //pm->updateHeads();
    //pm->updateStartTimes();

    /*** DEBUG PURPOUSES ONLY ***/
    // Iterate over all terminals an estimate the number of schedule-based arcs in the longest paths

    /*
    int sbarcs = 0;
    for (int i = 0; i < terminals.size(); i++) {
	    cpath = longestPath(terminals[i]);
	    for (int n = 0; n < cpath.length(); n++) {
		    if (!pm->conjunctive[cpath.nth(n)]) {
			    sbarcs++;
		    }
	    }
    }

    // Debugger::info << "Number of sbarcs on crit. paths : " << sbarcs << ENDL;

    //sbarcs = 0;
    //for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
    //	if (!pm->conjunctive[ait]) sbarcs++;
    //   }
    //Debugger::info << "Number of sbarcs totally : " << sbarcs << ENDL;

    // Check multiple schedule-based arcs

    int nmultiplesb = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

	    nmultiplesb = 0;
	    for (ListDigraph::InArcIt iait(pm->graph, nit); iait != INVALID; ++iait) {
		    if (!pm->conjunctive[iait]) nmultiplesb++;
	    }
	    if (nmultiplesb > 1) {
		    out << *pm << endl;
		    Debugger::err << "Too many incoming sb arcs!!! " << pm->ops[nit]->ID << ENDL;
	    }

	    nmultiplesb = 0;
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (!pm->conjunctive[oait]) nmultiplesb++;
	    }
	    if (nmultiplesb > 1) {
		    out << *pm << endl;
		    Debugger::err << "Too many outgoing sb arcs!!! " << pm->ops[nit]->ID << ENDL;
	    }
    }

     */

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency before the step..." << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */
    /******/

    //	opSelectionTimer.start();

    // Update the set of critical nodes
    //out << "Updating critical nodes..." << endl;
    //int critNodesUpdateFreq = 100;
    if (iter() % critNodesUpdateFreq == 0) {
	pm->updateHeads(topolOrdering);
	pm->updateStartTimes(topolOrdering);
	updateCriticalNodes();
	if (criticalNodes.size() == 0) {
	    pm->updateHeads();
	    pm->updateStartTimes();

	    QTextStream out(stdout);
	    out << *pm << endl;
	    out << "TWT of the partial schedule : " << TWT()(*pm) << endl;
	    Debugger::err << "LocalSearchPM::transitionPM : Failed to find critical nodes!!!" << endl;
	}
    }
    //out << "Updating critical nodes..." << endl;
    optomove = criticalNodes[intRNG->rnd(0, criticalNodes.size() - 1)];

    // Select a terminal
    //if (nisteps <= 100) {
    //theterminal = selectTerminalContrib(terminals);
    //} else {
    //    theterminal = selectTerminalContrib(terminals);
    //}

    // Find a critical path to the selected terminal

    //    cpath = longestPath(theterminal);

    // Select an operation to move
    //Debugger::iDebug("Selecting operation to move...");

    //    optomove = defaultSelectOperToMove(cpath);

    //Debugger::iDebug("Selected operation to move.");

    //	opSelectionElapsedMS += opSelectionTimer.elapsed();

    //QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs;

    //QPair<ListDigraph::Node, ListDigraph::Node> atb;

    //out << "Searching arc to break..." << endl;
    /*
    do {
	    // Select a random terminal
	    theterminal = selectTerminalNonContrib(terminals);

	    // Select a random path to the terminal
	    ncpath = randomPath(theterminal);

	    // Select relevant pairs of nodes
	    //out << "Selecting relevant arcs..." << endl;
	    relarcs = selectRelevantArcsFromPath(ncpath, optomove);
	    //out << "Selected relevant arcs. " << relarcs.size() << endl;
	    if (relarcs.size() > 0) {
		    // Select insert positions
		    //out << "Selecting arc to break..." << endl;
		    atb = selectArcToBreak(relarcs, optomove);
		    //out << "Selected arc to break." << endl;
	    }

    } while (relarcs.size() == 0 || (atb.first == INVALID && atb.second == INVALID));
     */
    //out << "Found arc to break." << endl;

    /*
     if (atb.first != INVALID) {
	     out << pm->ops[atb.first]->ID << " -> ";
     } else {
	     out << "INV. ->";
     }

     if (atb.second != INVALID) {
	     out << pm->ops[atb.second]->ID << endl;
     }
     */

    int targetMachID = selectTargetMach(optomove);

    // Select candidate arcs to break
    //	potentialPositionsSelectionTimer.start();
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs = selectBreakableArcs(targetMachID);
    //	potentialPositionsSelectionElapsedMS += potentialPositionsSelectionTimer.elapsed();

    //out << endl << endl;

    //    do {
    // Select an arc to break
    //out << "Selecting arc to break... " << endl;
    //	posSelectionTimer.start();
    //	if (bestPosToMove){
    //		Debugger::info << "Selecting best pos" << ENDL;
    //		getchar();
    //	}else{
    //		Debugger::info << "Not selecting best pos" << ENDL;
    //		getchar();
    //	}
    QPair<ListDigraph::Node, ListDigraph::Node> atb = ((bestPosToMove) ? selectBestArcToBreak(targetMachID, relarcs, optomove) : selectArcToBreak(relarcs, optomove));

    //	posSelectionElapsedMS += posSelectionTimer.elapsed();

    //out << "Selected arc to break." << endl;

    //out << "Selected operation : " << pm->ops[optomove]->ID << endl;
    //out << "Arc to break : (" << pm->ops[pm->graph.source(arc)]->ID << "," << pm->ops[pm->graph.target(arc)]->ID << ")" << endl;
    //out << *pm << endl;



    // Reverse the selected arc
    //out << "Graph before reversing an arc:" << endl;
    //out << *pm << endl;
    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Reversing arc ..." << endl;
    //out << pm->ops[pm->graph.source(carc)]->OID << ":" << pm->ops[pm->graph.source(carc)]->ID << " -> " << pm->ops[pm->graph.target(carc)]->OID << ":" << pm->ops[pm->graph.target(carc)]->ID << endl;


    //	    if (!dag(pm->graph)) {
    //		Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains cycles before moving an operation!!!");
    //	    } else {
    //		//Debugger::info<<"The resulting graph is DAG before reverting a critical arc!!!"<<ENDL;
    //	    }

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

    // Find the best possible move of the selected operation
    //out << "Searching for the best move option..." << endl;
    //findBestOperMove(optomove, targetMachID, atb);
    //out << "Found the best move option." << endl;

    //if (!dag(pm->graph)) {
    //Debugger::err << "Graph contains cycles before the operation move!!!" << ENDL;
    //}

    //out << "Critical arc: (" << pm->ops[pm->graph.source(carc)]->ID << " ; " << pm->ops[pm->graph.target(carc)]->ID << ")" << endl;
    //    out << " Moving operation with ID = " << pm->ops[optomove]->ID << endl;
    //getchar();
    //    out << " Between operations : " << ((atb.first != INVALID) ? pm->ops[atb.first]->ID : -1) << " and " << ((atb.second != INVALID) ? pm->ops[atb.second]->ID : -1) << endl;
    //out << "Graph before moving an operation: " << endl;
    //out << *pm << endl;
    //getchar();

    // Move the operation breaking the selected arc

    /*
    out << "Check reachability for every machine before moving the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    // ##### IMPORTANT !!! ##### Preserve the states of the operation BEFORE the move

    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Graph before moving the operation ..." << *pm << endl;
    //out << "Moving operation..." << endl;
    moveOper(targetMachID, atb.first, atb.second, optomove);
    //out << "Moved operation " << pm->ops[optomove]->ID << endl;

    /*
    out << "Check reachability for every machine after moving the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    //if (!dag(pm->graph)) moveBackOper(optomove);
    //	else {
    //		out << "Graph after the move:" << endl;
    //		out << *pm << endl;
    //	    break;
    //	}

    /*
    if (!dag(pm->graph)) {
	    out << *pm << endl;
	    Debugger::err << "Graph contains cycles after the operation move!!!" << ENDL;
    }
     */

    //out << " Moved operation with ID = " << pm->ops[optomove]->ID << endl;
    //out << *pm << endl;
    //getchar();
    //out << "Done moving operation." << endl;

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency after the step..." << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */

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
    //	    if (!dag(pm->graph)) {
    //		out << " Moved operation with ID = " << pm->ops[optomove]->ID << endl;
    //		out << " Between operations : " << ((atb.first != INVALID) ? pm->ops[atb.first]->ID : -1) << " and " << ((atb.second != INVALID) ? pm->ops[atb.second]->ID : -1) << endl;
    //		out << *pm;
    //		Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains cycles after moving the operation!!!");
    //	    } else {
    //		// Debugger::info << "The resulting graph is acyclic :)." << ENDL;
    //	    }

    //out << "Done reversing arc." << endl;

    //out << "Graph after reversing an arc:" << endl;
    //out << *pm << endl;

    //	} else {
    //	    Debugger::err << "LocalSearch::stepActions : Invalid arc to break!" << ENDL;


    //    } while (true);
    //out << "Finished one step of the local search." << endl;

}

void LocalSearchPM::selectOperToMoveCP(const Path<ListDigraph> &cpath, ListDigraph::Node &optomove, QPair<ListDigraph::Node, ListDigraph::Node> &atb) {
    // Find critical arcs on the critical path
    QList<ListDigraph::Arc> carcs;
    ListDigraph::Arc curarc;


    for (int i = 0; i < cpath.length(); i++) {
	curarc = cpath.nth(i);
	if (!pm->conjunctive[curarc] && !pm->conPathExists(pm->graph.source(curarc), pm->graph.target(curarc))) {
	    carcs.append(curarc);
	}
    }

    if (carcs.size() == 0) { // There are no reversible arcs on the path

	//optomove = INVALID;
	//return;

	curarc = cpath.nth(intRNG->rnd(0, cpath.length() - 1));
	optomove = pm->graph.source(curarc);
	atb.first = pm->graph.source(curarc);
	atb.second = pm->graph.target(curarc);

	return;
    }

    // Select randomly some critical arc
    curarc = carcs[intRNG->rnd(0, carcs.size() - 1)];

    optomove = pm->graph.source(curarc);

    atb.first = pm->graph.target(curarc);

    ListDigraph::Arc arc = INVALID;
    //bool sbarcfound = false;
    bool outarcexists = false;
    // Search schedule-based outgoing arcs
    for (ListDigraph::OutArcIt oait(pm->graph, atb.first); oait != INVALID; ++oait) {
	if (!pm->conjunctive[oait]) {
	    outarcexists = true;

	    arc = oait;

	    if (!pm->conPathExists(pm->graph.source(oait), pm->graph.target(oait))) {
		//sbarcfound = true;
		break;
	    }
	}
    }

    if (!outarcexists) {
	atb.second = INVALID;
	return;
    } else {
	atb.second = pm->graph.target(arc);
    }

}

void LocalSearchPM::transitionCP() {
    /** Algorithm:
     * 
     * 1. Get the terminal nodes of the graph.
     * 2. Select the most critical in some sense terminal node.
     * 3. Find a critical path to this terminal.
     * 4. Select an operation from the critical path
     * 5. Select a machine which the operation has to be moved to.
     * 6. Try to move the operation to the selected machine
     */

    // Get the terminals
    //QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath; // Critical path
    QPair<ListDigraph::Node, ListDigraph::Node> atb;

    //	opSelectionTimer.start();

    QList<ListDigraph::Node> terminals = pm->terminals();

    do {

	// Select a terminal
	theterminal = selectTerminalContrib(terminals);

	// Find a critical path to the selected terminal
	cpath = longestPath(theterminal);

	//Debugger::iDebug("Selecting operation to move...");
	selectOperToMoveCP(cpath, optomove, atb);
	//Debugger::iDebug("Selected operation to move.");


    } while (optomove == INVALID);

    //Debugger::iDebug("Selected operation to move.");
    //	opSelectionElapsedMS += opSelectionTimer.elapsed();


    //QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs;

    //QPair<ListDigraph::Node, ListDigraph::Node> atb;

    int targetMachID = pm->ops[optomove]->machID;

    //out << "Graph before moving the operation ..." << *pm << endl;
    //out << "Moving operation..." << endl;

    moveOper(targetMachID, atb.first, atb.second, optomove);
    //out << "Moved operation " << pm->ops[optomove]->ID << endl;

    /*
    if (!dag(pm->graph)) {
	    Debugger::err << "LocalSearchPM::transitionCP : Graph not DAG!!!" << ENDL;
    }
     */

}

Path<ListDigraph> LocalSearchPM::longestPath(const ListDigraph::Node & node) {
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

QList<Path<ListDigraph> > LocalSearchPM::longestPaths(const QList<ListDigraph::Node> &nodes) {
    //	longestPathsTimer.start();

    QList<Path<ListDigraph> > res;

    BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm->graph, pm->p);

    bf.init();
    bf.addSource(pm->head);
    //Debugger::info << "Running the BF algorithm..."<<ENDL;
    bf.start();
    //Debugger::info << "Done running the BF algorithm."<<ENDL;

#ifdef DEBUG
    for (int i = 0; i < nodes.size(); i++) {
	if (!bf.reached(nodes[i])) {
	    Debugger::err << "LocalSearch::longestPath : Operation ID= " << pm->ops[nodes[i]]->OID << ":" << pm->ops[nodes[i]]->ID << " can not be reached from the root node " << pm->ops[pm->head]->OID << ":" << pm->ops[pm->head]->ID << "!" << ENDL;
	}
    }
#endif
    for (int i = 0; i < nodes.size(); i++) {
	res.append(bf.path(nodes[i]));
    }

    //	longestPathsElapsedMS += longestPathsTimer.elapsed();

    return res;
}

Path<ListDigraph> LocalSearchPM::randomPath(const ListDigraph::Node & node) {
    Path<ListDigraph> res;
    ListDigraph::Node curnode = node;
    QList<ListDigraph::InArcIt> inarcs;

    while (curnode != pm->head) {
	// Select the outgoing arcs from the current node
	inarcs.clear();
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    inarcs.append(iait);
	}

	// Add a random arc to the result
	res.addFront(inarcs.at(intRNG->rnd(0, inarcs.size() - 1)));

	// Proceed to the next node
	curnode = pm->graph.source(res.front());
    }

    return res;
}

void LocalSearchPM::updateCriticalNodes() {
    /** Algorithm:
     * 
     * 1. Select the terminal nodes contributing to the optimization criterion
     * 2. Calculate the longest paths to these terminals
     * 3. Select all operations lying on the critical paths
     * 
     */

    QTextStream out(stdout);

    QList<ListDigraph::Node> terminals;

    terminals = pm->terminals();

    //	updateCritNodesTimer.start();

    ListDigraph::Arc ait = INVALID;
    QList<Path<ListDigraph> > cpaths; // Critical paths
    Path<ListDigraph> cpath; // Critical path
    ListDigraph::Node curnode = INVALID;

    criticalNodes.clear();

    //out << "LocalSearchPM::updateCriticalNodes : Searching the longest paths..." << endl;

    //if (!dag(pm->graph)){
    //	out << *pm << endl;
    //	Debugger::err << "LocalSearchPM::updateCriticalNodes : The PM is not DAG!!!" << ENDL;
    //}

    // Calculate the longest paths to the terminal nodes only once -
    cpaths = longestPaths(terminals);

    //out << "LocalSearchPM::updateCriticalNodes : Searching the longest paths DONE!" << endl;

    //criticalNodes.reserve();
    int termContrib = 0;
    for (int i = 0; i < terminals.size(); i++) {

	double currentContribution = 0.0;

	if (obj->name() == "TWT") {
	    currentContribution = pm->ops[terminals[i]]->wT();
	} else if (obj->name() == "Cmax") {
	    currentContribution = pm->ops[terminals[i]]->c();
	} else {
	    Debugger::err << "LocalSearchPM::updateCriticalNodes : Unknown objective!!!" << ENDL;
	}

	if (currentContribution > 0.0) { // The terminal is contributing

	    termContrib++;

	    // Find the critical path to the terminal
	    cpath = cpaths[i];

	    for (int j = 0; j < cpath.length(); j++) {
		ait = cpath.nth(j);

		curnode = pm->graph.source(ait);
		if (/*criticalNodes.count(curnode) == 0 &&*/ pm->ops[curnode]->machID > 0 /*&& (pm->ops[pm->graph.source(ait)]->d() - pm->ops[pm->graph.source(ait)]->c() < 0.0)*/) {
		    if (node2Movable[ListDigraph::id(curnode)]) criticalNodes.append(curnode);
		}

	    }

	    // Add the target node (here ait represents the last arc of the current critical path)
	    curnode = pm->graph.target(ait);
	    if (/*criticalNodes.count(curnode) == 0 &&*/ pm->ops[curnode]->machID > 0 /*&& (pm->ops[pm->graph.target(ait)]->d() - pm->ops[pm->graph.target(ait)]->c() < 0.0)*/) {
		if (node2Movable[ListDigraph::id(curnode)]) criticalNodes.append(curnode);
	    }
	}
    }

    if (criticalNodes.size() == 0) { // No movable critical nodes have been found -> set all movable nodes as critical (the algorithm will try to move them)

	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

	    ListDigraph::Node curNode = nit;

	    if (node2Movable[ListDigraph::id(curNode)]) criticalNodes.append(curNode);

	}

    }

    if (criticalNodes.size() == 0/*termContrib == 0*/) { // This should not happen since the algorithm must catch this situation

	pm->updateHeads();
	pm->updateStartTimes();
	QTextStream out(stdout);
	out << "LocalSearchPM::updateCriticalNodes : Current iteration : " << iter() << endl;
	out << "Current TWT of the partial schedule : " << TWT()(*pm) << endl;
	Debugger::err << "LocalSearchPM::updateCriticalNodes : No contributing terminals!!!" << ENDL;

    }

    //	updateCritNodesElapsedMS += updateCritNodesTimer.elapsed();

}

ListDigraph::Node LocalSearchPM::selectOperToMove(const Path<ListDigraph> &cpath) {
    // Select only those arcs which are schedule based and NOT conjunctive

    int n = cpath.length();

    QList<ListDigraph::Node> nodes;
    ListDigraph::Node res;

    QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs
    for (int i = 0; i < n; i++) {
	if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based (and therefore the operation is already assigned)

	    schedbased.append(cpath.nth(i));
	    ListDigraph::Arc curArc = cpath.nth(i);

	    ListDigraph::Node curStartNode = pm->graph.source(curArc);
	    ListDigraph::Node curEndNode = pm->graph.target(curArc);

	    if (nodes.count(curStartNode) == 0 && pm->ops[curStartNode]->machID > 0 && node2Movable[ListDigraph::id(curStartNode)]) {
		nodes.append(curStartNode);
	    }

	    if (nodes.count(curEndNode) == 0 && pm->ops[curEndNode]->machID > 0 && node2Movable[ListDigraph::id(curEndNode)]) {
		nodes.append(curEndNode);
	    }

	} else {
	    /*
	    if (pm->ops[pm->graph.source(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.source(cpath.nth(i))) == 0) {
		    nodes.append(pm->graph.source(cpath.nth(i)));
	    }

	    if (pm->ops[pm->graph.target(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.target(cpath.nth(i))) == 0) {
		    nodes.append(pm->graph.target(cpath.nth(i)));
	    }
	     */
	}

    }

    if (schedbased.size() == 0) {
	return INVALID;
	/*
	for (int i = 0; i < n; i++) {
		if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based (and therefore the operation is already assigned)
		} else {
			if (pm->ops[pm->graph.source(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.source(cpath.nth(i))) == 0) {
				nodes.append(pm->graph.source(cpath.nth(i)));
			}

			if (pm->ops[pm->graph.target(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.target(cpath.nth(i))) == 0) {
				nodes.append(pm->graph.target(cpath.nth(i)));
			}
		}

	}
	 */

    }

    //do {
    res = nodes[intRNG->rnd(0, nodes.size() - 1)];
    //} while (!scheduledtgs.contains(pm->ops[res]->toolID));

    return res;
}

ListDigraph::Node LocalSearchPM::defaultSelectOperToMove(const Path<ListDigraph> &cpath) {
    int n = cpath.length();

    QList<ListDigraph::Node> nodes;
    ListDigraph::Node res;

    QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs

    //for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
    ListDigraph::Arc ait;
    for (int i = 0; i < n; i++) {
	ait = cpath.nth(i);

	ListDigraph::Node curStartNode = pm->graph.source(ait);
	ListDigraph::Node curEndNode = pm->graph.target(ait);

	if (nodes.count(curStartNode) == 0 && pm->ops[curStartNode]->machID > 0 && node2Movable[ListDigraph::id(curStartNode)]) {
	    nodes.append(curStartNode);
	}
	if (nodes.count(curEndNode) == 0 && pm->ops[curEndNode]->machID > 0 && node2Movable[ListDigraph::id(curEndNode)]) {
	    nodes.append(curEndNode);
	}

    }

    if (nodes.size() == 0) return INVALID;

    /*
    // Select the operation which has a high tardiness
    QMultiMap<double, ListDigraph::Node> tdns2node;

    for (int i = 0; i < nodes.size(); i++) {
	    tdns2node.insert(pm->ops[nodes[i]]->wT(), nodes[i]);
    }
    int k = Rand::rndInt(1, tdns2node.size() / 2);
    QMultiMap<double, ListDigraph::Node>::iterator iter = tdns2node.end();
    for (int j = 0; j < k; j++) {
	    iter--;
    }
    return iter.value();
     */

    //do {
    res = nodes[intRNG->rnd(0, nodes.size() - 1)];
    //} while (!scheduledtgs.contains(pm->ops[res]->toolID));

    return res;
}

int LocalSearchPM::selectTargetMach(const ListDigraph::Node& optomove) {
    // Get the list of available in the TG machines which are able to execute the operation type
    QList<Machine*> tgmachines = ((*rc)(pm->ops[optomove]->toolID)).machines(pm->ops[optomove]->type);

    /*
    
    // For every machine calculate the total processing time of operations assigned to it
    QHash<int, double> machid2crit;
    Operation* curop;

    // Insert all machines of the tool group
    for (int i = 0; i < tgmachines.size(); i++) {
	    machid2crit[tgmachines[i]->ID] = 0.0;
    }

    // Calculate the CTs
    for (int i = 0; i < topolOrdering.size(); i++) {
	    curop = pm->ops[topolOrdering[i]];
	    if (curop->toolID == pm->ops[optomove]->toolID && curop->machID > 0) {
		    machid2crit[curop->machID] += curop->p(); // = Math::max(machid2crit[curop->machID], curop->w() * curop->c());
	    }
    }

    // Find the machine with the smallest WIP
    double curWIP = Math::MAX_DOUBLE;
    QList<int> machIDs;
    int machID = -1;
    for (QHash<int, double>::iterator iter = machid2crit.begin(); iter != machid2crit.end(); iter++) {
	    if (iter.value() <= curWIP) {
		    machIDs.prepend(iter.key());
		    curWIP = iter.value();
	    }
    }

    while (machIDs.size() > 3) {
	    machIDs.removeLast();
    }

    machID = machIDs[Rand::rndInt(0, machIDs.size() - 1)];

    if (machID == -1) {
	    Debugger::err << "LocalSearchPM::selectTargetMach : Failed to find the target machine!" << ENDL;
    }
     */
    // Return an arbitrary machine from the tool group
    //if (iter() % 1 == 0) {
    //return tgmachines[Rand::rndInt(0, tgmachines.size() - 1)]->ID;
    //}
    return /*machID; //pm->ops[optomove]->machID; //*/ tgmachines[intRNG->rnd(0, tgmachines.size() - 1)]->ID;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPM::selectRelevantArcs(const Path<ListDigraph>& /*cpath*/, const ListDigraph::Node& node) {
    /** Algorithm:
     * 
     * 1. Iterate over all arcs in the graph
     * 1.1. If the source node of the current arc can be processed by the same tool group
     *	    then add the arc to the list
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    ListDigraph::Node j;
    ListDigraph::Node k;
    bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine

    for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
	//for (int i = 0; i < cpath.length(); i++) {
	//ListDigraph::Arc ait = cpath.nth(i);

	if (!pm->conjunctive[ait]) {

	    if (pm->ops[node]->toolID == pm->ops[pm->graph.source(ait)]->toolID) { // This arc is relevant
		j = pm->graph.source(ait);
		k = pm->graph.target(ait);
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, k));

		// Check whether the arc corresponds to the first two or the last two operations on the machine
		fol = true;
		for (ListDigraph::InArcIt iait(pm->graph, j); iait != INVALID; ++iait) {
		    if (!pm->conjunctive[iait]) {
			fol = false;
			break;
		    }
		}

		if (fol) {
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, j));
		}

		fol = true;
		for (ListDigraph::OutArcIt oait(pm->graph, k); oait != INVALID; ++oait) {
		    if (!pm->conjunctive[oait]) {
			fol = false;
			break;
		    }
		}

		if (fol) {
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (k, INVALID));
		}

	    }
	}
    }

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPM::selectRelevantArcsNew(const Path<ListDigraph>& /*cpath*/, const ListDigraph::Node& node) {
    //QTextStream out(stdout);
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    ListDigraph::Node j;
    ListDigraph::Node k;
    //bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine

    QHash<int, QVector<ListDigraph::Node> > machid2node;
    QQueue<ListDigraph::Node> q;
    ListDigraph::Node curnode;
    ListDigraph::NodeMap<bool> scheduled(pm->graph, false);
    ListDigraph::NodeMap<bool> available(pm->graph, false);
    //bool predchecked = false;

    ListDigraph::Node suc;
    ListDigraph::Node sucpred;

    q.enqueue(pm->head);
    scheduled[pm->head] = false;
    available[pm->head] = true;

    // Collect operation sequences on every machine of the tool group
    while (q.size() > 0) {
	curnode = q.dequeue();

	if (available[curnode] && !scheduled[curnode]) {
	    if ((pm->ops[curnode]->ID > 0) && (pm->ops[curnode]->toolID == pm->ops[node]->toolID)) {
		machid2node[pm->ops[curnode]->machID].append(curnode);
	    }

	    scheduled[curnode] = true;

	    // Enqueue the successors
	    for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
		suc = pm->graph.target(oait);
		if (!scheduled[suc]) {

		    // Update availability

		    available[suc] = true;
		    for (ListDigraph::InArcIt iait(pm->graph, suc); iait != INVALID; ++iait) {
			sucpred = pm->graph.source(iait);
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

    for (QHash<int, QVector<ListDigraph::Node> >::iterator iter = machid2node.begin(); iter != machid2node.end(); iter++) {

	//	out << "operations on machines : " << endl;
	//	out << "Mach ID : " << iter.key() << " : ";

	for (int i = 0; i < iter.value().size(); i++) {
	    //	    out << pm->ops[iter.value()[i]]->ID << ",";
	}

	//	out << endl << endl;
	//getchar();

	for (int i = 0; i < iter.value().size() - 1; i++) {
	    //for (ListDigraph::OutArcIt oait(pm->graph, iter.value()[i]); oait != INVALID; ++oait) {
	    //if (pm->graph.target(oait) == iter.value()[i + 1] /*&& !pm->conjunctive[oait]*/) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().at(i), iter.value().at(i + 1)));
	    //}
	    //}
	}


	if (iter.value().size() > 0) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, iter.value().first()));
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().last(), INVALID));
	}

    }

    //out << "GBM:" << endl;
    //out << *pm << endl;

    for (int i = 0; i < res.size(); i++) {
	//	out << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	if (!reachable(res[i].first, res[i].second)) {
	    QTextStream out(stdout);
	    out << "Not reachable : " << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	    //out << *pm << endl;
	    getchar();
	}
    }

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPM::selectBreakableArcs(const int& mid) {
    /*
    out << "Selecting breakable arcs..." << endl;
    out << "MID : " << mid << endl;
    out << "Scheduled TGs : " << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    out << scheduledtgs[i] << ",";
    }
    out << endl;
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    QList<ListDigraph::Node> trgmachnodes; // Nodes currently on the target machine
    ListDigraph::Node curnode;
    int n = topolOrdering.size();
    QVector<ListDigraph::Node> tord;
    /*
    QList<ListDigraph::Node> testtrgmachnodes; // Nodes currently on the target machine
    QQueue<ListDigraph::Node> q;
    ListDigraph::NodeMap<bool> scheduled(pm->graph, false);
    ListDigraph::NodeMap<bool> available(pm->graph, false);

    ListDigraph::Node suc;
    ListDigraph::Node sucpred;

    q.enqueue(pm->head);
    scheduled[pm->head] = false;
    available[pm->head] = true;
     */

    // Collect operation sequences on the target machine
    /*
    while (q.size() > 0) {
	    curnode = q.dequeue();

	    if (available[curnode] && !scheduled[curnode]) {
		    if ((pm->ops[curnode]->ID > 0) && (pm->ops[curnode]->machID == mid)) {
			    trgmachnodes.append(curnode);
		    }

		    scheduled[curnode] = true;

		    // Enqueue the successors
		    for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
			    suc = pm->graph.target(oait);
			    if (!scheduled[suc]) {

				    // Update availability

				    available[suc] = true;
				    for (ListDigraph::InArcIt iait(pm->graph, suc); iait != INVALID; ++iait) {
					    sucpred = pm->graph.source(iait);
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
     */

    tord = topolOrdering.toVector();

    trgmachnodes.clear();
    trgmachnodes.reserve(topolOrdering.size());

    for (int i = 0; i < n; i++) {
	curnode = tord[i]; //topolOrdering[i];
	if (pm->ops[curnode]->machID == mid) {
	    trgmachnodes.append(curnode);
	}
    }

    //#######################  DEBUG  ##########################################
    /*
    if (testtrgmachnodes.size() != trgmachnodes.size()) {
	    Debugger::err << "Wrong nodes on the target machine!!!" << ENDL;
    }

    for (int i = 0; i < testtrgmachnodes.size(); i++) {
	    if (!trgmachnodes.contains(testtrgmachnodes[i])) {
		    Debugger::err << "Wrong nodes on the target machine (elements test)!!!" << ENDL;
	    }
    }
     */
    //##########################################################################

    res.clear();
    res.reserve(trgmachnodes.size() + 2);

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
	    out << pm->ops[trgmachnodes[k]]->ID << ",";
    }

    out << endl << endl;
     */
    //out << "GBM:" << endl;
    //out << *pm << endl;

    // Check reachability

    /*
    for (int j = 0; j < res.size(); j++) {
	    //	out << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	    if (!reachable(res[j].first, res[j].second)) {
		    out << "Not reachable : " << pm->ops[res[j].first]->ID << "->" << pm->ops[res[j].second]->ID << endl;

		    out << "operations on machine " << mid << " : " << endl;
		    for (int k = 0; k < trgmachnodes.size(); k++) {
			    out << pm->ops[trgmachnodes[k]]->ID << ",";
		    }

		    out << endl << endl;

		    out << *pm << endl;
		    getchar();
	    }
    }
     */
    // #############################################################################

    //out << "Done selecting breakable arcs." << endl;

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPM::selectRelevantArcsFromPath(const Path<ListDigraph> &path, const ListDigraph::Node& node) {
    /** Algorithm:
     * 
     * 1. Iterate over all arcs in the path
     * 2. For every machine from the needed tool group collect the operations to be processed on it
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res1;

    ListDigraph::Node j;
    ListDigraph::Node k;
    //bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine
    QMap<int, QVector<ListDigraph::Node> > machid2opers;

    for (int i = 0; i < path.length(); i++) {
	ListDigraph::Arc ait = path.nth(i);
	j = pm->graph.source(ait);

	if (pm->ops[j]->toolID == pm->ops[node]->toolID && pm->ops[j]->machID > 0) {
	    machid2opers[pm->ops[j]->machID].append(j);
	}
    }

    // And the last one
    j = pm->graph.target(path.back());

    if (pm->ops[j]->toolID == pm->ops[node]->toolID && pm->ops[j]->machID > 0) {
	machid2opers[pm->ops[j]->machID].append(j);
    }


    // Build the set of relevant arcs
    for (QMap<int, QVector<ListDigraph::Node> >::iterator iter = machid2opers.begin(); iter != machid2opers.end(); iter++) {

	for (int i = 0; i < iter.value().size() - 1; i++) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().at(i), iter.value().at(i + 1)));
	}

	int prevsize = res.size();

	if (iter.value().size() > 0) {
	    // Check whether insertion can be performed before the first operation
	    j = iter.value().first();
	    for (ListDigraph::InArcIt iait(pm->graph, j); iait != INVALID; ++iait) {
		if (pm->ops[pm->graph.source(iait)]->machID == pm->ops[j]->machID) { // Insertion 
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(iait), j));
		}
	    }
	    if (res.size() == prevsize) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, j));
	    }

	    prevsize = res.size();
	    // Check whether insertion can be performed after the last operation
	    j = iter.value().last();
	    for (ListDigraph::OutArcIt oait(pm->graph, j); oait != INVALID; ++oait) {
		if (pm->ops[pm->graph.target(oait)]->machID == pm->ops[j]->machID) { // Insertion 
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, pm->graph.target(oait)));
		}
	    }
	    if (prevsize == res.size()) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, INVALID));
	    }
	}



    }

    // Check the conjunctive arcs. If the start node has an outgoing or the end node has an incoming schedule-base arc =>
    // remove this arc and insert the other two
    bool incluconj = true;
    bool conj = false;
    ListDigraph::Arc arc;
    for (int i = 0; i < res.size(); i++) {

	if (res[i].first == INVALID || res[i].second == INVALID) {
	    res1.append(res[i]);
	    continue;
	}

	conj = false;
	for (ListDigraph::OutArcIt oait(pm->graph, res[i].first); oait != INVALID; ++oait) {
	    if (pm->graph.target(oait) == res[i].second) {
		conj = pm->conjunctive[oait];
		break;
	    }
	}

	if (conj) {
	    incluconj = true;
	    // Search the outgoing arcs for the start node
	    for (ListDigraph::OutArcIt oait(pm->graph, res[i].first); oait != INVALID; ++oait) {
		if (!pm->conjunctive[oait]) {
		    res1.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(oait), pm->graph.target(oait)));
		    incluconj = false;
		    break;
		}
	    }

	    // Search the incoming arcs for the end node
	    for (ListDigraph::InArcIt iait(pm->graph, res[i].second); iait != INVALID; ++iait) {
		if (!pm->conjunctive[iait]) {
		    res1.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(iait), pm->graph.target(iait)));
		    incluconj = false;
		    break;
		}
	    }

	    if (incluconj) {
		res1.append(res[i]);
	    }

	} else {
	    res1.append(res[i]);
	}
    }

    return res1;
}

bool LocalSearchPM::moveOperPossible(const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node & node) {
    //QTextStream out(stdout);
    //	opMovePossibleTimer.start();

    /**
     * We are trying to insert the given node between the start and the end nodes
     * of the specified arcs. The conditions described by Dauzere-Peres and Paulli
     * are checked, since no cycles should occur.
     */

    //if ((node == pm->graph.source(arc)) || (node == pm->graph.target(arc))) return false;

    //if (j == node || k == node) return false;

    /** This means that there are no other operations on the machine and 
     * therefore moving some operation to this machine will result in only 
     * deleting its previous connections in the graph. Thus, no cycles can occur.*/
    if (j == INVALID && k == INVALID) {
	//		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
	return true;
    }

    //if (pm->conPathExists(j, k)) return false;

    QList<ListDigraph::Node> fri; // IMPORTANT!!! There can be several routing predecessors or successors of the node i
    QList<ListDigraph::Node> pri;

    // Find the fri and the pri
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	if (pm->conjunctive[oait]) { // The routing based arcs
	    fri.append(pm->graph.target(oait));
	}
    }

    for (ListDigraph::InArcIt iait(pm->graph, node); iait != INVALID; ++iait) {
	if (pm->conjunctive[iait]) { // The routing based arcs
	    pri.append(pm->graph.source(iait));
	}
    }

    if (j != INVALID) {
	for (int i1 = 0; i1 < fri.size(); i1++) {
	    if (j == fri[i1]) {
		//				opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    }
	}
    }

    if (k != INVALID) {
	for (int i2 = 0; i2 < pri.size(); i2++) {
	    if (k == pri[i2]) {
		//				opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    }
	}
    }

    // Check condition (inequalities) with predecessors and successors
    for (int i1 = 0; i1 < fri.size(); i1++) {
	for (int i2 = 0; i2 < pri.size(); i2++) {
	    bool cond1;
	    bool cond2;

	    if (j != INVALID) {
		cond1 = Math::cmp(pm->ops[j]->r(), pm->ops[fri[i1]]->r() + pm->ops[fri[i1]]->p()) == -1;

		//cout << "Oper j " << pm->ops[j]->ID << " r =  " << pm->ops[j]->r() << endl;
		//cout << "Oper fri " << pm->ops[fri[i1]]->ID << " r+p =  " << pm->ops[fri[i1]]->r() + pm->ops[fri[i1]]->p() << endl;

	    } else {
		cond1 = true;
	    }
	    if (k != INVALID) {
		cond2 = Math::cmp(pm->ops[k]->r() + pm->ops[k]->p(), pm->ops[pri[i2]]->r()) == 1;

		//cout << "Oper k " << pm->ops[k]->ID << " r+p =  " << pm->ops[k]->r() + pm->ops[k]->p() << endl;
		//cout << "Oper prk " << pm->ops[pri[i2]]->ID << " r =  " << pm->ops[pri[i2]]->r() << endl;

	    } else {
		cond2 = true;
	    }

	    if (!(cond1 && cond2)) {
		//				opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    } else {
		//out << "Moving " << pm->ops[node]->ID << " between " << ((j != INVALID) ? pm->ops[j]->ID : -2) << " and " << ((k != INVALID) ? pm->ops[k]->ID : -1) << endl;
		//out << *pm << endl;
	    }
	}
    }

    //	opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
    return true;
}

QPair<ListDigraph::Node, ListDigraph::Node> LocalSearchPM::selectArcToBreak(const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node & node) {
    //QTextStream out(stdout);
    /** Algorithm:
     
     * 1. Traverse arcs until the needed criterion is satisfied
     * 2. Return the selected arc
     * 
     */

    QPair<ListDigraph::Node, ListDigraph::Node> curarc;
    ListDigraph::Node j;
    ListDigraph::Node k;

    //QSet<int> rndindices;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > modarcs = arcs;
    int idx;
    int lpos = modarcs.size() - 1;

    //out << "LocalSearchPM::selectArcToBreak : Before selecting an arc to break : " << endl << *pm << endl;

    //if (!debugCheckPMCorrectness("LocalSearchPM::selectArcToBreak")) {
    //	Debugger::err << "PM is not correct while selecting an arc to break!" << ENDL;
    //}

    do {

	if (lpos == -1) {
	    j = INVALID;
	    k = INVALID;
	    if (arcs.size() > 0) {
		QTextStream out(stdout);
		out << "Moving operation : " << pm->ops[node]->ID << endl;
		for (int i = 0; i < arcs.size(); i++) {
		    out << ((arcs[i].first == INVALID) ? -1 : pm->ops[arcs[i].first]->ID) << " -> " << ((arcs[i].second == INVALID) ? -1 : pm->ops[arcs[i].second]->ID) << endl;
		}
		out << *pm << endl;
		Debugger::eDebug("LocalSearchPM::selectArcToBreak : Failed to find other insertion positions!!!");
	    }
	    break;
	}

	// Select the next arc to be considered as a break candidate

	idx = intRNG->rnd(0, lpos);
	curarc = modarcs[idx];
	modarcs.move(idx, lpos);
	lpos--;

	j = curarc.first;
	k = curarc.second;

	//out << "Move option:  " << ((j == INVALID) ? -1 : pm->ops[j]->ID) << " -> " << ((k == INVALID) ? -1 : pm->ops[k]->ID) << endl;

    } while (!moveOperPossible(j, k, node));

    //out << "LocalSearchPM::selectArcToBreak : Done!" << endl;

    return QPair<ListDigraph::Node, ListDigraph::Node > (j, k);
}

QPair<ListDigraph::Node, ListDigraph::Node> LocalSearchPM::selectBestArcToBreak(const int& mid, const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node) {
    double bstobj = Math::MAX_DOUBLE;
    double cobj;
    QPair<ListDigraph::Node, ListDigraph::Node> bstmove;

    bstmove.first = INVALID;
    bstmove.second = INVALID;

    for (int j = 0; j < arcs.size(); j++) {
	if (moveOperPossible(arcs[j].first, arcs[j].second, node)) {

	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "LocalSearchPM::selectBestArcToBreak : Graph BEFORE moving the operation: " << endl << *pm << endl;
	    //out << "Moving operation : " << *pm->ops[optomove] << endl;
	    //out << "Moving between : " << ((arcs[j].first != INVALID) ? pm->ops[arcs[j].first]->ID : -1) << " and " << ((arcs[j].second != INVALID) ? pm->ops[arcs[j].second]->ID : -1) << endl;

	    // Try to move the operation 
	    moveOper(mid, arcs[j].first, arcs[j].second, node);

	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "Moved operation : " << *pm->ops[optomove] << endl;

	    //out << "LocalSearchPM::selectBestArcToBreak : Graph AFTER moving the operation: " << endl << *pm << endl;
	    /*
	    if (!dag(pm->graph)) {
		    Debugger::err << "LocalSearchPM::selectBestArcToBreak : Graph not DAG after operation move!!!" << ENDL;
	    }
	     */

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();
	    updateEval(nodeI, nodeT);

	    // Calculate the current objective
	    //			objTimer.start();
	    cobj = (*obj)(*pm);
	    //			objElapsedMS += objTimer.elapsed();

	    //out << "bstobj = " << bstobj << endl;
	    //out << "cobj = " << cobj << endl;

	    // Check whether the move is the best
	    if (bstobj >= cobj) { // This move is not the worse one
		bstobj = cobj;
		bstmove = arcs[j];

		//out << "Best objective found when moving to machine " << bstmachid << endl;
	    }

	    // Move back the operation
	    moveBackOper(node);

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "LocalSearchPM::selectBestArcToBreak : Graph after moving BACK the operation: " << endl << *pm << endl;

	    // IMPORTANT!!! Restore only if the graph has changed since the last move

	    /*
	    if (!prevRS.empty()) {
		    ListDigraph::Node curnode;
		    int n = topolOrdering.size(); //topolSorted.size();
		    //for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
		    for (int j = topolITStart; j < n; j++) {
			    curnode = topolOrdering[j]; //topolSorted[j];
			    pm->ops[curnode]->r(prevRS[curnode].first);
			    pm->ops[curnode]->s(prevRS[curnode].second);

			    //out << "Restoring for : " << pm->ops[curnode]->ID << endl;
			    //out << "r = ( " << pm->ops[curnode]->r() << " , " << prevRS[curnode].first << " ) " << endl;
			    //out << "s = ( " << pm->ops[curnode]->s() << " , " << prevRS[curnode].second << " ) " << endl;


			    //if (Math::cmp(pm->ops[curnode]->r(), prevRS[curnode].first, 0.0001) != 0) {
			    //Debugger::err << "LocalSearchPM::selectBestArcToBreak : Something is wrong with r while restoring!!!" << ENDL;
			    //}

			    //if (Math::cmp(pm->ops[curnode]->s(), prevRS[curnode].second, 0.0001) != 0) {
			    //Debugger::err << "LocalSearchPM::selectBestArcToBreak : Something is wrong with s while restoring!!!" << ENDL;
			    //}


		    }

	    }

	     */

	}
    }

    //out << "Found best move : " << ((bstmove.first != INVALID) ? pm->ops[bstmove.first]->ID : -1) << " and " << ((bstmove.second != INVALID) ? pm->ops[bstmove.second]->ID : -1) << endl;

    return bstmove;
}

void LocalSearchPM::findBestOperMove(const ListDigraph::Node& optm, int& targetMachID, QPair<ListDigraph::Node, ListDigraph::Node>& atb) {
    //QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Get the list of machines able to process the operation
     * 2. For every machine:
     * 2.1. Select pairs of operations between which the operation can be moved (Collect the move possibilities)
     * 3. For every selected move possibility estimate the total objective.
     * 4. Return the best possible move possibility
     */

    QHash< int, QList<QPair<ListDigraph::Node, ListDigraph::Node> > > machid2arcs;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > breakablearcs;

    // Get the list of available in the TG machines
    QList<Machine*> tgmachines = ((*rc)(pm->ops[optm]->toolID)).machines();

    // Iterate over the machines of the relative tool group
    for (int machidx = 0; machidx < tgmachines.size(); machidx++) {

	// Select potential insertion positions on the current machine
	breakablearcs = selectBreakableArcs(tgmachines[machidx]->ID);

	for (int j = 0; j < breakablearcs.size(); j++) {
	    if (moveOperPossible(breakablearcs[j].first, breakablearcs[j].second, optm)) {
		machid2arcs[tgmachines[machidx]->ID].append(breakablearcs[j]);
	    }
	}

	// In case the machine is empty
	if (machid2arcs[tgmachines[machidx]->ID].size() == 0) {
	    machid2arcs[tgmachines[machidx]->ID].append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, INVALID));
	}

	//out << "Found breakable arcs for machine " << tgmachines[machidx]->ID << " : " << machid2arcs[tgmachines[machidx]->ID].size() << endl;
    }

    // Iterate over the possible moves and select the best move possible
    double bstobj = Math::MAX_DOUBLE;
    double cobj;
    int bstmachid = -1;
    QPair<ListDigraph::Node, ListDigraph::Node> bstmove;


    for (QHash< int, QList<QPair<ListDigraph::Node, ListDigraph::Node> > >::iterator iter = machid2arcs.begin(); iter != machid2arcs.end(); iter++) {
	for (int j = 0; j < iter.value().size(); j++) {
	    // Try to move the operation 
	    moveOper(iter.key(), iter.value()[j].first, iter.value()[j].second, optm);

	    // Update the graph
	    pm->updateHeads();
	    pm->updateStartTimes();

	    // Calculate the current objective
	    cobj = (*obj)(*pm);

	    //out << "bstobj = " << bstobj << endl;
	    //out << "cobj = " << cobj << endl;

	    // Check whether the move is the best
	    if (bstobj >= cobj) { // This move is not the worse one
		bstobj = cobj;
		bstmachid = iter.key();
		bstmove = iter.value()[j];

		//out << "Best objective found when moving to machine " << bstmachid << endl;
	    }

	    // Move back the operation
	    moveBackOper(optm);

	    // Update the graph
	    pm->updateHeads();
	    pm->updateStartTimes();

	}
    }

    // Return the best found potential move
    if (bstmachid == -1) {
	QTextStream out(stdout);
	out << "Moving operation " << pm->ops[optm]->ID << endl;
	out << *pm << endl;
	Debugger::err << "LocalSearchPM::findBestOperMove : failed to find the best move!" << ENDL;
    } else {
	targetMachID = bstmachid;
	atb = bstmove;
    }

}

void LocalSearchPM::moveOper(const int& mid, const ListDigraph::Node &jNode, const ListDigraph::Node &kNode, const ListDigraph::Node& node) {
    //	opMoveTimer.start();

    /** Algorithm:
     * 
     * 1. Remove schedule-based arcs of the specified node
     * 2. Insert a single arc connecting the nodes which were preceding and
     *	  succeeding the node in the schedule
     * 3. Remove the specified arc
     * 4. Insert the node between the starting an the ending nodes of the 
     *    specified arc
     * 
     */

    //out << "PM before moving operation : " << pm->ops[node]->ID << endl << *pm << endl;

    //###########################  DEBUG  ######################################

    /*
    out << "Moving operation : " << pm->ops[node]->ID << endl;

    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;

			    for (int l = 0; l < topolOrdering.size(); l++) {
				    out << pm->ops[topolOrdering[l]]->ID << " ";
			    }
			    out << endl;

			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct before moving the operation!!!" << ENDL;
		    }
	    }
    }
     */

    /*
    out << *pm << endl;

    //debugCheckPMCorrectness("LocalSearchPM::moveOper : Before moving the next operation.");

    if (j != INVALID && k != INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << pm->ops[j]->ID << " and " << pm->ops[k]->ID << endl;
    }

    if (j != INVALID && k == INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << pm->ops[j]->ID << " and " << " * " << endl;
    }

    if (j == INVALID && k != INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << " * " << " and " << pm->ops[k]->ID << endl;
    }
     */

    //##########################################################################

    ListDigraph::Node sNode = INVALID;
    ListDigraph::Node tNode = INVALID;
    ListDigraph::Arc siArc = INVALID;
    ListDigraph::Arc itArc = INVALID;
    ListDigraph::Arc stArc = INVALID;
    ListDigraph::Arc jiArc = INVALID;
    ListDigraph::Arc ikArc = INVALID;

    arcsRem.clear();
    arcsIns.clear();
    weightsRem.clear();

    // IMPORTANT!!!
    //topolSorted.clear();
    prevRS.clear();

    if ((node == jNode) || (node == kNode)) { // The operation is not moved
	remMachID = pm->ops[node]->machID;

	arcsRem.clear();
	arcsIns.clear();
	weightsRem.clear();

	// No need to perform topological sorting since the graph stays unchanged
	// IMPORTANT!!! Clear the old topol. sorting so that the preserved ready times and start times are not restored incorrectly
	//topolSorted.clear();
	prevRS.clear();

	// IMPORTANT!!! Set the actual topological ordering! Else the incorrect TO is restored
	prevTopolOrdering = topolOrdering;
	prevTopolITStart = topolITStart;

	//out << "###################   Operation is not moved!" << endl;
	return;
    }

    //if (j == INVALID && k != INVALID) out << "#####################  Moving to the front." << endl;
    //if (j != INVALID && k == INVALID) out << "#####################  Moving to the back." << endl;
    //if (j != INVALID && k != INVALID) out << "#####################  Moving in the middle." << endl;
    //if (j == INVALID && k == INVALID) out << "#####################  Moving to the empty machine." << endl;

    // Find the fri and the pri
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	if (!pm->conjunctive[oait]) { // The schedule-based arcs
	    tNode = pm->graph.target(oait);
	    itArc = oait;
	    break;
	}

	// If there is no schedule-based arc then the routing-based successor might come into consideration
    }

    for (ListDigraph::InArcIt iait(pm->graph, node); iait != INVALID; ++iait) {
	if (!pm->conjunctive[iait]) { // The schedule-based arcs
	    sNode = pm->graph.source(iait);
	    siArc = iait;
	    break;
	}
    }

    //Debugger::iDebug("Removing previous connections...");
    // Remove the former connections
    if (sNode != INVALID) {
	arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (sNode, node));
	weightsRem.append(pm->p[siArc]);

	//out << "Erasing " << pm->ops[s]->ID << " -> " << pm->ops[node]->ID << endl;

	pm->graph.erase(siArc);
    }

    //###########################  DEBUG  ######################################

    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing si!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    if (tNode != INVALID) {
	arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (node, tNode));
	weightsRem.append(pm->p[itArc]);

	//out << "Erasing " << pm->ops[node]->ID << " -> " << pm->ops[t]->ID << endl;

	pm->graph.erase(itArc);
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing it!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    // Insert the direct connection between s and t
    if (sNode != INVALID && tNode != INVALID /*&& !pm->conPathExists(s, t)*/) {
	stArc = pm->graph.addArc(sNode, tNode);
	arcsIns.append(stArc);
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after inserting st!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    // Remove the arc to break (if this arc exists)
    if (jNode != INVALID && kNode != INVALID) {
	for (ListDigraph::OutArcIt oait(pm->graph, jNode); oait != INVALID; ++oait)
	    if (pm->graph.target(oait) == kNode) {
		if (!pm->conjunctive[oait]) {
		    arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (jNode, kNode));
		    weightsRem.append(/*-pm->ops[j]->p()*/pm->p[oait]);

		    pm->graph.erase(oait);
		    break;
		}
	    }
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing jk!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    //Debugger::iDebug("Removed previous connections.");


    //Debugger::iDebug("Inserting new connections...");

    // Insert the new connections
    if (jNode != INVALID /*&& !pm->conPathExists(j, node)*/) {
	jiArc = pm->graph.addArc(jNode, node);
	arcsIns.append(jiArc);
    } else {
	jiArc = INVALID;
    }
    if (kNode != INVALID /*&& !pm->conPathExists(node, k)*/) {
	ikArc = pm->graph.addArc(node, kNode);
	arcsIns.append(ikArc);
    } else {
	ikArc = INVALID;
    }

    //Debugger::iDebug("Inserted new connections.");

    // Update the topological ordering of the graph dynamically

    // Save the current topological ordering
    prevTopolOrdering = topolOrdering;
    prevTopolITStart = topolITStart;

    // Update the topological ordering
    //	dynTopSortTimer.start();
    //out << "Performing DTO..." << endl;
    /*
    if (!dag(pm->graph)) {
	    Debugger::err << "Graph is not DAG before the DTO!" << ENDL;
    }
     */
    dynUpdateTopolOrdering(topolOrdering, node, jNode, kNode);
    //out << "Done DTO." << endl;
    //	dynTopSortElapsedMS += dynTopSortTimer.elapsed();

    // Update the recalculation region
    int idxt = topolOrdering.indexOf(tNode);
    int idxi = topolOrdering.indexOf(node);
    if (idxt >= 0 && idxi >= 0) {
	topolITStart = Math::min(idxt, idxi);
    } else {
	topolITStart = Math::max(idxt, idxi);
    }


    /*
	    // Perform topological sorting of the nodes reachable from i and/or t in the NEW graph G-tilde
	    QList<ListDigraph::Node> startSet;
	    startSet.append(t);
	    startSet.append(node);
     */

    // Sort topologically all nodes reachable from i and/or from t
    //out << "Performing topological sorting..." << endl;
    //	topSortTimer.start();
    //QList<ListDigraph::Node> reachable = pm->reachableFrom(startSet);
    /*
    int idxt = topolOrdering.indexOf(t);
    int idxi = topolOrdering.indexOf(node);
    int startidx;
    if (idxt >= 0 && idxi >= 0) {
	    startidx = Math::min(idxt, idxi);
    } else {
	    startidx = Math::max(idxt, idxi);
    }

    topolSorted = topolOrdering.mid(startidx, topolOrdering.size() - startidx);
     */
    //	topSortElapsedMS += topSortTimer.elapsed();
    //out << "Performed topological sorting." << endl;

    //######################  DEBUG  ###########################################

    // Check the correctness of the topological sorting
    /*
    out << "Topological sorting : " << endl;
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    out << pm->ops[topolSorted[i]]->ID << " ";
    }
    out << endl;
     */


    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    for (int j = i + 1; j < topolSorted.size(); j++) {
		    if (reachable(topolSorted[j], topolSorted[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolSorted[j]]->ID << " -> " << pm->ops[topolSorted[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct!!!" << ENDL;
		    }
	    }
    }
     */

    //out << "PM before preserving : " << *pm << endl;

    //##########################################################################

    // Preserve the ready times and the start times of the previous graph for the topologically sorted nodes
    Operation *curop = NULL;
    int n = topolOrdering.size(); //topolSorted.size();
    //for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
    for (int j = topolITStart/*0*/; j < n; j++) {
	curop = pm->ops[topolOrdering[j]];
	// Preserve the former value => WRONG!!!
	//out << "Preserving for : " << pm->ops[curnode]->ID << endl;
	//out << "r = " << pm->ops[curnode]->r() << endl;
	//out << "s = " << pm->ops[curnode]->s() << endl;
	prevRS[curop->ID].first = curop->r();
	prevRS[curop->ID].second = curop->s();
    }


    //Debugger::iDebug("Updating the data of the newly inserted operation...");

    // Update the machine id of the moved operation and preserve the previous assignment ID
    remMachID = pm->ops[node]->machID;
    pm->ops[node]->machID = mid;
    /*
    if (j != INVALID) {
	    pm->ops[node]->machID = pm->ops[j]->machID;
    } else {
	    if (k != INVALID) {
		    pm->ops[node]->machID = pm->ops[k]->machID;
	    } else {
		    Debugger::eDebug("LocalSearchPM::moveOper : Moving operation between two invalid operations!");
	    }
    }
     */

    //Debugger::iDebug("Updating the proc. time of the newly inserted operation...");
    // Update the processing time of the moved operation
    pm->ops[node]->p(((*rc)(pm->ops[node]->toolID, pm->ops[node]->machID)).procTime(pm->ops[node]));
    //Debugger::iDebug("Updated the proc. time of the newly inserted operation.");

    //Debugger::iDebug("Setting the weights of the newly inserted arcs...");
    // Set the weights of the newly inserted arcs
    //Debugger::iDebug("st...");
    if (stArc != INVALID) {
	pm->p[stArc] = -pm->ops[sNode]->p();
    }
    //Debugger::iDebug("st.");
    if (jiArc != INVALID) {
	pm->p[jiArc] = -pm->ops[jNode]->p();
    }
    //Debugger::iDebug("Set the weights of the newly inserted arcs.");

    //Debugger::iDebug("Recalculating the processing time of the moved operation...");
    // Processing time for the moved operation must be updated
    if (ikArc != INVALID) {
	pm->p[ikArc] = -pm->ops[node]->p();
    }
    //Debugger::iDebug("Recalculated the processing time of the moved operation.");

    // Update length of all arcs going out from i
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	pm->p[oait] = -pm->ops[node]->p();
    }

    //Debugger::iDebug("Updated the data of the newly inserted operation.");

    // Set the outgoing nodes for the update
    nodeI = node;
    nodeT = tNode;

    //	opMoveElapsedMS += opMoveTimer.elapsed();
}

void LocalSearchPM::moveBackOper(const ListDigraph::Node & node) {
    //	opMoveBackTimer.start();
    //out << "Moving back operation : " << pm->ops[node]->ID << endl;

    // Remove the newly inserted arcs
    for (int i = 0; i < arcsIns.size(); i++) {
	pm->graph.erase(arcsIns[i]);
    }
    arcsIns.clear();

    // Insert the previous arcs
    ListDigraph::Arc curarc;
    for (int i = 0; i < arcsRem.size(); i++) {
	curarc = pm->graph.addArc(arcsRem[i].first, arcsRem[i].second);
	pm->p[curarc] = weightsRem[i];

	//out << "Restored " << pm->ops[arcsRem[i].first]->ID << " -> " << pm->ops[arcsRem[i].second]->ID << endl;

	/*
	if (pm->graph.source(curarc) == node) {
		pm->ops[node]->machID = pm->ops[pm->graph.target(curarc)]->machID;
	} else {
		if (pm->graph.target(curarc) == node) {
			pm->ops[node]->machID = pm->ops[pm->graph.source(curarc)]->machID;
		}
	}
	 */
    }

    // Restore the machine assignment of the operation
    pm->ops[node]->machID = remMachID;

    // Restore the processing time of the moved operation
    pm->ops[node]->p(((*rc)(pm->ops[node]->toolID, pm->ops[node]->machID)).procTime(pm->ops[node]));

    // Restore arc lengths of the arcs coming out from i
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	pm->p[oait] = -pm->ops[node]->p();
    }

    arcsRem.clear();
    weightsRem.clear();

    //out << "PM after moving Back operation : " << pm->ops[node]->ID << endl << *pm << endl;


    // Restore the ready times and the due dates if the graph has changed
    // IMPORTANT!!! Update only if the graph has been changed!!!
    // IMPORTANT!!! Restor r and s BEFORE the old topological ordering is restored
    if (!prevRS.empty()) {
	Operation *curop;
	int n = topolOrdering.size(); //topolSorted.size();
	//for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
	for (int j = topolITStart; j < n; j++) {
	    curop = pm->ops[topolOrdering[j]];
	    curop->r(prevRS[curop->ID].first);
	    curop->s(prevRS[curop->ID].second);

	    //out << "Restoring for : " << pm->ops[curnode]->ID << endl;
	    //out << "r = ( " << pm->ops[curnode]->r() << " , " << prevRS[curnode].first << " ) " << endl;
	    //out << "s = ( " << pm->ops[curnode]->s() << " , " << prevRS[curnode].second << " ) " << endl;


	    //if (Math::cmp(pm->ops[curnode]->r(), prevRS[curnode].first, 0.0001) != 0) {
	    //Debugger::err << "Something is wrong with r while restoring!!!" << ENDL;
	    //}

	    //if (Math::cmp(pm->ops[curnode]->s(), prevRS[curnode].second, 0.0001) != 0) {
	    //Debugger::err << "Something is wrong with s while restoring!!!" << ENDL;
	    //}

	}
    }

    // Restore the previous topological ordering of the nodes 
    topolOrdering = prevTopolOrdering;
    topolITStart = prevTopolITStart;

    // #########################    DEBUG    ###################################    

    /*
    out << "Check reachability for every machine after moving BACK the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency after moving back the operation..."<<endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */
    // #########################################################################
    //	opMoveBackElapsedMS += opMoveBackTimer.elapsed();
}

ListDigraph::Node LocalSearchPM::selectTerminalContrib(QList<ListDigraph::Node> &terminals) {
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
    double arbnum = floatRNG->rnd(0.0, totalobj);

    // Find an interval that contains the generated number
    for (int i = 0; i < interval2node.size(); i++) {
	if (interval2node[i].first.first <= arbnum && arbnum <= interval2node[i].first.second) {
	    res = interval2node[i].second;

	    break;
	}
    }

    return res;
}

ListDigraph::Node LocalSearchPM::selectTerminalRnd(QList<ListDigraph::Node> &terminals) {

    return terminals[intRNG->rnd(0, terminals.size() - 1)];
}

ListDigraph::Node LocalSearchPM::selectTerminalNonContrib(QList<ListDigraph::Node> &terminals) {
    QList<QPair<QPair<double, double>, ListDigraph::Node > > interval2node;
    double totalobj = 0.0;
    double istart;
    double iend;
    ListDigraph::Node res = INVALID;
    double maxtwt = 0.0;

    // Find the biggest weighted tardiness
    for (int i = 0; i < terminals.size(); i++) {
	maxtwt = Math::max(maxtwt, pm->ops[terminals[i]]->wT());
    }

    for (int i = 0; i < terminals.size(); i++) {
	istart = totalobj;
	totalobj += maxtwt - pm->ops[terminals[i]]->wT(); // The bigger the contribution the smaller the probability is
	iend = totalobj;

	interval2node.append(QPair<QPair<double, double>, ListDigraph::Node > (QPair<double, double>(istart, iend), terminals[i]));
    }

    // Choose an arbitrary number
    double arbnum = floatRNG->rnd(0.0, totalobj);

    // Find an interval that contains the generated number
    for (int i = 0; i < interval2node.size(); i++) {
	if (interval2node[i].first.first <= arbnum && arbnum <= interval2node[i].first.second) {
	    res = interval2node[i].second;

	    break;
	}
    }

    return res;
}

void LocalSearchPM::diversify() {
    //Debugger::info << "LocalSearchPM::diversify ... " << ENDL;
    //return;
    /** Algorithm:
     * 
     * 1. Select the random number of arcs to be reversed
     * 
     * 2. Reverse randomly the selected number of critical arcs 
     * 
     */

    pm->restore();
    topolOrdering = pm->topolSort();
    topolITStart = 0;
    pm->updateHeads(topolOrdering);
    pm->updateStartTimes(topolOrdering);
    curobj = (*obj)(*pm);

    updateCriticalNodes();

    int nops2move = intRNG->rnd(5, 10);
    int nopsmoved = 0;

    // Get the terminals
    QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath;
    ListDigraph::Node cop;

    //out <<"PM before the diversification:"<<endl;
    //out << *pm << endl;
    //getchar();

    do {

	do {
	    // Select some terminal for the manipulations (based on the contribution of the terminal to the objective)

	    theterminal = selectTerminalContrib(terminals);

	    // Find a critical path to the selected terminal
	    //cpath = longestPath(theterminal);

	    // Select operation to move
	    //cop = defaultSelectOperToMove(cpath);
	    cop = criticalNodes[intRNG->rnd(0, criticalNodes.size() - 1)];

	} while (cop == INVALID);

	int targetMachID = selectTargetMach(cop);

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs = selectBreakableArcs(targetMachID);

	// Select an arc to break
	QPair<ListDigraph::Node, ListDigraph::Node> atb = selectArcToBreak(relarcs, cop);

	// Move the operation
	moveOper(targetMachID, atb.first, atb.second, cop);

	//if (!dag(pm->graph)) moveBackOper(cop);

	// Update the ready times and the start times of the operations in the graph
	pm->updateHeads(topolOrdering);
	pm->updateStartTimes(topolOrdering);

	nopsmoved++;


	//out <<"PM after the first step of diversification:"<<endl;
	//out << *pm << endl;
	//getchar();


	if ((*obj)(*pm) < bestobj) {
	    pm->save();
	    curobj = (*obj)(*pm);
	    prevobj = curobj;
	    bestobj = curobj;
	    nisteps = 0;

	    updateCriticalNodes();
	    break;
	}


    } while (/*objimprov(*pm, pm->terminals()) > bestobjimprov &&*/ nopsmoved < nops2move);

    //if (objimprov(*pm, pm->terminals()) < bestobjimprov) {
    //pm->save();
    curobj = (*obj)(*pm);
    prevobj = curobj; // + 0.00000000001;
    //bestobj = curobj;
    nisteps = 0;
    //bestobjimprov = Math::MAX_DOUBLE;
    //prevobjimprov = Math::MAX_DOUBLE;
    //}

    updateCriticalNodes();

    //Debugger::info << "LocalSearchPM::diversify : Finished. " << ENDL;

}

void LocalSearchPM::updateEval(const ListDigraph::Node& /*iNode*/, const ListDigraph::Node& /*tNode*/) {
    //	updateEvalTimer.start();

    /** Algorithm:
     * 
     * 1. Collect nodes reachable from i
     * 2. Collect nodes reachable from t
     * 3. The union of these sets is the set of nodes to be updated for the evaluation
     * 4. If i is reachable from t then start updating from t
     *	  If t is reachable from i then start updating from i
     *    Else update starting from i and from t	
     * 
     */

    //out << "Running updateEval..." << endl;

    // For the topologically sorted nodes update the ready times and the start times of the operations (preserving the former values)

    // Update the ready times
    ListDigraph::Node curnode;
    ListDigraph::Node prevnode;
    double curR; // The calculated ready time of the current node

    int n = topolOrdering.size();

    for (int j = topolITStart; j < n; j++) {
	curnode = topolOrdering[j];

	curR = 0.0;
	// Iterate over all predecessors of the current node
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    prevnode = pm->graph.source(iait);
	    curR = Math::max(curR, pm->ops[prevnode]->r() + pm->ops[prevnode]->p());
	}

	//############################  DEBUG  #################################
	//out << "r = ( " << pm->ops[curnode]->r() << " , " << curR << " ) " << endl;

	/*
	if (pm->ops[curnode]->r() != curR) {
		out << "Current node ID = " << pm->ops[curnode]->ID << endl;
		out << *pm << endl;
		Debugger::err << "Something is wrong with r !!!" << ENDL;
	}
	 */
	//######################################################################

	pm->ops[curnode]->r(curR/*, false*/);

    }

    // Update the start times
    double curS; // The calculated start time of the current node

    for (int j = topolITStart; j < n; j++) {
	curnode = topolOrdering[j];

	curS = Math::max(pm->ops[curnode]->ir(), pm->ops[curnode]->r());
	// Iterate over all predecessors of the current node
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    prevnode = pm->graph.source(iait);
	    curS = Math::max(curS, pm->ops[prevnode]->c());
	}

	//############################  DEBUG  #################################

	/*
	//out << "s = ( " << pm->ops[curnode]->s() << " , " << curS << " ) " << endl;
	if (pm->ops[curnode]->s() != curS) {
		Debugger::err << "Something is wrong with s !!!" << ENDL;
	}
	 */
	//######################################################################

	// Take into account the machine's availability time
	curS = Math::max(curS, pm->ops[curnode]->machAvailTime());

	// Seth the start time of the operation
	pm->ops[curnode]->s(curS);
    }

    //out << "Running updateEval done." << endl;

    //######################  DEBUG  ###########################################

    // Check the correctness of the topological sorting

    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    out << pm->ops[topolSorted[i]]->ID << " ";
    }
    out << endl;
     */

    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    for (int j = i + 1; j < topolSorted.size(); j++) {
		    if (reachable(topolSorted[j], topolSorted[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolSorted[j]]->ID << " -> " << pm->ops[topolSorted[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    //	updateEvalElapsedMS += updateEvalTimer.elapsed();
}

void LocalSearchPM::dynUpdateTopolOrdering(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &j, const ListDigraph::Node &k) {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Find the positions of i, j and k
     * 2. IF posj < posi < posk => no changes to the topological sorting need to be performed. Return.
     * 3. IF posi > posk => reorder the nodes. The affected region is [posi, posk]. Return.
     * 4. IF posi < posj => reorder the nodes. The affected region is [posj, posi]. Return.
     * 
     */

    Math::intUNI posj = -1;
    Math::intUNI posi = -1;
    Math::intUNI posk = -1;

    if (j == INVALID) {
	posj = -1;
    } else {
	posj = topolOrdering.indexOf(j);
    }

    if (k == INVALID) {
	posk = Math::MAX_INTUNI;
	//out << "k is INVALID,  posk=" << posk << " " << "MAX_INTUNI=" << Math::MAX_INTUNI << endl;
    } else {
	posk = topolOrdering.indexOf(k);
    }

    posi = topolOrdering.indexOf(i);

    if (posj < posi && posi < posk) { // No changes to perform
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

    if (posj >= posk) {
	QTextStream out(stdout);
	out << "posj = " << posj << " ID = " << ((j == INVALID) ? -1 : pm->ops[j]->ID) << endl;
	out << "posi = " << posi << " ID = " << ((i == INVALID) ? -1 : pm->ops[i]->ID) << endl;
	out << "posk = " << posk << " ID = " << ((k == INVALID) ? -1 : pm->ops[k]->ID) << endl;

	for (Math::intUNI l = 0; l < topolOrdering.size(); l++) {
	    out << pm->ops[topolOrdering[l]]->ID << " ";
	}
	out << endl;

	Debugger::err << "LocalSearchPM::dynUpdateTopolOrdering : posj >= posk which is impossible!!!" << ENDL;
    }

    // Find the affected region
    Math::intUNI arbegin = -1;
    Math::intUNI arend = -1;
    ListDigraph::Node arstartnode = INVALID;
    ListDigraph::Node arendnode = INVALID;

    if (posi < posj) {
	arbegin = posi;
	arend = posj;
	arstartnode = i;
	arendnode = j;
    }

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
    Math::intUNI tmpidx;
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

    for (Math::intUNI l = 0; l < ar.size(); l++) {
	visited.append(false);
    }

    q.clear();
    q.enqueue(arstartnode);

    deltaF.append(arstartnode);
    while (q.size() != 0) {
	curnode = q.dequeue();

	// Check the successors of the current node
	for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
	    tmpnode = pm->graph.target(oait);

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
    Math::intUNI posF = ar.size() - 1;

    // Move elements in deltaF to the right
    while (!deltaF.isEmpty()) {
	// Find the first element in ar starting from posB that is in deltaB
	tmpidx = -1;
	for (Math::intUNI l = posF; l >= 0; l--) {
	    if (deltaF.contains(ar[l])) {
		tmpidx = l;
		break;
	    }
	}

	if (tmpidx == -1) {
	    QTextStream out(stdout);

	    if (j != INVALID && k != INVALID) {

		out << "Moving " << pm->ops[i]->ID << " between " << pm->ops[j]->ID << " and " << pm->ops[k]->ID << endl;
	    }

	    if (j != INVALID && k == INVALID) {

		out << "Moving " << pm->ops[i]->ID << " between " << pm->ops[j]->ID << " and " << " * " << endl;
	    }

	    if (j == INVALID && k != INVALID) {

		out << "Moving " << pm->ops[i]->ID << " between " << " * " << " and " << pm->ops[k]->ID << endl;
	    }

	    out << *pm << endl;
	    Debugger::err << "LocalSearchPM::dynUpdateTopolOrdering : tmpidx = -1 while shifting deltaF. Probably the graph is NOT DAG! " << ENDL;
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
    for (Math::intUNI l = 0; l < ar.size(); l++) {
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
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct after DTO!!!" << ENDL;
		    }
	    }
    }
     */

    //getchar();

    //##########################################################################

}

void LocalSearchPM::setMovableNodes(QMap<ListDigraph::Node, bool>& movableNodes) {
    node2Movable.clear();

    for (QMap < ListDigraph::Node, bool>::iterator iter = movableNodes.begin(); iter != movableNodes.end(); iter++) {
	node2Movable[ListDigraph::id(iter.key())] = iter.value();
    }

}

bool LocalSearchPM::reachable(const ListDigraph::Node& s, const ListDigraph::Node& t) {
    QQueue<ListDigraph::Node> q;
    ListDigraph::Node curnode;

    q.enqueue(t);

    if (s == t) return true;

    if (s == INVALID || t == INVALID) return true;

    while (q.size() > 0) {
	curnode = q.dequeue();

	// Iterate over the predecessors
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    ListDigraph::Node curStartNode = pm->graph.source(iait);
	    if (curStartNode == s) {
		return true;
	    } else {
		if (!q.contains(curStartNode)) {
		    q.enqueue(curStartNode);
		}
	    }
	}
    }

    return false;
}

bool LocalSearchPM::debugCheckPMCorrectness(const QString& location) {
    QTextStream out(stdout);

    out << "LocalSearchPM::debugCheckPMCorrectness : Checking correctness in < " + location + " > ... " << endl;

    totalChecksTimer.start();

    // Check cycles
    if (!dag(pm->graph)) {
	Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Graph is not DAG after scheduling!" << ENDL;
    } else {
	//Debugger::info << "LocalSearchPM::debugCheckPMCorrectness : Graph is DAG." << ENDL;
    }

    //out << "Checked DAG." << endl;
    //getchar();

    // Check the outgoing arcs: every node must have at most one schedule-based outgoing arc
    int noutarcs = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	noutarcs = 0;
	for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
	    if (!pm->conjunctive[oait]) {
		noutarcs++;
	    }

	    if (noutarcs > 1) {
		Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Too many outgoing schedule-based arcs!" << ENDL;
	    }
	}
    }

    //out << "Checked outgoing arcs." << endl;
    //getchar();

    // Check the incoming arcs: every node must have at most one schedule-based outgoing arc
    int ninarcs = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	ninarcs = 0;
	for (ListDigraph::InArcIt iait(pm->graph, nit); iait != INVALID; ++iait) {
	    if (!pm->conjunctive[iait]) {
		ninarcs++;
	    }

	    if (ninarcs > 1) {
		if (pm->ops[nit] != NULL) {
		    out << "LocalSearchPM::debugCheckPMCorrectness : Operation ID : " << pm->ops[nit]->ID << endl;
		}
		Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Too many incoming schedule-based arcs!" << ENDL;
	    }
	}
    }

    //out << "Checked the incoming schedule-based arcs." << endl;
    //getchar();

    // Check whether all nodes can be reached from the start node
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	if (nit != pm->head) {
	    if (!reachable(pm->head, nit)) {
		Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Node is not reachable from the start node!" << ENDL;
	    }
	}
    }

    //out << "Checked reachability from the start node." << endl;
    //getchar();

    // Check correctness of the processing times for the scheduled nodes
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
	    if (pm->ops[nit]->p() != -pm->p[oait]) {
		out << "Operation : " << *(pm->ops[nit]) << endl;
		out << "Arc : " << pm->ops[pm->graph.source(oait)]->ID << " -> " << pm->ops[pm->graph.target(oait)]->ID << " conj: " << ((pm->conjunctive[oait]) ? 1 : 0) << endl;
		out << "Arc length : " << -pm->p[oait] << endl;
		out << *pm << endl;
		Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Outgoing arcs have incorrect length!" << ENDL;
	    }
	}
    }

    //out << "Checked the lengths of the outgoing arcs." << endl;
    //getchar();

    // Check the ready times of the operations
    QList<ListDigraph::Node> ts = pm->topolSort();

    //out << "Got the topological ordering." << endl;
    //getchar();

    double maxr;
    ListDigraph::Node pred;
    for (int i = 0; i < ts.size(); i++) {
	maxr = pm->ops[ts[i]]->r();
	for (ListDigraph::InArcIt iait(pm->graph, ts[i]); iait != INVALID; ++iait) {
	    if (pm->conjunctive[iait]) { // In general this is true only for conjunctive arcs
		pred = pm->graph.source(iait);
		maxr = Math::max(maxr, pm->ops[pred]->r() - pm->p[iait]);

		if (pm->ops[pred]->r() - pm->p[iait] > pm->ops[ts[i]]->r()) {
		    out << "Node : " << pm->ops[ts[i]]->ID << endl;
		    out << "Pred : " << pm->ops[pred]->ID << endl;
		    out << *pm << endl;
		    Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Ready time of the succeeding node < r+p of at least one predecessor!" << ENDL;
		}
	    }
	}

	if (Math::cmp(maxr, pm->ops[ts[i]]->r(), 0.00001) != 0) {
	    out << "Operation : " << pm->ops[ts[i]]->ID << endl;
	    out << "r = " << pm->ops[ts[i]]->r() << endl;
	    out << "max r(prev) = " << maxr << endl;
	    out << *pm << endl;
	    Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Ready time of the succeeding node is too large!" << ENDL;
	}

    }

    // Start time of any operation should be at least as large as the availability time of the corresponding machine
    for (int i = 0; i < ts.size(); i++) {
	ListDigraph::Node curNode = ts[i];

	if (pm->ops[curNode]->s() < pm->ops[curNode]->machAvailTime()) {
	    out << *pm->ops[curNode] << endl;
	    Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Operation start time is less than the availability time of the corresponding machine!" << ENDL;
	}
    }

    // Check the start times of the operations
    double maxc;
    for (int i = 0; i < ts.size(); i++) {
	maxr = 0.0;
	for (ListDigraph::InArcIt iait(pm->graph, ts[i]); iait != INVALID; ++iait) {
	    pred = pm->graph.source(iait);
	    maxc = Math::max(maxc, pm->ops[pred]->c());

	    if (pm->ops[pred]->c() > pm->ops[ts[i]]->s()) {
		out << "Current operation : " << pm->ops[ts[i]]->ID << endl;
		out << "Predecessor : " << pm->ops[pred]->ID << endl;
		out << *pm << endl;
		Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Operation processing overlap!" << ENDL;
	    }
	}
    }

    // Check whether schedule-based arcs always connect operations from the same machine and tool group
    ListDigraph::Node s;
    ListDigraph::Node t;
    for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
	if (!pm->conjunctive[ait]) {
	    s = pm->graph.source(ait);
	    t = pm->graph.target(ait);

	    if (pm->ops[s]->toolID != pm->ops[t]->toolID || pm->ops[s]->machID != pm->ops[t]->machID) {
		Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Schedule-based arc connects incompatible operations!" << ENDL;
	    }
	}
    }

    // Check whether all operations are not assigned
    int nassigned = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	if (pm->ops[nit]->machID > 0) nassigned++;
    }
    if (nassigned == 0) {
	Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : There are no operations assigned to machines!" << ENDL;
    }

    // Check whether operations have correct tool groups assigned


    totalChecksElapsedMS += totalChecksTimer.elapsed();

    out << "LocalSearchPM::debugCheckPMCorrectness : Done checking correctness in < " + location + " > . " << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	Operation& curOp = *pm->ops[nit];
	if (curOp.toolID > 0 && !((*rc)(curOp.toolID)).types.contains(curOp.type)) {
	    out << endl << curOp << endl;
	    out << endl << *rc << endl;
	    Debugger::err << "LocalSearchPM::debugCheckPMCorrectness : Assigned operation to wrong TG!" << ENDL;
	}
    }

    return true;
}

/**  *********************************************************************** **/

/**  ************************  LocalSearchModPM  ****************************** **/

int kDiv = 1;
int kDivMax = 5;

LocalSearchModPM::LocalSearchModPM() {
    pm = NULL;

    //lsmode = IMPROV;

    alpha = 0.1;

    nisteps = 0;

    nodeI = INVALID;
    nodeT = INVALID;

    critNodesUpdateFreq = 100;

    checkCorrectness(true);
}

LocalSearchModPM::LocalSearchModPM(LocalSearchModPM& orig) : IterativeAlg(orig) {

    alpha = orig.alpha;

    nisteps = orig.nisteps;

    nodeI = orig.nodeI;
    nodeT = orig.nodeT;

    critNodesUpdateFreq = orig.critNodesUpdateFreq;

    checkCorrectness(orig._check_correctness);
}

LocalSearchModPM::~LocalSearchModPM() {
    this->pm = NULL;
    this->rc = NULL;
}

void LocalSearchModPM::setPM(ProcessModel *pm) {
    this->pm = pm;

    // Mark all nodes as movable by default
    node2Movable.clear();
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	ListDigraph::Node curNode = nit;

	node2Movable[curNode] = true;
    }

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

void LocalSearchModPM::setResources(Resources *rc) {
    this->rc = rc;

    // #########################    DEBUG    ###################################    
    /*
    out << "Check reachability for every machine during the LS initialization (having set the resources)..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;

    out << "Check whether the processing times of the operations are OK..." << endl;
    double pt;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    if (pm->ops[nit]->machID >= 0) {
		    pt = ((*this->rc)(pm->ops[nit]->toolID, pm->ops[nit]->machID)).procTime(pm->ops[nit]);

		    if (pm->ops[nit]->p() != pt) {
			    out << *pm << endl;
			    out << "pt = " << pt << endl;
			    out << "p = " << pm->ops[nit]->p() << endl;
			    Debugger::err << "Something is wrong with the processing time for " << pm->ops[nit]->ID << ENDL;
		    }
	    }
    }
    out << "The processing times of the operations are OK." << endl;
     */
    // #########################################################################
}

void LocalSearchModPM::init() {
    QTextStream out(stdout);

    IterativeAlg::init();

    if (pm == NULL) {
	Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with NULL process model!" << ENDL;
    }

    // Preserve the state of the schedule
    pm->save();

    // Get the terminal nodes
    QList<ListDigraph::Node> terminals;
    terminals = pm->terminals();

    // Find the initial topological ordering of the nodes in the graph
    topolOrdering = pm->topolSort();
    topolITStart = 0;

    //pm->updateHeads(topolOrdering);
    //pm->updateStartTimes(topolOrdering);
    pm->updateHeadsAndStartTimes(topolOrdering);

    TWT obj;

    curobj = obj(*pm, terminals);
    prevobj = curobj;
    bestobj = curobj;

    out << "LS (init) : bestobj = " << curobj << endl;

    alpha = 0.1;

    nisteps = 0;

    criticalNodes.clear();

    updateCriticalNodes();

    nodeI = INVALID;
    nodeT = INVALID;

    //########################  DEBUG  #########################################
    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency during the initialization..." << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchModPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */

    topSortElapsedMS = 0;
    objElapsedMS = 0;
    updateEvalElapsedMS = 0;
    totalElapsedMS = 0;
    opSelectionElapsedMS = 0;
    potentialPositionsSelectionElapsedMS = 0;
    posSelectionElapsedMS = 0;
    opMoveElapsedMS = 0;
    opMoveBackElapsedMS = 0;
    blocksExecElapsedMS = 0;
    opMovePossibleElapsedMS = 0;
    dynTopSortElapsedMS = 0;
    updateCritNodesElapsedMS = 0;
    longestPathsElapsedMS = 0;

    totalChecksElapsedMS = 0;

    totalTimer.start();
    //##########################################################################
}

void LocalSearchModPM::stepActions() {
    //if (iter() % 1 == 0) out << "iter : " << iter() << endl;

    blocksExecTimer.start();

    //if (Rand::rndDouble() < 1.99999) {
    if (Rand::rnd<double>() < 1.99999) {
	transitionPM();
    } else {
	transitionCP();
    }

    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

void LocalSearchModPM::assessActions() {
    blocksExecTimer.start();
    //out << "Assessing the step of the LS..." << endl;

    // Update the ready times and the start times of the operations in the graph
    //pm->updateHeads();
    //pm->updateStartTimes();

    updateEval(nodeI, nodeT);

    objTimer.start();
    curobj = obj(*pm, pm->terminals());
    objElapsedMS += objTimer.elapsed();


    //out << "Done assessing the step of the LS." << endl;
    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

bool LocalSearchModPM::acceptCondition() {
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

    alpha = Math::exp(-(curobj - prevobj) / (1.0 - Math::pow((double) iter() / double(maxIter()), 1.0))); ///*0.2; //*/0.5 * Math::exp(-((double) iter())); // 1.0 - 0.999 * Math::exp(1.0 - ((double) iter()) / double(_maxiter));
    //alpha = 0.001;

    if (curobj <= prevobj /*bestobj*/) {
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

}

void LocalSearchModPM::acceptActions() {
    QTextStream out(stdout);
    blocksExecTimer.start();
    //out << "accepted." << endl;

    kDiv = 1;

    if (acceptedworse || curobj == bestobj) {
	nisteps++;
    } else {
	if (curobj < bestobj) {
	    nisteps = 0;
	} else {
	    nisteps++;
	}
    }

    if (curobj <= bestobj) {
	if (curobj < bestobj) out << "LSPM (" << iter() << ") : bestobj = " << bestobj << endl;

	bestobj = curobj;

	// Preserve the state of the process model
	pm->save();

	bestobj = curobj;

	//if (curobj < bestobj) updateCriticalNodes(); // Notice : updating critical nodes every time a better solution has been found is REALLY EXPENSIVE.


    }

    prevobj = curobj;


    //out << "The step has been accepted." << endl;
    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

void LocalSearchModPM::declineActions() {
    blocksExecTimer.start();
    //out << "Declining the iteration..." << endl;

    moveBackOper(optomove);
    optomove = INVALID;

    // Restore the previous ready times and start times of the operations
    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Graph after moving BACK the operation " << *pm << endl;
    //out << "PM after restoring : " << *pm << endl;


    //out << "Done declining." << endl;


    // Preform diversification

    //if (iter() > 250000) {
    //	alpha = Math::min(alpha + 0.000001, 0.1);
    //out << "Alpha = " << alpha << endl;
    //}

    nisteps++;

    if (nisteps > 1000) {
	kDiv = kDiv % kDivMax + 1;
    }

    if (nisteps > 2000) {
	//out << "Diversifying (nisteps)..." << endl;
	diversify();
    }

    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

bool LocalSearchModPM::stopCondition() {
    return (curobj <= obj.LB(*pm)) || IterativeAlg::stopCondition();
}

void LocalSearchModPM::stopActions() {
    Debugger::info << "LocalSearchModPM::stopActions : Found local optimum with objective " << bestobj << ENDL;
    //getchar();
}

void LocalSearchModPM::preprocessingActions() {
    // Check correctness of the PM right before the processing
    //out << "LocalSearchModPM::preprocessingActions : Checking PM correctness..." << endl;
    if (_check_correctness) {
	debugCheckPMCorrectness("LocalSearchModPM::preprocessingActions");
    }
    //out << "LocalSearchModPM::preprocessingActions : PM is correct." << endl;
}

void LocalSearchModPM::postprocessingActions() {
    QTextStream out(stdout);
    // Restore the state corresponding to the best found value of the objective
    pm->restore();


    //out << "PM : " << endl << *pm << endl;

    // Check the correctness of the process model
    if (_check_correctness) {
	debugCheckPMCorrectness("LocalSearchModPM::postprocessingActions");
    }


    out << "                  " << endl;


    totalElapsedMS = totalTimer.elapsed();


    out << "Time (ms) for objective estimations : " << objElapsedMS << endl;
    out << "Time (ms) for topological sorting : " << topSortElapsedMS << endl;
    out << "Time (ms) for running the update evaluations : " << updateEvalElapsedMS << endl;
    out << "Time (ms) for selecting operations : " << opSelectionElapsedMS << endl;
    out << "Time (ms) for finding potential moves : " << potentialPositionsSelectionElapsedMS << endl;
    out << "Time (ms) for selecting the insert position : " << posSelectionElapsedMS << endl;
    out << "Time (ms) for moving operation : " << opMoveElapsedMS << endl;
    out << "Time (ms) for moving back operation : " << opMoveBackElapsedMS << endl;
    out << "Time (ms) for estimating feasibility of the moves : " << opMovePossibleElapsedMS << endl;
    out << "Time (ms) for DTO : " << dynTopSortElapsedMS << endl;
    out << "Time (ms) for updating critical nodes : " << updateCritNodesElapsedMS << endl;
    out << "Time (ms) for calculating the longest paths : " << longestPathsElapsedMS << endl;

    out << " -----------------" << endl;
    out << "Time (ms) for executing blocks (1) : " << objElapsedMS +
	    updateEvalElapsedMS + opSelectionElapsedMS + potentialPositionsSelectionElapsedMS + posSelectionElapsedMS +
	    opMoveElapsedMS + opMoveBackElapsedMS << endl;
    out << "Time (ms) for executing blocks (2) : " << blocksExecElapsedMS << endl;

    out << " -----------------" << endl;
    out << "Time (ms) for running the algorithm : " << totalElapsedMS << endl;
    out << " -----------------" << endl;

    out << "Time percentage for correctness checks : " << double(totalChecksElapsedMS) / double(totalElapsedMS) << endl;
    out << " -----------------" << endl;

}

void LocalSearchModPM::transitionPM() {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Get the terminal nodes of the graph.
     * 2. Select the most critical in some sense terminal node.
     * 3. Find a critical path to this terminal.
     * 4. Select an operation from the critical path
     * 5. Select a machine which the operation has to be moved to.
     * 6. Try to move the operation to the selected machine
     */

    // Get the terminals
    //QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath; // Critical path

    //pm->updateHeads();
    //pm->updateStartTimes();

    /*** DEBUG PURPOUSES ONLY ***/
    // Iterate over all terminals an estimate the number of schedule-based arcs in the longest paths

    /*
    int sbarcs = 0;
    for (int i = 0; i < terminals.size(); i++) {
	    cpath = longestPath(terminals[i]);
	    for (int n = 0; n < cpath.length(); n++) {
		    if (!pm->conjunctive[cpath.nth(n)]) {
			    sbarcs++;
		    }
	    }
    }

    // Debugger::info << "Number of sbarcs on crit. paths : " << sbarcs << ENDL;

    //sbarcs = 0;
    //for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
    //	if (!pm->conjunctive[ait]) sbarcs++;
    //   }
    //Debugger::info << "Number of sbarcs totally : " << sbarcs << ENDL;

    // Check multiple schedule-based arcs

    int nmultiplesb = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

	    nmultiplesb = 0;
	    for (ListDigraph::InArcIt iait(pm->graph, nit); iait != INVALID; ++iait) {
		    if (!pm->conjunctive[iait]) nmultiplesb++;
	    }
	    if (nmultiplesb > 1) {
		    out << *pm << endl;
		    Debugger::err << "Too many incoming sb arcs!!! " << pm->ops[nit]->ID << ENDL;
	    }

	    nmultiplesb = 0;
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (!pm->conjunctive[oait]) nmultiplesb++;
	    }
	    if (nmultiplesb > 1) {
		    out << *pm << endl;
		    Debugger::err << "Too many outgoing sb arcs!!! " << pm->ops[nit]->ID << ENDL;
	    }
    }

     */

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency before the step..." << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchModPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */
    /******/

    opSelectionTimer.start();

    // Update the set of critical nodes
    //out << "Updating critical nodes..." << endl;
    //int critNodesUpdateFreq = 100;
    if (iter() % critNodesUpdateFreq == 0) {
	//pm->updateHeads(topolOrdering);
	//pm->updateStartTimes(topolOrdering);
	pm->updateHeadsAndStartTimes(topolOrdering);
	updateCriticalNodes();
	if (criticalNodes.size() == 0) {
	    pm->updateHeads();
	    pm->updateStartTimes();
	    out << *pm << endl;
	    out << "TWT of the partial schedule : " << TWT()(*pm) << endl;
	    Debugger::err << "LocalSearchModPM::transitionPM : Failed to find critical nodes!!!" << endl;
	}
    }
    //out << "Updating critical nodes..." << endl;

    //optomove = criticalNodes[Rand::rndInt(0, criticalNodes.size() - 1)];
    optomove = criticalNodes[Rand::rnd<Math::uint32>(0, criticalNodes.size() - 1)];


    // Select a terminal
    //if (nisteps <= 100) {
    //theterminal = selectTerminalContrib(terminals);
    //} else {
    //    theterminal = selectTerminalContrib(terminals);
    //}

    // Find a critical path to the selected terminal

    //    cpath = longestPath(theterminal);

    // Select an operation to move
    //Debugger::iDebug("Selecting operation to move...");

    //    optomove = defaultSelectOperToMove(cpath);

    //Debugger::iDebug("Selected operation to move.");

    opSelectionElapsedMS += opSelectionTimer.elapsed();

    //QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs;

    //QPair<ListDigraph::Node, ListDigraph::Node> atb;

    //out << "Searching arc to break..." << endl;
    /*
    do {
	    // Select a random terminal
	    theterminal = selectTerminalNonContrib(terminals);

	    // Select a random path to the terminal
	    ncpath = randomPath(theterminal);

	    // Select relevant pairs of nodes
	    //out << "Selecting relevant arcs..." << endl;
	    relarcs = selectRelevantArcsFromPath(ncpath, optomove);
	    //out << "Selected relevant arcs. " << relarcs.size() << endl;
	    if (relarcs.size() > 0) {
		    // Select insert positions
		    //out << "Selecting arc to break..." << endl;
		    atb = selectArcToBreak(relarcs, optomove);
		    //out << "Selected arc to break." << endl;
	    }

    } while (relarcs.size() == 0 || (atb.first == INVALID && atb.second == INVALID));
     */
    //out << "Found arc to break." << endl;

    /*
     if (atb.first != INVALID) {
	     out << pm->ops[atb.first]->ID << " -> ";
     } else {
	     out << "INV. ->";
     }

     if (atb.second != INVALID) {
	     out << pm->ops[atb.second]->ID << endl;
     }
     */

    int targetMachID = selectTargetMach(optomove);

    // Select candidate arcs to break
    potentialPositionsSelectionTimer.start();
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs = selectBreakableArcs(targetMachID);
    potentialPositionsSelectionElapsedMS += potentialPositionsSelectionTimer.elapsed();

    //out << endl << endl;

    //    do {
    // Select an arc to break
    //out << "Selecting arc to break... " << endl;
    posSelectionTimer.start();
    QPair<ListDigraph::Node, ListDigraph::Node> atb = /*selectBestArcToBreak(targetMachID, relarcs, optomove); //*/selectArcToBreak(relarcs, optomove);
    posSelectionElapsedMS += posSelectionTimer.elapsed();
    //out << "Selected arc to break." << endl;

    //out << "Selected operation : " << pm->ops[optomove]->ID << endl;
    //out << "Arc to break : (" << pm->ops[pm->graph.source(arc)]->ID << "," << pm->ops[pm->graph.target(arc)]->ID << ")" << endl;
    //out << *pm << endl;



    // Reverse the selected arc
    //out << "Graph before reversing an arc:" << endl;
    //out << *pm << endl;
    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Reversing arc ..." << endl;
    //out << pm->ops[pm->graph.source(carc)]->OID << ":" << pm->ops[pm->graph.source(carc)]->ID << " -> " << pm->ops[pm->graph.target(carc)]->OID << ":" << pm->ops[pm->graph.target(carc)]->ID << endl;


    //	    if (!dag(pm->graph)) {
    //		Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains cycles before moving an operation!!!");
    //	    } else {
    //		//Debugger::info<<"The resulting graph is DAG before reverting a critical arc!!!"<<ENDL;
    //	    }

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

    // Find the best possible move of the selected operation
    //out << "Searching for the best move option..." << endl;
    //findBestOperMove(optomove, targetMachID, atb);
    //out << "Found the best move option." << endl;

    //if (!dag(pm->graph)) {
    //Debugger::err << "Graph contains cycles before the operation move!!!" << ENDL;
    //}

    //out << "Critical arc: (" << pm->ops[pm->graph.source(carc)]->ID << " ; " << pm->ops[pm->graph.target(carc)]->ID << ")" << endl;
    //    out << " Moving operation with ID = " << pm->ops[optomove]->ID << endl;
    //getchar();
    //    out << " Between operations : " << ((atb.first != INVALID) ? pm->ops[atb.first]->ID : -1) << " and " << ((atb.second != INVALID) ? pm->ops[atb.second]->ID : -1) << endl;
    //out << "Graph before moving an operation: " << endl;
    //out << *pm << endl;
    //getchar();

    // Move the operation breaking the selected arc

    /*
    out << "Check reachability for every machine before moving the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    // ##### IMPORTANT !!! ##### Preserve the states of the operation BEFORE the move

    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Graph before moving the operation ..." << *pm << endl;
    //out << "Moving operation..." << endl;
    moveOper(targetMachID, atb.first, atb.second, optomove);
    //out << "Moved operation " << pm->ops[optomove]->ID << endl;

    /*
    out << "Check reachability for every machine after moving the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    //if (!dag(pm->graph)) moveBackOper(optomove);
    //	else {
    //		out << "Graph after the move:" << endl;
    //		out << *pm << endl;
    //	    break;
    //	}

    /*
    if (!dag(pm->graph)) {
	    out << *pm << endl;
	    Debugger::err << "Graph contains cycles after the operation move!!!" << ENDL;
    }
     */

    //out << " Moved operation with ID = " << pm->ops[optomove]->ID << endl;
    //out << *pm << endl;
    //getchar();
    //out << "Done moving operation." << endl;

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency after the step..." << endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchModPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */

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
    //	    if (!dag(pm->graph)) {
    //		out << " Moved operation with ID = " << pm->ops[optomove]->ID << endl;
    //		out << " Between operations : " << ((atb.first != INVALID) ? pm->ops[atb.first]->ID : -1) << " and " << ((atb.second != INVALID) ? pm->ops[atb.second]->ID : -1) << endl;
    //		out << *pm;
    //		Debugger::eDebug("LocalSearch::stepActions : The resulting graph contains cycles after moving the operation!!!");
    //	    } else {
    //		// Debugger::info << "The resulting graph is acyclic :)." << ENDL;
    //	    }

    //out << "Done reversing arc." << endl;

    //out << "Graph after reversing an arc:" << endl;
    //out << *pm << endl;

    //	} else {
    //	    Debugger::err << "LocalSearch::stepActions : Invalid arc to break!" << ENDL;


    //    } while (true);
    //out << "Finished one step of the local search." << endl;

}

void LocalSearchModPM::selectOperToMoveCP(const Path<ListDigraph> &cpath, ListDigraph::Node &optomove, QPair<ListDigraph::Node, ListDigraph::Node> &atb) {
    // Find critical arcs on the critical path
    QList<ListDigraph::Arc> carcs;
    ListDigraph::Arc curarc;


    for (int i = 0; i < cpath.length(); i++) {
	curarc = cpath.nth(i);
	if (!pm->conjunctive[curarc] && !pm->conPathExists(pm->graph.source(curarc), pm->graph.target(curarc))) {
	    carcs.append(curarc);
	}
    }

    if (carcs.size() == 0) { // There are no reversible arcs on the path

	//optomove = INVALID;
	//return;

	//curarc = cpath.nth(Rand::rndInt(0, cpath.length() - 1));
	curarc = cpath.nth(Rand::rnd<Math::uint32>(0, cpath.length() - 1));
	optomove = pm->graph.source(curarc);
	atb.first = pm->graph.source(curarc);
	atb.second = pm->graph.target(curarc);

	return;
    }

    // Select randomly some critical arc
    //curarc = carcs[Rand::rndInt(0, carcs.size() - 1)];
    curarc = carcs[Rand::rnd<Math::uint32>(0, carcs.size() - 1)];

    optomove = pm->graph.source(curarc);

    atb.first = pm->graph.target(curarc);

    ListDigraph::Arc arc = INVALID;
    //bool sbarcfound = false;
    bool outarcexists = false;
    // Search schedule-based outgoing arcs
    for (ListDigraph::OutArcIt oait(pm->graph, atb.first); oait != INVALID; ++oait) {
	if (!pm->conjunctive[oait]) {
	    outarcexists = true;

	    arc = oait;

	    if (!pm->conPathExists(pm->graph.source(oait), pm->graph.target(oait))) {
		//sbarcfound = true;
		break;
	    }
	}
    }

    if (!outarcexists) {
	atb.second = INVALID;
	return;
    } else {
	atb.second = pm->graph.target(arc);
    }

}

void LocalSearchModPM::transitionCP() {
    /** Algorithm:
     * 
     * 1. Get the terminal nodes of the graph.
     * 2. Select the most critical in some sense terminal node.
     * 3. Find a critical path to this terminal.
     * 4. Select an operation from the critical path
     * 5. Select a machine which the operation has to be moved to.
     * 6. Try to move the operation to the selected machine
     */

    // Get the terminals
    //QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath; // Critical path
    QPair<ListDigraph::Node, ListDigraph::Node> atb;

    opSelectionTimer.start();

    QList<ListDigraph::Node> terminals = pm->terminals();

    do {

	// Select a terminal
	theterminal = selectTerminalContrib(terminals);

	// Find a critical path to the selected terminal
	cpath = longestPath(theterminal);

	//Debugger::iDebug("Selecting operation to move...");
	selectOperToMoveCP(cpath, optomove, atb);
	//Debugger::iDebug("Selected operation to move.");


    } while (optomove == INVALID);

    //Debugger::iDebug("Selected operation to move.");
    opSelectionElapsedMS += opSelectionTimer.elapsed();


    //QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs;

    //QPair<ListDigraph::Node, ListDigraph::Node> atb;

    int targetMachID = pm->ops[optomove]->machID;

    //out << "Graph before moving the operation ..." << *pm << endl;
    //out << "Moving operation..." << endl;

    moveOper(targetMachID, atb.first, atb.second, optomove);
    //out << "Moved operation " << pm->ops[optomove]->ID << endl;

    /*
    if (!dag(pm->graph)) {
	    Debugger::err << "LocalSearchModPM::transitionCP : Graph not DAG!!!" << ENDL;
    }
     */

}

Path<ListDigraph> LocalSearchModPM::longestPath(const ListDigraph::Node & node) {
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

QList<Path<ListDigraph> > LocalSearchModPM::longestPaths(const QList<ListDigraph::Node> &nodes) {
    longestPathsTimer.start();

    QList<Path<ListDigraph> > res;

    BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm->graph, pm->p);

    bf.init();
    bf.addSource(pm->head);
    //Debugger::info << "Running the BF algorithm..."<<ENDL;
    bf.start();
    //Debugger::info << "Done running the BF algorithm."<<ENDL;

#ifdef DEBUG
    for (int i = 0; i < nodes.size(); i++) {
	if (!bf.reached(nodes[i])) {
	    Debugger::err << "LocalSearch::longestPath : Operation ID= " << pm->ops[nodes[i]]->OID << ":" << pm->ops[nodes[i]]->ID << " can not be reached from the root node " << pm->ops[pm->head]->OID << ":" << pm->ops[pm->head]->ID << "!" << ENDL;
	}
    }
#endif
    for (int i = 0; i < nodes.size(); i++) {
	res.append(bf.path(nodes[i]));
    }

    longestPathsElapsedMS += longestPathsTimer.elapsed();

    return res;
}

Path<ListDigraph> LocalSearchModPM::randomPath(const ListDigraph::Node & node) {
    Path<ListDigraph> res;
    ListDigraph::Node curnode = node;
    QList<ListDigraph::InArcIt> inarcs;

    while (curnode != pm->head) {
	// Select the outgoing arcs from the current node
	inarcs.clear();
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    inarcs.append(iait);
	}

	// Add a random arc to the result
	//res.addFront(inarcs.at(Rand::rndInt(0, inarcs.size() - 1)));
	res.addFront(inarcs.at(Rand::rnd<Math::uint32>(0, inarcs.size() - 1)));

	// Proceed to the next node
	curnode = pm->graph.source(res.front());
    }

    return res;
}

void LocalSearchModPM::updateCriticalNodes() {
    /** Algorithm:
     * 
     * 1. Select the terminal nodes contributing to the optimization criterion
     * 2. Calculate the longest paths to these terminals
     * 3. Select all operations lying on the critical paths
     * 
     */

    QList<ListDigraph::Node> terminals;

    terminals = pm->terminals();

    updateCritNodesTimer.start();

    ListDigraph::Arc ait = INVALID;
    QList<Path<ListDigraph> > cpaths; // Critical paths
    Path<ListDigraph> cpath; // Critical path
    ListDigraph::Node curnode = INVALID;

    criticalNodes.clear();

    // Calculate the longest paths to the terminal nodes only once -
    cpaths = longestPaths(terminals);

    //criticalNodes.reserve();
    int termContrib = 0;
    for (int i = 0; i < terminals.size(); i++) {
	if (pm->ops[terminals[i]]->wT() > 0.0) { // The terminal is contributing

	    termContrib++;

	    // Find the critical path to the terminal
	    cpath = cpaths[i];

	    for (int j = 0; j < cpath.length(); j++) {
		ait = cpath.nth(j);

		curnode = pm->graph.source(ait);
		if (/*criticalNodes.count(curnode) == 0 &&*/ pm->ops[curnode]->machID > 0 /*&& (pm->ops[pm->graph.source(ait)]->d() - pm->ops[pm->graph.source(ait)]->c() < 0.0)*/) {
		    if (node2Movable[curnode]) criticalNodes.append(curnode);
		}

	    }

	    // Add the target node (here ait represents the last arc of the current critical path)
	    curnode = pm->graph.target(ait);
	    if (/*criticalNodes.count(curnode) == 0 &&*/ pm->ops[curnode]->machID > 0 /*&& (pm->ops[pm->graph.target(ait)]->d() - pm->ops[pm->graph.target(ait)]->c() < 0.0)*/) {
		if (node2Movable[curnode]) criticalNodes.append(curnode);
	    }
	}
    }

    if (criticalNodes.size() == 0) { // No movable critical nodes have been found -> set all movable nodes as critical (the algorithm will try to move them)

	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

	    ListDigraph::Node curNode = nit;

	    if (node2Movable[curNode]) criticalNodes.append(curNode);

	}

    }

    if (termContrib == 0) { // This should not happen since the algorithm must catch this situation

	pm->updateHeads();
	pm->updateStartTimes();
	QTextStream out(stdout);
	out << "Current TWT of the partial schedule : " << TWT()(*pm) << endl;
	Debugger::err << "LocalSearchModPM::updateCriticalNodes : No contributing terminals!!!" << endl;

    }

    updateCritNodesElapsedMS += updateCritNodesTimer.elapsed();

}

ListDigraph::Node LocalSearchModPM::selectOperToMove(const Path<ListDigraph> &cpath) {
    // Select only those arcs which are schedule based and NOT conjunctive

    int n = cpath.length();

    QList<ListDigraph::Node> nodes;
    ListDigraph::Node res;

    QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs
    for (int i = 0; i < n; i++) {
	if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based (and therefore the operation is already assigned)

	    schedbased.append(cpath.nth(i));
	    ListDigraph::Arc curArc = cpath.nth(i);

	    ListDigraph::Node curStartNode = pm->graph.source(curArc);
	    ListDigraph::Node curEndNode = pm->graph.target(curArc);

	    if (nodes.count(curStartNode) == 0 && pm->ops[curStartNode]->machID > 0 && node2Movable[curStartNode]) {
		nodes.append(curStartNode);
	    }

	    if (nodes.count(curEndNode) == 0 && pm->ops[curEndNode]->machID > 0 && node2Movable[curEndNode]) {
		nodes.append(curEndNode);
	    }

	} else {
	    /*
	    if (pm->ops[pm->graph.source(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.source(cpath.nth(i))) == 0) {
		    nodes.append(pm->graph.source(cpath.nth(i)));
	    }

	    if (pm->ops[pm->graph.target(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.target(cpath.nth(i))) == 0) {
		    nodes.append(pm->graph.target(cpath.nth(i)));
	    }
	     */
	}

    }

    if (schedbased.size() == 0) {
	return INVALID;
	/*
	for (int i = 0; i < n; i++) {
		if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based (and therefore the operation is already assigned)
		} else {
			if (pm->ops[pm->graph.source(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.source(cpath.nth(i))) == 0) {
				nodes.append(pm->graph.source(cpath.nth(i)));
			}

			if (pm->ops[pm->graph.target(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.target(cpath.nth(i))) == 0) {
				nodes.append(pm->graph.target(cpath.nth(i)));
			}
		}

	}
	 */

    }

    //do {
    //res = nodes[Rand::rndInt(0, nodes.size() - 1)];
    res = nodes[Rand::rnd<Math::uint32>(0, nodes.size() - 1)];
    //} while (!scheduledtgs.contains(pm->ops[res]->toolID));

    return res;
}

ListDigraph::Node LocalSearchModPM::defaultSelectOperToMove(const Path<ListDigraph> &cpath) {
    int n = cpath.length();

    QList<ListDigraph::Node> nodes;
    ListDigraph::Node res;

    QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs

    //for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
    ListDigraph::Arc ait;
    for (int i = 0; i < n; i++) {
	ait = cpath.nth(i);

	ListDigraph::Node curStartNode = pm->graph.source(ait);
	ListDigraph::Node curEndNode = pm->graph.target(ait);

	if (nodes.count(curStartNode) == 0 && pm->ops[curStartNode]->machID > 0 && node2Movable[curStartNode]) {
	    nodes.append(curStartNode);
	}
	if (nodes.count(curEndNode) == 0 && pm->ops[curEndNode]->machID > 0 && node2Movable[curEndNode]) {
	    nodes.append(curEndNode);
	}

    }

    if (nodes.size() == 0) return INVALID;

    /*
    // Select the operation which has a high tardiness
    QMultiMap<double, ListDigraph::Node> tdns2node;

    for (int i = 0; i < nodes.size(); i++) {
	    tdns2node.insert(pm->ops[nodes[i]]->wT(), nodes[i]);
    }
    int k = Rand::rndInt(1, tdns2node.size() / 2);
    QMultiMap<double, ListDigraph::Node>::iterator iter = tdns2node.end();
    for (int j = 0; j < k; j++) {
	    iter--;
    }
    return iter.value();
     */

    //do {
    //res = nodes[Rand::rndInt(0, nodes.size() - 1)];
    res = nodes[Rand::rnd<Math::uint32>(0, nodes.size() - 1)];
    //} while (!scheduledtgs.contains(pm->ops[res]->toolID));

    return res;
}

int LocalSearchModPM::selectTargetMach(const ListDigraph::Node& optomove) {

    // Get the list of available in the TG machines which are able to execute the operation type
    QList<Machine*> tgmachines = ((*rc)(pm->ops[optomove]->toolID)).machines(pm->ops[optomove]->type);

    /*
    // For every machine calculate the total processing time of operations assigned to it
    QHash<int, double> machid2crit;
    Operation* curop;

    // Insert all machines of the tool group
    for (int i = 0; i < tgmachines.size(); i++) {
	    machid2crit[tgmachines[i]->ID] = tgmachines[i]->procTime(pm->ops[optomove]);
    }

    // Calculate the CTs
    for (int i = 0; i < topolOrdering.size(); i++) {
	    curop = pm->ops[topolOrdering[i]];
	    if (curop->toolID == pm->ops[optomove]->toolID && curop->machID > 0) {
		    //machid2crit[curop->machID] += curop->p(); // = Math::max(machid2crit[curop->machID], curop->w() * curop->c());
	    }
    }

    // Find the machine with the smallest WIP
    double curWIP = Math::MAX_DOUBLE;
    QList<int> machIDs;
    int machID = -1;
    for (QHash<int, double>::iterator iter = machid2crit.begin(); iter != machid2crit.end(); iter++) {
	    if (iter.value() <= curWIP) {
		    machIDs.prepend(iter.key());
		    curWIP = iter.value();
	    }
    }

    while (machIDs.size() > 3) {
	    machIDs.removeLast();
    }

    machID = machIDs[Rand::rndInt(0, machIDs.size() - 1)];

    if (machID == -1) {
	    Debugger::err << "LocalSearchModPM::selectTargetMach : Failed to find the target machine!" << ENDL;
    }

    // Return an arbitrary machine from the tool group
    //if (iter() % 1 == 0) {
    //return tgmachines[Rand::rndInt(0, tgmachines.size() - 1)]->ID;
    //}
     */
    //return /*machID; //pm->ops[optomove]->machID; //*/ tgmachines[Rand::rndInt(0, tgmachines.size() - 1)]->ID;
    return /*machID; //pm->ops[optomove]->machID; //*/ tgmachines[Rand::rnd<Math::uint32>(0, tgmachines.size() - 1)]->ID;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchModPM::selectRelevantArcs(const Path<ListDigraph>& /*cpath*/, const ListDigraph::Node& node) {
    /** Algorithm:
     * 
     * 1. Iterate over all arcs in the graph
     * 1.1. If the source node of the current arc can be processed by the same tool group
     *	    then add the arc to the list
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    ListDigraph::Node j;
    ListDigraph::Node k;
    bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine

    for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
	//for (int i = 0; i < cpath.length(); i++) {
	//ListDigraph::Arc ait = cpath.nth(i);

	if (!pm->conjunctive[ait]) {

	    if (pm->ops[node]->toolID == pm->ops[pm->graph.source(ait)]->toolID) { // This arc is relevant
		j = pm->graph.source(ait);
		k = pm->graph.target(ait);
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, k));

		// Check whether the arc corresponds to the first two or the last two operations on the machine
		fol = true;
		for (ListDigraph::InArcIt iait(pm->graph, j); iait != INVALID; ++iait) {
		    if (!pm->conjunctive[iait]) {
			fol = false;
			break;
		    }
		}

		if (fol) {
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, j));
		}

		fol = true;
		for (ListDigraph::OutArcIt oait(pm->graph, k); oait != INVALID; ++oait) {
		    if (!pm->conjunctive[oait]) {
			fol = false;
			break;
		    }
		}

		if (fol) {
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (k, INVALID));
		}

	    }
	}
    }

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchModPM::selectRelevantArcsNew(const Path<ListDigraph>& /*cpath*/, const ListDigraph::Node& node) {
    QTextStream out(stdout);
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    ListDigraph::Node j;
    ListDigraph::Node k;
    //bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine

    QHash<int, QVector<ListDigraph::Node> > machid2node;
    QQueue<ListDigraph::Node> q;
    ListDigraph::Node curnode;
    ListDigraph::NodeMap<bool> scheduled(pm->graph, false);
    ListDigraph::NodeMap<bool> available(pm->graph, false);
    //bool predchecked = false;

    ListDigraph::Node suc;
    ListDigraph::Node sucpred;

    q.enqueue(pm->head);
    scheduled[pm->head] = false;
    available[pm->head] = true;

    // Collect operation sequences on every machine of the tool group
    while (q.size() > 0) {
	curnode = q.dequeue();

	if (available[curnode] && !scheduled[curnode]) {
	    if ((pm->ops[curnode]->ID > 0) && (pm->ops[curnode]->toolID == pm->ops[node]->toolID)) {
		machid2node[pm->ops[curnode]->machID].append(curnode);
	    }

	    scheduled[curnode] = true;

	    // Enqueue the successors
	    for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
		suc = pm->graph.target(oait);
		if (!scheduled[suc]) {

		    // Update availability

		    available[suc] = true;
		    for (ListDigraph::InArcIt iait(pm->graph, suc); iait != INVALID; ++iait) {
			sucpred = pm->graph.source(iait);
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

    for (QHash<int, QVector<ListDigraph::Node> >::iterator iter = machid2node.begin(); iter != machid2node.end(); iter++) {

	//	out << "operations on machines : " << endl;
	//	out << "Mach ID : " << iter.key() << " : ";

	for (int i = 0; i < iter.value().size(); i++) {
	    //	    out << pm->ops[iter.value()[i]]->ID << ",";
	}

	//	out << endl << endl;
	//getchar();

	for (int i = 0; i < iter.value().size() - 1; i++) {
	    //for (ListDigraph::OutArcIt oait(pm->graph, iter.value()[i]); oait != INVALID; ++oait) {
	    //if (pm->graph.target(oait) == iter.value()[i + 1] /*&& !pm->conjunctive[oait]*/) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().at(i), iter.value().at(i + 1)));
	    //}
	    //}
	}


	if (iter.value().size() > 0) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, iter.value().first()));
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().last(), INVALID));
	}

    }

    //out << "GBM:" << endl;
    //out << *pm << endl;

    for (int i = 0; i < res.size(); i++) {
	//	out << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	if (!reachable(res[i].first, res[i].second)) {
	    out << "Not reachable : " << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	    //out << *pm << endl;
	    getchar();
	}
    }

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchModPM::selectBreakableArcs(const int& mid) {
    /*
    out << "Selecting breakable arcs..." << endl;
    out << "MID : " << mid << endl;
    out << "Scheduled TGs : " << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    out << scheduledtgs[i] << ",";
    }
    out << endl;
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    QList<ListDigraph::Node> trgmachnodes; // Nodes currently on the target machine
    ListDigraph::Node curnode;
    int n = topolOrdering.size();
    QVector<ListDigraph::Node> tord;
    /*
    QList<ListDigraph::Node> testtrgmachnodes; // Nodes currently on the target machine
    QQueue<ListDigraph::Node> q;
    ListDigraph::NodeMap<bool> scheduled(pm->graph, false);
    ListDigraph::NodeMap<bool> available(pm->graph, false);

    ListDigraph::Node suc;
    ListDigraph::Node sucpred;

    q.enqueue(pm->head);
    scheduled[pm->head] = false;
    available[pm->head] = true;
     */

    // Collect operation sequences on the target machine
    /*
    while (q.size() > 0) {
	    curnode = q.dequeue();

	    if (available[curnode] && !scheduled[curnode]) {
		    if ((pm->ops[curnode]->ID > 0) && (pm->ops[curnode]->machID == mid)) {
			    trgmachnodes.append(curnode);
		    }

		    scheduled[curnode] = true;

		    // Enqueue the successors
		    for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
			    suc = pm->graph.target(oait);
			    if (!scheduled[suc]) {

				    // Update availability

				    available[suc] = true;
				    for (ListDigraph::InArcIt iait(pm->graph, suc); iait != INVALID; ++iait) {
					    sucpred = pm->graph.source(iait);
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
     */

    tord = topolOrdering.toVector();

    trgmachnodes.clear();
    trgmachnodes.reserve(topolOrdering.size());

    for (int i = 0; i < n; i++) {
	curnode = tord[i]; //topolOrdering[i];
	if (pm->ops[curnode]->machID == mid) {
	    trgmachnodes.append(curnode);
	}
    }

    //#######################  DEBUG  ##########################################
    /*
    if (testtrgmachnodes.size() != trgmachnodes.size()) {
	    Debugger::err << "Wrong nodes on the target machine!!!" << ENDL;
    }

    for (int i = 0; i < testtrgmachnodes.size(); i++) {
	    if (!trgmachnodes.contains(testtrgmachnodes[i])) {
		    Debugger::err << "Wrong nodes on the target machine (elements test)!!!" << ENDL;
	    }
    }
     */
    //##########################################################################

    res.clear();
    res.reserve(trgmachnodes.size() + 2);

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
	    out << pm->ops[trgmachnodes[k]]->ID << ",";
    }

    out << endl << endl;
     */
    //out << "GBM:" << endl;
    //out << *pm << endl;

    // Check reachability

    /*
    for (int j = 0; j < res.size(); j++) {
	    //	out << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	    if (!reachable(res[j].first, res[j].second)) {
		    out << "Not reachable : " << pm->ops[res[j].first]->ID << "->" << pm->ops[res[j].second]->ID << endl;

		    out << "operations on machine " << mid << " : " << endl;
		    for (int k = 0; k < trgmachnodes.size(); k++) {
			    out << pm->ops[trgmachnodes[k]]->ID << ",";
		    }

		    out << endl << endl;

		    out << *pm << endl;
		    getchar();
	    }
    }
     */
    // #############################################################################

    //out << "Done selecting breakable arcs." << endl;

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchModPM::selectRelevantArcsFromPath(const Path<ListDigraph> &path, const ListDigraph::Node& node) {
    /** Algorithm:
     * 
     * 1. Iterate over all arcs in the path
     * 2. For every machine from the needed tool group collect the operations to be processed on it
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res1;

    ListDigraph::Node j;
    ListDigraph::Node k;
    //bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine
    QMap<int, QVector<ListDigraph::Node> > machid2opers;

    for (int i = 0; i < path.length(); i++) {
	ListDigraph::Arc ait = path.nth(i);
	j = pm->graph.source(ait);

	if (pm->ops[j]->toolID == pm->ops[node]->toolID && pm->ops[j]->machID > 0) {
	    machid2opers[pm->ops[j]->machID].append(j);
	}
    }

    // And the last one
    j = pm->graph.target(path.back());

    if (pm->ops[j]->toolID == pm->ops[node]->toolID && pm->ops[j]->machID > 0) {
	machid2opers[pm->ops[j]->machID].append(j);
    }


    // Build the set of relevant arcs
    for (QMap<int, QVector<ListDigraph::Node> >::iterator iter = machid2opers.begin(); iter != machid2opers.end(); iter++) {

	for (int i = 0; i < iter.value().size() - 1; i++) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().at(i), iter.value().at(i + 1)));
	}

	int prevsize = res.size();

	if (iter.value().size() > 0) {
	    // Check whether insertion can be performed before the first operation
	    j = iter.value().first();
	    for (ListDigraph::InArcIt iait(pm->graph, j); iait != INVALID; ++iait) {
		if (pm->ops[pm->graph.source(iait)]->machID == pm->ops[j]->machID) { // Insertion 
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(iait), j));
		}
	    }
	    if (res.size() == prevsize) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, j));
	    }

	    prevsize = res.size();
	    // Check whether insertion can be performed after the last operation
	    j = iter.value().last();
	    for (ListDigraph::OutArcIt oait(pm->graph, j); oait != INVALID; ++oait) {
		if (pm->ops[pm->graph.target(oait)]->machID == pm->ops[j]->machID) { // Insertion 
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, pm->graph.target(oait)));
		}
	    }
	    if (prevsize == res.size()) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, INVALID));
	    }
	}



    }

    // Check the conjunctive arcs. If the start node has an outgoing or the end node has an incoming schedule-base arc =>
    // remove this arc and insert the other two
    bool incluconj = true;
    bool conj = false;
    ListDigraph::Arc arc;
    for (int i = 0; i < res.size(); i++) {

	if (res[i].first == INVALID || res[i].second == INVALID) {
	    res1.append(res[i]);
	    continue;
	}

	conj = false;
	for (ListDigraph::OutArcIt oait(pm->graph, res[i].first); oait != INVALID; ++oait) {
	    if (pm->graph.target(oait) == res[i].second) {
		conj = pm->conjunctive[oait];
		break;
	    }
	}

	if (conj) {
	    incluconj = true;
	    // Search the outgoing arcs for the start node
	    for (ListDigraph::OutArcIt oait(pm->graph, res[i].first); oait != INVALID; ++oait) {
		if (!pm->conjunctive[oait]) {
		    res1.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(oait), pm->graph.target(oait)));
		    incluconj = false;
		    break;
		}
	    }

	    // Search the incoming arcs for the end node
	    for (ListDigraph::InArcIt iait(pm->graph, res[i].second); iait != INVALID; ++iait) {
		if (!pm->conjunctive[iait]) {
		    res1.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(iait), pm->graph.target(iait)));
		    incluconj = false;
		    break;
		}
	    }

	    if (incluconj) {
		res1.append(res[i]);
	    }

	} else {
	    res1.append(res[i]);
	}
    }

    return res1;
}

bool LocalSearchModPM::moveOperPossible(const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node & node) {
    //QTextStream out(stdout);
    opMovePossibleTimer.start();

    /**
     * We are trying to insert the given node between the start and the end nodes
     * of the specified arcs. The conditions described by Dauzere-Peres and Paulli
     * are checked, since no cycles should occur.
     */

    //if ((node == pm->graph.source(arc)) || (node == pm->graph.target(arc))) return false;

    //if (j == node || k == node) return false;

    /** This means that there are no other operations on the machine and 
     * therefore moving some operation to this machine will result in only 
     * deleting its previous connections in the graph. Thus, no cycles can occur.*/
    if (j == INVALID && k == INVALID) {
	opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
	return true;
    }

    //if (pm->conPathExists(j, k)) return false;

    QList<ListDigraph::Node> fri; // IMPORTANT!!! There can be several routing predecessors or successors of the node i
    QList<ListDigraph::Node> pri;

    // Find the fri and the pri
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	if (pm->conjunctive[oait]) { // The routing based arcs
	    fri.append(pm->graph.target(oait));
	}
    }

    for (ListDigraph::InArcIt iait(pm->graph, node); iait != INVALID; ++iait) {
	if (pm->conjunctive[iait]) { // The routing based arcs
	    pri.append(pm->graph.source(iait));
	}
    }

    if (j != INVALID) {
	for (int i1 = 0; i1 < fri.size(); i1++) {
	    if (j == fri[i1]) {
		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    }
	}
    }

    if (k != INVALID) {
	for (int i2 = 0; i2 < pri.size(); i2++) {
	    if (k == pri[i2]) {
		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    }
	}
    }

    // Check condition (inequalities) with predecessors and successors
    for (int i1 = 0; i1 < fri.size(); i1++) {
	for (int i2 = 0; i2 < pri.size(); i2++) {
	    bool cond1;
	    bool cond2;

	    if (j != INVALID) {
		cond1 = Math::cmp(pm->ops[j]->r(), pm->ops[fri[i1]]->r() + pm->ops[fri[i1]]->p()) == -1;
	    } else {
		cond1 = true;
	    }
	    if (k != INVALID) {
		cond2 = Math::cmp(pm->ops[k]->r() + pm->ops[k]->p(), pm->ops[pri[i2]]->r()) == 1;

		//cout << "Oper " << pm->ops[k]->ID << " r+p =  " << pm->ops[k]->r() + pm->ops[k]->p() << endl;
		//cout << "Oper " << pm->ops[pri[i2]]->ID << " r =  " << pm->ops[pri[i2]]->r() << endl;

	    } else {
		cond2 = true;
	    }

	    if (!(cond1 && cond2)) {
		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    } else {
		//out << "Moving " << pm->ops[node]->ID << " between " << ((j!=INVALID) ? pm->ops[j]->ID : -2) << " and " << ((k!=INVALID) ? pm->ops[k]->ID : -1) << endl;
		//out << *pm << endl;
	    }
	}
    }

    opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
    return true;
}

QPair<ListDigraph::Node, ListDigraph::Node> LocalSearchModPM::selectArcToBreak(const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node & node) {
    QTextStream out(stdout);
    /** Algorithm:
     
     * 1. Traverse arcs until the needed criterion is satisfied
     * 2. Return the selected arc
     * 
     */

    QPair<ListDigraph::Node, ListDigraph::Node> curarc;
    ListDigraph::Node j;
    ListDigraph::Node k;

    //QSet<int> rndindices;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > modarcs = arcs;
    int idx;
    int lpos = modarcs.size() - 1;

    do {

	if (lpos == -1) {
	    j = INVALID;
	    k = INVALID;
	    if (arcs.size() > 0) {
		out << "Moving operation : " << pm->ops[node]->ID << endl;
		for (int i = 0; i < arcs.size(); i++) {
		    out << ((arcs[i].first == INVALID) ? -1 : pm->ops[arcs[i].first]->ID) << " -> " << ((arcs[i].second == INVALID) ? -1 : pm->ops[arcs[i].second]->ID) << endl;
		}
		Debugger::eDebug("Failed to find other insertion positions!!!");
	    }
	    break;
	}

	// Select the next arc to be considered as a break candidate

	//idx = Rand::rndInt(0, lpos);
	idx = Rand::rnd<Math::uint32>(0, lpos);
	curarc = modarcs[idx];
	modarcs.move(idx, lpos);
	lpos--;

	j = curarc.first;
	k = curarc.second;

    } while (!moveOperPossible(j, k, node));

    return QPair<ListDigraph::Node, ListDigraph::Node > (j, k);
}

QPair<ListDigraph::Node, ListDigraph::Node> LocalSearchModPM::selectBestArcToBreak(const int& mid, const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node) {
    double bstobj = Math::MAX_DOUBLE;
    double cobj;
    QPair<ListDigraph::Node, ListDigraph::Node> bstmove;

    bstmove.first = INVALID;
    bstmove.second = INVALID;

    for (int j = 0; j < arcs.size(); j++) {
	if (moveOperPossible(arcs[j].first, arcs[j].second, node)) {

	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "LocalSearchModPM::selectBestArcToBreak : Graph BEFORE moving the operation: " << endl << *pm << endl;
	    //out << "Moving operation : " << *pm->ops[optomove] << endl;
	    //out << "Moving between : " << ((arcs[j].first != INVALID) ? pm->ops[arcs[j].first]->ID : -1) << " and " << ((arcs[j].second != INVALID) ? pm->ops[arcs[j].second]->ID : -1) << endl;

	    // Try to move the operation 
	    moveOper(mid, arcs[j].first, arcs[j].second, node);

	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "Moved operation : " << *pm->ops[optomove] << endl;

	    //out << "LocalSearchModPM::selectBestArcToBreak : Graph AFTER moving the operation: " << endl << *pm << endl;
	    /*
	    if (!dag(pm->graph)) {
		    Debugger::err << "LocalSearchModPM::selectBestArcToBreak : Graph not DAG after operation move!!!" << ENDL;
	    }
	     */

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();
	    updateEval(nodeI, nodeT);

	    // Calculate the current objective
	    objTimer.start();
	    cobj = obj(*pm, pm->terminals());
	    objElapsedMS += objTimer.elapsed();

	    //out << "bstobj = " << bstobj << endl;
	    //out << "cobj = " << cobj << endl;

	    // Check whether the move is the best
	    if (bstobj >= cobj) { // This move is not the worse one
		bstobj = cobj;
		bstmove = arcs[j];

		//out << "Best objective found when moving to machine " << bstmachid << endl;
	    }

	    // Move back the operation
	    moveBackOper(node);

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "LocalSearchModPM::selectBestArcToBreak : Graph after moving BACK the operation: " << endl << *pm << endl;

	    // IMPORTANT!!! Restore only if the graph has changed since the last move

	    /*
	    if (!prevRS.empty()) {
		    ListDigraph::Node curnode;
		    int n = topolOrdering.size(); //topolSorted.size();
		    //for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
		    for (int j = topolITStart; j < n; j++) {
			    curnode = topolOrdering[j]; //topolSorted[j];
			    pm->ops[curnode]->r(prevRS[curnode].first);
			    pm->ops[curnode]->s(prevRS[curnode].second);

			    //out << "Restoring for : " << pm->ops[curnode]->ID << endl;
			    //out << "r = ( " << pm->ops[curnode]->r() << " , " << prevRS[curnode].first << " ) " << endl;
			    //out << "s = ( " << pm->ops[curnode]->s() << " , " << prevRS[curnode].second << " ) " << endl;


			    //if (Math::cmp(pm->ops[curnode]->r(), prevRS[curnode].first, 0.0001) != 0) {
			    //Debugger::err << "LocalSearchModPM::selectBestArcToBreak : Something is wrong with r while restoring!!!" << ENDL;
			    //}

			    //if (Math::cmp(pm->ops[curnode]->s(), prevRS[curnode].second, 0.0001) != 0) {
			    //Debugger::err << "LocalSearchModPM::selectBestArcToBreak : Something is wrong with s while restoring!!!" << ENDL;
			    //}


		    }

	    }

	     */

	}
    }

    //out << "Found best move : " << ((bstmove.first != INVALID) ? pm->ops[bstmove.first]->ID : -1) << " and " << ((bstmove.second != INVALID) ? pm->ops[bstmove.second]->ID : -1) << endl;

    return bstmove;
}

void LocalSearchModPM::findBestOperMove(const ListDigraph::Node& optm, int& targetMachID, QPair<ListDigraph::Node, ListDigraph::Node>& atb) {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Get the list of machines able to process the operation
     * 2. For every machine:
     * 2.1. Select pairs of operations between which the operation can be moved (Collect the move possibilities)
     * 3. For every selected move possibility estimate the total objective.
     * 4. Return the best possible move possibility
     */

    QHash< int, QList<QPair<ListDigraph::Node, ListDigraph::Node> > > machid2arcs;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > breakablearcs;

    // Get the list of available in the TG machines
    QList<Machine*> tgmachines = ((*rc)(pm->ops[optm]->toolID)).machines();

    // Iterate over the machines of the relative tool group
    for (int machidx = 0; machidx < tgmachines.size(); machidx++) {

	// Select potential insertion positions on the current machine
	breakablearcs = selectBreakableArcs(tgmachines[machidx]->ID);

	for (int j = 0; j < breakablearcs.size(); j++) {
	    if (moveOperPossible(breakablearcs[j].first, breakablearcs[j].second, optm)) {
		machid2arcs[tgmachines[machidx]->ID].append(breakablearcs[j]);
	    }
	}

	// In case the machine is empty
	if (machid2arcs[tgmachines[machidx]->ID].size() == 0) {
	    machid2arcs[tgmachines[machidx]->ID].append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, INVALID));
	}

	//out << "Found breakable arcs for machine " << tgmachines[machidx]->ID << " : " << machid2arcs[tgmachines[machidx]->ID].size() << endl;
    }

    // Iterate over the possible moves and select the best move possible
    double bstobj = Math::MAX_DOUBLE;
    double cobj;
    int bstmachid = -1;
    QPair<ListDigraph::Node, ListDigraph::Node> bstmove;


    for (QHash< int, QList<QPair<ListDigraph::Node, ListDigraph::Node> > >::iterator iter = machid2arcs.begin(); iter != machid2arcs.end(); iter++) {
	for (int j = 0; j < iter.value().size(); j++) {
	    // Try to move the operation 
	    moveOper(iter.key(), iter.value()[j].first, iter.value()[j].second, optm);

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();
	    pm->updateHeadsAndStartTimes(topolOrdering);

	    // Calculate the current objective
	    cobj = obj(*pm, pm->terminals());

	    //out << "bstobj = " << bstobj << endl;
	    //out << "cobj = " << cobj << endl;

	    // Check whether the move is the best
	    if (bstobj >= cobj) { // This move is not the worse one
		bstobj = cobj;
		bstmachid = iter.key();
		bstmove = iter.value()[j];

		//out << "Best objective found when moving to machine " << bstmachid << endl;
	    }

	    // Move back the operation
	    moveBackOper(optm);

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();
	    pm->updateHeadsAndStartTimes(topolOrdering);

	}
    }

    // Return the best found potential move
    if (bstmachid == -1) {
	out << "Moving operation " << pm->ops[optm]->ID << endl;
	out << *pm << endl;
	Debugger::err << "LocalSearchModPM::findBestOperMove : failed to find the best move!" << ENDL;
    } else {
	targetMachID = bstmachid;
	atb = bstmove;
    }

}

void LocalSearchModPM::moveOper(const int& mid, const ListDigraph::Node &jNode, const ListDigraph::Node &kNode, const ListDigraph::Node& node) {
    opMoveTimer.start();

    /** Algorithm:
     * 
     * 1. Remove schedule-based arcs of the specified node
     * 2. Insert a single arc connecting the nodes which were preceding and
     *	  succeeding the node in the schedule
     * 3. Remove the specified arc
     * 4. Insert the node between the starting an the ending nodes of the 
     *    specified arc
     * 
     */

    //out << "PM before moving operation : " << pm->ops[node]->ID << endl << *pm << endl;

    //###########################  DEBUG  ######################################

    /*
    out << "Moving operation : " << pm->ops[node]->ID << endl;

    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;

			    for (int l = 0; l < topolOrdering.size(); l++) {
				    out << pm->ops[topolOrdering[l]]->ID << " ";
			    }
			    out << endl;

			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct before moving the operation!!!" << ENDL;
		    }
	    }
    }
     */

    /*
    out << *pm << endl;

    //debugCheckPMCorrectness("LocalSearchModPM::moveOper : Before moving the next operation.");

    if (j != INVALID && k != INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << pm->ops[j]->ID << " and " << pm->ops[k]->ID << endl;
    }

    if (j != INVALID && k == INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << pm->ops[j]->ID << " and " << " * " << endl;
    }

    if (j == INVALID && k != INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << " * " << " and " << pm->ops[k]->ID << endl;
    }
     */

    //##########################################################################

    ListDigraph::Node sNode = INVALID;
    ListDigraph::Node tNode = INVALID;
    ListDigraph::Arc siArc = INVALID;
    ListDigraph::Arc itArc = INVALID;
    ListDigraph::Arc stArc = INVALID;
    ListDigraph::Arc jiArc = INVALID;
    ListDigraph::Arc ikArc = INVALID;

    arcsRem.clear();
    arcsIns.clear();
    weightsRem.clear();

    // IMPORTANT!!!
    //topolSorted.clear();
    prevRS.clear();

    if ((node == jNode) || (node == kNode)) { // The operation is not moved
	remMachID = pm->ops[node]->machID;

	arcsRem.clear();
	arcsIns.clear();
	weightsRem.clear();

	// No need to perform topological sorting since the graph stays unchanged
	// IMPORTANT!!! Clear the old topol. sorting so that the preserved ready times and start times are not restored incorrectly
	//topolSorted.clear();
	prevRS.clear();

	// IMPORTANT!!! Set the actual topological ordering! Else the incorrect TO is restored
	prevTopolOrdering = topolOrdering;
	prevTopolITStart = topolITStart;

	//out << "###################   Operation is not moved!" << endl;
	return;
    }

    //if (j == INVALID && k != INVALID) out << "#####################  Moving to the front." << endl;
    //if (j != INVALID && k == INVALID) out << "#####################  Moving to the back." << endl;
    //if (j != INVALID && k != INVALID) out << "#####################  Moving in the middle." << endl;
    //if (j == INVALID && k == INVALID) out << "#####################  Moving to the empty machine." << endl;

    // Find the fri and the pri
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	if (!pm->conjunctive[oait]) { // The schedule-based arcs
	    tNode = pm->graph.target(oait);
	    itArc = oait;
	    break;
	}

	// If there is no schedule-based arc then the routing-based successor might come into consideration
    }

    for (ListDigraph::InArcIt iait(pm->graph, node); iait != INVALID; ++iait) {
	if (!pm->conjunctive[iait]) { // The schedule-based arcs
	    sNode = pm->graph.source(iait);
	    siArc = iait;
	    break;
	}
    }

    //Debugger::iDebug("Removing previous connections...");
    // Remove the former connections
    if (sNode != INVALID) {
	arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (sNode, node));
	weightsRem.append(pm->p[siArc]);

	//out << "Erasing " << pm->ops[s]->ID << " -> " << pm->ops[node]->ID << endl;

	pm->graph.erase(siArc);
    }

    //###########################  DEBUG  ######################################

    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing si!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    if (tNode != INVALID) {
	arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (node, tNode));
	weightsRem.append(pm->p[itArc]);

	//out << "Erasing " << pm->ops[node]->ID << " -> " << pm->ops[t]->ID << endl;

	pm->graph.erase(itArc);
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing it!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    // Insert the direct connection between s and t
    if (sNode != INVALID && tNode != INVALID /*&& !pm->conPathExists(s, t)*/) {
	stArc = pm->graph.addArc(sNode, tNode);
	arcsIns.append(stArc);
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after inserting st!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    // Remove the arc to break (if this arc exists)
    if (jNode != INVALID && kNode != INVALID) {
	for (ListDigraph::OutArcIt oait(pm->graph, jNode); oait != INVALID; ++oait)
	    if (pm->graph.target(oait) == kNode) {
		if (!pm->conjunctive[oait]) {
		    arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (jNode, kNode));
		    weightsRem.append(/*-pm->ops[j]->p()*/pm->p[oait]);

		    pm->graph.erase(oait);
		    break;
		}
	    }
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing jk!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    //Debugger::iDebug("Removed previous connections.");


    //Debugger::iDebug("Inserting new connections...");

    // Insert the new connections
    if (jNode != INVALID /*&& !pm->conPathExists(j, node)*/) {
	jiArc = pm->graph.addArc(jNode, node);
	arcsIns.append(jiArc);
    } else {
	jiArc = INVALID;
    }
    if (kNode != INVALID /*&& !pm->conPathExists(node, k)*/) {
	ikArc = pm->graph.addArc(node, kNode);
	arcsIns.append(ikArc);
    } else {
	ikArc = INVALID;
    }

    //Debugger::iDebug("Inserted new connections.");

    // Update the topological ordering of the graph dynamically

    // Save the current topological ordering
    prevTopolOrdering = topolOrdering;
    prevTopolITStart = topolITStart;

    // Update the topological ordering
    dynTopSortTimer.start();
    //out << "Performing DTO..." << endl;
    /*
    if (!dag(pm->graph)) {
	    Debugger::err << "Graph is not DAG before the DTO!" << ENDL;
    }
     */
    dynUpdateTopolOrdering(topolOrdering, node, jNode, kNode);
    //out << "Done DTO." << endl;
    dynTopSortElapsedMS += dynTopSortTimer.elapsed();

    // Update the recalculation region
    int idxt = topolOrdering.indexOf(tNode);
    int idxi = topolOrdering.indexOf(node);
    if (idxt >= 0 && idxi >= 0) {
	topolITStart = Math::min(idxt, idxi);
    } else {
	topolITStart = Math::max(idxt, idxi);
    }


    /*
	    // Perform topological sorting of the nodes reachable from i and/or t in the NEW graph G-tilde
	    QList<ListDigraph::Node> startSet;
	    startSet.append(t);
	    startSet.append(node);
     */

    // Sort topologically all nodes reachable from i and/or from t
    //out << "Performing topological sorting..." << endl;
    topSortTimer.start();
    //QList<ListDigraph::Node> reachable = pm->reachableFrom(startSet);
    /*
    int idxt = topolOrdering.indexOf(t);
    int idxi = topolOrdering.indexOf(node);
    int startidx;
    if (idxt >= 0 && idxi >= 0) {
	    startidx = Math::min(idxt, idxi);
    } else {
	    startidx = Math::max(idxt, idxi);
    }

    topolSorted = topolOrdering.mid(startidx, topolOrdering.size() - startidx);
     */
    topSortElapsedMS += topSortTimer.elapsed();
    //out << "Performed topological sorting." << endl;

    //######################  DEBUG  ###########################################

    // Check the correctness of the topological sorting
    /*
    out << "Topological sorting : " << endl;
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    out << pm->ops[topolSorted[i]]->ID << " ";
    }
    out << endl;
     */


    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    for (int j = i + 1; j < topolSorted.size(); j++) {
		    if (reachable(topolSorted[j], topolSorted[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolSorted[j]]->ID << " -> " << pm->ops[topolSorted[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct!!!" << ENDL;
		    }
	    }
    }
     */

    //out << "PM before preserving : " << *pm << endl;

    //##########################################################################

    // Preserve the ready times and the start times of the previous graph for the topologically sorted nodes
    Operation *curop = NULL;
    int n = topolOrdering.size(); //topolSorted.size();
    //for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
    for (int j = topolITStart/*0*/; j < n; j++) {
	curop = pm->ops[topolOrdering[j]];
	// Preserve the former value => WRONG!!!
	//out << "Preserving for : " << pm->ops[curnode]->ID << endl;
	//out << "r = " << pm->ops[curnode]->r() << endl;
	//out << "s = " << pm->ops[curnode]->s() << endl;
	prevRS[curop->ID].first = curop->r();
	prevRS[curop->ID].second = curop->s();
    }


    //Debugger::iDebug("Updating the data of the newly inserted operation...");

    // Update the machine id of the moved operation and preserve the previous assignment ID
    remMachID = pm->ops[node]->machID;
    pm->ops[node]->machID = mid;
    /*
    if (j != INVALID) {
	    pm->ops[node]->machID = pm->ops[j]->machID;
    } else {
	    if (k != INVALID) {
		    pm->ops[node]->machID = pm->ops[k]->machID;
	    } else {
		    Debugger::eDebug("LocalSearchModPM::moveOper : Moving operation between two invalid operations!");
	    }
    }
     */

    //Debugger::iDebug("Updating the proc. time of the newly inserted operation...");
    // Update the processing time of the moved operation
    pm->ops[node]->p(((*rc)(pm->ops[node]->toolID, pm->ops[node]->machID)).procTime(pm->ops[node]));
    //Debugger::iDebug("Updated the proc. time of the newly inserted operation.");

    //Debugger::iDebug("Setting the weights of the newly inserted arcs...");
    // Set the weights of the newly inserted arcs
    //Debugger::iDebug("st...");
    if (stArc != INVALID) {
	pm->p[stArc] = -pm->ops[sNode]->p();
    }
    //Debugger::iDebug("st.");
    if (jiArc != INVALID) {
	pm->p[jiArc] = -pm->ops[jNode]->p();
    }
    //Debugger::iDebug("Set the weights of the newly inserted arcs.");

    //Debugger::iDebug("Recalculating the processing time of the moved operation...");
    // Processing time for the moved operation must be updated
    if (ikArc != INVALID) {
	pm->p[ikArc] = -pm->ops[node]->p();
    }
    //Debugger::iDebug("Recalculated the processing time of the moved operation.");

    // Update length of all arcs going out from i
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	pm->p[oait] = -pm->ops[node]->p();
    }

    //Debugger::iDebug("Updated the data of the newly inserted operation.");

    // Set the outgoing nodes for the update
    nodeI = node;
    nodeT = tNode;

    opMoveElapsedMS += opMoveTimer.elapsed();
}

void LocalSearchModPM::moveBackOper(const ListDigraph::Node & node) {
    opMoveBackTimer.start();
    //out << "Moving back operation : " << pm->ops[node]->ID << endl;

    // Remove the newly inserted arcs
    for (int i = 0; i < arcsIns.size(); i++) {
	pm->graph.erase(arcsIns[i]);
    }
    arcsIns.clear();

    // Insert the previous arcs
    ListDigraph::Arc curarc;
    for (int i = 0; i < arcsRem.size(); i++) {
	curarc = pm->graph.addArc(arcsRem[i].first, arcsRem[i].second);
	pm->p[curarc] = weightsRem[i];

	//out << "Restored " << pm->ops[arcsRem[i].first]->ID << " -> " << pm->ops[arcsRem[i].second]->ID << endl;

	/*
	if (pm->graph.source(curarc) == node) {
		pm->ops[node]->machID = pm->ops[pm->graph.target(curarc)]->machID;
	} else {
		if (pm->graph.target(curarc) == node) {
			pm->ops[node]->machID = pm->ops[pm->graph.source(curarc)]->machID;
		}
	}
	 */
    }

    // Restore the machine assignment of the operation
    pm->ops[node]->machID = remMachID;

    // Restore the processing time of the moved operation
    pm->ops[node]->p(((*rc)(pm->ops[node]->toolID, pm->ops[node]->machID)).procTime(pm->ops[node]));

    // Restore arc lengths of the arcs coming out from i
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	pm->p[oait] = -pm->ops[node]->p();
    }

    arcsRem.clear();
    weightsRem.clear();

    //out << "PM after moving Back operation : " << pm->ops[node]->ID << endl << *pm << endl;


    // Restore the ready times and the due dates if the graph has changed
    // IMPORTANT!!! Update only if the graph has been changed!!!
    // IMPORTANT!!! Restor r and s BEFORE the old topological ordering is restored
    if (!prevRS.empty()) {
	Operation *curop;
	int n = topolOrdering.size(); //topolSorted.size();
	//for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
	for (int j = topolITStart; j < n; j++) {
	    curop = pm->ops[topolOrdering[j]];
	    curop->r(prevRS[curop->ID].first);
	    curop->s(prevRS[curop->ID].second);

	    //out << "Restoring for : " << pm->ops[curnode]->ID << endl;
	    //out << "r = ( " << pm->ops[curnode]->r() << " , " << prevRS[curnode].first << " ) " << endl;
	    //out << "s = ( " << pm->ops[curnode]->s() << " , " << prevRS[curnode].second << " ) " << endl;


	    //if (Math::cmp(pm->ops[curnode]->r(), prevRS[curnode].first, 0.0001) != 0) {
	    //Debugger::err << "Something is wrong with r while restoring!!!" << ENDL;
	    //}

	    //if (Math::cmp(pm->ops[curnode]->s(), prevRS[curnode].second, 0.0001) != 0) {
	    //Debugger::err << "Something is wrong with s while restoring!!!" << ENDL;
	    //}

	}
    }

    // Restore the previous topological ordering of the nodes 
    topolOrdering = prevTopolOrdering;
    topolITStart = prevTopolITStart;

    // #########################    DEBUG    ###################################    

    /*
    out << "Check reachability for every machine after moving BACK the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency after moving back the operation..."<<endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchModPM::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */
    // #########################################################################
    opMoveBackElapsedMS += opMoveBackTimer.elapsed();
}

ListDigraph::Node LocalSearchModPM::selectTerminalContrib(QList<ListDigraph::Node> &terminals) {
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

ListDigraph::Node LocalSearchModPM::selectTerminalRnd(QList<ListDigraph::Node> &terminals) {

    //return terminals[Rand::rndInt(0, terminals.size() - 1)];
    return terminals[Rand::rnd<Math::uint32>(0, terminals.size() - 1)];
}

ListDigraph::Node LocalSearchModPM::selectTerminalNonContrib(QList<ListDigraph::Node> &terminals) {
    QList<QPair<QPair<double, double>, ListDigraph::Node > > interval2node;
    double totalobj = 0.0;
    double istart;
    double iend;
    ListDigraph::Node res = INVALID;
    double maxtwt = 0.0;

    // Find the biggest weighted tardiness
    for (int i = 0; i < terminals.size(); i++) {
	maxtwt = Math::max(maxtwt, pm->ops[terminals[i]]->wT());
    }

    for (int i = 0; i < terminals.size(); i++) {
	istart = totalobj;
	totalobj += maxtwt - pm->ops[terminals[i]]->wT(); // The bigger the contribution the smaller the probability is
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

void LocalSearchModPM::diversify() {
    //Debugger::info << "LocalSearchModPM::diversify ... " << ENDL;
    //return;
    /** Algorithm:
     * 
     * 1. Select the random number of arcs to be reversed
     * 
     * 2. Reverse randomly the selected number of critical arcs 
     * 
     */

    pm->restore();
    topolOrdering = pm->topolSort();
    topolITStart = 0;
    //pm->updateHeads(topolOrdering);
    //pm->updateStartTimes(topolOrdering);
    pm->updateHeadsAndStartTimes(topolOrdering);
    curobj = obj(*pm, pm->terminals());

    updateCriticalNodes();

    //int nops2move = Rand::rndInt(5, 10);
    int nops2move = Rand::rnd<Math::uint32>(5, 10);
    //int nops2move = Rand::rndInt(kDiv + 5, kDiv + 10);
    int nopsmoved = 0;

    // Get the terminals
    QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath;
    ListDigraph::Node cop;

    //out <<"PM before the diversification:"<<endl;
    //out << *pm << endl;
    //getchar();

    do {

	do {
	    // Select some terminal for the manipulations (based on the contribution of the terminal to the objective)

	    theterminal = selectTerminalContrib(terminals);

	    // Find a critical path to the selected terminal
	    //cpath = longestPath(theterminal);

	    // Select operation to move
	    //cop = defaultSelectOperToMove(cpath);
	    //cop = criticalNodes[Rand::rndInt(0, criticalNodes.size() - 1)];
	    cop = criticalNodes[Rand::rnd<Math::uint32>(0, criticalNodes.size() - 1)];

	} while (cop == INVALID);

	int targetMachID = selectTargetMach(cop);

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs = selectBreakableArcs(targetMachID);

	// Select an arc to break
	QPair<ListDigraph::Node, ListDigraph::Node> atb = selectArcToBreak(relarcs, cop);

	// Move the operation
	moveOper(targetMachID, atb.first, atb.second, cop);

	//if (!dag(pm->graph)) moveBackOper(cop);

	// Update the ready times and the start times of the operations in the graph
	//pm->updateHeads(topolOrdering);
	//pm->updateStartTimes(topolOrdering);
	pm->updateHeadsAndStartTimes(topolOrdering);

	nopsmoved++;


	//out <<"PM after the first step of diversification:"<<endl;
	//out << *pm << endl;
	//getchar();


	if (obj(*pm, pm->terminals()) < bestobj) {
	    pm->save();
	    curobj = obj(*pm, pm->terminals());
	    prevobj = curobj;
	    bestobj = curobj;
	    nisteps = 0;

	    kDiv = 1;

	    updateCriticalNodes();
	    break;
	}


    } while (/*objimprov(*pm, pm->terminals()) > bestobjimprov &&*/ nopsmoved < nops2move);

    //if (objimprov(*pm, pm->terminals()) < bestobjimprov) {
    //pm->save();
    curobj = obj(*pm, pm->terminals());
    prevobj = curobj; // + 0.00000000001;
    //bestobj = curobj;
    nisteps = 0;
    //bestobjimprov = Math::MAX_DOUBLE;
    //prevobjimprov = Math::MAX_DOUBLE;
    //}

    updateCriticalNodes();

    //Debugger::info << "LocalSearchModPM::diversify : Finished. " << ENDL;

}

void LocalSearchModPM::updateEval(const ListDigraph::Node& /*iNode*/, const ListDigraph::Node& /*tNode*/) {
    updateEvalTimer.start();

    /** Algorithm:
     * 
     * 1. Collect nodes reachable from i
     * 2. Collect nodes reachable from t
     * 3. The union of these sets is the set of nodes to be updated for the evaluation
     * 4. If i is reachable from t then start updating from t
     *	  If t is reachable from i then start updating from i
     *    Else update starting from i and from t	
     * 
     */

    //out << "Running updateEval..." << endl;

    // For the topologically sorted nodes update the ready times and the start times of the operations (preserving the former values)

    // Update the ready times
    ListDigraph::Node curnode;
    ListDigraph::Node prevnode;
    double curR; // The calculated ready time of the current node

    int n = topolOrdering.size();

    for (int j = topolITStart; j < n; j++) {
	curnode = topolOrdering[j];

	curR = pm->ops[curnode]->ir();
	// Iterate over all predecessors of the current node
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    prevnode = pm->graph.source(iait);
	    curR = Math::max(curR, pm->ops[prevnode]->c());
	}

	//############################  DEBUG  #################################
	//out << "r = ( " << pm->ops[curnode]->r() << " , " << curR << " ) " << endl;

	/*
	if (pm->ops[curnode]->r() != curR) {
		out << "Current node ID = " << pm->ops[curnode]->ID << endl;
		out << *pm << endl;
		Debugger::err << "Something is wrong with r !!!" << ENDL;
	}
	 */
	//######################################################################

	// Take into account the machine availability time

	curR = Math::max(curR, pm->ops[curnode]->machAvailTime());

	pm->ops[curnode]->r(curR, false); // The earliest time point to start the operation

	pm->ops[curnode]->s(curR/*, false*/); // The earliest time point to start the operation

    }

    updateEvalElapsedMS += updateEvalTimer.elapsed();
}

void LocalSearchModPM::dynUpdateTopolOrdering(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &j, const ListDigraph::Node &k) {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Find the positions of i, j and k
     * 2. IF posj < posi < posk => no changes to the topological sorting need to be performed. Return.
     * 3. IF posi > posk => reorder the nodes. The affected region is [posi, posk]. Return.
     * 4. IF posi < posj => reorder the nodes. The affected region is [posj, posi]. Return.
     * 
     */

    Math::intUNI posj = -1;
    Math::intUNI posi = -1;
    Math::intUNI posk = -1;

    if (j == INVALID) {
	posj = -1;
    } else {
	posj = topolOrdering.indexOf(j);
    }

    if (k == INVALID) {
	posk = Math::MAX_INTUNI;
    } else {
	posk = topolOrdering.indexOf(k);
    }

    posi = topolOrdering.indexOf(i);

    if (posj < posi && posi < posk) { // No changes to perform
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

    if (posj >= posk) {
	out << "posj = " << posj << " ID = " << ((j == INVALID) ? -1 : pm->ops[j]->ID) << endl;
	out << "posi = " << posi << " ID = " << ((i == INVALID) ? -1 : pm->ops[i]->ID) << endl;
	out << "posk = " << posk << " ID = " << ((k == INVALID) ? -1 : pm->ops[k]->ID) << endl;

	for (Math::intUNI l = 0; l < topolOrdering.size(); l++) {
	    out << pm->ops[topolOrdering[l]]->ID << " ";
	}
	out << endl;

	Debugger::err << "LocalSearchModPM::dynUpdateTopolOrdering : posj >= posk which is impossible!!!" << ENDL;
    }

    // Find the affected region
    Math::intUNI arbegin = -1;
    Math::intUNI arend = -1;
    ListDigraph::Node arstartnode = INVALID;
    ListDigraph::Node arendnode = INVALID;

    if (posi < posj) {
	arbegin = posi;
	arend = posj;
	arstartnode = i;
	arendnode = j;
    }

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
    Math::intUNI tmpidx;
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

    for (Math::intUNI l = 0; l < ar.size(); l++) {
	visited.append(false);
    }

    q.clear();
    q.enqueue(arstartnode);

    deltaF.append(arstartnode);
    while (q.size() != 0) {
	curnode = q.dequeue();

	// Check the successors of the current node
	for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
	    tmpnode = pm->graph.target(oait);

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
    Math::intUNI posF = ar.size() - 1;

    // Move elements in deltaF to the right
    while (!deltaF.isEmpty()) {
	// Find the first element in ar starting from posB that is in deltaB
	tmpidx = -1;
	for (Math::intUNI l = posF; l >= 0; l--) {
	    if (deltaF.contains(ar[l])) {
		tmpidx = l;
		break;
	    }
	}

	if (tmpidx == -1) {
	    if (j != INVALID && k != INVALID) {
		out << "Moving " << pm->ops[i]->ID << " between " << pm->ops[j]->ID << " and " << pm->ops[k]->ID << endl;
	    }

	    if (j != INVALID && k == INVALID) {
		out << "Moving " << pm->ops[i]->ID << " between " << pm->ops[j]->ID << " and " << " * " << endl;
	    }

	    if (j == INVALID && k != INVALID) {
		out << "Moving " << pm->ops[i]->ID << " between " << " * " << " and " << pm->ops[k]->ID << endl;
	    }

	    out << *pm << endl;
	    Debugger::err << "LocalSearchModPM::dynUpdateTopolOrdering : tmpidx = -1 while shifting deltaF. Probably the graph is NOT DAG! " << ENDL;
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
    for (Math::intUNI l = 0; l < ar.size(); l++) {
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
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct after DTO!!!" << ENDL;
		    }
	    }
    }
     */

    //getchar();

    //##########################################################################

}

void LocalSearchModPM::setMovableNodes(QMap<ListDigraph::Node, bool>& movableNodes) {
    node2Movable.clear();

    for (QMap < ListDigraph::Node, bool>::iterator iter = movableNodes.begin(); iter != movableNodes.end(); iter++) {
	node2Movable[iter.key()] = iter.value();
    }

}

bool LocalSearchModPM::reachable(const ListDigraph::Node& s, const ListDigraph::Node& t) {
    QQueue<ListDigraph::Node> q;
    ListDigraph::Node curnode;

    q.enqueue(t);

    if (s == t) return true;

    if (s == INVALID || t == INVALID) return true;

    while (q.size() > 0) {
	curnode = q.dequeue();

	// Iterate over the predecessors
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    ListDigraph::Node curStartNode = pm->graph.source(iait);
	    if (curStartNode == s) {
		return true;
	    } else {
		if (!q.contains(curStartNode)) {
		    q.enqueue(curStartNode);
		}
	    }
	}
    }

    return false;
}

bool LocalSearchModPM::debugCheckPMCorrectness(const QString& location) {
    QTextStream out(stdout);

    out << "LocalSearchModPM::debugCheckPMCorrectness : Checking correctness in < " + location + " > ... " << endl;

    totalChecksTimer.start();

    // Check cycles
    if (!dag(pm->graph)) {
	Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Graph is not DAG after scheduling!" << ENDL;
    } else {
	//Debugger::info << "LocalSearchModPM::debugCheckPMCorrectness : Graph is DAG." << ENDL;
    }

    //out << "Checked DAG." << endl;
    //getchar();

    // Check the outgoing arcs: every node must have at most one schedule-based outgoing arc
    int noutarcs = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	noutarcs = 0;
	for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
	    if (!pm->conjunctive[oait]) {
		noutarcs++;
	    }

	    if (noutarcs > 1) {
		Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Too many outgoing schedule-based arcs!" << ENDL;
	    }
	}
    }

    //out << "Checked outgoing arcs." << endl;
    //getchar();

    // Check the incoming arcs: every node must have at most one schedule-based outgoing arc
    int ninarcs = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	ninarcs = 0;
	for (ListDigraph::InArcIt iait(pm->graph, nit); iait != INVALID; ++iait) {
	    if (!pm->conjunctive[iait]) {
		ninarcs++;
	    }

	    if (ninarcs > 1) {
		Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Too many incoming schedule-based arcs!" << ENDL;
	    }
	}
    }

    //out << "Checked the incoming schedule-based arcs." << endl;
    //getchar();

    // Check whether all nodes can be reached from the start node
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	if (nit != pm->head) {
	    if (!reachable(pm->head, nit)) {
		Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Node is not reachable from the start node!" << ENDL;
	    }
	}
    }

    //out << "Checked reachability from the start node." << endl;
    //getchar();

    // Check correctness of the processing times for the scheduled nodes
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
	    if (pm->ops[nit]->p() != -pm->p[oait]) {
		out << "Operation : " << pm->ops[nit]->ID << endl;
		out << *pm << endl;
		Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Outgoing arcs have incorrect length!" << ENDL;
	    }
	}
    }

    //out << "Checked the lengths of the outgoing arcs." << endl;
    //getchar();

    // Check the ready times of the operations
    QList<ListDigraph::Node> ts = pm->topolSort();

    //out << "Got the topological ordering." << endl;
    //getchar();

    double maxr;
    ListDigraph::Node pred;
    for (int i = 0; i < ts.size(); i++) {
	maxr = pm->ops[ts[i]]->r();
	for (ListDigraph::InArcIt iait(pm->graph, ts[i]); iait != INVALID; ++iait) {
	    if (pm->conjunctive[iait]) { // In general this is true only for conjunctive arcs
		pred = pm->graph.source(iait);
		maxr = Math::max(maxr, pm->ops[pred]->r() - pm->p[iait]);

		if (pm->ops[pred]->r() - pm->p[iait] > pm->ops[ts[i]]->r()) {
		    out << "Node : " << pm->ops[ts[i]]->ID << endl;
		    out << "Pred : " << pm->ops[pred]->ID << endl;
		    out << *pm << endl;
		    Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Ready time of the succeeding node < r+p of at least one predecessor!" << ENDL;
		}
	    }
	}

	if (Math::cmp(maxr, pm->ops[ts[i]]->r(), 0.00001) != 0) {
	    out << "Operation : " << pm->ops[ts[i]]->ID << endl;
	    out << "r = " << pm->ops[ts[i]]->r() << endl;
	    out << "max r(prev) = " << maxr << endl;
	    out << *pm << endl;
	    Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Ready time of the succeeding node is too large!" << ENDL;
	}

    }

    // Start time of any operation should be at least as large as the availability time of the corresponding machine
    for (int i = 0; i < ts.size(); i++) {
	ListDigraph::Node curNode = ts[i];

	if (pm->ops[curNode]->s() < pm->ops[curNode]->machAvailTime()) {
	    out << *pm->ops[curNode] << endl;
	    Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Operation start time is less than the availability time of the corresponding machine!" << ENDL;
	}
    }

    // Check the start times of the operations
    double maxc;
    for (int i = 0; i < ts.size(); i++) {
	maxr = 0.0;
	for (ListDigraph::InArcIt iait(pm->graph, ts[i]); iait != INVALID; ++iait) {
	    pred = pm->graph.source(iait);
	    maxc = Math::max(maxc, pm->ops[pred]->c());

	    if (pm->ops[pred]->c() > pm->ops[ts[i]]->s()) {
		out << "Current operation : " << pm->ops[ts[i]]->ID << endl;
		out << "Predecessor : " << pm->ops[pred]->ID << endl;
		out << *pm << endl;
		Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Operation processing overlap!" << ENDL;
	    }
	}
    }

    // Check whether schedule-based arcs always connect operations from the same machine and tool group
    ListDigraph::Node s;
    ListDigraph::Node t;
    for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
	if (!pm->conjunctive[ait]) {
	    s = pm->graph.source(ait);
	    t = pm->graph.target(ait);

	    if (pm->ops[s]->toolID != pm->ops[t]->toolID || pm->ops[s]->machID != pm->ops[t]->machID) {
		Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : Schedule-based arc connects incompatible operations!" << ENDL;
	    }
	}
    }

    // Check whether all operations are not assigned
    int nassigned = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	if (pm->ops[nit]->machID > 0) nassigned++;
    }
    if (nassigned == 0) {
	Debugger::err << "LocalSearchModPM::debugCheckPMCorrectness : There are no operations assigned to machines!" << ENDL;
    }


    totalChecksElapsedMS += totalChecksTimer.elapsed();

    out << "LocalSearchModPM::debugCheckPMCorrectness : Done checking correctness in < " + location + " > . " << endl;

    return true;
}

/**  *********************************************************************** **/

/**  ***********************  LocalSearchPMCP  ***************************** **/

LocalSearchPMCP::LocalSearchPMCP() {
    pm = NULL;

    //lsmode = IMPROV;

    alpha = 0.1;

    nisteps = 0;

    nodeI = INVALID;
    nodeT = INVALID;

    critNodesUpdateFreq = 100;

    checkCorrectness(true);
}

LocalSearchPMCP::LocalSearchPMCP(LocalSearchPMCP& orig) : IterativeAlg(orig) {

    alpha = orig.alpha;

    nisteps = orig.nisteps;

    nodeI = orig.nodeI;
    nodeT = orig.nodeT;

    critNodesUpdateFreq = orig.critNodesUpdateFreq;

    checkCorrectness(orig._check_correctness);
}

LocalSearchPMCP::~LocalSearchPMCP() {
    this->pm = NULL;
    this->rc = NULL;
}

void LocalSearchPMCP::setPM(ProcessModel *pm) {
    this->pm = pm;

    // Mark all nodes as movable by default
    node2Movable.clear();
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	ListDigraph::Node curNode = nit;

	node2Movable[curNode] = true;
    }

    // Find the mapping of the operation IDs onto the nodes of the PM
    opID2Node = pm->opID2Node();

    // Initialize after setting the process model
    init();
}

void LocalSearchPMCP::setResources(Resources *rc) {

    this->rc = rc;

}

void LocalSearchPMCP::init() {
    QTextStream out(stdout);

    IterativeAlg::init();

    if (pm == NULL) {
	Debugger::err << "LocalSearch::init : Trying to initialize the algorithm with NULL process model!" << ENDL;
    }

    // Preserve the state of the schedule
    pm->save();

    // Find the initial topological ordering of the nodes in the graph
    topolOrdering = pm->topolSort();
    topolITStart = 0;

    pm->updateHeads(topolOrdering);
    pm->updateStartTimes(topolOrdering);

    TWT obj;

    curobj = obj(*pm);
    prevobj = curobj;
    bestobj = curobj;

    out << "LS (init) : bestobj = " << curobj << endl;

    alpha = 0.1;

    nisteps = 0;

    // Get the critical nodes in the graph
    updateCriticalNodes();

    nodeI = INVALID;
    nodeT = INVALID;

    topSortElapsedMS = 0;
    objElapsedMS = 0;
    updateEvalElapsedMS = 0;
    totalElapsedMS = 0;
    opSelectionElapsedMS = 0;
    potentialPositionsSelectionElapsedMS = 0;
    posSelectionElapsedMS = 0;
    opMoveElapsedMS = 0;
    opMoveBackElapsedMS = 0;
    blocksExecElapsedMS = 0;
    opMovePossibleElapsedMS = 0;
    dynTopSortElapsedMS = 0;
    updateCritNodesElapsedMS = 0;
    longestPathsElapsedMS = 0;

    totalChecksElapsedMS = 0;

    totalTimer.start();
    //##########################################################################
}

void LocalSearchPMCP::stepActions() {
    //if (iter() % 1 == 0) out << "iter : " << iter() << endl;

    blocksExecTimer.start();

    //if (Rand::rndDouble() < 1.99999) {
    if (Rand::rnd<double>() < 1.99999) {
	transitionPM();
    } else {
	transitionCP();
    }

    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

void LocalSearchPMCP::assessActions() {
    blocksExecTimer.start();
    //out << "Assessing the step of the LS..." << endl;

    // Update the ready times and the start times of the operations in the graph
    //pm->updateHeads();
    //pm->updateStartTimes();

    updateEval(nodeI, nodeT);

    objTimer.start();
    curobj = obj(*pm, pm->terminals());
    objElapsedMS += objTimer.elapsed();


    //out << "Done assessing the step of the LS." << endl;
    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

bool LocalSearchPMCP::acceptCondition() {
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

    alpha = Math::exp(-(curobj - prevobj) / Math::pow(1.0 - (double) iter() / double(maxIter()), 1.0)); // alpha = /*0.2; //*/0.5 * Math::exp(-((double) iter())); // 1.0 - 0.999 * Math::exp(1.0 - ((double) iter()) / double(_maxiter));
    //alpha = 0.001;

    if (curobj <= prevobj /*bestobj*/) {
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

}

void LocalSearchPMCP::acceptActions() {
    QTextStream out(stdout);
    blocksExecTimer.start();
    //out << "accepted." << endl;

    if (acceptedworse || curobj == bestobj) {
	nisteps++;
    } else {
	if (curobj < bestobj) {
	    nisteps = 0;
	} else {
	    nisteps++;
	}
    }

    if (curobj <= bestobj) {
	if (curobj < bestobj) out << "LSPM (" << iter() << ") : bestobj = " << bestobj << endl;

	bestobj = curobj;

	// Preserve the state of the process model
	pm->save();

	updateCriticalNodes(); // Notice : updating critical nodes every time a better solution has been found it REALLY EXPENSIVE.


    }

    // Update the critical nodes in the graph
    //updateCriticalNodes();

    prevobj = curobj;


    //out << "The step has been accepted." << endl;
    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

void LocalSearchPMCP::declineActions() {
    blocksExecTimer.start();
    //out << "Declining the iteration..." << endl;

    moveBackOper(optomove);
    optomove = INVALID;

    // Restore the previous ready times and start times of the operations
    //pm->updateHeads();
    //pm->updateStartTimes();

    //out << "Graph after moving BACK the operation " << *pm << endl;
    //out << "PM after restoring : " << *pm << endl;


    //out << "Done declining." << endl;


    // Preform diversification

    //if (iter() > 250000) {
    //	alpha = Math::min(alpha + 0.000001, 0.1);
    //out << "Alpha = " << alpha << endl;
    //}

    nisteps++;


    if (nisteps > 2000) {
	//out << "Diversifying (nisteps)..." << endl;
	diversify();
    }

    blocksExecElapsedMS += blocksExecTimer.elapsed();
}

bool LocalSearchPMCP::stopCondition() {
    return (curobj <= obj.LB(*pm)) || IterativeAlg::stopCondition();
}

void LocalSearchPMCP::stopActions() {
    Debugger::info << "LocalSearchPMCP::stopActions : Found local optimum with objective " << bestobj << ENDL;
    //getchar();
}

void LocalSearchPMCP::preprocessingActions() {
    // Check correctness of the PM right before the processing
    //out << "LocalSearchPMCP::preprocessingActions : Checking PM correctness..." << endl;
    if (_check_correctness) {
	debugCheckPMCorrectness("LocalSearchPMCP::preprocessingActions");
    }
    //out << "LocalSearchPMCP::preprocessingActions : PM is correct." << endl;
}

void LocalSearchPMCP::postprocessingActions() {
    QTextStream out(stdout);
    // Restore the state corresponding to the best found value of the objective
    pm->restore();


    //out << "PM : " << endl << *pm << endl;

    // Check the correctness of the process model
    if (_check_correctness) {
	debugCheckPMCorrectness("LocalSearchPMCP::postprocessingActions");
    }


    out << "                  " << endl;


    totalElapsedMS = totalTimer.elapsed();

    /*
    out << "Time (ms) for objective estimations : " << objElapsedMS << endl;
    out << "Time (ms) for topological sorting : " << topSortElapsedMS << endl;
    out << "Time (ms) for running the update evaluations : " << updateEvalElapsedMS << endl;
    out << "Time (ms) for selecting operations : " << opSelectionElapsedMS << endl;
    out << "Time (ms) for finding potential moves : " << potentialPositionsSelectionElapsedMS << endl;
    out << "Time (ms) for selecting the insert position : " << posSelectionElapsedMS << endl;
    out << "Time (ms) for moving operation : " << opMoveElapsedMS << endl;
    out << "Time (ms) for moving back operation : " << opMoveBackElapsedMS << endl;
    out << "Time (ms) for estimating feasibility of the moves : " << opMovePossibleElapsedMS << endl;
    out << "Time (ms) for DTO : " << dynTopSortElapsedMS << endl;
    out << "Time (ms) for updating critical nodes : " << updateCritNodesElapsedMS << endl;
    out << "Time (ms) for calculating the longest paths : " << longestPathsElapsedMS << endl;

    out << " -----------------" << endl;
    out << "Time (ms) for executing blocks (1) : " << objElapsedMS +
		    updateEvalElapsedMS + opSelectionElapsedMS + potentialPositionsSelectionElapsedMS + posSelectionElapsedMS +
		    opMoveElapsedMS + opMoveBackElapsedMS << endl;
    out << "Time (ms) for executing blocks (2) : " << blocksExecElapsedMS << endl;
     */
    out << " -----------------" << endl;
    out << "Time (ms) for running the algorithm : " << totalElapsedMS << endl;
    out << " -----------------" << endl;

    out << "Time percentage for correctness checks : " << double(totalChecksElapsedMS) / double(totalElapsedMS) << endl;
    out << " -----------------" << endl;

}

void LocalSearchPMCP::transitionPM() {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Get the terminal nodes of the graph.
     * 2. Select the most critical in some sense terminal node.
     * 3. Find a critical path to this terminal.
     * 4. Select an operation from the critical path
     * 5. Select a machine which the operation has to be moved to.
     * 6. Try to move the operation to the selected machine
     */

    opSelectionTimer.start();

    if (iter() % critNodesUpdateFreq == 0) {
	pm->updateHeads(topolOrdering);
	pm->updateStartTimes(topolOrdering);
	updateCriticalNodes();
	if (criticalNodes.size() == 0) {
	    pm->updateHeads();
	    pm->updateStartTimes();
	    out << *pm << endl;
	    out << "TWT of the partial schedule : " << TWT()(*pm) << endl;
	    Debugger::err << "LocalSearchPMCP::transitionPM : Failed to find critical nodes!!!" << endl;
	}
    }

    // Check whether there are critical nodes
    if (criticalNodes.size() == 0) {
	pm->updateHeads();
	pm->updateStartTimes();
	out << *pm << endl;
	out << "TWT of the partial schedule : " << TWT()(*pm) << endl;
	Debugger::err << "LocalSearchPMCP::transitionPM : Failed to find critical nodes!!!" << endl;
    }

    //optomove = criticalNodes[Rand::rndInt(0, criticalNodes.size() - 1)];
    optomove = criticalNodes[Rand::rnd<Math::uint32>(0, criticalNodes.size() - 1)];

    opSelectionElapsedMS += opSelectionTimer.elapsed();

    int targetMachID = selectTargetMach(optomove);

    // Select candidate arcs to break
    potentialPositionsSelectionTimer.start();
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs = selectBreakableArcs(targetMachID);
    potentialPositionsSelectionElapsedMS += potentialPositionsSelectionTimer.elapsed();

    // Select an arc to break
    //out << "Selecting arc to break... " << endl;
    posSelectionTimer.start();
    QPair<ListDigraph::Node, ListDigraph::Node> atb = /*selectBestArcToBreak(targetMachID, relarcs, optomove); //*/selectArcToBreak(relarcs, optomove);
    posSelectionElapsedMS += posSelectionTimer.elapsed();
    //out << "Selected arc to break." << endl;

    // ##### IMPORTANT !!! ##### Preserve the states of the operation BEFORE the move

    //out << "Graph before moving the operation ..." << *pm << endl;
    //out << "Moving operation..." << endl;
    moveOper(targetMachID, atb.first, atb.second, optomove);
    //out << "Moved operation " << pm->ops[optomove]->ID << endl;

}

void LocalSearchPMCP::selectOperToMoveCP(const Path<ListDigraph> &cpath, ListDigraph::Node &optomove, QPair<ListDigraph::Node, ListDigraph::Node> &atb) {
    // Find critical arcs on the critical path
    QList<ListDigraph::Arc> carcs;
    ListDigraph::Arc curarc;


    for (int i = 0; i < cpath.length(); i++) {
	curarc = cpath.nth(i);
	if (!pm->conjunctive[curarc] && !pm->conPathExists(pm->graph.source(curarc), pm->graph.target(curarc))) {
	    carcs.append(curarc);
	}
    }

    if (carcs.size() == 0) { // There are no reversible arcs on the path

	//optomove = INVALID;
	//return;

	//curarc = cpath.nth(Rand::rndInt(0, cpath.length() - 1));
	curarc = cpath.nth(Rand::rnd<Math::uint32>(0, cpath.length() - 1));
	optomove = pm->graph.source(curarc);
	atb.first = pm->graph.source(curarc);
	atb.second = pm->graph.target(curarc);

	return;
    }

    // Select randomly some critical arc
    //curarc = carcs[Rand::rndInt(0, carcs.size() - 1)];
    curarc = carcs[Rand::rnd<Math::uint32>(0, carcs.size() - 1)];

    optomove = pm->graph.source(curarc);

    atb.first = pm->graph.target(curarc);

    ListDigraph::Arc arc = INVALID;
    //bool sbarcfound = false;
    bool outarcexists = false;
    // Search schedule-based outgoing arcs
    for (ListDigraph::OutArcIt oait(pm->graph, atb.first); oait != INVALID; ++oait) {
	if (!pm->conjunctive[oait]) {
	    outarcexists = true;

	    arc = oait;

	    if (!pm->conPathExists(pm->graph.source(oait), pm->graph.target(oait))) {
		//sbarcfound = true;
		break;
	    }
	}
    }

    if (!outarcexists) {
	atb.second = INVALID;
	return;
    } else {
	atb.second = pm->graph.target(arc);
    }

}

void LocalSearchPMCP::transitionCP() {
    /** Algorithm:
     * 
     * 1. Get the terminal nodes of the graph.
     * 2. Select the most critical in some sense terminal node.
     * 3. Find a critical path to this terminal.
     * 4. Select an operation from the critical path
     * 5. Select a machine which the operation has to be moved to.
     * 6. Try to move the operation to the selected machine
     */

    // Get the terminals
    //QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath; // Critical path
    QPair<ListDigraph::Node, ListDigraph::Node> atb;

    opSelectionTimer.start();

    QList<ListDigraph::Node> terminals = pm->terminals();

    do {

	// Select a terminal
	theterminal = selectTerminalContrib(terminals);

	// Find a critical path to the selected terminal
	cpath = longestPath(theterminal);

	//Debugger::iDebug("Selecting operation to move...");
	selectOperToMoveCP(cpath, optomove, atb);
	//Debugger::iDebug("Selected operation to move.");


    } while (optomove == INVALID);

    //Debugger::iDebug("Selected operation to move.");
    opSelectionElapsedMS += opSelectionTimer.elapsed();


    //QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs;

    //QPair<ListDigraph::Node, ListDigraph::Node> atb;

    int targetMachID = pm->ops[optomove]->machID;

    //out << "Graph before moving the operation ..." << *pm << endl;
    //out << "Moving operation..." << endl;

    moveOper(targetMachID, atb.first, atb.second, optomove);
    //out << "Moved operation " << pm->ops[optomove]->ID << endl;

    /*
    if (!dag(pm->graph)) {
	    Debugger::err << "LocalSearchPMCP::transitionCP : Graph not DAG!!!" << ENDL;
    }
     */

}

Path<ListDigraph> LocalSearchPMCP::longestPath(const ListDigraph::Node & node) {
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

QList<Path<ListDigraph> > LocalSearchPMCP::longestPaths(const QList<ListDigraph::Node> &nodes) {
    longestPathsTimer.start();

    QList<Path<ListDigraph> > res;

    BellmanFord<ListDigraph, ListDigraph::ArcMap<double> > bf(pm->graph, pm->p);

    bf.init();
    bf.addSource(pm->head);
    //Debugger::info << "Running the BF algorithm..."<<ENDL;
    bf.start();
    //Debugger::info << "Done running the BF algorithm."<<ENDL;

#ifdef DEBUG
    for (int i = 0; i < nodes.size(); i++) {
	if (!bf.reached(nodes[i])) {
	    Debugger::err << "LocalSearch::longestPath : Operation ID= " << pm->ops[nodes[i]]->OID << ":" << pm->ops[nodes[i]]->ID << " can not be reached from the root node " << pm->ops[pm->head]->OID << ":" << pm->ops[pm->head]->ID << "!" << ENDL;
	}
    }
#endif
    for (int i = 0; i < nodes.size(); i++) {
	res.append(bf.path(nodes[i]));
    }

    longestPathsElapsedMS += longestPathsTimer.elapsed();

    return res;
}

QList<ListDigraph::Node> LocalSearchPMCP::criticalPath(const ListDigraph::Node& node) {
    /**
     * 
     * Restore the critical path the the given node based on the critical predecessors
     * 
     */

    QList<ListDigraph::Node> critPath;

    critPath.reserve(countNodes(pm->graph));

    QStack<ListDigraph::Node> stack;

    stack.push(node);

    while (stack.size() > 0) {

	ListDigraph::Node curNode = stack.pop();
	int curOpID = pm->ops[curNode]->ID;

	critPath.prepend(curNode); // Prepending since we are moving backwards

	// Check whether there are any critical predecessors of the node
	QList<int>& curCritPredOpIDs = opID2CPPredOpIDs[curOpID];
	if (curCritPredOpIDs.size() > 0) { // Push a randomly selected critical predecessor to the stack

	    //int curCritPredOpID = curCritPredOpIDs[Rand::rndInt(0, curCritPredOpIDs.size() - 1)];
	    int curCritPredOpID = curCritPredOpIDs[Rand::rnd<Math::uint32>(0, curCritPredOpIDs.size() - 1)];
	    ListDigraph::Node curCritPredNode = opID2Node[curCritPredOpID];

	    stack.push(curCritPredNode);

	} // Dealing with the critical predecessor

    }

    // ############# DEBUG

    /*
    double totalLen = 0.0;
    double curNodeC = pm->ops[node]->c();
    for (int i = 0; i < critPath.size(); i++) {
	    ListDigraph::Node curNode = critPath[i];
	    Operation& critOper = *(pm->ops[curNode]);

	    if (i == 0) {
		    totalLen += critOper.s();
	    }

	    totalLen += critOper.p();
    }

    if (totalLen != curNodeC) {
	    out << "Is : " << totalLen << endl;
	    out << "Should be : " << curNodeC << endl;
	    out << "Terminal ID : " << pm->ops[node]->ID << endl;
	    out << *pm << endl;
	    for (int i = 0; i < critPath.size(); i++) {
		    ListDigraph::Node curNode = critPath[i];
		    Operation& critOper = *(pm->ops[curNode]);
		    out << critOper.ID << "->";
	    }
	    out << endl;
	    Debugger::err << "LocalSearchPMCP::criticalPath : Critical path is not OK!!!" << ENDL;
    }
     */
    // ############# DEBUG	

    return critPath;

}

Path<ListDigraph> LocalSearchPMCP::randomPath(const ListDigraph::Node & node) {
    Path<ListDigraph> res;
    ListDigraph::Node curnode = node;
    QList<ListDigraph::InArcIt> inarcs;

    while (curnode != pm->head) {
	// Select the outgoing arcs from the current node
	inarcs.clear();
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    inarcs.append(iait);
	}

	// Add a random arc to the result
	//res.addFront(inarcs.at(Rand::rndInt(0, inarcs.size() - 1)));
	res.addFront(inarcs.at(Rand::rnd<Math::uint32>(0, inarcs.size() - 1)));

	// Proceed to the next node
	curnode = pm->graph.source(res.front());
    }

    return res;
}

void LocalSearchPMCP::updateCriticalNodes() {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Select the terminal nodes contributing to the optimization criterion
     * 2. Select all operations lying on the critical paths
     * 
     */

    // IMPORTANT!!! It is assumed that the heads and start times are correct!!! Update the predecessors of the nodes lying on the critical path.
    opID2CPPredOpIDs = pm->updateCPPred(topolOrdering);

    QList<ListDigraph::Node> terminals;

    terminals = pm->terminals();

    updateCritNodesTimer.start();

    criticalNodes.clear();

    int termContrib = 0;
    for (int i = 0; i < terminals.size(); i++) {

	ListDigraph::Node curTerm = terminals[i];
	Operation& curTermOp = *(pm->ops[curTerm]);

	if (curTermOp.wT() > 0.0) { // The terminal is contributing

	    termContrib++;

	    // Find the critical path to the terminal
	    QList<ListDigraph::Node> critPath = criticalPath(curTerm);

	    // Filter the already assigned to the machines nodes
	    for (int j = 0; j < critPath.size(); j++) {

		ListDigraph::Node curCritNode = critPath[j];
		Operation& curCritOp = *(pm->ops[curCritNode]);

		if (curCritOp.machID > 0) { // This operation is assigned to some machine

		    if (node2Movable[curCritNode]) { // This operation can be assigned to any alternative machine

			criticalNodes.append(curCritNode);

		    }

		}

	    } // Iterating over the critical nodes

	} // Considering the contributing terminal

    } // Iterating over the terminals

    //out << "Contributing terminals : " << termContrib << endl;

    if (criticalNodes.size() == 0) { // No movable critical nodes have been found -> set all movable nodes as critical (the algorithm will try to move them)

	out << "Iteration : " << iter() << endl;
	Debugger::warn << "LocalSearchPMCP::updateCriticalNodes : No critical nodes found!" << ENDL;
	//getchar();

	for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {

	    ListDigraph::Node curNode = nit;
	    Operation& curOp = *(pm->ops[curNode]);

	    if (curOp.machID > 0 && node2Movable[curNode]) criticalNodes.append(curNode);

	}

    }

    if (termContrib == 0) { // This should not happen since the algorithm must catch this situation

	pm->updateHeads();
	pm->updateStartTimes();
	QTextStream out(stdout);
	out << "Current TWT of the partial schedule : " << TWT()(*pm) << endl;
	Debugger::err << "LocalSearchPMCP::updateCriticalNodes : No contributing terminals!!!" << endl;

    }

    updateCritNodesElapsedMS += updateCritNodesTimer.elapsed();

}

ListDigraph::Node LocalSearchPMCP::selectOperToMove(const Path<ListDigraph> &cpath) {
    // Select only those arcs which are schedule based and NOT conjunctive

    int n = cpath.length();

    QList<ListDigraph::Node> nodes;
    ListDigraph::Node res;

    QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs
    for (int i = 0; i < n; i++) {
	if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based (and therefore the operation is already assigned)

	    schedbased.append(cpath.nth(i));
	    ListDigraph::Arc curArc = cpath.nth(i);

	    ListDigraph::Node curStartNode = pm->graph.source(curArc);
	    ListDigraph::Node curEndNode = pm->graph.target(curArc);

	    if (nodes.count(curStartNode) == 0 && pm->ops[curStartNode]->machID > 0 && node2Movable[curStartNode]) {
		nodes.append(curStartNode);
	    }

	    if (nodes.count(curEndNode) == 0 && pm->ops[curEndNode]->machID > 0 && node2Movable[curEndNode]) {
		nodes.append(curEndNode);
	    }

	} else {
	    /*
	    if (pm->ops[pm->graph.source(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.source(cpath.nth(i))) == 0) {
		    nodes.append(pm->graph.source(cpath.nth(i)));
	    }

	    if (pm->ops[pm->graph.target(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.target(cpath.nth(i))) == 0) {
		    nodes.append(pm->graph.target(cpath.nth(i)));
	    }
	     */
	}

    }

    if (schedbased.size() == 0) {
	return INVALID;
	/*
	for (int i = 0; i < n; i++) {
		if (!pm->conjunctive[cpath.nth(i)]) { // This arc is schedule-based (and therefore the operation is already assigned)
		} else {
			if (pm->ops[pm->graph.source(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.source(cpath.nth(i))) == 0) {
				nodes.append(pm->graph.source(cpath.nth(i)));
			}

			if (pm->ops[pm->graph.target(cpath.nth(i))]->machID > 0 && nodes.count(pm->graph.target(cpath.nth(i))) == 0) {
				nodes.append(pm->graph.target(cpath.nth(i)));
			}
		}

	}
	 */

    }

    //do {
    //res = nodes[Rand::rndInt(0, nodes.size() - 1)];
    res = nodes[Rand::rnd<Math::uint32>(0, nodes.size() - 1)];
    //} while (!scheduledtgs.contains(pm->ops[res]->toolID));

    return res;
}

ListDigraph::Node LocalSearchPMCP::defaultSelectOperToMove(const Path<ListDigraph> &cpath) {
    int n = cpath.length();

    QList<ListDigraph::Node> nodes;
    ListDigraph::Node res;

    QList<ListDigraph::Arc> schedbased; // List of schedule-based arcs

    //for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
    ListDigraph::Arc ait;
    for (int i = 0; i < n; i++) {
	ait = cpath.nth(i);

	ListDigraph::Node curStartNode = pm->graph.source(ait);
	ListDigraph::Node curEndNode = pm->graph.target(ait);

	if (nodes.count(curStartNode) == 0 && pm->ops[curStartNode]->machID > 0 && node2Movable[curStartNode]) {
	    nodes.append(curStartNode);
	}
	if (nodes.count(curEndNode) == 0 && pm->ops[curEndNode]->machID > 0 && node2Movable[curEndNode]) {
	    nodes.append(curEndNode);
	}

    }

    if (nodes.size() == 0) return INVALID;

    /*
    // Select the operation which has a high tardiness
    QMultiMap<double, ListDigraph::Node> tdns2node;

    for (int i = 0; i < nodes.size(); i++) {
	    tdns2node.insert(pm->ops[nodes[i]]->wT(), nodes[i]);
    }
    int k = Rand::rndInt(1, tdns2node.size() / 2);
    QMultiMap<double, ListDigraph::Node>::iterator iter = tdns2node.end();
    for (int j = 0; j < k; j++) {
	    iter--;
    }
    return iter.value();
     */

    //do {
    //res = nodes[Rand::rndInt(0, nodes.size() - 1)];
    res = nodes[Rand::rnd<Math::uint32>(0, nodes.size() - 1)];
    //} while (!scheduledtgs.contains(pm->ops[res]->toolID));

    return res;
}

int LocalSearchPMCP::selectTargetMach(const ListDigraph::Node& optomove) {
    // Get the list of available in the TG machines which are able to execute the operation type
    QList<Machine*> tgmachines = ((*rc)(pm->ops[optomove]->toolID)).machines(pm->ops[optomove]->type);

    /*
    
    // For every machine calculate the total processing time of operations assigned to it
    QHash<int, double> machid2crit;
    Operation* curop;

    // Insert all machines of the tool group
    for (int i = 0; i < tgmachines.size(); i++) {
	    machid2crit[tgmachines[i]->ID] = 0.0;
    }

    // Calculate the CTs
    for (int i = 0; i < topolOrdering.size(); i++) {
	    curop = pm->ops[topolOrdering[i]];
	    if (curop->toolID == pm->ops[optomove]->toolID && curop->machID > 0) {
		    machid2crit[curop->machID] += curop->p(); // = Math::max(machid2crit[curop->machID], curop->w() * curop->c());
	    }
    }

    // Find the machine with the smallest WIP
    double curWIP = Math::MAX_DOUBLE;
    QList<int> machIDs;
    int machID = -1;
    for (QHash<int, double>::iterator iter = machid2crit.begin(); iter != machid2crit.end(); iter++) {
	    if (iter.value() <= curWIP) {
		    machIDs.prepend(iter.key());
		    curWIP = iter.value();
	    }
    }

    while (machIDs.size() > 3) {
	    machIDs.removeLast();
    }

    machID = machIDs[Rand::rndInt(0, machIDs.size() - 1)];

    if (machID == -1) {
	    Debugger::err << "LocalSearchPMCP::selectTargetMach : Failed to find the target machine!" << ENDL;
    }
     */
    // Return an arbitrary machine from the tool group
    //if (iter() % 1 == 0) {
    //return tgmachines[Rand::rndInt(0, tgmachines.size() - 1)]->ID;
    //}
    //return /*machID; //pm->ops[optomove]->machID; //*/ tgmachines[Rand::rndInt(0, tgmachines.size() - 1)]->ID;
    return /*machID; //pm->ops[optomove]->machID; //*/ tgmachines[Rand::rnd<Math::uint32>(0, tgmachines.size() - 1)]->ID;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPMCP::selectRelevantArcs(const Path<ListDigraph>& /*cpath*/, const ListDigraph::Node& node) {
    /** Algorithm:
     * 
     * 1. Iterate over all arcs in the graph
     * 1.1. If the source node of the current arc can be processed by the same tool group
     *	    then add the arc to the list
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    ListDigraph::Node j;
    ListDigraph::Node k;
    bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine

    for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
	//for (int i = 0; i < cpath.length(); i++) {
	//ListDigraph::Arc ait = cpath.nth(i);

	if (!pm->conjunctive[ait]) {

	    if (pm->ops[node]->toolID == pm->ops[pm->graph.source(ait)]->toolID) { // This arc is relevant
		j = pm->graph.source(ait);
		k = pm->graph.target(ait);
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, k));

		// Check whether the arc corresponds to the first two or the last two operations on the machine
		fol = true;
		for (ListDigraph::InArcIt iait(pm->graph, j); iait != INVALID; ++iait) {
		    if (!pm->conjunctive[iait]) {
			fol = false;
			break;
		    }
		}

		if (fol) {
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, j));
		}

		fol = true;
		for (ListDigraph::OutArcIt oait(pm->graph, k); oait != INVALID; ++oait) {
		    if (!pm->conjunctive[oait]) {
			fol = false;
			break;
		    }
		}

		if (fol) {
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (k, INVALID));
		}

	    }
	}
    }

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPMCP::selectRelevantArcsNew(const Path<ListDigraph>& /*cpath*/, const ListDigraph::Node& node) {
    QTextStream out(stdout);
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    ListDigraph::Node j;
    ListDigraph::Node k;
    //bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine

    QHash<int, QVector<ListDigraph::Node> > machid2node;
    QQueue<ListDigraph::Node> q;
    ListDigraph::Node curnode;
    ListDigraph::NodeMap<bool> scheduled(pm->graph, false);
    ListDigraph::NodeMap<bool> available(pm->graph, false);
    //bool predchecked = false;

    ListDigraph::Node suc;
    ListDigraph::Node sucpred;

    q.enqueue(pm->head);
    scheduled[pm->head] = false;
    available[pm->head] = true;

    // Collect operation sequences on every machine of the tool group
    while (q.size() > 0) {
	curnode = q.dequeue();

	if (available[curnode] && !scheduled[curnode]) {
	    if ((pm->ops[curnode]->ID > 0) && (pm->ops[curnode]->toolID == pm->ops[node]->toolID)) {
		machid2node[pm->ops[curnode]->machID].append(curnode);
	    }

	    scheduled[curnode] = true;

	    // Enqueue the successors
	    for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
		suc = pm->graph.target(oait);
		if (!scheduled[suc]) {

		    // Update availability

		    available[suc] = true;
		    for (ListDigraph::InArcIt iait(pm->graph, suc); iait != INVALID; ++iait) {
			sucpred = pm->graph.source(iait);
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

    for (QHash<int, QVector<ListDigraph::Node> >::iterator iter = machid2node.begin(); iter != machid2node.end(); iter++) {

	//	out << "operations on machines : " << endl;
	//	out << "Mach ID : " << iter.key() << " : ";

	for (int i = 0; i < iter.value().size(); i++) {
	    //	    out << pm->ops[iter.value()[i]]->ID << ",";
	}

	//	out << endl << endl;
	//getchar();

	for (int i = 0; i < iter.value().size() - 1; i++) {
	    //for (ListDigraph::OutArcIt oait(pm->graph, iter.value()[i]); oait != INVALID; ++oait) {
	    //if (pm->graph.target(oait) == iter.value()[i + 1] /*&& !pm->conjunctive[oait]*/) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().at(i), iter.value().at(i + 1)));
	    //}
	    //}
	}


	if (iter.value().size() > 0) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, iter.value().first()));
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().last(), INVALID));
	}

    }

    //out << "GBM:" << endl;
    //out << *pm << endl;

    for (int i = 0; i < res.size(); i++) {
	//	out << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	if (!reachable(res[i].first, res[i].second)) {
	    out << "Not reachable : " << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	    //out << *pm << endl;
	    getchar();
	}
    }

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPMCP::selectBreakableArcs(const int& mid) {
    /*
    out << "Selecting breakable arcs..." << endl;
    out << "MID : " << mid << endl;
    out << "Scheduled TGs : " << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    out << scheduledtgs[i] << ",";
    }
    out << endl;
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;

    QList<ListDigraph::Node> trgmachnodes; // Nodes currently on the target machine
    ListDigraph::Node curnode;
    int n = topolOrdering.size();
    QVector<ListDigraph::Node> tord;
    /*
    QList<ListDigraph::Node> testtrgmachnodes; // Nodes currently on the target machine
    QQueue<ListDigraph::Node> q;
    ListDigraph::NodeMap<bool> scheduled(pm->graph, false);
    ListDigraph::NodeMap<bool> available(pm->graph, false);

    ListDigraph::Node suc;
    ListDigraph::Node sucpred;

    q.enqueue(pm->head);
    scheduled[pm->head] = false;
    available[pm->head] = true;
     */

    // Collect operation sequences on the target machine
    /*
    while (q.size() > 0) {
	    curnode = q.dequeue();

	    if (available[curnode] && !scheduled[curnode]) {
		    if ((pm->ops[curnode]->ID > 0) && (pm->ops[curnode]->machID == mid)) {
			    trgmachnodes.append(curnode);
		    }

		    scheduled[curnode] = true;

		    // Enqueue the successors
		    for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
			    suc = pm->graph.target(oait);
			    if (!scheduled[suc]) {

				    // Update availability

				    available[suc] = true;
				    for (ListDigraph::InArcIt iait(pm->graph, suc); iait != INVALID; ++iait) {
					    sucpred = pm->graph.source(iait);
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
     */

    tord = topolOrdering.toVector();

    trgmachnodes.clear();
    trgmachnodes.reserve(topolOrdering.size());

    for (int i = 0; i < n; i++) {
	curnode = tord[i]; //topolOrdering[i];
	if (pm->ops[curnode]->machID == mid) {
	    trgmachnodes.append(curnode);
	}
    }

    //#######################  DEBUG  ##########################################
    /*
    if (testtrgmachnodes.size() != trgmachnodes.size()) {
	    Debugger::err << "Wrong nodes on the target machine!!!" << ENDL;
    }

    for (int i = 0; i < testtrgmachnodes.size(); i++) {
	    if (!trgmachnodes.contains(testtrgmachnodes[i])) {
		    Debugger::err << "Wrong nodes on the target machine (elements test)!!!" << ENDL;
	    }
    }
     */
    //##########################################################################

    res.clear();
    res.reserve(trgmachnodes.size() + 2);

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
	    out << pm->ops[trgmachnodes[k]]->ID << ",";
    }

    out << endl << endl;
     */
    //out << "GBM:" << endl;
    //out << *pm << endl;

    // Check reachability

    /*
    for (int j = 0; j < res.size(); j++) {
	    //	out << pm->ops[res[i].first]->ID << "->" << pm->ops[res[i].second]->ID << endl;
	    if (!reachable(res[j].first, res[j].second)) {
		    out << "Not reachable : " << pm->ops[res[j].first]->ID << "->" << pm->ops[res[j].second]->ID << endl;

		    out << "operations on machine " << mid << " : " << endl;
		    for (int k = 0; k < trgmachnodes.size(); k++) {
			    out << pm->ops[trgmachnodes[k]]->ID << ",";
		    }

		    out << endl << endl;

		    out << *pm << endl;
		    getchar();
	    }
    }
     */
    // #############################################################################

    //out << "Done selecting breakable arcs." << endl;

    return res;
}

QList<QPair<ListDigraph::Node, ListDigraph::Node> > LocalSearchPMCP::selectRelevantArcsFromPath(const Path<ListDigraph> &path, const ListDigraph::Node& node) {
    /** Algorithm:
     * 
     * 1. Iterate over all arcs in the path
     * 2. For every machine from the needed tool group collect the operations to be processed on it
     */

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > res1;

    ListDigraph::Node j;
    ListDigraph::Node k;
    //bool fol = false; // Indicates whether some arc corresponds to the first or the last two operations on some machine
    QMap<int, QVector<ListDigraph::Node> > machid2opers;

    for (int i = 0; i < path.length(); i++) {
	ListDigraph::Arc ait = path.nth(i);
	j = pm->graph.source(ait);

	if (pm->ops[j]->toolID == pm->ops[node]->toolID && pm->ops[j]->machID > 0) {
	    machid2opers[pm->ops[j]->machID].append(j);
	}
    }

    // And the last one
    j = pm->graph.target(path.back());

    if (pm->ops[j]->toolID == pm->ops[node]->toolID && pm->ops[j]->machID > 0) {
	machid2opers[pm->ops[j]->machID].append(j);
    }


    // Build the set of relevant arcs
    for (QMap<int, QVector<ListDigraph::Node> >::iterator iter = machid2opers.begin(); iter != machid2opers.end(); iter++) {

	for (int i = 0; i < iter.value().size() - 1; i++) {
	    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (iter.value().at(i), iter.value().at(i + 1)));
	}

	int prevsize = res.size();

	if (iter.value().size() > 0) {
	    // Check whether insertion can be performed before the first operation
	    j = iter.value().first();
	    for (ListDigraph::InArcIt iait(pm->graph, j); iait != INVALID; ++iait) {
		if (pm->ops[pm->graph.source(iait)]->machID == pm->ops[j]->machID) { // Insertion 
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(iait), j));
		}
	    }
	    if (res.size() == prevsize) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, j));
	    }

	    prevsize = res.size();
	    // Check whether insertion can be performed after the last operation
	    j = iter.value().last();
	    for (ListDigraph::OutArcIt oait(pm->graph, j); oait != INVALID; ++oait) {
		if (pm->ops[pm->graph.target(oait)]->machID == pm->ops[j]->machID) { // Insertion 
		    res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, pm->graph.target(oait)));
		}
	    }
	    if (prevsize == res.size()) {
		res.append(QPair<ListDigraph::Node, ListDigraph::Node > (j, INVALID));
	    }
	}



    }

    // Check the conjunctive arcs. If the start node has an outgoing or the end node has an incoming schedule-base arc =>
    // remove this arc and insert the other two
    bool incluconj = true;
    bool conj = false;
    ListDigraph::Arc arc;
    for (int i = 0; i < res.size(); i++) {

	if (res[i].first == INVALID || res[i].second == INVALID) {
	    res1.append(res[i]);
	    continue;
	}

	conj = false;
	for (ListDigraph::OutArcIt oait(pm->graph, res[i].first); oait != INVALID; ++oait) {
	    if (pm->graph.target(oait) == res[i].second) {
		conj = pm->conjunctive[oait];
		break;
	    }
	}

	if (conj) {
	    incluconj = true;
	    // Search the outgoing arcs for the start node
	    for (ListDigraph::OutArcIt oait(pm->graph, res[i].first); oait != INVALID; ++oait) {
		if (!pm->conjunctive[oait]) {
		    res1.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(oait), pm->graph.target(oait)));
		    incluconj = false;
		    break;
		}
	    }

	    // Search the incoming arcs for the end node
	    for (ListDigraph::InArcIt iait(pm->graph, res[i].second); iait != INVALID; ++iait) {
		if (!pm->conjunctive[iait]) {
		    res1.append(QPair<ListDigraph::Node, ListDigraph::Node > (pm->graph.source(iait), pm->graph.target(iait)));
		    incluconj = false;
		    break;
		}
	    }

	    if (incluconj) {
		res1.append(res[i]);
	    }

	} else {
	    res1.append(res[i]);
	}
    }

    return res1;
}

bool LocalSearchPMCP::moveOperPossible(const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node & node) {
    //QTextStream out(stdout);
    opMovePossibleTimer.start();

    /**
     * We are trying to insert the given node between the start and the end nodes
     * of the specified arcs. The conditions described by Dauzere-Peres and Paulli
     * are checked, since no cycles should occur.
     */

    //if ((node == pm->graph.source(arc)) || (node == pm->graph.target(arc))) return false;

    //if (j == node || k == node) return false;

    /** This means that there are no other operations on the machine and 
     * therefore moving some operation to this machine will result in only 
     * deleting its previous connections in the graph. Thus, no cycles can occur.*/
    if (j == INVALID && k == INVALID) {
	opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
	return true;
    }

    //if (pm->conPathExists(j, k)) return false;

    QList<ListDigraph::Node> fri; // IMPORTANT!!! There can be several routing predecessors or successors of the node i
    QList<ListDigraph::Node> pri;

    // Find the fri and the pri
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	if (pm->conjunctive[oait]) { // The routing based arcs
	    fri.append(pm->graph.target(oait));
	}
    }

    for (ListDigraph::InArcIt iait(pm->graph, node); iait != INVALID; ++iait) {
	if (pm->conjunctive[iait]) { // The routing based arcs
	    pri.append(pm->graph.source(iait));
	}
    }

    if (j != INVALID) {
	for (int i1 = 0; i1 < fri.size(); i1++) {
	    if (j == fri[i1]) {
		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    }
	}
    }

    if (k != INVALID) {
	for (int i2 = 0; i2 < pri.size(); i2++) {
	    if (k == pri[i2]) {
		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    }
	}
    }

    // Check condition (inequalities) with predecessors and successors
    for (int i1 = 0; i1 < fri.size(); i1++) {
	for (int i2 = 0; i2 < pri.size(); i2++) {
	    bool cond1;
	    bool cond2;

	    if (j != INVALID) {
		cond1 = Math::cmp(pm->ops[j]->r(), pm->ops[fri[i1]]->r() + pm->ops[fri[i1]]->p()) == -1;
	    } else {
		cond1 = true;
	    }
	    if (k != INVALID) {
		cond2 = Math::cmp(pm->ops[k]->r() + pm->ops[k]->p(), pm->ops[pri[i2]]->r()) == 1;

		//cout << "Oper " << pm->ops[k]->ID << " r+p =  " << pm->ops[k]->r() + pm->ops[k]->p() << endl;
		//cout << "Oper " << pm->ops[pri[i2]]->ID << " r =  " << pm->ops[pri[i2]]->r() << endl;

	    } else {
		cond2 = true;
	    }

	    if (!(cond1 && cond2)) {
		opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
		return false;
	    } else {
		//out << "Moving " << pm->ops[node]->ID << " between " << ((j!=INVALID) ? pm->ops[j]->ID : -2) << " and " << ((k!=INVALID) ? pm->ops[k]->ID : -1) << endl;
		//out << *pm << endl;
	    }
	}
    }

    opMovePossibleElapsedMS += opMovePossibleTimer.elapsed();
    return true;
}

QPair<ListDigraph::Node, ListDigraph::Node> LocalSearchPMCP::selectArcToBreak(const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node & node) {
    QTextStream out(stdout);
    /** Algorithm:
     
     * 1. Traverse arcs until the needed criterion is satisfied
     * 2. Return the selected arc
     * 
     */

    QPair<ListDigraph::Node, ListDigraph::Node> curarc;
    ListDigraph::Node j;
    ListDigraph::Node k;

    //QSet<int> rndindices;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > modarcs = arcs;
    int idx;
    int lpos = modarcs.size() - 1;

    do {

	if (lpos == -1) {
	    j = INVALID;
	    k = INVALID;
	    if (arcs.size() > 0) {
		out << "Moving operation : " << pm->ops[node]->ID << endl;
		for (int i = 0; i < arcs.size(); i++) {
		    out << ((arcs[i].first == INVALID) ? -1 : pm->ops[arcs[i].first]->ID) << " -> " << ((arcs[i].second == INVALID) ? -1 : pm->ops[arcs[i].second]->ID) << endl;
		}
		Debugger::eDebug("Failed to find other insertion positions!!!");
	    }
	    break;
	}

	// Select the next arc to be considered as a break candidate

	//idx = Rand::rndInt(0, lpos);
	idx = Rand::rnd<Math::uint32>(0, lpos);
	curarc = modarcs[idx];
	modarcs.move(idx, lpos);
	lpos--;

	j = curarc.first;
	k = curarc.second;

    } while (!moveOperPossible(j, k, node));

    return QPair<ListDigraph::Node, ListDigraph::Node > (j, k);
}

QPair<ListDigraph::Node, ListDigraph::Node> LocalSearchPMCP::selectBestArcToBreak(const int& mid, const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node) {
    double bstobj = Math::MAX_DOUBLE;
    double cobj;
    QPair<ListDigraph::Node, ListDigraph::Node> bstmove;

    bstmove.first = INVALID;
    bstmove.second = INVALID;

    for (int j = 0; j < arcs.size(); j++) {
	if (moveOperPossible(arcs[j].first, arcs[j].second, node)) {

	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "LocalSearchPMCP::selectBestArcToBreak : Graph BEFORE moving the operation: " << endl << *pm << endl;
	    //out << "Moving operation : " << *pm->ops[optomove] << endl;
	    //out << "Moving between : " << ((arcs[j].first != INVALID) ? pm->ops[arcs[j].first]->ID : -1) << " and " << ((arcs[j].second != INVALID) ? pm->ops[arcs[j].second]->ID : -1) << endl;

	    // Try to move the operation 
	    moveOper(mid, arcs[j].first, arcs[j].second, node);

	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "Moved operation : " << *pm->ops[optomove] << endl;

	    //out << "LocalSearchPMCP::selectBestArcToBreak : Graph AFTER moving the operation: " << endl << *pm << endl;
	    /*
	    if (!dag(pm->graph)) {
		    Debugger::err << "LocalSearchPMCP::selectBestArcToBreak : Graph not DAG after operation move!!!" << ENDL;
	    }
	     */

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();
	    updateEval(nodeI, nodeT);

	    // Calculate the current objective
	    objTimer.start();
	    cobj = obj(*pm, pm->terminals());
	    objElapsedMS += objTimer.elapsed();

	    //out << "bstobj = " << bstobj << endl;
	    //out << "cobj = " << cobj << endl;

	    // Check whether the move is the best
	    if (bstobj >= cobj) { // This move is not the worse one
		bstobj = cobj;
		bstmove = arcs[j];

		//out << "Best objective found when moving to machine " << bstmachid << endl;
	    }

	    // Move back the operation
	    moveBackOper(node);

	    // Update the graph
	    //pm->updateHeads();
	    //pm->updateStartTimes();

	    //out << "LocalSearchPMCP::selectBestArcToBreak : Graph after moving BACK the operation: " << endl << *pm << endl;

	    // IMPORTANT!!! Restore only if the graph has changed since the last move

	    /*
	    if (!prevRS.empty()) {
		    ListDigraph::Node curnode;
		    int n = topolOrdering.size(); //topolSorted.size();
		    //for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
		    for (int j = topolITStart; j < n; j++) {
			    curnode = topolOrdering[j]; //topolSorted[j];
			    pm->ops[curnode]->r(prevRS[curnode].first);
			    pm->ops[curnode]->s(prevRS[curnode].second);

			    //out << "Restoring for : " << pm->ops[curnode]->ID << endl;
			    //out << "r = ( " << pm->ops[curnode]->r() << " , " << prevRS[curnode].first << " ) " << endl;
			    //out << "s = ( " << pm->ops[curnode]->s() << " , " << prevRS[curnode].second << " ) " << endl;


			    //if (Math::cmp(pm->ops[curnode]->r(), prevRS[curnode].first, 0.0001) != 0) {
			    //Debugger::err << "LocalSearchPMCP::selectBestArcToBreak : Something is wrong with r while restoring!!!" << ENDL;
			    //}

			    //if (Math::cmp(pm->ops[curnode]->s(), prevRS[curnode].second, 0.0001) != 0) {
			    //Debugger::err << "LocalSearchPMCP::selectBestArcToBreak : Something is wrong with s while restoring!!!" << ENDL;
			    //}


		    }

	    }

	     */

	}
    }

    //out << "Found best move : " << ((bstmove.first != INVALID) ? pm->ops[bstmove.first]->ID : -1) << " and " << ((bstmove.second != INVALID) ? pm->ops[bstmove.second]->ID : -1) << endl;

    return bstmove;
}

void LocalSearchPMCP::findBestOperMove(const ListDigraph::Node& optm, int& targetMachID, QPair<ListDigraph::Node, ListDigraph::Node>& atb) {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Get the list of machines able to process the operation
     * 2. For every machine:
     * 2.1. Select pairs of operations between which the operation can be moved (Collect the move possibilities)
     * 3. For every selected move possibility estimate the total objective.
     * 4. Return the best possible move possibility
     */

    QHash< int, QList<QPair<ListDigraph::Node, ListDigraph::Node> > > machid2arcs;
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > breakablearcs;

    // Get the list of available in the TG machines
    QList<Machine*> tgmachines = ((*rc)(pm->ops[optm]->toolID)).machines();

    // Iterate over the machines of the relative tool group
    for (int machidx = 0; machidx < tgmachines.size(); machidx++) {

	// Select potential insertion positions on the current machine
	breakablearcs = selectBreakableArcs(tgmachines[machidx]->ID);

	for (int j = 0; j < breakablearcs.size(); j++) {
	    if (moveOperPossible(breakablearcs[j].first, breakablearcs[j].second, optm)) {
		machid2arcs[tgmachines[machidx]->ID].append(breakablearcs[j]);
	    }
	}

	// In case the machine is empty
	if (machid2arcs[tgmachines[machidx]->ID].size() == 0) {
	    machid2arcs[tgmachines[machidx]->ID].append(QPair<ListDigraph::Node, ListDigraph::Node > (INVALID, INVALID));
	}

	//out << "Found breakable arcs for machine " << tgmachines[machidx]->ID << " : " << machid2arcs[tgmachines[machidx]->ID].size() << endl;
    }

    // Iterate over the possible moves and select the best move possible
    double bstobj = Math::MAX_DOUBLE;
    double cobj;
    int bstmachid = -1;
    QPair<ListDigraph::Node, ListDigraph::Node> bstmove;


    for (QHash< int, QList<QPair<ListDigraph::Node, ListDigraph::Node> > >::iterator iter = machid2arcs.begin(); iter != machid2arcs.end(); iter++) {
	for (int j = 0; j < iter.value().size(); j++) {
	    // Try to move the operation 
	    moveOper(iter.key(), iter.value()[j].first, iter.value()[j].second, optm);

	    // Update the graph
	    pm->updateHeads();
	    pm->updateStartTimes();

	    // Calculate the current objective
	    cobj = obj(*pm, pm->terminals());

	    //out << "bstobj = " << bstobj << endl;
	    //out << "cobj = " << cobj << endl;

	    // Check whether the move is the best
	    if (bstobj >= cobj) { // This move is not the worse one
		bstobj = cobj;
		bstmachid = iter.key();
		bstmove = iter.value()[j];

		//out << "Best objective found when moving to machine " << bstmachid << endl;
	    }

	    // Move back the operation
	    moveBackOper(optm);

	    // Update the graph
	    pm->updateHeads();
	    pm->updateStartTimes();

	}
    }

    // Return the best found potential move
    if (bstmachid == -1) {
	out << "Moving operation " << pm->ops[optm]->ID << endl;
	out << *pm << endl;
	Debugger::err << "LocalSearchPMCP::findBestOperMove : failed to find the best move!" << ENDL;
    } else {
	targetMachID = bstmachid;
	atb = bstmove;
    }

}

void LocalSearchPMCP::moveOper(const int& mid, const ListDigraph::Node &jNode, const ListDigraph::Node &kNode, const ListDigraph::Node& node) {
    opMoveTimer.start();

    /** Algorithm:
     * 
     * 1. Remove schedule-based arcs of the specified node
     * 2. Insert a single arc connecting the nodes which were preceding and
     *	  succeeding the node in the schedule
     * 3. Remove the specified arc
     * 4. Insert the node between the starting an the ending nodes of the 
     *    specified arc
     * 
     */

    //out << "PM before moving operation : " << pm->ops[node]->ID << endl << *pm << endl;

    //###########################  DEBUG  ######################################

    /*
    out << "Moving operation : " << pm->ops[node]->ID << endl;

    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;

			    for (int l = 0; l < topolOrdering.size(); l++) {
				    out << pm->ops[topolOrdering[l]]->ID << " ";
			    }
			    out << endl;

			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct before moving the operation!!!" << ENDL;
		    }
	    }
    }
     */

    /*
    out << *pm << endl;

    //debugCheckPMCorrectness("LocalSearchPMCP::moveOper : Before moving the next operation.");

    if (j != INVALID && k != INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << pm->ops[j]->ID << " and " << pm->ops[k]->ID << endl;
    }

    if (j != INVALID && k == INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << pm->ops[j]->ID << " and " << " * " << endl;
    }

    if (j == INVALID && k != INVALID) {
	    out << "Moving " << pm->ops[node]->ID << " between " << " * " << " and " << pm->ops[k]->ID << endl;
    }
     */

    //##########################################################################

    ListDigraph::Node sNode = INVALID;
    ListDigraph::Node tNode = INVALID;
    ListDigraph::Arc siArc = INVALID;
    ListDigraph::Arc itArc = INVALID;
    ListDigraph::Arc stArc = INVALID;
    ListDigraph::Arc jiArc = INVALID;
    ListDigraph::Arc ikArc = INVALID;

    arcsRem.clear();
    arcsIns.clear();
    weightsRem.clear();

    // IMPORTANT!!!
    //topolSorted.clear();
    prevRS.clear();

    if ((node == jNode) || (node == kNode)) { // The operation is not moved
	remMachID = pm->ops[node]->machID;

	arcsRem.clear();
	arcsIns.clear();
	weightsRem.clear();

	// No need to perform topological sorting since the graph stays unchanged
	// IMPORTANT!!! Clear the old topol. sorting so that the preserved ready times and start times are not restored incorrectly
	//topolSorted.clear();
	prevRS.clear();

	// IMPORTANT!!! Set the actual topological ordering! Else the incorrect TO is restored
	prevTopolOrdering = topolOrdering;
	prevTopolITStart = topolITStart;

	//out << "###################   Operation is not moved!" << endl;
	return;
    }

    //if (j == INVALID && k != INVALID) out << "#####################  Moving to the front." << endl;
    //if (j != INVALID && k == INVALID) out << "#####################  Moving to the back." << endl;
    //if (j != INVALID && k != INVALID) out << "#####################  Moving in the middle." << endl;
    //if (j == INVALID && k == INVALID) out << "#####################  Moving to the empty machine." << endl;

    // Find the fri and the pri
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	if (!pm->conjunctive[oait]) { // The schedule-based arcs
	    tNode = pm->graph.target(oait);
	    itArc = oait;
	    break;
	}

	// If there is no schedule-based arc then the routing-based successor might come into consideration
    }

    for (ListDigraph::InArcIt iait(pm->graph, node); iait != INVALID; ++iait) {
	if (!pm->conjunctive[iait]) { // The schedule-based arcs
	    sNode = pm->graph.source(iait);
	    siArc = iait;
	    break;
	}
    }

    //Debugger::iDebug("Removing previous connections...");
    // Remove the former connections
    if (sNode != INVALID) {
	arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (sNode, node));
	weightsRem.append(pm->p[siArc]);

	//out << "Erasing " << pm->ops[s]->ID << " -> " << pm->ops[node]->ID << endl;

	pm->graph.erase(siArc);
    }

    //###########################  DEBUG  ######################################

    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing si!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    if (tNode != INVALID) {
	arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (node, tNode));
	weightsRem.append(pm->p[itArc]);

	//out << "Erasing " << pm->ops[node]->ID << " -> " << pm->ops[t]->ID << endl;

	pm->graph.erase(itArc);
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing it!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    // Insert the direct connection between s and t
    if (sNode != INVALID && tNode != INVALID /*&& !pm->conPathExists(s, t)*/) {
	stArc = pm->graph.addArc(sNode, tNode);
	arcsIns.append(stArc);
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after inserting st!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    // Remove the arc to break (if this arc exists)
    if (jNode != INVALID && kNode != INVALID) {
	for (ListDigraph::OutArcIt oait(pm->graph, jNode); oait != INVALID; ++oait)
	    if (pm->graph.target(oait) == kNode) {
		if (!pm->conjunctive[oait]) {
		    arcsRem.append(QPair<ListDigraph::Node, ListDigraph::Node > (jNode, kNode));
		    weightsRem.append(/*-pm->ops[j]->p()*/pm->p[oait]);

		    pm->graph.erase(oait);
		    break;
		}
	    }
    }

    //###########################  DEBUG  ######################################
    /*
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    //out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological ordering is not correct after removing jk!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    //Debugger::iDebug("Removed previous connections.");


    //Debugger::iDebug("Inserting new connections...");

    // Insert the new connections
    if (jNode != INVALID /*&& !pm->conPathExists(j, node)*/) {
	jiArc = pm->graph.addArc(jNode, node);
	arcsIns.append(jiArc);
    } else {
	jiArc = INVALID;
    }
    if (kNode != INVALID /*&& !pm->conPathExists(node, k)*/) {
	ikArc = pm->graph.addArc(node, kNode);
	arcsIns.append(ikArc);
    } else {
	ikArc = INVALID;
    }

    //Debugger::iDebug("Inserted new connections.");

    // Update the topological ordering of the graph dynamically

    // Save the current topological ordering
    prevTopolOrdering = topolOrdering;
    prevTopolITStart = topolITStart;

    // Update the topological ordering
    dynTopSortTimer.start();
    //out << "Performing DTO..." << endl;
    /*
    if (!dag(pm->graph)) {
	    Debugger::err << "Graph is not DAG before the DTO!" << ENDL;
    }
     */
    dynUpdateTopolOrdering(topolOrdering, node, jNode, kNode);
    //out << "Done DTO." << endl;
    dynTopSortElapsedMS += dynTopSortTimer.elapsed();

    // Update the recalculation region
    int idxt = topolOrdering.indexOf(tNode);
    int idxi = topolOrdering.indexOf(node);
    if (idxt >= 0 && idxi >= 0) {
	topolITStart = Math::min(idxt, idxi);
    } else {
	topolITStart = Math::max(idxt, idxi);
    }


    /*
	    // Perform topological sorting of the nodes reachable from i and/or t in the NEW graph G-tilde
	    QList<ListDigraph::Node> startSet;
	    startSet.append(t);
	    startSet.append(node);
     */

    // Sort topologically all nodes reachable from i and/or from t
    //out << "Performing topological sorting..." << endl;
    topSortTimer.start();
    //QList<ListDigraph::Node> reachable = pm->reachableFrom(startSet);
    /*
    int idxt = topolOrdering.indexOf(t);
    int idxi = topolOrdering.indexOf(node);
    int startidx;
    if (idxt >= 0 && idxi >= 0) {
	    startidx = Math::min(idxt, idxi);
    } else {
	    startidx = Math::max(idxt, idxi);
    }

    topolSorted = topolOrdering.mid(startidx, topolOrdering.size() - startidx);
     */
    topSortElapsedMS += topSortTimer.elapsed();
    //out << "Performed topological sorting." << endl;

    //######################  DEBUG  ###########################################

    // Check the correctness of the topological sorting
    /*
    out << "Topological sorting : " << endl;
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    out << pm->ops[topolSorted[i]]->ID << " ";
    }
    out << endl;
     */


    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    for (int j = i + 1; j < topolSorted.size(); j++) {
		    if (reachable(topolSorted[j], topolSorted[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolSorted[j]]->ID << " -> " << pm->ops[topolSorted[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct!!!" << ENDL;
		    }
	    }
    }
     */

    //out << "PM before preserving : " << *pm << endl;

    //##########################################################################

    // Preserve the ready times and the start times of the previous graph for the topologically sorted nodes
    Operation *curop = NULL;
    int n = topolOrdering.size(); //topolSorted.size();
    //for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
    for (int j = topolITStart/*0*/; j < n; j++) {
	curop = pm->ops[topolOrdering[j]];
	// Preserve the former value => WRONG!!!
	//out << "Preserving for : " << pm->ops[curnode]->ID << endl;
	//out << "r = " << pm->ops[curnode]->r() << endl;
	//out << "s = " << pm->ops[curnode]->s() << endl;
	prevRS[curop->ID].first = curop->r();
	prevRS[curop->ID].second = curop->s();
    }


    //Debugger::iDebug("Updating the data of the newly inserted operation...");

    // Update the machine id of the moved operation and preserve the previous assignment ID
    remMachID = pm->ops[node]->machID;
    pm->ops[node]->machID = mid;
    /*
    if (j != INVALID) {
	    pm->ops[node]->machID = pm->ops[j]->machID;
    } else {
	    if (k != INVALID) {
		    pm->ops[node]->machID = pm->ops[k]->machID;
	    } else {
		    Debugger::eDebug("LocalSearchPMCP::moveOper : Moving operation between two invalid operations!");
	    }
    }
     */

    //Debugger::iDebug("Updating the proc. time of the newly inserted operation...");
    // Update the processing time of the moved operation
    pm->ops[node]->p(((*rc)(pm->ops[node]->toolID, pm->ops[node]->machID)).procTime(pm->ops[node]));
    //Debugger::iDebug("Updated the proc. time of the newly inserted operation.");

    //Debugger::iDebug("Setting the weights of the newly inserted arcs...");
    // Set the weights of the newly inserted arcs
    //Debugger::iDebug("st...");
    if (stArc != INVALID) {
	pm->p[stArc] = -pm->ops[sNode]->p();
    }
    //Debugger::iDebug("st.");
    if (jiArc != INVALID) {
	pm->p[jiArc] = -pm->ops[jNode]->p();
    }
    //Debugger::iDebug("Set the weights of the newly inserted arcs.");

    //Debugger::iDebug("Recalculating the processing time of the moved operation...");
    // Processing time for the moved operation must be updated
    if (ikArc != INVALID) {
	pm->p[ikArc] = -pm->ops[node]->p();
    }
    //Debugger::iDebug("Recalculated the processing time of the moved operation.");

    // Update length of all arcs going out from i
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	pm->p[oait] = -pm->ops[node]->p();
    }

    //Debugger::iDebug("Updated the data of the newly inserted operation.");

    // Set the outgoing nodes for the update
    nodeI = node;
    nodeT = tNode;

    opMoveElapsedMS += opMoveTimer.elapsed();
}

void LocalSearchPMCP::moveBackOper(const ListDigraph::Node & node) {
    opMoveBackTimer.start();
    //out << "Moving back operation : " << pm->ops[node]->ID << endl;

    // Remove the newly inserted arcs
    for (int i = 0; i < arcsIns.size(); i++) {
	pm->graph.erase(arcsIns[i]);
    }
    arcsIns.clear();

    // Insert the previous arcs
    ListDigraph::Arc curarc;
    for (int i = 0; i < arcsRem.size(); i++) {
	curarc = pm->graph.addArc(arcsRem[i].first, arcsRem[i].second);
	pm->p[curarc] = weightsRem[i];

	//out << "Restored " << pm->ops[arcsRem[i].first]->ID << " -> " << pm->ops[arcsRem[i].second]->ID << endl;

	/*
	if (pm->graph.source(curarc) == node) {
		pm->ops[node]->machID = pm->ops[pm->graph.target(curarc)]->machID;
	} else {
		if (pm->graph.target(curarc) == node) {
			pm->ops[node]->machID = pm->ops[pm->graph.source(curarc)]->machID;
		}
	}
	 */
    }

    // Restore the machine assignment of the operation
    pm->ops[node]->machID = remMachID;

    // Restore the processing time of the moved operation
    pm->ops[node]->p(((*rc)(pm->ops[node]->toolID, pm->ops[node]->machID)).procTime(pm->ops[node]));

    // Restore arc lengths of the arcs coming out from i
    for (ListDigraph::OutArcIt oait(pm->graph, node); oait != INVALID; ++oait) {
	pm->p[oait] = -pm->ops[node]->p();
    }

    arcsRem.clear();
    weightsRem.clear();

    //out << "PM after moving Back operation : " << pm->ops[node]->ID << endl << *pm << endl;


    // Restore the ready times and the due dates if the graph has changed
    // IMPORTANT!!! Update only if the graph has been changed!!!
    // IMPORTANT!!! Restor r and s BEFORE the old topological ordering is restored
    if (!prevRS.empty()) {
	Operation *curop;
	int n = topolOrdering.size(); //topolSorted.size();
	//for (ListDigraph::NodeIt curnode(pm->graph); curnode != INVALID; ++curnode) {
	for (int j = topolITStart; j < n; j++) {
	    curop = pm->ops[topolOrdering[j]];
	    curop->r(prevRS[curop->ID].first);
	    curop->s(prevRS[curop->ID].second);

	    //out << "Restoring for : " << pm->ops[curnode]->ID << endl;
	    //out << "r = ( " << pm->ops[curnode]->r() << " , " << prevRS[curnode].first << " ) " << endl;
	    //out << "s = ( " << pm->ops[curnode]->s() << " , " << prevRS[curnode].second << " ) " << endl;


	    //if (Math::cmp(pm->ops[curnode]->r(), prevRS[curnode].first, 0.0001) != 0) {
	    //Debugger::err << "Something is wrong with r while restoring!!!" << ENDL;
	    //}

	    //if (Math::cmp(pm->ops[curnode]->s(), prevRS[curnode].second, 0.0001) != 0) {
	    //Debugger::err << "Something is wrong with s while restoring!!!" << ENDL;
	    //}

	}
    }

    // Restore the previous topological ordering of the nodes 
    topolOrdering = prevTopolOrdering;
    topolITStart = prevTopolITStart;

    // #########################    DEBUG    ###################################    

    /*
    out << "Check reachability for every machine after moving BACK the operation..." << endl;
    for (int i = 0; i < scheduledtgs.size(); i++) {
	    for (int j = 0; j < ((*rc)(scheduledtgs[i])).machines().size(); j++) {
		    out << "Mach: " << ((*rc)(scheduledtgs[i])).machines().at(j)->ID << "," << endl;
		    selectBreakableArcs(((*rc)(scheduledtgs[i])).machines().at(j)->ID);
	    }
    }
    out << "Done checking reachability." << endl;
     */

    // For every node check whether processing times of the operation equals the length of the arc
    /*
    out << "Checking consistency after moving back the operation..."<<endl;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	    for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
		    if (pm->ops[nit]->p() != -pm->p[oait]) {
			    out << "op ID = " << pm->ops[nit]->ID << endl;
			    out << *pm << endl;
			    Debugger::err << "LocalSearchPMCP::stepActions : processing time does not equal the arc length!!! " << ENDL;
		    }
	    }
    }
     */
    // #########################################################################
    opMoveBackElapsedMS += opMoveBackTimer.elapsed();
}

ListDigraph::Node LocalSearchPMCP::selectTerminalContrib(QList<ListDigraph::Node> &terminals) {
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

ListDigraph::Node LocalSearchPMCP::selectTerminalRnd(QList<ListDigraph::Node> &terminals) {

    //return terminals[Rand::rndInt(0, terminals.size() - 1)];
    return terminals[Rand::rnd<Math::uint32>(0, terminals.size() - 1)];
}

ListDigraph::Node LocalSearchPMCP::selectTerminalNonContrib(QList<ListDigraph::Node> &terminals) {
    QList<QPair<QPair<double, double>, ListDigraph::Node > > interval2node;
    double totalobj = 0.0;
    double istart;
    double iend;
    ListDigraph::Node res = INVALID;
    double maxtwt = 0.0;

    // Find the biggest weighted tardiness
    for (int i = 0; i < terminals.size(); i++) {
	maxtwt = Math::max(maxtwt, pm->ops[terminals[i]]->wT());
    }

    for (int i = 0; i < terminals.size(); i++) {
	istart = totalobj;
	totalobj += maxtwt - pm->ops[terminals[i]]->wT(); // The bigger the contribution the smaller the probability is
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

void LocalSearchPMCP::diversify() {
    //Debugger::info << "LocalSearchPMCP::diversify ... " << ENDL;
    //return;
    /** Algorithm:
     * 
     * 1. Select the random number of arcs to be reversed
     * 
     * 2. Reverse randomly the selected number of critical arcs 
     * 
     */

    pm->restore();
    topolOrdering = pm->topolSort();
    topolITStart = 0;
    pm->updateHeads(topolOrdering);
    pm->updateStartTimes(topolOrdering);
    curobj = obj(*pm, pm->terminals());

    updateCriticalNodes();

    //int nops2move = Rand::rndInt(5, 10);
    int nops2move = Rand::rnd<Math::uint32>(5, 10);
    int nopsmoved = 0;

    // Get the terminals
    QList<ListDigraph::Node> terminals = pm->terminals();

    ListDigraph::Node theterminal;
    Path<ListDigraph> cpath;
    ListDigraph::Node cop;

    //out <<"PM before the diversification:"<<endl;
    //out << *pm << endl;
    //getchar();

    do {

	do {
	    // Select some terminal for the manipulations (based on the contribution of the terminal to the objective)

	    theterminal = selectTerminalContrib(terminals);

	    // Find a critical path to the selected terminal
	    cpath = /*randomPath(theterminal); //*/longestPath(theterminal);

	    // Select operation to move
	    cop = defaultSelectOperToMove(cpath);
	    //cop = criticalNodes[Rand::rndInt(0, criticalNodes.size() - 1)];


	} while (cop == INVALID);

	int targetMachID = selectTargetMach(cop);

	QList<QPair<ListDigraph::Node, ListDigraph::Node> > relarcs = selectBreakableArcs(targetMachID);

	// Select an arc to break
	QPair<ListDigraph::Node, ListDigraph::Node> atb = selectArcToBreak(relarcs, cop);

	// Move the operation
	moveOper(targetMachID, atb.first, atb.second, cop);

	//if (!dag(pm->graph)) moveBackOper(cop);

	// Update the ready times and the start times of the operations in the graph
	pm->updateHeads(topolOrdering);
	pm->updateStartTimes(topolOrdering);

	nopsmoved++;


	//out <<"PM after the first step of diversification:"<<endl;
	//out << *pm << endl;
	//getchar();


	if (obj(*pm, pm->terminals()) < bestobj) {
	    pm->save();
	    curobj = obj(*pm, pm->terminals());
	    prevobj = curobj;
	    bestobj = curobj;
	    nisteps = 0;

	    updateCriticalNodes();
	    //break;
	}


    } while (nopsmoved < nops2move);

    //if (objimprov(*pm, pm->terminals()) < bestobjimprov) {
    //pm->save();
    curobj = obj(*pm, pm->terminals());
    prevobj = curobj; // + 0.00000000001;
    //bestobj = curobj;
    nisteps = 0;
    //bestobjimprov = Math::MAX_DOUBLE;
    //prevobjimprov = Math::MAX_DOUBLE;
    //}

    updateCriticalNodes();

    //Debugger::info << "LocalSearchPMCP::diversify : Finished. " << ENDL;

}

void LocalSearchPMCP::updateEval(const ListDigraph::Node& /*iNode*/, const ListDigraph::Node& /*tNode*/) {
    updateEvalTimer.start();

    /** Algorithm:
     * 
     * 1. Collect nodes reachable from i
     * 2. Collect nodes reachable from t
     * 3. The union of these sets is the set of nodes to be updated for the evaluation
     * 4. If i is reachable from t then start updating from t
     *	  If t is reachable from i then start updating from i
     *    Else update starting from i and from t	
     * 
     */

    //out << "Running updateEval..." << endl;

    // For the topologically sorted nodes update the ready times and the start times of the operations (preserving the former values)

    // Update the ready times
    ListDigraph::Node curnode;
    ListDigraph::Node prevnode;
    double curR; // The calculated ready time of the current node

    int n = topolOrdering.size();

    for (int j = topolITStart; j < n; j++) {
	curnode = topolOrdering[j];

	curR = 0.0;
	// Iterate over all predecessors of the current node
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    prevnode = pm->graph.source(iait);
	    curR = Math::max(curR, pm->ops[prevnode]->r() + pm->ops[prevnode]->p());
	}

	//############################  DEBUG  #################################
	//out << "r = ( " << pm->ops[curnode]->r() << " , " << curR << " ) " << endl;

	/*
	if (pm->ops[curnode]->r() != curR) {
		out << "Current node ID = " << pm->ops[curnode]->ID << endl;
		out << *pm << endl;
		Debugger::err << "Something is wrong with r !!!" << ENDL;
	}
	 */
	//######################################################################

	pm->ops[curnode]->r(curR/*, false*/);

    }

    // Update the start times
    double curS; // The calculated start time of the current node

    for (int j = topolITStart; j < n; j++) {
	curnode = topolOrdering[j];

	curS = Math::max(pm->ops[curnode]->ir(), pm->ops[curnode]->r());
	// Iterate over all predecessors of the current node
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    prevnode = pm->graph.source(iait);
	    curS = Math::max(curS, pm->ops[prevnode]->c());
	}

	//############################  DEBUG  #################################

	/*
	//out << "s = ( " << pm->ops[curnode]->s() << " , " << curS << " ) " << endl;
	if (pm->ops[curnode]->s() != curS) {
		Debugger::err << "Something is wrong with s !!!" << ENDL;
	}
	 */
	//######################################################################

	// Take into account the machine's availability time
	curS = Math::max(curS, pm->ops[curnode]->machAvailTime());

	// Seth the start time of the operation
	pm->ops[curnode]->s(curS);
    }

    //out << "Running updateEval done." << endl;

    //######################  DEBUG  ###########################################

    // Check the correctness of the topological sorting

    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    out << pm->ops[topolSorted[i]]->ID << " ";
    }
    out << endl;
     */

    /*
    for (int i = 0; i < topolSorted.size() - 1; i++) {
	    for (int j = i + 1; j < topolSorted.size(); j++) {
		    if (reachable(topolSorted[j], topolSorted[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolSorted[j]]->ID << " -> " << pm->ops[topolSorted[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct!!!" << ENDL;
		    }
	    }
    }
     */
    //##########################################################################

    updateEvalElapsedMS += updateEvalTimer.elapsed();
}

void LocalSearchPMCP::dynUpdateTopolOrdering(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &j, const ListDigraph::Node &k) {
    QTextStream out(stdout);
    /** Algorithm:
     * 
     * 1. Find the positions of i, j and k
     * 2. IF posj < posi < posk => no changes to the topological sorting need to be performed. Return.
     * 3. IF posi > posk => reorder the nodes. The affected region is [posi, posk]. Return.
     * 4. IF posi < posj => reorder the nodes. The affected region is [posj, posi]. Return.
     * 
     */

    Math::intUNI posj = -1;
    Math::intUNI posi = -1;
    Math::intUNI posk = -1;

    if (j == INVALID) {
	posj = -1;
    } else {
	posj = topolOrdering.indexOf(j);
    }

    if (k == INVALID) {
	posk = Math::MAX_INTUNI;
    } else {
	posk = topolOrdering.indexOf(k);
    }

    posi = topolOrdering.indexOf(i);

    if (posj < posi && posi < posk) { // No changes to perform
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

    if (posj >= posk) {
	out << "posj = " << posj << " ID = " << ((j == INVALID) ? -1 : pm->ops[j]->ID) << endl;
	out << "posi = " << posi << " ID = " << ((i == INVALID) ? -1 : pm->ops[i]->ID) << endl;
	out << "posk = " << posk << " ID = " << ((k == INVALID) ? -1 : pm->ops[k]->ID) << endl;

	for (Math::intUNI l = 0; l < topolOrdering.size(); l++) {
	    out << pm->ops[topolOrdering[l]]->ID << " ";
	}
	out << endl;

	Debugger::err << "LocalSearchPMCP::dynUpdateTopolOrdering : posj >= posk which is impossible!!!" << ENDL;
    }

    // Find the affected region
    Math::intUNI arbegin = -1;
    Math::intUNI arend = -1;
    ListDigraph::Node arstartnode = INVALID;
    ListDigraph::Node arendnode = INVALID;

    if (posi < posj) {
	arbegin = posi;
	arend = posj;
	arstartnode = i;
	arendnode = j;
    }

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

    for (Math::intUNI l = 0; l < ar.size(); l++) {
	visited.append(false);
    }

    q.clear();
    q.enqueue(arstartnode);

    deltaF.append(arstartnode);
    while (q.size() != 0) {
	curnode = q.dequeue();

	// Check the successors of the current node
	for (ListDigraph::OutArcIt oait(pm->graph, curnode); oait != INVALID; ++oait) {
	    tmpnode = pm->graph.target(oait);

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
    Math::intUNI posF = ar.size() - 1;

    // Move elements in deltaF to the right
    while (!deltaF.isEmpty()) {
	// Find the first element in ar starting from posB that is in deltaB
	tmpidx = -1;
	for (Math::intUNI l = posF; l >= 0; l--) {
	    if (deltaF.contains(ar[l])) {
		tmpidx = l;
		break;
	    }
	}

	if (tmpidx == -1) {
	    if (j != INVALID && k != INVALID) {
		out << "Moving " << pm->ops[i]->ID << " between " << pm->ops[j]->ID << " and " << pm->ops[k]->ID << endl;
	    }

	    if (j != INVALID && k == INVALID) {
		out << "Moving " << pm->ops[i]->ID << " between " << pm->ops[j]->ID << " and " << " * " << endl;
	    }

	    if (j == INVALID && k != INVALID) {
		out << "Moving " << pm->ops[i]->ID << " between " << " * " << " and " << pm->ops[k]->ID << endl;
	    }

	    out << *pm << endl;
	    Debugger::err << "LocalSearchPMCP::dynUpdateTopolOrdering : tmpidx = -1 while shifting deltaF. Probably the graph is NOT DAG! " << ENDL;
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
    for (Math::intUNI l = 0; l < ar.size(); l++) {
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
    for (int i = 0; i < topolOrdering.size() - 1; i++) {
	    for (int j = i + 1; j < topolOrdering.size(); j++) {
		    if (reachable(topolOrdering[j], topolOrdering[i])) {
			    out << *pm << endl;
			    out << pm->ops[topolOrdering[j]]->ID << " -> " << pm->ops[topolOrdering[i]]->ID << endl;
			    Debugger::err << "Topological sorting is not correct after DTO!!!" << ENDL;
		    }
	    }
    }
     */

    //getchar();

    //##########################################################################

}

void LocalSearchPMCP::setMovableNodes(QMap<ListDigraph::Node, bool>& movableNodes) {
    node2Movable.clear();

    for (QMap < ListDigraph::Node, bool>::iterator iter = movableNodes.begin(); iter != movableNodes.end(); iter++) {
	node2Movable[iter.key()] = iter.value();
    }

}

bool LocalSearchPMCP::reachable(const ListDigraph::Node& s, const ListDigraph::Node& t) {
    QQueue<ListDigraph::Node> q;
    ListDigraph::Node curnode;

    q.enqueue(t);

    if (s == t) return true;

    if (s == INVALID || t == INVALID) return true;

    while (q.size() > 0) {
	curnode = q.dequeue();

	// Iterate over the predecessors
	for (ListDigraph::InArcIt iait(pm->graph, curnode); iait != INVALID; ++iait) {
	    ListDigraph::Node curStartNode = pm->graph.source(iait);
	    if (curStartNode == s) {
		return true;
	    } else {
		if (!q.contains(curStartNode)) {
		    q.enqueue(curStartNode);
		}
	    }
	}
    }

    return false;
}

bool LocalSearchPMCP::debugCheckPMCorrectness(const QString& location) {
    QTextStream out(stdout);

    out << "LocalSearchPMCP::debugCheckPMCorrectness : Checking correctness in < " + location + " > ... " << endl;

    totalChecksTimer.start();

    // Check cycles
    if (!dag(pm->graph)) {
	Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Graph is not DAG after scheduling!" << ENDL;
    } else {
	//Debugger::info << "LocalSearchPMCP::debugCheckPMCorrectness : Graph is DAG." << ENDL;
    }

    //out << "Checked DAG." << endl;
    //getchar();

    // Check the outgoing arcs: every node must have at most one schedule-based outgoing arc
    int noutarcs = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	noutarcs = 0;
	for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
	    if (!pm->conjunctive[oait]) {
		noutarcs++;
	    }

	    if (noutarcs > 1) {
		Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Too many outgoing schedule-based arcs!" << ENDL;
	    }
	}
    }

    //out << "Checked outgoing arcs." << endl;
    //getchar();

    // Check the incoming arcs: every node must have at most one schedule-based outgoing arc
    int ninarcs = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	ninarcs = 0;
	for (ListDigraph::InArcIt iait(pm->graph, nit); iait != INVALID; ++iait) {
	    if (!pm->conjunctive[iait]) {
		ninarcs++;
	    }

	    if (ninarcs > 1) {
		Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Too many incoming schedule-based arcs!" << ENDL;
	    }
	}
    }

    //out << "Checked the incoming schedule-based arcs." << endl;
    //getchar();

    // Check whether all nodes can be reached from the start node
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	if (nit != pm->head) {
	    if (!reachable(pm->head, nit)) {
		Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Node is not reachable from the start node!" << ENDL;
	    }
	}
    }

    //out << "Checked reachability from the start node." << endl;
    //getchar();

    // Check correctness of the processing times for the scheduled nodes
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	for (ListDigraph::OutArcIt oait(pm->graph, nit); oait != INVALID; ++oait) {
	    if (pm->ops[nit]->p() != -pm->p[oait]) {
		out << "Operation : " << pm->ops[nit]->ID << endl;
		out << *pm << endl;
		Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Outgoing arcs have incorrect length!" << ENDL;
	    }
	}
    }

    //out << "Checked the lengths of the outgoing arcs." << endl;
    //getchar();

    // Check the ready times of the operations
    QList<ListDigraph::Node> ts = pm->topolSort();

    //out << "Got the topological ordering." << endl;
    //getchar();

    double maxr;
    ListDigraph::Node pred;
    for (int i = 0; i < ts.size(); i++) {
	maxr = pm->ops[ts[i]]->r();
	for (ListDigraph::InArcIt iait(pm->graph, ts[i]); iait != INVALID; ++iait) {
	    if (pm->conjunctive[iait]) { // In general this is true only for conjunctive arcs
		pred = pm->graph.source(iait);
		maxr = Math::max(maxr, pm->ops[pred]->r() - pm->p[iait]);

		if (pm->ops[pred]->r() - pm->p[iait] > pm->ops[ts[i]]->r()) {
		    out << "Node : " << pm->ops[ts[i]]->ID << endl;
		    out << "Pred : " << pm->ops[pred]->ID << endl;
		    out << *pm << endl;
		    Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Ready time of the succeeding node < r+p of at least one predecessor!" << ENDL;
		}
	    }
	}

	if (Math::cmp(maxr, pm->ops[ts[i]]->r(), 0.00001) != 0) {
	    out << "Operation : " << pm->ops[ts[i]]->ID << endl;
	    out << "r = " << pm->ops[ts[i]]->r() << endl;
	    out << "max r(prev) = " << maxr << endl;
	    out << *pm << endl;
	    Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Ready time of the succeeding node is too large!" << ENDL;
	}

    }

    // Start time of any operation should be at least as large as the availability time of the corresponding machine
    for (int i = 0; i < ts.size(); i++) {
	ListDigraph::Node curNode = ts[i];

	if (pm->ops[curNode]->s() < pm->ops[curNode]->machAvailTime()) {
	    out << *pm->ops[curNode] << endl;
	    Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Operation start time is less than the availability time of the corresponding machine!" << ENDL;
	}
    }

    // Check the start times of the operations
    double maxc;
    for (int i = 0; i < ts.size(); i++) {
	maxr = 0.0;
	for (ListDigraph::InArcIt iait(pm->graph, ts[i]); iait != INVALID; ++iait) {
	    pred = pm->graph.source(iait);
	    maxc = Math::max(maxc, pm->ops[pred]->c());

	    if (pm->ops[pred]->c() > pm->ops[ts[i]]->s()) {
		out << "Current operation : " << pm->ops[ts[i]]->ID << endl;
		out << "Predecessor : " << pm->ops[pred]->ID << endl;
		out << *pm << endl;
		Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Operation processing overlap!" << ENDL;
	    }
	}
    }

    // Check whether schedule-based arcs always connect operations from the same machine and tool group
    ListDigraph::Node s;
    ListDigraph::Node t;
    for (ListDigraph::ArcIt ait(pm->graph); ait != INVALID; ++ait) {
	if (!pm->conjunctive[ait]) {
	    s = pm->graph.source(ait);
	    t = pm->graph.target(ait);

	    if (pm->ops[s]->toolID != pm->ops[t]->toolID || pm->ops[s]->machID != pm->ops[t]->machID) {
		Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : Schedule-based arc connects incompatible operations!" << ENDL;
	    }
	}
    }

    // Check whether all operations are not assigned
    int nassigned = 0;
    for (ListDigraph::NodeIt nit(pm->graph); nit != INVALID; ++nit) {
	if (pm->ops[nit]->machID > 0) nassigned++;
    }
    if (nassigned == 0) {
	Debugger::err << "LocalSearchPMCP::debugCheckPMCorrectness : There are no operations assigned to machines!" << ENDL;
    }


    totalChecksElapsedMS += totalChecksTimer.elapsed();

    out << "LocalSearchPMCP::debugCheckPMCorrectness : Done checking correctness in < " + location + " > . " << endl;

    return true;
}

/**  *********************************************************************** **/

/**  *********************************************************************** **/

LSScheduler::LSScheduler() {
}

LSScheduler::LSScheduler(LSScheduler& orig) : Scheduler(orig) {
}

LSScheduler::~LSScheduler() {
}

void LSScheduler::init() {
}

void LSScheduler::scheduleActions() {
    // Run the local search
    if (ls.maxIter() > 0) {
	pm.save();

	ls.setPM(&pm);
	ls.setResources(&rc);
	ls.run();

	pm.restore();
    }

    // Prepare the schedule
    sched->fromPM(pm, *obj);
}

/**  *********************************************************************** **/