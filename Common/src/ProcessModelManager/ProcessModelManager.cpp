/* 
 * File:   ProcessModelManager.cpp
 * Author: DrSobik
 * 
 * Created on August 3, 2011, 10:37 AM
 */

#include <QtCore/qtextstream.h>

#include "ProcessModelManager"
#include "BillOfMaterials"
#include "BillOfProcesses"
#include "Order"

ProcessModelManager::ProcessModelManager() {
    init();
}

ProcessModelManager::~ProcessModelManager() {
    clear();
}

void ProcessModelManager::init() {
    // Clear the process model
    //for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
    //	delete pm.ops[nit];
    //}
    //pm.graph.clear();

    pm.clear();

    prodid2prevbopid.clear();
    ordid2locpm.clear();

    // Create queue of operation IDs
    maxfreeIDs = 1000000000;
    //	freeIDs.clear();
    //	freeIDs.reserve(maxfreeIDs);
    //for (int i = maxfreeIDs; i >= 1; i--) { // IMPORTANT!!! Starting from 1
    //freeIDs.enqueue(i); // Important!!! All IDs MUST be positive.
    //		freeIDs.push(i);
    //}

    // Create queue of item IDs
    //maxfreeItemIDs = 10000000;
    //freeItemIDs.clear();
    //freeItemIDs.reserve(maxfreeItemIDs);
    //for (int i = maxfreeItemIDs; i >= 1; i--) { // IMPORTANT!!! Starting from 1
    //	freeItemIDs.push(i);
    //}

    // Create tail and head
    pm.head = pm.graph.addNode();
    pm.ops[pm.head] = new Operation(0.0);
    pm.ops[pm.head]->ID = -(maxfreeIDs + 2); //-1;
    pm.ops[pm.head]->itemID(-(maxfreeIDs + 2));
    pm.ops[pm.head]->type = 0;
    pm.ops[pm.head]->w(0.0);
    pm.ops[pm.head]->ir(0.0);
    pm.ops[pm.head]->r(0.0);
    pm.ops[pm.head]->d(double(Math::MAX_DOUBLE));

    pm.tail = pm.graph.addNode();
    pm.ops[pm.tail] = new Operation(0.0);
    pm.ops[pm.tail]->ID = -(maxfreeIDs + 1); //-2;
    pm.ops[pm.tail]->itemID(-(maxfreeIDs + 1));
    pm.ops[pm.tail]->type = 0;
    pm.ops[pm.tail]->w(0.0);
    pm.ops[pm.tail]->ir(0.0);
    pm.ops[pm.tail]->r(0.0);
    pm.ops[pm.tail]->d(double(Math::MAX_DOUBLE));
}

void ProcessModelManager::clear() {
    pm.clear();

    prodid2prevbopid.clear();
    ordid2locpm.clear();

    ordman = NULL;
    prodman = NULL;

    //	init(); // If uncommented then two operation are always created additionally (lost?)
}

void ProcessModelManager::deleteOrdSubgraph(const int& ID) {
    QTextStream out(stdout);

    if (ID <= 0) {
        Debugger::err << "ProcessModelManager::deleteOrdSubgraph : Trying to delete order " << ID << ENDL;
    }

    if (!ordid2locpm.contains(ID)) {
        Debugger::warn << "ProcessModelManager::deleteOrdSubgraph : Order " << ID << " is not in the PM!" << ENDL;
        //getchar();
        return;
    }

    ListDigraph::Node lhead = INVALID;
    ListDigraph::Node ltail = INVALID;
    ListDigraph::Node curnode = INVALID;
    ListDigraph::Arc curarc = INVALID;

    //out << "Deleting the subgraph ... " << endl;
    for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
        if (pm.ops[nit]->ID == ordid2locpm[ID].first) {
            lhead = nit;
        }

        if (pm.ops[nit]->ID == ordid2locpm[ID].second) {
            ltail = nit;
        }
    }

    // Remove the element
    ordid2locpm.remove(ID);

    if (lhead == INVALID || ltail == INVALID) {
        Debugger::err << "ProcessModelManager::deleteOrdSubgraph  : Invalid lhead or ltail!!!" << ENDL;
    }

    //out << "Current order ID: " << curorders[co]->ID << endl;

    // Isolate the subgraph

    //out << "Isolating the subgraph ..." << endl;

    //out << "Head operation is : " << pm.ops[lhead]->OID << ":" << pm.ops[lhead]->ID << endl;
    //out << "Tail operation is : " << pm.ops[ltail]->OID << ":" << pm.ops[ltail]->ID << endl;

    //out << pm << endl;

    //out << "Deleting the outgoing arcs of ltail ..." << endl;
    // Delete all outgoing arcs of the ltail
    QList<ListDigraph::Arc> arcs;
    arcs.clear();
    for (ListDigraph::OutArcIt oait(pm.graph, ltail); oait != INVALID; ++oait) {
        arcs.append(oait);
    }

    //out << "Done deleting the outgoing arcs of ltail." << endl;

    //out << "Deleting the incoming arcs of lhead ..." << endl;
    // Delete all incoming arcs of the lhead
    //out << "Number of incoming arcs for lhead is: " << countInArcs(pm.graph, lhead) << endl;
    //int n_in_arcs = countInArcs(pm.graph, lhead);
    for (ListDigraph::InArcIt iait(pm.graph, lhead); iait != INVALID; ++iait) {
        //out << "ID of the arc before eracing:"<< pm.graph.id(iait) << endl;
        //out << "Deleting the arc ..." << endl;
        //out << "Number of incoming arcs for lhead before arc deleting is: " << countInArcs(pm.graph, lhead) << endl;
        /** BUG: Something is wrong, because the loop tries to delete invalid arcs which in fact == INVALID !!! */

        //if (!pm.graph.valid(iait)) {
        //break;
        //}
        //pm.graph.erase(iait);
        arcs.append(iait);

        //n_in_arcs--;
        //if (n_in_arcs == 0) break;
        //out << "Number of incoming arcs for lhead after arc deleting is: " << countInArcs(pm.graph, lhead) << endl;
        //out << "ID of the arc after eracing:"<< pm.graph.id(iait) << endl;
        //getchar();
        //counter++;
        //out << "Done deleting the arc." << endl;
        //out << "Times deleted: " << counter << endl;
    }

    for (int i = 0; i < arcs.size(); i++) {
        pm.graph.erase(arcs[i]);
    }
    arcs.clear();
    //out << "Done deleting the incoming arcs of lhead." << endl;



    //out << "Done isolating the subgraph." << endl;

    // Run BFS and collect the nodes of the isolated subgraph
    QList<ListDigraph::Node> subnodes;

    Bfs<ListDigraph> bfs(pm.graph);
    bfs.init();
    bfs.addSource(lhead);

    //out << "Running BFS and collecting nodes ..." << endl;
    while (!bfs.emptyQueue()) {
        curnode = bfs.processNextNode();

        subnodes.append(curnode);
    }
    if (!subnodes.contains(lhead)) {
        Debugger::err << "ProcessModelManager::deleteOrdSubgraph : Subnodes does not contain lhead!" << ENDL;
    }
    if (!subnodes.contains(ltail)) {
        out << pm << endl;
        Debugger::warn << "Order : " << ID << ENDL;
        Debugger::err << "ProcessModelManager::deleteOrdSubgraph : Subnodes does not contain ltail!" << ENDL;
    }

    //################### DEBUG
    /*
    int numordnodes = 0;
    for (ListDigraph::NodeIt nit(pm.graph); nit != INVALID; ++nit) {
            if (pm.ops[nit]->OID == ID) numordnodes++;
    }
    if (numordnodes != subnodes.size()) {
            Debugger::err << "ProcessModelManager::deleteOrdSubgraph : Node deleting problem!!!" << ENDL;
    }
     */
    //################### DEBUG

    //out << "Size of the subnodes : " << subnodes.size() << endl;
    //getchar();
    //out << "Done running BFS an collecting nodes." << endl;

    // Iterate through the collected nodes, delete the operations and arcs associated with them and then the nodes themselves
    for (int i = 0; i < subnodes.size(); i++) {
        // Delete the associated operation (enqueue its ID first)

        //freeIDs.enqueue(Math::abs(pm.ops[subnodes[i]]->ID));
        //		freeIDs.push(Math::abs(pm.ops[subnodes[i]]->ID));
        //Debugger::info << pm.ops[subnodes[i]]->ID << ENDL;

        delete pm.ops[subnodes[i]];

        arcs.clear();
        // Delete the associated incoming arcs
        for (ListDigraph::InArcIt iait(pm.graph, subnodes[i]); iait != INVALID; ++iait) {
            //if (!pm.graph.valid(iait)) break;
            //pm.graph.erase(iait);
            arcs.append(iait);
        }

        // Delete the associated outgoing arcs
        for (ListDigraph::OutArcIt oait(pm.graph, subnodes[i]); oait != INVALID; ++oait) {
            //pm.graph.erase(oait);
            arcs.append(oait);
        }

        // Delete the collected arcs
        for (int i = 0; i < arcs.size(); i++) {
            pm.graph.erase(arcs[i]);
        }
        arcs.clear();

    }

    // Delete the node themselves
    for (int i = 0; i < subnodes.size(); i++) {
        pm.graph.erase(subnodes[i]);
    }

    //out << "Done deleting the subgraph." << endl;
}

