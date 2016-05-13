/* 
 * File:   TrivialScheduler.cpp
 * Author: DrSobik
 * 
 * Created on July 21, 2011, 9:59 AM
 */

//#include "TrivialScheduler.h"
#include "CombinedScheduler.h"

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
        schedulers.append(SmartPointer<Scheduler>((Scheduler*) orig.schedulers[i]->clone()));
    }

    bestPerformingStat = orig.bestPerformingStat;
    lastBestSchedulerIdx = orig.lastBestSchedulerIdx;
    _bestPM = orig._bestPM;

    allowedSchedIDs = orig.allowedSchedIDs;

    // IMPORTANT!!! Objective cloning is done in the Scheduler!!!
    //this->obj = orig.obj->clone();
}

CombinedScheduler::~CombinedScheduler() {
    //Debugger::info << "CombinedScheduler::~CombinedScheduler : Started" << ENDL;

    /*
    for (int i = 0; i < schedulers.size(); i++) {
                    //delete schedulers[i];
    }

    schedulers.clear();
     */

    clear();

    //Debugger::info << "CombinedScheduler::~CombinedScheduler : Finished" << ENDL;
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

    /* Not required due to usage of SmartPointer
    for (int i = 0; i < schedulers.size(); i++) {
        delete schedulers[i];
    }
     */

    schedulers.clear();

    bestPerformingStat.clear();

    //if (obj != NULL) {
    //	delete obj;
    //  obj = NULL;
    //}
}

