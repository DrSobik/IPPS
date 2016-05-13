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

SchedSolver* new_LocalSearchPM() {
	return new LocalSearchPM();
}

/******************************************************************************/

SchedSolver* new_LocalSearchPM_par_LocalSearchPM(LocalSearchPM& orig) {
	return new LocalSearchPM(orig);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/