/* 
 * File:   Item.cpp
 * Author: DrSobik
 * 
 * Created on August 6, 2013, 2:46 PM
 */

#include "Item.h"
#include "Operation.h"

int Item::itmCreated = 0;
int Item::itmDeleted = 0;

Item::Item() {
	init();
	
	itmCreated++;
}

Item::Item(const Item& orig) {
	*this = orig;
	
	itmCreated++;
}

Item::~Item() {
	//QTextStream out(stdout);
	
	clear();
	
	itmDeleted++;
			
	//out << "Items created : " << itmCreated << endl;
	//out << "Items deleted : " << itmDeleted << endl;
}

void Item::init() {
	//ID = -1;
	type = -1;

	orderID = -1;

	routeID = -1;
	curStepIdx = -1;
	curStepFinished = false;

	releaseTime = 0.0;

	operIDs.clear();

	action = Item::IA_PLAN_SCHED;
}

void Item::clear() {
	//ID = -1;
	type = -1;

	orderID = -1;

	routeID = -1;
	curStepIdx = -1;
	curStepFinished = false;

	releaseTime = 0.0;

	operIDs.clear();

	action = Item::IA_PLAN_SCHED;
}

void Item::copy(const Item& other) {
	ID = other.ID;
	type = other.type;

	orderID = other.orderID;

	routeID = other.routeID;
	curStepIdx = other.curStepIdx;
	curStepFinished = other.curStepFinished;

	releaseTime = other.releaseTime;

	operIDs = other.operIDs;

	action = other.action;
}

const Item& Item::operator=(const Item& other) {
	this->copy(other);

	return *this;
}

QTextStream& operator<<(QTextStream& out, Item& item) {
	out << "Item " << item.ID << " : [";

	out << "type = " << item.type << ", ";
	out << "route ID = " << item.routeID << ", ";
	out << "sidx = " << item.curStepIdx << "(fin=" << ((item.curStepFinished) ? "1" : "0") << ")" << ", ";
	out << "r = " << item.releaseTime << ", ";
	out << "order ID = " << item.orderID << ", ";
	out << "action = ";

	if (item.action == Item::IA_PLAN_SCHED) out << "PLAN_SCHED";
	if (item.action == Item::IA_SCHED) out << "SCHED";

	out << "]";

	return out;
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Item& item) {
	composer.writeStartElement("part");
	composer.writeAttribute("id", QString::number(item.ID));

	// Write the action which has been performed to the item
	if (item.action == Item::IA_PLAN_SCHED) {
		composer.writeAttribute("action", "PLAN_SCHED");
	}
	if (item.action == Item::IA_SCHED) {
		composer.writeAttribute("action", "SCHED");
	}

	composer.writeTextElement("type", QString::number(item.type));
	composer.writeTextElement("routeID", QString::number(item.routeID));
	composer.writeTextElement("curStepIdx", QString::number(item.curStepIdx));
	composer.writeTextElement("curStepFinished", (item.curStepFinished) ? "1" : "0");
	composer.writeTextElement("releaseTime", QString::number(item.releaseTime));
	composer.writeTextElement("unitID", QString::number(item.orderID));

	composer.writeStartElement("operIDs");
	QString operIDsStr = "";
	for (int i = 0; i < item.operIDs.size(); i++) {
		operIDsStr += QString::number(item.operIDs[i]);

		if (i < item.operIDs.size() - 1) {
			operIDsStr += ",";
		}
	}
	composer.writeCharacters(operIDsStr);
	composer.writeEndElement(); // operIDs

	composer.writeEndElement(); // part

	return composer;
}

QXmlStreamReader& operator>>(QXmlStreamReader& reader, Item& item) {
	// It is assumed that the reader has just read a token "part"

	item.clear();

	item.ID = reader.attributes().value("id").toString().toInt();

	QString itemAction = reader.attributes().value("action").toString();

	if (itemAction == "PLAN_SCHED") item.action = Item::IA_PLAN_SCHED;
	if (itemAction == "SCHED") item.action = Item::IA_SCHED;

	while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "part")) {

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "type") {
			item.type = reader.readElementText().toInt();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "unitID") {
			item.orderID = reader.readElementText().toInt();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "routeID") {
			item.routeID = reader.readElementText().toInt();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "curStepIdx") {
			item.curStepIdx = reader.readElementText().toInt();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "curStepFinished") {
			QString textEl = reader.readElementText();
			item.curStepFinished = ((textEl == "0") ? false : true);
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "releaseTime") {
			item.releaseTime = reader.readElementText().toDouble();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "operIDs") {
			QString elText = reader.readElementText();

			if (elText != "") {
				QStringList sl = elText.split(",");

				for (int i = 0; i < sl.size(); i++) {
					item.operIDs.append(sl[i].toInt());
				}
			}

		}

	}

	return reader;
}