/*
 * File:   main.cpp
 * Author: DrSobik
 *
 * Created on May 31, 2011, 3:22 PM
 */

#include <QCoreApplication>

#include "DebugExt.h"
#include "RandExt"
#include "MathExt"

#include "PlannerAgent/PlannerAgent.h"

#ifndef WINDOWS

int Main(int argc, char *argv[]) {
	// initialize resources, if needed
	// Q_INIT_RESOURCE(resfile);

	QCoreApplication app(argc, argv);

	// Start VNS Server
	PlannerAgent planner;
	planner.setObjectName("Planner");

	qDebug() << "Starting " << planner.objectName() << "...";

	// Start listening
	if (planner.schedlistener.listen(QHostAddress::LocalHost, 5555)) {
		qDebug() << "Started " << planner.objectName() << " on " << planner.schedlistener.serverAddress().toString() << ":" << planner.schedlistener.serverPort();
		//qDebug() << "Listening Product Agent.";
		planner.connectTo(QHostAddress::LocalHost, 7777);
	} else {
		qDebug() << "Failed to start " << planner.objectName() << " on " << planner.schedlistener.serverAddress().toString() << ":" << planner.schedlistener.serverPort() << ". " << "Error message : " << planner.schedlistener.errorString();
		return 0;
	}


	qDebug() << planner.objectName() << " successfully started.";

	return app.exec();
}

#endif

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <QTime>

#include "Operation.h"
#include "Product.h"
#include "Order.h"
#include "Route.h"
#include "BillOfMaterials.h"
#include "BillOfProcesses.h"
#include "Planner.h"
#include "VNSPlanner.h"

#include "Schedule.h"
#include "Scheduler.h"
#include "Resources.h"
#include "PriorityScheduler.h"
#include "CombinedScheduler.h"
//#include "BiDirScheduler.h"
//#include "OneDirScheduler.h"
#include "SBHScheduler.h"
#include "LocalSearchPM.h"
#include "VNSScheduler.h"
 
#ifdef GA_ALG

#include "GAPlanner.h"

#endif

using namespace Common;
using namespace Common::Interfaces;
using namespace Common::Exceptions;


/*  ****************  Suite to run the VNS  ********************************  */

