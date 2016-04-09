/* 
 * File:   BillOfProcesses.cpp
 * Author: DrSobik
 * 
 * Created on July 29, 2011, 3:37 PM
 */

#include "BillOfProcesses.h"

int BillOfProcesses::bopsCreated = 0;
int BillOfProcesses::bopsDeleted = 0;

BillOfProcesses::BillOfProcesses() : _otypeID(_graph), _oLocItemID(_graph) {
	// Clear the BOP
	clear();

	bopsCreated++;
}

BillOfProcesses::~BillOfProcesses() {
	//QTextStream out(stdout);

	bopsDeleted++;

	//out << "BOPs created : " << bopsCreated << endl;
	//out << "BOPs deleted : " << bopsDeleted << endl;
}

void BillOfProcesses::setData(BillOfMaterials *bom, QHash<int, QList<Route*> > &routes) {

	// Clear the BOP
	clear();

	// Assign the BOM
	_bom = bom;

	// Assign the possible item routes
	//_iroutes = routes;
	// IMPROVEMENT : Assign possible item routes only for the item types present within the given BOM
	for (ListDigraph::NodeIt nit(bom->graph); nit != INVALID; ++nit) {
		if (nit != bom->head && nit != bom->tail && bom->itypeID[nit] > 0) {
			_iroutes.insert(bom->itypeID[nit], routes[bom->itypeID[nit]]);
		}
	}

	// Initialize the item routes selector
	//_itype2routeidx.clear();
	//for (QHash<int, QList<Route*> >::iterator iter = _iroutes.begin(); iter != _iroutes.end(); iter++) {
	//	_itype2routeidx.insert(iter.key(), -1);
	//}

	_itemID2routeidx.clear();
	for (ListDigraph::NodeIt nit(_bom->graph); nit != INVALID; ++nit) {
		_itemID2routeidx.insert(_bom->itemID[nit], -1);
	}

	// Set item routes randomly
	setItemRouteIdxRnd();
}

void BillOfProcesses::generate() {
	// Degenerate first
	degenerate();

	//this->_bom = bom;
	//this->_routes = routes;

	// Build the graph based on the information from the bom and the route

	if (_bom == NULL) {
		Debugger::err << "BillOfProcesses::generate : Failed to generate BOP. BOM is not provided!" << ENDL;
	}

	if (_iroutes.size() == 0) {
		//Debugger::err << "BillOfProcesses::generate : Failed to generate BOP. Routes are not provided!" << ENDL;
		Debugger::warn << "BillOfProcesses::generate : Routes are not provided!" << ENDL;
	}

	// Copy the BOMs' graph
	ListDigraph::NodeMap<ListDigraph::Node> ref(_bom->graph); // Mapping of the BOM graph onto this graph

	digraphCopy(_bom->graph, this->_graph).node(_bom->head, _head).node(_bom->tail, _tail).nodeRef(ref).run();

	_otypeID[_head] = -1;
	_otypeID[_tail] = -2;

	// IMPORTANT!!! The item routes are already set and DO NOT change here!

	// Iterate through the nodes of the graph, replace the current node with the
	// chain of nodes from the route.
	ListDigraph::Node curnode;
	_bomNode2BOPNodes.clear();
	_bopNode2BOMNode.clear();
	for (ListDigraph::NodeIt nit(_bom->graph); nit != INVALID; ++nit) {
		if (nit == _bom->head || nit == _bom->tail) continue;

		curnode = ref[nit];

		_bomNode2BOPNodes[nit].append(curnode); // This will help identify which operations in the BOP belong to which item in the BOM
		_bopNode2BOMNode[curnode] = nit;

		// Set the local item ID of the operations in context of the BOM
		_oLocItemID[curnode] = _bom->itemID[nit];

		//Debugger::info << "Current item type: " << _bom->itypeID[nit] << ENDL;
		// Notice : Randomness during the BOP generation
		//int currouteidx = _itype2routeidx[_bom->itypeID[nit]]; //Math::rndInt(0, _iroutes[_bom->itypeID[nit]].size() - 1);
		int currouteidx = _itemID2routeidx[_bom->itemID[nit]];
		//Debugger::info << "Current route idx : " << currouteidx << ENDL;
		//cout << "Current itype ID : " << _bom->itypeID[nit] << endl;
		//cout << "Number of routes : " << _iroutes[_bom->itypeID[nit]].size() << endl;
		for (int i = 0; i < _iroutes[_bom->itypeID[nit]][currouteidx]->otypeIDs.size() - 1; i++) {
			//cout << "current route idx : " << currouteidx << endl;
			_otypeID[curnode] = _iroutes[_bom->itypeID[nit]][currouteidx]->otypeIDs[i];
			//Debugger::info << "Inserting operation type: " << _otypeID[curnode] << ENDL;
			curnode = _graph.split(curnode, true);

			_bomNode2BOPNodes[nit].append(curnode); // This will help identify which operations in the BOP belong to which item in the BOM
			_bopNode2BOMNode[curnode] = nit;

			// Set the local item ID of the operations in context of the BOM
			_oLocItemID[curnode] = _bom->itemID[nit];

		}
		_otypeID[curnode] = _iroutes[_bom->itypeID[nit]][currouteidx]->otypeIDs.last();



		//Debugger::info << "Inserting operation type: " << _otypeID[curnode] << ENDL;

	}

}

