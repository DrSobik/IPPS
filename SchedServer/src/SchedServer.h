#ifndef SCHEDSERVER_H
#define SCHEDSERVER_H

#include <QTextStream>
#include <QDataStream>

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <QWaitCondition>
#include <QMutex>

class SchedServer : public QTcpServer{
    Q_OBJECT
private:

    QTcpSocket* socket;

public:

    SchedServer();

    SchedServer(const SchedServer&);

    virtual ~SchedServer();

public slots:

    void incomingConnection();

    void messageReceived();
};

#endif // SCHEDSERVER_H
