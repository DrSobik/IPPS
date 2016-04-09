/* 
 * File:   GAPlanner.cpp
 * Author: DrSobik
 * 
 * Created on February 26, 2013, 4:00 PM
 */

#include "GAPlanner.h"
//#include "PlannerAgent.h"
#include "TrivialScheduler.h"

#include <QTextStream>
//#include <QtXml/qxmlstream.h>

/** ###########################  GAPlannerGenome  ########################### */

GAPlannerGenome::GAPlannerGenome() : obj(NULL) {
	initializer(init);
	mutator(mutate);
	comparator(compare);
	crossover(cross);
	evaluator(objective);

	pmm = NULL;
	scheduler = NULL;
	rc.clear();
	operations.clear();
	plan.clear();
	schedule.clear();
}

GAPlannerGenome::GAPlannerGenome(const GAPlannerGenome& orig) : GAGenome(orig) {

	this->copy(orig);

}

GAPlannerGenome::~GAPlannerGenome() {

}

void GAPlannerGenome::copy(const GAGenome& orig) {

	GAGenome::copy(orig);

	QTextStream out(stdout);

	//out << "Copying..." << endl;

	GAPlannerGenome &g = (GAPlannerGenome&) orig;

	pmm = g.pmm;

	scheduler = g.scheduler;

	rc = g.rc;

	operations = g.operations;
	operationPrecedences = g.operationPrecedences;
	operationPrecedencesConj = g.operationPrecedencesConj;

	schedOptions = g.schedOptions;

	plan = g.plan;

	schedule = g.schedule;

	crossSection = g.crossSection;
	crossStart = g.crossStart;
	crossEnd = g.crossEnd;

	_lsMaxIter = g._lsMaxIter;
	_lsChkCor = g._lsChkCor;
	_solImprove = g._solImprove;

	obj = g.obj;

	//out << "Done copying." << endl;

}

GAGenome & GAPlannerGenome::operator=(const GAGenome &orig) {

	QTextStream out(stdout);

	if (&orig != this) this->copy((GAPlannerGenome&) orig);

	return *this;
}

GAGenome* GAPlannerGenome::clone(CloneMethod) const {
	//cout<<"Genome:: Cloning"<<endl;
	return new GAPlannerGenome(*this);
}

int GAPlannerGenome::write(ostream &) const {
	// TODO
}

QTextStream& operator<<(QTextStream& out, const GAPlannerGenome& g) {

	out << "Genome : " << endl;
	out << "[" << endl;

	// Write the operations
	for (int i = 0; i < g.operations.size(); i++) {
		Operation& curOp = (Operation&) g.operations[i];

		out << curOp << endl;
	}

	out << "Objective : " << g.score() << endl;

	out << "]" << endl;


	return out;

}

bool GAPlannerGenome::valid() {
	//return true;

	QTextStream out(stdout);

	// Iterate over the operations and cound the number of operations for each order. Then check whether each order has the same number of operations
	QHash<int, QSet<int> > prodID2opIDs;
	QHash<int, QSet<int> > prodID2ordIDs;
	QSet<int> prodIDs;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2opIDs[curOp.OT] << curOp.ID;

		prodID2ordIDs[curOp.OT] << curOp.OID;

		prodIDs << curOp.OT;

	}

	foreach(const int& curProdID, prodIDs) {
		if (prodID2opIDs[curProdID].size() % prodID2ordIDs[curProdID].size() != 0) {
			out << "GAPlannerGenome::valid : Wrong number of operations in the orders!!!" << endl;
			return false;
		}
	}

	// Check whether there are operations with negative IDs
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		if (curOp.ID <= 0) {
			out << "GAPlannerGenome::valid : Invalid operation!!!" << ENDL;
			return false;
		}

	}

	return true;
}

void GAPlannerGenome::initialize() {

	QTextStream out(stdout);

	Debugger::info << "GAPlannerGenome::initialize : Initializing the genome ... " << ENDL;

	/** 
	 * Iterate over the products. Select a BOM for each product.
	 **/

	for (int i = 0; i < pmm->prodman->products.size(); i++) {

		Product& curProd = (Product&) *(pmm->prodman->products[i]);

		out << "GAPlannerGenome::initialize : Selecting a BOP for product " << curProd.ID << endl;

		BillOfProcesses& curBOP = (BillOfProcesses&) *(curProd.rndBOP());

		curProd.bopID = curBOP.ID;

		out << "GAPlannerGenome::initialize : Selected BOP " << curBOP.ID << endl;

		// Randomly set the item route indices
		curBOP.setItemRouteIdxRnd();

	}

	out << "GAPlannerGenome::initialize : Creating the initial plan for the genome... " << endl;

	Plan initPlan;

	// Collect the current plan for the genome
	initPlan = pmm->prodman->bops2Plan();

	out << initPlan << endl;

	// Restore the products' states from the initial process plan
	pmm->prodman->bopsFromPlan(initPlan);

	// Generate the actual process model
	ProcessModel planPM = pmm->plan2PM(initPlan);

	// Prepare the resources and assign the operations
	rc.init();
	for (ListDigraph::NodeIt nit(planPM.graph); nit != INVALID; ++nit) {
		if (!rc.assign(planPM.ops[nit])) {

			out << "GAPlannerGenome::initialize : Assigning operation: " << *pmm->pm.ops[nit] << endl;
			Debugger::err << "GAPlannerGenome::initialize : Failed to assign operation to resource!" << ENDL;
		}
	}

	out << planPM << endl;

	Schedule sched;

	// IMPORTANT!!! Set the ID of this scheduler as the index corresponding to the current plan
	scheduler->ID = 0;
	scheduler->pm = planPM;
	scheduler->rc = rc;
	scheduler->sched = &sched;
	scheduler->options = schedOptions;
	scheduler->obj = obj;

	// Now the process model can be cleaned
	planPM.clearSchedRelData();

	// Run the scheduler
	scheduler->schedule();

	Debugger::info << "GAPlannerGenome::initialize : Performed initial scheduling. " << ENDL;

	Debugger::info << "GAPlannerGenome::initialize : Found solution : " << sched.objective << ENDL;

	// Turn the found schedule into the genome's representation
	out << sched << endl;

	operations = sched.operations;
	operationPrecedences = sched.operationPrecedences; // Precedences which correspond to the current schedule
	operationPrecedencesConj = sched.operationPrecedencesConj;

	/*
	// Remove the non-conjunctive precedences
	QSet<int> nonConjPrecIdcs;
	for (int i = 0; i < operationPrecedencesConj.size(); i++) {
		if (!operationPrecedencesConj[i]) {
			nonConjPrecIdcs << i;
		}
	}
	QList<QPair<int, int> > newOperationPrecedences;
	QList<bool > newOperationPrecedencesConj;
	for (int i = 0; i < operationPrecedencesConj.size(); i++) {
		if (!nonConjPrecIdcs.contains(i)) {
			newOperationPrecedences << operationPrecedences[i];
			newOperationPrecedencesConj << true;
		}
	}
	operationPrecedences = newOperationPrecedences;
	operationPrecedencesConj = newOperationPrecedencesConj;
	 */


	plan = initPlan;

	schedule = sched;

}

void GAPlannerGenome::init(GAGenome &gen) {
	GAPlannerGenome &g = (GAPlannerGenome &) gen;

	g.initialize();

}

void GAPlannerGenome::mate(GAPlannerGenome &mate, GAPlannerGenome& child) {

	QTextStream out(stdout);

	//out << "GAPlannerGenome::mate : Mating... " << endl;

	// Select a crossing section of the mate
	crossStart = Rand::rndInt(0, Math::min(operations.size() - 1, mate.operations.size() - 1)); // [0, n-1] Inserting into this parent
	crossEnd = Rand::rndInt(crossStart, Math::min(crossStart + 100000, mate.operations.size() - 1) /*mate.operations.size() - 1*/); // [crossStart, n-1]
	int crossLen = crossEnd - crossStart + 1;
	crossSection = mate.operations.mid(crossStart, crossLen);

	// Copy the representation of this genome into the child
	child.operations = operations;

	// Insert the crossing section into the child
	int curInsertPos = crossStart;
	for (int i = 0; i < crossSection.size(); i++) {

		Operation curOp = crossSection[i];

		child.operations.insert(curInsertPos, curOp);

		curInsertPos++;

	}

	// Copy the information about the crossing section into the child
	child.crossSection = crossSection;
	child.crossStart = crossStart;
	child.crossEnd = crossEnd;

	// Repair the child
	//out << "GAPlannerGenome::mate : Repairing..."<< endl;
	child.repair(*this, mate);
	//out << "GAPlannerGenome::mate : Done repairing."<< endl;

	child.evaluate(gaTrue);

	//out << "GAPlannerGenome::mate : Done mating. " << endl;

	//out << "GAPlannerGenome::mate : Child genome " << child << endl;

	//getchar();
}

int GAPlannerGenome::cross(const GAGenome& a, const GAGenome& b, GAGenome* c, GAGenome* d) {
	//cout << "Genome:: crossing" << endl;
	GAPlannerGenome& dad = (GAPlannerGenome&) a;
	GAPlannerGenome& mom = (GAPlannerGenome&) b;
	GAPlannerGenome& sis = (GAPlannerGenome&) * c;
	GAPlannerGenome& bro = (GAPlannerGenome&) * d;

	int n_crossovers = 0; // Amount of the crossovers

	// Copy parents in order to initialize
	//if (c) sis.copy(mom);
	//if (d) bro.copy(dad);

	if (c && d) { // Make sister and brother

		mom.mate(dad, sis);
		dad.mate(mom, bro);

		sis.evaluate(gaTrue);
		bro.evaluate(gaTrue);
		n_crossovers = 2;
	} else {
		if (c) { // Only sister

			mom.mate(dad, sis);

			sis.evaluate(gaTrue);
			n_crossovers = 1;
		} else {
			if (d) { // Only brother

				dad.mate(mom, bro);

				bro.evaluate(gaTrue);
				n_crossovers = 1;
			}
		}
	}

	//Update the completion times
	//if (c) sis.CTs();
	//if (d) bro.CTs();

	//cout<< "n_crosses : "<<n_crossovers<<endl;
	//cout << "Cross finished" << endl;

	return n_crossovers;
}

void GAPlannerGenome::mutateMach(const float prob) {


	QTextStream out(stdout);

	int numOper = Rand::rndInt(1, Math::max((int) Math::ceil(prob * operations.size()), 1));


	while (numOper > 0) {

		// Select randomly an operation
		int curOpIdx = Rand::rndInt(0, operations.size() - 1);
		Operation& curOp = (Operation&) operations[curOpIdx];

		ToolGroup& curTG = (ToolGroup&) rc(curOp.toolID); // The current machine group for the operation

		QList<Machine*> curMachs = curTG.machines(curOp.type);

		Machine& m = (Machine&) * curMachs[Rand::rndInt(0, curMachs.size() - 1)];

		// Mutate the machine of the operation
		curOp.machID = m.ID;

		// Update the processing time of the operation
		curOp.p(m.procTime(&curOp));

		// Repair its disjunctive precedence constraints

		QSet<int> opPrecIdxRem; // Indices of the precedence constraints to be removed
		int oldPredOpID = -1;
		int oldSuccOpID = -1;
		for (int i = 0; i < operationPrecedences.size(); i++) {

			if (!operationPrecedencesConj[i]) { // This is a machine precedence

				QPair<int, int> curPrec = operationPrecedences[i];

				if (curPrec.first == curOp.ID || curPrec.second == curOp.ID) {
					opPrecIdxRem << i;
				}

				if (curPrec.second == curOp.ID) {
					oldPredOpID = curPrec.first;
				}

				if (curPrec.first == curOp.ID) {
					oldSuccOpID = curPrec.second;
				}

			}

		}
		// Remove the false precedence constraints
		QList<QPair<int, int> > newOperationPrecedences;
		QList<bool > newOperationPrecedencesConj;
		for (int i = 0; i < operationPrecedences.size(); i++) {
			if (!opPrecIdxRem.contains(i)) {
				newOperationPrecedences << operationPrecedences[i];
				newOperationPrecedencesConj << operationPrecedencesConj[i];
			}
		}
		operationPrecedences = newOperationPrecedences;
		operationPrecedencesConj = newOperationPrecedencesConj;

		// Add the disjunctive precedence constraint between the old predecessor and the old successor of this operation
		if (oldPredOpID != -1 && oldSuccOpID != -1) {
			operationPrecedences << QPair<int, int>(oldPredOpID, oldSuccOpID);
			operationPrecedencesConj << false;
		}

		// Add the new machine operation precedences
		int newPredOpID = -1;
		int newSuccOpID = -1;
		for (int j = curOpIdx + 1; j < operations.size(); j++) {
			if (curOp.machID == operations[j].machID) {
				newSuccOpID = operations[j].ID;
				break;
			}
		}
		for (int j = curOpIdx - 1; j >= 0; j--) {
			if (curOp.machID == operations[j].machID) {
				newPredOpID = operations[j].ID;
				break;
			}
		}
		if (newPredOpID != -1) {
			operationPrecedences << QPair<int, int>(newPredOpID, curOp.ID);
			operationPrecedencesConj << false;
		}
		if (newSuccOpID != -1) {
			operationPrecedences << QPair<int, int>(curOp.ID, newSuccOpID);
			operationPrecedencesConj << false;
		}

		// Remove the possible previous precedence between the new neighboring operations
		if (newPredOpID != -1 && newSuccOpID != -1) {

			int idxRem = -1;
			for (idxRem = 0; idxRem < operationPrecedences.size(); idxRem++) {
				if (operationPrecedences[idxRem].first == newPredOpID && operationPrecedences[idxRem].second == newSuccOpID && !operationPrecedencesConj[idxRem]) {
					break;
				}
			}

			operationPrecedences.removeAt(idxRem);
			operationPrecedencesConj.removeAt(idxRem);

		}

		numOper--;
	}

	//double prevObj = schedule.objective;

	// Restore the PM

	//out << "GAPlannerGenome::mutateMach : Restoring with precedence constraints : " << endl;
	//for (int i = 0; i < operationPrecedences.size(); i++) {
	//	out << operationPrecedences[i].first << "->" << operationPrecedences[i].second << " : " << operationPrecedencesConj[i] << endl;
	//}

	ProcessModel restoredPM = restorePM();
	schedule.fromPM(restoredPM, *obj);

	//operations = schedule.operations;
	//operationPrecedences = schedule.operationPrecedences;
	//operationPrecedencesConj = schedule.operationPrecedencesConj;

	//out << "After the machine mutator : " << endl << *this << endl;

	//out << restoredPM << endl;

	//out << " --------- " << prevObj << endl;
	//out << " -------- " << TWT()(restoredPM) << endl;
	//out << " --------- " << schedule.objective << endl;

	//getchar();

}

void GAPlannerGenome::mutateRoute(const float prob) {

	// Select the number of parts to mutate
	QTextStream out(stdout);

	// Select for how many products the routes of their parts are mutated
	int numProdsToChange = Rand::rndInt(1, Math::max((int) Math::ceil(prob * pmm->prodman->products.size()), 1));
	QSet<int> prodIDsToChange;
	for (int i = 0; i < numProdsToChange; i++) {
		prodIDsToChange << pmm->prodman->products[Rand::rndInt(0, pmm->prodman->products.size() - 1)]->ID;
	}

	// Collect orders of each product
	QHash<int, QSet<int> > prodID2ordIDs;
	QHash<int, int > prodID2BOPID;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2ordIDs[curOp.OT] << curOp.OID;

		prodID2BOPID[curOp.OT] = curOp.BID;


	}

	// For each selected product perform a route change

	foreach(const int& curProdID, prodIDsToChange) {

		Product& curProd = (*(pmm->prodman))(curProdID);

		QSet<int> curOrderIDs = prodID2ordIDs[curProdID]; // Order IDs of the current product

		BillOfProcesses& curBOP = (BillOfProcesses&) * curProd.bopByID(prodID2BOPID[curProdID]);

		int numParts = Rand::rndInt(1, Math::max(1, (int) prob * (countNodes(curBOP.bom()->graph) - 2)));

		QSet<int> locItmIDs; // Local IDs of the items which must be mutated
		QHash<int, int> locItmID2itmType;

		// Collect all local IDs of the items
		for (ListDigraph::NodeIt nit(curBOP.bom()->graph); nit != INVALID; ++nit) {
			if (nit != curBOP.bom()->head && nit != curBOP.bom()->tail) {

				locItmIDs << curBOP.bom()->itemID[nit];

				locItmID2itmType[curBOP.bom()->itemID[nit]] = curBOP.bom()->itypeID[nit];

			}
		}

		// Remove the spare item IDs
		while (locItmIDs.size() > numParts) {
			locItmIDs.remove(locItmIDs.toList()[Rand::rndInt(0, locItmIDs.toList().size() - 1)]);
		}

		// Iterate over the local item IDs

		foreach(const int& curLocItmID, locItmIDs) {

			// Get the type of the local item
			int curItmType = locItmID2itmType[curLocItmID];

			// Select a new route for the items
			const QList<Route*>& curRoutes = curBOP.itemTypeRoutes()[curItmType];
			int newItmRouteIdx = Rand::rndInt(0, curRoutes.size() - 1);
			int newItmRouteID = curRoutes[newItmRouteIdx]->ID;

			plan.prodID2ItemID2RouteID[curProdID][curLocItmID] = newItmRouteID;
			plan.prodID2ItemID2RouteIdx[curProdID][curLocItmID] = newItmRouteIdx;

#ifdef DEBUG
			QSet<int> curItmRouteIDs;

			foreach(const int& curOrdID, curOrderIDs) {
				for (int i = 0; i < operations.size(); i++) {
					Operation& curOp = (Operation&) operations[i];
					if (curOp.OID == curOrdID && (curOp.IID - (curOrdID << 16)) == curLocItmID) {
						curItmRouteIDs << curOp.RID;
					}
				}
			}

			if (curItmRouteIDs.size() > 1) {
				Debugger::err << "GAPlannerGenome::mutateRoute : Too many routes for the same item!!!" << ENDL;
			}
#endif

			// Iterate over the orders of the product

			foreach(const int& curOrdID, curOrderIDs) {

				// Find all operations of the current part
				QList<int> curItemOperIdcs;
				QList<Operation> curItemOps;
				QSet<int> curItemOpIDs;

				for (int i = 0; i < operations.size(); i++) {

					Operation& curOp = (Operation&) operations[i];

					if (curOp.OID == curOrdID && (curOp.IID - (curOrdID << 16)) == curLocItmID) {
						curItemOperIdcs << i;
						curItemOps << curOp;
						curItemOpIDs << curOp.ID;
					}

				} // Finding operation indices of the current part

				// Remove the disjunctive precedence constraints 
				QSet<int> opPrecIdcsRem;
				for (int i = 0; i < operationPrecedences.size(); i++) {

					if (!operationPrecedencesConj[i]) {

						QPair<int, int> curPrec = operationPrecedences[i];

						//if (curItemOpIDs.contains(curPrec.first) || curItemOpIDs.contains(curPrec.second)) {
						opPrecIdcsRem << i;
						//}

					}

				}
				QList<QPair<int, int> > newOperationPrecedences;
				QList<bool> newOperationPrecedencesConj;
				for (int i = 0; i < operationPrecedences.size(); i++) {
					if (!opPrecIdcsRem.contains(i)) {
						newOperationPrecedences << operationPrecedences[i];
						newOperationPrecedencesConj << operationPrecedencesConj[i];
					}
				}
				operationPrecedences = newOperationPrecedences;
				operationPrecedencesConj = newOperationPrecedencesConj;

				// Change the content of the item's operations without touching their IDs => conjunctive precedences do not change
				Route& curRoute = (Route&) * curRoutes[newItmRouteIdx];
				for (int i = 0; i < curRoute.otypeIDs.size(); i++) {

					int curOpType = curRoute.otypeIDs[i];

					for (int j = i; j < curItemOps.size(); j++) {
						if (curItemOps[j].type == curOpType) { // Move this operation to the i-th position
							curItemOps.move(j, i);
							curItemOps[i].RID = newItmRouteID;
							break;
						}
					}

				}

				// Move the permuted operations into the main array
				for (int i = 0; i < curItemOperIdcs.size(); i++) {
					int curIdx = curItemOperIdcs[i];

					// Preserve the ID
					int curID = operations[curIdx].ID;

					operations[curIdx] = curItemOps.takeFirst();

					// Restore the ID
					operations[curIdx].ID = curID;
				}

				// Restore all disjunctive precedence constraints
				for (int i = 0; i < operations.size(); i++) {
					int curMachID = operations[i].machID;

					// Search for its successor
					for (int j = i + 1; j < operations.size(); j++) {
						if (operations[j].machID == curMachID) {
							operationPrecedences << QPair<int, int>(operations[i].ID, operations[j].ID);
							operationPrecedencesConj << false;
							break;
						}

					}

				} // Restoring the disjunctive precedence constraints


			} // Iterating over the orders of the product

		} // Iterating over the local item IDs


	} // Iterating over the products


	ProcessModel restoredPM = restorePM();
	schedule.fromPM(restoredPM, *obj);

}

