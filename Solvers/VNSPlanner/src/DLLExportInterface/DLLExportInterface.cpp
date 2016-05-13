/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DLLExportInterface.cpp
 * Author: DrSobik
 * 
 * Created on April 11, 2016, 9:36 PM
 */

#include "DLLExportInterface.h"

/******************************************************************************/
/**********************  LocalSearchPM  ***************************************/

/******************************************************************************/

PlanSchedSolver* new_VNSPlanner() {
	return new VNSPlanner();
}

/******************************************************************************/

PlanSchedSolver* new_VNSPlanner_par_VNSPlanner(VNSPlanner& orig) {
	return new VNSPlanner(orig);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/**********************  VNSPlannerStrategy  **********************************/
/******************************************************************************/

VNSPlannerStrategy* new_VNSPlannerStrategy(){
	return new VNSPlannerStrategy();
}

/******************************************************************************/

VNSPlannerStrategy* new_VNSPlannerStrategy_par_VNSPlannerStrategy(VNSPlannerStrategy& orig){
	return new VNSPlannerStrategy(orig);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/