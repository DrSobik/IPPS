/* 
 * File:   VNSServer.h
 * Author: DrSobik
 *
 * Created on May 31, 2011, 4:00 PM
 * 
 * Description: Planner class is used to run VNS. 
 * 
 */

#ifndef IPPS_PLANNERAGENT_H
#define	IPPS_PLANNERAGENT_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

//#include "Plan.h"
//#include "VNS.h"

class PlannerAgent : public QObject {
	Q_OBJECT
private:
	QTcpSocket prodagsocket; // Socket for data transfer between planner and scheduler
	QTcpSocket schedagsocket; // Socket for data transfer between product agent(s) and the planner
	QTimer timer;

public:
	QTcpServer prodaglistener; // Server to listen the product agent(s)
	QTcpServer schedlistener; // Server to listen the scheduler agent

	//VNSProblem vnsproblem;

	PlannerAgent();
	PlannerAgent(const PlannerAgent& orig);
	virtual ~PlannerAgent();

	void connectTo(const QHostAddress &host, qint32 port) {
		connect(&schedagsocket, SIGNAL(hostFound()), this, SLOT(hostFound()));
		connect(&schedagsocket, SIGNAL(connected()), this, SLOT(connectionEstablished()));
		connect(&schedagsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));

		schedagsocket.setObjectName("Planner-Scheduler Socket");

		schedagsocket.connectToHost(host, port);


		connect(&timer, SIGNAL(timeout()), this, SLOT(sendMessage()));
		timer.start(1000);
	}


public slots:

	void hostFound() {
		qDebug() << "Host located:" << ((QTcpSocket*) sender())->peerAddress().toString();
	}

	void connectionEstablished() {
		qDebug() << "Connection established with: " << ((QTcpSocket*) sender())->peerAddress().toString() << ":" << ((QTcpSocket*) sender())->peerPort();
	}

	void connectionError(QAbstractSocket::SocketError error) {
		qDebug() << "Connection error : " << error << " " << ((QTcpSocket*) sender())->errorString();
	}

	void sendMessage() {
		if (!schedagsocket.isOpen()) {
			qDebug("Socket is not open!");
			return;
		}
		qDebug() << "Sending message ...";

		int bw = schedagsocket.write(QByteArray("Hello from Planner!"));

		if (schedagsocket.flush()) {
			qDebug() << "Message sent: " << bw << "bytes";
		}
	}

};

#endif	/* IPPS_PLANNERAGENT_H */

