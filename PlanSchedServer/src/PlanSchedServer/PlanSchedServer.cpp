/* 
 * File:   PlanSchedServer.cpp
 * Author: DrSobik
 * 
 * Created on August 2, 2013, 12:52 PM
 */

#include "PlanSchedServer.h"

PlanSchedServer::PlanSchedServer() : socket(NULL)/*, scheduler(NULL)*/ {
    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection()));

    // Notify the Server when the Algorithm finishes
    //    connect(&solver, SIGNAL(sigFinished()), this, SLOT(computationFinished()));
}

PlanSchedServer::PlanSchedServer(const PlanSchedServer&) : QTcpServer(0), Parser<void, const Settings&>() {
}

PlanSchedServer::~PlanSchedServer() {
}

void PlanSchedServer::clear() {
    QTextStream out(stdout);

    out << "Clearing all... " << endl;

    // Delete the connection socket
    if (socket != NULL) {
	delete socket;
	socket = NULL;
    }

    // Clear the messages
    inMessage.clear();
    outMessage.clear();

    // Clear the resources
    rc.clear();
    //getchar();

    out << "Clearing : routes" << endl;
    // Clear the routes
    for (QHash<int, QList<Route*> >::iterator iter = itype2Routes.begin(); iter != itype2Routes.end(); iter++) {
	for (int i = 0; i < iter.value().size(); i++) {
	    delete iter.value()[i];
	}
    }
    itype2Routes.clear();
    out << "Done clearing : routes" << endl;

    out << "Clearing : BOMs" << endl;
    // Clear the BOMs
    for (QHash<int, QList<BillOfMaterials*> >::iterator iter = ptype2Boms.begin(); iter != ptype2Boms.end(); iter++) {
	for (int i = 0; i < iter.value().size(); i++) {
	    delete iter.value()[i];
	}

	iter.value().clear();
    }
    ptype2Boms.clear();
    out << "Done clearing : BOMs" << endl;

    out << "Clearing : BOPs" << endl;
    // Clear the BOPs
    for (QHash<int, QList<BillOfProcesses*> >::iterator iter = ptype2Bops.begin(); iter != ptype2Bops.end(); iter++) {
	for (int i = 0; i < iter.value().size(); i++) {
	    delete iter.value()[i];
	}

	iter.value().clear();
    }
    ptype2Bops.clear();
    out << "Done clearing : BOPs" << endl;

    out << "Clearing : prodman" << endl;
    prodman.clear(); // Points to the orders in orders
    out << "Done clearing : prodman" << endl;
    //getchar();

    out << "Clearing : ordman" << endl;
    ordman.clear(); // Points to the products in products
    out << "Done clearing : ordman" << endl;
    //getchar();

    out << "Clearing : products" << endl;
    products.clear();
    out << "Done clearing : products" << endl;
    //getchar();

    out << "Clearing : PMM" << endl;
    pmm.clear();
    out << "Done clearing : PMM" << endl;
    //getchar();

    out << "Clearing : options" << endl;
    options.clear();
    out << "Done clearing : options" << endl;

    out << "Clearing : solver" << endl;
    //solver.clear();
    out << "Done clearing : solver" << endl;

    out << "Clearing : scheduler" << endl;
    //    if (scheduler != NULL) {
    //        delete scheduler;
    //        scheduler = NULL;
    //    }
    sched.clear();
    out << "Done clearing : scheduler" << endl;

    out << "Clearing : protocol" << endl;
    protoFile.close();
    protocol.clear();
    out << "Done clearing : protocol" << endl;

    origOrdID2OrigOrdType.clear();
    origOrdID2OrigOrdBOM.clear();

    out << "Everything is clear." << endl;

}

void PlanSchedServer::readResources(const QByteArray& message) {
    QTextStream out(stdout);
    QXmlStreamReader reader(message);

    out << "PlanSchedServer::readResources : Parsing resources ..." << endl;
    rc.ID = -1;
    rc.clear();

    while (!reader.atEnd()) {
	QXmlStreamReader::TokenType token = reader.readNext();

	//out << reader.tokenString() << endl;

	if (token == QXmlStreamReader::StartDocument) {
	    continue;
	}

	if (token == QXmlStreamReader::StartElement && reader.name() == "resources") { // Reading the resources

	    reader.readNextStartElement(); // resource

	    rc.ID = reader.attributes().value("id").toString().toInt();

	    reader.readNextStartElement(); // toolgroups

	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "toolgroups")) { // Read the tool groups
		reader.readNext(); // toolgroup

		if (reader.name() == "toolgroup" && reader.tokenType() == QXmlStreamReader::StartElement) { // Parse the tool group

		    //out << "Reading machine group : " << reader.attributes().value("id").toString() << endl;

		    // Create a new machine group
		    ToolGroup* tg = new ToolGroup;

		    tg->ID = reader.attributes().value("id").toString().toInt();

		    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "toolgroup")) { // Parsing the machine group

			reader.readNext();

			if (reader.name() == "machines" && reader.tokenType() == QXmlStreamReader::StartElement) { // Parse the machines in this tool group

			    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "machines")) {

				reader.readNext();

				if (reader.name() == "machine" && reader.tokenType() == QXmlStreamReader::StartElement) {

				    //out << "Reading machine : " << reader.attributes().value("id").toString() << endl;

				    Machine* m = new Machine;

				    reader >> *m;

				    /*
				    m->ID = reader.attributes().value("id").toString().toInt(); // ID of the machine

				    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "machine")) { // Parse the processing characteristics

					    reader.readNext(); // type2speed

					    if (reader.name() == "type2speed") {

						    reader.readNext(); // the type2speed string

						    QStringList sl = reader.text().toString().split(",");

						    m->type2speed[sl[0].toInt()] = sl[1].toDouble();

						    reader.readNext(); // end of type2speed

					    }

				    }
				     */

				    // Add the machine to the current machine group
				    *tg << m;

				}

			    }

			}

		    }

		    // Add the machine group to the resources
		    rc << tg;

		}

	    }

	}

    }

    if (reader.hasError()) {

	out << reader.errorString() << endl;
	Debugger::err << "PlanSchedServer::readResources : XML reader encountered an error!" << ENDL;
    }

    out << rc << endl;

    out << "PlanSchedServer::readResources : Done parsing resources." << endl;
    //getchar();

}

void PlanSchedServer::readRoutes(const QByteArray& message) {
    QTextStream out(stdout);
    QXmlStreamReader reader(message);

    out << "PlanSchedServer::readRoutes : Parsing item routes ..." << endl;

    itype2Routes.clear();

    while (!reader.atEnd()) {

	reader.readNext();

	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "itypes") {

	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "itypes")) { // Parse the itypes

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "itype") {
		    int itypeID = reader.attributes().value("id").toString().toInt();

		    //out << "Reading item type : " << itypeID << endl;

		    itype2Routes.insert(itypeID, QList<Route*>());

		    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "itype")) { // Parse the itype

			reader.readNext();

			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "routes") {
			    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "routes")) { // Parse the routes

				reader.readNext();

				if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "route") {

				    Route curRoute;

				    reader >> curRoute;

				    //out << "Read route : " << curRoute << endl;
				    //getchar();

				    // Add the route for the current part type
				    itype2Routes[itypeID].append(new Route(curRoute));

				}

			    }

			}

		    }

		}

	    }

	}

    }

    if (reader.hasError()) {

	out << reader.errorString() << endl;
	Debugger::err << "PlanSchedServer::readRoutes : XML reader encountered an error!" << ENDL;
    }

    for (QHash<int, QList<Route*> >::iterator iter = itype2Routes.begin(); iter != itype2Routes.end(); iter++) {
	out << "Itype : " << iter.key() << endl;

	for (int i = 0; i < iter.value().size(); i++) {
	    out << *(iter.value()[i]) << endl;
	}

    }

    out << "PlanSchedServer::readRoutes : Done parsing routes." << endl;

}

