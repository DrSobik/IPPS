/* 
 * File:   LocalSearchPM.h
 * Author: DrSobik
 *
 * Description:	Class LocalSearchPM represents a local search procedure based on
 *		neighborhoods presented by Paulli and Dausere-Peres. This procedure
 *		is capable of moving some operation from one machine to some other 
 *		machine of the same machine group.
 * 
 * Created on September 18, 2012, 12:28 PM
 */

#ifndef LOCALSEARCHPM_H
#define LOCALSEARCHPM_H

#include <QtCore>

#include <lemon/list_graph.h>
#include <lemon/path.h>
#include <lemon/connectivity.h>
#include <lemon/bellman_ford.h>

#include "Solver"
#include "DebugExt.h"
#include "RandExt"
#include "MathExt"
#include "Exceptions"
#include "SmartPointer"
#include "Loader"

#include "IPPSDefinitions"

#include "Resources"
#include "ProcessModel"

#include "IterativeAlg"
#include "Objective"

#include "SchedulingProblem"
#include "Schedule"
#include "Scheduler"

using namespace std;
using namespace lemon;
using namespace Common;
using namespace Common::Rand;
using namespace Common::Interfaces;
using namespace Common::Exceptions;
using namespace Common::Messages;
using namespace Common::SmartPointers;

//typedef Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&> SchedSolver;

class LocalSearchPM : public IterativeAlg, public SchedSolver {
    Q_OBJECT
private:
    //GeneralRandGen* randGen; // Random number generator for the algorithm
    SmartPointer<Common::Interfaces::RandGen<Math::uint32>> intRNG; // RNG for integers (uint32)
    SmartPointer<Common::Interfaces::RandGen<double>> floatRNG; // RNG for floats (double)

    SmartPointer<SchedSolver> initScheduler; // Used to generate initial schedule

    ProcessModel *pm; // Current process model
    Resources *rc;

    ScalarObjective* obj; // Objective

    //QList<ListDigraph::Node> terminals; // Terminal nodes in the graph

    double bestobj; // Objective value before the step of the algorithm
    double prevobj; // Previous objective value

    double curobj; // Objective value after the step of the algorithm

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsRem; // Arcs which have been removed from the graph at the last step of the algorithm
    QList<double> weightsRem; // Weights of the removed arcs
    QList<ListDigraph::Arc> arcsIns; // Arcs which have been inserted at the last step of the algorithm
    int remMachID; // The previous machine assignment of the operation. Useful if the last operation is removed from the machine
    ListDigraph::Node optomove; // Operation to be moved

    QList<ListDigraph::Node> criticalNodes; // Set of critical nodes from which the operation to move is selected

    // Notice: Reversing an arc is equal to removing it and inserting the reversed one
    ListDigraph::Node nodeI; // The node which has been moved during the last step
    ListDigraph::Node nodeT; // The node which was folloving nodeI during the last step

    //QList<ListDigraph::Node> topolSorted; // Nodes which are sorted topologically
    QList<ListDigraph::Node> topolOrdering; // Topological ordering of all nodes of the graph
    int topolITStart; // Index to start updating the nodes with
    QList<ListDigraph::Node> prevTopolOrdering; // Previous topological ordering for restoring purposes
    int prevTopolITStart; // Previous index to start updating the nodes with

    QHash<int, QPair<double, double> > prevRS; // The preserved values of the ready times and the start times

    bool acceptedworse; // Indicates whether a worse solution has been accepted

    int nisteps; // Number of non-improvement steps

    double alpha;

    //TWT obj; // Primary optimization criterion

    bool _check_correctness; // Defines whether correctness should be checked

    int critNodesUpdateFreq; // Frequency with which the critical nodes are updated

    //QMap<ListDigraph::Node, bool> node2Movable; // Indicates whether the node can be moved during the search procedures
    QHash<int, bool> node2Movable; // Indicates whether a node with the given ID can be moved during the search procedures

    bool bestPosToMove; // Select the best position to move during the transitions

    Settings settings; // Settings for this solver

    // ############ DEBUG ################
    QList<int> scheduledtgs;

