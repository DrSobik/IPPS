/* 
 * File:   IterativeAlg.cpp
 * Author: DrSobik
 * 
 * Created on February 27, 2012, 5:09 PM
 */

#include <QtCore/qtextstream.h>

#include "IterativeAlg.h"

//#include "RandomExt.h"
#include "RandExt"

IterativeAlg::IterativeAlg() {
	_maxiter = Math::MAX_INTUNI;
	_curiter = 0;

	_maxiterdeclined = Math::MAX_INTUNI;
	_curiterdeclined = 0;

	_maxtimems = Math::MAX_INTUNI;
}

IterativeAlg::IterativeAlg(IterativeAlg& orig) : QObject(0) {
	_maxiter = orig._maxiter;
	_maxiterdeclined = orig._maxiterdeclined;
	_maxtimems = orig._maxtimems;

	IterativeAlg::init();
}

IterativeAlg::~IterativeAlg() {
}

void IterativeAlg::init() {
	//QTextStream out(stdout);

	//out << "IterativeAlg::init : Running in thread : " << this->thread() << endl;

	_curiter = 0;

	_curiterdeclined = 0;

	// Start the internal timer
	_curtime.setHMS(0, 0, 0, 0);
	_curtime.start();
}

void IterativeAlg::run() {

	//QTextStream out(stdout);

	IterativeAlg::init();

	// Perform preprocessing actions
	preprocess();

	// Run the algorithm
	
	// DEBUG
	
	//double y = -1.0;
	//unsigned int m_w = 12345;
	//unsigned int m_z = 184553;
	//qsrand(1);

	//QList<quint32> seeds;
	//seeds << time(0);
	//RandGenLCGNL rg(seeds);
	//RandGenMT rg(time(0));
	//rg.setParent(0);
	//rg.moveToThread(this->thread());
	
	// END DEBUG
	

	while (!stop()) {

		// DEBUG
		//double x = 0.0;//23462876348762.0;
		//for (unsigned long int i = 0; i < 50000000; i++) {
		//x += Math::sqrt(x);
		//x = Math::rndDouble(); // 1 thread - 3.5s, 2 threads -  5 - 5.5 s
		//x*= 0.987654321; no difference between 1 and 2 threads
		//x = srand(); // Significant difference
		//x = Math::sqrt(x) + 1.0; // No significant difference



		//m_z = 36969*(m_z & 65535) + (m_z >> 16); // No difference
		//m_w = 18000*(m_w & 65535) + (m_w >> 16);
		//x = (m_z << 16) + m_w;

		//x = rg.rndInt(0,65000);
		//	x+= 0.00001* rg.rndFloat();

		//out << "i = " << i << endl;
		//out << "x = " << x << endl;
		//}

		//y = x;
		//out << "y = " << y << endl;

		//getchar();

		//_curiter = 1000000000;
		// END DEBUG




		// Perform one step of the algorithm. This will change the states of some products.
		step();

		// Assess the results of the step.
		assess();

		// Decide whether the changes will be accepted or declined
		acceptOrDecline();

	}

	// DEBUG
	//out << y << endl;
	// END DEBUG

	// Preform postprocessing actions
	postprocess();
}

EventIterativeAlg::EventIterativeAlg() {
	_maxiter = Math::MAX_INTUNI;
	_curiter = 0;

	_maxiterdeclined = Math::MAX_INTUNI;
	_curiterdeclined = 0;

	_maxtimems = Math::MAX_INTUNI;

	// Connect signals and slots
	connect(this, SIGNAL(sigInit()), this, SLOT(slotInit()));
	connect(this, SIGNAL(sigInitFinished()), this, SLOT(slotInitFinished()));

	connect(this, SIGNAL(sigStep()), this, SLOT(slotStep()));
	connect(this, SIGNAL(sigStepFinished()), this, SLOT(slotStepFinished()));

	connect(this, SIGNAL(sigAssess()), this, SLOT(slotAssess()));
	connect(this, SIGNAL(sigAssessFinished()), this, SLOT(slotAssessFinished()));

	connect(this, SIGNAL(sigAcceptConditionFinished(const bool&)), this, SLOT(slotAcceptConditionFinished(const bool&)));
	connect(this, SIGNAL(sigAcceptOrDecline()), this, SLOT(slotAcceptOrDecline()));
	connect(this, SIGNAL(sigAcceptOrDeclineFinished()), this, SLOT(slotAcceptOrDeclineFinished()));
	connect(this, SIGNAL(sigAccept()), this, SLOT(slotAccept()));
	connect(this, SIGNAL(sigDecline()), this, SLOT(slotDecline()));
	connect(this, SIGNAL(sigAcceptFinished()), this, SLOT(slotAcceptFinished()));
	connect(this, SIGNAL(sigDeclineFinished()), this, SLOT(slotDeclineFinished()));

	connect(this, SIGNAL(sigFinished()), this, SLOT(slotFinished()));
}

EventIterativeAlg::~EventIterativeAlg() {
}

void EventIterativeAlg::run() {

	//QTextStream out(stdout);

	IterativeAlg::init();

	// Initialize
	emit sigInit();

}

void EventIterativeAlg::slotInit() {
	// Call the initializer of the algorithm
	init();
}

void EventIterativeAlg::slotInitFinished() {
	//Debugger::info << "EventIterativeAlg::slotInitFinished : Initialization finished!" << ENDL;
	//getchar();

	// Perform preprocessing actions
	preprocess();

	// Run the algorithm
	if (!stop()) {

		emit sigStep();

	} else {
		// Postprocess
		postprocess();

		emit sigFinished();
	}
}

void EventIterativeAlg::slotStep() {
	// Update the iteration counter
	_curiter++;

	// Perform one step of the algorithm
	stepActions();
}

void EventIterativeAlg::slotStepFinished() {
	// Update the iteration counter
	//_curiter++;

	// Emit signal for assess actions
	emit sigAssess();
}

void EventIterativeAlg::slotAssess() {
	// Perform assess actions
	assessActions();
}

void EventIterativeAlg::slotAssessFinished() {

	emit sigAcceptOrDecline();
}

void EventIterativeAlg::slotAcceptOrDecline() {
	acceptCondition();
}

void EventIterativeAlg::slotAcceptConditionFinished(const bool& des) {
	if (des) {
		// Set the counter of the sequentially declined solutions to 0
		_curiterdeclined = 0;

		emit sigAccept();

	} else {
		// Increase the counter of the sequentially declined solutions
		_curiterdeclined++;

		emit sigDecline();
	}
}

void EventIterativeAlg::slotAccept() {
	acceptActions();
}

void EventIterativeAlg::slotDecline() {
	declineActions();
}

void EventIterativeAlg::slotAcceptFinished() {
	emit sigAcceptOrDeclineFinished();
}

void EventIterativeAlg::slotDeclineFinished() {
	emit sigAcceptOrDeclineFinished();
}

void EventIterativeAlg::slotAcceptOrDeclineFinished() {
	// Notify that the step has finished
	emit sigCompleteStepFinished();

	// If the stopping criteria not met then perform one more step of the algorithm
	if (!stop()) {
		// Perform one more step
		emit sigStep();
	} else {
		// Perform postprocessing
		postprocess();

		emit sigFinished();
	}
}

void EventIterativeAlg::slotFinished() {

}