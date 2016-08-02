
#include <QtCore/QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <qt5/QtCore/qiodevice.h>

#include "SchedClientDLLInterface.h"

// In case the program is compiled as an executable
int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    QTextStream out(stdout);

    QHostAddress hostAddress = QHostAddress::LocalHost;
    qint32 hostPort = 5555;
    QByteArray message;
    QFile inputFile("/home/DrSobik/Projects/IPPS/CPPALG/SchedClient/input.txt");

    //hostAddress.setAddress("132.176.74.60"); // IP of LABOR 1
    //hostAddress.setAddress("132.176.74.101"); // IP my laptop at the university
    //hostAddress.setAddress("176.198.54.196"); // IP of my home router (it does port forwarding)
    //hostAddress.setAddress("10.0.2.15"); //

    //message = "Hello";
    
    inputFile.open(QIODevice::ReadOnly);
    message = inputFile.readAll();
    inputFile.close();

    qDebug() << "Sending message: " << message << endl;
    getSched(hostAddress, hostPort, message);
    qDebug() << "Received message: " << message << endl;

    return 0;
}
