/* 
 * File:   Schedule.cpp
 * Author: DrSobik
 * 
 * Created on July 1, 2011, 1:15 PM
 */

#include "Schedule.h"

Schedule::Schedule()/* : obj(NULL)*/ {
}

/*
Schedule::Schedule(const ScalarObjective* otherObj) : obj(NULL)
{

	if (otherObj == NULL) {
		Debugger::err << "Schedule::Schedule : NULL objective!!!" << ENDL;
	}

	(*this) << (ScalarObjective*) otherObj;
}

 */

Schedule::Schedule(const Schedule& orig)/* : obj(NULL)*/ {
	*this = orig;
}

Schedule::~Schedule() {

	/*
	
	if (obj != NULL) {
		delete obj;
	}

	 */
}

void Schedule::init() {
	objective = Math::MAX_DOUBLE;

	ordID2objContrib.clear();

	operations.clear();

	operationPrecedences.clear();
	operationPrecedencesConj.clear();

	pm.clear();
}

void Schedule::clear() {
	objective = Math::MAX_DOUBLE;

	ordID2objContrib.clear();

	operations.clear();

	operationPrecedences.clear();
	operationPrecedencesConj.clear();

	pm.clear();
}

Schedule& Schedule::operator=(const Schedule& other) {
	this->objective = other.objective;
	this->ordID2objContrib = other.ordID2objContrib;
	this->operations = other.operations;
	this->operationPrecedences = other.operationPrecedences;
	this->operationPrecedencesConj = other.operationPrecedencesConj;

	//Debugger::info << "Copying PM" << ENDL;
	this->pm = other.pm;
	//Debugger::info << "Done copying PM" << ENDL;

	//	(*this) << other.obj;

	return *this;
}

/*
void Schedule::save() {
	
	getchar();
	
	_prevObj = objective;
	_prevOrdID2objContrib = ordID2objContrib;
	_prevResourceID2OperIDStartTime = resourceID2OperIDStartTime;
	_prevResourceID2ItemIDStartTime = resourceID2ItemIDStartTime;
}
 */

/*
void Schedule::restore() {
	if (_prevOrdID2objContrib.size() == 0) Debugger::warn << "Schedule::restore : Possibly restoring an empty schedule!" << ENDL;

	objective = _prevObj;
	ordID2objContrib = _prevOrdID2objContrib;

	resourceID2OperIDStartTime = _prevResourceID2OperIDStartTime;
	resourceID2ItemIDStartTime = _prevResourceID2ItemIDStartTime;
}
 */

void Schedule::fromPM(ProcessModel& pm, ScalarObjective& obj) {
	QTextStream out(stdout);

	if (&obj == NULL) {
		Debugger::err << "Schedule::fromPM : NULL objective!!!" << ENDL;
	}

	//TWT twt;
	QList<ListDigraph::Node> terminals = pm.terminals();

	init();

	// Copy the PM

	//out << "Copying the PM ... " << endl;
	this->pm = pm;
	//out << "Done copying the PM." << endl;

	objective = obj(pm); //twt(pm, terminals);

	for (int i = 0; i < terminals.size(); i++) {
		ordID2objContrib[pm.ops[terminals[i]]->OID] = pm.ops[terminals[i]]->wT();
		//Debugger::info << pm->ops[terminals[i]]->OID << ": " << pm->ops[terminals[i]]->wT() << ENDL;
	}

	// Iterate over all nodes of the PM and generate the schedule for execution
	operations.clear();

	// Get topological ordering of the nodes
	QList<ListDigraph::Node> topOrd = pm.topolSort();
	ListDigraph::Node curNode = INVALID;

	for (int i = 0; i < topOrd.size(); i++) {
		curNode = topOrd[i];

		if (pm.ops[curNode]->ID <= 0) {
			continue;
		} else {
			operations.append(*pm.ops[curNode]);

			// Collect all preceding operations for this operation
			int endOperID = pm.ops[curNode]->ID;
			int startOperID = -1;

			QStack<ListDigraph::Node> prevNodes;
			QStack<ListDigraph::Arc> prevArcs;
			for (ListDigraph::InArcIt iait(pm.graph, curNode); iait != INVALID; ++iait) {
				prevNodes.push(pm.graph.source(iait));
				prevArcs.push(iait);
			}

			ListDigraph::Node node = INVALID;
			ListDigraph::Arc arc = INVALID;

			while (!prevNodes.empty()) {
				node = prevNodes.pop();
				arc = prevArcs.pop();

				if (pm.ops[node]->ID <= 0) { // Push its predecessors
					for (ListDigraph::InArcIt iait(pm.graph, node); iait != INVALID; ++iait) {
						prevNodes.push(pm.graph.source(iait));
						prevArcs.push(iait);
					}
				} else {
					startOperID = pm.ops[node]->ID;

					// Add the found precedence
					operationPrecedences.append(QPair<int, int>(startOperID, endOperID));

					// Check whether this precedence is a technological one
					//operationPrecedencesConj.append(pm.conPathExists(node, curNode));
					operationPrecedencesConj.append(pm.conjunctive[arc]);
				}
			}

		}

	}

	//out << pm << endl;

}

/*
Schedule& Schedule::operator<<(ScalarObjective* otherObj) {

	if (otherObj == NULL) {
		Debugger::err << "Schedule::operator<< : Trying to clone a null objective!!!" << ENDL;
	}

	if (this->obj != NULL) {
		delete (this->obj);
	}

	this->obj = otherObj->clone();

	return *this;
}
 */

QTextStream& operator<<(QTextStream& out, Schedule& sched) {
	out << "Schedule : " << endl << "[ " << endl;

	for (int i = 0; i < sched.operations.size(); i++) {
		out << sched.operations[i];

		out << endl;
	}

	out << "]";

	return out;
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Schedule& sched) {

	composer.writeStartElement("schedule");

	composer.writeStartElement("operations");

	for (int i = 0; i < sched.operations.size(); i++) {

		composer << sched.operations[i];

	}

	composer.writeEndElement();

	composer.writeStartElement("precedences");

	for (int i = 0; i < sched.operationPrecedences.size(); i++) {
		composer.writeTextElement("arc", QString::number(sched.operationPrecedences[i].first) + "," + QString::number(sched.operationPrecedences[i].second) + "," + ((sched.operationPrecedencesConj[i]) ? "1" : "0"));
	}

	composer.writeEndElement(); // precedences

	composer.writeEndElement(); // schedule

	return composer;
}