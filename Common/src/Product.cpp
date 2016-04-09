/* 
 * File:   Product.cpp
 * Author: DrSobik
 * 
 * Created on July 29, 2011, 10:30 AM
 */

#include "Product.h"
#include "ProcessModel.h"

Product::Product() {
	bopid2idx.clear();
	bops.clear();
	//decompbopids.clear();
	//bopsDecompRate(0.0);
}

Product::~Product() {
}

double Product::expCost(Resources& rc) {
	double res = 0.0;

	for (int i = 0; i < bops.size(); i++) {
		res += bops[i]->expCost(rc);
	}

	if (bops.size() == 0) {
		Debugger::err << "Product::expCost : No BOPs specified for product " << ID << ENDL;
	}

	res /= double(bops.size());

	return res;
}

void Product::rankBOPs(Resources& rc) {
	// Rank the BOPs based on the expected total processing time
	double expBOPCost = 0.0;
	bopsRank.resize(bops.size());

	for (int i = 0; i < bops.size(); i++) {
            
		expBOPCost = bops[i]->expCost(rc/**rc*/);
		if (expBOPCost < 0.00000001) {
			bopsRank[i] = 0.0; // This is an empty BOP
		}

		bopsRank[i] = 1.0 / expBOPCost;
	}

	for (int i = 0; i < bops.size(); i++) {
		//Debugger::info << "Rank of BOP " << bops[i]->ID << " is " << bopsRank[i] << ENDL;
	}

}

BillOfProcesses* Product::selectBOPByRank() {
	//Debugger::info << "Selecting BOP by rank... " << ENDL;
	int idx = Rand::probSelectIdx(bopsRank, 1.0);//Rand::probSelect(bopsRank, 1.0);
	//Debugger::info << "Index :  " << idx << ENDL;
	BillOfProcesses* b = bops[idx];
	//Debugger::info << "Selected BOP by rank: " << b->ID << ENDL;
	return b;
}

BillOfProcesses* Product::bestBOPByRank() {
	// Select the index of the BOP with the highest ranking
	int idx = Math::maxIdx(bopsRank);
	BillOfProcesses* b = bops[idx];
	return b;
}

BillOfProcesses* Product::rndBOP() {
	//return bops[Rand::rndInt(0, bops.size() - 1)];
	return bops[Rand::rnd<Math::uint32>(0, bops.size() - 1)];
}

/*
void Product::bopsDecompRate(const double &rate) {
	if (rate < 0.0 || rate > 1.0) {
	Debugger::eDebug("Trying to set bopsdecomprate not from [0.0, 1.0] !");
	}

	_bopsdecomprate = rate;

	for (int i = 0; i < bops.size(); i++) {
	//bops[i]->generate();
	}

	// Decompose the set of BOPs
	decompBOPSet();

	for (int i = 0; i < bops.size(); i++) {
	//bops[i]->degenerate();
	}

}
 */