int main_VNS(int argc, char *argv[]) {
	// initialize resources, if needed
	// Q_INIT_RESOURCE(resfile);

	QCoreApplication app(argc, argv);

	QTextStream out(stdout);

	QString test_inst_dir;

	// Parse the arguments
	QStringList arguments = app.arguments();

	QHash<QString, QString> prefix2arg;
	for (int i = 1; i < arguments.size(); i++) { // The first one is the program's directory
		out << arguments[i] << endl;
		QStringList argssplitted = arguments[i].split(":");
		if (argssplitted.size() < 2) {
			Debugger::err << "Arguments not specified correctly!!!" << ENDL;
		} else {

			for (int j = 1; j < argssplitted.size() - 1; j++) {
				prefix2arg[argssplitted[0]] += argssplitted[j] + ":";
			}
			prefix2arg[argssplitted[0]] += argssplitted.last();
		}
	}

	if (!prefix2arg.contains("Test")) {
		Debugger::err << "No test specified!" << ENDL;
	} else {
		test_inst_dir = prefix2arg["Test"];
	}

	QFile proto_file;

	if (!prefix2arg.contains("Protocol")) {
		Debugger::err << "No protocol specified" << ENDL;
	} else {
		proto_file.setFileName(test_inst_dir + prefix2arg["Protocol"]);
		out << "Protocol file : " << proto_file.fileName() << endl;
	}

	if (!prefix2arg.contains("GlobMaxIter")) {
		Debugger::err << "No number of global iterations specified!" << ENDL;
	}

	if (!prefix2arg.contains("GlobMaxTimeM")) {
		Debugger::err << "No global time limit specified!" << ENDL;
	}

	if (!prefix2arg.contains("RandSeed")) {
		Debugger::err << "No seed for randomization specified!" << ENDL;
	} else {
		// Set the seed 
		out << "Input for RandSeed: " << prefix2arg["RandSeed"].toInt() << endl;
		Rand::rndSeed(prefix2arg["RandSeed"].toInt());	
		out << "Returned RandSeed: " << Rand::rndSeed() << endl;
	}

	if (!prefix2arg.contains("PlannerInitRule")) {
		Debugger::err << "No initialization rule for the planner specified!" << ENDL;
	}

	if (!prefix2arg.contains("NS")) {
		Debugger::err << "No general NS for the planner specified!" << ENDL;
	}

	if (!prefix2arg.contains("PrioScheduler")) {
		Debugger::err << "No priority scheduler specified!" << ENDL;
	}

	if (!prefix2arg.contains("NumNeigh")) {
		Debugger::err << "No number of neighbors specified!" << ENDL;
	}

	if (!prefix2arg.contains("SchedStrategy")) {
		Debugger::err << "No scheduling strategy specified!" << ENDL;
	}

	// Path to the directory with the input test instance
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Test2Prod/CPP/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/DataGenTest/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/DataGenTest_1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/DataGenTest_2/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/DataGenTest_3/";

	// Chapek test instances
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Chapek/Chapek_1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Chapek/Chapek_2/";

	// Integer test instances
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Test_1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Test_2/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Test_3/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Test_4/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Test_5/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Test_6/";

	// Test basis : Integer tests
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Big/Test_1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Small/Test_192/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Medium/Test_192/";

	// Brandimarte tests
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Brandimarte/WT1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Brandimarte/WT2/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Brandimarte/WT3/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Brandimarte/WT4/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Brandimarte/WT5/";

	// ILOG Tests
	//test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Smaller_ILOG/Test_10/";

	// MISTA tests


	//test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Medium_MISTA_2013/Test_24/";


	// Taillard test instances
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/Taillard/15x15/1/";
	// Standard JS test instances
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/abz5/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/abz6/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la16/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la17/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la18/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la19/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la20/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la21/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la22/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la23/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/la24/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/mt10/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb2/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb3/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb4/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb5/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb6/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb7/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb8/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb9/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/orb10/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.3/TWT/ps3x3/";

	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/abz5/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/abz6/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la16/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la17/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la18/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la19/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la20/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la21/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la22/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la23/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/la24/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/mt10/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb1/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb2/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb3/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb4/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb5/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb6/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb7/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb8/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb9/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/orb10/";
	//QString test_inst_dir = "/home/DrSobik/Projects/IPPS/Tests/ORLib/f_1.5/TWT/ps3x3/";



	/* #######################  Reading resources  ########################## */
	out << "Reading the resources information..." << endl;
	QDomDocument rc_doc("rc");
	QFile rc_file(test_inst_dir + "resources.xml");
	if (!rc_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << rc_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!rc_doc.setContent(&rc_file)) {
		rc_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	rc_file.close();

	QDomElement rcs_elem = rc_doc.elementsByTagName("resources").item(0).toElement();
	QDomNode rc_elem = rcs_elem.elementsByTagName("resource").item(0);

	Resources rc;
	rc.fromDOMElement(rc_elem.toElement());
	out << "Done reading the resources information." << endl;
	out << "Read resources : " << rc << endl;
	/* ####################  Done reading resources  ######################## */

	/* #####################  Read Routes  ################################## */
	QHash<int, QList<Route*> > itype2routes; // <detail_type> -> available routes

	// Read the routes from the XML file
	out << "Reading the routes information..." << endl;
	QDomDocument iroutes_doc("iroutes");
	QFile routes_file(test_inst_dir + "iroutes.xml");
	if (!routes_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << routes_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!iroutes_doc.setContent(&routes_file)) {
		routes_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	routes_file.close();
	out << "Done reading the routes information." << endl;

	/*
	// Get list of item nodes
	QDomNodeList item_list = iroutes_doc.elementsByTagName("itype");
	QDomNodeList route_list;
	int itm_type_id;
	for (int i = 0; i < item_list.size(); i++) {
					itm_type_id = item_list.item(i).toElement().attribute("id").toInt();
					//out << "Reading item type " << itm_type_id << endl;
					route_list = item_list.item(i).toElement().elementsByTagName("route");
					for (int j = 0; j < route_list.size(); j++) {
									iroutes[itm_type_id].append(new Route());
									iroutes[itm_type_id].last()->fromDOMElement(route_list.item(j).toElement());
					}
	}
	 */
	ProductManager::iroutesFromDOMElement(iroutes_doc.documentElement(), itype2routes);

	for (QHash<int, QList<Route*> >::iterator iter = itype2routes.begin(); iter != itype2routes.end(); iter++) {
		out << "Item type : " << iter.key() << endl;
		cout << "Routes : " << endl;
		for (int i = 0; i < iter.value().size(); i++) {
			out << *iter.value()[i] << endl;
		}
	}

	/* ######################  Done reading routes  ######################### */

	/* #######################  Read BOMs  ################################## */
	QHash<int, QList<BillOfMaterials*> > ptype2boms; // <product_type, bom_list>

	// Read the BOMs from the XML file
	out << "Reading the BOMs information..." << endl;
	QDomDocument boms_doc("boms");
	QFile bom_file(test_inst_dir + "boms.xml");
	if (!bom_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << bom_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!boms_doc.setContent(&bom_file)) {
		bom_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	bom_file.close();
	out << "Done reading the BOMs information." << endl;

	/*
	// Get the list of product types in the XML file
	QDomNodeList prod_list = boms_doc.elementsByTagName("ptype");
	//out << "Number of product types read: " << prod_list.size() << endl;
	QDomNodeList prod_bom_list;
	int prod_type_id; // ID of the current product
	// For every node create the corresponding BOMs
	for (int i = 0; i < prod_list.size(); i++) {
					prod_type_id = prod_list.item(i).toElement().attribute("type").toInt();
					//out << "Current product type: " << prod_type_id << endl;

					// Get the list of BOMs for the current product
					prod_bom_list = prod_list.item(i).toElement().elementsByTagName("bom");

					// Iterate over all BOM descriptions and create the actual BOMs
					for (int cb = 0; cb < prod_bom_list.size(); cb++) {
									pboms[prod_type_id].append(new BillOfMaterials());

									// Extract the BOM from the DOM element
									//out << "Parsing pbom from the DOM element ..." << endl;
									//out << prod_bom_list.item(cb).toElement() << endl;
									pboms[prod_type_id].last()->fromDOMElement(prod_bom_list.item(cb).toElement());
									//out << "Done parsing pbom from the DOM element." << endl;
					}
	}
	 */
	//out << "ProductManager::pbomsFromDOMElement..." << endl;
	ProductManager::pbomsFromDOMElement(boms_doc.documentElement(), ptype2boms);
	//out << "Finished ProductManager::pbomsFromDOMElement..." << endl;

	/* ######################  Done reading BOMs  ########################### */


	/* ################  Generate BOPs for each product type  ##############  */
	out << "Generating BOPs for each product type..." << endl;
	// List of possible BOPs for each product type
	QHash<int, QList<BillOfProcesses*> > ptype2bops; // <product_type, possible_BOPs>

	// Prepare the hash with the BOPs for different product types
	for (int i = 0; i < /*prod_list*/ptype2boms.size(); i++) {
		ptype2bops.insert(/*prod_list.item(i).toElement().attribute("type").toInt()*/ptype2boms.keys()[i], QList<BillOfProcesses*>());
	}

	// Iterate over all product types
	double total_comb = 1.0;
	double total_bops = 0.0;
	for (QHash<int, QList<BillOfProcesses*> >::iterator ptiter = ptype2bops.begin(); ptiter != ptype2bops.end(); ptiter++) {
		out << "BillOfProcesses::generateAll..." << endl;
		BillOfProcesses::generateAll(ptype2boms[ptiter.key()], itype2routes, ptype2bops[ptiter.key()]);
		out << "Finished BillOfProcesses::generateAll." << endl;
		out << "Generated " << ptype2bops[ptiter.key()].size() << " BOPs for product " << ptiter.key() << endl;
		total_comb *= double(ptype2bops[ptiter.key()].size());
		total_bops += ptype2bops[ptiter.key()].size();
	}
	out << "Done generating BOPs for each product type." << endl;
	out << "Total generated BOPs : " << total_bops << endl;
	out << "Total combinations for production : " << total_comb << endl;
	//getchar();
	/* ################  Done generating the BOPs  #########################  */

	/* ##############  Create products one for each product type  ############*/
	out << "Creating products..." << endl;
	QList<Product> products;
	QList<int> ptypes = ptype2boms.keys();
	for (int i = 0; i < ptypes.size(); i++) {

		out << "Creating product " << i + 1 << endl;

		products.append(Product());
		products.last().ID = ptypes[i];// i + 1; //1;
		products.last().type = ptypes[i];
		for (int j = 0; j < ptype2bops[products.last().type].size(); j++) {
			// Generate the ID for the current BOM of the current product
			//ptype2bops[products.last().type][j]->ID = products.last().ID;
			//ptype2bops[products.last().type][j]->ID = ptype2bops[products.last().type][j]->ID << 16; // Shift by 16 bits
			//ptype2bops[products.last().type][j]->ID += j;

			// IMPORTANT!!! It is assumed that BOP ID = BOM ID

			// Add the BOP to the product
			products.last() << ptype2bops[products.last().type][j];
		}

		out << "Product before ranking BOPs : " << endl << products.last() << endl;

		//products.last().bopsDecompRate(0.0);
		//products.last().rc = &rc;
		out << "Ranking BOPs... " << endl;
		products.last().rankBOPs(rc);
		out << "Done ranking BOPs." << endl;

		out << "Created product " << i + 1 << endl;

	}
	out << "Done creating products." << endl;
	/* ##############  Done creating products  ############################## */

	out << "Creating product manager ..." << endl;
	ProductManager prodman;
	for (int i = 0; i < products.size(); i++) {
		prodman << &products[i];
	}
	out << "Product manager created." << endl;

	/* #######################  Reading orders  ########################## */
	out << "Reading the orders information..." << endl;
	QDomDocument ord_doc("orders");
	QFile ord_file(test_inst_dir + "orders.xml");
	if (!ord_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << ord_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!ord_doc.setContent(&ord_file)) {
		ord_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	ord_file.close();

	QDomElement ords_elem = ord_doc.elementsByTagName("orders").item(0).toElement();
	QDomNodeList ords_list = ords_elem.elementsByTagName("order");

	QList<Order*> orders;

	for (int i = 0; i < ords_list.size(); i++) {
		orders.append(new Order);
		orders.last()->fromDOMElement(ords_list.item(i).toElement());
	}
	out << "Done reading the orders information." << endl;
	/* ####################  Done reading orders  ######################## */

	out << "Creating order manager ..." << endl;
	OrderManager ordman;
	for (int i = 0; i < orders.size(); i++) {
		ordman << *orders[i];
	}
	out << "Order manager created." << endl;


	// Add the products and the orders to the process model manager
	out << "Creating process model manager ..." << endl;
	ProcessModelManager pmm;
	pmm << &prodman;
	pmm << &ordman;
	out << "Process model manager created ..." << endl;

	out << "Updating the global process model ..." << endl;
	pmm.updatePM();
	//out << pmm.pm << endl;
	out << "Done updating the global process model." << endl;
	out << "Nodes in the updated process model: " << countNodes(pmm.pm.graph) << endl;

	out << "Done reading input data." << endl;

	// Schedule object
	Schedule sched;

	// Scheduler object
	Scheduler *scheduler = new SBHScheduler;

	// Protocol object
	Protocol protocol;
	protocol.init();
	//QFile proto_file(test_inst_dir + "protocol_1.xml");
	protocol.setFile(proto_file);


	// The search algorithm
	//TrivialPlanner planner;
	VNSPlanner plannerVNS;

	if (!prefix2arg.contains("Objective")) {
		Debugger::err << "No objective specified!!!" << ENDL;
	} else if (prefix2arg["Objective"] == "Cmax") {
		plannerVNS.schedOptions["OBJECTIVE"] = "Cmax";
	} else if (prefix2arg["Objective"] == "TWT") {
		plannerVNS.schedOptions["OBJECTIVE"] = "TWT";
	} else {
		out << prefix2arg["Objective"] << endl;
		Debugger::err << "Unknown objective!!!" << ENDL;
	}

	ScalarObjective* objective = NULL;

	if (!plannerVNS.schedOptions.contains("OBJECTIVE")) {
		Debugger::err << "No objective specified!!!" << ENDL;
	} else {
		if (plannerVNS.schedOptions["OBJECTIVE"] == "TWT") {
			objective = new TWT;
		} else if (plannerVNS.schedOptions["OBJECTIVE"] == "Cmax") {
			objective = new Cmax;
		} else {
			Debugger::err << "Invalid objective!!!" << ENDL;
		}
	}

	// Set the objective for the planner
	plannerVNS << objective;

	//getchar();

	if (prefix2arg["PlannerInitRule"] == "RND") {

		plannerVNS.solInitType = VNSPlanner::SOL_INIT_RND;

	} else if (prefix2arg["PlannerInitRule"] == "TPTRANK") {

		plannerVNS.solInitType = VNSPlanner::SOL_INIT_RANK;

	} else {

		Debugger::warn << "PlannerInitRule: " << prefix2arg["PlannerInitRule"].toStdString() << ENDL;
		Debugger::err << "Incorrect planner initialization rule set!" << ENDL;

	}


	if (prefix2arg["NS"] == "N1") {

		plannerVNS.ns = VNSPlanner::NS_N1;

	} else if (prefix2arg["NS"] == "N2") {

		plannerVNS.ns = VNSPlanner::NS_N2;

	} else if (prefix2arg["NS"] == "N3") {

		plannerVNS.ns = VNSPlanner::NS_N3;

	} else if (prefix2arg["NS"] == "N2N1") {

		plannerVNS.ns = VNSPlanner::NS_N2N1;

	} else if (prefix2arg["NS"] == "N2N3") {

		plannerVNS.ns = VNSPlanner::NS_N2N3;

	} else if (prefix2arg["NS"] == "N1N3") {

		plannerVNS.ns = VNSPlanner::NS_N1N3;

	} else if (prefix2arg["NS"] == "N3N1") {

		plannerVNS.ns = VNSPlanner::NS_N3N1;

	} else if (prefix2arg["NS"] == "N2N3N1") {

		plannerVNS.ns = VNSPlanner::NS_N2N3N1;

	} else if (prefix2arg["NS"] == "N2N1N3") {

		plannerVNS.ns = VNSPlanner::NS_N2N1N3;

	} else if (prefix2arg["NS"] == "N2N1PN3") {

		plannerVNS.ns = VNSPlanner::NS_N2N1PN3;

	} else if (prefix2arg["NS"] == "PN2PN1PN3") {

		plannerVNS.ns = VNSPlanner::NS_PN2PN1PN3;

	} else if (prefix2arg["NS"] == "N4") {

		plannerVNS.ns = VNSPlanner::NS_N4;

	} else if (prefix2arg["NS"] == "N5") {

		plannerVNS.ns = VNSPlanner::NS_N5;

	} else {
		Debugger::err << "No correct general NS for the planner specified!" << ENDL;
	}


	if (prefix2arg.contains("StepN1")) {
		plannerVNS.setStepN1(prefix2arg["StepN1"].toInt());
	}
	if (prefix2arg.contains("StepN2")) {
		plannerVNS.setStepN1(prefix2arg["StepN2"].toInt());
	}
	if (prefix2arg.contains("StepN3")) {
		plannerVNS.setStepN1(prefix2arg["StepN3"].toInt());
	}

	if (prefix2arg.contains("NumNeigh")) {
		plannerVNS.setNumNeighbors(prefix2arg["NumNeigh"].toInt());
	}

	if (prefix2arg["PrioScheduler"] == "RND") {

		plannerVNS.scheduler = new RNDScheduler;

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "FIFO") {

		plannerVNS.scheduler = new WFIFOScheduler;
		((WFIFOScheduler*) plannerVNS.scheduler)->weightedFIFO(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WFIFO") {

		plannerVNS.scheduler = new WFIFOScheduler;
		((WFIFOScheduler*) plannerVNS.scheduler)->weightedFIFO(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "W") {

		plannerVNS.scheduler = new WScheduler;

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "SPT") {

		plannerVNS.scheduler = new WSPTScheduler;
		((WSPTScheduler*) plannerVNS.scheduler)->weightedSPT(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WSPT") {

		plannerVNS.scheduler = new WSPTScheduler;
		((WSPTScheduler*) plannerVNS.scheduler)->weightedSPT(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "EDD") {

		plannerVNS.scheduler = new WEDDScheduler;
		((WEDDScheduler*) plannerVNS.scheduler)->weightedEDD(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WEDD") {

		plannerVNS.scheduler = new WEDDScheduler;
		((WEDDScheduler*) plannerVNS.scheduler)->weightedEDD(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "EOD") {

		//planner.scheduler = new EODScheduler;
		plannerVNS.scheduler = new WEODScheduler;
		((WEODScheduler*) plannerVNS.scheduler)->weightedEOD(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WEOD") {

		plannerVNS.scheduler = new WEODScheduler;
		((WEODScheduler*) plannerVNS.scheduler)->weightedEOD(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WEDD2") {

		plannerVNS.scheduler = new WEDD2Scheduler;
		((WEDD2Scheduler*) plannerVNS.scheduler)->weightedEDD(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "MOD") {

		plannerVNS.scheduler = new WMODScheduler;
		((WMODScheduler*) plannerVNS.scheduler)->weightedMOD(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WMOD") {

		plannerVNS.scheduler = new WMODScheduler;
		((WMODScheduler*) plannerVNS.scheduler)->weightedMOD(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "MDD") {

		plannerVNS.scheduler = new WMDDScheduler;
		((WMDDScheduler*) plannerVNS.scheduler)->weightedMDD(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WMDD") {

		plannerVNS.scheduler = new WMDDScheduler;
		((WMDDScheduler*) plannerVNS.scheduler)->weightedMDD(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "ATC") {

		plannerVNS.scheduler = new ATCANScheduler;
		((ATCANScheduler*) plannerVNS.scheduler)->considerSucc(false);
		((ATCANScheduler*) plannerVNS.scheduler)->kappaOptim(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "ATCK") {

		plannerVNS.scheduler = new ATCANScheduler;
		((ATCANScheduler*) plannerVNS.scheduler)->considerSucc(false);
		((ATCANScheduler*) plannerVNS.scheduler)->kappaOptim(true);

		plannerVNS.scheduler->setObjective(*(objective->clone()));

	} else if (prefix2arg["PrioScheduler"] == "ATCKTest") {

		plannerVNS.scheduler = new ATCSchedulerTest;
		((ATCSchedulerTest*) plannerVNS.scheduler)->considerSucc(false);
		((ATCSchedulerTest*) plannerVNS.scheduler)->kappaOptim(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "ATCCS") {

		plannerVNS.scheduler = new ATCANScheduler;
		((ATCANScheduler*) plannerVNS.scheduler)->considerSucc(true);
		((ATCANScheduler*) plannerVNS.scheduler)->kappaOptim(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "ATCCSK") {

		plannerVNS.scheduler = new ATCANScheduler;
		((ATCANScheduler*) plannerVNS.scheduler)->considerSucc(true);
		((ATCANScheduler*) plannerVNS.scheduler)->kappaOptim(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "MUB") {

		plannerVNS.scheduler = new MUBScheduler;

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "MTB") {

		plannerVNS.scheduler = new MTBScheduler;

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "T") {

		plannerVNS.scheduler = new WTScheduler;
		((WTScheduler*) plannerVNS.scheduler)->weightedT(false);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "WT") {

		plannerVNS.scheduler = new WTScheduler;
		((WTScheduler*) plannerVNS.scheduler)->weightedT(true);

		plannerVNS.scheduler->obj = objective->clone();

	} else if (prefix2arg["PrioScheduler"] == "SBHNR") { // SBH with ATC rule for subproblem solution and no reoptimization

		out << "Creating SBHATCNR scheduler ... " << endl;

		plannerVNS.scheduler = new SBHScheduler;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "NONE";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "5000";

		// Set the tool group scheduler for the SBHScheduler
		out << "Creating TG scheduler ... " << endl;

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.maxIter(plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"].toInt());
		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);

		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHATCNR") { // SBH with ATC rule for subproblem solution and no reoptimization

		out << "Creating SBHATCNR scheduler ... " << endl;

		plannerVNS.scheduler = new SBHScheduler;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "NONE";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "0"; // ATC only for solving SSP

		// Set the tool group scheduler for the SBHScheduler
		out << "Creating TG scheduler ... " << endl;

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.maxIter(plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"].toInt());
		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);

		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHLS") { // SBH scheduler with LS for SSP

		out << "Creating SBHLS scheduler ... " << endl;

		plannerVNS.scheduler = new SBHScheduler;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "false";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "5000";

		// Set the tool group scheduler for the SBHScheduler
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;



		/*
		((SBHScheduler*) planner.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxIter(5000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxIterDecl(3000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxTimeMs(30000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.maxIter(plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"].toInt());
		
		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)
		// Set the initial random number generator for the Ls
		//((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.setRandGen(randGen);
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.setRandGens(intRandGen, floatRandGen);

		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.setObjective(objective);
		
		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;
		/*
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */
		//((TGSchedulerLS*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		//((TGSchedulerLS*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;



		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHLS650s") { // SBH scheduler with LS for SSP

		out << "Creating SBHLS650s scheduler ... " << endl;

		plannerVNS.scheduler = new SBHScheduler;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "false";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "800000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "120000";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "10000";

		// Set the tool group scheduler for the SBHScheduler
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;



		/*
		((SBHScheduler*) planner.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxIter(5000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxIterDecl(3000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxTimeMs(30000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.maxIter(plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"].toInt());
		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;
		/*
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */
		//((TGSchedulerLS*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		//((TGSchedulerLS*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;



		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHLSLI") { // SBH scheduler with LS for SSP

		out << "Creating SBHLSLI scheduler ... " << endl;

		plannerVNS.scheduler = new SBHScheduler;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "true"; // Reoptimize only in the last iteration
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "5000";

		// Set the tool group scheduler for the SBHScheduler
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;



		/*
		((SBHScheduler*) planner.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxIter(5000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxIterDecl(3000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->maxTimeMs(30000);
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.maxIter(plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"].toInt());
		
		
		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)
		// Set the initial random number generator for the Ls
		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.setRandGens(intRandGen, floatRandGen);

		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->ls.setObjective(objective);
		
		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;
		/*
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */
		//((TGSchedulerLS*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		//((TGSchedulerLS*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;



		((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGSchedulerLS*) ((SBHScheduler*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHScheduler*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHVNS") { // SBH scheduler with VNS for SSP

		out << "Creating SBHVNS scheduler ... " << endl;

		plannerVNS.scheduler = new SBHSchedulerVNS;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "false";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";

		// Set the tool group scheduler for the SBHSchedulerVNS
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIter(100);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIterDecl(300000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxTimeMs(30 * 1000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;


		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGFIFOScheduler;
		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;

		/*
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */


		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHVNSLI") { // SBH scheduler with VNS for SSP

		out << "Creating SBHVNSLI scheduler ... " << endl;

		plannerVNS.scheduler = new SBHSchedulerVNS;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "true"; // Reoptimize only in the last iteration
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";

		// Set the tool group scheduler for the SBHSchedulerVNS
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIter(100);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIterDecl(300000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxTimeMs(30 * 1000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;


		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGFIFOScheduler;
		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;

		/*
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */


		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHATC") {

		out << "Creating SBHVNS scheduler ... " << endl;

		plannerVNS.scheduler = new SBHSchedulerVNS;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "false";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "0"; // Only ATC initial SSP solution

		// Set the tool group scheduler for the SBHSchedulerVNS
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;




		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIter(00);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIterDecl(300000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxTimeMs(30 * 1000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;

		/*
		((SBHSchedulerVNS*) planner.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGFIFOScheduler;
		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;

		/*
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */


		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "SBHATCAI") { // Reoptimization in all iterations of SBHATC with 200000 iterations in total

		out << "Creating SBHATC_AI scheduler ... " << endl;

		plannerVNS.scheduler = new SBHSchedulerVNS;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "false";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = QString::number(Math::round(200000.0 / double(rc.tools.size() - 1)));

		//out << "Number of iterations per machine group : " << planner.schedOptions["SBH_REOPT_LS_MAX_ITER"] << endl;
		//getchar();

		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";
		plannerVNS.schedOptions["SBH_TG_LS_MAX_ITER"] = "0"; // Only ATC initial SSP solution

		// Set the tool group scheduler for the SBHSchedulerVNS
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;




		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIter(00);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIterDecl(300000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxTimeMs(30 * 1000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;

		/*
		((SBHSchedulerVNS*) planner.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGFIFOScheduler;
		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;

		/*
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */


		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(2.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "VNSLS") {

		out << "Creating SBHVNS scheduler ... " << endl;

		plannerVNS.scheduler = new SBHSchedulerVNS;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "true"; // Reoptimize only in the last iteration
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";

		// Set the tool group scheduler for the SBHSchedulerVNS
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;




		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGVNSScheduler;
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIter(10000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxIterDecl(300000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->maxTimeMs(30 * 1000);
		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;

		/*
		((SBHSchedulerVNS*) planner.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGFIFOScheduler;
		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;

		/*
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */


		((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCScheduler;
		((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "BigVNS") {

		// Merge the resources
		//rc.mergeToolGroups();

		out << "Creating BigVNS scheduler ... " << endl;

		VNSScheduler* vnsScheduler = new VNSScheduler;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		vnsScheduler->getCS() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		vnsScheduler->getCS() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->setObjective(*objective);
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		vnsScheduler->getCS() << sch;

		sch = 0;

		// Set the objective
		vnsScheduler->getCS().setObjective(*objective);
		vnsScheduler->getLS().setObjective(objective);

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)
		// Set the initial random number generator for the Ls
		vnsScheduler->getLS().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = vnsScheduler;
		plannerVNS.scheduler->setObjective(*objective);

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_BEST_POS_TO_MOVE"] = "false";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "2000";
		plannerVNS.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "-1";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

		plannerVNS.schedOptions["VNSG_MAX_ITER"] = "100";
		plannerVNS.schedOptions["VNSG_MAX_ITER_DECL"] = "1000000";
		plannerVNS.schedOptions["VNSG_MAX_TIME_MS"] = "300000";
		plannerVNS.schedOptions["VNSG_TIME_BASED_ACCEPTANCE"] = "false";
		plannerVNS.schedOptions["VNSG_K_MAX"] = "5";
		plannerVNS.schedOptions["VNSG_K_STEP"] = "1";
		plannerVNS.schedOptions["VNSG_SMR"] = "2";

		out << "Done. " << endl;


	} else if (prefix2arg["PrioScheduler"] == "BigVNS110s") {

		// Merge the resources
		//rc.mergeToolGroups();

		out << "Creating BigVNS110s scheduler ... " << endl;

		VNSScheduler* vnsScheduler = new VNSScheduler;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		vnsScheduler->getCS() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		vnsScheduler->getCS() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->setObjective(*objective);
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		vnsScheduler->getCS() << sch;

		sch = 0;

		// Set the objective
		vnsScheduler->getCS().setObjective(*objective);
		vnsScheduler->getLS().setObjective(objective);

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)
		// Set the initial random number generator for the Ls
		vnsScheduler->getLS().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = vnsScheduler;
		plannerVNS.scheduler->setObjective(*objective);

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_BEST_POS_TO_MOVE"] = "false";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "2000";
		plannerVNS.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "-1";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

		plannerVNS.schedOptions["VNSG_MAX_ITER"] = "10000000";
		plannerVNS.schedOptions["VNSG_MAX_ITER_DECL"] = "10000000";
		plannerVNS.schedOptions["VNSG_MAX_TIME_MS"] = "110000";
		plannerVNS.schedOptions["VNSG_TIME_BASED_ACCEPTANCE"] = "true";
		plannerVNS.schedOptions["VNSG_K_MAX"] = "5";
		plannerVNS.schedOptions["VNSG_K_STEP"] = "1";
		plannerVNS.schedOptions["VNSG_SMR"] = "2";

		out << "Done. " << endl;


	} else if (prefix2arg["PrioScheduler"] == "BigVNS400s") {

		// Merge the resources
		//rc.mergeToolGroups();

		out << "Creating BigVNS400s scheduler ... " << endl;

		VNSScheduler* vnsScheduler = new VNSScheduler;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		vnsScheduler->getCS() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		vnsScheduler->getCS() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->setObjective(*objective);
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		vnsScheduler->getCS() << sch;

		sch = 0;

		// Set the objective
		vnsScheduler->getCS().setObjective(*objective);
		vnsScheduler->getLS().setObjective(objective);

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)
		// Set the initial random number generator for the Ls
		vnsScheduler->getLS().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = vnsScheduler;
		plannerVNS.scheduler->setObjective(*objective);

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_BEST_POS_TO_MOVE"] = "false";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "2000";
		plannerVNS.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "-1";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

		plannerVNS.schedOptions["VNSG_MAX_ITER"] = "10000000";
		plannerVNS.schedOptions["VNSG_MAX_ITER_DECL"] = "10000000";
		plannerVNS.schedOptions["VNSG_MAX_TIME_MS"] = "400000";
		plannerVNS.schedOptions["VNSG_TIME_BASED_ACCEPTANCE"] = "true";
		plannerVNS.schedOptions["VNSG_K_MAX"] = "5";
		plannerVNS.schedOptions["VNSG_K_STEP"] = "1";
		plannerVNS.schedOptions["VNSG_SMR"] = "2";

		out << "Done. " << endl;


	} else if (prefix2arg["PrioScheduler"] == "BigVNS650s") {

		// Merge the resources
		//rc.mergeToolGroups();

		out << "Creating BigVNS400s scheduler ... " << endl;

		VNSScheduler* vnsScheduler = new VNSScheduler;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		vnsScheduler->getCS() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		vnsScheduler->getCS() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		vnsScheduler->getCS() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->setObjective(*objective);
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		vnsScheduler->getCS() << sch;

		sch = 0;

		// Set the objective
		vnsScheduler->getCS().setObjective(*objective);
		vnsScheduler->getLS().setObjective(objective);

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)
		// Set the initial random number generator for the Ls
		vnsScheduler->getLS().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = vnsScheduler;
		plannerVNS.scheduler->setObjective(*objective);

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_BEST_POS_TO_MOVE"] = "false";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "2000";
		plannerVNS.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "-1";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

		plannerVNS.schedOptions["VNSG_MAX_ITER"] = "10000000";
		plannerVNS.schedOptions["VNSG_MAX_ITER_DECL"] = "10000000";
		plannerVNS.schedOptions["VNSG_MAX_TIME_MS"] = "650000";
		plannerVNS.schedOptions["VNSG_TIME_BASED_ACCEPTANCE"] = "true";
		plannerVNS.schedOptions["VNSG_K_MAX"] = "5";
		plannerVNS.schedOptions["VNSG_K_STEP"] = "1";
		plannerVNS.schedOptions["VNSG_SMR"] = "2";

		out << "Done. " << endl;


	} else if (prefix2arg["PrioScheduler"] == "VNSLSPinSin") {

		out << "Creating SBHVNSPinSin scheduler ... " << endl;

		plannerVNS.scheduler = new SBHSchedulerPinSin;

		plannerVNS.scheduler->obj = objective->clone();

		plannerVNS.schedOptions["SBH_REOPT_TYPE"] = "LS"; // or "STD" - standard reoptimization, or "NONE" - no reoptimization
		plannerVNS.schedOptions["SBH_REOPT_LAST_ITER"] = "true"; // Reoptimize only in the last iteration
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_ITER"] = "50000";
		plannerVNS.schedOptions["SBH_REOPT_LS_MAX_TIME_MS"] = "60000";

		// Set the tool group scheduler for the SBHSchedulerVNS
		out << "Creating TG scheduler ... " << endl;

		//TGScheduler* curTGScheduer = new TGWEODScheduler;

		/*new TGATCScheduler;
        
		((TGATCScheduler*) curTGScheduer)->setKappa(2.0);
		((TGATCScheduler*) curTGScheduer)->kappaOptim(true);
		 */

		//((SBHScheduler*) planner.scheduler)->tgscheduler = curTGScheduer;




		((SBHScheduler*) plannerVNS.scheduler)->tgscheduler = new TGVNSSchedulerPinSin;
		((TGVNSSchedulerPinSin*) ((SBHSchedulerPinSin*) plannerVNS.scheduler)->tgscheduler)->maxIter(100000);
		((TGVNSSchedulerPinSin*) ((SBHSchedulerPinSin*) plannerVNS.scheduler)->tgscheduler)->maxIterDecl(300000);
		((TGVNSSchedulerPinSin*) ((SBHSchedulerPinSin*) plannerVNS.scheduler)->tgscheduler)->maxTimeMs(30 * 1000);
		((TGVNSSchedulerPinSin*) ((SBHSchedulerPinSin*) plannerVNS.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) plannerVNS.scheduler;

		/*
		((SBHSchedulerVNS*) planner.scheduler)->tgscheduler = new TGSchedulerLS;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */

		out << "Done. " << endl;

		// Set the initial tool group scheduler 
		out << "Creating TG ini scheduler ... " << endl;

		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGFIFOScheduler;
		//((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;

		/*
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler = new TGWEODScheduler;
		((TGSchedulerLS*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler->sbhscheduler = (SBHScheduler*) planner.scheduler;
		 */


		((TGVNSSchedulerPinSin*) ((SBHSchedulerPinSin*) plannerVNS.scheduler)->tgscheduler)->iniScheduler = new TGATCSchedulerPinSin;
		((TGATCSchedulerPinSin*) ((TGVNSSchedulerPinSin*) ((SBHSchedulerPinSin*) plannerVNS.scheduler)->tgscheduler)->iniScheduler)->kappaOptim(true);
		//((TGATCScheduler*) ((TGVNSScheduler*) ((SBHSchedulerVNS*) planner.scheduler)->tgscheduler)->iniScheduler)->setKappa(1.0);
		out << "Done. " << endl;


		out << "Done. " << endl;

	} else if (prefix2arg["PrioScheduler"] == "CS") {

		plannerVNS.scheduler = new CombinedScheduler;

		plannerVNS.scheduler->obj = objective->clone();

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = new WMDDScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		*((CombinedScheduler*) plannerVNS.scheduler) << sch;

		sch = NULL;

		((CombinedScheduler*) plannerVNS.scheduler)->setObjective(*objective);
		
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLS20k") {

		CombinedSchedulerLS* csls = new CombinedSchedulerLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)

		out << "Random Seed: " << intRandGen->getSeed() << endl;
		//getchar();
		
		csls->combinedSchedulerObject().setObjective(*objective);
		csls->localSearchObject().setObjective(objective);

		// Set the initial random number generator for the Ls
		csls->localSearchObject().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->setObjective(*objective);

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_BEST_POS_TO_MOVE"] = "false";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "20000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLS2k") {

		CombinedSchedulerLS* csls = new CombinedSchedulerLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->setObjective(*objective);
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)

		csls->combinedSchedulerObject().obj = objective->clone();
		csls->localSearchObject().setObjective(objective);

		// Set the initial random number generator for the Ls
		csls->localSearchObject().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "20";
		plannerVNS.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "100000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

	} else if (prefix2arg["PrioScheduler"] == "VNS1ICSLS20k") { // Only one iteration of VNS => randomly selected process plan

		CombinedSchedulerLS* csls = new CombinedSchedulerLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		csls->combinedSchedulerObject().obj = objective->clone();
		csls->localSearchObject().setObjective(objective);

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "20";
		plannerVNS.schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "20000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6";

		// Perform only one iteration
		plannerVNS.maxIter(0);

	} else if (prefix2arg["PrioScheduler"] == "CSLS400s") {

		CombinedSchedulerLS* csls = new CombinedSchedulerLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "200000000";
		plannerVNS.schedOptions["LS_MAX_TIME_MS"] = "400000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLS650s") {

		CombinedSchedulerLS* csls = new CombinedSchedulerLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "200000000";
		plannerVNS.schedOptions["LS_MAX_TIME_MS"] = "650000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLS200k") { // 200000 iterations of the LS

		CombinedSchedulerLS* csls = new CombinedSchedulerLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		//RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator
		Common::Rand::MT19937<Math::uint32>* intRandGen = new Common::Rand::MT19937<Math::uint32>(Rand::rndSeed()); // The random numbers generator (integers)
		Common::Rand::MT19937<double>* floatRandGen = new Common::Rand::MT19937<double>(Rand::rndSeed()); // The random numbers generator (floats)

		csls->combinedSchedulerObject().setObjective(*objective);
		csls->localSearchObject().setObjective(objective);

		// Set the initial random number generator for the Ls
		csls->localSearchObject().setRandGens(intRandGen, floatRandGen);

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->setObjective(*objective);

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_BEST_POS_TO_MOVE"] = "false";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "200000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLSMod300k") { // 200000 iterations of the LS

		CombinedSchedulerModLS* csls = new CombinedSchedulerModLS;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "300000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLSCP200k") { // 200000 iterations of the LS

		CombinedSchedulerLSCP* csls = new CombinedSchedulerLSCP;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "200000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else if (prefix2arg["PrioScheduler"] == "CSLSCP20k") { // 200000 iterations of the LS

		CombinedSchedulerLSCP* csls = new CombinedSchedulerLSCP;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		plannerVNS.scheduler = csls;
		plannerVNS.scheduler->obj = objective->clone();

		// Options
		plannerVNS.schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
		plannerVNS.schedOptions["LS_CHK_COR"] = "false";
		plannerVNS.schedOptions["LS_MAX_ITER"] = "20000";
		plannerVNS.schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7";

	} else {

		Debugger::err << "No feasible priority scheduling rule specified!" << ENDL;

	}

	plannerVNS.maxIter(prefix2arg["GlobMaxIter"].toInt());
	plannerVNS.maxTimeMs(prefix2arg["GlobMaxTimeM"].toInt()* 60 * 1000);
	//planner.maxIterDecl(6);
	//planner.maxTimeMs(1 * 30 * 60 * 1000);

	VNSPlannerStrategy schedStrategy;
	schedStrategy.setStrategy(prefix2arg["SchedStrategy"]);

	//out << "Pushing into planner ... " << endl;
	plannerVNS << &pmm << &sched << &rc << scheduler << schedStrategy << &protocol;

	out << "Running the search algorithm ..." << endl;
	//out << "PM before the iterations:" << endl;
	//out << pmm.pm << endl;

	//planner.init();

	QTime time;

	//out << p1 << endl;
	//getchar();

	/*
	out << "List of BOPs in p1:" << endl;
	for (int i = 0; i < p1.bops.size(); i++) {
					out << p1.bops[i]->ID << endl;
	}
	out << "List of keys in p1:" << endl;
	for (int i = 0; i < p1.bopid2idx.size(); i++) {
					out << p1.bopid2idx.keys()[i] << endl;
	}
	getchar();
	 */
	//getchar();

	QObject::connect(&plannerVNS, SIGNAL(sigCompletelyFinished()), &app, SLOT(quit()));

	//getchar();

	out << "Running test in " << test_inst_dir << endl;

	plannerVNS.run();

	return 0; //*/ app.exec();
}


/******************************************************************************/


#ifdef GA_ALG

/*  ****************  Suite to run the GA  *********************************  */

int main_GA(int argc, char *argv[]) {
	// initialize resources, if needed
	// Q_INIT_RESOURCE(resfile);

	QCoreApplication app(argc, argv);

	QTextStream out(stdout);

	QString test_inst_dir;

	// Parse the arguments
	QStringList arguments = app.arguments();

	QHash<QString, QString> prefix2arg;
	for (int i = 1; i < arguments.size(); i++) { // The first one is the program's directory
		out << arguments[i] << endl;
		QStringList argssplitted = arguments[i].split(":");
		if (argssplitted.size() < 2) {
			Debugger::err << "Arguments not specified correctly!!!" << ENDL;
		} else {
			prefix2arg[argssplitted[0]] = argssplitted[1];
		}
	}

	if (!prefix2arg.contains("Test")) {
		Debugger::err << "No test specified!" << ENDL;
	} else {
		test_inst_dir = prefix2arg["Test"];
	}

	QFile proto_file;

	if (!prefix2arg.contains("Protocol")) {
		Debugger::err << "No protocol specified" << ENDL;
	} else {
		proto_file.setFileName(test_inst_dir + prefix2arg["Protocol"]);
		out << "Protocol file : " << proto_file.fileName() << endl;
	}

	if (!prefix2arg.contains("GlobMaxIter")) {
		Debugger::err << "No number of global iterations specified!" << ENDL;
	}

	if (!prefix2arg.contains("GlobMaxTimeM")) {
		Debugger::err << "No global time limit specified!" << ENDL;
	}

	if (!prefix2arg.contains("RandSeed")) {
		Debugger::err << "No seed for randomization specified!" << ENDL;
	} else {
		// Set the seed 
		Rand::rndSeed(prefix2arg["RandSeed"].toInt());
	}

	/* #######################  Reading resources  ########################## */
	out << "Reading the resources information..." << endl;
	QDomDocument rc_doc("rc");
	QFile rc_file(test_inst_dir + "resources.xml");
	if (!rc_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << rc_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!rc_doc.setContent(&rc_file)) {
		rc_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	rc_file.close();

	QDomElement rcs_elem = rc_doc.elementsByTagName("resources").item(0).toElement();
	QDomNode rc_elem = rcs_elem.elementsByTagName("resource").item(0);

	Resources rc;
	rc.fromDOMElement(rc_elem.toElement());
	out << "Done reading the resources information." << endl;
	out << "Read resources : " << rc << endl;
	/* ####################  Done reading resources  ######################## */

	/* #####################  Read Routes  ################################## */
	QHash<int, QList<Route*> > itype2routes; // <detail_type> -> available routes

	// Read the routes from the XML file
	out << "Reading the routes information..." << endl;
	QDomDocument iroutes_doc("iroutes");
	QFile routes_file(test_inst_dir + "iroutes.xml");
	if (!routes_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << routes_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!iroutes_doc.setContent(&routes_file)) {
		routes_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	routes_file.close();
	out << "Done reading the routes information." << endl;

	/*
	// Get list of item nodes
	QDomNodeList item_list = iroutes_doc.elementsByTagName("itype");
	QDomNodeList route_list;
	int itm_type_id;
	for (int i = 0; i < item_list.size(); i++) {
					itm_type_id = item_list.item(i).toElement().attribute("id").toInt();
					//out << "Reading item type " << itm_type_id << endl;
					route_list = item_list.item(i).toElement().elementsByTagName("route");
					for (int j = 0; j < route_list.size(); j++) {
									iroutes[itm_type_id].append(new Route());
									iroutes[itm_type_id].last()->fromDOMElement(route_list.item(j).toElement());
					}
	}
	 */
	ProductManager::iroutesFromDOMElement(iroutes_doc.documentElement(), itype2routes);

	for (QHash<int, QList<Route*> >::iterator iter = itype2routes.begin(); iter != itype2routes.end(); iter++) {
		out << "Item type : " << iter.key() << endl;
		cout << "Routes : " << endl;
		for (int i = 0; i < iter.value().size(); i++) {
			out << *iter.value()[i] << endl;
		}
	}

	/* ######################  Done reading routes  ######################### */

	/* #######################  Read BOMs  ################################## */
	QHash<int, QList<BillOfMaterials*> > ptype2boms; // <product_type, bom_list>

	// Read the BOMs from the XML file
	out << "Reading the BOMs information..." << endl;
	QDomDocument boms_doc("boms");
	QFile bom_file(test_inst_dir + "boms.xml");
	if (!bom_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << bom_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!boms_doc.setContent(&bom_file)) {
		bom_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	bom_file.close();
	out << "Done reading the BOMs information." << endl;

	/*
	// Get the list of product types in the XML file
	QDomNodeList prod_list = boms_doc.elementsByTagName("ptype");
	//out << "Number of product types read: " << prod_list.size() << endl;
	QDomNodeList prod_bom_list;
	int prod_type_id; // ID of the current product
	// For every node create the corresponding BOMs
	for (int i = 0; i < prod_list.size(); i++) {
					prod_type_id = prod_list.item(i).toElement().attribute("type").toInt();
					//out << "Current product type: " << prod_type_id << endl;

					// Get the list of BOMs for the current product
					prod_bom_list = prod_list.item(i).toElement().elementsByTagName("bom");

					// Iterate over all BOM descriptions and create the actual BOMs
					for (int cb = 0; cb < prod_bom_list.size(); cb++) {
									pboms[prod_type_id].append(new BillOfMaterials());

									// Extract the BOM from the DOM element
									//out << "Parsing pbom from the DOM element ..." << endl;
									//out << prod_bom_list.item(cb).toElement() << endl;
									pboms[prod_type_id].last()->fromDOMElement(prod_bom_list.item(cb).toElement());
									//out << "Done parsing pbom from the DOM element." << endl;
					}
	}
	 */
	//out << "ProductManager::pbomsFromDOMElement..." << endl;
	ProductManager::pbomsFromDOMElement(boms_doc.documentElement(), ptype2boms);
	//out << "Finished ProductManager::pbomsFromDOMElement..." << endl;

	/* ######################  Done reading BOMs  ########################### */


	/* ################  Generate BOPs for each product type  ##############  */
	out << "Generating BOPs for each product type..." << endl;
	// List of possible BOPs for each product type
	QHash<int, QList<BillOfProcesses*> > ptype2bops; // <product_type, possible_BOPs>

	// Prepare the hash with the BOPs for different product types
	for (int i = 0; i < /*prod_list*/ptype2boms.size(); i++) {
		ptype2bops.insert(/*prod_list.item(i).toElement().attribute("type").toInt()*/ptype2boms.keys()[i], QList<BillOfProcesses*>());
	}

	// Iterate over all product types
	double total_comb = 1.0;
	double total_bops = 0.0;
	for (QHash<int, QList<BillOfProcesses*> >::iterator ptiter = ptype2bops.begin(); ptiter != ptype2bops.end(); ptiter++) {
		out << "BillOfProcesses::generateAll..." << endl;
		BillOfProcesses::generateAll(ptype2boms[ptiter.key()], itype2routes, ptype2bops[ptiter.key()]);
		out << "Finished BillOfProcesses::generateAll." << endl;
		out << "Generated " << ptype2bops[ptiter.key()].size() << " BOPs for product " << ptiter.key() << endl;
		total_comb *= double(ptype2bops[ptiter.key()].size());
		total_bops += ptype2bops[ptiter.key()].size();
	}
	out << "Done generating BOPs for each product type." << endl;
	out << "Total generated BOPs : " << total_bops << endl;
	out << "Total combinations for production : " << total_comb << endl;
	//getchar();
	/* ################  Done generating the BOPs  #########################  */

	/* ##############  Create products one for each product type  ############*/
	out << "Creating products..." << endl;
	QList<Product> products;
	QList<int> ptypes = ptype2boms.keys();
	for (int i = 0; i < ptypes.size(); i++) {

		out << "Creating product " << i + 1 << endl;

		products.append(Product());
		products.last().ID = i + 1; //1;
		products.last().type = ptypes[i];
		for (int j = 0; j < ptype2bops[products.last().type].size(); j++) {
			// Generate the ID for the current BOM of the current product
			//ptype2bops[products.last().type][j]->ID = products.last().ID;
			//ptype2bops[products.last().type][j]->ID = ptype2bops[products.last().type][j]->ID << 16; // Shift by 16 bits
			//ptype2bops[products.last().type][j]->ID += j;

			// IMPORTANT!!! It is assumed that BOP ID = BOM ID

			// Add the BOP to the product
			products.last() << ptype2bops[products.last().type][j];
		}

		out << "Product before ranking BOPs : " << endl << products.last() << endl;

		//products.last().bopsDecompRate(0.0);
		//products.last().rc = &rc;
		out << "Ranking BOPs... " << endl;
		products.last().rankBOPs(rc);
		out << "Done ranking BOPs." << endl;

		out << "Created product " << i + 1 << endl;

	}
	out << "Done creating products." << endl;
	/* ##############  Done creating products  ############################## */

	out << "Creating product manager ..." << endl;
	ProductManager prodman;
	for (int i = 0; i < products.size(); i++) {
		prodman << &products[i];
	}
	out << "Product manager created." << endl;

	/* #######################  Reading orders  ########################## */
	out << "Reading the orders information..." << endl;
	QDomDocument ord_doc("orders");
	QFile ord_file(test_inst_dir + "orders.xml");
	if (!ord_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << ord_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!ord_doc.setContent(&ord_file)) {
		ord_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	ord_file.close();

	QDomElement ords_elem = ord_doc.elementsByTagName("orders").item(0).toElement();
	QDomNodeList ords_list = ords_elem.elementsByTagName("order");

	QList<Order*> orders;

	for (int i = 0; i < ords_list.size(); i++) {
		orders.append(new Order);
		orders.last()->fromDOMElement(ords_list.item(i).toElement());
	}
	out << "Done reading the orders information." << endl;
	/* ####################  Done reading orders  ######################## */

	out << "Creating order manager ..." << endl;
	OrderManager ordman;
	for (int i = 0; i < orders.size(); i++) {
		ordman << *orders[i];
	}
	out << "Order manager created." << endl;


	// Add the products and the orders to the process model manager
	out << "Creating process model manager ..." << endl;
	ProcessModelManager pmm;
	pmm << &prodman;
	pmm << &ordman;
	out << "Process model manager created ..." << endl;

	out << "Updating the global process model ..." << endl;
	pmm.updatePM();
	//out << pmm.pm << endl;
	out << "Done updating the global process model." << endl;
	out << "Nodes in the updated process model: " << countNodes(pmm.pm.graph) << endl;

	out << "Done reading input data." << endl;

	// Options
	SchedulerOptions schedOptions;

	if (!prefix2arg.contains("Objective")) {
		Debugger::err << "No objective specified!!!" << ENDL;
	} else if (prefix2arg["Objective"] == "Cmax") {
		schedOptions["OBJECTIVE"] = "Cmax";
	} else if (prefix2arg["Objective"] == "TWT") {
		schedOptions["OBJECTIVE"] = "TWT";
	} else {
		out << prefix2arg["Objective"] << endl;
		Debugger::err << "Unknown objective!!!" << ENDL;
	}


	schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
	schedOptions["LS_CHK_COR"] = "false";
	schedOptions["LS_MAX_ITER"] = "500";
	schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "20000";
	schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7"; // Allow ATC during the initialization (it will not be used later!!!)
	if (!prefix2arg.contains("GASolImpr")) {
		Debugger::err << "GASolImpr not specified!!!" << ENDL;
	} else {
		schedOptions["GA_IMPROVE_SOLUTION"] = prefix2arg["GASolImpr"];
	}
	ScalarObjective* objective = NULL;
	if (!schedOptions.contains("OBJECTIVE")) {
		Debugger::err << "No objective specified!!!" << ENDL;
	} else {
		if (schedOptions["OBJECTIVE"] == "TWT") {
			objective = new TWT;
		} else if (schedOptions["OBJECTIVE"] == "Cmax") {
			objective = new Cmax;
		} else {
			Debugger::err << "Invalid objective!!!" << ENDL;
		}
	}

	// Scheduler object

	Scheduler* scheduler = NULL;

	if (prefix2arg["PrioScheduler"] == "CSLS") {

		scheduler = new CombinedSchedulerLS;

		CombinedSchedulerLS* csls = (CombinedSchedulerLS*) scheduler;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		sch->obj = objective->clone();
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

	} else if (prefix2arg["PrioScheduler"] == "RND") {

		scheduler = new RNDScheduler;

		//getchar();

	}


	// Protocol object
	Protocol protocol;
	protocol.init();
	//QFile proto_file(test_inst_dir + "protocol_1.xml");
	protocol.setFile(proto_file);

	scheduler->obj = objective->clone();

	// Schedule object
	Schedule sched;

	out << "Creating the GA planner" << endl;

	// The search algorithm
	GAPlanner plannerGA;

	//plannerGA.maxIter(prefix2arg["GlobMaxIter"].toInt());
	//plannerGA.maxTimeMs(prefix2arg["GlobMaxTimeM"].toInt()* 60 * 1000);
	//planner.maxIterDecl(6);
	//planner.maxTimeMs(1 * 30 * 60 * 1000);

	plannerGA << &pmm << &sched << &rc << scheduler << &protocol << objective;

	plannerGA.schedOptions = schedOptions;

	out << "Running the search algorithm ..." << endl;
	//out << "PM before the iterations:" << endl;
	//out << pmm.pm << endl;

	//planner.init();

	QTime time;

	//out << p1 << endl;
	//getchar();

	/*
	out << "List of BOPs in p1:" << endl;
	for (int i = 0; i < p1.bops.size(); i++) {
					out << p1.bops[i]->ID << endl;
	}
	out << "List of keys in p1:" << endl;
	for (int i = 0; i < p1.bopid2idx.size(); i++) {
					out << p1.bopid2idx.keys()[i] << endl;
	}
	getchar();
	 */
	//getchar();

	QObject::connect(&plannerGA, SIGNAL(sigFinished()), &app, SLOT(quit()));

	//getchar();

	out << "Running test in " << test_inst_dir << endl;

	plannerGA.n_genomes = 50; // 200 - Park & Choi
	plannerGA.seed = prefix2arg["RandSeed"].toInt();
	plannerGA.n_cycles = 200; // 1000 - Park & Choi
	plannerGA.n_secs = 12000;
	plannerGA.p_cross = 0.8;
	plannerGA.p_mut = 0.2;
	plannerGA.p_repl = 0.6;


	plannerGA.run();

	return 0; // app.exec();
}

int main_GA1(int argc, char *argv[]) {
	// initialize resources, if needed
	// Q_INIT_RESOURCE(resfile);

	QCoreApplication app(argc, argv);

	QTextStream out(stdout);

	QString test_inst_dir;

	// Parse the arguments
	QStringList arguments = app.arguments();

	QHash<QString, QString> prefix2arg;
	for (int i = 1; i < arguments.size(); i++) { // The first one is the program's directory
		out << arguments[i] << endl;
		QStringList argssplitted = arguments[i].split(":");
		if (argssplitted.size() < 2) {
			Debugger::err << "Arguments not specified correctly!!!" << ENDL;
		} else {
			prefix2arg[argssplitted[0]] = argssplitted[1];
		}
	}

	if (!prefix2arg.contains("Test")) {
		Debugger::err << "No test specified!" << ENDL;
	} else {
		test_inst_dir = prefix2arg["Test"];
	}

	QFile proto_file;

	if (!prefix2arg.contains("Protocol")) {
		Debugger::err << "No protocol specified" << ENDL;
	} else {
		proto_file.setFileName(test_inst_dir + prefix2arg["Protocol"]);
		out << "Protocol file : " << proto_file.fileName() << endl;
	}

	if (!prefix2arg.contains("GlobMaxIter")) {
		Debugger::err << "No number of global iterations specified!" << ENDL;
	}

	if (!prefix2arg.contains("GlobMaxTimeM")) {
		Debugger::err << "No global time limit specified!" << ENDL;
	}

	if (!prefix2arg.contains("RandSeed")) {
		Debugger::err << "No seed for randomization specified!" << ENDL;
	} else {
		// Set the seed 
		Rand::rndSeed(prefix2arg["RandSeed"].toInt());
	}

	/* #######################  Reading resources  ########################## */
	out << "Reading the resources information..." << endl;
	QDomDocument rc_doc("rc");
	QFile rc_file(test_inst_dir + "resources.xml");
	if (!rc_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << rc_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!rc_doc.setContent(&rc_file)) {
		rc_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	rc_file.close();

	QDomElement rcs_elem = rc_doc.elementsByTagName("resources").item(0).toElement();
	QDomNode rc_elem = rcs_elem.elementsByTagName("resource").item(0);

	Resources rc;
	rc.fromDOMElement(rc_elem.toElement());
	out << "Done reading the resources information." << endl;
	out << "Read resources : " << rc << endl;
	/* ####################  Done reading resources  ######################## */

	/* #####################  Read Routes  ################################## */
	QHash<int, QList<Route*> > itype2routes; // <detail_type> -> available routes

	// Read the routes from the XML file
	out << "Reading the routes information..." << endl;
	QDomDocument iroutes_doc("iroutes");
	QFile routes_file(test_inst_dir + "iroutes.xml");
	if (!routes_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << routes_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!iroutes_doc.setContent(&routes_file)) {
		routes_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	routes_file.close();
	out << "Done reading the routes information." << endl;

	/*
	// Get list of item nodes
	QDomNodeList item_list = iroutes_doc.elementsByTagName("itype");
	QDomNodeList route_list;
	int itm_type_id;
	for (int i = 0; i < item_list.size(); i++) {
					itm_type_id = item_list.item(i).toElement().attribute("id").toInt();
					//out << "Reading item type " << itm_type_id << endl;
					route_list = item_list.item(i).toElement().elementsByTagName("route");
					for (int j = 0; j < route_list.size(); j++) {
									iroutes[itm_type_id].append(new Route());
									iroutes[itm_type_id].last()->fromDOMElement(route_list.item(j).toElement());
					}
	}
	 */
	ProductManager::iroutesFromDOMElement(iroutes_doc.documentElement(), itype2routes);

	for (QHash<int, QList<Route*> >::iterator iter = itype2routes.begin(); iter != itype2routes.end(); iter++) {
		out << "Item type : " << iter.key() << endl;
		cout << "Routes : " << endl;
		for (int i = 0; i < iter.value().size(); i++) {
			out << *iter.value()[i] << endl;
		}
	}

	/* ######################  Done reading routes  ######################### */

	/* #######################  Read BOMs  ################################## */
	QHash<int, QList<BillOfMaterials*> > ptype2boms; // <product_type, bom_list>

	// Read the BOMs from the XML file
	out << "Reading the BOMs information..." << endl;
	QDomDocument boms_doc("boms");
	QFile bom_file(test_inst_dir + "boms.xml");
	if (!bom_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << bom_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!boms_doc.setContent(&bom_file)) {
		bom_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	bom_file.close();
	out << "Done reading the BOMs information." << endl;

	/*
	// Get the list of product types in the XML file
	QDomNodeList prod_list = boms_doc.elementsByTagName("ptype");
	//out << "Number of product types read: " << prod_list.size() << endl;
	QDomNodeList prod_bom_list;
	int prod_type_id; // ID of the current product
	// For every node create the corresponding BOMs
	for (int i = 0; i < prod_list.size(); i++) {
					prod_type_id = prod_list.item(i).toElement().attribute("type").toInt();
					//out << "Current product type: " << prod_type_id << endl;

					// Get the list of BOMs for the current product
					prod_bom_list = prod_list.item(i).toElement().elementsByTagName("bom");

					// Iterate over all BOM descriptions and create the actual BOMs
					for (int cb = 0; cb < prod_bom_list.size(); cb++) {
									pboms[prod_type_id].append(new BillOfMaterials());

									// Extract the BOM from the DOM element
									//out << "Parsing pbom from the DOM element ..." << endl;
									//out << prod_bom_list.item(cb).toElement() << endl;
									pboms[prod_type_id].last()->fromDOMElement(prod_bom_list.item(cb).toElement());
									//out << "Done parsing pbom from the DOM element." << endl;
					}
	}
	 */
	//out << "ProductManager::pbomsFromDOMElement..." << endl;
	ProductManager::pbomsFromDOMElement(boms_doc.documentElement(), ptype2boms);
	//out << "Finished ProductManager::pbomsFromDOMElement..." << endl;

	/* ######################  Done reading BOMs  ########################### */


	/* ################  Generate BOPs for each product type  ##############  */
	out << "Generating BOPs for each product type..." << endl;
	// List of possible BOPs for each product type
	QHash<int, QList<BillOfProcesses*> > ptype2bops; // <product_type, possible_BOPs>

	// Prepare the hash with the BOPs for different product types
	for (int i = 0; i < /*prod_list*/ptype2boms.size(); i++) {
		ptype2bops.insert(/*prod_list.item(i).toElement().attribute("type").toInt()*/ptype2boms.keys()[i], QList<BillOfProcesses*>());
	}

	// Iterate over all product types
	double total_comb = 1.0;
	double total_bops = 0.0;
	for (QHash<int, QList<BillOfProcesses*> >::iterator ptiter = ptype2bops.begin(); ptiter != ptype2bops.end(); ptiter++) {
		out << "BillOfProcesses::generateAll..." << endl;
		BillOfProcesses::generateAll(ptype2boms[ptiter.key()], itype2routes, ptype2bops[ptiter.key()]);
		out << "Finished BillOfProcesses::generateAll." << endl;
		out << "Generated " << ptype2bops[ptiter.key()].size() << " BOPs for product " << ptiter.key() << endl;
		total_comb *= double(ptype2bops[ptiter.key()].size());
		total_bops += ptype2bops[ptiter.key()].size();
	}
	out << "Done generating BOPs for each product type." << endl;
	out << "Total generated BOPs : " << total_bops << endl;
	out << "Total combinations for production : " << total_comb << endl;
	//getchar();
	/* ################  Done generating the BOPs  #########################  */

	/* ##############  Create products one for each product type  ############*/
	out << "Creating products..." << endl;
	QList<Product> products;
	QList<int> ptypes = ptype2boms.keys();
	for (int i = 0; i < ptypes.size(); i++) {

		out << "Creating product " << i + 1 << endl;

		products.append(Product());
		products.last().ID = i + 1; //1;
		products.last().type = ptypes[i];
		for (int j = 0; j < ptype2bops[products.last().type].size(); j++) {
			// Generate the ID for the current BOM of the current product
			//ptype2bops[products.last().type][j]->ID = products.last().ID;
			//ptype2bops[products.last().type][j]->ID = ptype2bops[products.last().type][j]->ID << 16; // Shift by 16 bits
			//ptype2bops[products.last().type][j]->ID += j;

			// IMPORTANT!!! It is assumed that BOP ID = BOM ID

			// Add the BOP to the product
			products.last() << ptype2bops[products.last().type][j];
		}

		out << "Product before ranking BOPs : " << endl << products.last() << endl;

		//products.last().bopsDecompRate(0.0);
		//products.last().rc = &rc;
		out << "Ranking BOPs... " << endl;
		products.last().rankBOPs(rc);
		out << "Done ranking BOPs." << endl;

		out << "Created product " << i + 1 << endl;

	}
	out << "Done creating products." << endl;
	/* ##############  Done creating products  ############################## */

	out << "Creating product manager ..." << endl;
	ProductManager prodman;
	for (int i = 0; i < products.size(); i++) {
		prodman << &products[i];
	}
	out << "Product manager created." << endl;

	/* #######################  Reading orders  ########################## */
	out << "Reading the orders information..." << endl;
	QDomDocument ord_doc("orders");
	QFile ord_file(test_inst_dir + "orders.xml");
	if (!ord_file.open(QIODevice::ReadOnly)) {
		Debugger::err << "Failed to open file " << ord_file.fileName().toStdString() << " for reading!" << ENDL;
		return -1;
	}

	if (!ord_doc.setContent(&ord_file)) {
		ord_file.close();
		Debugger::err << "Failed to set document contents!" << ENDL;
		return -1;
	}
	ord_file.close();

	QDomElement ords_elem = ord_doc.elementsByTagName("orders").item(0).toElement();
	QDomNodeList ords_list = ords_elem.elementsByTagName("order");

	QList<Order*> orders;

	for (int i = 0; i < ords_list.size(); i++) {
		orders.append(new Order);
		orders.last()->fromDOMElement(ords_list.item(i).toElement());
	}
	out << "Done reading the orders information." << endl;
	/* ####################  Done reading orders  ######################## */

	out << "Creating order manager ..." << endl;
	OrderManager ordman;
	for (int i = 0; i < orders.size(); i++) {
		ordman << *orders[i];
	}
	out << "Order manager created." << endl;


	// Add the products and the orders to the process model manager
	out << "Creating process model manager ..." << endl;
	ProcessModelManager pmm;
	pmm << &prodman;
	pmm << &ordman;
	out << "Process model manager created ..." << endl;

	out << "Updating the global process model ..." << endl;
	pmm.updatePM();
	//out << pmm.pm << endl;
	out << "Done updating the global process model." << endl;
	out << "Nodes in the updated process model: " << countNodes(pmm.pm.graph) << endl;

	out << "Done reading input data." << endl;

	// Options
	SchedulerOptions schedOptions;

	if (!prefix2arg.contains("Objective")) {
		Debugger::err << "No objective specified!!!" << ENDL;
	} else if (prefix2arg["Objective"] == "Cmax") {
		schedOptions["OBJECTIVE"] = "Cmax";
	} else if (prefix2arg["Objective"] == "TWT") {
		schedOptions["OBJECTIVE"] = "TWT";
	} else {
		out << prefix2arg["Objective"] << endl;
		Debugger::err << "Unknown objective!!!" << ENDL;
	}


	schedOptions["LS_CRIT_NODES_UPDATE_FREQ"] = "100";
	schedOptions["LS_CHK_COR"] = "false";
	schedOptions["LS_MAX_ITER"] = "500";
	schedOptions["PLANNER_LAST_ITER_LS_MAX_ITER"] = "20000";
	schedOptions["CS_ALLOWED_SCHEDULERS"] = "1&2&3&4&5&6&7"; // Allow ATC during the initialization (it will not be used later!!!)
	if (!prefix2arg.contains("GASolImpr")) {
		Debugger::err << "GASolImpr not specified!!!" << ENDL;
	} else {
		schedOptions["GA_IMPROVE_SOLUTION"] = prefix2arg["GASolImpr"];
	}
	ScalarObjective* objective = NULL;
	if (!schedOptions.contains("OBJECTIVE")) {
		Debugger::err << "No objective specified!!!" << ENDL;
	} else {
		if (schedOptions["OBJECTIVE"] == "TWT") {
			objective = new TWT;
		} else if (schedOptions["OBJECTIVE"] == "Cmax") {
			objective = new Cmax;
		} else {
			Debugger::err << "Invalid objective!!!" << ENDL;
		}
	}

	// Scheduler object

	Scheduler* scheduler = NULL;

	if (prefix2arg["PrioScheduler"] == "CSLS") {

		scheduler = new CombinedSchedulerLS;

		CombinedSchedulerLS* csls = (CombinedSchedulerLS*) scheduler;

		Scheduler* sch = 0;

		sch = new WFIFOScheduler;
		sch->ID = 1;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WFIFOScheduler;
		sch->ID = 2;
		sch->obj = objective->clone();
		((WFIFOScheduler*) sch)->weightedFIFO(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 3;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WEODScheduler;
		sch->ID = 4;
		sch->obj = objective->clone();
		((WEODScheduler*) sch)->weightedEOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 5;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(false);
		csls->combinedSchedulerObject() << sch;

		sch = new WMODScheduler;
		sch->ID = 6;
		sch->obj = objective->clone();
		((WMODScheduler*) sch)->weightedMOD(true);
		csls->combinedSchedulerObject() << sch;

		sch = new ATCANScheduler;
		sch->ID = 7;
		//sch->obj = objective->clone();
		sch->setObjective(*objective);
		((ATCANScheduler*) sch)->considerSucc(false);
		((ATCANScheduler*) sch)->kappaOptim(true);
		csls->combinedSchedulerObject() << sch;

		sch = 0;

		RandGenMT* randGen = new RandGenMT(Rand::rndSeed()); // The random numbers generator

		csls->combinedSchedulerObject().obj = objective->clone();
		csls->localSearchObject().setObjective(objective);

		// Set the initial random number generator for the Ls
		csls->localSearchObject().setRandGen(randGen);

	} else if (prefix2arg["PrioScheduler"] == "RND") {

		scheduler = new RNDScheduler;

		//getchar();

	}

	// Protocol object
	Protocol protocol;
	protocol.init();
	//QFile proto_file(test_inst_dir + "protocol_1.xml");
	protocol.setFile(proto_file);

	scheduler->obj = objective->clone();

	// Schedule object
	Schedule sched;

	out << "Creating the GA1 planner" << endl;

	// The search algorithm
	GAPlanner1 plannerGA;

	//plannerGA.maxIter(prefix2arg["GlobMaxIter"].toInt());
	//plannerGA.maxTimeMs(prefix2arg["GlobMaxTimeM"].toInt()* 60 * 1000);
	//planner.maxIterDecl(6);
	//planner.maxTimeMs(1 * 30 * 60 * 1000);

	plannerGA << &pmm << &sched << &rc << scheduler << &protocol << objective;

	plannerGA.schedOptions = schedOptions;

	out << "Running the search algorithm ..." << endl;
	//out << "PM before the iterations:" << endl;
	//out << pmm.pm << endl;

	//planner.init();

	QTime time;

	//out << p1 << endl;
	//getchar();

	/*
	out << "List of BOPs in p1:" << endl;
	for (int i = 0; i < p1.bops.size(); i++) {
					out << p1.bops[i]->ID << endl;
	}
	out << "List of keys in p1:" << endl;
	for (int i = 0; i < p1.bopid2idx.size(); i++) {
					out << p1.bopid2idx.keys()[i] << endl;
	}
	getchar();
	 */
	//getchar();

	QObject::connect(&plannerGA, SIGNAL(sigFinished()), &app, SLOT(quit()));

	//getchar();

	out << "Running test in " << test_inst_dir << endl;

	plannerGA.n_genomes = 50; // 200 - Park & Choi
	plannerGA.seed = prefix2arg["RandSeed"].toInt();
	plannerGA.n_cycles = 200; // 1000 - Park & Choi
	plannerGA.n_secs = 12000;
	plannerGA.p_cross = 0.8;
	plannerGA.p_mut = 0.2;
	plannerGA.p_repl = 0.6;

	plannerGA.run();

	return 0; // app.exec();
}

/******************************************************************************/

#endif


int main(int argc, char *argv[]) {

	//QApplication app(argc, argv);

	QTextStream out(stdout);

	// Parse the arguments
	QStringList arguments; // = app.arguments();
	for (int i = 0; i < argc; i++) {
		arguments << QString(argv[i]);
	}

	QHash<QString, QString> prefix2arg;
	for (int i = 1; i < arguments.size(); i++) { // The first one is the program's directory
		out << arguments[i] << endl;
		QStringList argssplitted = arguments[i].split(":");
		if (argssplitted.size() < 2) {
			Debugger::err << "Arguments not specified correctly!!!" << ENDL;
		} else {
			prefix2arg[argssplitted[0]] = argssplitted[1];
		}
	}

	if (!prefix2arg.contains("Algorithm")) {
		Debugger::err << "No solution algorithm specified!" << ENDL;
	} else {
		if (prefix2arg["Algorithm"] == "VNS") { // Run the VNS scheduler

			return main_VNS(argc, argv);

		}
#ifdef GA_ALG
		else if (prefix2arg["Algorithm"] == "GA") {

			return main_GA(argc, argv);

		} else if (prefix2arg["Algorithm"] == "GA1") {

			return main_GA1(argc, argv);

		}
#endif
		else {
			Debugger::err << "No feasible solution algorithm specified!!!" << ENDL;
		}
	}

	return -1;

}
