/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PlanSched.h
 * Author: DrSobik
 *
 * Created on 8 травня 2016, 22:45
 */

#ifndef PLANSCHED_H
#define PLANSCHED_H

#include "Saveable"

#include "Plan"
#include "Schedule"

/** Combination the some process plan and the corresponding schedule. */
class PlanSched : public Saveable {
protected:

public:

	Plan plan;
	Schedule schedule;

	PlanSched();
	virtual ~PlanSched();

	virtual void init();

	virtual PlanSched& operator=(const PlanSched& other);

	virtual void clearCurrentActions();
	virtual void clearSavedActions();
	virtual void saveActions();
	virtual void restoreActions();

};

#endif /* PLANSCHED_H */