void ProcessModelManager::insertOrdSubgraph(const Product& prod, Order & ord) {

    insertOrdSubgraph(prod, ord, this->pm);

    return;

    /*
	
    ListDigraph::Node lhead;
    ListDigraph::Node ltail;
    ListDigraph::Node curnode;
    ListDigraph::Arc curarc;

    ListDigraph lpmg;
    ListDigraph::NodeMap<int> lotypeID(lpmg);
    ListDigraph::NodeMap<int> litemID(lpmg);

    // Erase the previous temporary graph
    lpmg.clear();

    //Common::Debugger::info << "Current size of lpmg before copying: " << countNodes(lpmg) << Common::ENDL;
    //Common::Debugger::info << "Current size of BOP graph before copying: " << countNodes(prodman->products[cp]->bops[prodman->products[cp]->bopid2idx[prodman->products[cp]->bopID]]->graph()) << Common::ENDL;

    // Generate the needed BOP before adding it to the process model
    //out << "Generating BOP..." << endl;

    BillOfProcesses* curBOP = prod.bops[prod.bopid2idx.value(prod.bopID)];

    curBOP->generate();
    //out << "Generated BOP." << endl;

    ListDigraph::NodeMap<int> bopItemID(curBOP->graph()); // Used to assign the generated item IDs to the BOP's nodes

    QMap<ListDigraph::Node, QList<ListDigraph::Node> > bomNode2BOPNodes = prod.bops[prod.bopid2idx.value(prod.bopID)]->bomNode2BopNodes();
    BillOfMaterials* curBOM = prod.bops[prod.bopid2idx.value(prod.bopID)]->bom();

    // Create items for the order according to the current product description
    ord.clearItems();
    Item* curItem = NULL;
    for (ListDigraph::NodeIt nit(curBOM->graph); nit != INVALID; ++nit) {
            if (nit == curBOM->head || nit == curBOM->tail) {
                    continue;
            } else {
                    curItem = new Item;

                    curItem->ID = freeItemIDs.pop(); // Global item ID

                    // Set the BOP's nodes item IDs
                    for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                            bopItemID[bomNode2BOPNodes[nit][i]] = curItem->ID;
                    }

                    //curItem->order = &ord;
                    //curItem->type = curBOM->itypeID[nit];
                    //curItem->route = curBOP->itemRoutes()[curBOM->itemID[nit]][curBOP->itemRouteIdx()[curBOM->itemID[nit]]];
                    //curItem->curStepIdx = -1; // Set to -1 when replanning an item. -1 means that the item has not started its processing in the simulation

                    // Add the current item to the order
                    //ord << curItem;
            }
    }

    // Copy the graph structure of the current local BOP
    digraphCopy(curBOP->graph(), lpmg)
                    .node(curBOP->graphHead(), lhead)
                    .node(curBOP->graphTail(), ltail)
                    .nodeMap(curBOP->otypeID(), lotypeID)
                    .nodeMap(bopItemID, litemID).run();

    // Degenerate the BOP to save memory
    curBOP->degenerate();

    //Common::Debugger::info << "Current size of lpmg after copying: " << countNodes(lpmg) << Common::ENDL;

    // Iterate through the nodes of the new subgraph and add the corresponding nodes and arcs to the global PM

    ListDigraph::NodeMap<ListDigraph::Node> nmap(lpmg);

    // Map the lhead and ltail!!!
    ListDigraph::Node pmlhead = pm.graph.addNode();
    nmap[lhead] = pmlhead;
    pm.ops[pmlhead] = NULL;
    ListDigraph::Node pmltail = pm.graph.addNode();
    nmap[ltail] = pmltail;
    pm.ops[pmltail] = NULL;


    // Copy the nodes into the PM.
    for (ListDigraph::NodeIt nit(lpmg); nit != INVALID; ++nit) {
            if (nit != lhead && nit != ltail) {
                    curnode = pm.graph.addNode();
                    pm.ops[curnode] = NULL;
                    nmap[nit] = curnode;
            }
    }

    // Copy the arcs and mark them as conjunctive
    for (ListDigraph::ArcIt ait(lpmg); ait != INVALID; ++ait) {
            curarc = pm.graph.addArc(nmap[lpmg.source(ait)], nmap[lpmg.target(ait)]);
            pm.conjunctive[curarc] = true;
    }

    // Create real operations associated with the nodes of the global PM
    for (ListDigraph::NodeIt nit(lpmg); nit != INVALID; ++nit) {
            curnode = nmap[nit];

            if (pm.ops[curnode] != NULL) {
                    Debugger::err << "ProcessModelManager::insertOrdSubgraph : Assigning to a non-NULL operation!!!" << ENDL;
            }

            if (nit == lhead) {
                    pm.ops[curnode] = new Operation(0.0);
                    pm.ops[curnode]->ID = -freeIDs.pop(); //-freeIDs.dequeue(); //-1;
                    pm.ops[curnode]->OID = ord.ID;
                    pm.ops[curnode]->itemID(-1);
                    pm.ops[curnode]->type = 0;
                    pm.ops[curnode]->ir(ord.r);
                    pm.ops[curnode]->r(ord.r);
                    pm.ops[curnode]->d(ord.d);
                    pm.ops[curnode]->w(ord.w);
                    pm.ops[curnode]->p(0.0);
            } else {
                    if (nit == ltail) {
                            pm.ops[curnode] = new Operation(0.0);
                            pm.ops[curnode]->ID = -freeIDs.pop(); //-freeIDs.dequeue(); //-2;
                            pm.ops[curnode]->OID = ord.ID;
                            pm.ops[curnode]->itemID(-1);
                            pm.ops[curnode]->type = 0;
                            pm.ops[curnode]->ir(0.0);
                            pm.ops[curnode]->r(0.0);
                            pm.ops[curnode]->d(ord.d);
                            pm.ops[curnode]->w(ord.w);
                            pm.ops[curnode]->p(0.0);
                    } else { // Other operations
                            pm.ops[curnode] = new Operation(1.0);
                            pm.ops[curnode]->ID = freeIDs.pop(); //freeIDs.dequeue(); //curopid;
                            pm.ops[curnode]->OID = ord.ID;
                            pm.ops[curnode]->itemID(litemID[nit]);
                            pm.ops[curnode]->type = lotypeID[nit];
                            pm.ops[curnode]->ir(ord.r);
                            pm.ops[curnode]->r(ord.r);
                            pm.ops[curnode]->d(ord.d);
                            pm.ops[curnode]->w(ord.w);

                            //curopid++;
                    }
            }
            //QTextStream out(stdout);
            //out << "Adding operation: " << *pm.ops[curnode] << endl;
    }

    // Connect the head of the global PM with the head of the local one. The same for the tails
    curarc = pm.graph.addArc(pm.head, pmlhead);
    pm.conjunctive[curarc] = true;
    curarc = pm.graph.addArc(pmltail, pm.tail);
    pm.conjunctive[curarc] = true;

    // Update hash with the pairs of heads and tails for the orders
    if (ordid2locpm.contains(ord.ID)) {
            Debugger::err << "ProcessModelManager::insertOrdSubgraph : ordid2locpm already contains order " << ord.ID << ENDL;
    }
    ordid2locpm[ord.ID] = QPair<int, int > (pm.ops[pmlhead]->ID, pm.ops[pmltail]->ID);

    lpmg.clear();
	
     */

}

