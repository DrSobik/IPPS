/* 
 * File:   TGSelection.h
 * Author: DrSobik
 *
 * Created on November 23, 2012, 10:12 AM
 */

#ifndef TGSELECTION_H
#define	TGSELECTION_H

#include <QList>
#include <QPair>

#include "Resources"
#include "ProcessModel"

#include <lemon/list_graph.h>

using namespace lemon;

class TGSelection {
public:
    int tgID; // ID of the tool group
    double localobj; // Value of the local objective corresponding to the selection
    QList<QPair<ListDigraph::Node, ListDigraph::Node> > selection; // Selection of the directed conjunctive arcs which reflect the scheduling decisions
    QMap<ListDigraph::Node, Operation> opNode2SchedOps; // Used for saving the scheduling state of the operations for the current tool group

	QList<QPair<int, int> > selectionOpIDs; // Selection of the directed conjunctive arcs which reflect the scheduling decisions (Operation IDs)
    QMap<int, Operation> opID2SchedOp; // Used for saving the scheduling state of the operations for the current tool group (Operation ID is used as a key value)
	
    TGSelection();
    TGSelection(const TGSelection& other); // IMPORTANT!!! The copy operator must be defined for correctness
    virtual ~TGSelection();

    TGSelection& operator=(const TGSelection &other);

};


#endif	/* TGSELECTION_H */

