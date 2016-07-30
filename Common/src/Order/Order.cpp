/* 
 * File:   Order.cpp
 * Author: DrSobik
 * 
 * Created on July 29, 2011, 10:30 AM
 */


#include <QtCore/qhash.h>

#include "Order.h"

int Order::ordCreated = 0;
int Order::ordDeleted = 0;

Order::Order() {
	ID = -1;
	type = -1;
	BID = -1;

	r = 0.0;
	d = 0.0;
	w = 0.0;

	//itemID2Item.clear();

	itemIDs.clear();
	itemIDPrececences.clear();

	action = Order::OA_PLAN_SCHED;

	ordCreated++;
}

Order::Order(const Order& other) {
	ID = other.ID;
	type = other.type;
	BID = other.BID;

	r = other.r;
	d = other.d;
	w = other.w;

	itemIDs = other.itemIDs;
	itemIDPrececences = other.itemIDPrececences;

	action = other.action;

	ordCreated++;
}

Order::~Order() {
	//QTextStream out(stdout);

	ordDeleted++;

	//out << "Orders created : " << ordCreated << endl;
	//out << "Orders deleted : " << ordDeleted << endl;
}

void Order::clear() {
	ID = -1;
	type = -1;
	BID = -1;

	r = 0.0;
	d = 0.0;
	w = 0.0;

	//itemID2Item.clear();

	itemIDs.clear();
	itemIDPrececences.clear();

	action = Order::OA_PLAN_SCHED;
}

void Order::fromDOMElement(const QDomElement& oelem) {
	ID = oelem.attribute("id").toInt();
	type = oelem.elementsByTagName("ptype").item(0).toElement().text().toInt();

	d = oelem.elementsByTagName("d").item(0).toElement().text().toDouble();
	w = oelem.elementsByTagName("w").item(0).toElement().text().toDouble();
	r = oelem.elementsByTagName("r").item(0).toElement().text().toDouble();
}

//Order& Order::operator<<(Item& item){
//	
//	itemID2Item[item.ID] = item;
//	
//	return *this;
//}

//Item& Order::itemByID(const int& itmID){
//#ifdef DEBUG
//	if (!itemID2Item.contains(itmID)){
//		Debugger::err << "Order::itemByID : Item with ID " << itmID << " does not exist in Order " << ID <<" !!!" << ENDL;
//	}
//#endif
//	return itemID2Item[itmID];
//}

//void Order::clearItems(){
//itemID2Item.clear();

//	itemIDs.clear();
//	itemIDPrececences.clear();
//}

QTextStream& operator<<(QTextStream& out, Order& order) {
	out << "Order " << order.ID << " : [";

	out << "prod=" << order.type << ", ";
	out << "BID=" << order.BID << ", ";
	out << "r=" << order.r << ", ";
	out << "d=" << order.d << ", ";
	out << "w=" << order.w << ", ";
	out << "partIDs: ";
	for (int i = 0; i < order.itemIDs.size(); i++) out << order.itemIDs[i] << ", ";
	out << "prec: ";
	for (int i = 0; i < order.itemIDPrececences.size(); i++) out << order.itemIDPrececences[i].first << "->" << order.itemIDPrececences[i].second << ", ";
	out << "]";

	return out;
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Order& order) {
	composer.writeStartElement("unit");
	composer.writeAttribute("id", QString::number(order.ID));
	if (order.action == Order::OA_PLAN_SCHED) {
		composer.writeAttribute("action", "PLAN_SCHED");
	}
	if (order.action == Order::OA_PLAN_PARTS_SCHED) {
		composer.writeAttribute("action", "PLAN_PARTS_SCHED");
	}
	if (order.action == Order::OA_SCHED) {
		composer.writeAttribute("action", "SCHED");
	}

	composer.writeTextElement("w", QString::number(order.w));
	composer.writeTextElement("r", QString::number(order.r));
	composer.writeTextElement("d", QString::number(order.d));

	composer.writeTextElement("type", QString::number(order.type));

	composer.writeTextElement("bomID", QString::number(order.BID));

	composer.writeStartElement("partIDs");
	QString partsSrt = "";
	for (int i = 0; i < order.itemIDs.size(); i++) {
		int curItemID = order.itemIDs[i];
		partsSrt += QString::number(curItemID);

		if (i < order.itemIDs.size() - 1) {
			partsSrt += ",";
		}
	}
	composer.writeCharacters(partsSrt);
	composer.writeEndElement(); // partIDs

	composer.writeStartElement("partPrec");
	for (int i = 0; i < order.itemIDPrececences.size(); i++) {
		int startItemID = order.itemIDPrececences[i].first;
		int endItemID = order.itemIDPrececences[i].second;

		composer.writeTextElement("arc", QString::number(startItemID) + "," + QString::number(endItemID));
	}
	composer.writeEndElement(); // partPrec

	composer.writeEndElement(); // unit	

	return composer;
}

