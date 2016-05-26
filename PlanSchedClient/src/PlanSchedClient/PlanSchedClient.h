#ifndef SCHEDCLIENT_H
#define SCHEDCLIENT_H


#include <QtCore>
#include <QtGlobal>
#include <QtNetwork>

#include <iostream>

class PlanSchedClient : public QObject{
    Q_OBJECT

private:

    QTcpSocket socket; // Socket for communication over TCP/IP
    QByteArray message; // For exchanging messages

public:

    PlanSchedClient();

    PlanSchedClient(const PlanSchedClient&);

    virtual ~PlanSchedClient();

    void connectTo(const QHostAddress& host, qint32 port);

    QByteArray getMessage(); // Current message that the client holds

public slots:

    void hostFound();

    void connectionEstablished();

    void connectionError(QAbstractSocket::SocketError);

    void sendMessage(QByteArray& message); // Asynchronious message sending to the server

    void waitForReply(); // Waits until the server replies. Performs synchrinization
};



#endif // SCHEDCLIENT_H
