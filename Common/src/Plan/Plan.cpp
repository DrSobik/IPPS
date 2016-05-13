/* 
 * File:   Plan.cpp
 * Author: DrSobik
 * 
 * Created on February 22, 2013, 9:44 AM
 */

#include <QtCore/qhash.h>

#include "Plan.h"

/** ##########################  Plan  ####################################### */

Plan::Plan() {
    init();
}

Plan::Plan(const Plan& orig) {
    *this = orig;
}

Plan::~Plan() {

}

void Plan::init() {
    prodID2BOPID.clear();
    prodID2ItemID2RouteIdx.clear();
	prodID2ItemID2RouteID.clear();
	prodID2ItemID2ItemType.clear();
}

void Plan::clear(){
	savedProdID2BOMID.clear();
	savedProdID2ItemID2ItemType.clear();
	savedProdID2ItemID2RouteIdx.clear();
	savedProdID2ItemID2RouteID.clear();
	
	prodID2BOPID.clear();
	prodID2ItemID2ItemType.clear();
	prodID2ItemID2RouteID.clear();
	prodID2ItemID2RouteIdx.clear();
}

Plan& Plan::operator=(const Plan& other) {
    prodID2BOPID = other.prodID2BOPID;
    prodID2ItemID2RouteIdx = other.prodID2ItemID2RouteIdx;
	prodID2ItemID2RouteID = other.prodID2ItemID2RouteID;
	prodID2ItemID2ItemType = other.prodID2ItemID2ItemType;

    return *this;
}

void Plan::clearCurrentActions() {
    prodID2BOPID.clear();
    prodID2ItemID2RouteIdx.clear();
	prodID2ItemID2RouteID.clear();
	prodID2ItemID2ItemType.clear();
}

void Plan::clearSavedActions() {
    savedProdID2BOMID.clear();
    savedProdID2ItemID2RouteIdx.clear();
	savedProdID2ItemID2RouteID.clear();
	savedProdID2ItemID2ItemType.clear();
}

void Plan::saveActions() {
    savedProdID2BOMID = prodID2BOPID;
    savedProdID2ItemID2RouteIdx = prodID2ItemID2RouteIdx;
	savedProdID2ItemID2RouteID = prodID2ItemID2RouteID;
	savedProdID2ItemID2ItemType = prodID2ItemID2ItemType;
}

void Plan::restoreActions() {
    prodID2BOPID = savedProdID2BOMID;
    prodID2ItemID2RouteIdx = savedProdID2ItemID2RouteIdx;
	prodID2ItemID2RouteID = savedProdID2ItemID2RouteID;
	prodID2ItemID2ItemType = savedProdID2ItemID2ItemType;
}

QTextStream& operator<<(QTextStream& out, Plan& plan){
	out << "Plan : " << endl << " [ " << endl;
	
	for (QHash<int,int>::iterator iter1 = plan.prodID2BOPID.begin() ; iter1 != plan.prodID2BOPID.end() ;  iter1++){
		out << "Prod ID = " << iter1.key() << ", BOM ID = " << iter1.value() << endl;	
		
		out << endl;
		
		//for(QHash<int,int>::iterator iter2 = plan.prodID2ItemID2RouteIdx[iter1.key()].begin() ; iter2 != plan.prodID2ItemID2RouteIdx[iter1.key()].end() ; iter2++){
		//	out << "Part ID = " << iter2.key() << ", route index = " << iter2.value() << endl;
		//}
		
		//out << endl;
		
		for(QHash<int,int>::iterator iter2 = plan.prodID2ItemID2RouteID[iter1.key()].begin() ; iter2 != plan.prodID2ItemID2RouteID[iter1.key()].end() ; iter2++){
			out << "Part ID = " << iter2.key() << " (Ty=" << plan.prodID2ItemID2ItemType[iter1.key()][iter2.key()] << ")" << ", route ID = " << iter2.value() << endl;
		}
		
		out << endl;
	}
	out << "]";
	
	return out;
}