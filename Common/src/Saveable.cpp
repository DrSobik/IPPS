/* 
 * File:   Saveable.cpp
 * Author: DrSobik
 * 
 * Created on February 19, 2013, 11:21 AM
 */

#include "Saveable.h"

Saveable::Saveable() {
    saved = false;
}

Saveable::~Saveable() {
}

void Saveable::clear() {
    clearSaved();
    clearCurrent();
}

void Saveable::clearCurrent() {
    clearCurrentActions();
}

void Saveable::clearSaved() {
    if (saved) {
	clearSavedActions();
    } else {
	// Debugger::warn << "Saveable::clearSaved : Clearing possibly empty object!" << ENDL;
    }

    saved = false;
}

void Saveable::save() {
    saveActions();
    saved = true;
}

void Saveable::restore() {
    if (saved) {
	restoreActions();
    } else {
	Debugger::warn << "Saveable::restore : Restoring possibly empty object!" << ENDL;
    }
}

void Saveable::clearCurrentActions() {
    Debugger::err << "Saveable::clearCurrentActions : Not implemented!" << ENDL;
}

void Saveable::clearSavedActions() {
    Debugger::err << "Saveable::clearSavedActions : Not implemented!" << ENDL;
}

void Saveable::saveActions() {
    Debugger::err << "Saveable::saveActions : Not implemented!" << ENDL;
}

void Saveable::restoreActions() {
    Debugger::err << "Saveable::restoreActions : Not implemented!" << ENDL;
}
