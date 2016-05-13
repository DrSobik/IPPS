/* 
 * File:   Resources.h
 * Author: DrSobik
 *
 * Created on July 1, 2011, 11:08 AM
 * 
 * Description:	This file contains definitions of Machine class as well as
 *				the ToolGroup class, which represents some tool group with a 
 *				number of machines with same functionality but possibly 
 *				different productivities in it.
 * 
 */

#ifndef RESOURCES_H
#define	RESOURCES_H

#include <QSet>
#include <QVector>
#include <QList>
#include <QTextStream>
#include <QHash>
#include <QStringList>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "Operation"
#include "MathExt"
#include "RandExt"

using namespace Common;

/*
 * Description: Class Machine implements some machine in the production system.
 * 
 * Contained data:
 *
 * 				ID	-	unique ID of the machine within the production system.
 * 
 *				type-	type of the operations, which can be executed on this 
 *						machine.
 *		
 *				type2speed
 *					-	processing speed coefficient depending on the type of 
 *						the operations which could be processed on the machine.
 *						Ex.: same operation but two different materials. 
 *						1/time2speed[type] is time needed to process a unit 
 *						etalon operation of the type on the etalon machine.
 * 
 *				t	-	time point, at which the machine finishes the last
 *						operation scheduled on it and becomes available for any
 *						new operations.
 */

class Machine {
protected:
	double topt; // Total clean processing time of all operations that have ever been scheduled on the machine

	double timeAvail; // Time point at which the machine becomes available

public:
	int ID;

	QHash<int, double> type2speed;
	double t;

	QList<Operation*> operations;

public:
	Machine();
	Machine(Machine& orig);
	virtual ~Machine();

	/* -----------------------  Utils  -------------------------------------- */

	/** Initializes this machine. */
	virtual void init();

	virtual void clear();

	/** Assigns all of the data of the "other" machine to this machine. */
	virtual void copy(Machine& other);

	/** Copy operator. */
	const Machine& operator=(Machine& other);

	/** Friend operator that checks whether machines are the same. Comparison
	 *  is based on the unique IDs of the machines. */
	friend bool operator==(const Machine& machine1, const Machine& machine2) {
		return machine1.ID == machine2.ID;
	}

	/** Extract the machine from the given DOM element. */
	void fromDOMElement(const QDomElement &melem);

	/** Write information about the current state of the machine. */
	friend QTextStream& operator<<(QTextStream &out, Machine &m);
	friend QXmlStreamReader& operator>>(QXmlStreamReader& reader, Machine& m);
	friend QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Machine& m);

	/* ---------------------------------------------------------------------- */

	/* ------------------------  Scheduling relevant  ----------------------- */

	inline void timeAvailable(const double& tAvail) {
		timeAvail = tAvail;

		t = timeAvail;
	}

	inline double timeAvailable() {
		return timeAvail;
	}

	/** Get the time point when the machine will be available again. */
	inline double time() {
		return t;
	}

	/** Add an operation to the machine. Operations completion time and the
	 *	new availability time of the machine will be set. Returns true if the 
	 *	operation has been assigned successfully. */
	virtual Machine& operator<<(Operation *o);

	/** Processing time of the operation on the machine. Depends on the 
	 *  operation and the speed of the machine.*/
	virtual double procTime(const Operation* o);

	/** Utilization of the machine. Sum of processing times of all the operations / total processing time. */
	virtual double util();

	/* ---------------------------------------------------------------------- */

private:

};

/*
 * Description: Class ToolGroup implements a group of machines, which are able 
 *				to execute similar operations. Thus, one ToolGroup can be 
 *				treated as a set of parallel machines, which have not 
 *				necessarily similar productivities.
 *				
 * Contained data:
 *				
 *				ID	-	id of the tool group.
 * 
 *				type-	type of the operations, which can be executed by
 *						machines of this tool group.
 *
 *				machines-container of references to the machines in this tool
 *						 group.
 * 
 *				mid2idx	-index of the machine based on its id.
 *													
 */

class ToolGroup {
public:

	int ID;

	QSet<int> types;

	QVector<Machine*> _machines;
	QHash<int, int> mid2idx;

public:
	ToolGroup();
	ToolGroup(ToolGroup& orig);
	virtual ~ToolGroup();

	/* -----------------------  Utils  -------------------------------------- */

	/** Initialize the tool group. */
	virtual void init();

	/** Clear the tool group */
	virtual void clear();

	/** Copy the tool group. */
	virtual void copy(ToolGroup& other);

	/** Copy operator. */
	ToolGroup& operator=(ToolGroup& other);

	/** Friend operator that checks whether the tool groups are the same.
	 *  Comparison is based on the unique IDs of the tool groups. */
	friend bool operator==(const ToolGroup& tg1, const ToolGroup& tg2) {
		return tg1.ID == tg2.ID;
	}

	/** Add machine to the tool group. */
	virtual ToolGroup& operator<<(Machine *machine) {
		this->types += machine->type2speed.keys().toSet();

		_machines.append(machine);
		mid2idx[machine->ID] = _machines.size() - 1;

		return *this;
	}

	/** Machine of the tool group by its ID. */
	inline Machine& operator()(const int ID) {
		return *(_machines.at(mid2idx[ID]));
	}

	/** The i-th indexed machine of the machine group. */
	inline Machine& operator[](const int i) {
		return *(_machines.at(i));
	}

	/** List of all machines in the tool group. */
	inline QList<Machine*> machines() {
		return _machines.toList();
	}