void ProcessModelManager::insertOrdSubgraph(const Product& prod, Order& ord, ProcessModel& apm) {
    QTextStream out(stdout);

#ifdef DEBUG	
    if (prod.ID != ord.type) {
        out << "Inserting order " << ord.ID << " of type " << ord.type << endl;
        out << "Corresponding product " << prod << endl;
        //out << "Corresponding BOP : " << endl << *prod.bopByID(prod.bopID) << endl;

        Debugger::err << "ProcessModelManager::insertOrdSubgraph : order-product mismatch!" << ENDL;
    }

    if (countNodes(apm.graph) < 2) { // This PM has some problems with its head and its tail
        out << apm << endl;
        out << ord << endl;
        Debugger::err << "ProcessModelManager::insertOrdSubgraph : A problem with the PM detected!" << ENDL;
    }
#endif	

    ListDigraph::Node lhead;
    ListDigraph::Node ltail;
    ListDigraph::Node curnode;
    ListDigraph::Arc curarc;

    ListDigraph lpmg;
    ListDigraph::NodeMap<int> lotypeID(lpmg);
    ListDigraph::NodeMap<int> litemID(lpmg);
    ListDigraph::NodeMap<int> litemType(lpmg);
    ListDigraph::NodeMap<int> lrouteID(lpmg);

    int nextOperCtr = 1; // Counter for operations within the order

    // Erase the previous temporary graph
    lpmg.clear();

    //Common::Debugger::info << "Current size of lpmg before copying: " << countNodes(lpmg) << Common::ENDL;
    //Common::Debugger::info << "Current size of BOP graph before copying: " << countNodes(prodman->products[cp]->bops[prodman->products[cp]->bopid2idx[prodman->products[cp]->bopID]]->graph()) << Common::ENDL;

    BillOfProcesses* curBOP = prod.bops[prod.bopid2idx.value(prod.bopID)];
    BillOfMaterials* curBOM = curBOP->bom();

    if (ord.action == Order::OA_PLAN_SCHED) { // In case the order must be completely replanned/rescheduled

        // Set the current BOM ID of the order 
        ord.BID = curBOM->ID;

        // Generate the needed BOP before adding it to the process model
        //prod.bops[prod.bopid2idx.value(prod.bopID)]->generate();
        curBOP->generate();

        ListDigraph::NodeMap<int> bopItemID(curBOP->graph()); // Used to assign the generated item IDs to the BOP's nodes
        ListDigraph::NodeMap<int> bopItemType(curBOP->graph()); // User to assign the generated item type to the BOP's nodes
        ListDigraph::NodeMap<int> bopRouteID(curBOP->graph()); // Used to assign the route IDs to the BOP's nodes

        // Initialize the item type of each operation in the BOP
        for (ListDigraph::NodeIt nit(curBOP->graph()); nit != INVALID; ++nit) {
            bopItemID[nit] = -1;
            bopItemType[nit] = -1;
            bopRouteID[nit] = -1;
        }

        QMap<ListDigraph::Node, QList<ListDigraph::Node> > bomNode2BOPNodes = curBOP->bomNode2BopNodes();

        //for (ListDigraph::NodeIt nit(curBOM->graph) ; nit != INVALID ; ++nit){
        //	out << "bomNode2BOPNodes size : " << bomNode2BOPNodes[nit].size() << endl;
        //}

        //getchar();

        // Create items for the order according to the current product description
        //ord.clearItems();
        Item curItem;
        int itemCtr = 0;
        for (ListDigraph::NodeIt nit(curBOM->graph); nit != INVALID; ++nit) {
            if (nit == curBOM->head || nit == curBOM->tail) {
                continue;
            } else {
                //curItem = new Item;

                itemCtr++;

                curItem.ID = (ord.ID << 16) + itemCtr; //freeItemIDs.pop(); // Global item ID

                curItem.orderID = ord.ID;
                curItem.type = curBOM->itypeID[nit];
                curItem.routeID = curBOP->itemRoutes()[curBOM->itemID[nit]][curBOP->itemRouteIdx()[curBOM->itemID[nit]]]->ID;
                curItem.curStepIdx = -1; // Set to -1 when replanning an item. -1 means that the item has not started its processing in the simulation

                // Set the BOP's nodes item IDs
                for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                    bopItemID[bomNode2BOPNodes[nit][i]] = curItem.ID;
                }

                // Set the BOP's nodes item IDs
                for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                    bopItemType[bomNode2BOPNodes[nit][i]] = curItem.type;
                }

                // Set the route IDs for each operation of the part
                for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                    bopRouteID[bomNode2BOPNodes[nit][i]] = curItem.routeID;
                }

                // ERROR!!! It is done in each iteration and not only for the best result!!! Add the current item to the order
                // ord << curItem;

                //out << "Created Item : " << endl << *curItem << endl;
            }
        }

        //for (ListDigraph::NodeIt nit(curBOP->graph()) ; nit != INVALID ; ++nit){
        //	out << bopItemID[nit] << endl;
        //}

        // Copy the graph structure of the current local BOP
        digraphCopy(curBOP->graph(), lpmg)
                .node(curBOP->graphHead(), lhead)
                .node(curBOP->graphTail(), ltail)
                .nodeMap(curBOP->otypeID(), lotypeID)
                .nodeMap(bopItemID, litemID)
                .nodeMap(bopItemType, litemType)
                .nodeMap(bopRouteID, lrouteID).run();

        //for (ListDigraph::NodeIt nit(lpmg) ; nit != INVALID ; ++nit){
        //	out << litemID[nit] << endl;
        //}

        // Degenerate the BOP to save memory
        //prod.bops[prod.bopid2idx.value(prod.bopID)]->degenerate();
        curBOP->degenerate();

        //Common::Debugger::info << "Current size of lpmg after copying: " << countNodes(lpmg) << Common::ENDL;

        // Iterate through the nodes of the new subgraph and add the corresponding nodes and arcs to the global PM

        ListDigraph::NodeMap<ListDigraph::Node> nmap(lpmg);

        // Map the lhead and ltail!!!
        ListDigraph::Node pmlhead = apm.graph.addNode();
        nmap[lhead] = pmlhead;
        apm.ops[pmlhead] = NULL;
        ListDigraph::Node pmltail = apm.graph.addNode();
        nmap[ltail] = pmltail;
        apm.ops[pmltail] = NULL;


        // Copy the nodes into the PM.
        for (ListDigraph::NodeIt nit(lpmg); nit != INVALID; ++nit) {
            if (nit != lhead && nit != ltail) {
                curnode = apm.graph.addNode();
                apm.ops[curnode] = NULL;
                nmap[nit] = curnode;
            }
        }

        // Copy the arcs and mark them as conjunctive
        for (ListDigraph::ArcIt ait(lpmg); ait != INVALID; ++ait) {
            curarc = apm.graph.addArc(nmap[lpmg.source(ait)], nmap[lpmg.target(ait)]);
            apm.conjunctive[curarc] = true;
        }

        // IMPORTANT !!! All the data assigned to the operations will be automatically saved when saving the best PM during the algorithm

        // Create real operations associated with the nodes of the global PM
        for (ListDigraph::NodeIt nit(lpmg); nit != INVALID; ++nit) {
            curnode = nmap[nit];

            if (apm.ops[curnode] != NULL) {
                Debugger::err << "ProcessModelManager::insertOrdSubgraph : Assigning to a non-NULL operation!!!" << ENDL;
            }

            if (nit == lhead) {
                apm.ops[curnode] = new Operation(0.0);
                apm.ops[curnode]->ID = -((ord.ID << 16) + nextOperCtr); //*/ -freeIDs.pop(); //-freeIDs.dequeue(); //-1;
                apm.ops[curnode]->OID = ord.ID;
                apm.ops[curnode]->orderType(-1);
                apm.ops[curnode]->bomID(-1);
                apm.ops[curnode]->itemID(-1);
                apm.ops[curnode]->itemType(-1);
                apm.ops[curnode]->routeID(-1);
                apm.ops[curnode]->stepIdx(-1);
                apm.ops[curnode]->type = 0;
                apm.ops[curnode]->ir(ord.r);
                apm.ops[curnode]->r(ord.r);
                apm.ops[curnode]->d(ord.d);
                apm.ops[curnode]->w(ord.w);
                apm.ops[curnode]->p(0.0);
            } else {
                if (nit == ltail) {
                    apm.ops[curnode] = new Operation(0.0);
                    apm.ops[curnode]->ID = -((ord.ID << 16) + nextOperCtr); //*/-freeIDs.pop(); //-freeIDs.dequeue(); //-2;
                    apm.ops[curnode]->OID = ord.ID;
                    apm.ops[curnode]->orderType(-1);
                    apm.ops[curnode]->bomID(-1);
                    apm.ops[curnode]->itemID(-1);
                    apm.ops[curnode]->itemType(-1);
                    apm.ops[curnode]->routeID(-1);
                    apm.ops[curnode]->stepIdx(-1);
                    apm.ops[curnode]->type = 0;
                    apm.ops[curnode]->ir(ord.r); //ir(0.0);
                    apm.ops[curnode]->r(ord.r); //r(0.0);
                    apm.ops[curnode]->d(ord.d);
                    apm.ops[curnode]->w(ord.w);
                    apm.ops[curnode]->p(0.0);
                } else { // Other operations
                    apm.ops[curnode] = new Operation(1.0);
                    apm.ops[curnode]->ID = ((ord.ID << 16) + nextOperCtr); //*/freeIDs.pop(); //freeIDs.dequeue(); //curopid;
                    apm.ops[curnode]->OID = ord.ID;
                    apm.ops[curnode]->orderType(ord.type);
                    apm.ops[curnode]->bomID(curBOM->ID);
                    apm.ops[curnode]->itemID(litemID[nit]);
                    apm.ops[curnode]->itemType(litemType[nit]);
                    apm.ops[curnode]->routeID(lrouteID[nit]);
                    //apm.ops[curnode]->stepIdx();
                    apm.ops[curnode]->type = lotypeID[nit];
                    apm.ops[curnode]->ir(ord.r);
                    apm.ops[curnode]->r(ord.r);
                    apm.ops[curnode]->d(ord.d);
                    apm.ops[curnode]->w(ord.w);

                    //curopid++;
                }
            }
            // QTextStream out(stdout);
            // out << "Added operation: " << *apm.ops[curnode] << endl;

            nextOperCtr++;
        }

        //out << apm << endl;

        // Connect the head of the global PM with the head of the local one. The same for the tails
        curarc = apm.graph.addArc(apm.head, pmlhead);
        apm.conjunctive[curarc] = true;
        curarc = apm.graph.addArc(pmltail, apm.tail);
        apm.conjunctive[curarc] = true;

        lpmg.clear();

        // Add for possible later deleting of the order from the PM
        ordid2locpm[ord.ID].first = apm.ops[pmlhead]->ID;
        ordid2locpm[ord.ID].second = apm.ops[pmltail]->ID;

    } // PLAN_SCHED

    // In case the order will have only its parts replanned
    if (ord.action == Order::OA_PLAN_PARTS_SCHED) {

        int nOperAdded = 0; // Number of real operations added to the graph while inserting this order

        out << "Adding order : " << ord << endl;

        //out << "PM before adding order : " << endl << apm << endl;

        // Generate the needed BOP before adding it to the process model
        //out << "Generating BOP..." << endl;
        //prod.bops[prod.bopid2idx.value(prod.bopID)]->generate();
        curBOP->generate();
        //out << "Generated BOP." << endl;

        ListDigraph::NodeMap<int> bopItemID(curBOP->graph()); // Used to assign the generated item IDs to the BOP's nodes
        ListDigraph::NodeMap<int> bopItemType(curBOP->graph()); // User to assign the generated item type to the BOP's nodes
        ListDigraph::NodeMap<int> bopRouteID(curBOP->graph()); // Used to assign the route IDs to the BOP's nodes

        // Initialize the item type of each operation in the BOP
        for (ListDigraph::NodeIt nit(curBOP->graph()); nit != INVALID; ++nit) {
            bopItemID[nit] = -1;
            bopItemType[nit] = -1;
            bopRouteID[nit] = -1;
        }

        QMap<ListDigraph::Node, QList<ListDigraph::Node> > bomNode2BOPNodes = curBOP->bomNode2BopNodes();

        //for (ListDigraph::NodeIt nit(curBOM->graph) ; nit != INVALID ; ++nit){
        //	out << "bomNode2BOPNodes size : " << bomNode2BOPNodes[nit].size() << endl;
        //}

        //getchar();

        // Create items for the order according to the current product description
        //ord.clearItems();
        Item curItem;
        //int itemCtr = 0;
        for (ListDigraph::NodeIt nit(curBOM->graph); nit != INVALID; ++nit) {
            if (nit == curBOM->head || nit == curBOM->tail) {
                continue;
            } else {

                // BUG!!!
                //itemCtr++; 

                // IMPORTANT !!! In this case the item IDs already exist and they should NOT be newly generated !!!
                curItem.ID = curBOM->itemID[nit]; // BUG!!! (ord.ID << 16) + itemCtr; //freeItemIDs.pop(); // Global item ID

                curItem.orderID = ord.ID;
                curItem.type = curBOM->itypeID[nit];
                curItem.routeID = curBOP->itemRoutes()[curBOM->itemID[nit]][curBOP->itemRouteIdx()[curBOM->itemID[nit]]]->ID;
                curItem.curStepIdx = -1; // Set to -1 when replanning an item. -1 means that the item has not started its processing in the simulation

                // Set the BOP's nodes item IDs
                for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                    bopItemID[bomNode2BOPNodes[nit][i]] = curItem.ID;
                }

                // Set the BOP's nodes item IDs
                for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                    bopItemType[bomNode2BOPNodes[nit][i]] = curItem.type;
                }

                // Set the route IDs for each operation of the part
                for (int i = 0; i < bomNode2BOPNodes[nit].size(); i++) {
                    bopRouteID[bomNode2BOPNodes[nit][i]] = curItem.routeID;
                }

                // ERROR!!! It is done in each iteration and not only for the best result!!! Add the current item to the order
                // ord << curItem;

                //out << "Created Item : " << endl << *curItem << endl;
            }
        }

        //for (ListDigraph::NodeIt nit(curBOP->graph()) ; nit != INVALID ; ++nit){
        //	out << bopItemID[nit] << endl;
        //}

        // Copy the graph structure of the current local BOP
        digraphCopy(curBOP->graph(), lpmg)
                .node(curBOP->graphHead(), lhead)
                .node(curBOP->graphTail(), ltail)
                .nodeMap(curBOP->otypeID(), lotypeID)
                .nodeMap(bopItemID, litemID)
                .nodeMap(bopItemType, litemType)
                .nodeMap(bopRouteID, lrouteID).run();

        //for (ListDigraph::NodeIt nit(lpmg) ; nit != INVALID ; ++nit){
        //	out << litemID[nit] << endl;
        //}

        // Degenerate the BOP to save memory
        //prod.bops[prod.bopid2idx.value(prod.bopID)]->degenerate();
        curBOP->degenerate();

        //Common::Debugger::info << "Current size of lpmg after copying: " << countNodes(lpmg) << Common::ENDL;

        // Iterate through the nodes of the new subgraph and add the corresponding nodes and arcs to the global PM

        ListDigraph::NodeMap<ListDigraph::Node> nmap(lpmg);

        // Map the lhead and ltail!!!
        ListDigraph::Node pmlhead = apm.graph.addNode();
        nmap[lhead] = pmlhead;
        apm.ops[pmlhead] = NULL;
        ListDigraph::Node pmltail = apm.graph.addNode();
        nmap[ltail] = pmltail;
        apm.ops[pmltail] = NULL;


        // Copy the nodes into the PM.
        for (ListDigraph::NodeIt nit(lpmg); nit != INVALID; ++nit) {
            if (nit != lhead && nit != ltail) {
                curnode = apm.graph.addNode();
                apm.ops[curnode] = NULL;
                nmap[nit] = curnode;
            }
        }

        // Copy the arcs and mark them as conjunctive
        for (ListDigraph::ArcIt ait(lpmg); ait != INVALID; ++ait) {
            curarc = apm.graph.addArc(nmap[lpmg.source(ait)], nmap[lpmg.target(ait)]);
            apm.conjunctive[curarc] = true;
        }

        // IMPORTANT !!! All the data assigned to the operations will be automatically saved when saving the best PM during the algorithm

        // Create real operations associated with the nodes of the global PM
        for (ListDigraph::NodeIt nit(lpmg); nit != INVALID; ++nit) {
            curnode = nmap[nit];

            if (apm.ops[curnode] != NULL) {
                Debugger::err << "ProcessModelManager::insertOrdSubgraph : Assigning to a non-NULL operation!!!" << ENDL;
            }

            if (nit == lhead) {
                apm.ops[curnode] = new Operation(0.0);
                apm.ops[curnode]->ID = -((ord.ID << 16) + nextOperCtr); //-freeIDs.pop(); //-freeIDs.dequeue(); //-1;
                apm.ops[curnode]->OID = ord.ID;
                apm.ops[curnode]->orderType(-1);
                apm.ops[curnode]->bomID(-1);
                apm.ops[curnode]->itemID(-1);
                apm.ops[curnode]->itemType(-1);
                apm.ops[curnode]->routeID(-1);
                apm.ops[curnode]->stepIdx(-1);
                apm.ops[curnode]->type = 0;
                apm.ops[curnode]->ir(ord.r);
                apm.ops[curnode]->r(ord.r);
                apm.ops[curnode]->d(ord.d);
                apm.ops[curnode]->w(ord.w);
                apm.ops[curnode]->p(0.0);
            } else {
                if (nit == ltail) {
                    apm.ops[curnode] = new Operation(0.0);
                    apm.ops[curnode]->ID = -((ord.ID << 16) + nextOperCtr); //-freeIDs.pop(); //-freeIDs.dequeue(); //-2;
                    apm.ops[curnode]->OID = ord.ID;
                    apm.ops[curnode]->orderType(-1);
                    apm.ops[curnode]->bomID(-1);
                    apm.ops[curnode]->itemID(-1);
                    apm.ops[curnode]->itemType(-1);
                    apm.ops[curnode]->routeID(-1);
                    apm.ops[curnode]->stepIdx(-1);
                    apm.ops[curnode]->type = 0;
                    apm.ops[curnode]->ir(ord.r); //ir(0.0);
                    apm.ops[curnode]->r(ord.r); //r(0.0);
                    apm.ops[curnode]->d(ord.d);
                    apm.ops[curnode]->w(ord.w);
                    apm.ops[curnode]->p(0.0);
                } else { // Other operations
                    apm.ops[curnode] = new Operation(1.0);
                    apm.ops[curnode]->ID = ((ord.ID << 16) + nextOperCtr); //freeIDs.pop(); //freeIDs.dequeue(); //curopid;
                    apm.ops[curnode]->OID = ord.ID;
                    apm.ops[curnode]->orderType(ord.type);
                    apm.ops[curnode]->bomID(curBOM->ID);
                    apm.ops[curnode]->itemID(litemID[nit]);
                    apm.ops[curnode]->itemType(litemType[nit]);
                    apm.ops[curnode]->routeID(lrouteID[nit]);
                    //apm.ops[curnode]->stepIdx();
                    apm.ops[curnode]->type = lotypeID[nit];
                    apm.ops[curnode]->ir(Math::max(ord.r, ordman->itemByID(apm.ops[curnode]->itemID()).releaseTime));
                    apm.ops[curnode]->r(apm.ops[curnode]->ir());
                    apm.ops[curnode]->d(ord.d);
                    apm.ops[curnode]->w(ord.w);

                    //curopid++;

                    // One more real operation has been added
                    nOperAdded++;

                }
            }
            // QTextStream out(stdout);
            // out << "Adding operation: " << *apm.ops[curnode] << endl;

            nextOperCtr++;
        }

        //out << apm << endl;

        // Connect the head of the global PM with the head of the local one. The same for the tails
        curarc = apm.graph.addArc(apm.head, pmlhead);
        apm.conjunctive[curarc] = true;
        curarc = apm.graph.addArc(pmltail, apm.tail);
        apm.conjunctive[curarc] = true;

        lpmg.clear();

        //out << "apm before adding started items : " << endl << apm << endl;

        // Mapping of item IDs onto the starting nodes of the replanned items
        QHash<int, ListDigraph::Node> itemID2StartNode;
        QHash<int, ListDigraph::Arc> itemID2StartNodeInArc;

        // Iterate over the first nodes in the apm and find the ones which belong to the item succeeding the current one
        for (ListDigraph::OutArcIt oait(apm.graph, pmlhead); oait != INVALID; ++oait) {
            ListDigraph::Node curNode = apm.graph.target(oait);
            int curItmID = apm.ops[curNode]->itemID();

            itemID2StartNode[curItmID] = curNode;
            itemID2StartNodeInArc[curItmID] = oait;

            //out << "First node operation : " << *apm.ops[curNode] << endl;
        }

        // In case there are no item precedences for the order, iterate over the items of the order and prepend their operations
        if (ord.itemIDPrececences.size() == 0) { // There are no precedence constraints between the items (probably the last item)

            // Remove the arc between pmlhead and pmltail
            QList<ListDigraph::Arc> remArcs;
            for (ListDigraph::OutArcIt oait(apm.graph, pmlhead); oait != INVALID; ++oait) {
                if (apm.graph.target(oait) == pmltail) {
                    remArcs.append(oait);
                }
            }

            for (int j = 0; j < remArcs.size(); j++) {
                apm.graph.erase(remArcs[j]);
            }

            // Iterate over the items of the order
            for (int i = 0; i < ord.itemIDs.size(); i++) {
                int curItemID = ord.itemIDs[i];
                Item& curItem = ordman->itemByID(curItemID);

                ListDigraph::Node sucNode = pmltail; // Succeeding node for the prepended one further
                ListDigraph::Node curNode = pmltail; // Points to the prepended node
                ListDigraph::Arc curArc = INVALID; // Points to the prepended arc

                // Check whether the current item has already started
                if (curItem.curStepIdx >= 0) { // This item has started indeed


                    // Add in parallel between apm.head and apm.tail
                    for (int j = curItem.operIDs.size() - 1; j > curItem.curStepIdx; j--) {
                        int curOperID = curItem.operIDs[j];

                        Operation& curOper = ordman->operByID(curOperID);

                        //out << "Prepending operation (old ID) " << curOper << endl;

                        // Add a new node to the model
                        curNode = apm.graph.addNode();

                        // Create a real operation in the PM within the apm
                        apm.ops[curNode] = new Operation(curOper);

                        // Initialize the scheduling relevant data of the operation
                        apm.ops[curNode]->s(0.0);
                        apm.ops[curNode]->p(0.0);
                        apm.ops[curNode]->toolID = -1;
                        apm.ops[curNode]->machID = -1;

                        // Set the ID of the operation
                        apm.ops[curNode]->ID = ((ord.ID << 16) + nextOperCtr);

                        // Set the current index of the operation in the item
                        apm.ops[curNode]->stepIdx(j);

                        //out << "Prepending operation (new ID) " << *apm.ops[curNode] << endl;

                        // out << "Added arc : " << apm.ops[curNode]->ID << " - > " << apm.ops[sucNode]->ID << endl;

                        // Add a connecting arc
                        curArc = apm.graph.addArc(curNode, sucNode);
                        apm.conjunctive[curArc] = true;

                        // To add correctly the operations
                        sucNode = curNode;

                        nextOperCtr++;

                        nOperAdded++;

                    } // Adding item's operation

                    // Add the incoming arc from pmlhead to curnode
                    curArc = apm.graph.addArc(pmlhead, curNode);
                    apm.conjunctive[curArc] = true;

                }

            } // Iterating over item IDs

        }

        // For each item of the order that has started, add it to the current local PM
        for (int i = 0; i < ord.itemIDPrececences.size(); i++) {
            QPair<int, int> curPrec = ord.itemIDPrececences[i];
            int startItemID = curPrec.first;
            int endItemID = curPrec.second;
            Item& curStartItem = ordman->itemByID(startItemID);
            //Item& curEndItem = ordman->itemByID(endItemID);

            if (curStartItem.curStepIdx >= 0) { // This item has started already -> add it to the local PM
                //out << "Adding item " << curStartItem.ID << " to the local PM ... " << endl;
                //out << "End item ID " << endItemID << endl;

                // For the current end item, remove the incoming arc 
                if (itemID2StartNodeInArc[endItemID] != INVALID) {
                    ListDigraph::Arc remArc = itemID2StartNodeInArc[endItemID];
                    //out << "Erasing arc : " <<  *apm.ops[apm.graph.source(remArc)] << " -> " << *apm.ops[apm.graph.target(remArc)] << endl;
                    apm.graph.erase(remArc);
                    itemID2StartNodeInArc[endItemID] = INVALID;
                }

                ListDigraph::Node sucItemStartNode = itemID2StartNode[endItemID];

                //out << "Got start node" << endl;

                ListDigraph::Node sucNode = sucItemStartNode; // Succeeding node for the prepended one further
                ListDigraph::Node curNode = sucItemStartNode; // Points to the prepended node
                ListDigraph::Arc curArc = INVALID; // Points to the prepended arc

                // Add all operations of the start item which have not finished yet AND are not processed at the moment
                for (int j = curStartItem.operIDs.size() - 1; j > curStartItem.curStepIdx; j--) {
                    int curOperID = curStartItem.operIDs[j];

                    Operation& curOper = ordman->operByID(curOperID);

                    //out << "Prepending operation (old ID) " << curOper << endl;

                    // Add a new node to the model
                    curNode = apm.graph.addNode();

                    // Create a real operation in the PM within the apm
                    apm.ops[curNode] = new Operation(curOper);

                    // Initialize the scheduling relevant data of the operation
                    apm.ops[curNode]->s(0.0);
                    apm.ops[curNode]->p(0.0);
                    apm.ops[curNode]->toolID = -1;
                    apm.ops[curNode]->machID = -1;

                    // Set the ID of the operation
                    apm.ops[curNode]->ID = ((ord.ID << 16) + nextOperCtr);

                    // Set the current index of the operation in the item
                    apm.ops[curNode]->stepIdx(j);

                    //out << "Prepending operation (new ID) " << *apm.ops[curNode] << endl;

                    // out << "Added arc : " << apm.ops[curNode]->ID << " - > " << apm.ops[sucNode]->ID << endl;

                    // Add a connecting arc
                    curArc = apm.graph.addArc(curNode, sucNode);
                    apm.conjunctive[curArc] = true;

                    // To add correctly the operations
                    sucNode = curNode;

                    nextOperCtr++;

                    nOperAdded++;

                }

                // Add the incoming arc from pmlhead to curnode
                curArc = apm.graph.addArc(pmlhead, curNode);
                apm.conjunctive[curArc] = true;

                //out << "Prepended started item : " << curStartItem.ID << endl;
            } // started item

        } // item precedences

        //out << "apm after adding started items : " << endl << apm << endl;

        if (nOperAdded == 0) {
            out << "No read operation inserted -> no corresponding information in the PM" << endl;
            //out << apm << endl;
            Debugger::warn << "No operations inserted into the PM despite PLAN_PARTS_SCHED option! Probably executing the last operation of the last part." << ENDL;

        }

        // Add for possible later deleting of the order from the PM
        ordid2locpm[ord.ID].first = apm.ops[pmlhead]->ID;
        ordid2locpm[ord.ID].second = apm.ops[pmltail]->ID;



        //Debugger::err << "ProcessModelManager::insertOrdSubgraph : Inserting orders for PLAN_PARTS_SCHED not implemented yet!" << ENDL;

    }

    // In case the order will be rescheduled only
    if (ord.action == Order::OA_SCHED) {

        //out << "PM before adding an order to reschedule : " << apm << endl;

        //out << "Adding order : " << ord << endl;

        int appendedOps = 0; // Just to check whether at least one operation in the order has been appended

        QHash<int, QPair<ListDigraph::Node, ListDigraph::Node> > itemID2StartEndNodes;

        itemID2StartEndNodes.clear();

        // Initialize the counted of the operations for this order
        nextOperCtr = 1;

        // Iterate over the items of the order and add them to the local PM
        for (int i = 0; i < ord.itemIDs.size(); i++) {
            int curItemID = ord.itemIDs[i];
            Item& curItem = ordman->itemByID(curItemID);
            QList<ListDigraph::Node> curPartGraphNodes;

            // Add the operations of the current items to the graph
            curPartGraphNodes.clear();
            curPartGraphNodes.reserve(curItem.operIDs.size() - curItem.curStepIdx);
            for (int j = curItem.curStepIdx + 1; j < curItem.operIDs.size(); j++) { // These operations have not started yet
                const int curOperID = curItem.operIDs[j];
                const Operation& curOper = ordman->operByID(curOperID);
                ListDigraph::Node curNode = INVALID;

                //out << "Appending operation (old ID) " << curOper << endl;

                // Add a new node to the graph
                curNode = apm.graph.addNode();

                // Preserve it for later
                curPartGraphNodes.append(curNode);

                // Create a real operation in the PM within the apm
                apm.ops[curNode] = new Operation(curOper);

                // Initialize the scheduling relevant data of the operation
                apm.ops[curNode]->s(0.0);
                apm.ops[curNode]->p(0.0);
                apm.ops[curNode]->toolID = -1;
                apm.ops[curNode]->machID = -1;

                // Set the ID of the operation
                apm.ops[curNode]->ID = ((ord.ID << 16) + nextOperCtr);

                // Set the current index of the operation in the item
                apm.ops[curNode]->stepIdx(j);

                apm.ops[curNode]->ir(Math::max(ord.r, ordman->itemByID(apm.ops[curNode]->itemID()).releaseTime));
                apm.ops[curNode]->r(apm.ops[curNode]->ir());
                apm.ops[curNode]->d(ord.d);
                apm.ops[curNode]->w(ord.w);

                //out << "Appending operation (new ID) " << *apm.ops[curNode] << endl;

                appendedOps++;

                nextOperCtr++;

            }

            // Add the precedence constraints between the operations of the current item
            for (int j = 0; j < curPartGraphNodes.size() - 1; j++) {
                ListDigraph::Node curStartNode = curPartGraphNodes[j];
                ListDigraph::Node curEndNode = curPartGraphNodes[j + 1];
                ListDigraph::Arc curArc = INVALID;

                // Add the arc
                curArc = apm.graph.addArc(curStartNode, curEndNode);

                // Set the arc to be conjunctive
                apm.conjunctive[curArc] = true;

                //out << "Added arc " << apm.ops[curStartNode]->ID << " -> " << apm.ops[curEndNode]->ID << endl;

            }

            // Get the start and the end nodes of the item
            //out << "Current item : " << curItemID << endl;
            //out << "Graph nodes for the item : " << curPartGraphNodes.size() << endl;
            if (curItem.operIDs.size() - curItem.curStepIdx - 1 > 0) { // This item has at least one operation which has not started
                itemID2StartEndNodes[curItemID] = QPair<ListDigraph::Node, ListDigraph::Node> (curPartGraphNodes.first(), curPartGraphNodes.last());
            }

        }

        //out << "Setting precedence constraints between the items ... " << endl;

        // Iterate over the available precedence constraints between the items of the order and add them to the graph
        for (int i = 0; i < ord.itemIDPrececences.size(); i++) {
            const int curStartItemID = ord.itemIDPrececences[i].first;
            const int curEndItemID = ord.itemIDPrececences[i].second;

            // Check whether the last operation of the start item has started. If yes -> continue
            Item& curStartItem = ordman->itemByID(curStartItemID);
            if (curStartItem.operIDs.size() - curStartItem.curStepIdx - 1 == 0) { // This item is performing its last operation -> ignore the precedence constraint
                continue;
            }

            ListDigraph::Node fromNode = itemID2StartEndNodes[curStartItemID].second; // The end node of the start item
            ListDigraph::Node toNode = itemID2StartEndNodes[curEndItemID].first; // The start node of the end item
            ListDigraph::Arc curArc = INVALID;

            //out << "Adding precedence constraint from item " << apm.ops[fromNode]->IID << " to item " << apm.ops[toNode]->IID << endl;
            //out << "Arc " << apm.ops[fromNode]->ID << " to " << apm.ops[toNode]->ID << endl;
            //getchar();

            // Add the corresponding arc
            curArc = apm.graph.addArc(fromNode, toNode);

            // Set the arc to be conjunctive
            apm.conjunctive[curArc] = true;

        }
        //out << "Done setting precedence constraints between the items. " << endl;

        if (appendedOps == 0) { // No operations have been appended (probably all "last" parts are executing their last operations) -> do not integrate this order into the graph

        } else { // There is at least one operation which must be integrated into the graph

            // Add a local head
            ListDigraph::Node pmLHead = apm.graph.addNode(); // Local head for the order
            apm.ops[pmLHead] = new Operation(0.0);
            apm.ops[pmLHead]->ID = -((ord.ID << 16) + nextOperCtr);
            apm.ops[pmLHead]->OID = ord.ID;
            apm.ops[pmLHead]->orderType(-1);
            apm.ops[pmLHead]->bomID(-1);
            apm.ops[pmLHead]->itemID(-1);
            apm.ops[pmLHead]->itemType(-1);
            apm.ops[pmLHead]->routeID(-1);
            apm.ops[pmLHead]->stepIdx(-1);
            apm.ops[pmLHead]->type = 0;
            apm.ops[pmLHead]->ir(ord.r);
            apm.ops[pmLHead]->r(ord.r);
            apm.ops[pmLHead]->d(ord.d);
            apm.ops[pmLHead]->w(ord.w);
            apm.ops[pmLHead]->p(0.0);

            nextOperCtr++;

            // Add a local tail
            ListDigraph::Node pmLTail = apm.graph.addNode(); // Local tail for the order
            apm.ops[pmLTail] = new Operation(0.0);
            apm.ops[pmLTail]->ID = -((ord.ID << 16) + nextOperCtr);
            apm.ops[pmLTail]->OID = ord.ID;
            apm.ops[pmLTail]->orderType(-1);
            apm.ops[pmLTail]->bomID(-1);
            apm.ops[pmLTail]->itemID(-1);
            apm.ops[pmLTail]->itemType(-1);
            apm.ops[pmLTail]->routeID(-1);
            apm.ops[pmLTail]->stepIdx(-1);
            apm.ops[pmLTail]->type = 0;
            apm.ops[pmLTail]->ir(ord.r);
            apm.ops[pmLTail]->r(ord.r);
            apm.ops[pmLTail]->d(ord.d);
            apm.ops[pmLTail]->w(ord.w);
            apm.ops[pmLTail]->p(0.0);

            //out << "Local head operation : " << *apm.ops[pmLHead] << endl;
            //out << "Local tail operation : " << *apm.ops[pmLTail] << endl;

            // Add for possible later deleting of the order from the PM
            ordid2locpm[ord.ID].first = apm.ops[pmLHead]->ID;
            ordid2locpm[ord.ID].second = apm.ops[pmLTail]->ID;

            // For arch item with starting/ending nodes having no incoming/outgoing arc -> connect the nodes with the pmLHead/pmLTail
            for (int i = 0; i < ord.itemIDs.size(); i++) {
                int curItemID = ord.itemIDs[i];

                // Check whether the item is performing its last operation.
                Item& curItem = ordman->itemByID(curItemID);
                if (curItem.operIDs.size() - curItem.curStepIdx - 1 == 0) { // This item is performing its last operation -> ignore it
                    continue;
                }

                ListDigraph::Node curItmStartNode = itemID2StartEndNodes[curItemID].first;
                ListDigraph::Node curItmEndNode = itemID2StartEndNodes[curItemID].second;

                if (countInArcs(apm.graph, curItmStartNode) == 0) { // This node should be connected to the local head
                    ListDigraph::Arc curArc = apm.graph.addArc(pmLHead, curItmStartNode);
                    apm.conjunctive[curArc] = true;
                }

                if (countOutArcs(apm.graph, curItmEndNode) == 0) { // This node should be connected to the local tail
                    ListDigraph::Arc curArc = apm.graph.addArc(curItmEndNode, pmLTail);
                    apm.conjunctive[curArc] = true;
                }

            }

            // Integrate the order subgraph into the PM
            ListDigraph::Arc startOutArc = apm.graph.addArc(apm.head, pmLHead);
            ListDigraph::Arc endInArc = apm.graph.addArc(pmLTail, apm.tail);

            apm.conjunctive[startOutArc] = true;
            apm.conjunctive[endInArc] = true;

            //if (appendedOps == 0) {
            //	ListDigraph::Arc fakeArc = apm.graph.addArc(pmLHead, pmLTail);
            //	apm.conjunctive[fakeArc] = true;
            //}

        }

        //out << "PM after adding a rescheduling order : " << apm << endl;
        //out << "Nodes in the PM : " << countNodes(apm.graph) << endl;

        //getchar();

        //Debugger::err << "ProcessModelManager::insertOrdSubgraph : Inserting orders for SCHED not implemented yet!" << ENDL;

    } // OA_SCHED

}

