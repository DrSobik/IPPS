/* 
 * File:   PlanSchedServer.h
 * Author: DrSobik
 *
 * Created on August 2, 2013, 12:52 PM
 */

#ifndef PLANSCHEDSERVER_H
#define PLANSCHEDSERVER_H

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QList>
#include <QDataStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QWaitCondition>
#include <QMutex>

#include "MathExt"
#include "SmartPointer"
#include "Loader"
#include "Parser"

#include "DebugExt.h"

#include "IPPSDefinitions"

#include "Resources"
#include "Route"
#include "BillOfMaterials"
#include "BillOfProcesses"
#include "Order"
#include "Product"
#include "ProcessModelManager"
#include "ProcessModel"
#include "Schedule"
#include "Scheduler"
#include "Plan"
#include "Planner"
#include "Protocol"

using namespace Common;
using namespace Common::SmartPointers;

class PlanSchedServer : public QTcpServer, public Parser<void, const Settings&> {
    Q_OBJECT
private:
    QTcpSocket* socket; // Socket for communication over TCP/IP

    QByteArray inMessage; // Message that is received
    QByteArray outMessage; // Message that is sent

    Resources rc; // Resources of the production system

    QHash<int, QList<Route*> > itype2Routes; // Routes for different item/part types

    QHash<int, QList<BillOfMaterials*> > ptype2Boms; // <product_type, bom_list>

    QHash<int, QList<BillOfProcesses*> > ptype2Bops; // <product_type, possible_BOPs>

    QList<Product> products; // The products to consider

    ProductManager prodman; // The product manager

    OrderManager ordman; // Order manager

    ProcessModelManager pmm; // Process Model Manager

    QHash<QString, QString> options; // Options describing the behaviour of the planner/scheduler

    //VNSPlanner solver; // The planning algorithm
    SmartPointer<PlanSchedSolver> solver;

    //Scheduler*  scheduler; // The scheduler to use
    Schedule sched; // The schedule

    Protocol protocol;
    QFile protoFile; // File to store the protocol

    QHash<int, int> origOrdID2OrigOrdType; // Used for restoring original input data for orders with incomplete products
    QHash<int, int> origOrdID2OrigOrdBOM; // Used for restoring original input data for orders with incomplete products

    Settings settings;
    
    IPPSProblem ippsProblem;
    
public:

    PlanSchedServer();

    PlanSchedServer(const PlanSchedServer&);

    virtual ~PlanSchedServer();

    /** Clears all data in the server. */
    virtual void clear();

    void readResources(const QByteArray& message);

    void readRoutes(const QByteArray& message);

    void readProducts(const QByteArray& message);

    void readOperationsAndPartsAndOrders(const QByteArray& message);

    void createProducts();

    void createOrders();

    /** Create the process model manager. */
    void createPMM();

    void createIncompleteProducts(); // Creates incomplete products for orders which are already being processed

    void prepareForPlanningAndScheduling();

    /** Reconstruct parts and orders based on the given PM. */
    void constructPartsAndOrders(ProcessModel& pm);

    /** Parse settings */
    void parse(const Settings&) override;


public slots:

    /** Synchronous incoming connection handling */
    void incomingConnection();

    /** Called when a connection is finished */
    void closedConnection();

    /** Error handling */
    void connectionError(QAbstractSocket::SocketError);

    /** Called whenever the schedule computation is finished. */
    void computationFinished();

};


#endif /* PLANSCHEDSERVER_H */