void GAPlannerGenome::mutateBOM(const float prob) {

	QTextStream out(stdout);

	//out << "Before the BOM mutator : " << endl << *this << endl;
	//out << schedule.pm << endl;

	schedule.objective = Math::MAX_DOUBLE;

	// Select the number of products to be mutated
	int numProdsToChange = Rand::rndInt(1, Math::max((int) Math::ceil(prob * pmm->prodman->products.size()), 1));
	QSet<int> prodIDsToChange;
	for (int i = 0; i < numProdsToChange; i++) {
		prodIDsToChange << pmm->prodman->products[Rand::rndInt(0, pmm->prodman->products.size() - 1)]->ID;
	}

	// Collect orders of each product
	QHash<int, QSet<int> > prodID2ordIDs;
	QHash<int, int > prodID2BOPID;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2ordIDs[curOp.OT] << curOp.OID;

		prodID2BOPID[curOp.OT] = curOp.BID;

	}

	// Remove all disjunctive precedence constraints
	QList<QPair<int, int> > newOperationPrecedences;
	QList<bool> newOperationPrecedencesConj;
	for (int i = 0; i < operationPrecedences.size(); i++) {
		if (operationPrecedencesConj[i]) { // Copy only the conjunctive precedence constraint
			newOperationPrecedences << operationPrecedences[i];
			newOperationPrecedencesConj << operationPrecedencesConj[i];
		}
	}
	operationPrecedences = newOperationPrecedences;
	operationPrecedencesConj = newOperationPrecedencesConj;

	if (!valid()) {
		Debugger::err << "GAPlannerGenome::mutateBOM : Invalid genome before the mutation!" << ENDL;
	}

	// For each product select a new BOM ID

	foreach(const int& curProdID, prodIDsToChange) {

		//out << "GAPlannerGenome::mutateBOM : Product " << curProdID << " , old BOP : " << prodID2BOPID[curProdID] << endl;

		// Select some BOP randomly
		BillOfProcesses& curBOP = (BillOfProcesses&) *(pmm->prodman->productByID(curProdID).bops[Rand::rndInt(0, pmm->prodman->productByID(curProdID).bops.size() - 1)]);
		int curBOPID = curBOP.ID; // ID of the newly selected BOP

		//out << "GAPlannerGenome::mutateBOM : Product " << curProdID << " , new BOP : " << curBOPID << endl;

		// Randomly select the routes in the BOP.
		curBOP.setItemRouteIdxRnd();

		// Generate the BOP
		curBOP.generate();

		// Sort the nodes of the generated BOP topologically
		//*********** Topological ordering of the nodes in the BOP *************
		ListDigraph::NodeMap<int> nodes2pos(curBOP.graph()); // Map of nodes sorted topologically

		// Sort the nodes topologically
		topologicalSort(curBOP.graph(), nodes2pos);

		QMap<int, ListDigraph::Node> pos2nodes; // Topologically sorted nodes 

		for (ListDigraph::NodeMap<int>::MapIt mi(nodes2pos); mi != INVALID; ++mi) {
			pos2nodes[*mi] = mi;
		}

		QList<ListDigraph::Node> tnodes;

		tnodes.clear();
		tnodes.reserve(pos2nodes.size());
		for (QMap<int, ListDigraph::Node>::iterator sti = pos2nodes.begin(); sti != pos2nodes.end(); sti++) {
			tnodes << sti.value();
		}
		tnodes.removeOne(curBOP.graphHead());
		tnodes.removeOne(curBOP.graphTail());

		//**********************************************************************

		if (!valid()) {
			Debugger::err << "GAPlannerGenome::mutateBOM : Invalid genome before changing product/orders!" << ENDL;
		}

		// Get the number of operations in the old BOP, mark the operations
		int oldNumOper = 0;
		QSet<int> opIDPrecRem;
		for (int i = 0; i < operations.size(); i++) {

			Operation& curOp = (Operation&) operations[i];

			if (curOp.OT == curProdID) {
				oldNumOper++;

				opIDPrecRem << curOp.ID;

				curOp.ID = -1;
				curOp.IT = -1;
				curOp.IID = -1;
				curOp.RID = -1;
				curOp.type = -1;
				curOp.toolID = -1;
				curOp.machID = -1;
			}

		}
		if (oldNumOper % prodID2ordIDs[curProdID].size() != 0) {
			//out << *this << endl;
			Debugger::err << "GAPlannerGenome::mutateBOM : Invalid genome : Number of operations mismatch!" << ENDL;
		}
		oldNumOper /= prodID2ordIDs[curProdID].size(); // Since each order of the product has exactly the same number of operations

		// Remove the relevant precedence constraints for all operations corresponding to the product
		QList<QPair<int, int> > newOperationPrecedences;
		QList<bool> newOperationPrecedencesConj;
		for (int i = 0; i < operationPrecedences.size(); i++) {
			QPair<int, int> curPrec = operationPrecedences[i];
			if (!opIDPrecRem.contains(curPrec.first) && !opIDPrecRem.contains(curPrec.second)) {
				newOperationPrecedences << operationPrecedences[i];
				newOperationPrecedencesConj << operationPrecedencesConj[i];
			}
		}
		operationPrecedences = newOperationPrecedences;
		operationPrecedencesConj = newOperationPrecedencesConj;

		// Get the number of spare operations per order
		int numSpareOper = oldNumOper - tnodes.size();

		//out << "Changing orders ... " << endl;
		//out << "Should be operations per order : " << tnodes.size() << endl;
		//out << "Number of spare operations per order : " << numSpareOper << endl;

		foreach(const int& curOrdID, prodID2ordIDs[curProdID]) {

			// Get the operation indices for each order of the orders
			QHash<int, QList<int> > ordID2opIdcs;
			for (int i = 0; i < operations.size(); i++) {

				Operation& curOp = (Operation&) operations[i];

				ordID2opIdcs[curOp.OID] << i;

			}

			int tmpSpareOper = numSpareOper;

			// Check whether any operations should be deleted
			QHash<int, QSet<int> > ordID2opIdcsRem;
			while (tmpSpareOper > 0) {

				// Select randomly an operation position
				int curOpPosRem = Rand::rndInt(0, ordID2opIdcs[curOrdID].size() - 1);

				// Add the index for removal
				ordID2opIdcsRem[curOrdID] << ordID2opIdcs[curOrdID][curOpPosRem];

				// Remove the already selected index
				ordID2opIdcs[curOrdID].removeAt(curOpPosRem);

				tmpSpareOper--;
			}
			// Remove the operations with the selected indices
			QList<Operation> newOperations;
			for (int i = 0; i < operations.size(); i++) {
				if (!ordID2opIdcsRem[curOrdID].contains(i)) {
					newOperations << operations[i];
				}
			}
			operations = newOperations;


			// Check whether any operations corresponding to the order should be added
			while (tmpSpareOper < 0) {

				//out << " Inserting additional operations... " << endl;

				Operation newOper;

				newOper.ID = -1;
				newOper.OID = curOrdID;
				newOper.OT = curProdID;
				newOper.IT = -1;
				newOper.IID = -1;
				newOper.RID = -1;
				newOper.type = -1;
				newOper.toolID = -1;
				newOper.machID = -1;

				int insPos = Rand::rndInt(0, operations.size());
				operations.insert(insPos, newOper);

				tmpSpareOper++;

				//out << " Done inserting additional operations. " << endl;
			}

			// Insert the generated BOP (TOPOLOGICALLY ORDERED!!!)
			// Get the operations' positions for the current order
			QList<int> opPos;
			for (int i = 0; i < operations.size(); i++) {
				Operation& curOp = (Operation&) operations[i];
				if (curOp.OID == curOrdID) {
					opPos << i;
				}
			}

			// Iterate over the topologically sorted nodes of the BOP
			//out << "Inserting BOP ... " << endl;
			QMap<ListDigraph::Node, int> bopNode2OpIdx;
			for (int i = 0; i < tnodes.size(); i++) {

				//out << "test... " << endl;
				int curPos = opPos.takeFirst();
				//out << "Done test. " << endl;
				ListDigraph::Node curNode = tnodes[i];
				Operation& curOp = (Operation&) operations[curPos];

				curOp.ID = (curOrdID << 16) + i + 1; // ID of the operations depending on the order's ID

				curOp.BID = curBOPID;

				curOp.IID = curBOP.oLocItemID()[curNode]; // Temporary

				curOp.IT = curBOP.itemID2ItemType()[curOp.IID];

				curOp.RID = curBOP.itemRoutes()[curOp.IID][curBOP.itemRouteIdx()[curOp.IID]]->ID;

				// Correct the item ID
				curOp.IID = (curOrdID << 16) + curOp.IID;

				curOp.SI = -1;

				curOp.type = curBOP.otypeID()[curNode];

				rc.assign(&curOp); // Assign to the machine group -> sets the toolID

				// Select the machine ID randomly
				curOp.machID = rc(curOp.toolID).machines(curOp.type)[Rand::rndInt(0, rc(curOp.toolID).machines(curOp.type).size() - 1)]->ID;

				// Update the processing time of the operation
				curOp.p(rc(curOp.toolID, curOp.machID).procTime(&curOp));

				bopNode2OpIdx[curNode] = curPos;

			}
			//out << "Done inserting BOP. " << endl;

			// Collect the precedence constraints between the operations
			for (ListDigraph::ArcIt ait(curBOP.graph()); ait != INVALID; ++ait) {

				ListDigraph::Node startNode = curBOP.graph().source(ait);
				ListDigraph::Node endNode = curBOP.graph().target(ait);

				if (startNode == curBOP.graphHead() || endNode == curBOP.graphTail()) {
					continue;
				} else {
					QPair<int, int> curPrec;

					curPrec.first = operations[bopNode2OpIdx[startNode]].ID;

					curPrec.second = operations[bopNode2OpIdx[endNode]].ID;

					operationPrecedences << curPrec;
					operationPrecedencesConj << true;
				}

			}


		} // Iterating over the order IDs


		if (!valid()) {
			Debugger::err << "GAPlannerGenome::mutateBOM : Invalid genome after changing orders!" << ENDL;
		}

		//out << "Done changing orders." << endl;

		// Degenerate the BOP
		curBOP.degenerate();
	} // Iterating over the products

	// Restore the disjunctive precedences between the operations
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		int curToolId = curOp.toolID;
		int curMachID = curOp.machID;

		for (int j = i + 1; j < operations.size(); j++) {

			Operation& nextOp = operations[j];

			if (nextOp.toolID == curToolId && nextOp.machID == curMachID) { // This is a disjunctive precedence constraint
				operationPrecedences << QPair<int, int> (curOp.ID, nextOp.ID);
				operationPrecedencesConj << false;
				break;
			}

		} // Next operation

	} // Restoring the disjunctive precedence constraints

	// Reconstruct the PM based on the current genome's representation
	ProcessModel restoredPM = restorePM();

	// Get the schedule based on the restored PM
	schedule.fromPM(restoredPM, *obj);

	//out << "After the BOM mutator : " << endl << *this << endl;

	//out << restoredPM << endl;

	//out << " --------- " << prevObj << endl;
	//out << " -------- " << TWT()(restoredPM) << endl;
	//out << " --------- " << schedule.objective << endl;

	//getchar();

}

int GAPlannerGenome::mut(const float prob) {

	// Decide whether to mutate
	if (Rand::rndDouble() > prob) {
		return 0;
	}

	//for (int i = 0; i < operations.size(); i++) {

	//	Operation& curOp = (Operation&) operations[i];

	//	if (curOp.ID <= 0) {
	//		Debugger::err << "GAPlannerGenome::mut : Invalid operation before mutating!!!" << ENDL;
	//	}

	//}

	// Select a mutation approach
	double p = Rand::rndDouble();

	if (p < 0.5) { // Mutate BOM
		mutateBOM(prob); // The mutator performs reparation as well

		//for (int i = 0; i < operations.size(); i++) {

		//	Operation& curOp = (Operation&) operations[i];

		//	if (curOp.ID <= 0) {
		//		Debugger::err << "GAPlannerGenome::mut : Invalid operation after mutating!!!" << ENDL;
		//	}

		//}

	} else if (p < 0.7) { // Mutate some route
		mutateRoute(prob); // The mutator performs reparation as well

	} else { // Mutate some machine
		mutateMach(prob); // The mutator performs reparation as well

	}

	this->evaluate(gaTrue);

	return 0;

}

int GAPlannerGenome::mutate(GAGenome& gen, float prob) {
	GAPlannerGenome& g = (GAPlannerGenome&) gen;
	int res = g.mut(prob);

	// Important !!! Tell GALib to reevaluate the genome in order not to use the cached  value
	g.evaluate(gaTrue);

	return res;
}

