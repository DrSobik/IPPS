/* 
 * File:   DispatchScheduler.cpp
 * Author: DrSobik
 * 
 * Created on July 1, 2011, 3:39 PM
 */

#include "DispatchScheduler.h"

DispatchScheduler::DispatchScheduler() {
}

DispatchScheduler::~DispatchScheduler() {
}

bool DispatchScheduler::schedule(ProcessModel &, Resources &, Schedule &) {
	Debugger::info << "Building schedule using dispatching rules" << ENDL;
	return false;
}