    QTime topSortTimer; // Timer for topological sorting
    int topSortElapsedMS; // Number of ms elapsed for topological sorting
    QTime objTimer; // Timer for objective function
    int objElapsedMS; // Number of ms elapsed for calculating the objective
    QTime updateEvalTimer; // Timer for objective function
    int updateEvalElapsedMS; // Number of ms elapsed for calculating the objective
    QTime opSelectionTimer; // Timer for selecting operations to be moved
    int opSelectionElapsedMS; // Number of ms for selecting operations to be moved
    QTime potentialPositionsSelectionTimer; // Timer for selecting potential positions for operation move
    int potentialPositionsSelectionElapsedMS; // Number of ms for selecting potential move positions
    QTime posSelectionTimer; // Timer for selecting potential positions for operation move
    int posSelectionElapsedMS; // Number of ms for selecting potential move positions
    QTime opMoveTimer; // Timer for selecting potential positions for operation move
    int opMoveElapsedMS; // Number of ms for selecting potential move positions
    QTime opMoveBackTimer; // Timer for selecting potential positions for operation move
    int opMoveBackElapsedMS; // Number of ms for selecting potential move positions
    QTime opMovePossibleTimer; // Timer for checking feasibility of the move
    int opMovePossibleElapsedMS; // Number of ms for checking the feasibility of the move
    QTime dynTopSortTimer; // Timer for DTO
    int dynTopSortElapsedMS; // Number of ms for DTO routine
    QTime updateCritNodesTimer; // Timer updating critical nodes
    int updateCritNodesElapsedMS; // Number of ms for updating the critical nodes
    QTime longestPathsTimer; // Timer for calculating the longest path in the graph
    int longestPathsElapsedMS; // Number of ms for calculating the longest paths

    QTime totalChecksTimer; // Timer for performing total checks in preprocessing and postprocessing
    int totalChecksElapsedMS; // Number of ms for total checking

    QTime blocksExecTimer; // Timer for total timing
    int blocksExecElapsedMS; // Number of ms elapsed for total timing
    QTime totalTimer; // Timer for total timing
    int totalElapsedMS; // Number of ms elapsed for total timing

public:

    LocalSearchPM();
    LocalSearchPM(LocalSearchPM& orig);
    virtual ~LocalSearchPM();

    LocalSearchPM* clone() override;

    /** Set the process model for the local search. */
    void setPM(ProcessModel *pm);

    /** Set the pointer to the resources available. */
    void setResources(Resources *rc);

    /** Set the objective. */
    void setObjective(ScalarObjective* obj);

    /** Set the generator of random numbers. */
    void setRandGens(Common::Interfaces::RandGen<Math::uint32>* irg, Common::Interfaces::RandGen<double>* frg);

    void setCritNodesUpdateFreq(const int& freq);

    void setBestPosToMove(const bool& active);

    /* ---------------------  Solver  --------------------------------------- */

    virtual void parse(const SchedulerOptions&) override;

    virtual Schedule solve(const SchedulingProblem& problem/*, const SchedulerOptions& options*/) override;

    /* ---------------------  Flow control relevant  ------------------------ */

    /** Initialize the scheduler. */
    virtual void init();

    /** One step of the VNS heuristic. */
    virtual void stepActions();

    /** Assess the new solution. */
    virtual void assessActions();

    /** Check whether the new solution should be accepted. */
    virtual bool acceptCondition();

    /** Actions in case the new solution is accepted. */
    virtual void acceptActions();

    /** Actions in case the new solution is declined. */
    virtual void declineActions();

    /** Stopping condition for the algorithm. */
    virtual bool stopCondition();

    /** Actions to be performed after the stop condition has been met. */
    virtual void stopActions();

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions();

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions();

    /* ---------------------------------------------------------------------- */

    /* ------------------------  Search  relevant  -------------------------- */

    /** Transition considering all possible parallel 
     *  machines and inset positions. */
    void transitionPM();

    /** Transition by reversing only some critical arc. */
    void transitionCP();

    /** Find longest path from the start node of the graph to the given node. */
    Path<ListDigraph> longestPath(const ListDigraph::Node &node);

    /** Find longest paths to the given nodes in only one run of the Bellmann-Ford algorithm. */
    QList<Path<ListDigraph> > longestPaths(const QList<ListDigraph::Node> &nodes);

