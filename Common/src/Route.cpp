/* 
 * File:   Route.cpp
 * Author: DrSobik
 * 
 * Created on July 29, 2011, 1:35 PM
 */

#include "Route.h"
#include "Resources.h"

Route::Route() {
}

Route::Route(const Route &other) {
	*this = other;
}

Route::~Route() {
}

void Route::clear() {
	ID = -1;

	otypeIDs.clear();
}

Route& Route::operator=(const Route& other) {
	ID = other.ID;

	otypeIDs = other.otypeIDs;

	return *this;
}

void Route::fromDOMElement(const QDomElement &domel) {
	QString str_route; // Current route read as a string
	QStringList str_otypeids; // otypeIDs in a string form

	str_route = domel.text();
	//out << "Read route: " << str_route << endl;

	// Split the elements of the str_route and create a normal route
	ID = domel.attribute("id").toInt(); //10 * itm_type_id + j + 1;
	str_otypeids = str_route.split(",");
	for (int k = 0; k < str_otypeids.size(); k++) {
		otypeIDs.append(str_otypeids[k].toInt());
	}
}

QTextStream& operator<<(QTextStream &out, Route &route) {
	out << "Route " << route.ID <<  " (";
	for (int i = 0; i < route.otypeIDs.size() - 1; i++) {
		out << route.otypeIDs[i] << " -> ";
	}
	out << route.otypeIDs.last();

	out << ")";

	return out;
}

QXmlStreamReader& operator>>(QXmlStreamReader& reader, Route& route) {

	// IMPORTANT !!! It is assumed that the current token of the reader is "route"

	route.clear();

	route.ID = reader.attributes().value("id").toString().toInt();

	QString str = reader.readElementText();
	QStringList sl = str.split(",");

	if (str != "") {
		for (int i = 0; i < sl.size(); i++) {
			route.otypeIDs.append(sl[i].toInt());
		}
	}

	return reader;
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Route&) {

	Debugger::err << "operator<<(QXmlStreamWriter& composer, Route& route) : Not implemented yet! " << ENDL;

	return composer;
}