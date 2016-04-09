/* 
 * File:   Saveable.h
 * Author: DrSobik
 *
 * Created on February 19, 2013, 11:21 AM
 */

#ifndef SAVEABLE_H
#define	SAVEABLE_H

#include "DebugExt.h"

using namespace Common;

class Saveable {
protected:
    bool saved;

public:

    Saveable();
    virtual ~Saveable();

    void clear();
    void clearCurrent();
    void clearSaved();

    void save();
    void restore();

    virtual void clearCurrentActions();
    virtual void clearSavedActions();
    virtual void saveActions();
    virtual void restoreActions();
};

#endif	/* SAVEABLE_H */