    /** Get an arbitrary path from the start node to the given node. */
    Path<ListDigraph> randomPath(const ListDigraph::Node & node);

    /** Update the set of critical nodes in the graph. */
    void updateCriticalNodes();

    /** Select an operation to move. */
    ListDigraph::Node selectOperToMove(const Path<ListDigraph> &cpath);

    /** Select the first best operation which can be moved. */
    ListDigraph::Node defaultSelectOperToMove(const Path<ListDigraph> &cpath);

    /** Select operation to move in crit. arc reversing method. */
    void selectOperToMoveCP(const Path<ListDigraph> &cpath, ListDigraph::Node &optomove, QPair<ListDigraph::Node, ListDigraph::Node> &atb);

    /** Select the target machine which the specified operation must be moved to. */
    int selectTargetMach(const ListDigraph::Node& optomove);

    /** Find relevant arcs corresponding to the pairs of operations processed
     *  by the same tool group. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcs(const Path<ListDigraph> &cpath, const ListDigraph::Node& node);

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcsNew(const Path<ListDigraph> &cpath, const ListDigraph::Node& node);

    /** Select breakable arcs on the specified machine. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectBreakableArcs(const int& mid);

    /** Find relevant pairs of operation to insert the new operation between them. The pairs of operations correspond to 
     *  arcs laying on the specified path. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcsFromPath(const Path<ListDigraph> &path, const ListDigraph::Node& node);

    /** Check whether the move of the operation is possible. */
    bool moveOperPossible(const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node& node);

    /** Select an arc corresponding to the pair of operations satisfying the
     *  condition that after moving the selected operation no cycles occur. */
    QPair<ListDigraph::Node, ListDigraph::Node> selectArcToBreak(const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node);

    /** Select the best position to insert the operation. */
    QPair<ListDigraph::Node, ListDigraph::Node> selectBestArcToBreak(const int& mid, const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node);

    /** Find the best possible move of the operation which reduces the objective
     *  the most. Returns the machine and the pair of operation the selected 
     *  operation must be moved to. */
    void findBestOperMove(const ListDigraph::Node& optm, int& targetMachID, QPair<ListDigraph::Node, ListDigraph::Node>& atb);

    /** Move an operation from one machine to another one. */
    void moveOper(const int& mid, const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node &node);

    /** Restore the state of the graph in which it was before the move. */
    void moveBackOper(const ListDigraph::Node &node);

    /** Select some terminal node to which the critical path will be searched.
     *  The probability that the terminal is selected is proportional to its'
     *  contribution to the criterion. */
    ListDigraph::Node selectTerminalContrib(QList<ListDigraph::Node> &terminals);

    /** Select some terminal node to which the critical path will be searched.
     *  The terminal is selected randomly. */
    ListDigraph::Node selectTerminalRnd(QList<ListDigraph::Node> &terminals);

    /** Select some terminal node to which contributes the least to the objective. */
    ListDigraph::Node selectTerminalNonContrib(QList<ListDigraph::Node> &terminals);

    /** Perform random diversification of the solution in order to avoid local
     *  optima. */
    void diversify();

    /** Update the graph after the transition for the fast transition evaluation. */
    void updateEval(const ListDigraph::Node& iNode, const ListDigraph::Node& t);

    /** Dynamically update the topological ordering of the graph. */
    void dynUpdateTopolOrdering(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &j, const ListDigraph::Node &k);

    /** Specify movable nodes */
    void setMovableNodes(QMap<ListDigraph::Node, bool>& movableNodes);

    /* ---------------------------------------------------------------------- */

    /* -------------------------  DEBUG    -----------------------------------*/

    inline void checkCorrectness(const bool& check = true) {
	_check_correctness = check;
    }

    /** Check whether node t is reachable from node s. */
    bool reachable(const ListDigraph::Node& s, const ListDigraph::Node& t);

    void setScheduledTGs(const QList<int>& stgs) {
	scheduledtgs = stgs;
    }

    /** Check whether the process model is correct. */
    bool debugCheckPMCorrectness(const QString& location);

};