void PlanSchedServer::readProducts(const QByteArray& message) {
    QTextStream out(stdout);
    QXmlStreamReader reader(message);

    out << "PlanSchedServer::readProducts : Parsing BOMs ..." << endl;

    ptype2Boms.clear();

    while (!reader.atEnd()) {

	reader.readNext();

	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "ptypes") {

	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "ptypes")) { // Parse the products

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "ptype") {
		    int ptypeID = reader.attributes().value("type").toString().toInt();

		    //out << "Reading product " << ptypeID << endl;

		    ptype2Boms.insert(ptypeID, QList<BillOfMaterials*>());

		    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "ptype")) { // Parse the product till the end

			reader.readNext();

			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "boms") {
			    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "boms")) { // Parse the BOMs

				reader.readNext();

				if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "bom") { // Parse the BOM
				    BillOfMaterials curBOM;

				    reader >> curBOM;

				    ptype2Boms[ptypeID].append(new BillOfMaterials(curBOM));

				    //ptype2Boms[ptypeID].append(new BillOfMaterials);
				    //reader >> *ptype2Boms[ptypeID].last();

				    //out << "Read BOM " << *ptype2Boms[ptypeID].last() << endl;
				    //getchar();

				}

			    }
			}

		    }

		}

	    }

	}

    }

    if (reader.hasError()) {

	out << reader.errorString() << endl;
	Debugger::err << "PlanSchedServer::readProducts : XML reader encountered an error!" << ENDL;
    }

    for (QHash<int, QList<BillOfMaterials*> >::iterator iter = ptype2Boms.begin(); iter != ptype2Boms.end(); iter++) {
	out << "Read product " << iter.key() << endl;

	for (int i = 0; i < iter.value().size(); i++) {
	    out << *(iter.value()[i]) << endl;
	}

    }

    out << "PlanSchedServer::readProducts : Done parsing BOMs." << endl;
}

void PlanSchedServer::readOperationsAndPartsAndOrders(const QByteArray& message) {
    QTextStream out(stdout);
    QXmlStreamReader reader(message);

    out << "PlanSchedServer::readOrders : Parsing orders(units) and parts ..." << endl;

    ordman.clear();

    while (!reader.atEnd()) {

	reader.readNext();

	// Reading operations which are in the system and haven't started yet
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "operations") {
	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "operations")) { // Parse the already existing operations

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "operation") { // Parse an operation
		    Operation curOper(1.0);

		    reader >> curOper;

		    ordman << curOper;

		    //out << "Read operation : " << curOper << endl;
		    //getchar();
		}

	    }

	}

	// Reading precedence constraints between the operations 
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "precedences") {
	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "precedences")) { // Parse the already existing precedence constraint

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "arc") { // Parse a precedence constraint
		    QPair<int, int> curPrec;
		    bool curPrecConj = false;

		    QStringList sl = reader.readElementText().split(",");

		    curPrec.first = sl[0].toInt();
		    curPrec.second = sl[1].toInt();

		    curPrecConj = (sl[2] == "0") ? false : true;


		    ordman.operIDPrecedences.append(curPrec);
		    ordman.operIDPrecedencesConj.append(curPrecConj);

		    //out << "Read precedence constraint : " << curPrec.first << "->" << curPrec.second << " " << ((curPrecConj) ? "1" : "0") << endl;
		    //getchar();
		}

	    }

	}

	// Reading parts which are already in the system
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "parts") {
	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "parts")) { // Parse the already existing parts

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "part") { // Parse a part
		    Item curItem;

		    reader >> curItem;

		    ordman << curItem;

		    //out << "Read item : " << curItem << endl;
		    //getchar();
		}

	    }

	}

	// Reading units which are in the system + which the new ones
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "units") {

	    while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "units")) { // Parse the units

		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.name() == "unit") { // Parse the unit

		    Order curOrder;

		    reader >> curOrder;

		    // Check whether this order will get a new product
		    if (curOrder.action == Order::OA_PLAN_PARTS_SCHED) { // This order will get a new incomplete product type
			// Preserve some original data
			origOrdID2OrigOrdType[curOrder.ID] = curOrder.type;
			origOrdID2OrigOrdBOM[curOrder.ID] = curOrder.BID;

			curOrder.type = (curOrder.ID << 16);
		    }

		    // For all items of this order which have started and performing their last operation, set the initial release time of the succeeding items
		    if (curOrder.action == Order::OA_PLAN_PARTS_SCHED) {

			// Initialization
			for (int i = 0; i < curOrder.itemIDs.size(); i++) {
			    int curItemID = curOrder.itemIDs[i];
			    Item& curItem = ordman.itemByID(curItemID);

			    if (curItem.curStepIdx == curItem.operIDs.size() - 1) { // Last operation of the item

				QList<int> succItemIDs;

				// Find the successors of the current item
				for (int j = 0; j < curOrder.itemIDPrececences.size(); j++) {
				    QPair<int, int> curPrec = curOrder.itemIDPrececences[j];

				    if (curPrec.first == curItemID) { // Add a successor
					succItemIDs.append(curPrec.second);
				    }

				}

				// Iterate over all successors and set maximal release time
				for (int j = 0; j < succItemIDs.size(); j++) {
				    int curSuccItemID = succItemIDs[j];
				    Item& curSuccItem = ordman.itemByID(curSuccItemID);

				    curSuccItem.releaseTime = 0.0; // For initialization only
				}


			    } // Last operation

			}

			for (int i = 0; i < curOrder.itemIDs.size(); i++) {
			    int curItemID = curOrder.itemIDs[i];
			    Item& curItem = ordman.itemByID(curItemID);

			    if (curItem.curStepIdx == curItem.operIDs.size() - 1) { // Last operation of the item

				QList<int> succItemIDs;

				// Find the successors of the current item
				for (int j = 0; j < curOrder.itemIDPrececences.size(); j++) {
				    QPair<int, int> curPrec = curOrder.itemIDPrececences[j];

				    if (curPrec.first == curItemID) { // Add a successor
					succItemIDs.append(curPrec.second);
				    }

				}

				// Iterate over all successors and set maximal release time
				for (int j = 0; j < succItemIDs.size(); j++) {
				    int curSuccItemID = succItemIDs[j];
				    Item& curSuccItem = ordman.itemByID(curSuccItemID);

				    curSuccItem.releaseTime = Math::max(curSuccItem.releaseTime, curItem.releaseTime); // curItem.release time shows actually the finish time of the last operation
				}


				out << "Finish time of " << curItem.ID << " is " << curItem.releaseTime << endl;

			    } // Last operation

			}

		    } // OA_PLAN_PARTS_SCHED

		    ordman << curOrder;

		    //out << "Read order : " << curOrder << endl;
		    //getchar();

		}

	    }

	}

    }

    if (reader.hasError()) {

	out << reader.errorString() << endl;
	Debugger::err << "PlanSchedServer::readOrders : XML reader encountered an error!" << ENDL;
    }

    for (int i = 0; i < ordman.orders.size(); i++) {
	out << ordman.orders[i] << endl;
	//getchar();
    }

    out << "PlanSchedServer::readOrders : Done parsing orders and parts." << endl;
}