void GAPlannerGenome::repair(const GAPlannerGenome& mom, const GAPlannerGenome& dad) {

	QTextStream out(stdout);

	//out << "GAPlannerGenome::repair : Repairing the child genome ... " << endl;

	//out << "Mom : " << mom << endl;

	//out << "Dad : " << dad << endl;

	//out << " Child : " << *this << endl;

	// Iterate over the crossing section and collect all products which are in it
	QSet<int> prodIDsToRepair;
	for (int i = 0; i < crossSection.size(); i++) {

		Operation& curOp = (Operation&) crossSection[i];

		prodIDsToRepair << curOp.OT;

	}

	//out << "Products to be considered : " << endl;

	//foreach(const int& curProdID, prodIDsToRepair) {
	//	out << curProdID << ",";
	//}
	//out << endl;


	// Iterate over all operations and collect the orders to be repaired
	QSet<int> ordIDsToRepair;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		if (prodIDsToRepair.contains(curOp.OT)) {
			ordIDsToRepair << curOp.OID;

			// Mark the operation for consideration
			curOp.ID = -1;

		}

	}

	//out << "Orders to be considered : " << endl;

	//foreach(const int& curOrdID, ordIDsToRepair) {
	//	out << curOrdID << ",";
	//}
	//out << endl;

	// Collect the inherited sequence of machines
	QHash<int, QList<int> > ordID2machIDSeq;
	QHash<int, QList<int> > ordID2toolIDSeq;

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ordID2machIDSeq[curOp.OID] << curOp.machID;
		ordID2toolIDSeq[curOp.OID] << curOp.toolID;

	}

	// Collect inherited BOMs for different products 
	QHash<int, QSet<int> > prodID2bomIDs; // IDs of the BOMs for each product. The ones which contain > 1 product must be considered
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2bomIDs[curOp.OT].insert(curOp.BID);

	}

	// For each product to be repaired, calculate the number of operations in the mom's/dad's genome per order of this product
	QHash<int, int> prodID2numOpsPerOrdMom;
	QHash<int, int> prodID2numOpsPerOrdDad;

	// The number of operations per product in mom's genome
	for (int i = 0; i < mom.operations.size(); i++) {

		Operation& curOp = (Operation&) mom.operations[i];

		prodID2numOpsPerOrdMom[curOp.OT]++;

	}
	for (int i = 0; i < dad.operations.size(); i++) {

		Operation& curOp = (Operation&) dad.operations[i];

		prodID2numOpsPerOrdDad[curOp.OT]++;

	}

	for (int i = 0; i < pmm->prodman->products.size(); i++) {

		int curProdID = pmm->prodman->products[i]->ID;

		int curOrdsOfProd = pmm->ordman->ordersByType(curProdID).size();

		prodID2numOpsPerOrdMom[curProdID] /= curOrdsOfProd;
		prodID2numOpsPerOrdDad[curProdID] /= curOrdsOfProd;

	}

	QHash<int, int> ordID2NumSpareOps; // Number of spare operations for each order (a negative value means that some operations must be added)

	// For each product to be changed select only one of the alternative BOMs

	foreach(const int& curProdID, prodIDsToRepair) {

		if (prodID2bomIDs[curProdID].size() > 1) { // There are two BOMs for selection

			//out << "Product : " << curProdID << " - BOMs : " << prodID2bomIDs[curProdID].toList()[0] << " , " << prodID2bomIDs[curProdID].toList()[1] << endl;

			Product& curProd = (Product&) pmm->prodman->productByID(curProdID);

			int momBOMID = mom.plan.prodID2BOPID[curProdID];
			int dadBOMID = dad.plan.prodID2BOPID[curProdID];

			//out << "Mom's BOM : " << momBOMID << endl;
			//out << "Dad's BOM : " << dadBOMID << endl;

			// Get the number of operations in the current product 
			//out << "Operations of the product in mom's BOP (per order) : " << prodID2numOpsPerOrdMom[curProdID] << endl;

			//out << "Operations of the product in dad's BOP (per order) : " << prodID2numOpsPerOrdDad[curProdID] << endl;

			int childBOMID = -1;

			int numOpsShouldBe = -1; // The number of operations which should be present in each order of the product

			if (Rand::rndDouble() < 0.5) {
				childBOMID = momBOMID;
				numOpsShouldBe = prodID2numOpsPerOrdMom[curProdID];
			} else {
				childBOMID = dadBOMID;
				numOpsShouldBe = prodID2numOpsPerOrdDad[curProdID];
			}

			//out << "Selected BOM " << childBOMID << " for product " << curProdID << endl;
			//out << "Should be operations per order of product " << curProdID << " : " << numOpsShouldBe << endl;

			// Set the selected BOM
			prodID2bomIDs[curProdID].clear();
			prodID2bomIDs[curProdID] << childBOMID;

			// For each order of the product calculate the number of spare/missing parts
			QList<Order*> prodOrders = pmm->ordman->ordersByType(curProdID);

			for (int i = 0; i < prodOrders.size(); i++) {

				int curOrdID = prodOrders[i]->ID;

				// Get the number of operations for the order
				int curOrdOpNum = 0;
				for (int j = 0; j < operations.size(); j++) {
					if (operations[j].OID == curOrdID) {
						curOrdOpNum++;
					}
				}

				ordID2NumSpareOps[curOrdID] = curOrdOpNum - numOpsShouldBe;

				//out << "Spare parts for order " << curOrdID << " : " << ordID2NumSpareOps[curOrdID] << endl;

			}

		} else { // The BOM is the same

			Product& curProd = (Product&) pmm->prodman->productByID(curProdID);

			int momBOMID = mom.plan.prodID2BOPID[curProdID];

			//out << "Mom's/Dad's BOM : " << momBOMID << endl;

			// Get the number of operations in the current product 
			//out << "Operations of the product in mom's BOP (per order) : " << prodID2numOpsPerOrdMom[curProdID] << endl;

			int numOpsShouldBe = prodID2numOpsPerOrdMom[curProdID]; // The number of operations which should be present in each order of the product

			//out << "Should be operations per order of product " << curProdID << " : " << numOpsShouldBe << endl;

			// For each order of the product calculate the number of spare/missing parts
			QList<Order*> prodOrders = pmm->ordman->ordersByType(curProdID);

			for (int i = 0; i < prodOrders.size(); i++) {

				int curOrdID = prodOrders[i]->ID;

				// Get the number of operations for the order
				int curOrdOpNum = 0;
				for (int j = 0; j < operations.size(); j++) {
					if (operations[j].OID == curOrdID) {
						curOrdOpNum++;
					}
				}

				ordID2NumSpareOps[curOrdID] = curOrdOpNum - numOpsShouldBe;

				//out << "Spare parts for order " << curOrdID << " : " << ordID2NumSpareOps[curOrdID] << endl;

			}

		}

	} // Affected products

	// Randomly remove the spare operation in each order to be modified

	foreach(const int& curOrdID, ordIDsToRepair) {

		if (ordID2NumSpareOps[curOrdID] > 0) { // Removing spare operations

			QList<int> curOrdOperPos;
			for (int i = 0; i < operations.size(); i++) {
				if (operations[i].OID == curOrdID) {
					curOrdOperPos << i;
				}
			}

			QSet<int> curOrdOperPosRem;
			int ctr = ordID2NumSpareOps[curOrdID];
			while (ctr > 0) { // There are spare parts which should be removed
				int curIdx = Rand::rndInt(0, curOrdOperPos.size() - 1);

				curOrdOperPosRem << curOrdOperPos[curIdx];

				curOrdOperPos.removeAt(curIdx);

				// Decrease the number of spare operations in the order
				ctr--;
			}

			QList<Operation> newOperations;

			for (int i = 0; i < operations.size(); i++) {
				if (curOrdOperPosRem.contains(i)) {
					continue;
				} else {
					newOperations << operations[i];
				}
			}

			operations = newOperations;

			ordID2NumSpareOps[curOrdID] = 0;
		}

		if (ordID2NumSpareOps[curOrdID] < 0) { // Adding missing operations
			//Debugger::err << "GAPlannerGenome::repair : Adding missing operations is not implemented yet!!!" << ENDL;

			while (ordID2NumSpareOps[curOrdID] < 0) {

				int insPos = Rand::rndInt(0, operations.size() - 1);

				Operation insOp;

				insOp.ID = -1;

				insOp.OID = curOrdID;

				insOp.OT = pmm->ordman->orderByID(curOrdID).type;

				insOp.type = -1;

				insOp.IID = -1;
				insOp.IT = -1;

				insOp.RID = -1;
				insOp.SI = -1;

				insOp.toolID = -1;
				insOp.machID = -1;

				operations.insert(insPos, insOp);

				ordID2NumSpareOps[curOrdID]++;
			}

			ordID2NumSpareOps[curOrdID] = 0;
		}

	}

	// ########## For each order generate a BOP and insert it ##############

	QList<QPair<int, int> > childPrec; // Precedence relations between the operations of the child genome

	QSet<int> prodIDBOPGen; // Products with generated BOPs (to avoid generation for each order of the product)

	foreach(const int& curOrdID, ordIDsToRepair) {

		Order& curOrd = pmm->ordman->orderByID(curOrdID);

		Product& curProd = pmm->prodman->productByID(curOrd.type);

		int curBOMID = prodID2bomIDs[curProd.ID].toList()[0]; // There is only one BOM selected right now

		BillOfProcesses& curBOP = (BillOfProcesses&) * curProd.bopByID(curBOMID);

		if (!prodIDBOPGen.contains(curBOP.ID)) { // To avoid different routes in pars belonging to different orders of the same product

			// Set the routes in the BOP
			curBOP.setItemRouteIdxRnd();

			prodIDBOPGen << curBOP.ID;
		}

		// Generate the current BOP
		curBOP.generate();

		//out << "Current BOP : " << endl << curBOP << endl;

		//*********** Topological ordering of the nodes in the BOP *************
		ListDigraph::NodeMap<int> nodes2pos(curBOP.graph()); // Map of nodes sorted topologically

		// Sort the nodes topologically
		topologicalSort(curBOP.graph(), nodes2pos);

		QMap<int, ListDigraph::Node> pos2nodes; // Topologically sorted nodes 

		for (ListDigraph::NodeMap<int>::MapIt mi(nodes2pos); mi != INVALID; ++mi) {
			pos2nodes[*mi] = mi;
		}

		QList<ListDigraph::Node> tnodes;

		tnodes.clear();
		tnodes.reserve(pos2nodes.size());
		for (QMap<int, ListDigraph::Node>::iterator sti = pos2nodes.begin(); sti != pos2nodes.end(); sti++) {
			tnodes << sti.value();
		}
		tnodes.removeOne(curBOP.graphHead());
		tnodes.removeOne(curBOP.graphTail());

		//**********************************************************************


		// Get the operations' positions for the current order
		QList<int> opPos;
		for (int i = 0; i < operations.size(); i++) {
			Operation& curOp = (Operation&) operations[i];
			if (curOp.OID == curOrdID) {
				opPos << i;
			}
		}

		// Iterate over the topologically sorted nodes of the BOP
		QMap<ListDigraph::Node, int> bopNode2OpIdx;
		for (int i = 0; i < tnodes.size(); i++) {
			int curPos = opPos.takeFirst();
			ListDigraph::Node curNode = tnodes[i];
			Operation& curOp = (Operation&) operations[curPos];

			curOp.ID = (curOrdID << 16) + i + 1; // ID of the operations depending on the order's ID

			curOp.BID = curBOMID;

			curOp.IID = curBOP.oLocItemID()[curNode];

			curOp.IT = curBOP.itemID2ItemType()[curOp.IID];

			curOp.RID = curBOP.itemRoutes()[curOp.IID][curBOP.itemRouteIdx()[curOp.IID]]->ID;

			// Correct the item ID
			curOp.IID = (curOrdID << 16) + curOp.IID;

			curOp.SI = -1;

			curOp.type = curBOP.otypeID()[curNode];

			rc.assign(&curOp); // Assign to the machine group -> sets the toolID





			bool randomMachSelect = false;





			if (randomMachSelect) {
				// Select the machine ID randomly
				curOp.machID = rc(curOp.toolID).machines(curOp.type)[Rand::rndInt(0, rc(curOp.toolID).machines(curOp.type).size() - 1)]->ID;
				//curOp.machID = -1;
			} else {
				// Select the machine ID inherited
				QList<int>& curToolIDSeq = ordID2toolIDSeq[curOrdID];
				QList<int>& curMachIDSeq = ordID2machIDSeq[curOrdID];

				int machIdx = -1;
				for (int k = 0; k < curToolIDSeq.size(); k++) {
					if (curToolIDSeq[k] == curOp.toolID) {
						machIdx = k;
						break;
					}
				}

				if (machIdx == -1) { // Nothing found
					curOp.machID = rc(curOp.toolID).machines(curOp.type)[Rand::rndInt(0, rc(curOp.toolID).machines(curOp.type).size() - 1)]->ID;
				} else {
					// IMPORTANT!!! If there are machines with different functionality in one machine group => there is a problem!!!
					curOp.machID = curMachIDSeq[machIdx];

					curMachIDSeq.removeAt(machIdx);
					curToolIDSeq.removeAt(machIdx);
				}
			}





			bopNode2OpIdx[curNode] = curPos;

		}

		// Collect the precedence constraints between the operations
		for (ListDigraph::ArcIt ait(curBOP.graph()); ait != INVALID; ++ait) {

			ListDigraph::Node startNode = curBOP.graph().source(ait);
			ListDigraph::Node endNode = curBOP.graph().target(ait);

			if (startNode == curBOP.graphHead() || endNode == curBOP.graphTail()) {
				continue;
			} else {
				QPair<int, int> curPrec;

				curPrec.first = operations[bopNode2OpIdx[startNode]].ID;

				curPrec.second = operations[bopNode2OpIdx[endNode]].ID;

				childPrec << curPrec;
			}

		}

		// Degenerate the current BOP
		curBOP.degenerate();
	}
	// #####################################################################

	// ########## Collect the precedence constraints ###########################

	// Collect the precedence constraint from the mom's genome (ONLY FOR THE ORDERS WHICH MUST NOT BE CHANGED)
	QList<QPair<int, int> > momPrec;
	QHash<int, int> momOpID2OrdID;
	for (int i = 0; i < mom.operations.size(); i++) {

		Operation& curOp = (Operation&) mom.operations[i];

		momOpID2OrdID[curOp.ID] = curOp.OID;

	}

	// IMPORTANT!!! No operation precedences are inherited from the dad, since all of the corresponding orders are recreated

	for (int i = 0; i < mom.schedule.operationPrecedences.size(); i++) {
		if (mom.schedule.operationPrecedencesConj[i]) {

			int opIDStart = mom.schedule.operationPrecedences[i].first;
			int opIDEnd = mom.schedule.operationPrecedences[i].second;

			// Check whether the operations belong to a repaired order, or not
			if (ordIDsToRepair.contains(momOpID2OrdID[opIDStart]) || ordIDsToRepair.contains(momOpID2OrdID[opIDEnd])) { // These operations are removed in the child genome
				continue;
			} else { // These operations are inherited by the child and are not touched
				momPrec << mom.schedule.operationPrecedences[i];
			}
		}
	}

	/*
	out << "Precedence constraints in the mom's genome : " << endl;
	for (int i = 0; i < momPrec.size(); i++) {
		out << momPrec[i].first << "->" << momPrec[i].second << endl;
	}
	 */

	// Merge all precedence constraints
	childPrec << momPrec;

	/*
	out << "Precedence constraints in the child's genome : " << endl;
	for (int i = 0; i < childPrec.size(); i++) {
		out << childPrec[i].first << "->" << childPrec[i].second << endl;
	}
	 */

	// #########################################################################

	// ### Perform scheduling under consideration of precedences ###############

	QHash<int, QList<int> > opID2SuccOpIDs; // Direct successors of the current operation
	QHash<int, QList<int> > opID2PredOpIDs; // Direct successors of the current operation
	for (int i = 0; i < childPrec.size(); i++) {
		int startOpID = childPrec[i].first;
		int endOpID = childPrec[i].second;

		opID2SuccOpIDs[startOpID] << endOpID;
		opID2PredOpIDs[endOpID] << startOpID;
	}

	QHash<int, double> opID2EST; // The earliest start time of each operation (the operations are sorted topologically)

	for (int i = 0; i < operations.size(); i++) {
		opID2EST[operations[i].ID] = 0.0;
	}

	rc.init();

	// Schedule the operations
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		// Set the earliest start time of the operations
		curOp.r(opID2EST[curOp.ID]);

		if (false/*curOp.machID == -1*/) {

			// Select the machine able to finish the operation the earliest
			Machine& m = rc(curOp.toolID).earliestToFinish(&curOp);

			m << &curOp;

		} else {

			// Set the operation's machine
			Machine& m = rc(curOp.toolID, curOp.machID);

			m << &curOp;

		}


		// Update the earliest start time of the direct successors
		QList<int> curSuccIDs = opID2SuccOpIDs[curOp.ID];

		for (int j = 0; j < curSuccIDs.size(); j++) {
			int curSuccID = curSuccIDs[j];

			opID2EST[curSuccID] = Math::max(opID2EST[curSuccID], curOp.c());
		}

	}

	// #########################################################################

	// #################### Calculate the objective ############################

	double curObj = 0.0;

	QList<Order>& orders = (QList<Order>&) pmm->ordman->orders;

	QHash<int, Operation> ordId2LastOper;

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ordId2LastOper[curOp.OID] = curOp;
	}

	for (int i = 0; i < orders.size(); i++) {

		Order& curOrd = (Order&) orders[i];

		double curOrdC = ordId2LastOper[curOrd.ID].c();
		double curOrdD = curOrd.d;
		double curOrdW = curOrd.w;

		if (schedOptions["OBJECTIVE"] == "Cmax") {

			curObj = Math::max(curObj, curOrdC); //*/ += curOrdW * Math::max(0.0, curOrdC - curOrdD);

		} else if (schedOptions["OBJECTIVE"] == "TWT") {

			curObj += curOrdW * Math::max(0.0, curOrdC - curOrdD);

		}

	}

	// #########################################################################

	// ###################### Build the schedule ###############################

	schedule.clear();

	schedule.objective = curObj;

	schedule.operations = operations;

	schedule.operationPrecedences = childPrec;

	schedule.operationPrecedencesConj.clear();
	for (int i = 0; i < schedule.operationPrecedences.size(); i++) {
		schedule.operationPrecedencesConj << true;
	}

	// Add the other operations precedences
	QList<Machine*> machs = rc.machines();

	for (int i = 0; i < machs.size(); i++) {

		Machine& curMach = (Machine&) *(machs[i]);

		for (int j = 0; j < curMach.operations.size() - 1; j++) {

			int startOpID = curMach.operations[j]->ID;
			int endOpID = curMach.operations[j + 1]->ID;

			schedule.operationPrecedences << QPair<int, int>(startOpID, endOpID);

			schedule.operationPrecedencesConj << false;

		}

	}

	operationPrecedences = schedule.operationPrecedences;
	operationPrecedencesConj = schedule.operationPrecedencesConj;

	/*
	out << "Operation precedences before restoring from PM : " << endl;
	for (int i = 0; i < schedule.operationPrecedences.size(); i++) {
		QPair<int, int> curPrec = schedule.operationPrecedences[i];
		out << curPrec.first << "->" << curPrec.second << " : " << schedule.operationPrecedencesConj[i] << endl;
	}
	 */

	ProcessModel restoredPM = restorePM();

	//out << "Restored PM : " << endl << restoredPM << endl;

#ifdef DEBUG

	int numDisjPrecConstr = 0;
	for (int i = 0; i < operationPrecedences.size(); i++) {
		if (operationPrecedencesConj[i]) {
			numDisjPrecConstr++;
		}
	}
	if (numDisjPrecConstr == 0) {
		Debugger::err << "GAPlannerGenome::repair : No disjunctive precedence constraints!" << ENDL;
	}

	// out << schedule.pm << endl;

	if (Math::cmp(schedule.objective, (*obj)(restoredPM), 0.0000001) != 0) {

		out << "Schedule objective : " << schedule.objective << endl;
		out << "PM objective : " << (*obj)(restoredPM) << endl;

		Debugger::err << "GAPlannerGenome::repair : Schedule/PM objective mismatch!" << ENDL;
	}
#endif

	schedule.fromPM(restoredPM, *obj);

	//out << "Objective after repairing : " << schedule.objective << endl;
	//getchar();

	/*
	out << "Operation precedences after restoring from PM : " << endl;
	for (int i = 0; i < schedule.operationPrecedences.size(); i++) {
		QPair<int, int> curPrec = schedule.operationPrecedences[i];
		out << curPrec.first << "->" << curPrec.second << " : " << schedule.operationPrecedencesConj[i] << endl;
	}
	 */

	// #########################################################################

	// ###################### Build the plan ###################################

	plan.clear();

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		plan.prodID2BOPID[curOp.OT] = curOp.BID;

		plan.prodID2ItemID2ItemType[curOp.OT][curOp.IID] = curOp.IT;

		plan.prodID2ItemID2RouteID[curOp.OT][curOp.IID] = curOp.RID;

		plan.prodID2ItemID2RouteIdx[curOp.OT][curOp.IID] = pmm->prodman->productByID(curOp.OT).bopByID(curOp.BID)->itemRouteIdx()[curOp.IID];

	}

	// #########################################################################

	//out << "Calculated objective : " << curObj << endl;

	if (!valid()) {
		Debugger::err << "GAPlannerGenome::repair : Invalid genome after repairing!" << ENDL;
	}

}

float GAPlannerGenome::compare(const GAGenome &other) {
	return 0.0;
}

float GAPlannerGenome::compare(const GAGenome& g1, const GAGenome& g2) {
	return g1.compare(g2);
}

float GAPlannerGenome::objective(GAGenome & gen) {
	//cout << "Genome:: calculating objective" << endl;
	GAPlannerGenome& g = (GAPlannerGenome&) gen;

	// Improve the genome
	if (g._solImprove) {
		g.improve();
	}

	return g.schedule.objective;
}

void GAPlannerGenome::improve() {

	if (Rand::rndDouble() > 0.05) { // Only with some probability improve the solutions
		return;
	}

	QTextStream out(stdout);

	LocalSearchPM ls;

	//if (this->geneticAlgorithm()->generation() == this->geneticAlgorithm()->nGenerations() - 1) {
	//	ls.maxIter(schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"].toInt());
	//} else {
	ls.maxIter(_lsMaxIter);
	//}

	ls.checkCorrectness(_lsChkCor);

	ProcessModel pm = schedule.pm;
	rc.init();

	//out << "PM before improving : " << endl << pm << endl;

	//out << "GAPlannerGenome::improve : TWT before improving : " << TWT()(pm) << endl;
	ls.setObjective(obj);
	ls.setPM(&pm);
	ls.setResources(&rc);

	//out << "Running the local search reoptimization ..." << endl;
	if (ls.maxIter() > 0) {
		ls.run();
	}

	//out << "GAPlannerGenome::improve : TWT after improving : " << TWT()(pm) << endl;

	schedule.fromPM(pm, *obj);

	// Set the state of the genome according to the improved schedule
	operations = schedule.operations;
	operationPrecedences = schedule.operationPrecedences;
	operationPrecedencesConj = schedule.operationPrecedencesConj;

}

void GAPlannerGenome::setPMM(ProcessModelManager* pmm) {
	this->pmm = pmm;
}

void GAPlannerGenome::setResources(Resources& origRes) {
	this->rc = origRes;
}

void GAPlannerGenome::setScheduler(Scheduler* scheduler) {

	this->scheduler = scheduler;

}

void GAPlannerGenome::setOptions(SchedulerOptions& options) {

	this->schedOptions = options;

	_lsMaxIter = schedOptions["LS_MAX_ITER"].toInt();
	_lsChkCor = (schedOptions["LS_CHK_COR"] == "true");
	_solImprove = (schedOptions["GA_IMPROVE_SOLUTION"] == "true");

}

ProcessModel GAPlannerGenome::restorePM() {
	ProcessModel pm;

	pm.head = pm.graph.addNode();
	pm.ops[pm.head] = new Operation();
	pm.ops[pm.head]->type = 0;
	pm.ops[pm.head]->ID = -10000002;
	pm.ops[pm.head]->p(0.0);

	pm.tail = pm.graph.addNode();
	pm.ops[pm.tail] = new Operation();
	pm.ops[pm.tail]->type = 0;
	pm.ops[pm.tail]->ID = -10000001;
	pm.ops[pm.tail]->p(0.0);

	QHash<int, QList<Operation> > ordID2Operaions;

	QList<Order>& orders = pmm->ordman->orders;

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ordID2Operaions[curOp.OID] << curOp;

	}

	QHash<int, ListDigraph::Node> opID2Node;

	QHash<int, ListDigraph::Node> ordID2LHead;
	QHash<int, ListDigraph::Node> ordID2LTail;

	for (int i = 0; i < orders.size(); i++) {

		int curOrdID = orders[i].ID;

		int maxOrdOpCount = 0;

		for (int j = 0; j < ordID2Operaions[curOrdID].size(); j++) {

			ListDigraph::Node curNode = pm.graph.addNode();

			Operation& curOp = (Operation&) ordID2Operaions[curOrdID][j];

			opID2Node[curOp.ID] = curNode;

			pm.ops[curNode] = new Operation(curOp);

			maxOrdOpCount = Math::max(maxOrdOpCount, curOp.ID - (curOrdID << 16));

		}

		// Create two fake nodes for the order
		ListDigraph::Node lHead = pm.graph.addNode();
		pm.ops[lHead] = new Operation();
		pm.ops[lHead]->type = 0;
		pm.ops[lHead]->ID = -((curOrdID << 16) + maxOrdOpCount + 1);
		pm.ops[lHead]->OID = curOrdID;
		pm.ops[lHead]->toolID = 0;
		pm.ops[lHead]->machID = 0;
		pm.ops[lHead]->p(0.0);
		pm.ops[lHead]->w(orders[i].w);
		pm.ops[lHead]->d(orders[i].d);
		pm.ops[lHead]->ir(orders[i].r);
		ListDigraph::Node lTail = pm.graph.addNode();
		pm.ops[lTail] = new Operation();
		pm.ops[lTail]->type = 0;
		pm.ops[lTail]->ID = -((curOrdID << 16) + maxOrdOpCount + 2);
		pm.ops[lTail]->OID = curOrdID;
		pm.ops[lTail]->toolID = 0;
		pm.ops[lTail]->machID = 0;
		pm.ops[lTail]->p(0.0);
		pm.ops[lTail]->w(orders[i].w);
		pm.ops[lTail]->d(orders[i].d);
		pm.ops[lTail]->ir(orders[i].r);

		ListDigraph::Arc ordStartArc = pm.graph.addArc(pm.head, lHead);
		ListDigraph::Arc ordEndArc = pm.graph.addArc(lTail, pm.tail);

		pm.conjunctive[ordStartArc] = true;
		pm.conjunctive[ordEndArc] = true;

		ordID2LHead[curOrdID] = lHead;
		ordID2LTail[curOrdID] = lTail;

	}

	// Add the conjunctive precedence constraints between the operations to the graph
	for (int i = 0; i < operationPrecedences.size(); i++) {

		QPair<int, int> curPrec = operationPrecedences[i];
		bool curPrecConj = operationPrecedencesConj[i];

		if (curPrecConj) {

			ListDigraph::Node startNode = opID2Node[curPrec.first];
			ListDigraph::Node endNode = opID2Node[curPrec.second];

			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = true;
		}

	}

	// Add the connecting arcs between order head/tail nodes and the others
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ListDigraph::Node curNode = opID2Node[curOp.ID];

		if (countInArcs(pm.graph, curNode) == 0 && curNode != ordID2LTail[curOp.OID]) { // Connect with the fake head
			ListDigraph::Node startNode = ordID2LHead[curOp.OID];
			ListDigraph::Node endNode = curNode;
			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = true;
		}

		if (countOutArcs(pm.graph, curNode) == 0 && curNode != ordID2LHead[curOp.OID]) { // Connect with the fake tail	
			ListDigraph::Node startNode = curNode;
			ListDigraph::Node endNode = ordID2LTail[curOp.OID];
			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = true;
		}

	}

	// Add the disjunctive precedence constraints between the operations to the graph
	for (int i = 0; i < operationPrecedences.size(); i++) {

		QPair<int, int> curPrec = operationPrecedences[i];
		bool curPrecConj = operationPrecedencesConj[i];

		if (!curPrecConj) {

			ListDigraph::Node startNode = opID2Node[curPrec.first];
			ListDigraph::Node endNode = opID2Node[curPrec.second];

			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = false;
		}

	}

	// Set correctly the lengths of the arcs
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {

		ListDigraph::Node startNode = pm.graph.source(ait);

		pm.p[ait] = -pm.ops[startNode]->p();

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	return pm;
}

