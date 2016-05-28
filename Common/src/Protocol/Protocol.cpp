/* 
 * File:   Protocol.cpp
 * Author: DrSobik
 * 
 * Created on February 4, 2013, 10:13 AM
 */

#include <QtXml/qdom.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcoreapplication.h>

#include "Protocol.h"

Protocol::Protocol() {
	init();
}

Protocol::~Protocol() {
}

void Protocol::init() {
	this->clear();

	QDomElement protoel = this->createElement("proto");
	protoel.setAttribute("timestamp", QTime::currentTime().toString());

	this->appendChild(protoel);

	//QTextStream out(stdout);
	//out << "Current protocol: " << endl;
	//out << *this << endl;
	//getchar();

	file = NULL;
}

void Protocol::write(QTextStream &stream) {
	stream << this->toByteArray() << endl;
}

void Protocol::setFile(QFile &file) {
	this->file = &file;
}

QFile& Protocol::getFile() {
	return *file;
}

void Protocol::save() {
	
	if (file == NULL){
		Debugger::err << "Protocol::save : File is NULL " << ENDL;
		return;
	}
	
	// Set/create path where the protocol file should be stored
    QString curFilePath = file->fileName();
	QDir curDir = QFileInfo(curFilePath).absoluteDir();
	if (!curDir.exists()){
	    curDir.mkpath(curDir.path());
	}
		
	if (!file->open(QIODevice::WriteOnly)) {
        Debugger::err << "Protocol::save : Failed to open path " << curDir.path().toStdString() << " and file " << file->fileName().toStdString() << " for writing!" << ENDL;
		return;
	}else{
		//Debugger::info << "Protocol::save : Opened file " << file->fileName().toStdString() << " for writing!" << ENDL;
	}

	//QTextStream stream(file);

	file->write(this->toByteArray());

	file->close();

}

Protocol& Protocol::operator<<(QDomDocument &protodoc) {
	this->elementsByTagName("proto").at(0).appendChild(this->importNode(protodoc.documentElement(), true));

	return *this;
}