/*
void Product::decompBOPSet() {
 //
 * Algorithm:
 * 
 * 1. Estimate the maximum size (maxsize) of the BOP subsets.
 * 2. For all yet ungrouped BOPs
 * 2.a) Build matrix of pairwise differences;
 * 2.b) Select BOP* with the most differences to the otherBOPs and add it to
 *		the new subset;
 * 2.c) Select maxsize-1 BOPs which have the least differences from the BOP*
 *		and add them to the set with BOP*.
 * 2.d) Go to 2.
 *  
 *

	QTextStream out(stdout);

	if (bops.size() == 0) {
	Debugger::wDebug("Trying to decompose an empty set of BOPs.");
	return;
	}

	// Define maximum size of one subset of BOPs
	int maxsize = (int) Math::round(double(bops.size())*(1.0 - _bopsdecomprate));
	out << "maxsize = " << maxsize << endl;
	if (maxsize <= 0) maxsize = 1;

	// Clear the previous decomposed BOP IDs
	decompbopids.clear();

	// If no decomposition has to be done
	if (_bopsdecomprate == 0.0) {
	decompbopids.append(QList<int>());
	for (int i = 0; i < bops.size(); i++) {
		decompbopids[0].append(bops[i]->ID);
	}

	return;
	}

	QSet<int> ungrouped;
	for (int i = 0; i < bops.size(); i++) ungrouped.insert(bops[i]->ID);

	// Repeat until all of the BOPs are grouped. *
	QHash<int, double> totaldiffr; // Total differences from the other BOPs for each ungrouped BOP.
	QHash<int, QHash<int, double> > pairwisediffr; // Pairwise difference between each pair of the undgrouped BOPs.
	int mostdiffrid; // ID of the ungrouped BOP with the most differences
	int leastdiffrid; // ID of the ungrouped BOP with the least difference
	double maxdiffr;
	double mindiffr;
	while (ungrouped.size() > 0) {

	//out << "Ungrouped BOPs:" << endl;
	//out << "(";
	//for (int i = 0; i < ungrouped.size(); i++) {
	//	out << ungrouped.toList()[i] << " ";
	//}
	//out << ")" << endl;
	//getchar();

	totaldiffr.clear();
	pairwisediffr.clear();

	// Initialize
	for (QSet<int>::iterator it = ungrouped.begin(); it != ungrouped.end(); it++) {
		totaldiffr[*it] = 0.0;
		for (QSet<int>::iterator itoth = ungrouped.begin(); itoth != ungrouped.end(); itoth++) {
		pairwisediffr[*it][*itoth] = 0.0;
		}
	}

	// For each pair of BOPs get the difference between them
	for (QSet<int>::iterator it = ungrouped.begin(); it != ungrouped.end(); it++) {
		for (QSet<int>::iterator itoth = ungrouped.begin(); itoth != ungrouped.end(); itoth++) {
		pairwisediffr[*it][*itoth] = bopDifference(*it, *itoth);

		//out << "Diffr. of " << *it << " and " << *itoth << " : " << pairwisediffr[*it][*itoth] << endl;
		}
	}

	//getchar();

	// For each BOP get the total difference from the other BOPs
	for (QSet<int>::iterator it = ungrouped.begin(); it != ungrouped.end(); it++) {
		totaldiffr[*it] = 0.0;
		for (QSet<int>::iterator itoth = ungrouped.begin(); itoth != ungrouped.end(); itoth++) {
		totaldiffr[*it] += pairwisediffr[*it][*itoth];
		}
	}

	// Find the BOP with the most differences
	mostdiffrid = -1;
	maxdiffr = 0.0;
	for (QSet<int>::iterator it = ungrouped.begin(); it != ungrouped.end(); it++) {
		if (totaldiffr[*it] >= maxdiffr) {
		maxdiffr = totaldiffr[*it];
		mostdiffrid = *it;
		}
	}

	//out << "BOP with the most differences: " << mostdiffrid << endl;

	//getchar();

	// Add into the new subset the BOP* with the greatest total difference
	decompbopids.append(QList<int>());
	decompbopids.last().clear();
	decompbopids.last().append(mostdiffrid);
	ungrouped.remove(mostdiffrid);

	// Add into the new subset <= (maxsize-1) BOPs with the smallest pairwise difference to BOP*
	while (decompbopids.last().size() < maxsize && ungrouped.size() > 0) {
		mindiffr = Math::MAX_DOUBLE;
		leastdiffrid = -1;
		for (QSet<int>::iterator it = ungrouped.begin(); it != ungrouped.end(); it++) {
		if (mindiffr > pairwisediffr[mostdiffrid][*it]) {
			mindiffr = pairwisediffr[mostdiffrid][*it];
			leastdiffrid = *it;
		}
		}
		decompbopids.last().append(leastdiffrid);
		ungrouped.remove(leastdiffrid);
	}

	//out << "BOP subsets:" << endl;
	//for (int i = 0; i < decompbopids.size(); i++) {
	//	out << "(";
	//	for (int j = 0; j < decompbopids[i].size(); j++) {
	//		out << decompbopids[i][j] << " ";
	//	}
	//	out << ")" << endl;
	//}
	//getchar();

	}


}
 */

/*
double Product::bopDifference(const int id1, const int id2) {
	double res = 0.0;

	if (!bopid2idx.contains(id1) || !bopid2idx.contains(id2)) {
	Debugger::wDebug("Product::bopDifference : Unknown BOP ID.");
	res = Math::MAX_DOUBLE;
	return res;
	}

	BillOfProcesses *bop1 = bops[bopid2idx[id1]];
	BillOfProcesses *bop2 = bops[bopid2idx[id2]];

	// Generate the BOPs before manipulating them
	//bop1->generate();
	//bop2->generate();

	QList<QPair<QPair<int, int>, double> > bop1m = bop1->toMatrix();
	QList<QPair<QPair<int, int>, double> > bop2m = bop2->toMatrix();

	// Degenerate the BOPs after manipulating them
	//bop1->degenerate();
	//bop2->degenerate();

	int curidx1 = 0;
	int curidx2 = 0;

	while (curidx1 < bop1m.size()) {

	// Find the index with the same element as bop1m[curidx1]
	for (curidx2 = 0; curidx2 < bop2m.size(); curidx2++) {
		if (bop1m[curidx1].first.first == bop2m[curidx2].first.first && bop1m[curidx1].first.second == bop2m[curidx2].first.second) {
		break;
		}
	}

	// Eliminate the elements from the both arrays
	if (curidx2 < bop2m.size()) {
		bop1m.removeAt(curidx1);
		bop2m.removeAt(curidx2);

		curidx1 = 0;
		continue;
	}

	curidx1++;
	}

	// Collect the difference values
	res = 0.0;
	for (int i = 0; i < bop1m.size(); i++) {
	res += bop1m[i].second;
	}

	for (int i = 0; i < bop2m.size(); i++) {
	res += bop2m[i].second;
	}

	return res;
}
 */