void GAPlannerGenome::setObjective(ScalarObjective* o) {
	this->obj = o;
	//schedule << o;
}

/** ######################################################################### */

/** ###########################  GAPlannerGenome1  ########################### */

GAPlannerGenome1::GAPlannerGenome1() : obj(NULL) {
	initializer(init);
	mutator(mutate);
	comparator(compare);
	crossover(cross);
	evaluator(objective);

	pmm = NULL;
	scheduler = NULL;
	rc.clear();
	operations.clear();
	plan.clear();
	schedule.clear();
}

GAPlannerGenome1::GAPlannerGenome1(const GAPlannerGenome1& orig) : GAGenome(orig) {

	this->copy(orig);

}

GAPlannerGenome1::~GAPlannerGenome1() {

}

void GAPlannerGenome1::copy(const GAGenome& orig) {

	GAGenome::copy(orig);

	QTextStream out(stdout);

	//out << "Copying..." << endl;

	GAPlannerGenome1 &g = (GAPlannerGenome1&) orig;

	pmm = g.pmm;

	scheduler = g.scheduler;

	rc = g.rc;

	operations = g.operations;
	operationPrecedences = g.operationPrecedences;
	operationPrecedencesConj = g.operationPrecedencesConj;

	schedOptions = g.schedOptions;

	plan = g.plan;

	schedule = g.schedule;

	crossOrdID2OpIdx = g.crossOrdID2OpIdx;

	crossSection = g.crossSection;
	crossStart = g.crossStart;
	crossEnd = g.crossEnd;

	_lsMaxIter = g._lsMaxIter;
	_lsChkCor = g._lsChkCor;
	_solImprove = g._solImprove;

	obj = g.obj;

	//out << "Done copying." << endl;

}

GAGenome & GAPlannerGenome1::operator=(const GAGenome &orig) {

	QTextStream out(stdout);

	if (&orig != this) this->copy((GAPlannerGenome1&) orig);

	return *this;
}

GAGenome* GAPlannerGenome1::clone(CloneMethod) const {
	//cout<<"Genome:: Cloning"<<endl;
	return new GAPlannerGenome1(*this);
}

int GAPlannerGenome1::write(ostream &) const {
	// TODO
}

QTextStream& operator<<(QTextStream& out, const GAPlannerGenome1& g) {

	out << "Genome : " << endl;
	out << "[" << endl;

	// Write the operations
	for (int i = 0; i < g.operations.size(); i++) {
		Operation& curOp = (Operation&) g.operations[i];

		out << curOp << endl;
	}

	out << "Objective : " << g.score() << endl;

	out << "]" << endl;


	return out;

}

bool GAPlannerGenome1::valid() {
	//return true;

	QTextStream out(stdout);

	// Iterate over the operations and cound the number of operations for each order. Then check whether each order has the same number of operations
	QHash<int, QSet<int> > prodID2opIDs;
	QHash<int, QSet<int> > prodID2ordIDs;
	QSet<int> prodIDs;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2opIDs[curOp.OT] << curOp.ID;

		prodID2ordIDs[curOp.OT] << curOp.OID;

		prodIDs << curOp.OT;

	}

	foreach(const int& curProdID, prodIDs) {
		if (prodID2opIDs[curProdID].size() % prodID2ordIDs[curProdID].size() != 0) {
			out << "GAPlannerGenome1::valid : Wrong number of operations in the orders!!!" << endl;
			return false;
		}
	}

	// Check whether there are operations with negative IDs
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		if (curOp.ID <= 0) {
			out << "GAPlannerGenome1::valid : Invalid operation!!!" << ENDL;
			return false;
		}

	}

	return true;
}

void GAPlannerGenome1::initialize() {

	QTextStream out(stdout);

	Debugger::info << "GAPlannerGenome1::initialize : Initializing the genome ... " << ENDL;

	/** 
	 * Iterate over the products. Select a BOM for each product.
	 **/

	for (int i = 0; i < pmm->prodman->products.size(); i++) {

		Product& curProd = (Product&) *(pmm->prodman->products[i]);

		out << "GAPlannerGenome1::initialize : Selecting a BOP for product " << curProd.ID << endl;

		BillOfProcesses& curBOP = (BillOfProcesses&) *(curProd.rndBOP());

		curProd.bopID = curBOP.ID;

		out << "GAPlannerGenome1::initialize : Selected BOP " << curBOP.ID << endl;

		// Randomly set the item route indices
		curBOP.setItemRouteIdxRnd();

	}

	out << "GAPlannerGenome1::initialize : Creating the initial plan for the genome... " << endl;

	Plan initPlan;

	// Collect the current plan for the genome
	initPlan = pmm->prodman->bops2Plan();

	out << initPlan << endl;

	// Restore the products' states from the initial process plan
	pmm->prodman->bopsFromPlan(initPlan);

	// Generate the actual process model
	ProcessModel planPM = pmm->plan2PM(initPlan);

	// Prepare the resources and assign the operations
	rc.init();
	for (ListDigraph::NodeIt nit(planPM.graph); nit != INVALID; ++nit) {
		if (!rc.assign(planPM.ops[nit])) {

			out << "GAPlannerGenome1::initialize : Assigning operation: " << *pmm->pm.ops[nit] << endl;
			Debugger::err << "GAPlannerGenome1::initialize : Failed to assign operation to resource!" << ENDL;
		}
	}

	out << planPM << endl;

	Schedule sched;

	// IMPORTANT!!! Set the ID of this scheduler as the index corresponding to the current plan
	scheduler->ID = 0;
	scheduler->pm = planPM;
	scheduler->rc = rc;
	scheduler->sched = &sched;
	scheduler->options = schedOptions;
	scheduler->obj = obj;

	// Now the process model can be cleaned
	planPM.clearSchedRelData();

	// Run the scheduler
	scheduler->schedule();

	Debugger::info << "GAPlannerGenome1::initialize : Performed initial scheduling. " << ENDL;

	Debugger::info << "GAPlannerGenome1::initialize : Found solution : " << sched.objective << ENDL;
	// Turn the found schedule into the genome's representation
	//out << sched << endl;

	operations = sched.operations;
	operationPrecedences = sched.operationPrecedences; // Precedences which correspond to the current schedule
	operationPrecedencesConj = sched.operationPrecedencesConj;

	/*
	// Remove the non-conjunctive precedences
	QSet<int> nonConjPrecIdcs;
	for (int i = 0; i < operationPrecedencesConj.size(); i++) {
		if (!operationPrecedencesConj[i]) {
			nonConjPrecIdcs << i;
		}
	}
	QList<QPair<int, int> > newOperationPrecedences;
	QList<bool > newOperationPrecedencesConj;
	for (int i = 0; i < operationPrecedencesConj.size(); i++) {
		if (!nonConjPrecIdcs.contains(i)) {
			newOperationPrecedences << operationPrecedences[i];
			newOperationPrecedencesConj << true;
		}
	}
	operationPrecedences = newOperationPrecedences;
	operationPrecedencesConj = newOperationPrecedencesConj;
	 */


	plan = initPlan;

	schedule = sched;

}

void GAPlannerGenome1::init(GAGenome &gen) {
	GAPlannerGenome1 &g = (GAPlannerGenome1 &) gen;

	g.initialize();

}

void GAPlannerGenome1::mate(GAPlannerGenome1 &mate, GAPlannerGenome1& child) {

	QTextStream out(stdout);

	//out << "GAPlannerGenome::mate : Mating... " << endl;

	// Select a crossing section of the mate
	crossStart = Rand::rndInt(0, Math::min(operations.size() - 1, mate.operations.size() - 1)); // [0, n-1] Inserting into this parent
	crossEnd = Rand::rndInt(crossStart, Math::min(crossStart + 100000, mate.operations.size() - 1) /*mate.operations.size() - 1*/); // [crossStart, n-1]
	int crossLen = crossEnd - crossStart + 1;
	crossSection = mate.operations.mid(crossStart, crossLen);





	// Operation indices in the parent genomes
	QList<QPair<int, int> > thisOrdID2OpIdx;
	QList<QPair<int, int> > mateOrdID2OpIdx;

	QHash<int, int> curOrdID2OpIdx;
	for (int i = 0; i < operations.size(); i++) {
		curOrdID2OpIdx[operations[i].OID] = 1; // Initial index is 1
	}

	for (int i = 0; i < operations.size(); i++) {
		thisOrdID2OpIdx << QPair<int, int>(operations[i].OID, curOrdID2OpIdx[operations[i].OID]);
		curOrdID2OpIdx[operations[i].OID]++;
	}
	crossOrdID2OpIdx = thisOrdID2OpIdx;

	curOrdID2OpIdx.clear();
	for (int i = 0; i < mate.operations.size(); i++) {
		curOrdID2OpIdx[mate.operations[i].OID] = 1; // Initial index is 1
	}

	for (int i = 0; i < mate.operations.size(); i++) {
		mateOrdID2OpIdx << QPair<int, int>(mate.operations[i].OID, curOrdID2OpIdx[mate.operations[i].OID]);
		curOrdID2OpIdx[mate.operations[i].OID]++;
	}
	mate.crossOrdID2OpIdx = mateOrdID2OpIdx;

	// Assign the idx array to the child genome
	child.crossOrdID2OpIdx = thisOrdID2OpIdx;

	QList<QPair<int, int> > crossSectionOpIdx;
	crossSectionOpIdx = mateOrdID2OpIdx.mid(crossStart, crossLen);

	// Insert the idx cross section into the array of indices of this
	int curInsertPosIdx = crossStart;
	for (int i = 0; i < crossSectionOpIdx.size(); i++) {

		child.crossOrdID2OpIdx.insert(curInsertPosIdx, crossSectionOpIdx[i]);

		curInsertPosIdx++;
	}







	// Copy the representation of this genome into the child
	child.operations = operations;

	// Insert the crossing section into the child
	int curInsertPos = crossStart;
	for (int i = 0; i < crossSection.size(); i++) {

		Operation curOp = crossSection[i];

		child.operations.insert(curInsertPos, curOp);

		curInsertPos++;

	}

	// DEBUG
	for (int i = 0; i < child.operations.size(); i++) {
		if (child.operations[i].OID != child.crossOrdID2OpIdx[i].first) {
			Debugger::err << "GAPlannerGenome1::mate : Sequence with indices is false!!!" << ENDL;
		}
	}
	// END DEBUG	

	// Copy the information about the crossing section into the child
	child.crossSection = crossSection;
	child.crossStart = crossStart;
	child.crossEnd = crossEnd;

	// Repair the child
	//out << "GAPlannerGenome1::mate : Repairing..."<< endl;
	child.repair(*this, mate);
	//out << "GAPlannerGenome1::mate : Done repairing."<< endl;

	//child.evaluate(gaTrue);
	
	mutNS(2,true);

	//out << "GAPlannerGenome1::mate : Done mating. " << endl;

	//out << "GAPlannerGenome1::mate : Child genome " << child << endl;

	//getchar();
}

int GAPlannerGenome1::cross(const GAGenome& a, const GAGenome& b, GAGenome* c, GAGenome* d) {
	//cout << "Genome:: crossing" << endl;
	GAPlannerGenome1& dad = (GAPlannerGenome1&) a;
	GAPlannerGenome1& mom = (GAPlannerGenome1&) b;
	GAPlannerGenome1& sis = (GAPlannerGenome1&) * c;
	GAPlannerGenome1& bro = (GAPlannerGenome1&) * d;

	int n_crossovers = 0; // Amount of the crossovers

	// Copy parents in order to initialize
	//if (c) sis.copy(mom);
	//if (d) bro.copy(dad);

	if (c && d) { // Make sister and brother

		mom.mate(dad, sis);
		dad.mate(mom, bro);

		sis.evaluate(gaTrue);
		bro.evaluate(gaTrue);
		n_crossovers = 2;
	} else {
		if (c) { // Only sister

			mom.mate(dad, sis);

			sis.evaluate(gaTrue);
			n_crossovers = 1;
		} else {
			if (d) { // Only brother

				dad.mate(mom, bro);

				bro.evaluate(gaTrue);
				n_crossovers = 1;
			}
		}
	}

	// Mutate with the incumbent
	if (c){ // DONE DIRECTLY IN MATE
		//sis.mutNS(2, true);
		//sis.evaluate(gaTrue);
	}
	if (d){ // DONE DIRECTLY IN MATE
		//bro.mutNS(2, true);
		//bro.evaluate(gaTrue);
	}

	//cout<< "n_crosses : "<<n_crossovers<<endl;
	//cout << "Cross finished" << endl;

	return n_crossovers;
}

void GAPlannerGenome1::mutateMach(const float prob) {


	QTextStream out(stdout);

	int numOper = Rand::rndInt(1, Math::max((int) Math::ceil(prob * operations.size()), 1));


	while (numOper > 0) {

		// Select randomly an operation
		int curOpIdx = Rand::rndInt(0, operations.size() - 1);
		Operation& curOp = (Operation&) operations[curOpIdx];

		ToolGroup& curTG = (ToolGroup&) rc(curOp.toolID); // The current machine group for the operation

		QList<Machine*> curMachs = curTG.machines(curOp.type);

		Machine& m = (Machine&) * curMachs[Rand::rndInt(0, curMachs.size() - 1)];

		// Mutate the machine of the operation
		curOp.machID = m.ID;

		// Update the processing time of the operation
		curOp.p(m.procTime(&curOp));

		// Repair its disjunctive precedence constraints

		QSet<int> opPrecIdxRem; // Indices of the precedence constraints to be removed
		int oldPredOpID = -1;
		int oldSuccOpID = -1;
		for (int i = 0; i < operationPrecedences.size(); i++) {

			if (!operationPrecedencesConj[i]) { // This is a machine precedence

				QPair<int, int> curPrec = operationPrecedences[i];

				if (curPrec.first == curOp.ID || curPrec.second == curOp.ID) {
					opPrecIdxRem << i;
				}

				if (curPrec.second == curOp.ID) {
					oldPredOpID = curPrec.first;
				}

				if (curPrec.first == curOp.ID) {
					oldSuccOpID = curPrec.second;
				}

			}

		}
		// Remove the false precedence constraints
		QList<QPair<int, int> > newOperationPrecedences;
		QList<bool > newOperationPrecedencesConj;
		for (int i = 0; i < operationPrecedences.size(); i++) {
			if (!opPrecIdxRem.contains(i)) {
				newOperationPrecedences << operationPrecedences[i];
				newOperationPrecedencesConj << operationPrecedencesConj[i];
			}
		}
		operationPrecedences = newOperationPrecedences;
		operationPrecedencesConj = newOperationPrecedencesConj;

		// Add the disjunctive precedence constraint between the old predecessor and the old successor of this operation
		if (oldPredOpID != -1 && oldSuccOpID != -1) {
			operationPrecedences << QPair<int, int>(oldPredOpID, oldSuccOpID);
			operationPrecedencesConj << false;
		}

		// Add the new machine operation precedences
		int newPredOpID = -1;
		int newSuccOpID = -1;
		for (int j = curOpIdx + 1; j < operations.size(); j++) {
			if (curOp.machID == operations[j].machID) {
				newSuccOpID = operations[j].ID;
				break;
			}
		}
		for (int j = curOpIdx - 1; j >= 0; j--) {
			if (curOp.machID == operations[j].machID) {
				newPredOpID = operations[j].ID;
				break;
			}
		}
		if (newPredOpID != -1) {
			operationPrecedences << QPair<int, int>(newPredOpID, curOp.ID);
			operationPrecedencesConj << false;
		}
		if (newSuccOpID != -1) {
			operationPrecedences << QPair<int, int>(curOp.ID, newSuccOpID);
			operationPrecedencesConj << false;
		}

		// Remove the possible previous precedence between the new neighboring operations
		if (newPredOpID != -1 && newSuccOpID != -1) {

			int idxRem = -1;
			for (idxRem = 0; idxRem < operationPrecedences.size(); idxRem++) {
				if (operationPrecedences[idxRem].first == newPredOpID && operationPrecedences[idxRem].second == newSuccOpID && !operationPrecedencesConj[idxRem]) {
					break;
				}
			}

			operationPrecedences.removeAt(idxRem);
			operationPrecedencesConj.removeAt(idxRem);

		}

		numOper--;
	}

	//double prevObj = schedule.objective;

	// Restore the PM

	//out << "GAPlannerGenome::mutateMach : Restoring with precedence constraints : " << endl;
	//for (int i = 0; i < operationPrecedences.size(); i++) {
	//	out << operationPrecedences[i].first << "->" << operationPrecedences[i].second << " : " << operationPrecedencesConj[i] << endl;
	//}

	ProcessModel restoredPM = restorePM();
	schedule.fromPM(restoredPM, *obj);

	//operations = schedule.operations;
	//operationPrecedences = schedule.operationPrecedences;
	//operationPrecedencesConj = schedule.operationPrecedencesConj;

	//out << "After the machine mutator : " << endl << *this << endl;

	//out << restoredPM << endl;

	//out << " --------- " << prevObj << endl;
	//out << " -------- " << TWT()(restoredPM) << endl;
	//out << " --------- " << schedule.objective << endl;

	//getchar();

}

void GAPlannerGenome1::mutateRoute(const float prob) {

	// Select the number of parts to mutate
	QTextStream out(stdout);

	// Select for how many products the routes of their parts are mutated
	int numProdsToChange = Rand::rndInt(1, Math::max((int) Math::ceil(prob * pmm->prodman->products.size()), 1));
	QSet<int> prodIDsToChange;
	for (int i = 0; i < numProdsToChange; i++) {
		prodIDsToChange << pmm->prodman->products[Rand::rndInt(0, pmm->prodman->products.size() - 1)]->ID;
	}

	// Collect orders of each product
	QHash<int, QSet<int> > prodID2ordIDs;
	QHash<int, int > prodID2BOPID;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2ordIDs[curOp.OT] << curOp.OID;

		prodID2BOPID[curOp.OT] = curOp.BID;


	}

	// For each selected product perform a route change

	foreach(const int& curProdID, prodIDsToChange) {

		Product& curProd = (*(pmm->prodman))(curProdID);

		QSet<int> curOrderIDs = prodID2ordIDs[curProdID]; // Order IDs of the current product

		BillOfProcesses& curBOP = (BillOfProcesses&) * curProd.bopByID(prodID2BOPID[curProdID]);

		int numParts = Rand::rndInt(1, Math::max(1, (int) prob * (countNodes(curBOP.bom()->graph) - 2)));

		QSet<int> locItmIDs; // Local IDs of the items which must be mutated
		QHash<int, int> locItmID2itmType;

		// Collect all local IDs of the items
		for (ListDigraph::NodeIt nit(curBOP.bom()->graph); nit != INVALID; ++nit) {
			if (nit != curBOP.bom()->head && nit != curBOP.bom()->tail) {

				locItmIDs << curBOP.bom()->itemID[nit];

				locItmID2itmType[curBOP.bom()->itemID[nit]] = curBOP.bom()->itypeID[nit];

			}
		}

		// Remove the spare item IDs
		while (locItmIDs.size() > numParts) {
			locItmIDs.remove(locItmIDs.toList()[Rand::rndInt(0, locItmIDs.toList().size() - 1)]);
		}

		// Iterate over the local item IDs

		foreach(const int& curLocItmID, locItmIDs) {

			// Get the type of the local item
			int curItmType = locItmID2itmType[curLocItmID];

			// Select a new route for the items
			const QList<Route*>& curRoutes = curBOP.itemTypeRoutes()[curItmType];
			int newItmRouteIdx = Rand::rndInt(0, curRoutes.size() - 1);
			int newItmRouteID = curRoutes[newItmRouteIdx]->ID;

			plan.prodID2ItemID2RouteID[curProdID][curLocItmID] = newItmRouteID;
			plan.prodID2ItemID2RouteIdx[curProdID][curLocItmID] = newItmRouteIdx;

#ifdef DEBUG
			QSet<int> curItmRouteIDs;

			foreach(const int& curOrdID, curOrderIDs) {
				for (int i = 0; i < operations.size(); i++) {
					Operation& curOp = (Operation&) operations[i];
					if (curOp.OID == curOrdID && (curOp.IID - (curOrdID << 16)) == curLocItmID) {
						curItmRouteIDs << curOp.RID;
					}
				}
			}

			if (curItmRouteIDs.size() > 1) {
				Debugger::err << "GAPlannerGenome1::mutateRoute : Too many routes for the same item!!!" << ENDL;
			}
#endif

			// Iterate over the orders of the product

			foreach(const int& curOrdID, curOrderIDs) {

				// Find all operations of the current part
				QList<int> curItemOperIdcs;
				QList<Operation> curItemOps;
				QSet<int> curItemOpIDs;

				for (int i = 0; i < operations.size(); i++) {

					Operation& curOp = (Operation&) operations[i];

					if (curOp.OID == curOrdID && (curOp.IID - (curOrdID << 16)) == curLocItmID) {
						curItemOperIdcs << i;
						curItemOps << curOp;
						curItemOpIDs << curOp.ID;
					}

				} // Finding operation indices of the current part

				// Remove the disjunctive precedence constraints 
				QSet<int> opPrecIdcsRem;
				for (int i = 0; i < operationPrecedences.size(); i++) {

					if (!operationPrecedencesConj[i]) {

						QPair<int, int> curPrec = operationPrecedences[i];

						//if (curItemOpIDs.contains(curPrec.first) || curItemOpIDs.contains(curPrec.second)) {
						opPrecIdcsRem << i;
						//}

					}

				}
				QList<QPair<int, int> > newOperationPrecedences;
				QList<bool> newOperationPrecedencesConj;
				for (int i = 0; i < operationPrecedences.size(); i++) {
					if (!opPrecIdcsRem.contains(i)) {
						newOperationPrecedences << operationPrecedences[i];
						newOperationPrecedencesConj << operationPrecedencesConj[i];
					}
				}
				operationPrecedences = newOperationPrecedences;
				operationPrecedencesConj = newOperationPrecedencesConj;

				// Change the content of the item's operations without touching their IDs => conjunctive precedences do not change
				Route& curRoute = (Route&) * curRoutes[newItmRouteIdx];
				for (int i = 0; i < curRoute.otypeIDs.size(); i++) {

					int curOpType = curRoute.otypeIDs[i];

					for (int j = i; j < curItemOps.size(); j++) {
						if (curItemOps[j].type == curOpType) { // Move this operation to the i-th position
							curItemOps.move(j, i);
							curItemOps[i].RID = newItmRouteID;
							break;
						}
					}

				}

				// Move the permuted operations into the main array
				for (int i = 0; i < curItemOperIdcs.size(); i++) {
					int curIdx = curItemOperIdcs[i];

					// Preserve the ID
					int curID = operations[curIdx].ID;

					operations[curIdx] = curItemOps.takeFirst();

					// Restore the ID
					operations[curIdx].ID = curID;
				}

				// Restore all disjunctive precedence constraints
				for (int i = 0; i < operations.size(); i++) {
					int curMachID = operations[i].machID;

					// Search for its successor
					for (int j = i + 1; j < operations.size(); j++) {
						if (operations[j].machID == curMachID) {
							operationPrecedences << QPair<int, int>(operations[i].ID, operations[j].ID);
							operationPrecedencesConj << false;
							break;
						}

					}

				} // Restoring the disjunctive precedence constraints


			} // Iterating over the orders of the product

		} // Iterating over the local item IDs


	} // Iterating over the products


	ProcessModel restoredPM = restorePM();
	schedule.fromPM(restoredPM, *obj);

}

