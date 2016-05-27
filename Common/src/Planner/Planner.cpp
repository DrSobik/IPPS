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

PlannerStrategy::PlannerStrategy(const PlannerStrategy& other)/* : QObject(0)*/ {

	*this = other;

}

PlannerStrategy::~PlannerStrategy() {

}

bool PlannerStrategy::setStrategy(const QString& strStrategy) {

	strategyString = strStrategy;

	bool parsed = parseStrategy(strategyString);

	// Notify that the strategy has changed
//	emit sigStrategySchanged(parsed);

	return parsed;
}

QString PlannerStrategy::getStrategy() const {
	return strategyString;
}

bool PlannerStrategy::parseStrategy(const QString&) {

	Debugger::err << "PlannerStrategy::parseStrategy : Not implemented!!!" << ENDL;

	return false;
}

PlannerStrategy& PlannerStrategy::operator=(const PlannerStrategy& other) {

	setStrategy(other.getStrategy());

	return *this;
}

PlannerStrategy* PlannerStrategy::clone() {
	return new PlannerStrategy(*this);
}

/** ######################################################################### */


/** ##########################  Planner  #################################### */

Planner::Planner() : pmm(NULL), scheduler(NULL), rc(NULL), objective(NULL) {
}

Planner::~Planner() {

	if (this->objective != NULL) {
		delete this->objective;
	}

}
