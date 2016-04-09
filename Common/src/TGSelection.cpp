/* 
 * File:   TGSelection.cpp
 * Author: DrSobik
 * 
 * Created on November 23, 2012, 10:12 AM
 */

#include "TGSelection.h"

/* -------------------  Tool group selection  ------------------------------- */

TGSelection::TGSelection() {
	tgID = -1;
}

TGSelection::TGSelection(const TGSelection& other) {
	*this = other;
}

TGSelection::~TGSelection() {

}

TGSelection& TGSelection::operator=(const TGSelection &other) {
	QTextStream out(stdout);
	//Debugger::info << "Copying ..." << ENDL;

	tgID = other.tgID;

	localobj = other.localobj;
	selection = other.selection;

	selectionOpIDs = other.selectionOpIDs;
	
	// Copy the operations
	//out << "Deleting old operations..." << endl;
	//for (QMap<ListDigraph::Node, Operation>::iterator iter = opNode2SchedOps.begin(); iter != opNode2SchedOps.end(); iter++) {

	//out << "Operation ..." << endl;
	//out << *iter.value() << endl;
	//out << "Operation." << endl;
	//out << "Deleting ..." << endl;
	//delete iter.value();
	//out << "Deleted." << endl;
	//}
	//out << "Deleted old operations." << endl;

	opNode2SchedOps.clear();

	for (QMap<ListDigraph::Node, Operation>::const_iterator iter = other.opNode2SchedOps.begin(); iter != other.opNode2SchedOps.end(); iter++) {
		//out << "Copying operation: " << iter.value()->ID << endl;
		opNode2SchedOps[iter.key()] = Operation(iter.value());
		//getchar();
	}

	opID2SchedOp = other.opID2SchedOp;
	
	
	//Debugger::info << "Copied." << ENDL;

	return *this;
}

/* -------------------------------------------------------------------------- */