void GAPlannerGenome1::mutateBOM(const float prob) {

	QTextStream out(stdout);

	//out << "Before the BOM mutator : " << endl << *this << endl;
	//out << schedule.pm << endl;

	schedule.objective = Math::MAX_DOUBLE;

	// Select the number of products to be mutated
	int numProdsToChange = Rand::rndInt(1, Math::max((int) Math::ceil(prob * pmm->prodman->products.size()), 1));
	QSet<int> prodIDsToChange;
	for (int i = 0; i < numProdsToChange; i++) {
		prodIDsToChange << pmm->prodman->products[Rand::rndInt(0, pmm->prodman->products.size() - 1)]->ID;
	}

	// Collect orders of each product
	QHash<int, QSet<int> > prodID2ordIDs;
	QHash<int, int > prodID2BOPID;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2ordIDs[curOp.OT] << curOp.OID;

		prodID2BOPID[curOp.OT] = curOp.BID;

	}

	// Remove all disjunctive precedence constraints
	QList<QPair<int, int> > newOperationPrecedences;
	QList<bool> newOperationPrecedencesConj;
	for (int i = 0; i < operationPrecedences.size(); i++) {
		if (operationPrecedencesConj[i]) { // Copy only the conjunctive precedence constraint
			newOperationPrecedences << operationPrecedences[i];
			newOperationPrecedencesConj << operationPrecedencesConj[i];
		}
	}
	operationPrecedences = newOperationPrecedences;
	operationPrecedencesConj = newOperationPrecedencesConj;

	if (!valid()) {
		Debugger::err << "GAPlannerGenome1::mutateBOM : Invalid genome before the mutation!" << ENDL;
	}

	// For each product select a new BOM ID

	foreach(const int& curProdID, prodIDsToChange) {

		//out << "GAPlannerGenome1::mutateBOM : Product " << curProdID << " , old BOP : " << prodID2BOPID[curProdID] << endl;

		// Select some BOP randomly
		BillOfProcesses& curBOP = (BillOfProcesses&) *(pmm->prodman->productByID(curProdID).bops[Rand::rndInt(0, pmm->prodman->productByID(curProdID).bops.size() - 1)]);
		int curBOPID = curBOP.ID; // ID of the newly selected BOP

		//out << "GAPlannerGenome1::mutateBOM : Product " << curProdID << " , new BOP : " << curBOPID << endl;

		// Randomly select the routes in the BOP.
		curBOP.setItemRouteIdxRnd();

		// Generate the BOP
		curBOP.generate();

		// Sort the nodes of the generated BOP topologically
		//*********** Topological ordering of the nodes in the BOP *************
		ListDigraph::NodeMap<int> nodes2pos(curBOP.graph()); // Map of nodes sorted topologically

		// Sort the nodes topologically
		topologicalSort(curBOP.graph(), nodes2pos);

		QMap<int, ListDigraph::Node> pos2nodes; // Topologically sorted nodes 

		for (ListDigraph::NodeMap<int>::MapIt mi(nodes2pos); mi != INVALID; ++mi) {
			pos2nodes[*mi] = mi;
		}

		QList<ListDigraph::Node> tnodes;

		tnodes.clear();
		tnodes.reserve(pos2nodes.size());
		for (QMap<int, ListDigraph::Node>::iterator sti = pos2nodes.begin(); sti != pos2nodes.end(); sti++) {
			tnodes << sti.value();
		}
		tnodes.removeOne(curBOP.graphHead());
		tnodes.removeOne(curBOP.graphTail());

		//**********************************************************************

		if (!valid()) {
			Debugger::err << "GAPlannerGenome1::mutateBOM : Invalid genome before changing product/orders!" << ENDL;
		}

		// Get the number of operations in the old BOP, mark the operations
		int oldNumOper = 0;
		QSet<int> opIDPrecRem;
		for (int i = 0; i < operations.size(); i++) {

			Operation& curOp = (Operation&) operations[i];

			if (curOp.OT == curProdID) {
				oldNumOper++;

				opIDPrecRem << curOp.ID;

				curOp.ID = -1;
				curOp.IT = -1;
				curOp.IID = -1;
				curOp.RID = -1;
				curOp.type = -1;
				curOp.toolID = -1;
				curOp.machID = -1;
			}

		}
		if (oldNumOper % prodID2ordIDs[curProdID].size() != 0) {
			//out << *this << endl;
			Debugger::err << "GAPlannerGenome1::mutateBOM : Invalid genome : Number of operations mismatch!" << ENDL;
		}
		oldNumOper /= prodID2ordIDs[curProdID].size(); // Since each order of the product has exactly the same number of operations

		// Remove the relevant precedence constraints for all operations corresponding to the product
		QList<QPair<int, int> > newOperationPrecedences;
		QList<bool> newOperationPrecedencesConj;
		for (int i = 0; i < operationPrecedences.size(); i++) {
			QPair<int, int> curPrec = operationPrecedences[i];
			if (!opIDPrecRem.contains(curPrec.first) && !opIDPrecRem.contains(curPrec.second)) {
				newOperationPrecedences << operationPrecedences[i];
				newOperationPrecedencesConj << operationPrecedencesConj[i];
			}
		}
		operationPrecedences = newOperationPrecedences;
		operationPrecedencesConj = newOperationPrecedencesConj;

		// Get the number of spare operations per order
		int numSpareOper = oldNumOper - tnodes.size();

		//out << "Changing orders ... " << endl;
		//out << "Should be operations per order : " << tnodes.size() << endl;
		//out << "Number of spare operations per order : " << numSpareOper << endl;

		foreach(const int& curOrdID, prodID2ordIDs[curProdID]) {

			// Get the operation indices for each order of the orders
			QHash<int, QList<int> > ordID2opIdcs;
			for (int i = 0; i < operations.size(); i++) {

				Operation& curOp = (Operation&) operations[i];

				ordID2opIdcs[curOp.OID] << i;

			}

			int tmpSpareOper = numSpareOper;

			// Check whether any operations should be deleted
			QHash<int, QSet<int> > ordID2opIdcsRem;
			while (tmpSpareOper > 0) {

				// Select randomly an operation position
				int curOpPosRem = Rand::rndInt(0, ordID2opIdcs[curOrdID].size() - 1);

				// Add the index for removal
				ordID2opIdcsRem[curOrdID] << ordID2opIdcs[curOrdID][curOpPosRem];

				// Remove the already selected index
				ordID2opIdcs[curOrdID].removeAt(curOpPosRem);

				tmpSpareOper--;
			}
			// Remove the operations with the selected indices
			QList<Operation> newOperations;
			for (int i = 0; i < operations.size(); i++) {
				if (!ordID2opIdcsRem[curOrdID].contains(i)) {
					newOperations << operations[i];
				}
			}
			operations = newOperations;


			// Check whether any operations corresponding to the order should be added
			while (tmpSpareOper < 0) {

				//out << " Inserting additional operations... " << endl;

				Operation newOper;

				newOper.ID = -1;
				newOper.OID = curOrdID;
				newOper.OT = curProdID;
				newOper.IT = -1;
				newOper.IID = -1;
				newOper.RID = -1;
				newOper.type = -1;
				newOper.toolID = -1;
				newOper.machID = -1;

				int insPos = Rand::rndInt(0, operations.size());
				operations.insert(insPos, newOper);

				tmpSpareOper++;

				//out << " Done inserting additional operations. " << endl;
			}

			// Insert the generated BOP (TOPOLOGICALLY ORDERED!!!)
			// Get the operations' positions for the current order
			QList<int> opPos;
			for (int i = 0; i < operations.size(); i++) {
				Operation& curOp = (Operation&) operations[i];
				if (curOp.OID == curOrdID) {
					opPos << i;
				}
			}

			// Iterate over the topologically sorted nodes of the BOP
			//out << "Inserting BOP ... " << endl;
			QMap<ListDigraph::Node, int> bopNode2OpIdx;
			for (int i = 0; i < tnodes.size(); i++) {

				//out << "test... " << endl;
				int curPos = opPos.takeFirst();
				//out << "Done test. " << endl;
				ListDigraph::Node curNode = tnodes[i];
				Operation& curOp = (Operation&) operations[curPos];

				curOp.ID = (curOrdID << 16) + i + 1; // ID of the operations depending on the order's ID

				curOp.BID = curBOPID;

				curOp.IID = curBOP.oLocItemID()[curNode]; // Temporary

				curOp.IT = curBOP.itemID2ItemType()[curOp.IID];

				curOp.RID = curBOP.itemRoutes()[curOp.IID][curBOP.itemRouteIdx()[curOp.IID]]->ID;

				// Correct the item ID
				curOp.IID = (curOrdID << 16) + curOp.IID;

				curOp.SI = -1;

				curOp.type = curBOP.otypeID()[curNode];

				rc.assign(&curOp); // Assign to the machine group -> sets the toolID

				// Select the machine ID randomly
				curOp.machID = rc(curOp.toolID).machines(curOp.type)[Rand::rndInt(0, rc(curOp.toolID).machines(curOp.type).size() - 1)]->ID;

				// Update the processing time of the operation
				curOp.p(rc(curOp.toolID, curOp.machID).procTime(&curOp));

				bopNode2OpIdx[curNode] = curPos;

			}
			//out << "Done inserting BOP. " << endl;

			// Collect the precedence constraints between the operations
			for (ListDigraph::ArcIt ait(curBOP.graph()); ait != INVALID; ++ait) {

				ListDigraph::Node startNode = curBOP.graph().source(ait);
				ListDigraph::Node endNode = curBOP.graph().target(ait);

				if (startNode == curBOP.graphHead() || endNode == curBOP.graphTail()) {
					continue;
				} else {
					QPair<int, int> curPrec;

					curPrec.first = operations[bopNode2OpIdx[startNode]].ID;

					curPrec.second = operations[bopNode2OpIdx[endNode]].ID;

					operationPrecedences << curPrec;
					operationPrecedencesConj << true;
				}

			}


		} // Iterating over the order IDs


		if (!valid()) {
			Debugger::err << "GAPlannerGenome1::mutateBOM : Invalid genome after changing orders!" << ENDL;
		}

		//out << "Done changing orders." << endl;

		// Degenerate the BOP
		curBOP.degenerate();
	} // Iterating over the products

	// Restore the disjunctive precedences between the operations
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		int curToolId = curOp.toolID;
		int curMachID = curOp.machID;

		for (int j = i + 1; j < operations.size(); j++) {

			Operation& nextOp = operations[j];

			if (nextOp.toolID == curToolId && nextOp.machID == curMachID) { // This is a disjunctive precedence constraint
				operationPrecedences << QPair<int, int> (curOp.ID, nextOp.ID);
				operationPrecedencesConj << false;
				break;
			}

		} // Next operation

	} // Restoring the disjunctive precedence constraints

	// Reconstruct the PM based on the current genome's representation
	ProcessModel restoredPM = restorePM();

	// Get the schedule based on the restored PM
	schedule.fromPM(restoredPM, *obj);

	//out << "After the BOM mutator : " << endl << *this << endl;

	//out << restoredPM << endl;

	//out << " --------- " << prevObj << endl;
	//out << " -------- " << TWT()(restoredPM) << endl;
	//out << " --------- " << schedule.objective << endl;

	//getchar();

}

int GAPlannerGenome1::mut(const float prob) {


	// Decide whether to mutate
	if (Rand::rndDouble() > prob) {
		return 0;
	}

	mutNS(3, false); // Mutate without considering the incumbent
	
	return 0;
	
	//for (int i = 0; i < operations.size(); i++) {

	//	Operation& curOp = (Operation&) operations[i];

	//	if (curOp.ID <= 0) {
	//		Debugger::err << "GAPlannerGenome1::mut : Invalid operation before mutating!!!" << ENDL;
	//	}

	//}

	// Select a mutation approach
	double p = Rand::rndDouble();

	if (p < 0.5) { // Mutate BOM
		mutateBOM(prob); // The mutator performs reparation as well

		//for (int i = 0; i < operations.size(); i++) {

		//	Operation& curOp = (Operation&) operations[i];

		//	if (curOp.ID <= 0) {
		//		Debugger::err << "GAPlannerGenome1::mut : Invalid operation after mutating!!!" << ENDL;
		//	}

		//}

	} else if (p < 0.7) { // Mutate some route
		mutateRoute(prob); // The mutator performs reparation as well

	} else { // Mutate some machine
		mutateMach(prob); // The mutator performs reparation as well

	}

	this->evaluate(gaTrue);

	return 0;

}

int GAPlannerGenome1::mutNS(const int& nPos, const bool& withIncumbent) {

	QTextStream out(stdout);
	
	ProcessModel curPM = restorePM();
	
	//if (!dag(curPM.graph)){
	//	Debugger::err << "GAPlannerGenome1::mutNS : The the restored PM is not DAG BEFORE NS mutation!!!" << ENDL;
	//}
	
	double incumbentObjective = (*obj)(curPM);
	//out << "GAPlannerGenome1::mutNS : Incumbent TWT : " << incumbentObjective << endl;

	// Get the vector of order IDs for the current genome
	QVector<int> curOrdIDs(operations.size());
	for (int i = 0; i < operations.size(); i++) {
		curOrdIDs[i] = operations[i].OID;
	}

	if (operations.size() < nPos) {
		Debugger::err << "GAPlannerGenome1::mutNS : Too few operations for the given number of permutated positions!!!" << ENDL;
	}

	// Select nPos different positions for permutations
	QList<int> permPositions;
	while (permPositions.size() < nPos) {
		// Generate a random position
		int curPos = Rand::rndInt(0, operations.size() - 1);

		if (!permPositions.contains(curPos)) {
			permPositions << curPos;
		}

	}

	// Sort the positions
	Math::sort(permPositions);

	// Generate the next permutation of the permuted order IDs
	QVector<int> newPermPositions(permPositions.toVector());
	QVector<int> newCurOrdIDs(operations.size());
	QVector<Operation> newOperations(operations.size());
	QList<QPair<int, int> > newOperationPrecedences;
	QList<bool> newOperationPrecedencesConj;

	// DEBUG 
	/*
	// Build all lexicographic permutations
	out << "Permutations : " << endl;
	do{
		
		for (int i = 0 ; i < permPositions.size() ; i++){
			out << permPositions[i] << ",";
		}
		out << endl;
		
	}while(Math::nextLexPermutation(permPositions));
	getchar();
	 */
	// END DEBUG

	// The best new genome
	GAPlannerGenome1 bestNewGenome(*this);

	Math::nextLexPermutation(newPermPositions); // To avoid the incumbent

	if (!withIncumbent) {
		incumbentObjective = Math::MAX_DOUBLE;
	}

	do {

		newOperationPrecedences.clear();
		newOperationPrecedencesConj.clear();
		
		// IMPORTANT!!! Stub for generating next permutations
		//qSwap(newPermPositions[newPermPositions.size() - 2], newPermPositions[newPermPositions.size() - 1]);

		newCurOrdIDs = curOrdIDs;

		for (int i = 0; i < newPermPositions.size(); i++) {
			newCurOrdIDs[permPositions[i]] = curOrdIDs[newPermPositions[i]];
		}

		// Generate the new sequence of operations
		QList<Operation> tmpOperations = operations;

		for (int i = 0; i < newCurOrdIDs.size(); i++) {

			int newCurOrdID = newCurOrdIDs[i];

			for (int j = 0; j < tmpOperations.size(); j++) {
				if (tmpOperations[j].OID == newCurOrdID) {
					newOperations[i] = tmpOperations[j];
					tmpOperations.removeAt(j);
					break;
				}
			}

		} // Now we have a new operations sequence corresponding to the newly generated genome
		// DEBUG
		if (tmpOperations.size() > 0) {
			Debugger::err << "GAPlannerGenome1::mutNS : Not all operations have been moved!!!" << ENDL;
		}
		// END DEBUG

		// Restore the disjunctive AND conjunctive precedence constraints

		//. Restore the conjunctive constraints
		for (int i = 0; i < newOperations.size(); i++) {
			int curOrdID = newOperations[i].OID;

			// Search for the next operation belonging to the same order
			for (int j = i + 1; j < newOperations.size(); j++) {
				if (newOperations[j].OID == curOrdID) { // Conjunctive constraint (Only linear BOMs => at most 1 predecessor/successor)
					newOperationPrecedences << QPair<int, int>(newOperations[i].ID, newOperations[j].ID);
					newOperationPrecedencesConj << true;
					break;
				}
			}

		}

		// Restore the conjunctive constraints
		for (int i = 0; i < newOperations.size(); i++) {
			int curMachID = newOperations[i].machID;

			if (curMachID < 0){
				Debugger::err << "Negative machine while mutating!!!" << ENDL;
			}
			
			// Search for the next operation belonging to the same order
			for (int j = i + 1; j < newOperations.size(); j++) {
				if (newOperations[j].machID == curMachID) { // Sched-base constraint
					newOperationPrecedences << QPair<int, int>(newOperations[i].ID, newOperations[j].ID);
					newOperationPrecedencesConj << false;
					break;
				}
			}

		}


		// Create a new genome for evaluation
		GAPlannerGenome1 newGenome(*this);

		// Set the new operation sequence and the precedence constraints
		newGenome.operations = /*operations;//*/newOperations.toList();
		newGenome.operationPrecedences = /*operationPrecedences; // */ newOperationPrecedences;
		newGenome.operationPrecedencesConj = /*operationPrecedencesConj; // */ newOperationPrecedencesConj;

		// Restore the new process model from the sequence of operations
		ProcessModel newPM = newGenome.restorePM();

		// Get the objective value of the new genome
		double newObjective = (*obj)(newPM);

		//out << "GAPlannerGenome1::mutNS : TWT of the newly generated genome : " << newObjective << endl;
		//getchar();

		if (newObjective < incumbentObjective) { // A better solution has been found
			//out << "Incumbent : " << incumbentObjective << endl;
			//out << "New : " << newObjective << endl;
			//Debugger::err << "Found a better solution!!!" << ENDL;

			bestNewGenome = GAPlannerGenome1(newGenome);
			
			incumbentObjective = newObjective;
			
			//out << "TWT(mut) : " << newObjective << endl;
			
			//if (newObjective < this->geneticAlgorithm()->statistics().bestIndividual().score()){
			//	Debugger::err << "Found a better solution through mutation!!! " << ENDL;
			//}
		}

	} while (Math::nextLexPermutation(newPermPositions)); // Continuously generate new permutations

	*this = GAPlannerGenome1(bestNewGenome);

	// Restore the schedule and the PM of the best found genome
	ProcessModel restoredPM = restorePM();
	schedule.fromPM(restoredPM, *obj);
	
	//if (!dag(restoredPM.graph)){
	//	Debugger::err << "GAPlannerGenome1::mutNS : The the restored PM is not DAG AFTER NS mutation!!!" << ENDL;
	//}
	
	//this->evaluate(gaTrue);
}

