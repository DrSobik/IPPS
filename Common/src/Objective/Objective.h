/* 
 * File:   Objective.h
 * Author: DrSobik
 *
 * Created on March 27, 2012, 4:14 PM
 * 
 * Description : Class Objective is a template class for representing different
 *				 Objective functions.
 * 
 */

#ifndef OBJECTIVE_H
#define	OBJECTIVE_H

#include "MathExt"

#include "ProcessModel"

#include "TGSelection"

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE ScalarObjective {
protected:

	ScalarObjective() {

	}

	ScalarObjective(const ScalarObjective&) {

	}

public:

	virtual ~ScalarObjective() {

	}

	virtual double operator()(ProcessModel&) {

		Debugger::err << "ScalarObjective::operator() : Not implemented!!!" << ENDL;

		return Math::MAX_DOUBLE;
	}

	virtual double operator()(const ProcessModel&) {

		Debugger::err << "ScalarObjective::operator() const : Not implemented!!!" << ENDL;

		return Math::MAX_DOUBLE;
	}

	virtual double LB(ProcessModel&) {

		Debugger::err << "ScalarObjective::LB : Lower bound not implemented!!!" << ENDL;

		return Math::MAX_DOUBLE;
	}

	virtual double LB(const ProcessModel&) {

		Debugger::err << "ScalarObjective::LB const : Lower bound not implemented!!!" << ENDL;

		return Math::MAX_DOUBLE;
	}

	virtual ScalarObjective* clone() {
		Debugger::err << "ScalarObjective::clone : Not implemented!!!" << ENDL;
		return NULL;
	}

	virtual QString name() {
		Debugger::err << "ScalarObjective::name : Not implemented!!!" << ENDL;
		return "ScalarObjective";
	}

};

class TWT : public ScalarObjective {
public:

	TWT() {

	}

	TWT(const TWT& other) : ScalarObjective(other) {

	}

	virtual ScalarObjective* clone() {

		//Debugger::info << "Cloning TWT" << ENDL;

		return new TWT(*this);
	}

	virtual ~TWT() {
	}

	double operator()(ProcessModel& pm) {

		QList<ListDigraph::Node> terminals = pm.terminals();

		return (*this)(pm, terminals);
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {

		// Walk through the terminal nodes and calculate the TWT
		double objective = 0.0;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective += pm.ops[terminals[i]]->wT();
		}

		return objective;

	}

	double LB(ProcessModel&) {
		return 0.0;
	}

	double LB(const ProcessModel&) {
		return 0.0;
	}

	virtual QString name() {
		return "TWT";
	}

};

class UTWT : public ScalarObjective { // Unified TWT function which considers the local due dates in view of all terminal nodes
public:

	UTWT() {

	}

	virtual ~UTWT() {

	}

	double operator()(ProcessModel& pm, QList<ListDigraph::Node>& nodes, QMap<ListDigraph::Node, QMap<ListDigraph::Node, double> >& locD) {
		QList<ListDigraph::Node> terminals = pm.terminals();

		// Local objective for TWT minimization
		double utwt = 0.0;
		double maxTk = 0.0;

		// Find the largest tardiness for each terminal node
		for (int k = 0; k < terminals.size(); k++) {

			ListDigraph::Node curTerm = terminals[k];

			maxTk = 0.0;

			for (int i = 0; i < nodes.size(); i++) {

				ListDigraph::Node curNode = nodes[i];

				maxTk = Math::max(maxTk, Math::max(0.0, pm.ops[curNode]->c() - locD[curNode][curTerm]));

			}

			utwt += pm.ops[curTerm]->w() * maxTk;

		}

		return utwt;

	}

};

class TT : public ScalarObjective {
public:

	TT() {
	}

	virtual ~TT() {
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {
		// Walk through the terminal nodes and calculate the TWT
		double objective = 0.0;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective += Math::max(0.0, pm.ops[terminals[i]]->c() - pm.ops[terminals[i]]->d());
		}

		return objective;
	}
};

class SBHTWTLocalObj : public ScalarObjective {
public:

	SBHTWTLocalObj() {

	}

	virtual ~SBHTWTLocalObj() {

	}

