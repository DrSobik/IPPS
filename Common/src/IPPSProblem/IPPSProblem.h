/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   IPPSProblem.h
 * Author: DrSobik
 *
 * Created on April 15, 2016, 4:01 AM
 */

#ifndef IPPSPROBLEM_H
#define IPPSPROBLEM_H

#include "SmartPointer"

#include "Objective"
#include "ProcessModel"
#include "ProcessModelManager"
#include "Resources"

using namespace Common;
using namespace Common::SmartPointers;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE IPPSProblem {
private:	
	
public:
	
	Resources* rc;
	ProcessModelManager* pmm;
	//SmartPointer<ScalarObjective> obj;
	
	
	IPPSProblem();
	IPPSProblem(const IPPSProblem& orig);
	virtual ~IPPSProblem();

};

#endif /* IPPSPROBLEM_H */

