#include "SchedServer.h"

SchedServer::SchedServer(){
    connect(this, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
}

SchedServer::SchedServer(const SchedServer&) : QTcpServer(nullptr){

}

SchedServer::~SchedServer(){

}

void SchedServer::incomingConnection(){
    qDebug() << "Incoming connection.";

    socket = nextPendingConnection();

    connect(socket, SIGNAL(readyRead()) ,this, SLOT(messageReceived()));
}

void SchedServer::messageReceived(){
    QTextStream out(stdout);

    /*******************************  Read the message  ***************************************************************/

    //out << "Incoming connection." << endl;

    //socket = nextPendingConnection();

    //connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));
    //connect(socket, SIGNAL(disconnected()), this, SLOT(closedConnection()));

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
    QByteArray inMessage;

    inMessage = socket->read(messageSize);

    QMutex mutex;

    mutex.lock();

    out << "Receiving message of " << messageSize << " bytes" << endl;

    out << endl << inMessage << endl;

    mutex.unlock();

    //getchar();

    /******************************************************************************************************************/

    /*****************************************  Reply   ***************************************************************/

    // Create a reply message
    QByteArray outMessage = "Hello from PlanSchedServer!";

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
        //Debugger::err << "Failed to disconnect socket!" << ENDL;
        out << "Failed to disconnect socket!" << endl;
    }

    /******************************************************************************************************************/
}
