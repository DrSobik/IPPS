/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DLLExportInterface.h
 * Author: DrSobik
 *
 * Created on April 11, 2016, 9:36 PM
 */

#ifndef DLLEXPORTINTERFACE_H
#define DLLEXPORTINTERFACE_H

#include <QtGlobal>

#include "IPPSDefinitions"

#include "SchedulingProblem"
#include "Schedule"

#include "../CombinedScheduler/CombinedScheduler.h"

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

/******************************************************************************/
/**********************  CombinedScheduler  ***************************************/
/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE SchedSolver* new_CombinedScheduler();

/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE SchedSolver* new_CombinedScheduler_par_CombinedScheduler(CombinedScheduler& orig);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

///******************************************************************************/
///**********************  CombinedSchedulerLS  ***************************************/
///******************************************************************************/
//
//extern "C" DLL_EXPORT_INTEFACE SchedSolver* new_CombinedSchedulerLS();
//
///******************************************************************************/
//
//extern "C" DLL_EXPORT_INTEFACE SchedSolver* new_CombinedSchedulerLS_par_CombinedSchedulerLS(CombinedSchedulerLS& orig);
//
///******************************************************************************/
///******************************************************************************/
///******************************************************************************/

#endif /* DLLEXPORTINTERFACE_H */

