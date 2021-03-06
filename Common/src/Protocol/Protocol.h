/* 
 * File:   Protocol.h
 * Author: DrSobik
 *
 * Created on February 4, 2013, 10:13 AM
 */

#ifndef PROTOCOL_H
#define	PROTOCOL_H

#include "DebugExt.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>

using namespace Common;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE Protocol : public QDomDocument {
private:
    QFile *file;

public:
    Protocol();
    virtual ~Protocol();

    /** Initialize the protocol. */
    virtual void init();

    /** Write the protocol to the stream. */
    virtual void write(QTextStream &stream);

    /** Set file associated with the protocol. */
    virtual void setFile(QFile &file);

    /** Get the associated file */
    virtual QFile& getFile();

    /** Save the protocol to the specified file. */
    virtual void save();

    /** Write some DOM element to the protocol. */
    virtual Protocol& operator <<(QDomDocument &protodoc);

private:

};

#endif	/* PROTOCOL_H */

