/* 
 * File:   Order.h
 * Author: DrSobik
 *
 * Created on July 29, 2011, 10:30 AM
 * 
 * Description : Class Order represents one physical entity of some product
 *				 type which has to be completed. It does not contain information
 *				 about the design of the product, since such information is 
 *				 provided by the Product class. It contains information about
 *				 order priority, its due date and so on.
 * 
 * Contained data:
 *				
 *				ID	-	ID of the order;
 *				
 *				type-	type of the product type which this order represents.
 * 
 *				r	-	ready time of the order.
 * 
 *				d	-	due date of the order.
 * 
 *				w	-	priority of the order.
 * 
 */

#ifndef ORDER_H
#define	ORDER_H

#include <QList>
#include <QHash>
#include <QSet>
#include <QDomElement>
#include <QDomNode>

#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "DebugExt.h"

#include "Item"
#include "Operation"

using namespace Common;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE Order {
public:

	int ID;

	int type;

	int BID; // ID of the BOM of the corresponding product

	double r;
	double d;
	double w;

	//QHash<int, Item> itemID2Item; // Items which this product will be assembled of

	QList<int> itemIDs; // IDs of the items belonging to this order
	QList<QPair<int, int> > itemIDPrececences; // Technological precedence constraints between the items

	enum OrderAction {
		OA_PLAN_SCHED, OA_PLAN_PARTS_SCHED, OA_SCHED
	};

	OrderAction action;

	static int ordCreated;
	static int ordDeleted;
	
	Order();
	Order(const Order& other);
	virtual ~Order();

	virtual void clear();

	/** Extract the order data from the DOM element. */
	virtual void fromDOMElement(const QDomElement& oelem);

	/** Add an item to the order. */
	//virtual Order& operator<<(Item& item);

	//virtual Item& itemByID(const int& itmID);

	//virtual void clearItems();

    DLL_EXPORT_INTEFACE friend QTextStream& operator<<(QTextStream& out, Order& order);

    DLL_EXPORT_INTEFACE friend QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Order& order);

    DLL_EXPORT_INTEFACE friend QXmlStreamReader& operator>>(QXmlStreamReader& reader, Order& order);

private:

};

/**
 * Description : Class OrderManager contains all of the orders in the system
 *				 which have not yet been finished.
 * 
 * Contained data:
 *			
 *				orders	-	list of orders to be finished.
 * 
 *				ordid2idx
 *						-	mapping of the order IDs onto the set of indices in 
 *							the list.
 * 
 *				type2idcs
 *						-	list of order indices of the specified types.
 * 
 */
class DLL_EXPORT_INTEFACE OrderManager {
public:

	QHash<int, int> operID2Idx; // Indices of the operations
	QList<Operation> operations; // Operations which have been generated
	
	QHash<int, int> incompleteOperID2Idx; // Indices of the incomplete operations
	QList<Operation> incompleteOperations; // Operations which have been generated
		
	QList<QPair<int, int> > operIDPrecedences; // Precedence constraints between the operations in the actual schedule
	QList<bool> operIDPrecedencesConj; // Indicates whether the corresponding precedence constraint is a technological one

	QHash<int, int> itemID2Idx; // Item indices
	QList<Item> items; // All items which have been generated

	QHash<int, int> incompleteItemID2Idx; // Incomplete item indices
	QList<Item> incompleteItems; // All items which have been generated
	
	QList<Order> orders;
	QHash<int, int> ordid2idx;
	QHash<int, QList<int> > type2idcs;

	OrderManager() {
	}

	virtual ~OrderManager() {
	}

	/** Full initialization of the order manager. All of the current orders
	 *	are deleted. */
	virtual void init();

	virtual void clear();

	OrderManager& operator<<(Order& ord) {
		orders.append(ord);
		ordid2idx[ord.ID] = orders.size() - 1;
		type2idcs[ord.type].append(orders.size() - 1);

		return *this;
	}

	OrderManager& operator<<(Item& itm) {
		items.append(itm);
		itemID2Idx[itm.ID] = items.size() - 1;

		return *this;
	}

	OrderManager& operator<<(Operation& oper) {
		operations.append(oper);
		operID2Idx[oper.ID] = operations.size() - 1;

		return *this;
	}

	/** Order by ID. */
	inline Order& operator()(const int ID) {
		return orders[ordid2idx[ID]];
	}

	/** Order by index. */
	inline Order& operator[](const int idx) {
		return orders[idx];
	}

	/** The set of products (the types) which are represented between the orders. */
	inline QSet<int> availProducts() {
		return type2idcs.keys().toSet();
	}

	/** List of orders of the specified type. */
	QList<Order*> ordersByType(const int type);

	Order& orderByID(const int& oID) {
		#ifdef DEBUG
		if (!ordid2idx.contains(oID)){
			Debugger::err << "OrdMan::orderByID : Order with ID " << oID << " not found!" << ENDL;
		}
		#endif
		
		return orders[ordid2idx[oID]];
	}

	Item& itemByID(const int& iID) {
		#ifdef DEBUG
		if (!itemID2Idx.contains(iID)){
			Debugger::err << "OrdMan::itemByID : Item with ID " << iID << " not found!" << ENDL;
		}
		#endif
		
		return items[itemID2Idx[iID]];
	}

	Item& incompleteItemByID(const int& iID) {
		#ifdef DEBUG
		if (!incompleteItemID2Idx.contains(iID)){
			Debugger::err << "OrdMan::incompleteItemByID : Incomplete item with ID " << iID << " not found!" << ENDL;
		}
		#endif
		
		return incompleteItems[incompleteItemID2Idx[iID]];
	}
	
	Operation& operByID(const int& operID) {
		#ifdef DEBUG
		if (!operID2Idx.contains(operID)){
			Debugger::err << "OrdMan::operByID : Operation with ID " << operID << " not found!" << ENDL;
		}
		#endif

		return operations[operID2Idx[operID]];
	}

	Operation& incompleteOperByID(const int& operID) {
		#ifdef DEBUG
		if (!incompleteOperID2Idx.contains(operID)){
			Debugger::err << "OrdMan::incompleteOperByID : Incomplete operation with ID " << operID << " not found!" << ENDL;
		}
		#endif

		return incompleteOperations[incompleteOperID2Idx[operID]];
	}
};

#endif	/* ORDER_H */