CombinedScheduler* CombinedScheduler::clone() {
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

    /*
    QSet<int> allowedSchedIDs;

    if (!options.contains("CS_ALLOWED_SCHEDULERS")) {
        Debugger::err << "CombinedScheduler::scheduleActions : No allowed schedulers specified!" << ENDL;
    }

    QStringList schedIDsList = options["CS_ALLOWED_SCHEDULERS"].split("&");

    for (int i = 0; i < schedIDsList.size(); i++) {
        allowedSchedIDs.insert(schedIDsList[i].toInt());
        //cout << schedIDsList[i].toStdString();
    }
     */
    //cout << endl;

    //    out << "CombinedScheduler::scheduleActions: Allowed schedulers: " << endl;
    //    for (auto id : allowedSchedIDs) {
    //        out << id << endl;
    //    }
    //    out << endl;
    //
    //    out << "CombinedScheduler::scheduleActions: Schedulers: " << endl;
    //    for (int i = 0; i < schedulers.size(); i++) {
    //        out << schedulers.at(i)->ID << endl;
    //    }
    //    out << endl;

    for (int i = 0; i < schedulers.size(); i++) {
        if (!allowedSchedIDs.contains(schedulers[i]->ID)) continue; // Skip the scheduler

        //out << "Setting pm "<< endl;
        //out << "Subscheduler ID = " << schedulers[i]->ID << endl;

        //        schedulers[i]->pm = pmInit;
        //        schedulers[i]->rc = rcInit;
        //        schedulers[i]->sched = sched;
        //        schedulers[i]->settings = settings;
        //        //out << "pm set. "<< endl;
        //
        //        schedulers[i]->schedule();

        SchedulingProblem curSP;
        curSP.pm = pmInit;
        curSP.rc = rcInit;

        *sched = schedulers[i]->solve(curSP);

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
    QTextStream out(stdout);

    // IMPORTANT!!! Clone the input scheduler and do not pass it as a pointer!!!
    schedulers.append(SmartPointer<Scheduler>((Scheduler*) (sch->clone())));
    //schedulers.append(SmartPointer<Scheduler>());
    //schedulers.last().setPointer((Scheduler*) sch->clone());
    //getchar();

    /*
    out << "CombinedScheduler::operator<< : Scheduler ID =  " << schedulers.last()->ID << endl;

    out << "CombinedScheduler:: before init: Schedulers: " << schedulers.size() << endl;
    out << "CombinedScheduler::operator<< : Scheduler ID =  " << schedulers.first()->ID << endl;
    out << "CombinedScheduler::operator<< : Scheduler ID =  " << schedulers.last()->ID << endl;
    // Iterated by value!!! Should also be fine!!!
    for (SmartPointer<Scheduler> curSP : schedulers) {
        out << curSP->ID << endl;
    }
    out << endl;
     */

    init();

    /*
    out << "CombinedScheduler:: after init: Schedulers: " << endl;
    for (SmartPointer<Scheduler>& curSP : schedulers) {
        out << curSP->ID << endl;
    }
    out << endl;
     */

    return *this;
}

Scheduler * CombinedScheduler::lastBestScheduler() {
    if (lastBestSchedulerIdx == -1) {
        Debugger::err << "CombinedScheduler::lastBestScheduler : Failer to find the last best performing scheduler!" << ENDL;
    }

    return schedulers[lastBestSchedulerIdx].getPointer();
}

ProcessModel & CombinedScheduler::bestPM() {
    return _bestPM;
}

void CombinedScheduler::setObjective(ScalarObjective& newObj) {

    // Initialize the base class
    Scheduler::setObjective(newObj);

    // Set the objectives for all subschedulers
    for (int i = 0; i < schedulers.size(); i++) {

        Scheduler& curScheduler = (Scheduler&) * schedulers[i].getPointer();

        curScheduler.setObjective(newObj);

    }

}

/*  ##################  Solver  ########################################  */

void CombinedScheduler::parse(const SchedulerOptions& options) {
    QTextStream out(stdout);

//    if (settings["ALL_SETTINGS"].get() != options["ALL_SETTINGS"].get()) {
//        out << "!=" << endl;
//        out << settings["ALL_SETTINGS"].get().compare(options["ALL_SETTINGS"].get()) << endl;
//        out << "Comp1: " << settings["ALL_SETTINGS"].get() << endl;
//        out << "Comp2: " << options["ALL_SETTINGS"].get() << endl;
//    } else {
//        out << "==" << endl;
//        if (settings["ALL_SETTINGS"].changed()) {
//            out << "CombinedScheduler::parse : pre ALL_SETTINGS changed!" << endl;
//        }
//    }

    settings = options;

    //    out << "CombinedScheduler::parse : Settings size: " << settings.container().size() << endl;
    //    for (auto curKey : settings.container().keys()) {
    //        out << "CombinedScheduler::parse : Setting: " << settings[curKey].get() << endl;
    //    }

    //settings.setChanged(true);

    bool checkSingleSettings = true;

    // First, check ALL_SETTINGS
    if (settings.container().contains("ALL_SETTINGS")) {

        if (settings["ALL_SETTINGS"].changed()) {

            //out << "CombinedScheduler::parse : ALL_SETTINGS changed!" << endl;
            //getchar();

            QRegularExpression settingsRE;
            QString allSettingsStr = settings["ALL_SETTINGS"].get();
            QRegularExpressionMatch matchSetting;

            // Objective
            settingsRE.setPattern("CS_PRIMARY_OBJECTIVE=([^;]+@[^;]+);{0,1}");
            matchSetting = settingsRE.match(allSettingsStr);
            out << "CombinedScheduler::parse : Parsed: " << matchSetting.captured(1) << endl;
            //getchar();
            if (matchSetting.captured(1) != "") {
                settings["CS_PRIMARY_OBJECTIVE"] = matchSetting.captured(1);
            }

            // Allowed schedulers
            settingsRE.setPattern("CS_ALLOWED_SCHEDULERS=(\\d+(?:&\\d+|\\d+)*)");
            matchSetting = settingsRE.match(allSettingsStr);
            out << "CombinedScheduler::parse : Parsed: " << matchSetting.captured(1) << endl;
            //getchar();
            if (matchSetting.captured(1) != "") {
                settings["CS_ALLOWED_SCHEDULERS"] = matchSetting.captured(1);
            }

            // Schedulers
            settingsRE.setPattern("CS_SCHEDULERS="
                    "(\\["
                    "("
                    "(?:[^\\[\\]]*(?1)*[^\\[\\]]*)*" // Really cool!!! :)
                    ")"
                    "\\])");
            matchSetting = settingsRE.match(allSettingsStr);

            if (!settingsRE.isValid()) {
                QString errorString = settingsRE.errorString();
                int errorOffset = settingsRE.patternErrorOffset();

                out << "Pattern error: " << errorString << " at offset " << errorOffset << endl;
            }
            //QRegularExpressionMatch match = re.match("[12345678[9[10]11]a[12]]13[14]");

            out << "CombinedScheduler::parse : Parsed: " << matchSetting.captured(2) << endl;
            //getchar();
            if (matchSetting.captured(1) != "") {
                settings["CS_SCHEDULERS"] = matchSetting.captured(2);
            }

            checkSingleSettings = true;

        } else { // ALL_SETTINGS exists but did not change => no need to perform further checks!!!

            checkSingleSettings = false;

        }

    } else {
        checkSingleSettings = true;
    } // ALL_SETTINGS

    if (checkSingleSettings) {

        // Parse the objective
        if (settings.container().contains("CS_PRIMARY_OBJECTIVE")) {

            if (settings["CS_PRIMARY_OBJECTIVE"].changed()) {

                QString objStr = settings["CS_PRIMARY_OBJECTIVE"].get();

                QRegularExpression curObjRE("([^\\(\\)]+)@([^\\(\\)]+)");
                QRegularExpressionMatch match;
                match = curObjRE.match(objStr);

                QString objLibName = match.captured(2); // Library where the objective is located
                QString objName = match.captured(1); // Objective name

                QLibrary objLib(objLibName);

                out << "CS_PRIMARY_OBJECTIVE : objLibName : " << objLibName << endl;
                out << "CS_PRIMARY_OBJECTIVE : objName : " << objName << endl;

                // The search algorithm
                Common::Util::DLLCallLoader<ScalarObjective*, QLibrary&, const char*> objLoader;
                SmartPointer<ScalarObjective> curObj;
                try {

                    //curObj = objLoader.load(objLib, QString("new_" + initSchedulerName).toStdString().data());
                    curObj.setPointer(objLoader.load(objLib, QString("new_" + objName).toStdString().data()));

                } catch (...) {

                    out << objLib.fileName() << endl;
                    throw ErrMsgException<>(std::string("CombinedScheduler::parse : Failed to resolve objective!"));

                }

                // Set the objective
                this->setObjective(*curObj.getPointer());

            }

        } else {
            throw ErrMsgException<>(std::string("CombinedScheduler::parse : CS_PRIMARY_OBJECTIVE not specified!"));
        }

        // Parse allowed shcedulers' numbers
        if (settings.container().contains("CS_ALLOWED_SCHEDULERS")) {

            if (settings["CS_ALLOWED_SCHEDULERS"].changed()) {

                out << "CombinedScheduler::parse : Parsing CS_ALLOWED_SCHEDULERS = " << settings["CS_ALLOWED_SCHEDULERS"].get() << endl;

                allowedSchedIDs.clear();

                QStringList schedIDsList = settings["CS_ALLOWED_SCHEDULERS"].get().split("&");

                for (int i = 0; i < schedIDsList.size(); i++) {
                    allowedSchedIDs.insert(schedIDsList[i].toInt());
                    //    out << schedIDsList[i] << "&";
                }
                //out << endl;

            }

            //getchar();
        } else {
            throw ErrMsgException<>(std::string("CombinedScheduler::parse : CS_ALLOWED_SCHEDULERS not specified!"));
        }

        // Parse the schedulers
        if (settings.container().contains("CS_SCHEDULERS")) {

            if (settings["CS_SCHEDULERS"].changed()) {

                // Remove the already existing schedulers
                schedulers.clear();

                // Parse the setting
                QString csSchedsStr = settings["CS_SCHEDULERS"].get();
                QRegularExpression curSchedsRE("((?<schedulerName>[^\\(\\)]+)"
                        "(\\((?<parameterList>"
                        "(?:[^\\(\\)]*(?3)*[^\\(\\)]*)*"
                        ")\\))"
                        "@(?<libName>[^\\(\\)\\[\\]@;]+));?");
                if (!curSchedsRE.isValid()) {
                    QString errorString = curSchedsRE.errorString();
                    int errorOffset = curSchedsRE.patternErrorOffset();

                    out << "Pattern error: " << errorString << " at offset " << errorOffset << endl;
                }
                QRegularExpressionMatchIterator curIter = curSchedsRE.globalMatch(csSchedsStr);
                QRegularExpressionMatch match;

                QStringList curSchedStrs;
                QString curSchedName;
                QString curSchedLibName;
                QString curSchedAllParams;

                while (curIter.hasNext()) {
                    match = curIter.next();

                    curSchedStrs << match.captured(1);

                    curSchedName = match.captured("schedulerName");
                    curSchedLibName = match.captured("libName");
                    curSchedAllParams = match.captured("parameterList");

                    out << "CombinedScheduler::parseOptions: CS_SCHEDULERS: " << match.captured(1) << endl;
                    out << "Parsed scheduler name: " << curSchedName << endl;
                    out << "Parsed scheduler parameters: " << curSchedAllParams << endl;
                    out << "Parsed scheduler lib: " << curSchedLibName << endl;

                    // Try to load the schedulers from the library
                    QLibrary curSchedulerLib(curSchedLibName);
                    // The search algorithm
                    Common::Util::DLLCallLoader<SchedSolver*, QLibrary&, const char*> initSchedulerLoader;
                    SmartPointer<SchedSolver> curScheduler;

                    SchedulerOptions curSchedSettings;
                    curSchedSettings["ALL_SETTINGS"] = curSchedAllParams;

                    out << "CombinedScheduler::parse : Trying to load the scheduler..." << endl;

                    try {

                        //curScheduler = initSchedulerLoader.load(curSchedulerLib, QString("new_" + curSchedName).toStdString().data());
                        curScheduler.setPointer(initSchedulerLoader.load(curSchedulerLib, QString("new_" + curSchedName).toStdString().data()));

                    } catch (Common::Util::DLLLoadException<Common::Util::DLLResolveLoader<SchedSolver*, QLibrary&, const char*>>&) {

                        out << "Load exception! " << curSchedulerLib.fileName() << endl;
                        getchar();

                    } catch (...) {

                        out << curSchedulerLib.fileName() << endl;
                        throw ErrMsgException<>(std::string("CombinedScheduler::parseSettings : Failed to resolve scheduler algorithm ") + curSchedName.toStdString() + std::string("!"));

                    }

                    out << "CombinedScheduler::parse : Loaded the scheduler." << endl;

                    // Parse the settings
                    curScheduler->parse(curSchedSettings);

                    // Finally, add the scheduler
                    *this << (Scheduler*) curScheduler.getPointer();

                } // Iterating over schedulers

                out << "CombinedScheduler::parse : Parsed all schedulers" << endl;
                //getchar();

            }

        } else {
            throw ErrMsgException<>(std::string("CombinedScheduler::parse : CS_SCHEDULERS not specified!"));
        }

        // Initialize after parsing new settings
        init();

    } // checkSingleSettings

    out << "CombinedScheduler::parse : Parsed all settings" << endl;

    // Set all settings as unchanged
    settings.setChanged(false);

}

Schedule CombinedScheduler::solve(const SchedulingProblem& problem/*, const SchedulerOptions& options*/) {
    QTextStream out(stdout);

    Schedule res;

    /* The settings are assumed to be parsed already!!!
    try {

        parse(options);

    } catch (ErrMsgException<>& ex) {

        out << QString::fromStdString(ex.getMsg().getMsgData()) << endl;
        throw ex;

    }
     */

    // Set the PM
    this->pm = (ProcessModel&) problem.pm;

    // Set the resources
    this->rc = (Resources&) problem.rc;

    // Set the Schedule
    this->sched = &res;

    // Run the scheduler
    schedule();

    return res;

}

/*  ####################################################################  */


/** ************************************************************************ **/

///** *********************** Combined scheduler with LS ********************* **/
//
//CombinedSchedulerLS::CombinedSchedulerLS() {
//
//    cs.setParent(this);
//    ls.setParent(this);
//
//}
//
//CombinedSchedulerLS::CombinedSchedulerLS(CombinedSchedulerLS & orig) : Scheduler(orig), cs(orig.cs), ls(orig.ls) {
//
//    //Debugger::info << "CombinedSchedulerLS::CombinedSchedulerLS(CombinedSchedulerLS & orig)..." << ENDL;
//
//    cs.setParent(this);
//    ls.setParent(this);
//
//    obj = orig.obj->clone();
//
//}
//
//CombinedSchedulerLS::~CombinedSchedulerLS() {
//
//    Debugger::info << "CombinedSchedulerLS::~CombinedSchedulerLS" << ENDL;
//
//}
//
//void CombinedSchedulerLS::init() {
//    cs.init();
//    ls.init();
//}
//
//CombinedSchedulerLS * CombinedSchedulerLS::clone() {
//    //Debugger::info << "CombinedSchedulerLS::clone : Cloning..." << ENDL;
//    return new CombinedSchedulerLS(*this);
//}
//
//void CombinedSchedulerLS::scheduleActions() {
//    // Parse options
//    if (options.container().contains("LS_CHK_COR")) {
//        if (options["LS_CHK_COR"].get() == "true") {
//            ls.checkCorrectness(true);
//            //cout << "Checking correctness" << endl;
//        } else {
//            ls.checkCorrectness(false);
//            //cout << "NOT checking correctness" << endl;
//        }
//    } else {
//        Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_CHK_COR not specified!" << ENDL;
//    }
//
//    if (options.container().contains("LS_MAX_ITER")) {
//        ls.maxIter(options["LS_MAX_ITER"].get().toInt());
//    } else {
//        Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
//    }
//
//    if (options.container().contains("LS_MAX_TIME_MS")) {
//        ls.maxTimeMs(options["LS_MAX_TIME_MS"].get().toInt());
//    }
//
//    if (options.container().contains("LS_CRIT_NODES_UPDATE_FREQ")) {
//        ls.setCritNodesUpdateFreq(options["LS_CRIT_NODES_UPDATE_FREQ"].get().toInt());
//    } else {
//        Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_CRIT_NODES_UPDATE_FREQ not specified!" << ENDL;
//    }
//
//    if (options.container().contains("LS_BEST_POS_TO_MOVE")) {
//        if (options["LS_BEST_POS_TO_MOVE"].get() == "true") {
//            ls.setBestPosToMove(true);
//            //cout << "Checking correctness" << endl;
//        } else {
//            ls.setBestPosToMove(false);
//            //cout << "NOT checking correctness" << endl;
//        }
//    } else {
//        Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_BEST_POS_TO_MOVE not specified!" << ENDL;
//    }
//
//    // Run the Combined scheduler
//    cs.pm = pm;
//    cs.rc = rc;
//    cs.sched = sched;
//    cs.options = options;
//
//    cs.schedule();
//
//    // Get the best PM found by the CombinedScheduler
//    pm = cs.bestPM();
//
//    int iterCtr = 0;
//    for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
//        iterCtr += cs.bestPerformingStat[i];
//    }
//
//    //RandGenMT randGen(Math::rndSeed());
//
//    //QTextStream out(stdout);
//    //out << "Current seed : " << Math::rndSeed() << endl;
//    //getchar();
//
//    //if (iterCtr > 400) {
//    // Improve the PM by the local search
//    //ls.maxIter(0);
//    //ls.checkCorectness(false);
//    if (ls.maxIter() > 0) {
//        pm.save();
//
//        ls.setObjective(obj);
//        ls.setPM(&pm);
//        ls.setResources(&rc);
//        //ls.setRandGen(&randGen);
//
//        ls.run();
//
//        pm.restore();
//    }
//    //}
//
//    // Prepare the schedule
//    sched->fromPM(pm, *obj);
//
//    //getchar();
//
//    /*
//    // TESTING : Try to leave the selections of only one machine group and check the TWT
//
//    QTextStream out(stdout);
//    pm.updateHeads();
//    pm.updateStartTimes();
//    ls.debugCheckPMCorrectness("");
//    out << "TWT of the full schedule is " << TWT()(pm) << endl;
//
//    int curTG = 10;
//
//    QList<ListDigraph::Arc> arcsToRem;
//
//    for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
//
//            ListDigraph::Node curNode = nit;
//            Operation& curOp = (Operation&) *(pm.ops[curNode]);
//
//            if (curOp.toolID != curTG) { // Mark the disjunctive arcs for removal and restore the expected processing time
//
//                    curOp.p(rc(curOp.toolID).expectedProcTime(&curOp));
//
//                    for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
//
//                            ListDigraph::Arc curArc = oait;
//
//                            pm.p[curArc] = -curOp.p();
//
//                            if (!pm.conjunctive[curArc]) arcsToRem.append(curArc);
//
//                    }
//
//            }
//
//    }
//
//    // Remove the arcs
//    for (int i = 0; i < arcsToRem.size(); i++) {
//            pm.graph.erase(arcsToRem[i]);
//    }
//
//    pm.updateHeads();
//    pm.updateStartTimes();
//
//    out << "TWT of the partial schedule with machine group " << curTG << " is " << TWT()(pm) << endl;
//    getchar();
//	
//    // TESTING
//     */
//
//}
//
///*  ##################  Solver  ########################################  */
//
//void CombinedSchedulerLS::parse(const SchedulerOptions&) {
//
//    throw ErrMsgException<>("CombinedSchedulerLS::parse not implemented!");
//
//}
//
//Schedule CombinedSchedulerLS::solve(const SchedulingProblem& problem/*, const SchedulerOptions& options*/) {
//    
//    throw ErrMsgException<>("CombinedSchedulerLS::solve is deprecated!");
//    
//    QTextStream out(stdout);
//
//    Schedule res;
//
//    out << "Reached: CombinedSchedulerLS::solve!" << endl;
//
//    /*
//    try {
//        parse(options);
//    } catch (ErrMsgException<>& ex) {
//        out << QString::fromStdString(ex.getMsg().getMsgData()) << endl;
//        throw ex;
//    }
//     */
//
//    // Set the PM
//    this->pm = (ProcessModel&) problem.pm;
//
//    // Set the resources
//    this->rc = (Resources&) problem.rc;
//
//    // Set the Schedule
//    this->sched = &res;
//
//    // Run the scheduler
//    schedule();
//
//    return res;
//
//}
//
///*  ####################################################################  */
//
///** ************************************************************************ **/
//
///** *********************** Combined scheduler with modified LS ************ **/
//
//CombinedSchedulerModLS::CombinedSchedulerModLS() {
//}
//
//CombinedSchedulerModLS::CombinedSchedulerModLS(CombinedSchedulerModLS & orig) : Scheduler(orig), cs(orig.cs), ls(orig.ls) {
//}
//
//CombinedSchedulerModLS::~CombinedSchedulerModLS() {
//}
//
//void CombinedSchedulerModLS::init() {
//    cs.init();
//    ls.init();
//}
//
//CombinedSchedulerModLS * CombinedSchedulerModLS::clone() {
//    return new CombinedSchedulerModLS(*this);
//}
//
//void CombinedSchedulerModLS::scheduleActions() {
//    // Parse options
//    if (options.container().contains("LS_CHK_COR")) {
//        if (options["LS_CHK_COR"].get() == "true") {
//            ls.checkCorrectness(true);
//            //cout << "Checking correctness" << endl;
//        } else {
//            ls.checkCorrectness(false);
//            //cout << "NOT checking correctness" << endl;
//        }
//    } else {
//        Debugger::err << "CombinedSchedulerModLS::scheduleActions : LS_CHK_COR not specified!" << ENDL;
//    }
//
//    if (options.container().contains("LS_MAX_ITER")) {
//        ls.maxIter(options["LS_MAX_ITER"].get().toInt());
//    } else {
//        Debugger::err << "CombinedSchedulerModLS::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
//    }
//
//    if (options.container().contains("LS_MAX_TIME_MS")) {
//        ls.maxTimeMs(options["LS_MAX_TIME_MS"].get().toInt());
//    }
//
//    // Run the Combined scheduler
//    cs.pm = pm;
//    cs.rc = rc;
//    cs.sched = sched;
//    cs.options = options;
//
//    cs.schedule();
//
//    // Get the best PM found by the CombinedScheduler
//    pm = cs.bestPM();
//
//    int iterCtr = 0;
//    for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
//        iterCtr += cs.bestPerformingStat[i];
//    }
//
//    //if (iterCtr > 400) {
//    // Improve the PM by the local search
//    //ls.maxIter(0);
//    //ls.checkCorectness(false);
//    if (ls.maxIter() > 0) {
//        pm.save();
//
//        ls.setPM(&pm);
//        ls.setResources(&rc);
//        ls.run();
//
//        pm.restore();
//    }
//    //}
//
//    // Prepare the schedule
//    sched->fromPM(pm, *obj);
//
//
//    /*
//    // TESTING : Try to leave the selections of only one machine group and check the TWT
//
//    QTextStream out(stdout);
//    pm.updateHeads();
//    pm.updateStartTimes();
//    ls.debugCheckPMCorrectness("");
//    out << "TWT of the full schedule is " << TWT()(pm) << endl;
//
//    int curTG = 10;
//
//    QList<ListDigraph::Arc> arcsToRem;
//
//    for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
//
//            ListDigraph::Node curNode = nit;
//            Operation& curOp = (Operation&) *(pm.ops[curNode]);
//
//            if (curOp.toolID != curTG) { // Mark the disjunctive arcs for removal and restore the expected processing time
//
//                    curOp.p(rc(curOp.toolID).expectedProcTime(&curOp));
//
//                    for (ListDigraph::OutArcIt oait(pm.graph, curNode); oait != INVALID; ++oait) {
//
//                            ListDigraph::Arc curArc = oait;
//
//                            pm.p[curArc] = -curOp.p();
//
//                            if (!pm.conjunctive[curArc]) arcsToRem.append(curArc);
//
//                    }
//
//            }
//
//    }
//
//    // Remove the arcs
//    for (int i = 0; i < arcsToRem.size(); i++) {
//            pm.graph.erase(arcsToRem[i]);
//    }
//
//    pm.updateHeads();
//    pm.updateStartTimes();
//
//    out << "TWT of the partial schedule with machine group " << curTG << " is " << TWT()(pm) << endl;
//    getchar();
//	
//    // TESTING
//     */
//
//}
//
///** ************************************************************************ **/
//
///** *********************** Combined scheduler with LSCP ********************* **/
//
//CombinedSchedulerLSCP::CombinedSchedulerLSCP() {
//}
//
//CombinedSchedulerLSCP::CombinedSchedulerLSCP(CombinedSchedulerLSCP & orig) : Scheduler(orig), cs(orig.cs), ls(orig.ls) {
//}
//
//CombinedSchedulerLSCP::~CombinedSchedulerLSCP() {
//}
//
//void CombinedSchedulerLSCP::init() {
//    cs.init();
//    ls.init();
//}
//
//CombinedSchedulerLSCP * CombinedSchedulerLSCP::clone() {
//    return new CombinedSchedulerLSCP(*this);
//}
//
//void CombinedSchedulerLSCP::scheduleActions() {
//    // Parse options
//    if (options.container().contains("LS_CHK_COR")) {
//        if (options["LS_CHK_COR"].get() == "true") {
//            ls.checkCorrectness(true);
//            //cout << "Checking correctness" << endl;
//        } else {
//            ls.checkCorrectness(false);
//            //cout << "NOT checking correctness" << endl;
//        }
//    } else {
//        Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_CHK_COR not specified!" << ENDL;
//    }
//
//    if (options.container().contains("LS_MAX_ITER")) {
//        ls.maxIter(options["LS_MAX_ITER"].get().toInt());
//    } else {
//        Debugger::err << "CombinedSchedulerLS::scheduleActions : LS_MAX_ITER not specified!" << ENDL;
//    }
//
//    // Run the Combined scheduler
//    cs.pm = pm;
//    cs.rc = rc;
//    cs.sched = sched;
//    cs.options = options;
//
//    cs.schedule();
//
//    // Get the best PM found by the CombinedScheduler
//    pm = cs.bestPM();
//
//    int iterCtr = 0;
//    for (int i = 0; i < cs.bestPerformingStat.size(); i++) {
//        iterCtr += cs.bestPerformingStat[i];
//    }
//
//    //if (iterCtr > 400) {
//    // Improve the PM by the local search
//    //ls.maxIter(0);
//    //ls.checkCorectness(false);
//    if (ls.maxIter() > 0) {
//        pm.save();
//
//        ls.setPM(&pm);
//        ls.setResources(&rc);
//        ls.run();
//
//        pm.restore();
//    }
//    //}
//
//    // Prepare the schedule
//    sched->fromPM(pm, *obj);
//
//}