QXmlStreamReader& operator>>(QXmlStreamReader& reader, Order& order) {
	QTextStream out(stdout);

	order.clear();

	order.ID = reader.attributes().value("id").toString().toInt();
	QString orderAction = reader.attributes().value("action").toString(); // Action to perform with the order

	// Check what actions schould be done to the order
	if (orderAction == "PLAN_SCHED") {
		order.action = Order::OA_PLAN_SCHED;
	}

	if (orderAction == "PLAN_PARTS_SCHED") {
		order.action = Order::OA_PLAN_PARTS_SCHED;
	}

	if (orderAction == "SCHED") {
		order.action = Order::OA_SCHED;
	}

	while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "unit")) {

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "ptype") {
			reader.readNext();

			order.type = reader.text().toString().toInt();

			reader.readNext();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "r") {
			reader.readNext();

			order.r = reader.text().toString().toDouble();

			reader.readNext();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "d") {
			reader.readNext();

			order.d = reader.text().toString().toDouble();

			reader.readNext();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "w") {
			reader.readNext();

			order.w = reader.text().toString().toDouble();

			reader.readNext();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "bomID") {
			reader.readNext();

			order.BID = reader.text().toString().toInt();

			reader.readNext();
		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "partIDs") {
			QString elText = reader.readElementText();

			if (elText != "") {

				QStringList sl = elText.split(",");

				for (int i = 0; i < sl.size(); i++) {
					order.itemIDs.append(sl[i].toInt());
				}

			}

		}

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "partPrec") {
			while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "partPrec")) {
				reader.readNext();

				if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "arc") {
					QStringList sl = reader.readElementText().split(",");

					QPair<int, int> curArc(sl[0].toInt(), sl[1].toInt());

					order.itemIDPrececences.append(curArc);
				}

			}

		}

	}

	return reader;
}

void OrderManager::init() {
	operations.clear();
	incompleteOperations.clear();
	operIDPrecedences.clear();
	operIDPrecedencesConj.clear();
	items.clear();
	incompleteItems.clear();
	orders.clear();
	ordid2idx.clear();
	type2idcs.clear();

	operID2Idx.clear();
	incompleteOperID2Idx.clear();
	itemID2Idx.clear();
	incompleteItemID2Idx.clear();
}

void OrderManager::clear() {
	QTextStream out(stdout);

	out << "OrderManager::clear : clearing operations" << endl;
	operations.clear();
	incompleteOperations.clear();
	operIDPrecedences.clear();
	operIDPrecedencesConj.clear();
	out << "OrderManager::clear : done clearing operations" << endl;

	out << "OrderManager::clear : clearing items" << endl;
	out << "Number of items to clear : " << items.size() << endl;
	for (int i = 0; i < items.size(); i++) {
		out << items[i] << endl;
	}
	//getchar();
	items.clear();
	incompleteItems.clear();
	out << "OrderManager::clear : done clearing items" << endl;

	out << "OrderManager::clear : clearing orders" << endl;
	orders.clear();
	out << "OrderManager::clear : done clearing orders" << endl;

	type2idcs.clear();
	ordid2idx.clear();

	operID2Idx.clear();
	incompleteOperID2Idx.clear();
	itemID2Idx.clear();
	incompleteItemID2Idx.clear();
}

QList<Order*> OrderManager::ordersByType(const int type) {
	QList<int> indices = type2idcs[type];
	QList<Order*> res;

	for (int i = 0; i < indices.size(); i++) {
		res.append(&orders[indices[i]]);
	}

	return res;
}

