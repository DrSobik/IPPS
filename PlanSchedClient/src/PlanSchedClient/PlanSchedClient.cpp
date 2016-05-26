
#include "PlanSchedClientDefinitions.h"
#include "PlanSchedClient.h"

PlanSchedClient::PlanSchedClient() : QObject(0), socket(this) {
    connect(&socket, SIGNAL(hostFound()), this, SLOT(hostFound()));
    connect(&socket, SIGNAL(connected()), this, SLOT(connectionEstablished()));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));

    this->setObjectName("theClient");
    socket.setObjectName("theClientSocket");
}

PlanSchedClient::PlanSchedClient(const PlanSchedClient&) : QObject(0), socket(this){

}

PlanSchedClient::~PlanSchedClient(){

}

void PlanSchedClient::connectTo(const QHostAddress& host, qint32 port){
    //QTextStream out(qDebug);

    socket.connectToHost(host, port); // Send connections request

    if(!socket.waitForConnected(30000)){ // Wait until the connection is established
        qDebug() << "SchedClient::connectTo : Failed connection : " << host.toString() << ":" << port << " error: " << socket.errorString() << endl;
    }
}

void PlanSchedClient::hostFound(){

}

void PlanSchedClient::connectionEstablished(){

}

void PlanSchedClient::connectionError(QAbstractSocket::SocketError){

}

void PlanSchedClient::sendMessage(QByteArray& message){
    //QTextStream out(stdout);

    if (!socket.isOpen()){
        qDebug() << "Socket is not open!" << endl;
        return;
    }

    quint64 messageSize = message.size();

    // Copy it byte wise
    for (uint i = 0 ; i < sizeof(quint64); i++){
        message.prepend(((uchar*)&messageSize)[i]);
    }

    int bw = socket.write(message);

    socket.waitForBytesWritten(-1);

    qDebug() << "Sent " << bw << " bytes" << endl;

}

void PlanSchedClient::waitForReply(){
    qDebug("SchedClient is waiting ... ");

    socket.waitForReadyRead(-1);

    // Wait for the message size
    while(socket.bytesAvailable() < (qint64) sizeof(quint64)){
        if(!socket.waitForReadyRead(-1)){

        }
    }

    quint64 msgSize;
    QDataStream in(&socket);

    // Read the size of the message
    in >> msgSize;

    while(socket.bytesAvailable() < (qint64) msgSize){
        if(!socket.waitForReadyRead(-1)){

        }
    }

    message = socket.read(msgSize);

    socket.disconnectFromHost();

    //qDebug() << "SchedClient received a reply : " << endl << message << endl;
}

QByteArray PlanSchedClient::getMessage(){
    return message;
}

