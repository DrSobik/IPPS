/* 
 * File:   VNSPlanner.h
 * Author: DrSobik
 *
 * Created on August 17, 2011, 9:52 AM
 * 
 * Description : Class VNSPlanner encapsulates the VNS search algorithm for
 *				 selection of the best possible process plans.
 * 
 * Contained data:
 *				
 *				bestObj	-	currently best found objective value.
 * 
 *				bestBOPIDs
 *						-	currently the best found set of BOP IDs of the
 *							products.
 * 
 *				prodBOPIDs
 *						-	sets of available BOPs for each product.
 *				
 *				kmax	-	VNS-specific variable which defines the maximum
 *							degree of the global neighborhoods.
 * 
 *				k		-	VNS-specific variable which shows the current degree
 *							of the global neighborhood for the shaking phase.
 */

#ifndef VNSPLANNER_H
#define VNSPLANNER_H

#include <QtCore>
#include <QtConcurrent>

#include "MathExt"
#include "DebugExt.h"

#include "Solver"
#include "Loader"
#include "SmartPointer"
#include "Runnable"

#include "IPPSDefinitions"

#include "Planner"
#include "ProcessModelManager"

#include "Scheduler"
#include "SchedulingProblem"
#include "Plan"
#include "IPPSProblem"

using namespace Common;
using namespace Common::Interfaces;
using namespace Common::Util;
using namespace Common::SmartPointers;

/** Qt wrapper for SchedSolver */
//class QtSchedSolver : public Object<QObject>, public RunnableIn<QThread> {
//    Q_OBJECT
//
//private:
//
//    SmartPointer<SchedSolver> solver;
//
//    SchedulingProblem schedProblem;
//    SchedulerOptions schedOptions;
//
//    Schedule* resSched;
//
//protected:
//
//
//public:
//
//    QtSchedSolver() {
//
//	QObject::connect(&env, SIGNAL(started()), this, SLOT(runActions()));
//	this->moveToThread(&env);
//
//    }
//
//    QtSchedSolver(const QtSchedSolver&) : BasicObject(), Object(), RunnableIn() {
//
//	throw ErrMsgException<>(std::string("QtSchedSolver(const QtSchedSolver&) not implemented!!!"));
//
//    }
//
//    virtual ~QtSchedSolver() {
//	env.quit();
//	env.wait();
//    }
//
//
//signals:
//
//    void sigFinished();
//
//public slots:
//
//    // Used for thread synchronization
//
//    void join() {
//	env.wait();
//    }
//
//    SmartPointer<SchedSolver>& solverSPtr() {
//	return solver;
//    }
//
//    void solverPtr(const SchedSolver* solver) {
//	this->solver.setPointer(solver, true);
//    }
//
//    void runActions() {
//
//	QTextStream out(stdout);
//
//	out << objectName() << "(" << QThread::currentThread() << ")" << ": rndSeed = " << Rand::rndSeed() << endl;
//	//getchar();
//
//	*resSched = solver->solve(schedProblem, schedOptions);
//
//	emit sigFinished();
//
//	env.quit();
//    }
//
//    void run() override {
//
//	QTextStream out(stdout);
//
//	out << "Running QtSchedSolver... " << endl;
//
//	env.start();
//
//    }
//
//    void solve(Schedule& resSched, const SchedulingProblem& schedProblem, const SchedulerOptions& schedOptions) {
//
//	this->schedProblem = schedProblem;
//	this->schedOptions = schedOptions;
//
//	this->resSched = &resSched;
//
//	// Run the solver
//	run();
//
//    }
//
//
//};

/** VNSPlannerStrategy defines the behavior of the VNSPlanner */
class VNSPlannerStrategy : public PlannerStrategy {
    Q_OBJECT
private:

    QVector<QPair<double, double> > plannerProgressIntervals; // Percentages of the algorithm's progress
    QVector<QPair<QString, QString> > plannerProgressIntervalsBrackets; // To define whether an interval is open or close
    QVector<PlannerOptions> plannerOptions; // Options of the scheduling algorithm (corresponds to the percentages)