int GAPlannerGenome1::mutate(GAGenome& gen, float prob) {
	GAPlannerGenome1& g = (GAPlannerGenome1&) gen;
	int res = g.mut(prob);

	// Important !!! Tell GALib to reevaluate the genome in order not to use the cached  value
	g.evaluate(gaTrue);

	return res;
}

void GAPlannerGenome1::repair(const GAPlannerGenome1& mom, const GAPlannerGenome1& dad) {

	QTextStream out(stdout);

	//out << "GAPlannerGenome1::repair : Repairing the child genome ... " << endl;

	//out << "Mom : " << mom << endl;

	//out << "Dad : " << dad << endl;

	//out << " Child : " << *this << endl;

	// Iterate over the crossing section and collect all products which are in it
	QSet<int> prodIDsToRepair;
	for (int i = 0; i < crossSection.size(); i++) {

		Operation& curOp = (Operation&) crossSection[i];

		prodIDsToRepair << curOp.OT;

	}

	//out << "Products to be considered : " << endl;

	//foreach(const int& curProdID, prodIDsToRepair) {
	//	out << curProdID << ",";
	//}
	//out << endl;


	// Iterate over all operations and collect the orders to be repaired
	QSet<int> ordIDsToRepair;
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		if (prodIDsToRepair.contains(curOp.OT)) {
			ordIDsToRepair << curOp.OID;

			// Mark the operation for consideration
			curOp.ID = -1;

		}

	}

	//out << "Orders to be considered : " << endl;

	//foreach(const int& curOrdID, ordIDsToRepair) {
	//	out << curOrdID << ",";
	//}
	//out << endl;

	// Collect the inherited sequence of machines
	QHash<int, QList<int> > ordID2machIDSeq;
	QHash<int, QList<int> > ordID2toolIDSeq;

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ordID2machIDSeq[curOp.OID] << curOp.machID;
		ordID2toolIDSeq[curOp.OID] << curOp.toolID;

	}

	// Collect inherited BOMs for different products 
	QHash<int, QSet<int> > prodID2bomIDs; // IDs of the BOMs for each product. The ones which contain > 1 product must be considered
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		prodID2bomIDs[curOp.OT].insert(curOp.BID);

	}

	// For each product to be repaired, calculate the number of operations in the mom's/dad's genome per order of this product
	QHash<int, int> prodID2numOpsPerOrdMom;
	QHash<int, int> prodID2numOpsPerOrdDad;

	// The number of operations per product in mom's genome
	for (int i = 0; i < mom.operations.size(); i++) {

		Operation& curOp = (Operation&) mom.operations[i];

		prodID2numOpsPerOrdMom[curOp.OT]++;

	}
	for (int i = 0; i < dad.operations.size(); i++) {

		Operation& curOp = (Operation&) dad.operations[i];

		prodID2numOpsPerOrdDad[curOp.OT]++;

	}

	for (int i = 0; i < pmm->prodman->products.size(); i++) {

		int curProdID = pmm->prodman->products[i]->ID;

		int curOrdsOfProd = pmm->ordman->ordersByType(curProdID).size();

		prodID2numOpsPerOrdMom[curProdID] /= curOrdsOfProd;
		prodID2numOpsPerOrdDad[curProdID] /= curOrdsOfProd;

	}

	QHash<int, int> ordID2NumSpareOps; // Number of spare operations for each order (a negative value means that some operations must be added)

	// For each product to be changed select only one of the alternative BOMs

	foreach(const int& curProdID, prodIDsToRepair) {

		if (prodID2bomIDs[curProdID].size() > 1) { // There are two BOMs for selection

			//out << "Product : " << curProdID << " - BOMs : " << prodID2bomIDs[curProdID].toList()[0] << " , " << prodID2bomIDs[curProdID].toList()[1] << endl;

			Product& curProd = (Product&) pmm->prodman->productByID(curProdID);

			int momBOMID = mom.plan.prodID2BOPID[curProdID];
			int dadBOMID = dad.plan.prodID2BOPID[curProdID];

			//out << "Mom's BOM : " << momBOMID << endl;
			//out << "Dad's BOM : " << dadBOMID << endl;

			// Get the number of operations in the current product 
			//out << "Operations of the product in mom's BOP (per order) : " << prodID2numOpsPerOrdMom[curProdID] << endl;

			//out << "Operations of the product in dad's BOP (per order) : " << prodID2numOpsPerOrdDad[curProdID] << endl;

			int childBOMID = -1;

			int numOpsShouldBe = -1; // The number of operations which should be present in each order of the product

			if (Rand::rndDouble() < 0.5) {
				childBOMID = momBOMID;
				numOpsShouldBe = prodID2numOpsPerOrdMom[curProdID];
			} else {
				childBOMID = dadBOMID;
				numOpsShouldBe = prodID2numOpsPerOrdDad[curProdID];
			}

			//out << "Selected BOM " << childBOMID << " for product " << curProdID << endl;
			//out << "Should be operations per order of product " << curProdID << " : " << numOpsShouldBe << endl;

			// Set the selected BOM
			prodID2bomIDs[curProdID].clear();
			prodID2bomIDs[curProdID] << childBOMID;

			// For each order of the product calculate the number of spare/missing parts
			QList<Order*> prodOrders = pmm->ordman->ordersByType(curProdID);

			for (int i = 0; i < prodOrders.size(); i++) {

				int curOrdID = prodOrders[i]->ID;

				// Get the number of operations for the order
				int curOrdOpNum = 0;
				for (int j = 0; j < operations.size(); j++) {
					if (operations[j].OID == curOrdID) {
						curOrdOpNum++;
					}
				}

				ordID2NumSpareOps[curOrdID] = curOrdOpNum - numOpsShouldBe;

				//out << "Spare parts for order " << curOrdID << " : " << ordID2NumSpareOps[curOrdID] << endl;

			}

		} else { // The BOM is the same

			Product& curProd = (Product&) pmm->prodman->productByID(curProdID);

			int momBOMID = mom.plan.prodID2BOPID[curProdID];

			//out << "Mom's/Dad's BOM : " << momBOMID << endl;

			// Get the number of operations in the current product 
			//out << "Operations of the product in mom's BOP (per order) : " << prodID2numOpsPerOrdMom[curProdID] << endl;

			int numOpsShouldBe = prodID2numOpsPerOrdMom[curProdID]; // The number of operations which should be present in each order of the product

			//out << "Should be operations per order of product " << curProdID << " : " << numOpsShouldBe << endl;

			// For each order of the product calculate the number of spare/missing parts
			QList<Order*> prodOrders = pmm->ordman->ordersByType(curProdID);

			for (int i = 0; i < prodOrders.size(); i++) {

				int curOrdID = prodOrders[i]->ID;

				// Get the number of operations for the order
				int curOrdOpNum = 0;
				for (int j = 0; j < operations.size(); j++) {
					if (operations[j].OID == curOrdID) {
						curOrdOpNum++;
					}
				}

				ordID2NumSpareOps[curOrdID] = curOrdOpNum - numOpsShouldBe;

				//out << "Spare parts for order " << curOrdID << " : " << ordID2NumSpareOps[curOrdID] << endl;

			}

		}

	} // Affected products

	// DEBUG
	for (int i = 0; i < operations.size(); i++) {
		if (operations[i].OID != crossOrdID2OpIdx[i].first) {
			Debugger::err << "GAPlannerGenome1::repair : Sequence with indices is affected!!!" << ENDL;
		}
	}
	// END DEBUG

	// Randomly remove the spare operation in each order to be modified

	foreach(const int& curOrdID, ordIDsToRepair) {

		if (ordID2NumSpareOps[curOrdID] > 0) { // Removing spare operations

			QList<int> curOrdOperPos;
			for (int i = 0; i < operations.size(); i++) {
				if (operations[i].OID == curOrdID) {
					curOrdOperPos << i;
				}
			}



			//out << "test" << endl;
			// Collect operation indices of the first parent which have to be removed
			QSet<int> indicesInCrossSection;
			for (int i = crossStart; i <= crossEnd; i++) {
				if (operations[i].OID == curOrdID) {
					if (curOrdID != crossOrdID2OpIdx[i].first) {
						out << "Should be : " << curOrdID << endl;
						out << "Is : " << crossOrdID2OpIdx[i].first << endl;
						Debugger::err << "GAPlannerGenome1::repair : Order mismatch!!!" << ENDL;
					}
					indicesInCrossSection << crossOrdID2OpIdx[i].second;
				}
			}
			//			out << "Indices in cross section : ";
			//			for (int i = 0; i < indicesInCrossSection.size(); i++) {
			//				out << indicesInCrossSection.toList()[i] << ",";
			//			}
			//			out << endl;

			QList<int> opPosToRemove;
			for (int i = 0; i < crossStart; i++) {
				if (operations[i].OID == curOrdID) {
					if (indicesInCrossSection.contains(crossOrdID2OpIdx[i].second)) {
						opPosToRemove << i;
					}
				}
			}
			for (int i = crossEnd + 1; i < operations.size(); i++) {
				if (operations[i].OID == curOrdID) {
					if (indicesInCrossSection.contains(crossOrdID2OpIdx[i].second)) {
						opPosToRemove << i;
					}
				}
			}

			//out << "Cross section start : " << crossStart << endl;
			//out << "Cross section end : " << crossEnd << endl;




			//QSet<int> curOrdOperPosRem = opPosToRemove;
			QSet<int> curOrdOperPosRem;


			curOrdOperPos = opPosToRemove;


			int ctr = ordID2NumSpareOps[curOrdID];
			//out << "Operations to remove : " << opPosToRemove.size() << endl;
			//getchar();
			while (ctr > 0) { // There are spare parts which should be removed
				int curIdx = Rand::rndInt(0, curOrdOperPos.size() - 1);
				//int curIdx = Rand::rndInt(0, opPosToRemove.size() - 1);

				curOrdOperPosRem << curOrdOperPos[curIdx];
				//curOrdOperPosRem << opPosToRemove[curIdx];

				curOrdOperPos.removeAt(curIdx);
				//opPosToRemove.removeAt(curIdx);

				// Decrease the number of spare operations in the order
				ctr--;
			}

			//			out << "Num Indices found - should remove : " << opPosToRemove.size() - ordID2NumSpareOps[curOrdID] << endl;
			//			out << "Removed - should remove : " << curOrdOperPosRem.size() - ordID2NumSpareOps[curOrdID] << endl;
			//			getchar();


			QList<Operation> newOperations;
			QList<QPair<int, int> > newCrossOrdID2OpIdx;

			int crossSectionShift = 0;
			for (int i = 0; i < operations.size(); i++) {
				if (curOrdOperPosRem.contains(i)) {

					if (i < crossStart) {
						// Shift left the cross section
						crossSectionShift++;
					}

					continue;
				} else {
					newOperations << operations[i];

					newCrossOrdID2OpIdx << crossOrdID2OpIdx[i];
				}
			}

			crossStart -= crossSectionShift;
			crossEnd -= crossSectionShift;

			operations = newOperations;

			crossOrdID2OpIdx = newCrossOrdID2OpIdx;

			ordID2NumSpareOps[curOrdID] = 0;


			// DEBUG
			int curProdID = pmm->ordman->orderByID(curOrdID).type;
			int numOpsShouldBe = prodID2numOpsPerOrdMom[curProdID];
			int numOpsIs = 0;
			for (int i = 0; i < operations.size(); i++) {
				if (operations[i].OID == curOrdID) {
					numOpsIs++;
				}
			}
			if (numOpsIs != numOpsShouldBe) {

				out << "Mom genome: " << endl;
				for (int i = 0; i < mom.operations.size(); i++) {
					if (mom.operations[i].OID == curOrdID) {
						out << "Pos : " << i << " idx : " << mom.crossOrdID2OpIdx[i].second << "  " << mom.operations[i] << endl;
					}
				}

				out << "Dad genome: " << endl;
				for (int i = 0; i < dad.operations.size(); i++) {
					if (dad.operations[i].OID == curOrdID) {
						out << "Pos : " << i << " idx : " << dad.crossOrdID2OpIdx[i].second << "  " << dad.operations[i] << endl;
					}
				}

				out << "Child genome: " << endl;
				for (int i = 0; i < operations.size(); i++) {
					if (operations[i].OID == curOrdID) {
						out << "Pos : " << i << " idx : " << crossOrdID2OpIdx[i].second << "  " << operations[i] << endl;
					}
				}
				out << "Cross Start : " << crossStart << endl;
				out << "Cross End : " << crossEnd << endl;
				out << "Order ID : " << curOrdID << endl;
				out << "Product ID : " << curProdID << endl;
				out << "Is operations : " << numOpsIs << endl;
				out << "Should be operations : " << numOpsShouldBe << endl;
				Debugger::err << "Too many/few operations!!!" << ENDL;
			}
			// END DEBUG	

		}

		/*
		if (ordID2NumSpareOps[curOrdID] < 0) { // Adding missing operations (In case without BOMs, there are no missing operations => this step will be ignored)
			//Debugger::err << "GAPlannerGenome1::repair : Adding missing operations is not implemented yet!!!" << ENDL;

			while (ordID2NumSpareOps[curOrdID] < 0) {

				int insPos = Rand::rndInt(0, operations.size() - 1);

				Operation insOp;

				insOp.ID = -1;

				insOp.OID = curOrdID;

				insOp.OT = pmm->ordman->orderByID(curOrdID).type;

				insOp.type = -1;

				insOp.IID = -1;
				insOp.IT = -1;

				insOp.RID = -1;
				insOp.SI = -1;

				insOp.toolID = -1;
				insOp.machID = -1;

				operations.insert(insPos, insOp);

				ordID2NumSpareOps[curOrdID]++;
			}

			ordID2NumSpareOps[curOrdID] = 0;
		}
		 */

	}

	// ########## For each order generate a BOP and insert it ##############

	QList<QPair<int, int> > childPrec; // Precedence relations between the operations of the child genome

	QSet<int> prodIDBOPGen; // Products with generated BOPs (to avoid generation for each order of the product)

	foreach(const int& curOrdID, ordIDsToRepair) {

		Order& curOrd = pmm->ordman->orderByID(curOrdID);

		Product& curProd = pmm->prodman->productByID(curOrd.type);

		int curBOMID = prodID2bomIDs[curProd.ID].toList()[0]; // There is only one BOM selected right now

		BillOfProcesses& curBOP = (BillOfProcesses&) * curProd.bopByID(curBOMID);

		if (!prodIDBOPGen.contains(curBOP.ID)) { // To avoid different routes in pars belonging to different orders of the same product

			// Set the routes in the BOP
			curBOP.setItemRouteIdxRnd();

			prodIDBOPGen << curBOP.ID;
		}

		// Generate the current BOP
		curBOP.generate();

		//out << "Current BOP : " << endl << curBOP << endl;

		//*********** Topological ordering of the nodes in the BOP *************
		ListDigraph::NodeMap<int> nodes2pos(curBOP.graph()); // Map of nodes sorted topologically

		// Sort the nodes topologically
		topologicalSort(curBOP.graph(), nodes2pos);

		QMap<int, ListDigraph::Node> pos2nodes; // Topologically sorted nodes 

		for (ListDigraph::NodeMap<int>::MapIt mi(nodes2pos); mi != INVALID; ++mi) {
			pos2nodes[*mi] = mi;
		}

		QList<ListDigraph::Node> tnodes;

		tnodes.clear();
		tnodes.reserve(pos2nodes.size());
		for (QMap<int, ListDigraph::Node>::iterator sti = pos2nodes.begin(); sti != pos2nodes.end(); sti++) {
			tnodes << sti.value();
		}
		tnodes.removeOne(curBOP.graphHead());
		tnodes.removeOne(curBOP.graphTail());

		//**********************************************************************


		// Get the operations' positions for the current order
		QList<int> opPos;
		for (int i = 0; i < operations.size(); i++) {
			Operation& curOp = (Operation&) operations[i];
			if (curOp.OID == curOrdID) {
				opPos << i;
			}
		}

		// Iterate over the topologically sorted nodes of the BOP
		QMap<ListDigraph::Node, int> bopNode2OpIdx;
		for (int i = 0; i < tnodes.size(); i++) {
			int curPos = opPos.takeFirst();
			ListDigraph::Node curNode = tnodes[i];
			Operation& curOp = (Operation&) operations[curPos];

			curOp.ID = (curOrdID << 16) + i + 1; // ID of the operations depending on the order's ID

			curOp.BID = curBOMID;

			curOp.IID = curBOP.oLocItemID()[curNode];

			curOp.IT = curBOP.itemID2ItemType()[curOp.IID];

			curOp.RID = curBOP.itemRoutes()[curOp.IID][curBOP.itemRouteIdx()[curOp.IID]]->ID;

			// Correct the item ID
			curOp.IID = (curOrdID << 16) + curOp.IID;

			curOp.SI = -1;

			curOp.type = curBOP.otypeID()[curNode];

			rc.assign(&curOp); // Assign to the machine group -> sets the toolID





			bool randomMachSelect = false;





			if (randomMachSelect) {
				// Select the machine ID randomly
				curOp.machID = rc(curOp.toolID).machines(curOp.type)[Rand::rndInt(0, rc(curOp.toolID).machines(curOp.type).size() - 1)]->ID;
				//curOp.machID = -1;
			} else {
				// Select the machine ID inherited
				QList<int>& curToolIDSeq = ordID2toolIDSeq[curOrdID];
				QList<int>& curMachIDSeq = ordID2machIDSeq[curOrdID];

				int machIdx = -1;
				for (int k = 0; k < curToolIDSeq.size(); k++) {
					if (curToolIDSeq[k] == curOp.toolID) {
						machIdx = k;
						break;
					}
				}

				if (machIdx == -1) { // Nothing found
					curOp.machID = rc(curOp.toolID).machines(curOp.type)[Rand::rndInt(0, rc(curOp.toolID).machines(curOp.type).size() - 1)]->ID;
				} else {
					// IMPORTANT!!! If there are machines with different functionality in one machine group => there is a problem!!!
					curOp.machID = curMachIDSeq[machIdx];

					curMachIDSeq.removeAt(machIdx);
					curToolIDSeq.removeAt(machIdx);
				}
			}





			bopNode2OpIdx[curNode] = curPos;

		}

		// Collect the precedence constraints between the operations
		for (ListDigraph::ArcIt ait(curBOP.graph()); ait != INVALID; ++ait) {

			ListDigraph::Node startNode = curBOP.graph().source(ait);
			ListDigraph::Node endNode = curBOP.graph().target(ait);

			if (startNode == curBOP.graphHead() || endNode == curBOP.graphTail()) {
				continue;
			} else {
				QPair<int, int> curPrec;

				curPrec.first = operations[bopNode2OpIdx[startNode]].ID;

				curPrec.second = operations[bopNode2OpIdx[endNode]].ID;

				childPrec << curPrec;
			}

		}

		// Degenerate the current BOP
		curBOP.degenerate();
	}
	// #####################################################################

	// ########## Collect the precedence constraints ###########################

	// Collect the precedence constraint from the mom's genome (ONLY FOR THE ORDERS WHICH MUST NOT BE CHANGED)
	QList<QPair<int, int> > momPrec;
	QHash<int, int> momOpID2OrdID;
	for (int i = 0; i < mom.operations.size(); i++) {

		Operation& curOp = (Operation&) mom.operations[i];

		momOpID2OrdID[curOp.ID] = curOp.OID;

	}

	// IMPORTANT!!! No operation precedences are inherited from the dad, since all of the corresponding orders are recreated

	for (int i = 0; i < mom.schedule.operationPrecedences.size(); i++) {
		if (mom.schedule.operationPrecedencesConj[i]) {

			int opIDStart = mom.schedule.operationPrecedences[i].first;
			int opIDEnd = mom.schedule.operationPrecedences[i].second;

			// Check whether the operations belong to a repaired order, or not
			if (ordIDsToRepair.contains(momOpID2OrdID[opIDStart]) || ordIDsToRepair.contains(momOpID2OrdID[opIDEnd])) { // These operations are removed in the child genome
				continue;
			} else { // These operations are inherited by the child and are not touched
				momPrec << mom.schedule.operationPrecedences[i];
			}
		}
	}

	/*
	out << "Precedence constraints in the mom's genome : " << endl;
	for (int i = 0; i < momPrec.size(); i++) {
		out << momPrec[i].first << "->" << momPrec[i].second << endl;
	}
	 */

	// Merge all precedence constraints
	childPrec << momPrec;

	/*
	out << "Precedence constraints in the child's genome : " << endl;
	for (int i = 0; i < childPrec.size(); i++) {
		out << childPrec[i].first << "->" << childPrec[i].second << endl;
	}
	 */

	// #########################################################################

	// ### Perform scheduling under consideration of precedences ###############

	QHash<int, QList<int> > opID2SuccOpIDs; // Direct successors of the current operation
	QHash<int, QList<int> > opID2PredOpIDs; // Direct successors of the current operation
	for (int i = 0; i < childPrec.size(); i++) {
		int startOpID = childPrec[i].first;
		int endOpID = childPrec[i].second;

		opID2SuccOpIDs[startOpID] << endOpID;
		opID2PredOpIDs[endOpID] << startOpID;
	}

	QHash<int, double> opID2EST; // The earliest start time of each operation (the operations are sorted topologically)

	for (int i = 0; i < operations.size(); i++) {
		opID2EST[operations[i].ID] = 0.0;
	}

	rc.init();

	// Schedule the operations
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		// Set the earliest start time of the operations
		curOp.r(opID2EST[curOp.ID]);

		if (false/*curOp.machID == -1*/) {

			// Select the machine able to finish the operation the earliest
			Machine& m = rc(curOp.toolID).earliestToFinish(&curOp);

			m << &curOp;

		} else {

			// Set the operation's machine
			Machine& m = rc(curOp.toolID, curOp.machID);

			m << &curOp;

		}


		// Update the earliest start time of the direct successors
		QList<int> curSuccIDs = opID2SuccOpIDs[curOp.ID];

		for (int j = 0; j < curSuccIDs.size(); j++) {
			int curSuccID = curSuccIDs[j];

			opID2EST[curSuccID] = Math::max(opID2EST[curSuccID], curOp.c());
		}

	}

	// #########################################################################

	// #################### Calculate the objective ############################

	double curObj = 0.0;

	QList<Order>& orders = (QList<Order>&) pmm->ordman->orders;

	QHash<int, Operation> ordId2LastOper;

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ordId2LastOper[curOp.OID] = curOp;
	}

	for (int i = 0; i < orders.size(); i++) {

		Order& curOrd = (Order&) orders[i];

		double curOrdC = ordId2LastOper[curOrd.ID].c();
		double curOrdD = curOrd.d;
		double curOrdW = curOrd.w;

		if (schedOptions["OBJECTIVE"] == "Cmax") {

			curObj = Math::max(curObj, curOrdC); //*/ += curOrdW * Math::max(0.0, curOrdC - curOrdD);

		} else if (schedOptions["OBJECTIVE"] == "TWT") {

			curObj += curOrdW * Math::max(0.0, curOrdC - curOrdD);

		}

	}

	// #########################################################################

	// ###################### Build the schedule ###############################

	schedule.clear();

	schedule.objective = curObj;

	schedule.operations = operations;

	schedule.operationPrecedences = childPrec;

	schedule.operationPrecedencesConj.clear();
	for (int i = 0; i < schedule.operationPrecedences.size(); i++) {
		schedule.operationPrecedencesConj << true;
	}

	// Add the other operations precedences
	QList<Machine*> machs = rc.machines();

	for (int i = 0; i < machs.size(); i++) {

		Machine& curMach = (Machine&) *(machs[i]);

		for (int j = 0; j < curMach.operations.size() - 1; j++) {

			int startOpID = curMach.operations[j]->ID;
			int endOpID = curMach.operations[j + 1]->ID;

			schedule.operationPrecedences << QPair<int, int>(startOpID, endOpID);

			schedule.operationPrecedencesConj << false;

		}

	}

	operationPrecedences = schedule.operationPrecedences;
	operationPrecedencesConj = schedule.operationPrecedencesConj;

	/*
	out << "Operation precedences before restoring from PM : " << endl;
	for (int i = 0; i < schedule.operationPrecedences.size(); i++) {
		QPair<int, int> curPrec = schedule.operationPrecedences[i];
		out << curPrec.first << "->" << curPrec.second << " : " << schedule.operationPrecedencesConj[i] << endl;
	}
	 */

	ProcessModel restoredPM = restorePM();

	//out << "Restored PM : " << endl << restoredPM << endl;

