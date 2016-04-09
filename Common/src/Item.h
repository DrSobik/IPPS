/* 
 * File:   Item.h
 * Author: DrSobik
 *
 * Created on August 6, 2013, 2:46 PM
 * 
 *	Description : Class Item encapsulates real items which will be created within
 *  the products. Each Item has an ID, its type, ID of its order, ID of the route
 *  which it has to follow, step index in of the current operation in the current
 *  route.
 * 
 */

#ifndef ITEM_H
#define	ITEM_H

#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QList>

#include "Route.h"
#include "Operation.h"

class Order;

class Item {
public:
	int ID; // ID of the item
	int type; // Type of the item

	int orderID; // Order which this item belongs to
	
	int routeID; // Route which this item has to follow
	int curStepIdx; // Index of the current step in the route
	bool curStepFinished; // Defines whether the current step is finished or still processing
	
	double releaseTime; // A time point at which the item becomes available for scheduling
	
	QList<int> operIDs; // References to the concrete operations on the route

	enum ItemAction{IA_PLAN_SCHED, IA_SCHED};
	
	ItemAction action;
	
	static int itmCreated;
	static int itmDeleted;
	
	Item();
	Item(const Item& orig);
	virtual ~Item();

	/** Initializes this operation. */
	virtual void init();

	virtual void clear();
	
	/** Assigns all of the data of the "other" operation to this one. */
	virtual void copy(const Item& other);

	/** Copy operator. */
	const Item& operator=(const Item& other);

	/** Friend operator that checks whether the operations are the same. 
	 * Comparison is based on the unique IDs of the operations. */
	friend bool operator==(const Item& i1, const Item& i2) {
		return i1.ID == i2.ID;
	}

	friend QTextStream& operator<<(QTextStream& out, Item& item);
	
	friend QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Item& item);
	friend QXmlStreamReader& operator>>(QXmlStreamReader& reader, Item& item);
	
private:

};

#endif	/* ITEM_H */