void PlanSchedServer::createProducts() {
    QTextStream out(stdout);

    /* ################  Generate BOPs for each product type  ##############  */
    out << "Generating BOPs for each product type..." << endl;
    // List of possible BOPs for each product type

    // Prepare the hash with the BOPs for different product types
    for (int i = 0; i < ptype2Boms.size(); i++) {
	ptype2Bops.insert(ptype2Boms.keys()[i], QList<BillOfProcesses*>());
    }

    // Iterate over all product types
    double total_comb = 1.0;
    double total_bops = 0.0;
    for (QHash<int, QList<BillOfProcesses*> >::iterator ptiter = ptype2Bops.begin(); ptiter != ptype2Bops.end(); ptiter++) {
	out << "BillOfProcesses::generateAll..." << endl;
	BillOfProcesses::generateAll(ptype2Boms[ptiter.key()], itype2Routes, ptype2Bops[ptiter.key()]);
	out << "Finished BillOfProcesses::generateAll." << endl;
	out << "Generated " << ptype2Bops[ptiter.key()].size() << " BOPs for product " << ptiter.key() << endl;
	total_comb *= double(ptype2Bops[ptiter.key()].size());
	total_bops += ptype2Bops[ptiter.key()].size();
    }
    out << "Done generating BOPs for each product type." << endl;
    out << "Total generated BOPs : " << total_bops << endl;
    out << "Total combinations for production : " << total_comb << endl;
    /* ################  Done generating the BOPs  #########################  */

    /* ##############  Create products one for each product type  ############*/
    out << "Creating products..." << endl;

    QList<int> ptypes = ptype2Boms.keys();
    for (int i = 0; i < ptypes.size(); i++) {
	products.append(Product());
	products.last().ID = ptypes[i]; //i + 1; //1; // Proruct::ID should be always equal to Product::type (basic assumption to make life easier)
	products.last().type = ptypes[i];
	for (int j = 0; j < ptype2Bops[products.last().type].size(); j++) {
	    // Generate the ID for the current BOM of the current product
	    //ptype2Bops[products.last().type][j]->ID = products.last().ID;
	    //ptype2Bops[products.last().type][j]->ID = ptype2Bops[products.last().type][j]->ID << 16; // Shift by 16 bits
	    //ptype2Bops[products.last().type][j]->ID += j;

	    // IMPORTANT!!! It is assumed that BOP ID = BOM ID

	    // Add the BOP to the product
	    products.last() << ptype2Bops[products.last().type][j];
	}

	//products.last().bopsDecompRate(0.0);
	//products.last().rc = &rc;
	products.last().rankBOPs(rc);

	out << "Created product: " << products.last() << endl;
    }
    out << "Done creating products." << endl;
    /* ##############  Done creating products  ############################## */

    out << "Creating product manager ..." << endl;
    for (int i = 0; i < products.size(); i++) {
	prodman << &products[i];
    }
    out << "Product manager created." << endl;
}

void PlanSchedServer::createOrders() {
    QTextStream out(stdout);

    out << "Creating order manager ..." << endl;
    //for (int i = 0; i < orders.size(); i++) {
    //	ordman << orders[i];
    //}
    out << "Order manager created." << endl;
}

void PlanSchedServer::createPMM() {
    QTextStream out(stdout);

    // Add the products and the orders to the process model manager
    out << "PlanSchedServer::createPMM : Creating process model manager ..." << endl;
    pmm << &prodman;
    pmm << &ordman;
    pmm.init();
    out << "PlanSchedServer::createPMM : Process model manager created ..." << endl;

    out << "PlanSchedServer::createPMM : Updating the global process model ..." << endl;
    pmm.updatePM();
    out << pmm.pm << endl;
    out << "PlanSchedServer::createPMM : Done updating the global process model." << endl;
    out << "PlanSchedServer::createPMM : Nodes in the updated process model: " << countNodes(pmm.pm.graph) << endl;
}

QHash<int, BillOfMaterials > createIncompleteBOMs(OrderManager& ordMan) {

    // Consists of the parts within the orders

    QTextStream out(stdout);

    QHash<int, BillOfMaterials > ordID2Bom;

    for (int i = 0; i < ordMan.orders.size(); i++) {
	Order& curOrder = (Order&) ordMan.orders[i];

	// Check whether this order is for part replanning only
	if (curOrder.action != Order::OA_PLAN_PARTS_SCHED) continue;

	BillOfMaterials curBOM;

	curBOM.init();

	curBOM.ID = curOrder.type; // This would generate a unique ID for the BOM in view of other orders and products
	QHash<int, ListDigraph::Node> itemID2Node;

	// Iterate over the items of the order
	for (int j = 0; j < curOrder.itemIDs.size(); j++) {
	    int curItemID = curOrder.itemIDs[j];
	    Item& curItem = (Item&) ordMan.itemByID(curItemID);

	    // BUG!!! This condition leads to a situation where not all arcs are added to the BOM!!! Solution: create a complete BOM and then exclude the started items
	    //if (curItem.curStepIdx < 0) { // This item has not been started -> add it to the incomplete BOM
	    ListDigraph::Node curNode;

	    // Create an additional node in the BOM
	    curNode = curBOM.graph.addNode();

	    itemID2Node[curItem.ID] = curNode;

	    // Set the type of the item
	    curBOM.itypeID[curNode] = curItem.type;

	    // Set the unique ID of the item
	    curBOM.itemID[curNode] = curItem.ID;
	    //}

	}

	// Set the precedence constraints between the items
	for (int j = 0; j < curOrder.itemIDPrececences.size(); j++) {
	    int startItemID = curOrder.itemIDPrececences[j].first;
	    int endItemID = curOrder.itemIDPrececences[j].second;

	    // BUG!!! This condition in error prone
	    //if (ordMan.itemByID(startItemID).curStepIdx < 0 && ordMan.itemByID(endItemID).curStepIdx < 0) { // These items have not started yet -> add to the incomplete BOM

	    ListDigraph::Node startNode = itemID2Node[startItemID];
	    ListDigraph::Node endNode = itemID2Node[endItemID];

	    curBOM.graph.addArc(startNode, endNode);

	    //}

	}

	// Connect head and tail of the BOM
	QList<ListDigraph::Node> nodesNoPreds;
	QList<ListDigraph::Node> nodesNoSuccs;

	for (ListDigraph::NodeIt nit(curBOM.graph); nit != INVALID; ++nit) {
	    if (countInArcs(curBOM.graph, nit) == 0 && nit != curBOM.head && nit != curBOM.tail) {
		nodesNoPreds.append(nit);
	    }

	    if (countOutArcs(curBOM.graph, nit) == 0 && nit != curBOM.head && nit != curBOM.tail) {
		nodesNoSuccs.append(nit);
	    }
	}

	for (int j = 0; j < nodesNoPreds.size(); j++) {
	    ListDigraph::Node curNode = nodesNoPreds[j];

	    curBOM.graph.addArc(curBOM.head, curNode);
	}

	for (int j = 0; j < nodesNoSuccs.size(); j++) {
	    ListDigraph::Node curNode = nodesNoSuccs[j];

	    curBOM.graph.addArc(curNode, curBOM.tail);
	}

	out << "Complete BOM : " << endl << curBOM << endl;
	
	// IMPORTANT!!! Replace items which have already started with "incomplete parts"

	QList<ListDigraph::Node> nodesReplace;
	// Find the nodes which have to be replaced
	for (ListDigraph::NodeIt nit(curBOM.graph); nit != INVALID; ++nit) {

	    if (nit == curBOM.head || nit == curBOM.tail) continue;

	    int curItemID = curBOM.itemID[nit];
	    Item& curItem = (Item&) ordMan.itemByID(curItemID);

	    if (curItem.curStepIdx >= 0) { // This item should be replaced
		nodesReplace.append(nit);
	    }
	}

	

	// Remove the collected nodes
	for (int j = 0; j < nodesRem.size(); j++) {
	    ListDigraph::Node curNode = nodesRem[j];
	    QList<ListDigraph::Node> curInNodes; // Incoming nodes for the current node
	    QList<ListDigraph::Node> curOutNodes; // Outgoing nodes
	    QList<ListDigraph::Arc> curInArcs; // Incoming arcs for the current node
	    QList<ListDigraph::Arc> curOutArcs; // Outgoing arcs

	    // Collect the incoming nodes
	    for (ListDigraph::InArcIt iait(curBOM.graph, curNode); iait != INVALID; ++iait) {
		ListDigraph::Node curInNode = curBOM.graph.source(iait);

		//if (curInNode != curBOM.head) {
		curInNodes.append(curInNode);
		curInArcs.append(iait);
		//}
	    }

	    // Collect the outgoing nodes
	    for (ListDigraph::OutArcIt oait(curBOM.graph, curNode); oait != INVALID; ++oait) {
		ListDigraph::Node curOutNode = curBOM.graph.target(oait);

		//if (curOutNode != curBOM.tail) {
		curOutNodes.append(curOutNode);
		curOutArcs.append(oait);
		//}
	    }

	    // Delete all incoming arcs
	    for (int k = 0; k < curInArcs.size(); k++) {
		ListDigraph::Arc curArc = curInArcs[k];
		curBOM.graph.erase(curArc);
	    }

	    // Delete all outgoing arcs
	    for (int k = 0; k < curOutArcs.size(); k++) {
		ListDigraph::Arc curArc = curOutArcs[k];
		curBOM.graph.erase(curArc);
	    }

	    // For each outgoing node connect it with each incoming one only if such arc does not exist already
	    for (int k = 0; k < curOutNodes.size(); k++) {
		ListDigraph::Node curOutNode = curOutNodes[k];

		for (int m = 0; m < curInNodes.size(); m++) {
		    ListDigraph::Node curInNode = curInNodes[m];

		    bool arcExists = false;

		    // Check whether such arc already exists
		    for (ListDigraph::OutArcIt oait(curBOM.graph, curInNode); oait != INVALID; ++oait) {
			if (curBOM.graph.target(oait) == curOutNode) { // Such arc already exists
			    arcExists = true;
			    break;
			}
		    }

		    if (!arcExists) curBOM.graph.addArc(curInNode, curOutNode);
		}

	    }

	    out << "Erasing item : " << curBOM.itemID[curNode] << endl;

	    // Remove the node
	    curBOM.graph.erase(curNode);

	} // Removing correctly the collected nodes

	//out << "Generated incomplete BOM" << endl;
	//out << curBOM << endl;
	//getchar();

	ordID2Bom.insert(curOrder.ID, curBOM);

	//out << "Saved it" << endl;
	//getchar();

    }

    return ordID2Bom;
}

