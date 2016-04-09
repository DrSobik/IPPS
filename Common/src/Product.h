/* 
 * File:   Product.h
 * Author: DrSobik
 *
 * Created on July 29, 2011, 10:30 AM
 * 
 * Description:	Class Product represents one product type in the manufacturing
 *				system. It contains information about possible designs of the
 *				product: available Bills of Materials (BOMs) and information
 *				about alternative process plans (PPs) of every item which is 
 *				contained in the BOMs. The class is capable of decomposing	the
 *				available BOPs which are generated based on 
 *				the BOMs and PPs into disjunctive subsets so that each of the
 *				subsets contains information only about such BOPs 
 *				which have similar characteristics in some sense.
 * 
 * Contained data:
 *	
 *				ID	-	ID of the product.
 * 
 *				type-	type of the product.
 * 
 *				bopID	
 *					-	ID of the current BOP for the product.
 * 
 *				bops-	list of all alternative BOPs for this product type.
 * 
 *				bopid2idx
 *					-	index of the corresponding BOP based on its' ID.
 * 
 *				decompbopids
 *					-	list of disjunctive subsets of the of the BOP IDs each 
 *						of which contains IDs of those BOPs which somehow 
 *						similar to each other. Different subsets contain
 *						non-similar BOPs.
 * 
 *				_bopsdecomprate
 *					-	decomposition rate defines maximum number of similar
 *						BOPs that can be present in one subset of the BOPs. 
 *						This value should be in [0.0, 1.0].
 */

#ifndef PRODUCT_H
#define	PRODUCT_H

#include "BillOfProcesses.h"
#include "Plan.h"

#include <QVector>
#include <QHash>
#include <QTextStream>

class Product {
private:
    double _bopsdecomprate;

public:
    int ID;

    int type;

    int bopID;
	
    QVector<BillOfProcesses*> bops; // A BOP is the combination of a concrete BOM and the selected item type routes
    QVector<double> bopsRank;
    QHash<int, int> bopid2idx;

    //QList<QList<int> > decompbopids;
    Resources *rc; // Used for ranking the products

    Product();
    virtual ~Product();

    /** Add a bop to the product. */
    Product& operator<<(BillOfProcesses *bop) {
	bops.append(bop);
	bopid2idx[bop->ID] = bops.size() - 1;

	bopID = bops.last()->ID;

	return *this;
    }

    inline BillOfProcesses* bopByID(const int& bid) const{
	return bops[bopid2idx[bid]];
    }

    /** Expected processing cost of one entity of the product over all possible BOPs. */
    virtual double expCost(Resources& rc);

    /** Perform ranking of different BOPs. */
    virtual void rankBOPs(Resources& rc);

    /** Select a BOP based on the ranking of the BOPs. The higher is the rank 
     *  bigger is the selecting probability.  */
    virtual BillOfProcesses* selectBOPByRank();

    /** Select the best BOP based on the ranking. */
    virtual BillOfProcesses* bestBOPByRank();

    virtual BillOfProcesses* rndBOP();

    //void bopsDecompRate(const double &rate);

    /** Decompose set of the BOPs into disjunctive subsets each of which 
     *	contains BOPs similar to each other. */
    //virtual void decompBOPSet();

    /** Pairwise difference measure between the BOPs with the given IDs. */
    //virtual double bopDifference(const int id1, const int id2);

    /** Output of the product to the text stream. */
    //friend QTextStream& operator<<(QTextStream& out, Product &prod);
	friend QTextStream& operator<<(QTextStream& out, const Product &prod);

private:

};

/**
 * Description : Class ProductManager contains all of the product in the system
 *				 which could be produced.
 * 
 * Contained data:
 *			
 *				products
 *						-	list of products.
 * 
 *				prodid2idx
 *						-	mapping of the product IDs onto the set of 
 *							indices in the list.
 * 
 */

class ProductManager {
public:

    QList<Product*> products;
    QHash<int, int> prodid2idx;

    ProductManager() {
    }

    virtual ~ProductManager() {
    }

    /** Full initialization of the product manager. */
    virtual void init() {
	products.clear();
	prodid2idx.clear();
    }

	/** Clear all data. */
    virtual void clear() {
		prodid2idx.clear();
		for (int i = 0 ; i < products.size() ; i++){
			//delete products[i];
		}
		products.clear();
    }
	
    ProductManager& operator<<(Product *prod) {
	products.append(prod);
	prodid2idx[prod->ID] = products.size() - 1;

	return *this;
    }

    /** Product by ID. */
    inline Product& operator()(const int ID) {
	return *products[prodid2idx[ID]];
    }

    /** Product by index. */
    inline Product& operator[](const int idx) {
	return *products[idx];
    }

	inline Product& productByID(const int ID){
		
		#ifdef DEBUG
		if (!prodid2idx.contains(ID)){
			Debugger::err << "ProductManager::productByID : Unknown product " << ID << ENDL;
		}
		#endif
		
		return *products[prodid2idx[ID]];
	}
	
    /** Return reference to the list of products. */
    inline const QList<Product*>& productsList() {
	return products;
    }

    /** Generate plan for all products. */
    virtual Plan bops2Plan();

    /** Change current states of the products based on the given plan. */
    virtual void bopsFromPlan(const Plan& plan);

    /* ------------------  Utilities to manage product data  ---------------- */
    /** Routine to read the alternative routes for different item types from
     *  the DOM element. */
    static void iroutesFromDOMElement(const QDomElement &elem, QHash<int, QList<Route*> > &iroutes);

    /** Routine to read the alternative BOMs for different product types from
     *  the DOM element. */
    static void pbomsFromDOMElement(const QDomElement &elem, QHash<int, QList<BillOfMaterials*> > &pboms);

    /* ---------------------------------------------------------------------- */

};

#endif	/* PRODUCT_H */

