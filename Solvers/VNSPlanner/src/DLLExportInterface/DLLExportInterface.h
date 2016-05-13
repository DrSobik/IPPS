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

#include "Solver"

#include "IPPSProblem"

#include "../VNSPlanner/VNSPlanner.h"

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

/******************************************************************************/
/**********************  VNSPlanner  ******************************************/
/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE PlanSchedSolver* new_VNSPlanner();

/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE PlanSchedSolver* new_VNSPlanner_par_VNSPlanner(VNSPlanner& orig);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/**********************  VNSPlannerStrategy  **********************************/
/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE VNSPlannerStrategy* new_VNSPlannerStrategy();

/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE VNSPlannerStrategy* new_VNSPlannerStrategy_par_VNSPlannerStrategy(VNSPlannerStrategy& orig);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


#endif /* DLLEXPORTINTERFACE_H */

