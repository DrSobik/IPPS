/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FlowFactor.cpp
 * Author: DrSobik
 * 
 * Created on 5 травня 2016, 21:02
 */

#include "FlowFactor.h"

FlowFactor::FlowFactor() {
}

FlowFactor::FlowFactor(const FlowFactor&) : Functor() {
}

FlowFactor::~FlowFactor() {
}

double FlowFactor::operator()(const ProcessModel& somePM) {

    QTextStream out(stdout);

    // Calculate the FF based on the pm
    ProcessModel pm = somePM;

    // The average cycle time
    double averageCycleTime = 0.0;

    QList<ListDigraph::Node> terminals = pm.terminals();

    for (int i = 0; i < terminals.size(); i++) {
        averageCycleTime += pm.ops[terminals[i]]->c();

        // In case there are release times of the orders
        //out << "Scheduler::flowFactor : release time : " << pm.ops[terminals[i]]->ir() << endl;
        averageCycleTime -= pm.ops[terminals[i]]->ir();
	
	// What about incomplete orders?
    }

    //out << pm << endl;
    //getchar();

    averageCycleTime /= (terminals.size() > 0) ? double(terminals.size()) : 1.0;

    double rawProcTime = 0.0;
    double curRPT = 0.0;

    ListDigraph::NodeMap<double> pRemain(pm.graph);
    ListDigraph::NodeMap<int> lenRemain(pm.graph);
    QList<ListDigraph::Node> ordNodes; // Nodes in the current order


    for (int i = 0; i < terminals.size(); i++) {
        //QStack<ListDigraph::Node> stack;
        QQueue<ListDigraph::Node> q;
        ListDigraph::Node curnode = INVALID;
        ListDigraph::Node curpred = INVALID;
        ListDigraph::Node cursucc = INVALID;

        lenRemain[terminals[i]] = 1;
        pRemain[terminals[i]] = 0.0;

        for (ListDigraph::InArcIt iait(pm.graph, terminals[i]); iait != INVALID; ++iait) {
            if (pm.conjunctive[iait]) { // Consider only conjunctive arcs
                curpred = pm.graph.source(iait);
                //stack.push(pm.graph.source(iait));
                q.enqueue(curpred);

                lenRemain[curpred] = 2;
                pRemain[curpred] = pm.ops[curpred]->p() + 0.0; // 0.0 - for the terminal node

                //Debugger::info << "Terminal : " << pm.ops[terminals[i]]->ID << ENDL;
                //Debugger::info << pm.ops[curpred]->ID << " : " << d[curpred] << ENDL;
                //getchar();
            }
        }

        ordNodes.clear();
        while (/*!stack.empty()*/!q.empty()) {
            //curnode = stack.pop();
            curnode = q.dequeue();

            // Save the node 
            ordNodes.append(curnode);

            // Find the largest number of the operations to be processed after the current node (including the current node)
            ListDigraph::Node longerSucc = INVALID;
            double maxP = -1.0;
            for (ListDigraph::OutArcIt oait(pm.graph, curnode); oait != INVALID; ++oait) {
                if (pm.conjunctive[oait]) {
                    if (pRemain[pm.graph.target(oait)] > maxP) {
                        maxP = pRemain[pm.graph.target(oait)];
                        longerSucc = pm.graph.target(oait);
                    }
                }
            }
            //Debugger::info << "Found : " << pm.ops[longerSucc]->ID << ENDL;
            //getchar();
            if (longerSucc == INVALID) {
                Debugger::err << "ATCANScheduler::preparePM : Failed to find successor with the largest remaining processing time!!!" << ENDL;
            }
            pRemain[curnode] = pRemain[longerSucc] + pm.ops[curnode]->p();
            lenRemain[curnode] = lenRemain[longerSucc] + 1;

            //out << "Dequed : " << pm.ops[curnode]->ID << endl;
            //out << "pRemain : " << pRemain[curnode] << endl;

            //Debugger::info << pm.ops[longerSucc]->ID << " : " << d[longerSucc] << ENDL;
            //getchar();

            // Consider the direct predecessors
            for (ListDigraph::InArcIt iait(pm.graph, curnode); iait != INVALID; ++iait) {
                if (pm.conjunctive[iait]) {
                    curpred = pm.graph.source(iait);

                    // Push the current predecessor into the queue
                    q.enqueue(curpred);
                }
            }
        }

        curnode = INVALID;
        // Find the dummy start node of the current order
        for (ListDigraph::OutArcIt oait(pm.graph, pm.head); oait != INVALID; ++oait) {
            if (pm.ops[pm.graph.target(oait)]->OID == pm.ops[terminals[i]]->OID) {
                curnode = pm.graph.target(oait);
            }
        }

        if (curnode == INVALID) {
            out << "Current order : " << pm.ops[terminals[i]]->OID << endl;
            out << pm << endl;
            Debugger::err << "Scheduler::FF : Failed to find a start node of the current order!!!" << ENDL;
        }

        curRPT = pRemain[curnode];

        rawProcTime += curRPT;

        if (Math::cmp(pm.ops[pm.terminals()[i]]->c(), curRPT, 0.0001) == -1) {
            out << " Cycle time < RPT!!! " << pm.ops[pm.terminals()[i]]->c() << " < " << curRPT << endl;
            out << "Terminal : " << pm.ops[pm.terminals()[i]]->ID << endl;
            out << pm << endl;
            Debugger::err << "Scheduler::FF : RPT > C " << endl;
        }

        //getchar();
    }

    rawProcTime /= (terminals.size() > 0) ? double(terminals.size()) : 1.0;

    return averageCycleTime / rawProcTime;

}