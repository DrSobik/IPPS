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
/**********************  CombinedScheduler  ***************************************/
/******************************************************************************/

SchedSolver* new_CombinedScheduler(){
    return new CombinedScheduler;
}

/******************************************************************************/

SchedSolver* new_CombinedScheduler_par_CombinedScheduler(CombinedScheduler& orig){
    return new CombinedScheduler(orig);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

///******************************************************************************/
///**********************  CombinedSchedulerLS  ***************************************/
///******************************************************************************/
//
//SchedSolver* new_CombinedSchedulerLS(){
//    return new CombinedSchedulerLS;
//}
//
///******************************************************************************/
//
//SchedSolver* new_CombinedSchedulerLS_par_CombinedSchedulerLS(CombinedSchedulerLS& orig){
//    return new CombinedSchedulerLS(orig);
//}
//
///******************************************************************************/
///******************************************************************************/
///******************************************************************************/