void BillOfProcesses::degenerate() {
	// Clear the BOP but DO NOT clear the generation data
	QList<ListDigraph::Arc> arcs;

	for (ListDigraph::ArcIt ait(_graph); ait != INVALID; ++ait) {
		arcs.append(ait);
	}
	for (int i = 0; i < arcs.size(); i++) {
		_graph.erase(arcs[i]);
	}

	_graph.clear();
	_head = INVALID;
	_tail = INVALID;

	_bomNode2BOPNodes.clear();
	_bopNode2BOMNode.clear();
}

void BillOfProcesses::generateAll(QList<BillOfMaterials*> &boms, QHash<int, QList<Route*> > &iroutes, QList<BillOfProcesses*> &bops) {
	/**
	 * Algorithm:
	 * 
	 * 1. Iterate over the list of the BOMs.
	 * 2. For current BOM:
	 * 2.1. Iterate over the nodes of the BOMs graph and collect the item types in it;
	 * 2.2. Generate hashes of the item types based on different possibilities of alternative route selection
	 * 2.3. Generate the corresponding BOPs
	 * 
	 */

	//QTextStream out(stdout);

	//out << "BillOfProcesses::generateAll : Number of BOMs : " << boms.size() << endl;
	//out << "BillOfProcesses::generateAll : Number of item types : " << iroutes.size() << endl;

	//out << "Routes for item types:" << endl;
	/*
	for (QHash<int, QList<Route*> >::iterator iter = iroutes.begin(); iter != iroutes.end(); iter++) {
	out << "Item type : " << iter.key() << endl;
	out << "Routes:" << endl;
	for (int i = 0; i < iter.value().size(); i++) {
		out << iter.value()[i]->ID << " : ";
		for (int j = 0; j < iter.value()[i]->otypeID.size(); j++) {
		out << iter.value()[i]->otypeID[j] << " ";
		}
		out << endl;
	}
	}
	 */

	//static double totalsz = 0.0;

	//QSet<int> bomitypeset;
	//QList<int> bomitypeslist;
	//QHash<int, int> cur_idcs; // Indices of the current Route for each item type
	//int carrier;
	//QHash<int, Route*> cur_routes;

	// Iterations over the list of BOMs
	for (int cb = 0; cb < boms.size(); cb++) {

		/*
		// Initialize current indices
		for (QHash<int, QList<Route*> >::iterator iter = iroutes.begin(); iter != iroutes.end(); iter++) {
			cur_idcs.insert(iter.key(), 0);
		}

		// Collect the item types in the BOM
		bomitypeset.clear();
		for (ListDigraph::NodeIt nit(boms[cb]->graph); nit != INVALID; ++nit) {
			if (boms[cb]->itypeID[nit] < 0) {
			continue;
			} else {
			if (!bomitypeset.contains(boms[cb]->itypeID[nit])) {
				bomitypeset.insert(boms[cb]->itypeID[nit]);
			}
			}
		}

		bomitypeslist = bomitypeset.toList();
		 */

		/*
		do {

			// Select the actual routes for the items
			cur_routes.clear();
			for (int i = 0; i < bomitypeslist.size(); i++) {
			cur_routes[bomitypeslist[i]] = iroutes[bomitypeslist[i]][cur_idcs[bomitypeslist[i]]];
			}

			// Perform the BOP initialization according to the current indices of the routes
		 */
		bops.append(new BillOfProcesses);
		bops.last()->setData(boms[cb], /*cur_routes*/iroutes);

		// Set the ID of BOP = ID of the BOM : Changed on 05.08.2013
		bops.last()->ID = boms[cb]->ID;

		//totalsz += (cur_routes.size() * sizeof (*(bops.last()))) / 1024.0 / 1024.0;
		//Debugger::info << "Total size :  " << totalsz << ENDL;

		//bops.last()->generate();

		/*
		// Update the current indices
		carrier = 1;
		for (int i = 0; i < bomitypeslist.size(); i++) {
			cur_idcs[bomitypeslist[i]] += carrier;

			if (cur_idcs[bomitypeslist[i]] == iroutes[bomitypeslist[i]].size()) {
			carrier = 1;
			if (i < bomitypeslist.size() - 1) {
				cur_idcs[bomitypeslist[i]] = 0;
			}
			} else {
			carrier = 0;
			}
		}


		//cout << "Current indices:" << endl;
		//for (int i = 0; i < bomitypeslist.size(); i++) {
		//    cout << cur_idcs[bomitypeslist[i]] << " ";
		//}
		//cout << endl;


		} while (cur_idcs[bomitypeslist.last()] < iroutes[bomitypeslist.last()].size());
		 */
	}

}

