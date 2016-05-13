/* 
 * File:   Plan.h
 * Author: DrSobik
 *
 * Created on February 22, 2013, 9:44 AM
 */

#ifndef PLAN_H
#define	PLAN_H

#include <QHash>
#include <QTextStream>

#include "Saveable"

class Plan : public Saveable {
protected:
    QHash<int, int> savedProdID2BOMID;
    QHash<int, QHash<int, int> > savedProdID2ItemID2RouteIdx;
	QHash<int, QHash<int, int> > savedProdID2ItemID2RouteID;
	QHash<int, QHash<int, int> > savedProdID2ItemID2ItemType;

public:
    QHash<int, int> prodID2BOPID; // BOMS for each product
    QHash<int, QHash<int, int> > prodID2ItemID2RouteIdx; // Route indices within the selected BOMs
	QHash<int, QHash<int, int> > prodID2ItemID2RouteID; // Route IDs(not indices) within the selected BOMs
	QHash<int, QHash<int, int> > prodID2ItemID2ItemType; // Item types within the BOM

    Plan();
    Plan(const Plan& orig);
    virtual ~Plan();

    virtual void init();

	virtual void clear();
	
    virtual Plan& operator=(const Plan& other);

    virtual void clearCurrentActions();
    virtual void clearSavedActions();
    virtual void saveActions();
    virtual void restoreActions();

	friend QTextStream& operator<<(QTextStream& out, Plan& plan);
	
};

#endif	/* PLAN_H */