    QVector<QPair<double, double> > schedulerProgressIntervals; // Percentages of the algorithm's progress
    QVector<QPair<QString, QString> > schedulerProgressIntervalsBrackets; // To define whether an interval is open or close
    QVector<QString> schedulerNames; // Name of the scheduler which should be used for scheduling
    QVector<QString> schedulerLibNames; // Libs where the the corresponding schedulers are located 
    QVector<SchedulerOptions> schedulerOptions; // Options of the scheduling algorithm (corresponds to the percentages)


protected:

    virtual bool parseStrategy(const QString& strStrategy);

public:

    VNSPlannerStrategy();
    VNSPlannerStrategy(const VNSPlannerStrategy&);
    virtual ~VNSPlannerStrategy();

    virtual VNSPlannerStrategy* clone();

    const QVector<QPair<double, double> >& getPlannerProgressIntervals() {
	return plannerProgressIntervals;
    }

    const QVector<QPair<QString, QString> >& getPlannerProgressIntervalsBrackets() {
	return plannerProgressIntervalsBrackets;
    }

    const QVector<PlannerOptions>& getPlannerOptions() {
	return plannerOptions;
    }

    const QVector<QPair<double, double> >& getSchedulerProgressIntervals() {
	return schedulerProgressIntervals;
    }

    const QVector<QPair<QString, QString> >& getSchedulerProgressIntervalsBrackets() {
	return schedulerProgressIntervalsBrackets;
    }

    const QVector<QString >& getSchedulerNames() {
	return schedulerNames;
    }

    const QVector<QString >& getSchedulerLibNames() {
	return schedulerLibNames;
    }

    const QVector<SchedulerOptions>& getSchedulerOptions() {
	return schedulerOptions;
    }

};

class VNSPlanner : public Planner, public PlanSchedSolver {
    Q_OBJECT
private:

    bool acceptedWorse; // Defines whether worse solutions must be accepted
    double worseAcceptRate; // Defines which worse solutions can be accepted
    int maxIterDeclToAccWorse; // Max iterations to be declined in order to start accepting worse solutions

    int kmax;
    int k;
    int kStep; // Step to increase k

    QHash<int, int> prodID2itmK; // Number of items which should change their routes (for every product)
    QHash<int, int> prodID2itmKmax; // The largest number of items which should change their routes (for every product)
    int itmKStep; // Step to increase itmK

    int kContribMax; // Maximal number of products to be changed for N3
    int kContrib; // Current number of products to be changed
    int kContribStep; // Step to increase kContrib
    double kContribPow; // Probability scaling power when applying N3

    QHash<int, double> prodID2expCost; // Expected processing costs for one item of the product type
    QHash<int, double> prodID2minExpCostDev; // Minimal expected cost deviation for one item of the product. Range [0.0; 1.0]
    QHash<int, double> prodID2maxExpCostDev; // Maximal expected cost deviation for one item of the product. Range [0.0; +infinity]

    Plan bestPlan; // Best found plan so far
    Schedule bestSched; // The schedule for the best plan

    Plan curPlan; // Current plan
    Schedule curSched; // The schedule for the current plan

    Plan prevPlan; // Previous plan 
    Schedule prevSched; // The schedule for the previous plan

    int maxNewPlans; // The number of newly generated plans

    QList<Plan> newPlans; // The newly generated plans which have to be considered
    QList<Schedule> newScheds; // The schedules of the newly generated plans

    //    QList<SchedThread*> schedThreads;
    //    QList<Scheduler*> schedAlgs;
    QList<SmartPointer<SchedSolver>> schedSolvers;
    //QThread* initSchedThread;

    //QSet<int> assessedNewPlansIdx; // Indices of the already assessed plans

    VNSPlannerStrategy strategy; // Current strategy for the planner
    bool strategySettings_BEST_PLAN; // Defines whether the best plan so far needs to be amont the currently assessed ones

    PlannerOptions settings; // Settings used by everyone

    QFile proto_file; // File for saving protocol


    Setting curSchedulerName; // Solver which is currently used for scheduling according to the strategy
    Setting curSchedulerLib; // The solver's librady
    SchedulerOptions curSchedulerSettings; // The solver's settings

    SmartPointer<SchedSolver> initScheduler; // Used for initial scheduling
    
    /** Generate the initial solution. */
    virtual Plan initialSolution();

    void applyStrategy();

public:

    //    Scheduler *scheduler;