void BillOfProcesses::setItemRouteIdxRnd() {

	/*
	// In case all items of the same type have exactly the same route
	for (QHash<int, QList<Route*> >::iterator iter = _iroutes.begin(); iter != _iroutes.end(); iter++) {
		_itype2routeidx[iter.key()] = Math::rndInt(0, iter.value().size() - 1);
	}
	 */

	// In case items of the same type can follow different routes
	for (ListDigraph::NodeIt nit(_bom->graph); nit != INVALID; ++nit) {
		if (nit == _bom->head || nit == _bom->tail) {
			_itemID2routeidx[_bom->itemID[nit]] = -1; // This item has no route
		} else {
			//_itemID2routeidx[_bom->itemID[nit]] = Rand::rndInt(0, _iroutes[_bom->itypeID[nit]].size() - 1);
			_itemID2routeidx[_bom->itemID[nit]] = Rand::rnd<Math::uint32>(0, _iroutes[_bom->itypeID[nit]].size() - 1);
		}
	}

}

void BillOfProcesses::setItemRouteIdx(const QHash<int, int> &routeidcs) {
	//_itype2routeidx = routeidcs;
	_itemID2routeidx = routeidcs;
}

const QHash<int, QList<Route*> >& BillOfProcesses::itemTypeRoutes() {
	return _iroutes;
}

const QHash<int, QList<Route*> > BillOfProcesses::itemRoutes() {
	QHash<int, QList<Route*> > res;

	res.clear();

	// Iterate over the nodes of the BOM
	for (ListDigraph::NodeIt nit(_bom->graph); nit != INVALID; ++nit) {
		if (nit == _bom->head || nit == _bom->tail) {
			continue;
		} else {
			res[_bom->itemID[nit]] = _iroutes[_bom->itypeID[nit]];
		}
	}

	return res;
}

const QHash<int, int >& BillOfProcesses::itemRouteIdx() {
	return _itemID2routeidx; //_itype2routeidx;
}

const QHash<int, int > BillOfProcesses::itemID2ItemType() {
	QHash<int, int > res;

	res.clear();

	// Iterate over the nodes of the BOM
	for (ListDigraph::NodeIt nit(_bom->graph); nit != INVALID; ++nit) {
		if (nit == _bom->head || nit == _bom->tail) {
			res[_bom->itemID[nit]] = -1;
		} else {
			res[_bom->itemID[nit]] = _bom->itypeID[nit];
		}
	}

	return res;
}