/** A modified version of LSPM where only the operation heads and completion times are updated simultaneously. */
class LocalSearchModPM : public IterativeAlg {
private:
    ProcessModel *pm; // Current process model
    Resources *rc;

    //QList<ListDigraph::Node> terminals; // Terminal nodes in the graph

    double bestobj; // Objective value before the step of the algorithm
    double prevobj; // Previous objective value

    double curobj; // Objective value after the step of the algorithm

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsRem; // Arcs which have been removed from the graph at the last step of the algorithm
    QList<double> weightsRem; // Weights of the removed arcs
    QList<ListDigraph::Arc> arcsIns; // Arcs which have been inserted at the last step of the algorithm
    int remMachID; // The previous machine assignment of the operation. Useful if the last operation is removed from the machine
    ListDigraph::Node optomove; // Operation to be moved

    QList<ListDigraph::Node> criticalNodes; // Set of critical nodes from which the operation to move is selected

    // Notice: Reversing an arc is equal to removing it and inserting the reversed one
    ListDigraph::Node nodeI; // The node which has been moved during the last step
    ListDigraph::Node nodeT; // The node which was folloving nodeI during the last step

    //QList<ListDigraph::Node> topolSorted; // Nodes which are sorted topologically
    QList<ListDigraph::Node> topolOrdering; // Topological ordering of all nodes of the graph
    int topolITStart; // Index to start updating the nodes with
    QList<ListDigraph::Node> prevTopolOrdering; // Previous topological ordering for restoring purposes
    int prevTopolITStart; // Previous index to start updating the nodes with

    QHash<int, QPair<double, double> > prevRS; // The preserved values of the ready times and the start times

    bool acceptedworse; // Indicates whether a worse solution has been accepted

    int nisteps; // Number of non-improvement steps

    double alpha;

    TWT obj; // Primary optimization criterion

    bool _check_correctness; // Defines whether correctness should be checked

    int critNodesUpdateFreq; // Frequency with which the critical nodes are updated

    QMap<ListDigraph::Node, bool> node2Movable; // Indicates whether the node can be moved during the search procedures

    // ############ DEBUG ################
    QList<int> scheduledtgs;

    QTime topSortTimer; // Timer for topological sorting
    int topSortElapsedMS; // Number of ms elapsed for topological sorting
    QTime objTimer; // Timer for objective function
    int objElapsedMS; // Number of ms elapsed for calculating the objective
    QTime updateEvalTimer; // Timer for objective function
    int updateEvalElapsedMS; // Number of ms elapsed for calculating the objective
    QTime opSelectionTimer; // Timer for selecting operations to be moved
    int opSelectionElapsedMS; // Number of ms for selecting operations to be moved
    QTime potentialPositionsSelectionTimer; // Timer for selecting potential positions for operation move
    int potentialPositionsSelectionElapsedMS; // Number of ms for selecting potential move positions
    QTime posSelectionTimer; // Timer for selecting potential positions for operation move
    int posSelectionElapsedMS; // Number of ms for selecting potential move positions
    QTime opMoveTimer; // Timer for selecting potential positions for operation move
    int opMoveElapsedMS; // Number of ms for selecting potential move positions
    QTime opMoveBackTimer; // Timer for selecting potential positions for operation move
    int opMoveBackElapsedMS; // Number of ms for selecting potential move positions
    QTime opMovePossibleTimer; // Timer for checking feasibility of the move
    int opMovePossibleElapsedMS; // Number of ms for checking the feasibility of the move
    QTime dynTopSortTimer; // Timer for DTO
    int dynTopSortElapsedMS; // Number of ms for DTO routine
    QTime updateCritNodesTimer; // Timer updating critical nodes
    int updateCritNodesElapsedMS; // Number of ms for updating the critical nodes
    QTime longestPathsTimer; // Timer for calculating the longest path in the graph
    int longestPathsElapsedMS; // Number of ms for calculating the longest paths

    QTime totalChecksTimer; // Timer for performing total checks in preprocessing and postprocessing
    int totalChecksElapsedMS; // Number of ms for total checking

    QTime blocksExecTimer; // Timer for total timing
    int blocksExecElapsedMS; // Number of ms elapsed for total timing
    QTime totalTimer; // Timer for total timing
    int totalElapsedMS; // Number of ms elapsed for total timing

public:

