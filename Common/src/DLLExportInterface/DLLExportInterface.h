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

#include "Objective"

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

/******************************************************************************/
/**********************  Cmax  ************************************************/
/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE ScalarObjective* new_Cmax();

/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE ScalarObjective* new_Cmax_par_Cmax(const Cmax& orig);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/**********************  TWT  *************************************************/
/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE ScalarObjective* new_TWT();

/******************************************************************************/

extern "C" DLL_EXPORT_INTEFACE ScalarObjective* new_TWT_par_TWT(const TWT& orig);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


#endif /* DLLEXPORTINTERFACE_H */

