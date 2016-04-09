/*
 * File:   main.cpp
 * Author: DrSobik
 *
 * Created on August 2, 2013, 12:50 PM
 */

#include <QApplication>
#include <QtCore>
#include <QtGlobal>

#include "PlanSchedServer/PlanSchedServer.h"

int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

	QTextStream out(stdout);

	PlanSchedServer server;
	server.setObjectName("PlanSchedServer");
	
	out << "Starting server ... " << endl;
	
	QHostAddress hostAddress = QHostAddress::LocalHost;
	qint32 hostPort = 5555;
	
	//host.setAddress("132.176.74.101"); // IP of this computer at the university (static)
	//host.setAddress("192.168.1.101"); // Local IP at home
	
	if (server.listen(hostAddress, hostPort)){
		out << "Started " << server.objectName() << endl;
	}else{
		out << "Failed to start the server!" << endl;
		app.exit();
	}

	return app.exec();
}
