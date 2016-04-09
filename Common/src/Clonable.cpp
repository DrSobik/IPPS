/* 
 * File:   Clonable.cpp
 * Author: DrSobik
 * 
 * Created on February 23, 2013, 11:55 PM
 */

#include "Clonable.h"

Clonable::Clonable() {
}

Clonable::Clonable(Clonable&) {
}

Clonable::~Clonable() {
}

Clonable* Clonable::clone() {
    return new Clonable(*this);
}