//QTextStream& operator<<(QTextStream& out, Product &prod) {
//	return (out << prod);
//}

QTextStream& operator<<(QTextStream& out, const Product &prod){
	out << "Product: ["
			<< "<"
			<< "ID=" << prod.ID << ","
			<< "Ty=" << prod.type << ","
			<< "bopID=" << prod.bopID << ">" << endl;
	for (int i = 0; i < prod.bops.size(); i++) {
		out << *prod.bops[i] << endl;
	}

	out << "]";

	return out;
}

Plan ProductManager::bops2Plan() {
	Plan plan;

	plan.clear();
	for (int i = 0; i < products.size(); i++) {
		plan.prodID2BOPID[products[i]->ID] = products[i]->bopID;
		plan.prodID2ItemID2RouteIdx[products[i]->ID] = products[i]->bopByID(products[i]->bopID)->itemRouteIdx();
		plan.prodID2ItemID2ItemType[products[i]->ID] = products[i]->bopByID(products[i]->bopID)->itemID2ItemType();
		plan.prodID2ItemID2RouteID[products[i]->ID].clear();
		for (QHash<int, int>::iterator iter = plan.prodID2ItemID2RouteIdx[products[i]->ID].begin(); iter != plan.prodID2ItemID2RouteIdx[products[i]->ID].end(); iter++) {
			if (iter.key() < 0) { // This item is fake
				plan.prodID2ItemID2RouteID[products[i]->ID][iter.key()] = -1;
			} else {
				plan.prodID2ItemID2RouteID[products[i]->ID][iter.key()] = products[i]->bopByID(products[i]->bopID)->itemRoutes()[iter.key()][iter.value()]->ID;
			}
		}
	}

	return plan;
}

void ProductManager::bopsFromPlan(const Plan& plan) {
	Product* prod;
	for (int i = 0; i < products.size(); i++) {
		prod = products[i];
		if (!plan.prodID2BOPID.contains(prod->ID)) {
			Debugger::err << "ProductManager::bopsFromPlan : Unknown product " << prod->ID << "!!!" << ENDL;
		} else {
			if (!plan.prodID2ItemID2RouteIdx.contains(prod->ID)) {
				Debugger::err << "ProductManager::bopsFromPlan : Unknown route indices for product " << prod->ID << "!!!" << ENDL;
			} else {
				prod->bopID = plan.prodID2BOPID[prod->ID];

				// Set the routes indices for the current product
				prod->bopByID(prod->bopID)->setItemRouteIdx(plan.prodID2ItemID2RouteIdx[prod->ID]);
			}
		}
	}
}

void ProductManager::iroutesFromDOMElement(const QDomElement &elem, QHash<int, QList<Route*> > &iroutes) {
	iroutes.clear();

	QDomNodeList item_list = elem.elementsByTagName("itype");
	QDomNodeList route_list;
	int itm_type_id;
	for (int i = 0; i < item_list.size(); i++) {
		itm_type_id = item_list.item(i).toElement().attribute("id").toInt();
		//out << "Reading item type " << itm_type_id << endl;
		route_list = item_list.item(i).toElement().elementsByTagName("route");
		for (int j = 0; j < route_list.size(); j++) {
			iroutes[itm_type_id].append(new Route());
			iroutes[itm_type_id].last()->fromDOMElement(route_list.item(j).toElement());
		}
	}
}

void ProductManager::pbomsFromDOMElement(const QDomElement &elem, QHash<int, QList<BillOfMaterials*> > &pboms) {
	QTextStream out(stdout);

	// Get the list of product types in the XML file
	QDomNodeList prod_list = elem.elementsByTagName("ptype");
	//out << "Number of product types read: " << prod_list.size() << endl;
	QDomNodeList prod_bom_list;
	int prod_type_id; // ID of the current product
	// For every node create the corresponding BOMs
	for (int i = 0; i < prod_list.size(); i++) {
		prod_type_id = prod_list.item(i).toElement().attribute("type").toInt();
		//out << "Current product type: " << prod_type_id << endl;

		// Get the list of BOMs for the current product
		prod_bom_list = prod_list.item(i).toElement().elementsByTagName("bom");

		// Iterate over all BOM descriptions and create the actual BOMs
		for (int cb = 0; cb < prod_bom_list.size(); cb++) {
			pboms[prod_type_id].append(new BillOfMaterials());

			// Extract the BOM from the DOM element
			//out << "Parsing pbom from the DOM element ..." << endl;
			//out << prod_bom_list.item(cb).toElement() << endl;
			pboms[prod_type_id].last()->fromDOMElement(prod_bom_list.item(cb).toElement());
			//out << *(pboms[prod_type_id].last()) << endl;
			//getchar();
			//out << "Done parsing pbom from the DOM element." << endl;
		}
	}
}