void PlanSchedServer::createIncompleteProducts() {
    QTextStream out(stdout);

    QHash<int, BillOfMaterials> icplProdType2IcplBom;
    QHash<int, BillOfMaterials> ordID2IcplBoms = createIncompleteBOMs(ordman);

    for (QHash<int, BillOfMaterials>::iterator iter = ordID2IcplBoms.begin(); iter != ordID2IcplBoms.end(); iter++) {
	int curOrdID = iter.key();
	BillOfMaterials curBOM = iter.value();

	out << "Incomplete BOM : " << endl << curBOM << endl;

	int curProdID = curBOM.ID;

	icplProdType2IcplBom.insert(curProdID, curBOM);

	// Set the order type to the new incomplete product
	ordman.orderByID(curOrdID).type = curProdID;

	//ordman.orderByID(curOrdID).BID = curBOM.ID;  The BID must stay the same as in the simulation

	// Add the current BOM
	ptype2Boms[curProdID] = QList<BillOfMaterials*>();
	ptype2Boms[curProdID].append(new BillOfMaterials(curBOM));

	// Add the current placeholder for the BOP
	ptype2Bops[curProdID] = QList<BillOfProcesses*>();

	// Generate the current BOP
	//out << "Generating incomplete BOPs ... " << endl;
	BillOfProcesses::generateAll(ptype2Boms[curProdID], itype2Routes, ptype2Bops[curProdID]);
	//out << "Done generating incomplete BOPs. " << endl;

	// Append the incomplete product
	products.append(Product());
	products.last().ID = curProdID;
	products.last().type = curProdID;
	for (int j = 0; j < ptype2Bops[products.last().type].size(); j++) {
	    // IMPORTANT!!! It is assumed that BOP ID = BOM ID

	    // Add the BOP to the product
	    products.last() << ptype2Bops[curProdID][j];

	    //out << "Adding BOP : " << endl << *ptype2Bops[curProdID][j] << endl;
	    //getchar();
	}

	//products.last().bopsDecompRate(0.0);
	//products.last().rc = &rc;
	products.last().rankBOPs(rc);

	out << "Incomplete product : " << endl << products.last() << endl;
	//getchar();

	// Add the incomplete product to the prodman
	prodman << &products.last();
    }

}

