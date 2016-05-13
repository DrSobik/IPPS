/* 
 * File:   Route.h
 * Author: DrSobik
 *
 * Created on July 29, 2011, 1:35 PM
 * 
 * Description : Class Route represents a sequence of operation types 
 *				 (NOTE: not concrete operations). This sequence could later 
 *				 be converted into a sequence of concrete operations of 
 *				 the corresponding types. The sequence is strictly defined.
 * 
 * Contained data : 
 * 
 *				ID	- ID of the sequence of the operation types.
 * 
 *				otypeID 
 *					-
 *					  type ID of the operation which has to be performed.
 * 
 */

#ifndef ROUTE_H
#define	ROUTE_H

#include <lemon/list_graph.h>

#include <QList>
#include <QStringList>
#include <QTextStream>
#include <QDomElement>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "DebugExt.h"

using namespace lemon;
using namespace Common;

class Route {
public:

	int ID;

	QList<int> otypeIDs;

	Route();
	Route(const Route& other);
	virtual ~Route();

	virtual void clear();
	
	Route& operator=(const Route& other);

	/** Extract the route from the given DOM element. */
	virtual void fromDOMElement(const QDomElement& domel);
	
	friend QTextStream& operator<<(QTextStream& out, Route& route);
	friend QXmlStreamReader& operator>>(QXmlStreamReader& reader, Route& route);
	friend QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Route& route);

private:

};

#endif	/* ROUTE_H */

