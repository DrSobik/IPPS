/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PlanSched.cpp
 * Author: DrSobik
 * 
 * Created on 8 травня 2016, 22:45
 */

#include "PlanSched.h"

/** ##########################  PlanSched  ################################## */

PlanSched::PlanSched() {

}

PlanSched::~PlanSched() {

}

void PlanSched::init() {
	plan.init();
	schedule.init();
}

PlanSched& PlanSched::operator=(const PlanSched& other) {
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