#ifdef DEBUG

	int numDisjPrecConstr = 0;
	for (int i = 0; i < operationPrecedences.size(); i++) {
		if (operationPrecedencesConj[i]) {
			numDisjPrecConstr++;
		}
	}
	if (numDisjPrecConstr == 0) {
		Debugger::err << "GAPlannerGenome1::repair : No disjunctive precedence constraints!" << ENDL;
	}

	// out << schedule.pm << endl;

	if (Math::cmp(schedule.objective, (*obj)(restoredPM), 0.0000001) != 0) {

		out << "Schedule objective : " << schedule.objective << endl;
		out << "PM objective : " << (*obj)(restoredPM) << endl;

		Debugger::err << "GAPlannerGenome1::repair : Schedule/PM objective mismatch!" << ENDL;
	}
#endif

	schedule.fromPM(restoredPM, *obj);

	//out << "Objective after repairing : " << schedule.objective << endl;
	//getchar();

	/*
	out << "Operation precedences after restoring from PM : " << endl;
	for (int i = 0; i < schedule.operationPrecedences.size(); i++) {
		QPair<int, int> curPrec = schedule.operationPrecedences[i];
		out << curPrec.first << "->" << curPrec.second << " : " << schedule.operationPrecedencesConj[i] << endl;
	}
	 */

	// #########################################################################

	// ###################### Build the plan ###################################

	plan.clear();

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		plan.prodID2BOPID[curOp.OT] = curOp.BID;

		plan.prodID2ItemID2ItemType[curOp.OT][curOp.IID] = curOp.IT;

		plan.prodID2ItemID2RouteID[curOp.OT][curOp.IID] = curOp.RID;

		plan.prodID2ItemID2RouteIdx[curOp.OT][curOp.IID] = pmm->prodman->productByID(curOp.OT).bopByID(curOp.BID)->itemRouteIdx()[curOp.IID];

	}

	// #########################################################################

	//out << "Calculated objective : " << curObj << endl;

	if (!valid()) {
		Debugger::err << "GAPlannerGenome1::repair : Invalid genome after repairing!" << ENDL;
	}

}

float GAPlannerGenome1::compare(const GAGenome &other) {
	return 0.0;
}

float GAPlannerGenome1::compare(const GAGenome& g1, const GAGenome& g2) {
	return g1.compare(g2);
}

float GAPlannerGenome1::objective(GAGenome & gen) {
	//cout << "Genome:: calculating objective" << endl;
	GAPlannerGenome1& g = (GAPlannerGenome1&) gen;

	// Improve the genome
	if (g._solImprove) {
		g.improve();
	}

	return g.schedule.objective;
}

void GAPlannerGenome1::improve() {

	
	if (Rand::rndDouble() > 0.05) { // Only with some probability improve the solutions
		return;
	}
	QTextStream out(stdout);

	LocalSearchPM ls;

	RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator

	// Set the initial random number generator for the LS
	ls.setRandGen(randGen);

	//if (this->geneticAlgorithm()->generation() == this->geneticAlgorithm()->nGenerations() - 1) {
	//	ls.maxIter(schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"].toInt());
	//} else {
	ls.maxIter(_lsMaxIter);
	//}

	ls.checkCorrectness(_lsChkCor);

	ProcessModel pm = schedule.pm;
	rc.init();

	//out << "PM before improving : " << endl << pm << endl;

	//out << "GAPlannerGenome1::improve : TWT before improving : " << TWT()(pm) << endl;
	ls.setObjective(obj);
	ls.setPM(&pm);
	ls.setResources(&rc);

	//out << "Running the local search reoptimization ..." << endl;
	if (ls.maxIter() > 0) {
		ls.run();
	}

	//out << "GAPlannerGenome1::improve : TWT after improving : " << TWT()(pm) << endl;

	schedule.fromPM(pm, *obj);

	// Set the state of the genome according to the improved schedule
	operations = schedule.operations;
	operationPrecedences = schedule.operationPrecedences;
	operationPrecedencesConj = schedule.operationPrecedencesConj;

}

void GAPlannerGenome1::setPMM(ProcessModelManager* pmm) {
	this->pmm = pmm;
}

void GAPlannerGenome1::setResources(Resources& origRes) {
	this->rc = origRes;
}

void GAPlannerGenome1::setScheduler(Scheduler* scheduler) {

	this->scheduler = scheduler;

}

void GAPlannerGenome1::setOptions(SchedulerOptions& options) {

	this->schedOptions = options;

	_lsMaxIter = schedOptions["LS_MAX_ITER"].toInt();
	_lsChkCor = (schedOptions["LS_CHK_COR"] == "true");
	_solImprove = (schedOptions["GA_IMPROVE_SOLUTION"] == "true");

}

ProcessModel GAPlannerGenome1::restorePM() {
	ProcessModel pm;

	pm.head = pm.graph.addNode();
	pm.ops[pm.head] = new Operation();
	pm.ops[pm.head]->type = 0;
	pm.ops[pm.head]->ID = -10000002;
	pm.ops[pm.head]->p(0.0);

	pm.tail = pm.graph.addNode();
	pm.ops[pm.tail] = new Operation();
	pm.ops[pm.tail]->type = 0;
	pm.ops[pm.tail]->ID = -10000001;
	pm.ops[pm.tail]->p(0.0);

	QHash<int, QList<Operation> > ordID2Operaions;

	QList<Order>& orders = pmm->ordman->orders;

	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ordID2Operaions[curOp.OID] << curOp;

	}

	QHash<int, ListDigraph::Node> opID2Node;

	QHash<int, ListDigraph::Node> ordID2LHead;
	QHash<int, ListDigraph::Node> ordID2LTail;

	for (int i = 0; i < orders.size(); i++) {

		int curOrdID = orders[i].ID;

		int maxOrdOpCount = 0;

		for (int j = 0; j < ordID2Operaions[curOrdID].size(); j++) {

			ListDigraph::Node curNode = pm.graph.addNode();

			Operation& curOp = (Operation&) ordID2Operaions[curOrdID][j];

			opID2Node[curOp.ID] = curNode;

			pm.ops[curNode] = new Operation(curOp);

			maxOrdOpCount = Math::max(maxOrdOpCount, curOp.ID - (curOrdID << 16));

		}

		// Create two fake nodes for the order
		ListDigraph::Node lHead = pm.graph.addNode();
		pm.ops[lHead] = new Operation();
		pm.ops[lHead]->type = 0;
		pm.ops[lHead]->ID = -((curOrdID << 16) + maxOrdOpCount + 1);
		pm.ops[lHead]->OID = curOrdID;
		pm.ops[lHead]->toolID = 0;
		pm.ops[lHead]->machID = 0;
		pm.ops[lHead]->p(0.0);
		pm.ops[lHead]->w(orders[i].w);
		pm.ops[lHead]->d(orders[i].d);
		pm.ops[lHead]->ir(orders[i].r);
		ListDigraph::Node lTail = pm.graph.addNode();
		pm.ops[lTail] = new Operation();
		pm.ops[lTail]->type = 0;
		pm.ops[lTail]->ID = -((curOrdID << 16) + maxOrdOpCount + 2);
		pm.ops[lTail]->OID = curOrdID;
		pm.ops[lTail]->toolID = 0;
		pm.ops[lTail]->machID = 0;
		pm.ops[lTail]->p(0.0);
		pm.ops[lTail]->w(orders[i].w);
		pm.ops[lTail]->d(orders[i].d);
		pm.ops[lTail]->ir(orders[i].r);

		ListDigraph::Arc ordStartArc = pm.graph.addArc(pm.head, lHead);
		ListDigraph::Arc ordEndArc = pm.graph.addArc(lTail, pm.tail);

		pm.conjunctive[ordStartArc] = true;
		pm.conjunctive[ordEndArc] = true;

		ordID2LHead[curOrdID] = lHead;
		ordID2LTail[curOrdID] = lTail;

	}

	// Add the conjunctive precedence constraints between the operations to the graph
	for (int i = 0; i < operationPrecedences.size(); i++) {

		QPair<int, int> curPrec = operationPrecedences[i];
		bool curPrecConj = operationPrecedencesConj[i];

		if (curPrecConj) {

			ListDigraph::Node startNode = opID2Node[curPrec.first];
			ListDigraph::Node endNode = opID2Node[curPrec.second];

			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = true;
		}

	}

	// Add the connecting arcs between order head/tail nodes and the others
	for (int i = 0; i < operations.size(); i++) {

		Operation& curOp = (Operation&) operations[i];

		ListDigraph::Node curNode = opID2Node[curOp.ID];

		if (countInArcs(pm.graph, curNode) == 0 && curNode != ordID2LTail[curOp.OID]) { // Connect with the fake head
			ListDigraph::Node startNode = ordID2LHead[curOp.OID];
			ListDigraph::Node endNode = curNode;
			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = true;
		}

		if (countOutArcs(pm.graph, curNode) == 0 && curNode != ordID2LHead[curOp.OID]) { // Connect with the fake tail	
			ListDigraph::Node startNode = curNode;
			ListDigraph::Node endNode = ordID2LTail[curOp.OID];
			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = true;
		}

	}

	// Add the disjunctive precedence constraints between the operations to the graph
	for (int i = 0; i < operationPrecedences.size(); i++) {

		QPair<int, int> curPrec = operationPrecedences[i];
		bool curPrecConj = operationPrecedencesConj[i];

		if (!curPrecConj) {

			ListDigraph::Node startNode = opID2Node[curPrec.first];
			ListDigraph::Node endNode = opID2Node[curPrec.second];

			ListDigraph::Arc curArc = pm.graph.addArc(startNode, endNode);
			pm.conjunctive[curArc] = false;
		}

	}

	// Set correctly the lengths of the arcs
	for (ListDigraph::ArcIt ait(pm.graph); ait != INVALID; ++ait) {

		ListDigraph::Node startNode = pm.graph.source(ait);

		pm.p[ait] = -pm.ops[startNode]->p();

	}

	QList<ListDigraph::Node> ts = pm.topolSort();
	pm.updateHeads(ts);
	pm.updateStartTimes(ts);

	return pm;
}

void GAPlannerGenome1::setObjective(ScalarObjective* o) {
	this->obj = o;
	//schedule << o;
}

/** ######################################################################### */

/** ###########################  GAPlanner  ########################### */

GAPlanner::GAPlanner() : Planner() {
	n_cycles = 100;
	n_genomes = 10;
	n_secs = 120;

	p_repl = 0.6;
	p_mut = 0.1;
	p_cross = 0.8;
}

GAPlanner::GAPlanner(const GAPlanner& orig) /*: Planner(orig)*/ {
	n_cycles = orig.n_cycles;
	n_genomes = orig.n_genomes;
	n_secs = orig.n_secs;

	p_repl = orig.p_repl;
	p_mut = orig.p_mut;
	p_cross = orig.p_cross;

}

GAPlanner::~GAPlanner() {
}

void GAPlanner::init() {

}

void GAPlanner::run(GAPlannerGenome &res) {

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) setting statics...");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) statics set.");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) creating GA...");

	GAPlannerGenome genome;

	// Create a population of the specific genomes which "know" the concrete static specific operators.
	GAPopulation population;
	population.size(0);
	// Add genomes with random initialization
	for (int i = 0; i < n_genomes; i++) {
		population.add(genome);
	}

	GASteadyStateGA ga(population);

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) GA created.");


	/*
	for (int i = 0; i < population.size(); i++) {
		((AGAGenome&) population.individual(i)).write(cout);
	}
	return;
	 */


	Debugger::wDebug("GAPlanner::run : Adjusting GA...");
	// Perform minimization
	ga.minimize();

	// Make the appropriate adjustments for the algorithm
	//ga.populationSize(n_genes);
	ga.nGenerations(n_cycles);
	ga.pReplacement(p_repl);
	ga.pMutation(p_mut);
	ga.pCrossover(p_cross);
	ga.initialize(seed);
	ga.scoreFrequency(0.1);
	ga.flushFrequency(0.1);
	ga.selectScores(GAStatistics::Deviation);
	Debugger::wDebug("GAPlanner::run : GA adjusted.");

	//for (int i = 0; i < population.size(); i++) {
	//    if (!((AGAGenome&) population.individual(i)).valid())// population.remove(i);
	//    Debugger::wDebug("Removed invalid genome during initialization!");
	//}

	double best_fitness;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores...");

	cout << "Best initial Obj : " << ga.statistics().bestIndividual().score() << endl;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores done.");

	cout << "Starting evolution ... " << endl;
	time_t start_t;
	time_t t;
	int nodevsteps = 0;

	start_t = time(NULL);
	while (!ga.done()) {
		//cout<<"Entered main loop.";
		//cout.flush();
		ga.step();
		//cout<<"First step performed.";
		//cout.flush();
		if (ga.statistics().generation() % 50 == 0) { // Every 10-th generation
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << endl;
		} else {
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << '\r';
		}
		cout.flush();

		t = time(NULL);
		if (t >= start_t + n_secs) {
			cout << endl << "Time out for the test instance. Terminating ..." << endl;
			break;
		}

		if (ga.statistics().bestIndividual().score() == 0) {
			cout << endl << "Best solution found. Terminating ..." << endl;
			break;
		}

		if (ga.population().dev() > 0.0) {
			nodevsteps = 0;
		}

		if (ga.population().dev() == 0.0) {
			nodevsteps++;
			if (nodevsteps > 200) {
				cout << endl << "No deviation. Terminating ..." << endl;
				break;
			}
		}

	}

	best_fitness = ga.statistics().bestIndividual().score();

	cout << "Number of iterations => " << ga.statistics().generation() << endl;
	cout << "Best Obj => " << best_fitness << endl << endl;

	//ga.statistics().bestIndividual().write(cout);

	//Debugger::wDebug("Copying best genome ...");
	res.copy(ga.statistics().bestIndividual());
	//Debugger::wDebug("Best genome copied.");


}

