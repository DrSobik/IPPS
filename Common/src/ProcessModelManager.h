/* 
 * File:   ProcessModelManager.h
 * Author: DrSobik
 *
 * Description : Class ProcessModelManager is the layer class between Products
 *				 and Orders. It contains the ProductManager and OrderManager. 
 *				 It is capable of generating the
 *				 process model based on the information about the current 
 *				 Bills of Processes within each Product and the Orders to be 
 *				 processed.
 * 
 * Contained data:
 *				
 *				prodman		-	product manager.
 *			
 *				ordman		-	order manager.
 * 
 *				pm			-	current process model in the production system.
 * 
 *				prodid2prevbopid
 *							-	IDs of BOPs of the products during the previous
 *								step of process model generation. It is used to 
 *								trace and update only those local PMs where the 
 *								BOPs of the corresponding products have been 
 *								changed.
 * 
 *				ordid2locpm	-	mapping of the order IDs onto the pairs of nodes
 *								<start, end> of the local PMs in the global PM.
 *								With these nodes local PMs can be easily 
 *								identified and modified. Fiction operations
 *								must be associated with each such node.
 *
 *				freeIDs		-	a queue of available IDs for the real operations
 *								which are created during updating the global PM.
 *								The IDs of the operations which are to be 
 *								deleted	must be enqueued so that they can be 
 *								reused. The number of the elements in the queue
 *								defines the maximum number of operations which
 *								can get a unique ID.
 * 
 *				maxfreeIDs	-	maximum number of IDs that can be present in the
 *								ID queue.
 * 
 * Created on August 3, 2011, 10:37 AM
 */

#ifndef PROCESSMODELMANAGER_H
#define	PROCESSMODELMANAGER_H

#include "Product.h"
#include "Order.h"
#include "ProcessModel.h"
#include "Plan.h"

#include <QHash>
#include <QQueue>
#include <QStack>

#include <lemon/connectivity.h>

using namespace lemon;

class ProcessModelManager {
protected:
    //QQueue<int> freeIDs;
	//QStack<int> freeItemIDs; // IDs which will be user for items
    //int maxfreeItemIDs;
	
//    QStack<int> freeIDs;
    int maxfreeIDs;

public:

    ProcessModel pm;

    ProductManager *prodman;
    OrderManager *ordman;

    QHash<int, int> prodid2prevbopid;

    QHash<int, QPair<int, int> > ordid2locpm;

    ProcessModelManager();
    virtual ~ProcessModelManager();

    /** Initialize the process model. */
    virtual void init();

	/** Clear everything. */
	virtual void clear();
	
    /** Set the product manager with the orders. */
    ProcessModelManager& operator<<(ProductManager* prodman) {
	this->prodman = prodman;

	return *this;
    }

    /** Set the order manager with the orders. */
    ProcessModelManager& operator<<(OrderManager *ordman) {
	this->ordman = ordman;

	return *this;
    }

    /** Remove the subgraph of the order with the specified ID from the PM. */
    void deleteOrdSubgraph(const int& ID);

    /** Generate and insert the subgraph corresponding to the order based on the product. */
    void insertOrdSubgraph(const Product& prod, Order& ord);

    /** Generate and insert the subgraph corresponding to the order based on the product into the specified PM. */
    void insertOrdSubgraph(const Product& prod, Order& ord, ProcessModel& pm);

    /** Build the process model based on the products and the orders in the 
     *	system. */
    void updatePM();

    /** Generate a process model based on the given plan. */
    ProcessModel plan2PM(const Plan& plan);

private:

};

#endif	/* PROCESSMODELMANAGER_H */

