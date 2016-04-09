/* 
 * File:   GAPlanner.h
 * Author: DrSobik
 *
 * Created on February 26, 2013, 4:00 PM
 */

#ifndef GAPLANNER_H
#define	GAPLANNER_H

#include <ga/GAGenome.h>
#include <ga/ga.h>

#include <iomanip>

#include "MathExt"
#include "DebugExt.h"

#include "Plan.h"
#include "Schedule.h"
#include "Planner.h"
#include "Scheduler.h"
#include "Operation.h"

#include "ProcessModel.h"
#include "ProcessModelManager.h"
#include "Resources.h"
#include "Objective.h"

#include <QList>

using namespace std;
using namespace Common;

class GAPlannerGenome : public GAGenome {
	friend class GAPlanner;
private:

	int _lsMaxIter;
	bool _lsChkCor;
	bool _solImprove;

	ScalarObjective* obj; // Objective for the algorithm (its pointer)
	
private:

	ProcessModelManager* pmm; // The process model manager which will be used to get information about the sctructural data of the products/orders
	Resources rc; // A local copy of the resources which are used for evaluation of the genome
	Scheduler* scheduler; // Used for scheduling (initialization etc.)

	SchedulerOptions schedOptions; // Options which must be considered by the scheduling algorithms

	QList<Operation> operations; // The representation of genomes
	QList<QPair<int, int> > operationPrecedences; // Precedence constraints between the operations in this schedule
	QList<bool > operationPrecedencesConj; // Indicates whether the corresponding precedence constraint is a technological or a schedule-based one


	Plan plan; // Plan represented by this genome
	Schedule schedule; // Schedule represented by this genome

	QList<Operation> crossSection; // The crossing section during the crossover
	int crossStart; // Start of the crossing section in the parent genome
	int crossEnd; // End of the crossing section in the parent genome

public:
	GAPlannerGenome();
	GAPlannerGenome(const GAPlannerGenome& orig);
	virtual ~GAPlannerGenome();

	// Copying the other object
	virtual void copy(const GAGenome& orig);
	GAGenome & operator=(const GAGenome &orig);

	// Cloning myself
	virtual GAGenome* clone(CloneMethod) const;

	// Output
	virtual int write(ostream &) const;

	friend QTextStream& operator<<(QTextStream& out, const GAPlannerGenome& g);

	/** Check if the orders could be correctly distributed beween FOUPs. */
	virtual bool valid();

	// ############################  Initializer   #############################
	/** Initializer specific to the genome. */
	virtual void initialize();

	/** Static initializer for the GA. */
	static void init(GAGenome &);

	// ############################  Crossover #################################
	/** Crossover specific to the genome. This pairs with the mate. */
	virtual void mate(GAPlannerGenome &mate, GAPlannerGenome& child);

	/** Static crossover for the genetic algorithm. */
	static int cross(const GAGenome &, const GAGenome &, GAGenome *, GAGenome *);

	//#############################  Mutator   #################################
	/** Mutator which changes only operations' machines. */
	virtual void mutateMach(const float prob);

	/** Mutator which changes different routes of different parts. */
	virtual void mutateRoute(const float prob);

	/** Changes BOMs for some products.  */
	virtual void mutateBOM(const float prob);

	/** Mutator specific to the genome. */
	virtual int mut(const float prob);

	/** Static genome mutator for the GA. */
	static int mutate(GAGenome &, float);

	//#############################  Reparator  ################################
	virtual void repair(const GAPlannerGenome& mom, const GAPlannerGenome& dad);

	//#############################  Comparator  ###############################
	/** Compare this genome to the other one. */
	virtual float compare(const GAGenome &other);

	/** Static comparator of the genomes for the genetic algorithm. */
	static float compare(const GAGenome &, const GAGenome &);

	//#############################  Objective function   ######################

	/** Static objective for the GA. */
	static float objective(GAGenome &);

	//#####################  Improve the current solution   ####################

	/** Run the local search in order to improve the current solution. */
	virtual void improve();

	//#############################  Routines  #################################

	/** Set a process model manager for this genome. */
	virtual void setPMM(ProcessModelManager* pmm);

	/** Set the resources. */
	virtual void setResources(Resources& origRes);

	/** Set the initial scheduler. */
	virtual void setScheduler(Scheduler* scheduler);

	/** Set the options */
	virtual void setOptions(SchedulerOptions& options);

	/** Generate a PM based on the genetic data. */
	virtual ProcessModel restorePM();
	
	virtual void setObjective(ScalarObjective* o);

};

class GAPlannerGenome1 : public GAGenome {
	friend class GAPlanner1;
private:

	int _lsMaxIter;
	bool _lsChkCor;
	bool _solImprove;

	ScalarObjective* obj; // Objective for the algorithm (its pointer)
	
private:

	ProcessModelManager* pmm; // The process model manager which will be used to get information about the sctructural data of the products/orders
	Resources rc; // A local copy of the resources which are used for evaluation of the genome
	Scheduler* scheduler; // Used for scheduling (initialization etc.)

	SchedulerOptions schedOptions; // Options which must be considered by the scheduling algorithms

	QList<Operation> operations; // The representation of genomes
	QList<QPair<int, int> > operationPrecedences; // Precedence constraints between the operations in this schedule
	QList<bool > operationPrecedencesConj; // Indicates whether the corresponding precedence constraint is a technological or a schedule-based one


