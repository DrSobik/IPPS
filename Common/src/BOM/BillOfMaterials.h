/* 
 * File:   BillOfMaterials.h
 * Author: DrSobik
 *
 * Created on July 29, 2011, 11:02 AM
 * 
 * Description : Class BillOfMaterials represents a Bill of Materials for
 *				 some product type in the production system. It contains 
 *				 information about types of items needed to manufacture some 
 *				 desired product type and precedence constraints between the 
 *				 item types. It could later be converted to a sequence of real
 *				 items of the corresponding types.
 * 
 * Contained data:
 * 
 *				ID		- ID of this BOM.
 * 
 *				graph	- graph which represents precedence constraints between
 *						  different item types in this BOM.
 *				
 *				head	- fake head of the graph. It points at the not existing
 *						  operation type.
 * 
 *				tail	- fake tail of the graph. It represents the not existing
 *						  item type that follows immediately after the final 
 *						  item type (which is the product itself) of the graph;
 * 
 *				itypeID	- maps the graph nodes onto the set of item type IDs.
 * 
 */

#ifndef BILLOFMATERIALS_H
#define	BILLOFMATERIALS_H

#include <lemon/list_graph.h>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <QHash>
#include <QMap>
#include <QStringList>

#include "DebugExt.h"
#include "Route"

using namespace lemon;
using namespace Common;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE BillOfMaterials {
public:

	int ID;

	ListDigraph graph;

	ListDigraph::Node head;
	ListDigraph::Node tail;

	ListDigraph::NodeMap<int> itypeID;
	ListDigraph::NodeMap<int> itemID; // IDs of the concrete items (not item types) within this BOM 

	static int bomsCreated;
	static int bomsDeleted;
	
	BillOfMaterials();
	BillOfMaterials(const BillOfMaterials& other);
	virtual ~BillOfMaterials();

	virtual void init();
	virtual void clear();
	
	BillOfMaterials& operator=(const BillOfMaterials& other);

	/** Create the graph of the BOM based on the given DOM element. */
	virtual void fromDOMElement(const QDomElement& domel);
		
    DLL_EXPORT_INTEFACE friend QTextStream& operator<<(QTextStream& out, BillOfMaterials& bm);
    DLL_EXPORT_INTEFACE friend QXmlStreamReader& operator>>(QXmlStreamReader& reader, BillOfMaterials& bm);
    DLL_EXPORT_INTEFACE friend QXmlStreamWriter& operator<<(QXmlStreamWriter& writer, BillOfMaterials& bm);
	
	/** Define part IDs for each part in this BOM. */
	virtual void setItemIDs();
	
private:

};

#endif	/* BILLOFMATERIALS_H */

