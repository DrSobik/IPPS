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

#include "IPPSDefinitions"

#include "SchedulingProblem"
#include "Schedule"

#include "../LocalSearchPM/LocalSearchPM.h"

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

/******************************************************************************/
/**********************  LocalSearchPM  ***************************************/
/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE SchedSolver* new_LocalSearchPM();

/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE SchedSolver* new_LocalSearchPM_par_LocalSearchPM(LocalSearchPM& orig);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

#endif /* DLLEXPORTINTERFACE_H */