    LocalSearchModPM();
    LocalSearchModPM(LocalSearchModPM& orig);
    virtual ~LocalSearchModPM();

    /** Set the process model for the local search. */
    void setPM(ProcessModel *pm);

    /** Set the pointer to the resources available. */
    void setResources(Resources *rc);

    /* ---------------------  Flow control relevant  ------------------------ */

    /** Initialize the scheduler. */
    virtual void init();

    /** One step of the VNS heuristic. */
    virtual void stepActions();

    /** Assess the new solution. */
    virtual void assessActions();

    /** Check whether the new solution should be accepted. */
    virtual bool acceptCondition();

    /** Actions in case the new solution is accepted. */
    virtual void acceptActions();

    /** Actions in case the new solution is declined. */
    virtual void declineActions();

    /** Stopping condition for the algorithm. */
    virtual bool stopCondition();

    /** Actions to be performed after the stop condition has been met. */
    virtual void stopActions();

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions();

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions();

    /* ---------------------------------------------------------------------- */

    /* ------------------------  Search  relevant  -------------------------- */

    /** Transition considering all possible parallel 
     *  machines and inset positions. */
    void transitionPM();

    /** Transition by reversing only some critical arc. */
    void transitionCP();

    /** Find longest path from the start node of the graph to the given node. */
    Path<ListDigraph> longestPath(const ListDigraph::Node &node);

    /** Find longest paths to the given nodes in only one run of the Bellmann-Ford algorithm. */
    QList<Path<ListDigraph> > longestPaths(const QList<ListDigraph::Node> &nodes);

    /** Get an arbitrary path from the start node to the given node. */
    Path<ListDigraph> randomPath(const ListDigraph::Node & node);

    /** Update the set of critical nodes in the graph. */
    void updateCriticalNodes();

    /** Select an operation to move. */
    ListDigraph::Node selectOperToMove(const Path<ListDigraph> &cpath);

    /** Select the first best operation which can be moved. */
    ListDigraph::Node defaultSelectOperToMove(const Path<ListDigraph> &cpath);

    /** Select operation to move in crit. arc reversing method. */
    void selectOperToMoveCP(const Path<ListDigraph> &cpath, ListDigraph::Node &optomove, QPair<ListDigraph::Node, ListDigraph::Node> &atb);

    /** Select the target machine which the specified operation must be moved to. */
    int selectTargetMach(const ListDigraph::Node& optomove);

    /** Find relevant arcs corresponding to the pairs of operations processed
     *  by the same tool group. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcs(const Path<ListDigraph> &cpath, const ListDigraph::Node& node);

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcsNew(const Path<ListDigraph> &cpath, const ListDigraph::Node& node);

    /** Select breakable arcs on the specified machine. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectBreakableArcs(const int& mid);

    /** Find relevant pairs of operation to insert the new operation between them. The pairs of operations correspond to 
     *  arcs laying on the specified path. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcsFromPath(const Path<ListDigraph> &path, const ListDigraph::Node& node);

    /** Check whether the move of the operation is possible. */
    bool moveOperPossible(const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node& node);

    /** Select an arc corresponding to the pair of operations satisfying the
     *  condition that after moving the selected operation no cycles occur. */
    QPair<ListDigraph::Node, ListDigraph::Node> selectArcToBreak(const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node);

    /** Select the best position to insert the operation. */
    QPair<ListDigraph::Node, ListDigraph::Node> selectBestArcToBreak(const int& mid, const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node);

    /** Find the best possible move of the operation which reduces the objective
     *  the most. Returns the machine and the pair of operation the selected 
     *  operation must be moved to. */
    void findBestOperMove(const ListDigraph::Node& optm, int& targetMachID, QPair<ListDigraph::Node, ListDigraph::Node>& atb);

    /** Move an operation from one machine to another one. */
    void moveOper(const int& mid, const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node &node);

    /** Restore the state of the graph in which it was before the move. */
    void moveBackOper(const ListDigraph::Node &node);

    /** Select some terminal node to which the critical path will be searched.
     *  The probability that the terminal is selected is proportional to its'
     *  contribution to the criterion. */
    ListDigraph::Node selectTerminalContrib(QList<ListDigraph::Node> &terminals);