void PlanSchedServer::prepareForPlanningAndScheduling() {
    //    QTextStream out(stdout);
    //
    //    out << "Preparing for planning/scheduling" << endl;
    //    //getchar();
    //
    //    // Set the planner/scheduler options
    //    options.clear();
    //    options["PlannerInitRule"] = /*"RND";//*/"TPT_RANK";
    //    options["NS"] = "N3_N2";
    //    options["StepN1"] = "1";
    //    options["StepN2"] = "1";
    //    options["StepN3"] = "1";
    //    options["NumNeigh"] = "1";
    //    options["PrioScheduler"] = "CS_LS"; //*/"ATC_K";
    //    options["GlobMaxIter"] = "100";
    //    options["GlobMaxTimeM"] = "300";
    //    options["RandSeed"] = "1325467";
    //    //options["LS_CHK_COR"] = "true";
    //    //options["LS_MAX_ITER"] = "0";
    //    //options["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";
    //
    //    //if (!options.contains("Protocol")) {
    //    //	Debugger::err << "No protocol specified" << ENDL;
    //    //} else {
    //    //	proto_file.setFileName(test_inst_dir + options["Protocol"]);
    //    //	out << "Protocol file : " << proto_file.fileName() << endl;
    //    //}
    //
    //    if (!options.contains("GlobMaxIter")) {
    //	Debugger::err << "No number of global iterations specified!" << ENDL;
    //    }
    //
    //    if (!options.contains("GlobMaxTimeM")) {
    //	Debugger::err << "No global time limit specified!" << ENDL;
    //    }
    //
    //    if (!options.contains("RandSeed")) {
    //	Debugger::err << "No seed for randomization specified!" << ENDL;
    //    } else {
    //	// Set the seed 
    //	Rand::rndSeed(options["RandSeed"].toInt());
    //    }
    //
    //    if (!options.contains("PlannerInitRule")) {
    //	Debugger::err << "No initialization rule for the planner specified!" << ENDL;
    //    }
    //
    //    if (!options.contains("NS")) {
    //	Debugger::err << "No general NS for the planner specified!" << ENDL;
    //    }
    //
    //    if (!options.contains("PrioScheduler")) {
    //	Debugger::err << "No priority scheduler specified!" << ENDL;
    //    }
    //
    //    if (!options.contains("NumNeigh")) {
    //	Debugger::err << "No number of neighbors specified specified!" << ENDL;
    //    }
    //
    //
    //
    //    // Schedule object
    //    //Schedule sched;
    //
    //    // Scheduler object
    //    //Scheduler *scheduler = new SBHScheduler;
    //    //scheduler = new SBHScheduler;
    //
    //    // Protocol object
    //    protocol.init();
    //    protoFile.setFileName(qApp->applicationDirPath() + "/" + "protocol.xml");
    //    protocol.setFile(protoFile);
    //
    //    // The search algorithm
    //    //TrivialPlanner planner;
    //    //VNSPlanner planner;
    //
    //    if (options["PlannerInitRule"] == "RND") {
    //
    //	solver.solInitType = VNSPlanner::SOL_INIT_RND;
    //
    //    } else if (options["PlannerInitRule"] == "TPT_RANK") {
    //
    //	solver.solInitType = VNSPlanner::SOL_INIT_RANK;
    //
    //    } else {
    //
    //	Debugger::warn << "PlannerInitRule: " << options["PlannerInitRule"].toStdString() << ENDL;
    //	Debugger::err << "Incorrect planner initialization rule set!" << ENDL;
    //
    //    }
    //
    //
    //    if (options["NS"] == "N1") {
    //
    //	solver.ns = VNSPlanner::NS_N1;
    //
    //    } else if (options["NS"] == "N2") {
    //
    //	solver.ns = VNSPlanner::NS_N2;
    //
    //    } else if (options["NS"] == "N3") {
    //
    //	solver.ns = VNSPlanner::NS_N3;
    //
    //    } else if (options["NS"] == "N2N1") {
    //
    //	solver.ns = VNSPlanner::NS_N2N1;
    //
    //    } else if (options["NS"] == "N2N3") {
    //
    //	solver.ns = VNSPlanner::NS_N2N3;
    //
    //    } else if (options["NS"] == "N4") {
    //
    //	solver.ns = VNSPlanner::NS_N4;
    //
    //    } else if (options["NS"] == "N5") {
    //
    //	solver.ns = VNSPlanner::NS_N5;
    //
    //    } else {
    //
    //	Debugger::err << "No correct general NS for the planner specified!" << ENDL;
    //
    //    }
    //
    //    if (options.contains("StepN1")) {
    //
    //	solver.setStepN1(options["StepN1"].toInt());
    //
    //    }
    //    if (options.contains("StepN2")) {
    //
    //	solver.setStepN1(options["StepN2"].toInt());
    //
    //    }
    //    if (options.contains("StepN3")) {
    //
    //	solver.setStepN1(options["StepN3"].toInt());
    //
    //    }
    //
    //    if (options.contains("NumNeigh")) {
    //
    //	solver.setNumNeighbors(options["NumNeigh"].toInt());
    //
    //    }
    //
    //    if (options["PrioScheduler"] == "RND") {
    //
    //	solver.scheduler = new RNDScheduler;
    //
    //    } else if (options["PrioScheduler"] == "FIFO") {
    //
    //	solver.scheduler = new WFIFOScheduler;
    //	((WFIFOScheduler*) solver.scheduler)->weightedFIFO(false);
    //
    //    } else if (options["PrioScheduler"] == "WFIFO") {
    //
    //	solver.scheduler = new WFIFOScheduler;
    //	((WFIFOScheduler*) solver.scheduler)->weightedFIFO(true);
    //
    //    } else if (options["PrioScheduler"] == "W") {
    //
    //	solver.scheduler = new WScheduler;
    //
    //    } else if (options["PrioScheduler"] == "SPT") {
    //
    //	solver.scheduler = new WSPTScheduler;
    //	((WSPTScheduler*) solver.scheduler)->weightedSPT(false);
    //
    //    } else if (options["PrioScheduler"] == "WSPT") {
    //
    //	solver.scheduler = new WSPTScheduler;
    //	((WSPTScheduler*) solver.scheduler)->weightedSPT(true);
    //
    //    } else if (options["PrioScheduler"] == "EOD") {
    //
    //	//solver.scheduler = new EODScheduler;
    //	solver.scheduler = new WEODScheduler;
    //	((WEODScheduler*) solver.scheduler)->weightedEOD(false);
    //
    //    } else if (options["PrioScheduler"] == "WEOD") {
    //
    //	solver.scheduler = new WEODScheduler;
    //	((WEODScheduler*) solver.scheduler)->weightedEOD(true);
    //
    //    } else if (options["PrioScheduler"] == "WEDD2") {
    //
    //	solver.scheduler = new WEDD2Scheduler;
    //	((WEDD2Scheduler*) solver.scheduler)->weightedEDD(true);
    //
    //    } else if (options["PrioScheduler"] == "MDD") {
    //
    //	solver.scheduler = new WMDDScheduler;
    //	((WMDDScheduler*) solver.scheduler)->weightedMDD(false);
    //
    //    } else if (options["PrioScheduler"] == "WMDD") {
    //
    //	solver.scheduler = new WMDDScheduler;
    //	((WMDDScheduler*) solver.scheduler)->weightedMDD(true);
    //
    //    } else if (options["PrioScheduler"] == "ATC") {
    //
    //	solver.scheduler = new ATCANScheduler;
    //	((ATCANScheduler*) solver.scheduler)->considerSucc(false);
    //	((ATCANScheduler*) solver.scheduler)->kappaOptim(false);
    //
    //    } else if (options["PrioScheduler"] == "ATC_K") {
    //
    //	solver.scheduler = new ATCANScheduler;
    //	((ATCANScheduler*) solver.scheduler)->considerSucc(false);
    //	((ATCANScheduler*) solver.scheduler)->kappaOptim(true);
    //
    //    } else if (options["PrioScheduler"] == "ATC_CS") {
    //
    //	solver.scheduler = new ATCANScheduler;
    //	((ATCANScheduler*) solver.scheduler)->considerSucc(true);
    //	((ATCANScheduler*) solver.scheduler)->kappaOptim(false);
    //
    //    } else if (options["PrioScheduler"] == "ATC_CS_K") {
    //
    //	solver.scheduler = new ATCANScheduler;
    //	((ATCANScheduler*) solver.scheduler)->considerSucc(true);
    //	((ATCANScheduler*) solver.scheduler)->kappaOptim(true);
    //
    //    } else if (options["PrioScheduler"] == "MUB") {
    //
    //	solver.scheduler = new MUBScheduler;
    //
    //    } else if (options["PrioScheduler"] == "MTB") {
    //
    //	solver.scheduler = new MTBScheduler;
    //
    //    } else if (options["PrioScheduler"] == "T") {
    //
    //	solver.scheduler = new WTScheduler;
    //	((WTScheduler*) solver.scheduler)->weightedT(false);
    //
    //    } else if (options["PrioScheduler"] == "WT") {
    //
    //	solver.scheduler = new WTScheduler;
    //	((WTScheduler*) solver.scheduler)->weightedT(true);
    //
    //    } else if (options["PrioScheduler"] == "SBH") {
    //
    //	out << "Creating SBH scheduler ... " << endl;
    //
    //	solver.scheduler = new SBHScheduler;
    //
    //	// Set the tool group scheduler for the SBHScheduler
    //	out << "Creating TG scheduler ... " << endl;
    //	((SBHScheduler*) solver.scheduler)->tgscheduler = new TGVNSScheduler;
    //	((TGVNSScheduler*) ((SBHScheduler*) solver.scheduler)->tgscheduler)->maxIter(000);
    //	((TGVNSScheduler*) ((SBHScheduler*) solver.scheduler)->tgscheduler)->maxIterDecl(3000);
    //	((TGVNSScheduler*) ((SBHScheduler*) solver.scheduler)->tgscheduler)->maxTimeMs(30000);
    //	((TGVNSScheduler*) ((SBHScheduler*) solver.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) solver.scheduler;
    //	out << "Done. " << endl;
    //
    //	// Set the initial tool group scheduler 
    //	out << "Creating TG ini scheduler ... " << endl;
    //	((TGVNSScheduler*) ((SBHScheduler*) solver.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
    //	//((TGATCScheduler*) ((TGVNSScheduler1*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
    //	((TGATCScheduler*) ((TGVNSScheduler*) ((SBHScheduler*) solver.scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
    //	out << "Done. " << endl;
    //
    //	out << "Done. " << endl;
    //
    //    } else if (options["PrioScheduler"] == "CS") {
    //
    //	solver.scheduler = new CombinedScheduler;
    //
    //	Scheduler* sch = 0;
    //
    //	sch = new WFIFOScheduler;
    //	sch->ID = 1;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WFIFOScheduler*) sch)->weightedFIFO(false);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //	sch = new WFIFOScheduler;
    //	sch->ID = 2;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WFIFOScheduler*) sch)->weightedFIFO(true);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //
    //	sch = new WEODScheduler;
    //	sch->ID = 3;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WEODScheduler*) sch)->weightedEOD(false);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //	sch = new WEODScheduler;
    //	sch->ID = 4;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WEODScheduler*) sch)->weightedEOD(true);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //	sch = new WMDDScheduler;
    //	sch->ID = 5;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WMDDScheduler*) sch)->weightedMDD(false);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //	sch = new WMDDScheduler;
    //	sch->ID = 6;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WMDDScheduler*) sch)->weightedMDD(true);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //	sch = new ATCANScheduler;
    //	sch->ID = 7;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((ATCANScheduler*) sch)->considerSucc(false);
    //	((ATCANScheduler*) sch)->kappaOptim(true);
    //	*((CombinedScheduler*) solver.scheduler) << sch;
    //
    //	delete sch;
    //
    //    } else if (options["PrioScheduler"] == "CS_LS") {
    //
    //	CombinedSchedulerLS* csls = new CombinedSchedulerLS;
    //
    //	Scheduler* sch = 0;
    //
    //	sch = new WFIFOScheduler;
    //	sch->ID = 1;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WFIFOScheduler*) sch)->weightedFIFO(false);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = new WFIFOScheduler;
    //	sch->ID = 2;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WFIFOScheduler*) sch)->weightedFIFO(true);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = new WEODScheduler;
    //	sch->ID = 3;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WEODScheduler*) sch)->weightedEOD(false);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = new WEODScheduler;
    //	sch->ID = 4;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WEODScheduler*) sch)->weightedEOD(true);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = new WMDDScheduler;
    //	sch->ID = 5;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WMDDScheduler*) sch)->weightedMDD(false);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = new WMDDScheduler;
    //	sch->ID = 6;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((WMDDScheduler*) sch)->weightedMDD(true);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = new ATCANScheduler;
    //	sch->ID = 7;
    //	sch->rc.ID = (sch->ID << 16) + 1;
    //
    //	((ATCANScheduler*) sch)->considerSucc(false);
    //	((ATCANScheduler*) sch)->kappaOptim(true);
    //	csls->combinedSchedulerObject() << sch;
    //
    //	delete sch;
    //
    //	sch = 0;
    //
    //	//csls->localSearchObject().checkCorrectness(true);
    //	//csls->localSearchObject().maxIter(1000);
    //
    //	solver.scheduler = csls;
    //
    //	csls = NULL;
    //
    //    } else {
    //
    //	Debugger::err << "No feasible priority scheduling rule specified!" << ENDL;
    //
    //    }
    //
    //
    //    solver.maxIter(options["GlobMaxIter"].toInt());
    //    solver.maxTimeMs(options["GlobMaxTimeM"].toInt()* 60 * 1000);
    //    //planner.maxIterDecl(6);
    //    //planner.maxTimeMs(1 * 30 * 60 * 1000);
    //
    //    out << "Prepared for planning/scheduling" << endl;
    //    //getchar();
    //
    //    // Initialize the PMM so that the PM is correct
    //    pmm.init();
    //
    //    solver << &pmm << &sched << &rc << &protocol;



    pmm.init();

    ippsProblem.pmm = &pmm;
    ippsProblem.rc = &rc;

}

