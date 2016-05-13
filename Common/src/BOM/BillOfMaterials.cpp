/* 
 * File:   BillOfMaterials.cpp
 * Author: DrSobik
 * 
 * Created on July 29, 2011, 11:02 AM
 */

#include "BillOfMaterials.h"

int BillOfMaterials::bomsCreated = 0;
int BillOfMaterials::bomsDeleted = 0;

BillOfMaterials::BillOfMaterials() : itypeID(graph), itemID(graph) {
	bomsCreated++;
}

BillOfMaterials::BillOfMaterials(const BillOfMaterials& other) : itypeID(graph), itemID(graph) {
	*this = other;
	
	bomsCreated++;
}

BillOfMaterials::~BillOfMaterials() {
	//QTextStream out(stdout);
	
	bomsDeleted++;
	
	//out << "BOMs created : " << bomsCreated << endl;
	//out << "BOMs deleted : " << bomsDeleted << endl;
}

void BillOfMaterials::init(){
	clear();
	
	// Create a head and a tail
	head = graph.addNode();
	itypeID[head] = 0;
	itemID[head] = -2;
	
	tail = graph.addNode();
	itypeID[tail] = 0;
	itemID[tail] = -1;
}
	
void BillOfMaterials::clear(){
	ID = -1;
	
	graph.clear();
	
	head = INVALID;
	tail = INVALID;
}

BillOfMaterials& BillOfMaterials::operator=(const BillOfMaterials& other) {
	ID = other.ID;

	//Debugger::info << "BillOfMaterials::operator= ... "<< ENDL;
	
	digraphCopy(other.graph, this->graph).nodeMap(other.itypeID, this->itypeID).nodeMap(other.itemID, this->itemID).node(other.head, this->head).node(other.tail, this->tail).run();

	//Debugger::info << "BillOfMaterials::operator= done "<< ENDL;
	
	return *this;
}

void BillOfMaterials::fromDOMElement(const QDomElement& domel) {
	QDomNodeList bom_node_list; // List of nodes of the BOM in the DOM document
	QDomNodeList bom_arc_list; // List of arcs of the BOM in the DOM document
	QDomNode cur_node;
	QHash<int, ListDigraph::Node> node_map; // <node_id, actual_node>
	QMap<ListDigraph::Node, int> node_itm_type_map; // <actual_node, item_type>
	QList<QPair<int, int> > bom_arcs; // List of pairs of node_ids
	QStringList bom_arcs_str; // String representation of BOM arcs
	int bom_id;

	bom_id = domel.attribute("id").toInt();

	ID = bom_id;
	//out << "Current read BOM id: " << pboms[prod_type_id].last()->ID << endl;

	// Read the node descriptions
	cur_node = domel.firstChildElement("nodes");
	bom_node_list = cur_node.childNodes();
	node_map.clear();
	node_itm_type_map.clear();
	for (int k = 0; k < bom_node_list.size(); k++) {
		node_map[bom_node_list.item(k).toElement().attribute("id").toInt()] = graph.addNode();
		node_itm_type_map[node_map[bom_node_list.item(k).toElement().attribute("id").toInt()]] = bom_node_list.item(k).toElement().text().toInt();
		//out << "Read BOM node with id: " << bom_node_list.item(k).toElement().attribute("id").toInt() << endl;
	}

	// Read the arc descriptions
	cur_node = domel.firstChildElement("arcs");
	bom_arc_list = cur_node.childNodes();
	bom_arcs.clear();
	for (int k = 0; k < bom_arc_list.size(); k++) {
		bom_arcs_str = bom_arc_list.item(k).toElement().text().split(",");
		bom_arcs.append(QPair<int, int>());
		bom_arcs.last().first = bom_arcs_str[0].toInt();
		bom_arcs.last().second = bom_arcs_str[1].toInt();
		//out << "Read BOM arc: " << bom_arcs.last().first << "," << bom_arcs.last().second << endl;
	}

	// Create the actual BOM
	for (int ca = 0; ca < bom_arcs.size(); ca++) {
		// Add the current arc to the BOM graph
		graph.addArc(node_map[bom_arcs[ca].first], node_map[bom_arcs[ca].second]);
	}

	// Set the types of the items for each non-fictive node of the BOM graph
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		itypeID[nit] = node_itm_type_map[nit];
	}

	// Add the head and the tail
	head = graph.addNode();
	itypeID[head] = -1;
	tail = graph.addNode();
	itypeID[tail] = -2;

	// Add the arcs from the head and the tail
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		if (nit == head) continue;
		if (nit == tail) continue;
		// If the current node has no incoming arcs then it must be connected to the head 
		if (countInArcs(graph, nit) == 0) {
			graph.addArc(head, nit);
		}
		// If the current node has no outgoing arcs then it must be connected to the tail 
		if (countOutArcs(graph, nit) == 0) {
			graph.addArc(nit, tail);
		}
	}

	setItemIDs();

}