    /** Select some terminal node to which the critical path will be searched.
     *  The terminal is selected randomly. */
    ListDigraph::Node selectTerminalRnd(QList<ListDigraph::Node> &terminals);

    /** Select some terminal node to which contributes the least to the objective. */
    ListDigraph::Node selectTerminalNonContrib(QList<ListDigraph::Node> &terminals);

    /** Perform random diversification of the solution in order to avoid local
     *  optima. */
    void diversify();

    /** Update the graph after the transition for the fast transition evaluation. */
    void updateEval(const ListDigraph::Node& iNode, const ListDigraph::Node& t);

    /** Dynamically update the topological ordering of the graph. */
    void dynUpdateTopolOrdering(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &j, const ListDigraph::Node &k);

    /** Specify movable nodes */
    void setMovableNodes(QMap<ListDigraph::Node, bool>& movableNodes);

    /* ---------------------------------------------------------------------- */

    /* -------------------------  DEBUG    -----------------------------------*/

    inline void checkCorrectness(const bool& check = true) {
	_check_correctness = check;
    }

    /** Check whether node t is reachable from node s. */
    bool reachable(const ListDigraph::Node& s, const ListDigraph::Node& t);

    void setScheduledTGs(const QList<int>& stgs) {
	scheduledtgs = stgs;
    }

    /** Check whether the process model is correct. */
    bool debugCheckPMCorrectness(const QString& location);

};

/** An improved version of the local search heuristic with a better considering of the critical paths */
class LocalSearchPMCP : public IterativeAlg {
private:
    ProcessModel *pm; // Current process model
    Resources *rc;

    //QList<ListDigraph::Node> terminals; // Terminal nodes in the graph

    double bestobj; // Objective value before the step of the algorithm
    double prevobj; // Previous objective value

    double curobj; // Objective value after the step of the algorithm

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > arcsRem; // Arcs which have been removed from the graph at the last step of the algorithm
    QList<double> weightsRem; // Weights of the removed arcs
    QList<ListDigraph::Arc> arcsIns; // Arcs which have been inserted at the last step of the algorithm
    int remMachID; // The previous machine assignment of the operation. Useful if the last operation is removed from the machine
    ListDigraph::Node optomove; // Operation to be moved

    QList<ListDigraph::Node> criticalNodes; // Set of critical nodes from which the operation to move is selected

    // Notice: Reversing an arc is equal to removing it and inserting the reversed one
    ListDigraph::Node nodeI; // The node which has been moved during the last step
    ListDigraph::Node nodeT; // The node which was folloving nodeI during the last step

    QList<ListDigraph::Node> topolOrdering; // Topological ordering of all nodes of the graph
    int topolITStart; // Index to start updating the nodes with
    QList<ListDigraph::Node> prevTopolOrdering; // Previous topological ordering for restoring purposes
    int prevTopolITStart; // Previous index to start updating the nodes with

    QHash<int, QList<int> > opID2CPPredOpIDs; // Predecessor operation IDs lying on the critical paths
    QHash<int, ListDigraph::Node> opID2Node; // A mapping of the operation IDs onto the corresponding nodes of the PM

    QHash<int, QPair<double, double> > prevRS; // The preserved values of the ready times and the start times

    bool acceptedworse; // Indicates whether a worse solution has been accepted

    int nisteps; // Number of non-improvement steps

    double alpha;

    TWT obj; // Primary optimization criterion

    bool _check_correctness; // Defines whether correctness should be checked

    int critNodesUpdateFreq; // Frequency with which the critical nodes are updated

    QMap<ListDigraph::Node, bool> node2Movable; // Indicates whether the node can be moved during the search procedures

    // ############ DEBUG ################
    QList<int> scheduledtgs;

