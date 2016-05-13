/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FlowFactor.h
 * Author: DrSobik
 *
 * Created on 5 травня 2016, 21:02
 */

#ifndef FLOWFACTOR_H
#define FLOWFACTOR_H

#include <QtCore>

#include "Functor"

#include "ProcessModel"

using namespace Common;
using namespace Common::Interfaces;

class FlowFactor : public Functor<double, ProcessModel> {
private:
    
protected:

public:
    
    FlowFactor();
    FlowFactor(const FlowFactor& orig);
    virtual ~FlowFactor();

    double operator()(const ProcessModel&) override;
    
};

#endif /* FLOWFACTOR_H */