	/** List of machines in the tool group able to process the operation type. */
	inline QList<Machine*> machines(const int& otype) {
		QList<Machine*> res;

		for (int i = 0; i < _machines.size(); i++) {
			if (_machines[i]->type2speed.contains(otype)) {
				res.append(_machines[i]);
			}
		}

		return res;
	}

	virtual void fromDOMElement(const QDomElement &tgelem);

	friend QTextStream& operator<<(QTextStream &out, ToolGroup &tg);

	/* ---------------------------------------------------------------------- */

	/* ------------------------  Scheduling relevant  ----------------------- */

	/** Returns randomly selected machine of this tool group. */
	virtual Machine& randomMachine();

	/** Returns next earliest available machine of this tool group. */
	virtual Machine& nextAvailable();

	virtual Machine& nextAvailable(const int &type);

	/** Fastest machine available at time point t for the operation type. */
	virtual Machine& fastestAvailable(const double &t, const int &type);

	/** Fastest machine capable to process operations of the given type. */
	virtual Machine& fastestMachine(const int &type);

	/** Slowest machine capable to process operations of the given type. */
	virtual Machine& slowestMachine(const int &type);

	/** Return the machine which is able to finish the operation the earliest. */
	virtual Machine& earliestToFinish(const Operation *o);

	/** Expected processing speed of the machines capable of processing the operation type. */
	double expectedSpeed(const int& type);

	/** Expected processing time of the operation by the tool group. */
	double expectedProcTime(Operation *o);

	/** Shortest processing time of the operation by the tool group. */
	double shortestProcTime(Operation *o);

	/** Longest processing time of the operation by the tool group. */
	double longestProcTime(Operation *o);

	/** Average utilization of the machines in this tool group. */
	double avgUtil();

	/* ---------------------------------------------------------------------- */

private:

};

/*
 * Description: Class Resources implements production facilities of the
 *				manufacturing system.
 * 
 * Contained data:
 *				
 *				ID	-	ID of the whole set of resources.
 *		
 *				tools - tool groups in the resource.
 *			
 *				tid2idx-	index of the tool group based on its ID.
 * 
 *				type2idx-
 *						index of the tool group based on its type.
 */

class Resources {
public:
	int ID;

	QVector<ToolGroup*> tools;
	QHash<int, int> tid2idx;
	QHash<int, QList<int> > type2idcs;

	static int rcCreated;
	static int rcDeleted;

public:
	Resources();
	Resources(Resources& orig);
	virtual ~Resources();

	virtual Resources& operator=(Resources& other);

	/* -----------------------  Utils  -------------------------------------- */

	/* Initialize the resources. */
	virtual void init();

	/* Clear the resources. */
	virtual void clear();

	/** Merge all available machine groups into one group without taking machine
	 *  types into account. */
	virtual void mergeToolGroups();
	
	/** Tool group by ID. */
	inline ToolGroup& operator()(const int ID) {

#ifdef DEBUG
		if (!tid2idx.contains(ID)) {
			Debugger::err << "ToolGroup& operator() : Machine group " << ID << " not found!!!" << ENDL;
		}
#endif 

		return *(tools[tid2idx[ID]]);
	}

	/** The i-th tool group. */
	inline ToolGroup& operator[](const int i) {
		return *(tools.at(i));
	}

	/** Machine of the tool group by ID. */
	inline Machine& operator()(const int tID, const int mID) {
		// IMPORTANT!!! BUG FIXED!!! id2idx in ToolGroup and in Resources classed are DIFFERENT!!! 
		//return *(tools[id2idx[tID]]->_machines[id2idx[mID]]);

		return *(tools[tid2idx[tID]]->_machines[tools[tid2idx[tID]]->mid2idx[mID]]);
	}

	/** Add tool group to the resources. */
	virtual Resources& operator<<(ToolGroup *tg) {

		tools.append(tg);
		tid2idx[tg->ID] = tools.size() - 1;

		for (QSet<int>::iterator sit = tg->types.begin(); sit != tg->types.end(); sit++) {
			//type2idx[*sit] = tools.size() - 1;
			type2idcs[*sit].append(tools.size() - 1);
		}

		return *this;
	}

	/** All machines in all of the tool groups. */
	inline QList<Machine*> machines() {
		QList<Machine*> res;

		for (int i = 0; i < tools.size(); i++) {
			res.append(tools[i]->machines());
		}

		return res;
	}

	/** Extract the resources from the DOM Element. */
	virtual void fromDOMElement(const QDomElement &rcelem);

	friend QTextStream& operator<<(QTextStream &out, Resources &rc);
	
	/* ---------------------------------------------------------------------- */

	/* ------------------------  Scheduling relevant  ----------------------- */

	/** The next available machine in all tool groups. Does not take machine
	 *	types into consideration. */
	virtual Machine& nextAvailable();

	/** Assign the operation to the corresponding tool group. Will assign
	 *  the correct index of the corresponding tool to the "tool" field of the 
	 *  operation based on the type of the operation. */
	virtual bool assign(Operation *o) {
		//if (!type2idx.contains(o->type)) return false;
		//o->toolID = tools[type2idx[o->type]]->ID;

		if (!type2idcs.contains(o->type)) return false;
		//o->toolID = tools[type2idcs[o->type][Rand::rndInt(0, type2idcs[o->type].size() - 1)]]->ID;
		o->toolID = tools[type2idcs[o->type][Rand::rnd<Math::uint32>(0, type2idcs[o->type].size() - 1)]]->ID;

		return true;
	}

	/* ---------------------------------------------------------------------- */
};

#endif	/* RESOURCES_H */