void ProcessModelManager::updatePM() {

    /**
     * Algorithm:
     * 1. Iterate over the products.
     * 2. If the BOP of the product has been changed:
     * 2. a) Select all orders of the current products type.
     * 2. b) For each of the orders eliminate the corresponding local PM from the global PM. 
     * 2. c) For each of the orders insert the corresponding local PM into the global PM. 
     * 
     */

    QTextStream out(stdout);

    pm.clearSchedRelData();

    //Debugger::wDebug("Updating the process model ...");

    const QList<Product*> &prodlist = prodman->productsList();
    QSet<int> availProdTypes = ordman->availProducts();
    QSet<int> availprods;
    for (int i = 0; i < prodlist.size(); i++) {
        if (availProdTypes.contains(prodlist[i]->type)) {
            availprods.insert(prodlist[i]->ID);
        }
    }

    QList<Order*> curorders;

    /*
        out << "ProcessModelManager::updatePM : prodList : "<<endl;
        for (int i = 0 ; i < prodlist.size() ; i++){
                out << prodlist[i]->ID << ",";
        }
        out<<endl;
        out << "ProcessModelManager::updatePM : available products : "<<endl;
        for (int i = 0 ; i < availprods.toList().size() ; i++){
                out << availprods.toList()[i] << ",";
        }
        out << endl;
     */

    // Iterate over the product list and perform modifications only for the orders whose associated product has changed its BOP.
    //Debugger::info << "Current prodlistsize = " << prodlist.size() << ENDL;
    for (int cp = 0; cp < prodlist.size(); cp++) {
        //out << "cp = " << cp << endl;
        //out << "Product ID : " << prodlist[cp]->ID << endl;
        //out << "Product type : " << prodlist[cp]->type << endl;

        // Continue of the product is not available
        if (!availprods.contains(prodlist[cp]->ID)) continue;

        //Common::Debugger::info << "Current product ID: " << prodlist[cp]->ID << Common::ENDL;
        //Common::Debugger::info << "Current product type: " << prodlist[cp]->type << Common::ENDL;
        //getchar();

        // No changes for the orders of products with the "old" BOPs
        if (prodid2prevbopid.size() != 0) {
            if (!prodid2prevbopid.contains(prodlist[cp]->ID)) {
                Debugger::err << "ProcessModelManager::updatePM : prodid2prevbopid does not contain product " << prodlist[cp]->ID << ENDL;
            }
            if (prodid2prevbopid.value(prodlist[cp]->ID) == prodlist[cp]->bopID) continue;
        }

        // Get the list of orders of the current type
        curorders = ordman->ordersByType(prodlist[cp]->type);
        //out << "Current type = " << prodlist[cp]->type <<endl;
        //Common::Debugger::info << "Current orders: " << curorders.size() << Common::ENDL;

        //out << "Iterating over the orders ..." << endl;
        // Iterate over the orders that correspond to the current product
        for (int co = 0; co < curorders.size(); co++) {

            //Common::Debugger::info << "Current order ID: " << curorders[co]->ID << Common::ENDL;
            //Common::Debugger::info << "Current order type: " << curorders[co]->type << Common::ENDL;
            //getchar();

            //out << "Check/delete the subgraph ..." << endl;
            //if (ordid2locpm.contains(curorders[co]->ID)) { // Delete the subgraph if it is already contained
            //out << pm << endl;
            //out << "deleting order " << curorders[co]->ID << endl;
            //getchar();
            deleteOrdSubgraph(curorders[co]->ID);
            //out << pm << endl;
            //out << "done deleting order " << curorders[co]->ID << endl;
            //getchar();
            //}
            //out << "Done check/delete the subgraph." << endl;


            // Create and insert new subgraph into the global PM
            //out << pm << endl;
            //out << "inserting order " << curorders[co]->ID << endl;
            //getchar();
            insertOrdSubgraph(*prodlist[cp], *curorders[co]);
            //out << pm << endl;
            //out << "done inserting order " << curorders[co]->ID << endl;
            //getchar();
        }

        //out << "Done iterating over the orders." << endl;

    }

    //out << "#####################################################################" << endl << endl;
    //out << pm << endl;

    // Update the IDs of the BOPs in the products according to which the PM has been generated
    prodid2prevbopid.clear();
    for (int cp = 0; cp < prodlist.size(); cp++) {
        prodid2prevbopid[prodlist[cp]->ID] = prodlist[cp]->bopID;
    }

    /*
    Debugger::info << "ProcessModelManager::updatePM : Created operations : " << opcr << ENDL;
    Debugger::info << "ProcessModelManager::updatePM : Deleted operations : " << opdel << ENDL;
    Debugger::info << "ProcessModelManager::updatePM : Lost operations : " << opcr - opdel << ENDL;
    Debugger::info << "ProcessModelManager::updatePM : freeIDs size : " << freeIDs.size() << ENDL;
    Debugger::info << "ProcessModelManager::updatePM : prodid2prevbopid size : " << prodid2prevbopid.size() << ENDL;
    Debugger::info << "ProcessModelManager::updatePM : ordid2locpm size : " << ordid2locpm.size() << ENDL;
     */
    //Debugger::wDebug("Done updating the process model.");
}

