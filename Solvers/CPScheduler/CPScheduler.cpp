/* 
 * File:   CPScheduler.cpp
 * Author: DrSobik
 * 
 * Created on March 28, 2012, 10:06 PM
 */

#include "CPScheduler.h"

CPScheduler::CPScheduler() {
}

CPScheduler::~CPScheduler() {
}

void CPScheduler::schedule(ProcessModel &pm, Resources &rc, QSet<int> &M0, QHash<int, TGSelection> &toolid2selection) {
	// Set the input data
	this->pm = &pm;
	this->rc = &rc;
	this->M0 = &M0;
	this->toolid2selection = &toolid2selection;

	// Initialize the algorithm
	init();

	// Run the scheduling algorithm
	run();
}

void CPScheduler::init() {
	// Get the list of terminals
	for (ListDigraph::InArcIt iait(pm->graph, pm->tail); iait != INVALID; ++iait) {
		terminals.append(pm->graph.source(iait));
	}

}

void CPScheduler::preprocessingActions() {

}

void CPScheduler::postprocessingActions() {

}

void CPScheduler::stepActions() {

}

void CPScheduler::assessActions() {

}

bool CPScheduler::acceptCondition() {
    return false;
}

void CPScheduler::acceptActions() {

}

void CPScheduler::declineActions() {

}

bool CPScheduler::stopCondition() {
    return false;
}

void CPScheduler::stopActions() {

}
