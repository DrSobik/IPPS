/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   newmain.cpp
 * Author: DrSobik
 *
 * Created on April 8, 2016, 12:12 AM
 */

#include <QApplication>

#include "LocalSearchPM/LocalSearchPM.h"

int main(int argc, char *argv[]) {
	// initialize resources, if needed
	// Q_INIT_RESOURCE(resfile);

	QApplication app(argc, argv);

	// create and show your widgets here

	LocalSearchPM lspm;

	return app.exec();
}