ProcessModel ProcessModelManager::plan2PM(const Plan& plan) {
    ProcessModel res;

    /**
     * Algorithm:
     * 1. Iterate over the products.
     * 2. For the current product:
     * 2. a) Select all orders of the current products type.
     * 2. b) For each of the orders insert the corresponding local PM into the global PM. 
     * 
     */

    QTextStream out(stdout);

    const QList<Product*> &prodlist = prodman->productsList();
    QSet<int> availprods = ordman->availProducts();
    QList<Order*> curorders;

    // Restore the state of the products based on the current plan
    prodman->bopsFromPlan(plan);

    res.clear();

    // Create tail and head
    res.head = res.graph.addNode();
    res.ops[pm.head] = new Operation(0.0);
    res.ops[pm.head]->ID = -(maxfreeIDs + 2); //-1;
    res.ops[pm.head]->itemID(-1);
    res.ops[pm.head]->type = 0;
    res.ops[pm.head]->w(0.0);
    res.ops[pm.head]->ir(0.0);
    res.ops[pm.head]->r(0.0);
    res.ops[pm.head]->d(double(Math::MAX_DOUBLE));

    res.tail = res.graph.addNode();
    res.ops[pm.tail] = new Operation(0.0);
    res.ops[pm.tail]->ID = -(maxfreeIDs + 1); //-2;
    res.ops[pm.tail]->itemID(-1);
    res.ops[pm.tail]->type = 0;
    res.ops[pm.tail]->w(0.0);
    res.ops[pm.tail]->ir(0.0);
    res.ops[pm.tail]->r(0.0);
    res.ops[pm.tail]->d(double(Math::MAX_DOUBLE));

    //out << "Available products : ";
    //for(int i = 0 ; i < availprods.size() ; i++) out << availprods.toList()[i] << ",";
    //out << endl;

    // Iterate over the product list and perform modifications only for the orders whose associated product has changed its BOP.
    for (int cp = 0; cp < prodlist.size(); cp++) {
        // Continue of the product is not available
        if (!availprods.contains(prodlist[cp]->ID)) continue;

        // Get the list of orders of the current type
        curorders = ordman->ordersByType(prodlist[cp]->type);

        //out << "Current product : " << prodlist[cp]->ID << endl;
        //out << "Orders for the product : ";
        //for(int i = 0 ; i < curorders.size() ; i++) out << curorders[i]->ID << ":"<< curorders[i]->type << ",";
        //out << endl;

        // Iterate over the orders that correspond to the current product
        for (int co = 0; co < curorders.size(); co++) {
            //Debugger::info << "Inserting the subgraph..." << ENDL;
            //out << "Inserting subgraph for order : " << curorders[co]->ID << endl;
            insertOrdSubgraph(*prodlist[cp], *curorders[co], res);
            //Debugger::info << "Done inserting the subgraph." << ENDL;
        }

    }

    // Push back the available operation IDs
    for (ListDigraph::NodeIt nit(res.graph); nit != INVALID; ++nit) {

        if (nit != res.head && nit != res.tail) {
            //			freeIDs.push(Math::abs(res.ops[nit]->ID));
        }

    }

    return res;
}