int BillOfProcesses::nItemTypes() {
	QSet<int> contained;

	for (ListDigraph::NodeIt nit(_bom->graph); nit != INVALID; ++nit) {
		if (nit != _bom->head && nit != _bom->tail) {
			contained.insert(_bom->itypeID[nit]);
		}
	}

	return contained.size();
}

int BillOfProcesses::nItems() {
	return countNodes(_bom->graph) - 2; // Head and tail are avoided
}

double BillOfProcesses::expCost(Resources &rc) {
	double res = 0.0;
	double expspeed = 0.0;

	// Generate the BOP
	generate();

	// Iterate over all operations in the BOP and get the expected processing cost for all operations
	for (ListDigraph::NodeIt nit(_graph); nit != INVALID; ++nit) {
		if (nit != _head && nit != _tail && _otypeID[nit] > 0) {
			// Get the expected processing speed for the operation type
			int ntools = rc.type2idcs[_otypeID[nit]].size();
			if (ntools <= 0) {
				Debugger::err << "BillOfProcesses::expCost : Failed to find TG for processing operation type " << _otypeID[nit] << ENDL;
			}
			expspeed = 0.0;
			for (int i = 0; i < ntools; i++) {
				expspeed += rc.tools[rc.type2idcs[_otypeID[nit]][i]]->expectedSpeed(_otypeID[nit]);
			}
			expspeed /= double(ntools);

			if (Math::cmp(expspeed, 0.0) == 0) {
				Debugger::err << "BillOfProcesses::expCost : Expected processing speed is 0 for operation type " << _otypeID[nit] << ENDL;
			}

			// Update the expected cost
			res += 1.0 / expspeed;
		}
	}

	// Degenerate the BOP
	degenerate();

	return res;
}

QList<QPair<QPair<int, int>, double > > BillOfProcesses::toMatrix() {
	/**
	 * Algorithm:
	 * 
	 * 1. Run BFS and for each non-fake node which is adjacent with the other
	 *	  non-fake node add the pair of the nodes (corresponding operation types)
	 *	  to the result.
	 * 
	 */

	QList<QPair<QPair<int, int>, double > > res;
	res.clear();

	// Run BFS and collect the desired pairs of the nodes
	Bfs<ListDigraph> bfs(_graph);
	bfs.init();
	bfs.addSource(_head);

	ListDigraph::Node curnode;
	while (!bfs.emptyQueue()) {

		curnode = bfs.processNextNode();

		if (curnode == _head || curnode == _tail) continue;

		for (ListDigraph::OutArcIt oait(_graph, curnode); oait != INVALID; ++oait) {

			if (_graph.target(oait) == _tail) continue;

			// Add the pair and the associated value to the result
			res.append(QPair<QPair<int, int>, double >(QPair<int, int>(_otypeID[_graph.source(oait)], _otypeID[_graph.target(oait)]), 1.0));
		}

	}

	return res;
}

void BillOfProcesses::clear() {

	_graph.clear();
	_head = INVALID;
	_tail = INVALID;

	_bom = NULL;
	_iroutes.clear();
	//_itype2routeidx.clear();
	_itemID2routeidx.clear();
}

QTextStream& operator<<(QTextStream& out, const BillOfProcesses& bop) {
	out << "BOP: [" << "<" << "ID=" << bop.ID << ">" << endl;

	out << "BOM : " << endl << *bop._bom << endl;

	// Print operation types and precedence constraints within this BOP.
	out << "nodes : " << countNodes(bop._graph) << endl;
	for (ListDigraph::ArcIt ait(bop._graph); ait != INVALID; ++ait) {
		out << "(" << bop._otypeID[bop._graph.source(ait)] << "->" << bop._otypeID[bop._graph.target(ait)] << ")" << endl;
	}

	out << "]";

	return out;
}

//QTextStream& operator<<(QTextStream &out, BillOfProcesses &bop) {
//	return (out << bop);
//}
