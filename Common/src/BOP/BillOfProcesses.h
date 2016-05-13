/* 
 * File:   BillOfProcesses.h
 * Author: DrSobik
 *
 * Created on July 29, 2011, 3:37 PM
 * 
 * Description : Class BillOfProcesses contains information about the
 *				 BOM and the Routes for item types. It describes operations of 
 *				 which type and in which sequence must be executed to produce 
 *				 one entity of the desired product. It is generated based on 
 *				 the given BOM and the given route for each operation type.
 * 
 * Contained data:
 * 
 *				ID		-	ID of the BOP. Important!!! It is formed by setting 
 *							the BOM ID into the first four bytes and the 
 *							route ID into the latter four bytes of this ID. Thus,
 *							it can later be easily decoded.
 *							
 *				
 *				graph	-	graph for representing the BOP, i.e. the nodes
 *							are associated with operation types and the arcs are
 *							represent the precedence constraints.
 * 
 *				head	-	head or entry point of the graph (fake).
 *	
 *				tail	-	terminal node of the graph (fake).
 * 
 *				otypeID	-	mapping of the set of operation types onto the 
 *							nodes of the graph.
 * 
 */

#ifndef BILLOFPROCESSES_H
#define	BILLOFPROCESSES_H

#include <iostream>

#include <lemon/list_graph.h>
#include <lemon/connectivity.h>

#include "BillOfMaterials"
#include "Route"
#include "Resources"

#include "DebugExt.h"
#include "MathExt"

#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QVector>
#include <QTextStream>

using namespace lemon;
using namespace Common;

class BillOfProcesses {
private:

	ListDigraph _graph;

	ListDigraph::Node _head;
	ListDigraph::Node _tail;

	ListDigraph::NodeMap<int> _otypeID; // Types of operations corresponding to each node of the graph
	ListDigraph::NodeMap<int> _oLocItemID; // Local item IDs (within the BOM) which these operations belong to

	BillOfMaterials* _bom; // BOM based on which the BOP can be generated

	QMap<ListDigraph::Node, QList<ListDigraph::Node> > _bomNode2BOPNodes; // 
	QMap<ListDigraph::Node, ListDigraph::Node> _bopNode2BOMNode; // For each node in the BOP the corresponding node in the BOM is set

	QHash<int, QList<Route*> > _iroutes; // Item type routes based on which the BOP can be generated

	// QHash<int, int> _itype2routeidx; // Current route indices for different item types
	QHash<int, int> _itemID2routeidx; // Current route indices for the items ( NOT ITEM TYPES !!! Two items of the same type can have different routes.)

	static int bopsCreated;
	static int bopsDeleted;

public:

	int ID;


	BillOfProcesses();
	virtual ~BillOfProcesses();

	/** Set the data based on which the BOP can be generated. */
	void setData(BillOfMaterials *bom, QHash<int, QList<Route*> > &routes);

	/** Generate the BOP based on the given BOM and the routes. */
	void generate();

	/** Degenerate the BOP. Used for memory optimization. */
	void degenerate();

	/** Generate a list of all possible BOPs for the given list of BOMs of 
	 *  some product and for the given list of alternative routes for different
	 *  item types. */
	static void generateAll(QList<BillOfMaterials*> &boms, QHash<int, QList<Route*> > &iroutes, QList<BillOfProcesses*> &bops);

	/** For all given items (not types!) select one of the appropriate routes randomly. */
	void setItemRouteIdxRnd();

	/** Set some specific item route indices. */
	void setItemRouteIdx(const QHash<int, int> &routeidcs);

	/** Routes of all item types. */
	const QHash<int, QList<Route*> >& itemTypeRoutes();

	const QHash<int, QList<Route*> > itemRoutes();

	/** Routes of all item types. */
	const QHash<int, int >& itemRouteIdx();

	/** Returns item types of the items in the current BOM. */
	const QHash<int, int > itemID2ItemType();

	/** Number of different item types (with ID > 0) in the corresponding BOM. */
	int nItemTypes();

	/** The number of items in the BOM */
	int nItems();

	/** Graph of the BOP. */
	inline const ListDigraph& graph() {
		return _graph;
	}

	/** Graph head of the BOP. */
	inline const ListDigraph::Node& graphHead() {
		return _head;
	}

	/** Graph tail of the BOP. */
	inline const ListDigraph::Node& graphTail() {
		return _tail;
	}

	/** Return BOM of this BOP. */
	inline BillOfMaterials* bom() {
		return _bom;
	}

	/** Mapping of the nodes in the current BOM onto the nodes of the generated BOP. */
	inline const QMap<ListDigraph::Node, QList<ListDigraph::Node> >& bomNode2BopNodes() {
		return _bomNode2BOPNodes;
	}

	inline const QMap<ListDigraph::Node, ListDigraph::Node >& bopNode2BomNode() {
		return _bopNode2BOMNode;
	}

	/** Type IDs associated with the nodes of the graph of the BOP. */
	inline const ListDigraph::NodeMap<int>& otypeID() {
		return _otypeID;
	}

	/** Local item IDs associated with the nodes of the graph of the BOP. */
	inline const ListDigraph::NodeMap<int>& oLocItemID() {
		return _oLocItemID;
	}

	/** Expected processing cost for one entity of the product.  */
	virtual double expCost(Resources &rc);

	/** Returns list of precedences of the operation types with the associated
	 *  values. It can be interpreted as a matrix. Note: some operation types
	 *  can be duplicated since one BOP can contain several operations of the
	 *	same type. */
	QList<QPair<QPair<int, int>, double > > toMatrix();

	/** Clear the BOP */
	void clear();

	/** Output of the BOP to the text stream. */
	//friend QTextStream& operator<<(QTextStream &out, BillOfProcesses &bop);
	friend QTextStream& operator<<(QTextStream &out, const BillOfProcesses &bop);

private:

};

#endif	/* BILLOFPROCESSES_H */