void GAPlanner::run() {

	QTextStream out(stdout);

	out << "GAPlanner::run : Will run the GA algorithm for IPPS." << endl;

	_curtime.restart();

	GAPlannerGenome res;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) setting statics...");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) statics set.");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) creating GA...");

	GAPlannerGenome genome;

	// Set the process model manager
	genome.setPMM(pmm);

	// Set the resources
	genome.setResources(*rc);

	// Set the scheduler
	genome.setScheduler(scheduler);

	// Set the options
	genome.setOptions(schedOptions);

	// Set the objective
	genome.setObjective(objective);

	// A random scheduler
	Scheduler* altScheduler;
	altScheduler = new CombinedScheduler;

	altScheduler->obj = scheduler->obj->clone();

	Scheduler* sch = 0;

	sch = new WFIFOScheduler;
	sch->ID = 1;
	sch->obj = objective->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(false);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WFIFOScheduler;
	sch->ID = 2;
	sch->obj = objective->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(true);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WEODScheduler;
	sch->ID = 3;
	sch->obj = objective->clone();
	((WEODScheduler*) sch)->weightedEOD(false);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WEODScheduler;
	sch->ID = 4;
	sch->obj = objective->clone();
	((WEODScheduler*) sch)->weightedEOD(true);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WMODScheduler;
	sch->ID = 5;
	sch->obj = objective->clone();
	((WMODScheduler*) sch)->weightedMOD(false);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WMODScheduler;
	sch->ID = 6;
	sch->obj = objective->clone();
	((WMODScheduler*) sch)->weightedMOD(true);
	*((CombinedScheduler*) altScheduler) << sch;

	// Random scheduler
	RNDScheduler* rndScheduler = new RNDScheduler;
	rndScheduler->obj = scheduler->obj->clone();

	// Create a population of the specific genomes which "know" the concrete static specific operators.
	GAPopulation population;
	population.size(0);
	// Add genomes with random initialization
	for (int i = 0; i < n_genomes; i++) {

		double rndDbl = Rand::rndDouble();

		if (rndDbl < 0.5) { // Set a random scheduler in 95% if genomes

			genome.setScheduler(rndScheduler);

		} else if (rndDbl < 0.85) {

			genome.setScheduler(altScheduler);

		} else {

			genome.setScheduler(scheduler);

		}

		population.add(genome);
	}

	GASteadyStateGA ga(population);

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) GA created.");


	/*
	for (int i = 0; i < population.size(); i++) {
		((AGAGenome&) population.individual(i)).write(cout);
	}
	return;
	 */


	Debugger::wDebug("GAPlanner::run : Adjusting GA...");
	// Perform minimization
	ga.minimize();

	// Make the appropriate adjustments for the algorithm
	//ga.populationSize(n_genes);
	ga.nGenerations(n_cycles);
	ga.pReplacement(p_repl);
	ga.pMutation(p_mut);
	ga.pCrossover(p_cross);
	ga.initialize(seed);
	ga.scoreFrequency(0.1);
	ga.flushFrequency(0.1);
	ga.selectScores(GAStatistics::Deviation);
	Debugger::wDebug("GAPlanner::run : GA adjusted.");

	//for (int i = 0; i < population.size(); i++) {
	//    if (!((AGAGenome&) population.individual(i)).valid())// population.remove(i);
	//    Debugger::wDebug("Removed invalid genome during initialization!");
	//}

	double best_fitness;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores...");

	bestSched = ((GAPlannerGenome&) (ga.statistics().bestIndividual())).schedule;
	cout << "Best initial Obj : " << bestSched.objective /*ga.statistics().bestIndividual().score()*/ << endl;

	//getchar();

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores done.");

	preprocessingActions();

	slotUpdateProtocol();
	_curiter++;

	cout << "Starting evolution ... " << endl;
	time_t start_t;
	time_t t;
	int nodevsteps = 0;

	start_t = time(NULL);
	while (!ga.done()) {
		//cout<<"Entered main loop.";
		//cout.flush();
		ga.step();
		//cout<<"First step performed.";

		if (((GAPlannerGenome&) (ga.statistics().bestIndividual())).schedule.objective <= bestSched.objective) {
			bestSched = ((GAPlannerGenome&) (ga.statistics().bestIndividual())).schedule;
			slotUpdateProtocol();
		}

		_curiter++;

		//cout.flush();
		if (ga.statistics().generation() % 50 == 0) { // Every 10-th generation
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << endl;
		} else {
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << '\r';
		}
		cout.flush();

		t = time(NULL);
		if (t >= start_t + n_secs) {
			cout << endl << "Time out for the test instance. Terminating ..." << endl;
			break;
		}

		if (ga.statistics().bestIndividual().score() == 0) {
			cout << endl << "Best solution found. Terminating ..." << endl;
			break;
		}

		if (ga.population().dev() > 0.0) {
			nodevsteps = 0;
		}

		if (ga.population().dev() == 0.0) {
			nodevsteps++;
			if (nodevsteps > 2000) {
				cout << endl << "No deviation. Terminating ..." << endl;
				break;
			}
		}

	}

	cout << "Improving the best individual..." << endl;

	res.copy(ga.statistics().bestIndividual());

	cout << "Objective before improvement : " << res.score() << endl;

	res.schedOptions["LS_MAX_ITER"] = res.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"];

	schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	res.setOptions(res.schedOptions);

	res.improve();

	res.evaluate(gaTrue);

	cout << "Objective after improvement : " << res.score() << endl;

	bestSched = res.schedule;
	slotUpdateProtocol();

	cout << "Done improving the best individual." << endl;

	best_fitness = res.score();

	cout << "Number of iterations => " << ga.statistics().generation() << "                               " << endl;
	cout << "Best Obj => " << best_fitness << endl << endl;

	//ga.statistics().bestIndividual().write(cout);

	//Debugger::wDebug("Copying best genome ...");
	res.copy(ga.statistics().bestIndividual());
	//Debugger::wDebug("Best genome copied.");

	postprocessingActions();

	emit sigFinished();
}

void GAPlanner::preprocessingActions() {

	QDomDocument run_start_doc("run start");
	QDomElement run_start_elem = run_start_doc.createElement("start");
	//QDomElement obj_elem = newsol_doc.createElement("obj");

	run_start_doc.appendChild(run_start_elem);
	//run_end_elem.setAttribute("iterations", iter());
	run_start_elem.setAttribute("timestamp", QTime::currentTime().toString());
	//run_end_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	//newsol_elem.appendChild(obj_elem);

	protocol << run_start_doc;

	protocol.save();

}

void GAPlanner::postprocessingActions() {

	QTextStream out(stdout);

	Debugger::info << "GAPlanner::postprocessingActions : Postprocessing the results" << ENDL;

	//out << "Final resource assignment:" << endl;
	//out << *rc << endl;

	out << bestSched << endl;

	out << bestSched.pm << endl;

	out << "GAPlanner::postprocessingActions : Best objective : " << bestSched.objective << endl;

	// Write the protocol
	QDomDocument run_end_doc("run summary");
	QDomElement run_end_elem = run_end_doc.createElement("finish");
	//QDomElement obj_elem = newsol_doc.createElement("obj");

	QTime elapsed_hrs;
	elapsed_hrs.setHMS(0, 0, 0, 0);
	elapsed_hrs = elapsed_hrs.addMSecs(_curtime.elapsed());

	run_end_doc.appendChild(run_end_elem);
	run_end_elem.setAttribute("iterations", n_cycles);
	run_end_elem.setAttribute("timestamp", QTime::currentTime().toString());
	run_end_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	run_end_elem.setAttribute("elapsed_hrsm", elapsed_hrs.toString("HH:mm:ss:zzz"));
	//newsol_elem.appendChild(obj_elem);

	protocol << run_end_doc;

	/*
	QDomNodeList solNodes = protocol.elementsByTagName("solution");
	double bestObj = Math::MAX_DOUBLE;
	int bestSolIdx = -1;
	for (int i = 0 ; i < solNodes.size() ; i++){
			out << solNodes.at(i).toElement().attribute("iteration") << endl;
			out << solNodes.at(i).toElement().elementsByTagName("solution").at(0).childNodes().at(0).toElement().text() << endl;
	}
	 */

	protocol.save();

}

void GAPlanner::slotUpdateProtocol() {
	// Update the protocol
	QTextStream out(stdout);
	QDomDocument newsol_doc("new solution");
	QDomElement newsol_elem = newsol_doc.createElement("solution");
	QDomElement obj_elem = newsol_doc.createElement("obj");

	//out << " GAPlanner::slotUpdateProtocol : Updating the protocol " << endl;

	QTime elapsed_hrs;
	elapsed_hrs.setHMS(0, 0, 0, 0);
	elapsed_hrs = elapsed_hrs.addMSecs(_curtime.elapsed());
	newsol_doc.appendChild(newsol_elem);
	newsol_elem.setAttribute("iteration", (int) iter());
	newsol_elem.setAttribute("timestamp", QTime::currentTime().toString());
	newsol_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	newsol_elem.setAttribute("elapsed_hrsm", elapsed_hrs.toString("HH:mm:ss:zzz"));
	newsol_elem.appendChild(obj_elem);

	obj_elem.appendChild(newsol_doc.createTextNode(QString::number(bestSched.objective)));

	protocol << newsol_doc;

	//out << " GAPlanner::slotUpdateProtocol : Done updating the protocol " << endl;

	//protocol.save();
}

/** ######################################################################### */

/** ###########################  GAPlanner1  ########################### */

GAPlanner1::GAPlanner1() : Planner() {
	n_cycles = 100;
	n_genomes = 10;
	n_secs = 120;

	p_repl = 0.6;
	p_mut = 0.1;
	p_cross = 0.8;
}

GAPlanner1::GAPlanner1(const GAPlanner1& orig) /*: Planner(orig)*/ {
	n_cycles = orig.n_cycles;
	n_genomes = orig.n_genomes;
	n_secs = orig.n_secs;

	p_repl = orig.p_repl;
	p_mut = orig.p_mut;
	p_cross = orig.p_cross;

}

GAPlanner1::~GAPlanner1() {
}

void GAPlanner1::init() {

}

void GAPlanner1::run(GAPlannerGenome1 &res) {

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) setting statics...");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) statics set.");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) creating GA...");

	GAPlannerGenome1 genome;

	// Create a population of the specific genomes which "know" the concrete static specific operators.
	GAPopulation population;
	population.size(0);
	// Add genomes with random initialization
	for (int i = 0; i < n_genomes; i++) {
		population.add(genome);
	}

	GASteadyStateGA ga(population);

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) GA created.");


	/*
	for (int i = 0; i < population.size(); i++) {
		((AGAGenome&) population.individual(i)).write(cout);
	}
	return;
	 */


	Debugger::wDebug("GAPlanner::run : Adjusting GA...");
	// Perform minimization
	ga.minimize();

	// Make the appropriate adjustments for the algorithm
	//ga.populationSize(n_genes);
	ga.nGenerations(n_cycles);
	ga.pReplacement(p_repl);
	ga.pMutation(p_mut);
	ga.pCrossover(p_cross);
	ga.initialize(seed);
	ga.scoreFrequency(0.1);
	ga.flushFrequency(0.1);
	ga.selectScores(GAStatistics::Deviation);
	Debugger::wDebug("GAPlanner::run : GA adjusted.");

	//for (int i = 0; i < population.size(); i++) {
	//    if (!((AGAGenome&) population.individual(i)).valid())// population.remove(i);
	//    Debugger::wDebug("Removed invalid genome during initialization!");
	//}

	double best_fitness;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores...");

	cout << "Best initial Obj : " << ga.statistics().bestIndividual().score() << endl;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores done.");

	cout << "Starting evolution ... " << endl;
	time_t start_t;
	time_t t;
	int nodevsteps = 0;

	start_t = time(NULL);
	while (!ga.done()) {
		//cout<<"Entered main loop.";
		//cout.flush();
		ga.step();
		//cout<<"First step performed.";
		//cout.flush();
		if (ga.statistics().generation() % 50 == 0) { // Every 10-th generation
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << endl;
		} else {
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << '\r';
		}
		cout.flush();

		t = time(NULL);
		if (t >= start_t + n_secs) {
			cout << endl << "Time out for the test instance. Terminating ..." << endl;
			break;
		}

		if (ga.statistics().bestIndividual().score() == 0) {
			cout << endl << "Best solution found. Terminating ..." << endl;
			break;
		}

		if (ga.population().dev() > 0.0) {
			nodevsteps = 0;
		}

		if (ga.population().dev() == 0.0) {
			nodevsteps++;
			if (nodevsteps > 200) {
				cout << endl << "No deviation. Terminating ..." << endl;
				break;
			}
		}

	}

	best_fitness = ga.statistics().bestIndividual().score();

	cout << "Number of iterations => " << ga.statistics().generation() << endl;
	cout << "Best Obj => " << best_fitness << endl << endl;

	//ga.statistics().bestIndividual().write(cout);

	//Debugger::wDebug("Copying best genome ...");
	res.copy(ga.statistics().bestIndividual());
	//Debugger::wDebug("Best genome copied.");


}

void GAPlanner1::run() {

	QTextStream out(stdout);

	out << "GAPlanner1::run : Will run the GA algorithm for IPPS." << endl;

	_curtime.restart();

	GAPlannerGenome1 res;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) setting statics...");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) statics set.");

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) creating GA...");

	GAPlannerGenome1 genome;

	// Set the process model manager
	genome.setPMM(pmm);

	// Set the resources
	genome.setResources(*rc);

	// Set the scheduler
	genome.setScheduler(scheduler);

	// Set the options
	genome.setOptions(schedOptions);

	// Set the objective
	genome.setObjective(objective);

	// A random scheduler
	Scheduler* altScheduler;
	altScheduler = new CombinedScheduler;

	altScheduler->obj = scheduler->obj->clone();

	Scheduler* sch = 0;

	sch = new WFIFOScheduler;
	sch->ID = 1;
	sch->obj = objective->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(false);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WFIFOScheduler;
	sch->ID = 2;
	sch->obj = objective->clone();
	((WFIFOScheduler*) sch)->weightedFIFO(true);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WEODScheduler;
	sch->ID = 3;
	sch->obj = objective->clone();
	((WEODScheduler*) sch)->weightedEOD(false);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WEODScheduler;
	sch->ID = 4;
	sch->obj = objective->clone();
	((WEODScheduler*) sch)->weightedEOD(true);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WMODScheduler;
	sch->ID = 5;
	sch->obj = objective->clone();
	((WMODScheduler*) sch)->weightedMOD(false);
	*((CombinedScheduler*) altScheduler) << sch;

	sch = new WMODScheduler;
	sch->ID = 6;
	sch->obj = objective->clone();
	((WMODScheduler*) sch)->weightedMOD(true);
	*((CombinedScheduler*) altScheduler) << sch;

	// Random scheduler
	RNDScheduler* rndScheduler = new RNDScheduler;
	rndScheduler->obj = scheduler->obj->clone();


	// Create a population of the specific genomes which "know" the concrete static specific operators.
	GAPopulation population;
	population.size(0);
	// Add genomes with random initialization
	for (int i = 0; i < n_genomes; i++) {

		double rndDbl = Rand::rndDouble();

		if (rndDbl < 0.5) { // Set a random scheduler in 95% if genomes

			genome.setScheduler(rndScheduler);

		} else if (rndDbl < 0.85) {

			genome.setScheduler(altScheduler);

		} else {

			genome.setScheduler(scheduler);

		}

		population.add(genome);
	}

	GASteadyStateGA ga(population);

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) GA created.");


	/*
	for (int i = 0; i < population.size(); i++) {
		((AGAGenome&) population.individual(i)).write(cout);
	}
	return;
	 */


	Debugger::wDebug("GAPlanner1::run : Adjusting GA...");
	// Perform minimization
	ga.minimize();

	// Make the appropriate adjustments for the algorithm
	//ga.populationSize(n_genes);
	ga.nGenerations(n_cycles);
	ga.pReplacement(p_repl);
	ga.pMutation(p_mut);
	ga.pCrossover(p_cross);
	ga.initialize(seed);
	ga.scoreFrequency(0.1);
	ga.flushFrequency(0.1);
	ga.selectScores(GAStatistics::Deviation);
	Debugger::wDebug("GAPlanner1::run : GA adjusted.");

	//for (int i = 0; i < population.size(); i++) {
	//    if (!((AGAGenome&) population.individual(i)).valid())// population.remove(i);
	//    Debugger::wDebug("Removed invalid genome during initialization!");
	//}

	double best_fitness;

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores...");

	bestSched = ((GAPlannerGenome1&) (ga.statistics().bestIndividual())).schedule;
	cout << "Best initial Obj : " << bestSched.objective /*ga.statistics().bestIndividual().score()*/ << endl;

	//getchar();

	//Debugger::wDebug("AGATestInst::solve(AGAGenome *res, GAPopulation &population) initial scores done.");

	preprocessingActions();

	slotUpdateProtocol();
	_curiter++;

	cout << "Starting evolution ... " << endl;
	time_t start_t;
	time_t t;
	int nodevsteps = 0;

	start_t = time(NULL);
	while (!ga.done()) {
		//cout<<"Entered main loop.";
		//cout.flush();
		ga.step();
		//cout<<"First step performed.";

		if (((GAPlannerGenome1&) (ga.statistics().bestIndividual())).schedule.objective <= bestSched.objective) {
			bestSched = ((GAPlannerGenome1&) (ga.statistics().bestIndividual())).schedule;
			slotUpdateProtocol();
		}

		_curiter++;

		//cout.flush();
		if (ga.statistics().generation() % 50 == 0) { // Every 10-th generation
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << endl;
		} else {
			cout << setfill(' ') << setw(100) << '\r';
			cout << "Iter: " << setw(5) << ga.statistics().generation() << '\t';
			cout << "dev : " << ga.population().dev() << ", Obj : " << ga.statistics().bestIndividual().score() << '\r';
		}
		cout.flush();

		t = time(NULL);
		if (t >= start_t + n_secs) {
			cout << endl << "Time out for the test instance. Terminating ..." << endl;
			break;
		}

		if (ga.statistics().bestIndividual().score() == 0) {
			cout << endl << "Best solution found. Terminating ..." << endl;
			break;
		}

		if (ga.population().dev() > 0.0) {
			nodevsteps = 0;
		}

		if (ga.population().dev() == 0.0) {
			nodevsteps++;
			if (nodevsteps > 2000) {
				cout << endl << "No deviation. Terminating ..." << endl;
				break;
			}
		}

	}

	cout << "Improving the best individual..." << endl;

	res.copy(ga.statistics().bestIndividual());

	cout << "Objective before improvement : " << res.score() << endl;

	res.schedOptions["LS_MAX_ITER"] = res.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"];

	schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	res.setOptions(res.schedOptions);

	res.improve();

	res.evaluate(gaTrue);

	cout << "Objective after improvement : " << res.score() << endl;

	bestSched = res.schedule;
	slotUpdateProtocol();

	cout << "Done improving the best individual." << endl;

	best_fitness = res.score();

	cout << "Number of iterations => " << ga.statistics().generation() << "                               " << endl;
	cout << "Best Obj => " << best_fitness << endl << endl;

	//ga.statistics().bestIndividual().write(cout);

	//Debugger::wDebug("Copying best genome ...");
	res.copy(ga.statistics().bestIndividual());
	//Debugger::wDebug("Best genome copied.");

	postprocessingActions();

	emit sigFinished();
}

void GAPlanner1::preprocessingActions() {

	QDomDocument run_start_doc("run start");
	QDomElement run_start_elem = run_start_doc.createElement("start");
	//QDomElement obj_elem = newsol_doc.createElement("obj");

	run_start_doc.appendChild(run_start_elem);
	//run_end_elem.setAttribute("iterations", iter());
	run_start_elem.setAttribute("timestamp", QTime::currentTime().toString());
	//run_end_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	//newsol_elem.appendChild(obj_elem);

	protocol << run_start_doc;

	protocol.save();

}

void GAPlanner1::postprocessingActions() {

	QTextStream out(stdout);

	Debugger::info << "GAPlanner::postprocessingActions : Postprocessing the results" << ENDL;

	//out << "Final resource assignment:" << endl;
	//out << *rc << endl;

	out << bestSched << endl;

	out << bestSched.pm << endl;

	out << "GAPlanner::postprocessingActions : Best objective : " << bestSched.objective << endl;

	// Write the protocol
	QDomDocument run_end_doc("run summary");
	QDomElement run_end_elem = run_end_doc.createElement("finish");
	//QDomElement obj_elem = newsol_doc.createElement("obj");

	QTime elapsed_hrs;
	elapsed_hrs.setHMS(0, 0, 0, 0);
	elapsed_hrs = elapsed_hrs.addMSecs(_curtime.elapsed());

	run_end_doc.appendChild(run_end_elem);
	run_end_elem.setAttribute("iterations", n_cycles);
	run_end_elem.setAttribute("timestamp", QTime::currentTime().toString());
	run_end_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	run_end_elem.setAttribute("elapsed_hrsm", elapsed_hrs.toString("HH:mm:ss:zzz"));
	//newsol_elem.appendChild(obj_elem);

	protocol << run_end_doc;

	/*
	QDomNodeList solNodes = protocol.elementsByTagName("solution");
	double bestObj = Math::MAX_DOUBLE;
	int bestSolIdx = -1;
	for (int i = 0 ; i < solNodes.size() ; i++){
			out << solNodes.at(i).toElement().attribute("iteration") << endl;
			out << solNodes.at(i).toElement().elementsByTagName("solution").at(0).childNodes().at(0).toElement().text() << endl;
	}
	 */

	protocol.save();

}

void GAPlanner1::slotUpdateProtocol() {
	// Update the protocol
	QTextStream out(stdout);
	QDomDocument newsol_doc("new solution");
	QDomElement newsol_elem = newsol_doc.createElement("solution");
	QDomElement obj_elem = newsol_doc.createElement("obj");

	//out << " GAPlanner::slotUpdateProtocol : Updating the protocol " << endl;

	QTime elapsed_hrs;
	elapsed_hrs.setHMS(0, 0, 0, 0);
	elapsed_hrs = elapsed_hrs.addMSecs(_curtime.elapsed());
	newsol_doc.appendChild(newsol_elem);
	newsol_elem.setAttribute("iteration", (int) iter());
	newsol_elem.setAttribute("timestamp", QTime::currentTime().toString());
	newsol_elem.setAttribute("elapsed_ms", QString::number(_curtime.elapsed()));
	newsol_elem.setAttribute("elapsed_hrsm", elapsed_hrs.toString("HH:mm:ss:zzz"));
	newsol_elem.appendChild(obj_elem);

	obj_elem.appendChild(newsol_doc.createTextNode(QString::number(bestSched.objective)));

	protocol << newsol_doc;

	//out << " GAPlanner::slotUpdateProtocol : Done updating the protocol " << endl;

	//protocol.save();
}

/** ######################################################################### */
