/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   IPPSDefinitions.h
 * Author: DrSobik
 *
 * Created on 30 квітня 2016, 15:04
 */

#ifndef IPPSDEFINITIONS_H
#define IPPSDEFINITIONS_H

#include "Solver"
#include "Parser"
#include "Clonable"
#include "Variables"

#include "SchedulingProblem"
#include "Schedule"
#include "Plan"
#include "PlanSched"

#include "IPPSProblem"

using namespace Common;
using namespace Common::Interfaces;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

/******************************************************************************/
class Setting : public Variable<QString> {
private:

    bool isChanged;

    QString val;

protected:

public:

    Setting() : isChanged(false) {
    }

    Setting(const Setting& other) : Variable() {
	*this = other;
    }

    virtual ~Setting() {
    }

    virtual Setting& set(const QString& val) override {
	//qDebug("Setter");
	if (this->val == val) { // Leave as is

	} else {
	    this->val = val; // Changing value leads to status "changed"
	    setChanged(true);
	}

	return *this;

    }

    virtual QString& get() override {
	//qDebug("Getter");
	return val;
    }

    virtual const QString& get() const override {
	//qDebug("Getter const");
	return val;
    }

    using Variable<QString>::operator=;

    virtual Setting& operator=(const Setting & other) {
	set(other.get()); // Will set isChanged by default
	return *this;
    }

    void setChanged(const bool& ch = true) override {
	isChanged = ch;
    }

    bool changed() const override {
	return isChanged;
    }


};

/******************************************************************************/

/******************************************************************************/
class Settings : public Variables<QHash, QString, Setting> {
private:

public:

    Settings() {
    }

    Settings(const Settings& other) : Variables() {
	*this = other;
    }

    virtual ~Settings() {
    }

    using Variables::operator[];

    const Setting& operator[](const QString& key) const override {
	return container().constFind(key).value();
    }

    virtual Settings& operator=(const Settings& other) {

	// Remove the already existing entries if they do not exist in other.vars
	QList<QString> keyRem;
	for (Variables<QHash, QString, Setting>::ContainerType::const_iterator iter = vars.begin(); iter != vars.end(); ++iter) {
	    if (!other.vars.contains(iter.key())) {
		keyRem << iter.key();
	    }
	}
	for (auto curKey : keyRem) {
	    vars.remove(curKey);
	}

	// Set the other settings
	for (Variables<QHash, QString, Setting>::ContainerType::const_iterator iter = other.vars.begin(); iter != other.vars.end(); ++iter) {
	    vars[iter.key()] = iter.value();
	}

	return *this;
    }

    void setChanged(const bool& ch = true) override {
	for (Variables<QHash, QString, Setting>::ContainerType::const_iterator iter = vars.begin(); iter != vars.end(); ++iter) {
	    vars[iter.key()].setChanged(ch);
	}
    }

    bool changed() const override {
	for (const QString& curKey : container().keys()) {
	    if ((*this)[curKey].changed()) {
		return true;
	    }
	}

	return false;
    }

    void accept() override {
	for (const QString& curKey : container().keys()) {
	    (*this)[curKey].accept();
	}
    }

    void clear() override {
	vars.clear();
    }


};
/******************************************************************************/

/** A class with general scheduling options */

/*
class SchedulerOptions : public QHash<QString, QString> {
public:

    SchedulerOptions() {

    }

    SchedulerOptions(SchedulerOptions& orig) : QHash<QString, QString> (orig) {

    }

    SchedulerOptions(const SchedulerOptions& orig) : QHash<QString, QString> (orig) {

    }

    virtual ~SchedulerOptions() {

    }

    friend QTextStream& operator<<(QTextStream& out, SchedulerOptions& options) {
	for (SchedulerOptions::iterator iter = options.begin(); iter != options.end(); iter++) {
	    out << iter.key() << "=" << iter.value() << endl;
	}
	return out;
    }

};
 */

typedef Settings SchedulerOptions;
typedef Settings PlannerOptions;

/* SchedSolver */
class DLL_EXPORT_INTEFACE SchedSolver : public Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&>, public Solver<Schedule, const SchedulingProblem&>, public Parser<void, const SchedulerOptions&>, ClonableTo<SchedSolver> {
private:

    SchedSolver(const SchedSolver&) : Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&>(), Solver<Schedule, const SchedulingProblem&>(), Parser(), ClonableTo() {
    }

protected:

    SchedSolver() {
    }

public:

    ~SchedSolver() {
    }

    //using Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&>::solve;
    //using Parser<void, SchedulerOptions>::parse;

    virtual void parse(const SchedulerOptions&) override = 0;

    virtual Schedule solve(const SchedulingProblem&) override = 0;

    virtual Schedule solve(const SchedulingProblem& problem, const SchedulerOptions& settings) override {

	// Try to parse the settings
	try {

	    parse(settings);

	} catch (ErrMsgException<>& ex) {

	    //out << QString::fromStdString(ex.getMsg().getMsgData()) << endl;
	    throw ex;

	}

	// Solve the problem
	return solve(problem);

    }



    virtual SchedSolver* clone() override = 0;

};

/* PlanSchedSolver */
class DLL_EXPORT_INTEFACE PlanSchedSolver : public Solver<PlanSched, const IPPSProblem&, const PlannerOptions&>, public Solver<PlanSched, const IPPSProblem&>, public Parser<void, const PlannerOptions&>, ClonableTo<PlanSchedSolver> {
private:

    PlanSchedSolver(const PlanSchedSolver&) : Solver<PlanSched, const IPPSProblem&, const PlannerOptions&>(), Solver<PlanSched, const IPPSProblem&>(), Parser(), ClonableTo() {
    }

protected:

    PlanSchedSolver() {
    }

public:

    ~PlanSchedSolver() {
    }

    //using Solver<Schedule, const SchedulingProblem&, const SchedulerOptions&>::solve;
    //using Parser<void, SchedulerOptions>::parse;

    virtual void parse(const PlannerOptions&) override = 0;

    virtual PlanSched solve(const IPPSProblem&) override = 0;

    virtual PlanSched solve(const IPPSProblem& problem, const PlannerOptions& settings) override {

	// Try to parse the settings
	try {

	    parse(settings);

	} catch (ErrMsgException<>& ex) {

	    //out << QString::fromStdString(ex.getMsg().getMsgData()) << endl;
	    throw ex;

	}

	// Solve the problem
	return solve(problem);

    }



    virtual PlanSchedSolver* clone() override = 0;

};

#endif /* IPPSDEFINITIONS_H */

