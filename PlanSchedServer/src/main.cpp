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

    // PLANSCHEDSERVER_RANDSEED
    if (!prefix2arg.contains("RandSeed")) {

	Debugger::err << "No seed for randomization specified!" << ENDL;

    } else {

	settings["PLANSCHEDSERVER_RANDSEED"] = prefix2arg["RandSeed"];

    }

    // PLANSCHEDSERVER_SOLVER
    if (!prefix2arg.contains("PlanSchedSolver")) {

	Debugger::err << "No PlanSchedSolver specified!" << ENDL;

    } else {

	settings["PLANSCHEDSERVER_SOLVER"] = prefix2arg["PlanSchedSolver"];

    }

    // PLANSCHEDSERVER_HOST
    if (!prefix2arg.contains("Host")) {

	Debugger::err << "No Host specified!" << ENDL;

    } else {

	settings["PLANSCHEDSERVER_HOST"] = prefix2arg["Host"];

    }
    
    // PLANSCHEDSERVER_HOST
    if (!prefix2arg.contains("Port")) {

	Debugger::err << "No Port specified!" << ENDL;

    } else {

	settings["PLANSCHEDSERVER_PORT"] = prefix2arg["Port"];

    }
    
    // Parse the settings (will also parse host and port + try to start listening)
    server.parse(settings);


    return app.exec();

}