	double operator()(ProcessModel& /*pm*/, QList<ListDigraph::Node> &opnodes, TGSelection &tgselection, QList<ListDigraph::Node> & /*terminals*/, QHash<int, QList<double> >& /*dloc*/) {
		// Local objective for TWT minimization
		double localobj = 0.0;
		//double maxTk = 0.0;
		//int szk = terminals.size();
		int szi = opnodes.size();

		/*
		for (int k = 0; k < szk; k++) {
				maxTk = 0.0;
				for (int i = 0; i < szi; i++) {
						maxTk = Math::max(maxTk, Math::max(0.0, pm.ops[opnodes.at(i)]->c() - (dloc[pm.ops[opnodes.at(i)]->ID])[k]));
				}
				localobj += pm.ops[terminals.at(k)]->w() * maxTk;
		}
		 */
		//for (int k = 0; k < szk; k++) {
		//    maxTk = 0.0;
		for (int i = 0; i < szi; i++) {
			//	maxTk = Math::max(maxTk, Math::max(0.0, tgselection.opNode2SchedOps[opnodes.at(i)]->c() - (dloc[tgselection.opNode2SchedOps[opnodes.at(i)]->ID])[k]));
			//    }
			localobj += tgselection.opNode2SchedOps[opnodes.at(i)].w() * Math::max(0.0, tgselection.opNode2SchedOps[opnodes.at(i)].c() - tgselection.opNode2SchedOps[opnodes.at(i)].d()); // pm.ops[terminals.at(k)]->w() * maxTk;
		}

		// Additional subobjective
		/*
		double lmaxobj = 0.0;
		double alpha = 0.95;
		for (int k = 0; k < terminals.size(); k++) {
				maxTk = Math::MIN_DOUBLE;
				for (int i = 0; i < opnodes.size(); i++) {
						maxTk = Math::max(maxTk, pm.ops[opnodes.at(i)]->c() - (dloc[pm.ops[opnodes.at(i)]->ID])[k]);
				}
				lmaxobj += Math::abs(maxTk);
		}

		localobj = alpha * localobj + (1.0 - alpha) * lmaxobj;
		 */

		return localobj;
	}
};

// Total Lj - total lateness of all terminal nodes

class TL : public ScalarObjective {
public:

	TL() {
	}

	virtual ~TL() {
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {
		// Walk through the terminal nodes and calculate the TWT
		double objective = 0.0;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective += (pm.ops[terminals[i]]->c() - pm.ops[terminals[i]]->d());
		}

		return objective;
	}
};

class TAL : public ScalarObjective {
public:

	TAL() {
	}

	virtual ~TAL() {
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {
		// Walk through the terminal nodes and calculate the TWT
		double objective = 0.0;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective += Math::abs(pm.ops[terminals[i]]->c() - pm.ops[terminals[i]]->d());
		}

		return objective;
	}
};

class Lmax : public ScalarObjective {
public:

	Lmax() {
	}

	virtual ~Lmax() {
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {
		// Walk through the terminal nodes and calculate the TWT
		double objective = Math::MIN_DOUBLE;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective = Math::max(objective, pm.ops[terminals[i]]->c() - pm.ops[terminals[i]]->d());
		}

		return objective;
	}
};

class Cmax : public ScalarObjective {
public:

	Cmax() {
	}

	Cmax(const Cmax& other) : ScalarObjective(other) {

	}

	virtual ScalarObjective* clone() {

		//Debugger::info << "Cloning Cmax" << ENDL;

		return new Cmax(*this);
	}

	virtual ~Cmax() {
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {
		// Walk through the terminal nodes and calculate the TWT
		double objective = 0.0;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective = Math::max(objective, pm.ops[terminals[i]]->c());
		}

		return objective;
	}

	double operator()(ProcessModel &pm) {
		// Walk through the terminal nodes and calculate the TWT
		QList<ListDigraph::Node> terminals = pm.terminals();
		return (*this)(pm, terminals);
	}

	double LB(ProcessModel&) {
		return 0.0;
	}

	double LB(const ProcessModel&) {
		return 0.0;
	}

	virtual QString name() {
		return "Cmax";
	}
};

class Csum : public ScalarObjective {
public:

	Csum() {
	}

	virtual ~Csum() {
	}

	double operator()(const ProcessModel &pm, const QList<ListDigraph::Node> &terminals) {
		// Walk through the terminal nodes and calculate the TWT
		double objective = 0.0;
		int sz = terminals.size();

		for (int i = 0; i < sz; i++) {
			objective += pm.ops[terminals[i]]->c();
		}

		return objective;
	}
};

#endif	/* OBJECTIVE_H */