	Plan plan; // Plan represented by this genome
	Schedule schedule; // Schedule represented by this genome

	QList<Operation> crossSection; // The crossing section during the crossover
	int crossStart; // Start of the crossing section in the parent genome
	int crossEnd; // End of the crossing section in the parent genome

	QList<QPair<int, int> > crossOrdID2OpIdx; // Used later for removing spare operations
	
public:
	GAPlannerGenome1();
	GAPlannerGenome1(const GAPlannerGenome1& orig);
	virtual ~GAPlannerGenome1();

	// Copying the other object
	virtual void copy(const GAGenome& orig);
	GAGenome & operator=(const GAGenome &orig);

	// Cloning myself
	virtual GAGenome* clone(CloneMethod) const;

	// Output
	virtual int write(ostream &) const;

	friend QTextStream& operator<<(QTextStream& out, const GAPlannerGenome1& g);

	/** Check if the orders could be correctly distributed beween FOUPs. */
	virtual bool valid();

	// ############################  Initializer   #############################
	/** Initializer specific to the genome. */
	virtual void initialize();

	/** Static initializer for the GA. */
	static void init(GAGenome &);

	// ############################  Crossover #################################
	/** Crossover specific to the genome. This pairs with the mate. */
	virtual void mate(GAPlannerGenome1& mate, GAPlannerGenome1& child);

	/** Static crossover for the genetic algorithm. */
	static int cross(const GAGenome &, const GAGenome &, GAGenome *, GAGenome *);

	//#############################  Mutator   #################################
	/** Mutator which changes only operations' machines. */
	virtual void mutateMach(const float prob);

	/** Mutator which changes different routes of different parts. */
	virtual void mutateRoute(const float prob);

	/** Changes BOMs for some products.  */
	virtual void mutateBOM(const float prob);

	/** Mutator specific to the genome. */
	virtual int mut(const float prob);

	/** NS-based mutation with the given number of permutations. */
	virtual int mutNS(const int&, const bool& withIncumbent = true);
	
	/** Static genome mutator for the GA. */
	static int mutate(GAGenome &, float);

	//#############################  Reparator  ################################
	virtual void repair(const GAPlannerGenome1& mom, const GAPlannerGenome1& dad);

	//#############################  Comparator  ###############################
	/** Compare this genome to the other one. */
	virtual float compare(const GAGenome &other);

	/** Static comparator of the genomes for the genetic algorithm. */
	static float compare(const GAGenome &, const GAGenome &);

	//#############################  Objective function   ######################

	/** Static objective for the GA. */
	static float objective(GAGenome &);

	//#####################  Improve the current solution   ####################

	/** Run the local search in order to improve the current solution. */
	virtual void improve();

	//#############################  Routines  #################################

	/** Set a process model manager for this genome. */
	virtual void setPMM(ProcessModelManager* pmm);

	/** Set the resources. */
	virtual void setResources(Resources& origRes);

	/** Set the initial scheduler. */
	virtual void setScheduler(Scheduler* scheduler);

	/** Set the options */
	virtual void setOptions(SchedulerOptions& options);

	/** Generate a PM based on the genetic data. */
	virtual ProcessModel restorePM();
	
	virtual void setObjective(ScalarObjective* o);

};

class GAPlanner : public Planner {
	Q_OBJECT

public:
	int n_cycles; // Maximum number of iterations
	int n_genomes; // Number of genomes for the GA.
	int n_secs; // Time limit for the GA (in seconds)

	double p_repl; // Replacement probability for the GA.
	double p_mut; // Mutation probability for the GA.
	double p_cross; // Crossover probability for the GA.

	int seed;

	GAPlannerGenome bestGenome; // The best solution found so far

	SchedulerOptions schedOptions; // Options which must be considered by the scheduling algorithms

	Schedule bestSched; // The best schedule found so far

	GAPlanner();
	GAPlanner(const GAPlanner& orig);
	virtual ~GAPlanner();

	virtual void init();

	/** Run the algorithm. */
	virtual void run(GAPlannerGenome &res);

	/** Run the algorithm. */
	virtual void run();

	virtual void preprocessingActions();

	virtual void postprocessingActions();

	void slotUpdateProtocol();

private:

};


class GAPlanner1 : public Planner {
	Q_OBJECT

public:
	int n_cycles; // Maximum number of iterations
	int n_genomes; // Number of genomes for the GA.
	int n_secs; // Time limit for the GA (in seconds)

	double p_repl; // Replacement probability for the GA.
	double p_mut; // Mutation probability for the GA.
	double p_cross; // Crossover probability for the GA.

	int seed;

	GAPlannerGenome bestGenome; // The best solution found so far

	SchedulerOptions schedOptions; // Options which must be considered by the scheduling algorithms

	Schedule bestSched; // The best schedule found so far

	GAPlanner1();
	GAPlanner1(const GAPlanner1& orig);
	virtual ~GAPlanner1();

	virtual void init();

	/** Run the algorithm. */
	virtual void run(GAPlannerGenome1 &res);

	/** Run the algorithm. */
	virtual void run();

	virtual void preprocessingActions();

	virtual void postprocessingActions();

	void slotUpdateProtocol();

private:

};

#endif	/* GAPLANNER_H */