void PlanSchedServer::incomingConnection() {
    QTextStream out(stdout);

    /*******************************  Read the message  ***************************************************************/

    out << "PlanSchedServer::incomingConnection..." << endl;

    socket = nextPendingConnection();

    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(closedConnection()));

    // Wait until the connection is established
    if (!socket->waitForConnected(30000)) {
	emit socket->error();
	return;
    }

    // Wait until 8 bytes defining the size of the message are available
    while (socket->bytesAvailable() < (qint64) sizeof (quint64)) {
	if (!socket->waitForReadyRead(30000)) {
	    emit socket->error();
	    return;
	}
    }

    // Read the size of the message
    qint64 messageSize;
    QDataStream in(socket);
    in >> messageSize;

    // Wait until the whole message is available
    while (socket->bytesAvailable() < (qint64) messageSize) {
	if (!socket->waitForReadyRead(30000)) {
	    emit socket->error();
	    return;
	}
    }

    // Read the whole message
    inMessage = socket->read(messageSize);

    QMutex mutex;

    mutex.lock();

    out << "PlanSchedServer::incomingConnection : Receiving message of " << messageSize << " bytes" << endl;

    out << endl << inMessage << endl;

    mutex.unlock();

    //getchar();

    /******************************************************************************************************************/

    bool performPlanningAndScheduling = (inMessage == "Hello") ? false : true;

    if (performPlanningAndScheduling) {

	/**************************  Perform planning and scheduling   ****************************************************/

	// Read the resources
	readResources(inMessage);

	// Read the part routes
	readRoutes(inMessage);

	// Read product BOMs
	readProducts(inMessage);

	// Read the actual orders(unit) to be fulfilled
	readOperationsAndPartsAndOrders(inMessage);

	// Check the input correctness
	if (ordman.orders.size() == 0) {

	    Debugger::warn << "PlanSchedServer::incomingConnection : No orders provided -> performing no actions (clearing)!" << ENDL;

	    // Disconnect
	    socket->disconnectFromHost();
	    if (socket->state() == QAbstractSocket::UnconnectedState || socket->waitForDisconnected(10000)) {
		out << "Socket disconnected!" << endl;
	    } else {
		Debugger::err << "Failed to disconnect socket!" << ENDL;
	    }

	    // Clear
	    clear();

	    return;
	}

	// Create products
	createProducts();

	// Create orders and order manager
	createOrders();

	// Create PMM
	createPMM();

	// Check correctness of the input PM
	bool pmCorrect = true;
	//if (pmm.ordman->operations.size() == 0) { // Only head and tail

	//    out << "PlanSchedServer::incomingConnection : No operations provided -> performing no actions!" << endl;

	    //out << pmm.pm << endl;
	    
	//    pmCorrect = false;
	//}
	
	// Create incomplete products based on the started orders
	createIncompleteProducts();

	// Prepare everything for planning and scheduling (pmm.pm is not used by the solvers (old stuff))
	prepareForPlanningAndScheduling();

	if (pmCorrect) {

	    // Perform planning and scheduling ...
	    PlanSched ps;
	    //    solver.run();
	    ps = solver->solve(ippsProblem);

	    /******************************************************************************************************************/

	    /*****************************************  Collect the schedule  *************************************************/

	    //    Plan plan = solver.getBestPlan();
	    //    Schedule sched = solver.getBestSchedule();
	    Plan plan = ps.plan;
	    Schedule sched = ps.schedule;

	    // Set the proper product types and BOM IDs for operations of the incomplete products
	    ProcessModel schedPM = sched.pm;
	    for (ListDigraph::NodeIt nit(schedPM.graph); nit != INVALID; ++nit) {
		if (nit != schedPM.head && nit != schedPM.tail && origOrdID2OrigOrdType.contains(schedPM.ops[nit]->orderID())) { // This operation belongs to a fake incomplete product -> set the correct one
		    schedPM.ops[nit]->orderType(origOrdID2OrigOrdType[schedPM.ops[nit]->orderID()]);
		    schedPM.ops[nit]->bomID(origOrdID2OrigOrdBOM[schedPM.ops[nit]->orderID()]);
		}
	    }

	    TWT twt;
	    sched.fromPM(schedPM, twt);

	    out << "PlanSchedServer::computationFinished : Best plan is " << endl << plan << endl;

	    out << "PlanSchedServer::computationFinished : Best schedule is " << endl << sched << endl;

	    out << "PlanSchedServer::computationFinished : Best PM is " << endl << sched.pm << endl;

	    // Reconstruct parts and orders based on the best schedule
	    out << "Constructing parts and orders ... " << endl;
	    constructPartsAndOrders(sched.pm);
	    out << "Done constructing parts and orders." << endl;

	    QXmlStreamWriter composer(&outMessage);

	    composer.setAutoFormatting(true);

	    composer.writeStartDocument();

	    composer.writeStartElement("solution");

	    // Write the parts
	    composer.writeStartElement("parts");

	    for (int i = 0; i < ordman.items.size(); i++) {
		Item& curItem = (Item&) ordman.items[i];

		composer << curItem;
	    }

	    composer.writeEndElement(); // parts

	    // Write the orders/units
	    composer.writeStartElement("units");

	    for (int i = 0; i < ordman.orders.size(); i++) {
		composer << ordman.orders[i];
	    }

	    composer.writeEndElement(); // units	

	    // Write the schedule
	    composer << sched;

	    composer.writeEndElement(); // solution

	    composer.writeEndDocument();
	    /******************************************************************************************************************/
	    
	} else {
	    
	    outMessage = "<no actions performed>";
	    
	}

    } else { // Do not perform planning and scheduling (useful for testing)

	outMessage = "Hello there!";

    }


    /*****************************************  Reply   ***************************************************************/

    //qint64 messageSize; // Size of the message in bytes

    // Create a reply message
    //outMessage = "Hello from PlanSchedServer!";
    messageSize = outMessage.size();

    // Prepend the message's size
    for (uint i = 0; i < sizeof (quint64); i++) {
	outMessage.prepend(((uchar*) & messageSize)[i]);
    }

    // Send the message
    socket->write(outMessage);

    if (socket->waitForBytesWritten(-1)) {
	out << "Bytes are written!" << endl;
    } else {
	out << "Not all bytes are written!!!" << endl;
    }

    // Disconnect
    socket->disconnectFromHost();
    if (socket->state() == QAbstractSocket::UnconnectedState || socket->waitForDisconnected(10000)) {
	out << "Socket disconnected!" << endl;
    } else {
	Debugger::err << "Failed to disconnect socket!" << ENDL;
    }



    /******************************************************************************************************************/

    /*****************************************  Clear the server  *****************************************************/

    this->clear();

    /******************************************************************************************************************/

}

