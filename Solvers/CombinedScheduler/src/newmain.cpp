/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   newmain.cpp
 * Author: DrSobik
 *
 * Created on April 8, 2016, 8:22 PM
 */

#include <QCoreApplication>

#include "CombinedScheduler/CombinedScheduler.h"

int main(int argc, char *argv[]) {
	// initialize resources, if needed
	// Q_INIT_RESOURCE(resfile);

    QCoreApplication app(argc, argv);

	// create and show your widgets here

	CombinedScheduler csls;
	
	return app.exec();
}