    QTime topSortTimer; // Timer for topological sorting
    int topSortElapsedMS; // Number of ms elapsed for topological sorting
    QTime objTimer; // Timer for objective function
    int objElapsedMS; // Number of ms elapsed for calculating the objective
    QTime updateEvalTimer; // Timer for objective function
    int updateEvalElapsedMS; // Number of ms elapsed for calculating the objective
    QTime opSelectionTimer; // Timer for selecting operations to be moved
    int opSelectionElapsedMS; // Number of ms for selecting operations to be moved
    QTime potentialPositionsSelectionTimer; // Timer for selecting potential positions for operation move
    int potentialPositionsSelectionElapsedMS; // Number of ms for selecting potential move positions
    QTime posSelectionTimer; // Timer for selecting potential positions for operation move
    int posSelectionElapsedMS; // Number of ms for selecting potential move positions
    QTime opMoveTimer; // Timer for selecting potential positions for operation move
    int opMoveElapsedMS; // Number of ms for selecting potential move positions
    QTime opMoveBackTimer; // Timer for selecting potential positions for operation move
    int opMoveBackElapsedMS; // Number of ms for selecting potential move positions
    QTime opMovePossibleTimer; // Timer for checking feasibility of the move
    int opMovePossibleElapsedMS; // Number of ms for checking the feasibility of the move
    QTime dynTopSortTimer; // Timer for DTO
    int dynTopSortElapsedMS; // Number of ms for DTO routine
    QTime updateCritNodesTimer; // Timer updating critical nodes
    int updateCritNodesElapsedMS; // Number of ms for updating the critical nodes
    QTime longestPathsTimer; // Timer for calculating the longest path in the graph
    int longestPathsElapsedMS; // Number of ms for calculating the longest paths

    QTime totalChecksTimer; // Timer for performing total checks in preprocessing and postprocessing
    int totalChecksElapsedMS; // Number of ms for total checking

    QTime blocksExecTimer; // Timer for total timing
    int blocksExecElapsedMS; // Number of ms elapsed for total timing
    QTime totalTimer; // Timer for total timing
    int totalElapsedMS; // Number of ms elapsed for total timing

public:

    LocalSearchPMCP();
    LocalSearchPMCP(LocalSearchPMCP& orig);
    virtual ~LocalSearchPMCP();

    /** Set the process model for the local search. */
    void setPM(ProcessModel *pm);

    /** Set the pointer to the resources available. */
    void setResources(Resources *rc);

    /* ---------------------  Flow control relevant  ------------------------ */

    /** Initialize the scheduler. */
    virtual void init();

    /** One step of the VNS heuristic. */
    virtual void stepActions();

    /** Assess the new solution. */
    virtual void assessActions();

    /** Check whether the new solution should be accepted. */
    virtual bool acceptCondition();

    /** Actions in case the new solution is accepted. */
    virtual void acceptActions();

    /** Actions in case the new solution is declined. */
    virtual void declineActions();

    /** Stopping condition for the algorithm. */
    virtual bool stopCondition();

    /** Actions to be performed after the stop condition has been met. */
    virtual void stopActions();

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions();

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions();

    /* ---------------------------------------------------------------------- */

    /* ------------------------  Search  relevant  -------------------------- */

    /** Transition considering all possible parallel 
     *  machines and inset positions. */
    void transitionPM();

    /** Transition by reversing only some critical arc. */
    void transitionCP();

    /** Find longest path from the start node of the graph to the given node. */
    Path<ListDigraph> longestPath(const ListDigraph::Node &node);

    /** Find longest paths to the given nodes in only one run of the Bellmann-Ford algorithm. */
    QList<Path<ListDigraph> > longestPaths(const QList<ListDigraph::Node> &nodes);

    /** A critical path to the given node. */
    QList<ListDigraph::Node> criticalPath(const ListDigraph::Node& node);

    /** Get an arbitrary path from the start node to the given node. */
    Path<ListDigraph> randomPath(const ListDigraph::Node & node);

    /** Update the set of critical nodes in the graph. */
    void updateCriticalNodes();

    /** Select an operation to move. */
    ListDigraph::Node selectOperToMove(const Path<ListDigraph> &cpath);

    /** Select the first best operation which can be moved. */
    ListDigraph::Node defaultSelectOperToMove(const Path<ListDigraph> &cpath);

    /** Select operation to move in crit. arc reversing method. */
    void selectOperToMoveCP(const Path<ListDigraph> &cpath, ListDigraph::Node &optomove, QPair<ListDigraph::Node, ListDigraph::Node> &atb);