void PlanSchedServer::closedConnection() {
    QTextStream out(stdout);

    out << "PlanSchedServer::closedConnection : Socket disconnected from host." << endl;
}

void PlanSchedServer::connectionError(QAbstractSocket::SocketError) {
    QTextStream out(stdout);

    out << "PlanSchedServer::slotError : An error occured! " << socket->errorString() << endl;

}

void PlanSchedServer::constructPartsAndOrders(ProcessModel& pm) {
    QTextStream out(stdout);

    out << "PlanSchedServer::constructPartsAndOrders ... " << endl;

    // Remove the items which are not present in the schedule
    //out << "Removing not needed items ... " << endl;
    for (int i = 0; i < ordman.items.size(); i++) {
	Item& curItem = ordman.items[i];
	// Check whether the last operation of this item is in progress
	if (curItem.curStepIdx == curItem.operIDs.size() - 1) {

	    //out <<"Removing item : " << curItem << endl;

	    //ordman.itemID2Idx.remove(curItem.ID);
	    //ordman.items.removeAt(i);

	    // IMPORTANT!!!
	    //i = 0;

	    for (int j = 0; j < curItem.operIDs.size(); j++) {
		curItem.operIDs[j] = -1; // To denote the finished operations and operation in progress
	    }

	}
    }

    //out << "PlanSchedServer::constructPartsAndOrders : Items currently in ordman : " << endl;
    // IMPORTANT!!! Set correct mapping of the item IDs to indices
    ordman.itemID2Idx.clear();
    for (int i = 0; i < ordman.items.size(); i++) {
	Item& curItem = ordman.items[i];

	ordman.itemID2Idx[curItem.ID] = i;

	//out << curItem << endl;
    }

    //out << "Done removing not needed items. " << endl;

    // Collect information about the parts/units etc.

    QHash<int, QList<Operation*> > partID2Operations;
    QHash<int, int> partID2RouteID;
    QHash<int, int> partID2PartType;
    QHash<int, QList<Operation*> > unitID2Operations;
    QHash<int, QSet<int> > unitID2PartIDs;
    QHash<int, QSet<int> > unitID2BOMIDs;


    Operation* curOp = NULL;

    QList<ListDigraph::Node> topOrd = pm.topolSort();

    ListDigraph::Node curNode = INVALID;

    //out << "Collecting data... " << endl;

    for (int i = 0; i < topOrd.size(); i++) {

	curNode = topOrd[i];

	if (pm.ops[curNode]->ID <= 0) {
	    continue;
	} else {
	    curOp = pm.ops[curNode];

	    partID2Operations[curOp->itemID()].append(curOp);

	    partID2RouteID[curOp->itemID()] = curOp->routeID();

	    partID2PartType[curOp->itemID()] = curOp->itemType();

	    unitID2Operations[curOp->orderID()].append(curOp);

	    unitID2PartIDs[curOp->orderID()].insert(curOp->itemID());

	    unitID2BOMIDs[curOp->orderID()].insert(curOp->bomID());
	}
    }

    // Some checks
    for (QHash<int, QSet<int> >::iterator iter = unitID2BOMIDs.begin(); iter != unitID2BOMIDs.end(); ++iter) {
	if (iter.value().size() > 1) {
	    throw ErrMsgException<>("PlanSchedServer::constructPartsAndOrders : too many BOMs!");
	} else if (iter.value().size() < 1) {
	    throw ErrMsgException<>("PlanSchedServer::constructPartsAndOrders : no BOMs!");
	}
    }

    //out << "Done collecting data. " << endl;

    // Based on the collected data creates items of the units
    for (QHash<int, QSet<int> >::iterator iter1 = unitID2PartIDs.begin(); iter1 != unitID2PartIDs.end(); iter1++) {
	int curUnitID = iter1.key();

	// Set the right BOMs
	ordman.orderByID(curUnitID).BID = unitID2BOMIDs[curUnitID].toList().first(); // There should be only one!

	// Delete information about the items in the current order
	ordman.orderByID(curUnitID).itemIDs.clear();
	ordman.orderByID(curUnitID).itemIDPrececences.clear();

	// Restore the unit's product type
	if (origOrdID2OrigOrdType.contains(curUnitID)) {
	    ordman.orderByID(curUnitID).type = origOrdID2OrigOrdType[curUnitID];
	}

	for (QSet<int>::iterator iter2 = iter1.value().begin(); iter2 != iter1.value().end(); iter2++) {

	    int curPartID = *iter2;

	    //out << "Current part ID : " << curPartID << endl;

	    Item curItem; // = new Item;

	    curItem.ID = curPartID;
	    curItem.type = partID2PartType[curPartID];
	    curItem.orderID = curUnitID;
	    curItem.routeID = partID2RouteID[curPartID];

	    // Append the operation ID to the item
	    for (int i = 0; i < partID2Operations[curPartID].size(); i++) {
		curItem.operIDs.append(partID2Operations[curPartID][i]->ID);
	    }

	    // Add the part ID to the unit
	    ordman.orderByID(curUnitID).itemIDs.append(curItem.ID);

	    // Add the part to the ordman
	    if (ordman.itemID2Idx.contains(curPartID)) { // In case the item already exists -> just modify it in the ordman

		//out << "Original item : " << ordman.itemByID(curPartID) << endl;

		//out << "Some bug..." << endl;

		//out << "curPartID = " << curPartID << endl;
		//out << ordman.itemByID(curPartID) << endl;

		// The original item's data
		curItem.curStepIdx = ordman.itemByID(curPartID).curStepIdx;
		curItem.curStepFinished = ordman.itemByID(curPartID).curStepFinished;
		curItem.action = ordman.itemByID(curPartID).action;

		// Prepend dummy operation IDs
		for (int i = 0; i < curItem.curStepIdx + 1; i++) {
		    curItem.operIDs.prepend(-1);
		}

		//out << "Replacing " << ordman.itemByID(curPartID) << " with " << curItem << endl;

		// Replace the already existing item in the ordman
		ordman.itemByID(curPartID) = curItem;

		//out << "Adding item : " << curItem << endl;
		//getchar();

		//out << "Done some bug." << endl;

	    } else { // This item has been created and did not exist before -> add it to the ordman

		// Set the index of the current step in the original route
		curItem.curStepIdx = -1;
		curItem.curStepFinished = true;

		// The action that has been performed over the item
		curItem.action = Item::IA_PLAN_SCHED; // The default value

		// Add the new item to the ordman
		ordman << curItem;

		//out << "Pushing item to the order : " << curItem << endl;
	    }


	} // curPartID

    } // curUnitID

}

