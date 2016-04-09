#ifndef SCHEDCLIENT_H
#define SCHEDCLIENT_H


#include <QDataStream>
#include <QTextStream>
#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QCoreApplication>
#include <QByteArray>


class SchedClient : public QObject{
    Q_OBJECT

private:

    QTcpSocket socket; // Socket for communication over TCP/IP
    QByteArray message; // For exchanging messages

public:

    SchedClient();

    SchedClient(const SchedClient&);

    virtual ~SchedClient();

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