    /** Select the target machine which the specified operation must be moved to. */
    int selectTargetMach(const ListDigraph::Node& optomove);

    /** Find relevant arcs corresponding to the pairs of operations processed
     *  by the same tool group. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcs(const Path<ListDigraph> &cpath, const ListDigraph::Node& node);

    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcsNew(const Path<ListDigraph> &cpath, const ListDigraph::Node& node);

    /** Select breakable arcs on the specified machine. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectBreakableArcs(const int& mid);

    /** Find relevant pairs of operation to insert the new operation between them. The pairs of operations correspond to 
     *  arcs laying on the specified path. */
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selectRelevantArcsFromPath(const Path<ListDigraph> &path, const ListDigraph::Node& node);

    /** Check whether the move of the operation is possible. */
    bool moveOperPossible(const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node& node);

    /** Select an arc corresponding to the pair of operations satisfying the
     *  condition that after moving the selected operation no cycles occur. */
    QPair<ListDigraph::Node, ListDigraph::Node> selectArcToBreak(const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node);

    /** Select the best position to insert the operation. */
    QPair<ListDigraph::Node, ListDigraph::Node> selectBestArcToBreak(const int& mid, const QList<QPair<ListDigraph::Node, ListDigraph::Node> > &arcs, const ListDigraph::Node &node);

    /** Find the best possible move of the operation which reduces the objective
     *  the most. Returns the machine and the pair of operation the selected 
     *  operation must be moved to. */
    void findBestOperMove(const ListDigraph::Node& optm, int& targetMachID, QPair<ListDigraph::Node, ListDigraph::Node>& atb);

    /** Move an operation from one machine to another one. */
    void moveOper(const int& mid, const ListDigraph::Node &j, const ListDigraph::Node &k, const ListDigraph::Node &node);

    /** Restore the state of the graph in which it was before the move. */
    void moveBackOper(const ListDigraph::Node &node);

    /** Select some terminal node to which the critical path will be searched.
     *  The probability that the terminal is selected is proportional to its'
     *  contribution to the criterion. */
    ListDigraph::Node selectTerminalContrib(QList<ListDigraph::Node> &terminals);

    /** Select some terminal node to which the critical path will be searched.
     *  The terminal is selected randomly. */
    ListDigraph::Node selectTerminalRnd(QList<ListDigraph::Node> &terminals);

    /** Select some terminal node to which contributes the least to the objective. */
    ListDigraph::Node selectTerminalNonContrib(QList<ListDigraph::Node> &terminals);

    /** Perform random diversification of the solution in order to avoid local
     *  optima. */
    void diversify();

    /** Update the graph after the transition for the fast transition evaluation. */
    void updateEval(const ListDigraph::Node& iNode, const ListDigraph::Node& t);

    /** Dynamically update the topological ordering of the graph. */
    void dynUpdateTopolOrdering(QList<ListDigraph::Node> &topolOrdering, const ListDigraph::Node &i, const ListDigraph::Node &j, const ListDigraph::Node &k);

    /** Specify movable nodes */
    void setMovableNodes(QMap<ListDigraph::Node, bool>& movableNodes);

    /* ---------------------------------------------------------------------- */

    /* -------------------------  DEBUG    -----------------------------------*/

    inline void checkCorrectness(const bool& check = true) {
	_check_correctness = check;
    }

    /** Check whether node t is reachable from node s. */
    bool reachable(const ListDigraph::Node& s, const ListDigraph::Node& t);

    void setScheduledTGs(const QList<int>& stgs) {
	scheduledtgs = stgs;
    }

    /** Check whether the process model is correct. */
    bool debugCheckPMCorrectness(const QString& location);

};

/** Scheduler that is empowered by the local search technique. Runs only the local search. */
class LSScheduler : public Scheduler {
    Q_OBJECT
public:
    LocalSearchPMCP ls;

    LSScheduler();
    LSScheduler(LSScheduler& orig);
    virtual ~LSScheduler();

    /** Initialization of the scheduler. */
    virtual void init();

    /** The scheduling is actually performed here. */
    virtual void scheduleActions();

};

#endif /* LOCALSEARCHPM_H */