void PlanSchedServer::computationFinished() {
    QTextStream out(stdout);


    //    /*****************************************  Collect the schedule  *************************************************/
    //    PlanSched ps;
    //    //    Plan plan = solver.getBestPlan();
    //    //    Schedule sched = solver.getBestSchedule();
    //    Plan plan = ps.plan;
    //    Schedule sched = ps.schedule;
    //
    //    // Set the proper product types and BOM IDs for operations of the incomplete products
    //    ProcessModel schedPM = sched.pm;
    //    for (ListDigraph::NodeIt nit(schedPM.graph); nit != INVALID; ++nit) {
    //	if (nit != schedPM.head && nit != schedPM.tail && origOrdID2OrigOrdType.contains(schedPM.ops[nit]->orderID())) { // This operation belongs to a fake incomplete product -> set the correct one
    //	    schedPM.ops[nit]->orderType(origOrdID2OrigOrdType[schedPM.ops[nit]->orderID()]);
    //	    schedPM.ops[nit]->bomID(origOrdID2OrigOrdBOM[schedPM.ops[nit]->orderID()]);
    //	}
    //    }
    //
    //    TWT twt;
    //    sched.fromPM(schedPM, twt);
    //
    //    out << "PlanSchedServer::computationFinished : Best plan is " << endl << plan << endl;
    //
    //    out << "PlanSchedServer::computationFinished : Best schedule is " << endl << sched << endl;
    //
    //    out << "PlanSchedServer::computationFinished : Best PM is " << endl << sched.pm << endl;
    //
    //    // Reconstruct parts and orders based on the best schedule
    //    out << "Constructing parts and orders ... " << endl;
    //    constructPartsAndOrders(sched.pm);
    //    out << "Done constructing parts and orders." << endl;
    //
    //    QXmlStreamWriter composer(&outMessage);
    //
    //    composer.setAutoFormatting(true);
    //
    //    composer.writeStartDocument();
    //
    //    composer.writeStartElement("solution");
    //
    //    // Write the parts
    //    composer.writeStartElement("parts");
    //
    //    for (int i = 0; i < ordman.items.size(); i++) {
    //	Item& curItem = (Item&) ordman.items[i];
    //
    //	composer << curItem;
    //    }
    //
    //    composer.writeEndElement(); // parts
    //
    //    // Write the orders/units
    //    composer.writeStartElement("units");
    //
    //    for (int i = 0; i < ordman.orders.size(); i++) {
    //	composer << ordman.orders[i];
    //    }
    //
    //    composer.writeEndElement(); // units	
    //
    //    // Write the schedule
    //    composer << sched;
    //
    //    composer.writeEndElement(); // solution
    //
    //    composer.writeEndDocument();
    //    /******************************************************************************************************************/
    //
    //
    //    /*****************************************  Reply   ***************************************************************/
    //
    //    qint64 messageSize; // Size of the message in bytes
    //
    //    // Create a reply message
    //    //outMessage = "Hello from PlanSchedServer!";
    //    messageSize = outMessage.size();
    //
    //    // Prepend the message's size
    //    for (uint i = 0; i < sizeof (quint64); i++) {
    //	outMessage.prepend(((uchar*) & messageSize)[i]);
    //    }
    //
    //    // Send the message
    //    socket->write(outMessage);
    //
    //    if (socket->waitForBytesWritten(-1)) {
    //	out << "Bytes are written!" << endl;
    //    } else {
    //	out << "Not all bytes are written!!!" << endl;
    //    }
    //
    //    // Disconnect
    //    socket->disconnectFromHost();
    //    if (socket->state() == QAbstractSocket::UnconnectedState || socket->waitForDisconnected(10000)) {
    //	out << "Socket disconnected!" << endl;
    //    } else {
    //	Debugger::err << "Failed to disconnect socket!" << ENDL;
    //    }
    //
    //
    //
    //    /******************************************************************************************************************/
    //
    //    /*****************************************  Clear the server  *****************************************************/
    //
    //    this->clear();
    //
    //    /******************************************************************************************************************/

}

void PlanSchedServer::parse(const Settings& settings) {
    QTextStream out(stdout);

    out << "PlanSchedServer::parse : Parsing settings..." << endl;

    this->settings = settings;

    // Parse the solver
    if (this->settings.container().contains("PLANSCHEDSERVER_SOLVER")) {

	if (this->settings["PLANSCHEDSERVER_SOLVER"].changed()) {

	    // Parse the string
	    QRegularExpression re("(?<solver>[^\\(]+)\\({1,1}(?<parameters>.*)\\){1,1}@(?<lib>[^@^\\(^\\)]+)");
	    QRegularExpressionMatch match;

	    match = re.match(this->settings["PLANSCHEDSERVER_SOLVER"].get());

	    // Library with the planning algorithm
	    QString solverLibName = match.captured("lib");
	    QString solverName = match.captured("solver");
	    QString solverParams = match.captured("parameters");

	    out << "PlanSchedServer::parse : Solver lib : " << solverLibName << endl;
	    out << "PlanSchedServer::parse : Solver name : " << solverName << endl;
	    out << "PlanSchedServer::parse : Solver params : " << solverParams << endl;

	    QLibrary solverLib(solverLibName);

	    // The search algorithm
	    Common::Util::DLLCallLoader<PlanSchedSolver*, QLibrary&, const char*> plannerLoader;

	    try {

		solver.setPointer(plannerLoader.load(solverLib, QString("new_" + solverName).toStdString().data()), true);

	    } catch (...) {

		out << this->settings["PLANSCHEDSERVER_SOLVER"].get() << endl;
		throw ErrMsgException<>(std::string("PlanSchedServer::parse : Failed to resolve PLANSCHEDSERVER_SOLVER!"));

	    }

	    // Let the solver parse its settings
	    Settings solverSettings;
	    solverSettings["ALL_SETTINGS"] = solverParams;
	    solver->parse(solverSettings);

	}

    } else {
	throw ErrMsgException<>(std::string("PlanSchedServer::parse : PLANSCHEDSERVER_SOLVER not specified!"));
    }

    // Parse the RNG seed
    if (this->settings.container().contains("PLANSCHEDSERVER_RANDSEED")) {

	if (this->settings["PLANSCHEDSERVER_RANDSEED"].changed()) {

	    // Set the seed 
	    Rand::rndSeed(this->settings["PLANSCHEDSERVER_RANDSEED"].get().toInt());
	    out << "PlanSchedServer::parse : PLANSCHEDSERVER_RANDSEED: " << Rand::rndSeed() << endl;

	}

    } else {
	throw ErrMsgException<>(std::string("PlanSchedServer::parse : PLANSCHEDSERVER_RANDSEED not specified!"));
    }

    // Parse the Host and Port seed
    if (this->settings.container().contains("PLANSCHEDSERVER_HOST") && this->settings.container().contains("PLANSCHEDSERVER_PORT")) {

	if (this->settings["PLANSCHEDSERVER_HOST"].changed() || this->settings["PLANSCHEDSERVER_PORT"].changed()) {

	    // Start the server 
	    QHostAddress hostAddress;
	    qint32 hostPort;

	    // Host
	    if (this->settings["PLANSCHEDSERVER_HOST"].get().toLower() == "localhost") {
		hostAddress = QHostAddress::LocalHost;
	    } else {
		hostAddress.setAddress(this->settings["PLANSCHEDSERVER_HOST"].get());
	    }

	    // Port
	    hostPort = (qint32) this->settings["PLANSCHEDSERVER_PORT"].get().toInt();

	    //hostAddress.setAddress("132.176.74.60"); // IP of LABOR 1
	    //host.setAddress("132.176.74.101"); // IP of this computer at the university (static)
	    //host.setAddress("192.168.1.101"); // Local IP at home
	    //hostAddress.setAddress("192.168.1.100"); // Local IP at home

	    out << "PlanSchedServer::parse : Trying to listen at " << hostAddress.toString() << ":" << hostPort << " ..." << endl;

	    if (this->listen(hostAddress, hostPort)) {

		out << "PlanSchedServer::parse : Started " << this->objectName() << endl;

	    } else {

		out << "PlanSchedServer::parse : Failed to start the server!" << endl;
		QCoreApplication::exit();

	    }

	}

    } else {
	throw ErrMsgException<>(std::string("PlanSchedServer::parse : PLANSCHEDSERVER_HOST or PLANSCHEDSERVER_PORT not specified!"));
    }

    this->settings.setChanged(false);

    out << "PlanSchedServer::parse : Done parsing settings." << endl;

}
