/* 
 * File:   Planner.cpp
 * Author: DrSobik
 * 
 * Created on July 29, 2011, 9:59 AM
 */

#include "Planner.h"

/** ##########################  PlannerStrategy  ############################ */

PlannerStrategy::PlannerStrategy() : strategyString("") {

}

PlannerStrategy::PlannerStrategy(PlannerStrategy& other) : QObject(0) {

	*this = other;

}

PlannerStrategy::~PlannerStrategy() {

}

bool PlannerStrategy::setStrategy(const QString& strStrategy) {

	strategyString = strStrategy;

	bool parsed = parseStrategy(strategyString);

	// Notify that the strategy has changed
	emit sigStrategySchanged(parsed);

	return parsed;
}

QString PlannerStrategy::getStrategy() {
	return strategyString;
}

bool PlannerStrategy::parseStrategy(const QString&) {

	Debugger::err << "PlannerStrategy::parseStrategy : Not implemented!!!" << ENDL;

	return false;
}

PlannerStrategy& PlannerStrategy::operator=(PlannerStrategy& other) {

	setStrategy(other.getStrategy());

	return *this;
}

PlannerStrategy* PlannerStrategy::clone() {
	return new PlannerStrategy(*this);
}

/** ######################################################################### */

/** ##########################  PlanSched  ################################## */

PlanSched::PlanSched() {

}

PlanSched::~PlanSched() {

}

void PlanSched::init() {
	plan.init();
	schedule.init();
}

PlanSched& PlanSched::operator=(PlanSched& other) {
	plan = other.plan;
	schedule = other.schedule;

	return *this;
}

void PlanSched::clearCurrentActions() {
	plan.clearCurrent();
	//schedule.clearCurrent();
}

void PlanSched::clearSavedActions() {
	plan.clearSaved();
	//schedule.clearSaved();
}

void PlanSched::saveActions() {
	plan.save();
	//schedule.save();
}

void PlanSched::restoreActions() {
	plan.restore();
	//schedule.restore();
}

/** ##########################  Planner  #################################### */

Planner::Planner() : pmm(NULL), scheduler(NULL), rc(NULL), objective(NULL) {
}

Planner::~Planner() {

	if (this->objective != NULL) {
		delete this->objective;
	}

}