QTextStream& operator<<(QTextStream& out, BillOfMaterials& bm) {
	out << "BOM " << bm.ID << " : [" << endl;

	for (ListDigraph::NodeIt ni(bm.graph); ni != INVALID; ++ni) {
		//out << bm.graph.id(ni) << endl;
		out << bm.itemID[ni] << ":" << bm.itypeID[ni] << endl;
	}

	for (ListDigraph::ArcIt ai(bm.graph); ai != INVALID; ++ai) {
		//out << "(" << bm.graph.id(bm.graph.source(ai)) << ":" << bm.itypeID[bm.graph.source(ai)] << "->"
		//		<< bm.graph.id(bm.graph.target(ai)) << ":" << bm.itypeID[bm.graph.target(ai)] << ")" << endl;
		out << "(" << bm.itemID[bm.graph.source(ai)] << ":" << bm.itypeID[bm.graph.source(ai)] << "->"
				<< bm.itemID[bm.graph.target(ai)] << ":" << bm.itypeID[bm.graph.target(ai)] << ")" << endl;
	}

	out << "]";

	return out;
}

QXmlStreamReader& operator>>(QXmlStreamReader& reader, BillOfMaterials& bm) {
	QTextStream out(stdout);

	QHash<int, ListDigraph::Node> nodeID2Node;

	// IMPORTANT!!! It is assumed that the reader has just read a token named "bom" !!!

	bm.ID = reader.attributes().value("id").toString().toInt();
	
	while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "bom")) { // Parse the BOM

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "nodes") { // Parse the nodes
			while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "nodes")) {

				reader.readNext();

				if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "node") { // Parse the node
					// The node's ID
					int nodeID = reader.attributes().value("id").toString().toInt();

					// Add a new graph node
					ListDigraph::Node curNode = bm.graph.addNode();
					nodeID2Node[nodeID] = curNode;

					// Part type
					reader.readNext();
					int partType = reader.text().toString().toInt();

					// Set the part type of the node
					bm.itypeID[curNode] = partType;

					//out << "Node " << nodeID << " : " << partType << endl;

				} // node

			} // while

		} // nodes

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "arcs") { // Parse the arcs
			while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "arcs")) {

				reader.readNext();

				if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "arc") { // Parse the arc									

					reader.readNext();
					QStringList sl = reader.text().toString().split(",");
					int startNodeID = sl[0].toInt();
					int endNodeID = sl[1].toInt();

					//out << "Arc " << "(" << sl[0] << "," << sl[1] << ")" << endl;

					// Add an arc
					bm.graph.addArc(nodeID2Node[startNodeID], nodeID2Node[endNodeID]);

				} // arc

			} // while

		} // arcs

	} // BOM

	// Add the head and the tail
	bm.head = bm.graph.addNode();
	bm.itypeID[bm.head] = -1;
	bm.tail = bm.graph.addNode();
	bm.itypeID[bm.tail] = -2;

	// Add the arcs from the head and the tail
	for (ListDigraph::NodeIt nit(bm.graph); nit != INVALID; ++nit) {
		if (nit == bm.head) continue;
		if (nit == bm.tail) continue;
		// If the current node has no incoming arcs then it must be connected to the head 
		if (countInArcs(bm.graph, nit) == 0) {
			bm.graph.addArc(bm.head, nit);
		}
		// If the current node has no outgoing arcs then it must be connected to the tail 
		if (countOutArcs(bm.graph, nit) == 0) {
			bm.graph.addArc(nit, bm.tail);
		}
	}

	bm.setItemIDs();
	
	return reader;
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& writer, BillOfMaterials&) {

	Debugger::err << "operator<<(QXmlStreamWriter &writer, BillOfMaterials &bm) : Not implemented yet!" << ENDL;

	return writer;
}

void BillOfMaterials::setItemIDs() {
	// Iterate over the nodes of the BOM and set an item ID ( not item type !!! ) to each node. Head has ID "-2" and tail has ID "-1"
	int partCtr = 1;
	for (ListDigraph::NodeIt nit(graph); nit != INVALID; ++nit) {
		if (nit == head) {
			itemID[nit] = -2;
		} else {
			if (nit == tail) {
				itemID[nit] = -1;
			} else {
				itemID[nit] = partCtr;
				partCtr++;
			}
		}
	}
}