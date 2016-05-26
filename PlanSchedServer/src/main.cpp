/*
 * File:   main.cpp
 * Author: DrSobik
 *
 * Created on August 2, 2013, 12:50 PM
 */

#include <QCoreApplication>
#include <QtCore>
#include <QtGlobal>

#include "MathExt"
#include "RandExt"

#include "PlanSchedServer/PlanSchedServer.h"

int main(int argc, char *argv[]) {
    QTextStream out(stdout);
    QCoreApplication app(argc, argv);
    PlanSchedServer server;
    server.setObjectName("PlanSchedServer");
    Settings settings;


    // Parse the arguments
    QStringList arguments = app.arguments();

    QHash<QString, QString> prefix2arg;
    for (int i = 1; i < arguments.size(); i++) { // The first one is the program's directory
	//out << arguments[i] << endl;
	QStringList argssplitted = arguments[i].split(":");
	if (argssplitted.size() < 2) {
	    Debugger::err << "Arguments not specified correctly!!!" << ENDL;
	} else {

	    for (int j = 1; j < argssplitted.size() - 1; j++) {
		prefix2arg[argssplitted[0]] += argssplitted[j] + ":";
	    }
	    prefix2arg[argssplitted[0]] += argssplitted.last();
	}
    }

    if (!prefix2arg.contains("RandSeed")) {

	Debugger::err << "No seed for randomization specified!" << ENDL;

    } else {

	settings["PLANSCHEDSERVER_RANDSEED"] = prefix2arg["RandSeed"];

    }

    if (!prefix2arg.contains("PlanSchedSolver")) {

	Debugger::err << "No PlanSchedSolver specified!" << ENDL;

    } else {

	settings["PLANSCHEDSERVER_SOLVER"] = prefix2arg["PlanSchedSolver"];

    }

    // Parse the settings
    server.parse(settings);

    out << "Starting server ... " << endl;

    QHostAddress hostAddress = QHostAddress::LocalHost;
    qint32 hostPort = 5555;

    //hostAddress.setAddress("132.176.74.60"); // IP of LABOR 1
    //host.setAddress("132.176.74.101"); // IP of this computer at the university (static)
    //host.setAddress("192.168.1.101"); // Local IP at home
    //hostAddress.setAddress("192.168.1.100"); // Local IP at home

    if (server.listen(hostAddress, hostPort)) {

	out << "Started " << server.objectName() << endl;

    } else {

	out << "Failed to start the server!" << endl;
	app.exit();

    }

    return app.exec();

}