    //    SchedulerOptions schedOptions; // Options which must be considered by the scheduling algorithms

    enum SolInitType {
	SOL_INIT_RND, SOL_INIT_RANK
    };

    SolInitType solInitType;

    enum NS {
	NS_N1, NS_N2, NS_N3, NS_N2N1, NS_N2N3, NS_N1N3, NS_N3N1, NS_N2N1N3, NS_N2N3N1, NS_N2N1PN3, NS_PN2PN1PN3, NS_N4, NS_N5
    };

    NS ns; // Defines the main neighborhood structure to be used during the shaking phase

    VNSPlanner();
    VNSPlanner(const VNSPlanner&);
    virtual ~VNSPlanner();

    virtual VNSPlanner* clone() override {
	return new VNSPlanner(*this);
    }

    void init();

    void clear();

    void stepActions();

    void assessActions();

    bool acceptCondition();

    void acceptActions();

    void declineActions();

    bool stopCondition();

    void stopActions();

    void preprocessingActions();

    void postprocessingActions();

    /** Perform shaking phase for the VNS. */
    void shake();

    /** Neighborhood for searching by changing the BOMs. Does not change the
     *  item type routes. */
    Plan N1(const Plan& curplan);

    inline void setStepN1(const int& stepN1 = 1) {
	kStep = stepN1;
    }

    /** Neighborhood for searching by changing only the routes. */
    Plan N2(const Plan& curplan);

    inline void setStepN2(const int& stepN2 = 1) {
	itmKStep = stepN2;
    }

    /** Neighborhood for changing the products contributing to the objective the most.
     *  Does not change the item type routes routes!!! */
    Plan N3(const Plan& curplan);

    inline void setStepN3(const int& stepN3 = 1) {
	kContribStep = stepN3;
    }

    /** Change some BOMs and the routes within the affected BOMs. */
    Plan N4(const Plan& curplan);

    /** Change some BOMs and the routes within the non-affected BOMs. */
    Plan N5(const Plan& curplan);

    inline void setNumNeighbors(const int& num = 1) {
	maxNewPlans = num;
    }

    /** Perform local search for the VNS. */
    void localSearch();

    /** Best plan found so far. */
    const Plan& getBestPlan() {
	return bestPlan;
    }

    /** Best schedule that corresponds to the best found plan. */
    const Schedule& getBestSchedule() {
	return bestSched;
    }

    VNSPlanner& operator<<(Scheduler *scheduler) {
	return (VNSPlanner&) (Planner::operator<<(scheduler));
    }

    VNSPlanner& operator<<(Schedule *) {
	return (VNSPlanner&) * this;
    }

    /** Set the resources. */
    VNSPlanner& operator<<(Resources *rc) {
	return (VNSPlanner&) (Planner::operator<<(rc));
    }

    /** Set the process model manger. */
    VNSPlanner& operator<<(ProcessModelManager *pmm) {
	return (VNSPlanner&) (Planner::operator<<(pmm));
    }

    /** Set the protocol. */
    VNSPlanner& operator<<(Protocol *protocol) {
	return (VNSPlanner&) (Planner::operator<<(protocol));
    }

    /** Set the objective. */
    VNSPlanner& operator<<(ScalarObjective* obj) {
	return (VNSPlanner&) (Planner::operator<<(obj));
    }

    /** Set a planner strategy. */
    VNSPlanner& operator<<(VNSPlannerStrategy& strategy);

    /**************************************************************************/

    SchedulingProblem formulateSchedulingProblem(const Plan& curPlan);

    /**************************************************************************/

    virtual void parse(const PlannerOptions&) override;

    virtual PlanSched solve(const IPPSProblem&/*, const PlannerOptions&*/) override;

    /**************************************************************************/

signals:
    void sigInitSched();

    //    void sigAssessPlan(const int&);
    //    void sigPlanAssessFinished(const int&);
    //    void sigAllPlansAssessed();

    void sigCompletelyFinished();

public slots:

    void slotInitSched();
    void slotInitSchedFinished();

    void slotAssessPlan(const int&);
    void slotPlanAssessFinished(const int&);
    void slotAllPlansAssessed();

    void slotIterationFinished();

    void slotUpdateProtocol();

    virtual void slotFinished();
};

#endif /* VNSPLANNER_H */

