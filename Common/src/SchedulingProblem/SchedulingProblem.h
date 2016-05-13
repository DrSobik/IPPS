/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SchedulingProblem.h
 * Author: DrSobik
 *
 * Created on April 12, 2016, 11:10 PM
 */

#ifndef SCHEDULINGPROBLEM_H
#define SCHEDULINGPROBLEM_H

#include "Assignable"

#include "ProcessModel"
#include "Resources"
#include "Objective"

using namespace Common::Interfaces;

class SchedulingProblem : public AssignableFrom<SchedulingProblem> {
private:

public:

    ProcessModel pm;
    Resources rc;
    //ScalarObjective* obj;

    SchedulingProblem() {
    }

    SchedulingProblem(const SchedulingProblem& orig) : AssignableFrom() {
        pm = (ProcessModel&) orig.pm;
        rc = (Resources&) orig.rc;
        //obj = orig.obj;
    }

    virtual ~SchedulingProblem() {
    }

    AssignableFrom& operator=(const SchedulingProblem& orig) override {
        pm = (ProcessModel&) orig.pm;
        rc = (Resources&) orig.rc;
        
        return *this;
    }


};

#endif /* SCHEDULINGPROBLEM_H */

