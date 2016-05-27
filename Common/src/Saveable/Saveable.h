/* 
 * File:   Saveable.h
 * Author: DrSobik
 *
 * Created on February 19, 2013, 11:21 AM
 */

#ifndef SAVEABLE_H
#define	SAVEABLE_H

#include <QtGlobal>

#include "DebugExt.h"

using namespace Common;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE Saveable {
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

