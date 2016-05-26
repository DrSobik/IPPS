
#include <QtCore/QCoreApplication>
#include <QTextStream>

#include "PlanSchedClientDLLInterface.h"

// In case the program is compiled as an executable
int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    QTextStream out(stdout);

    QHostAddress hostAddress = QHostAddress::LocalHost;
    qint32 hostPort = 5555;
    QByteArray message;

    //hostAddress.setAddress("132.176.74.60"); // IP of LABOR 1
    //hostAddress.setAddress("132.176.74.101"); // IP my laptop at the university
    //hostAddress.setAddress("176.198.54.196"); // IP of my home router (it does port forwarding)
    //hostAddress.setAddress("10.0.2.15"); //
    //hostAddress.setAddress("37.201.195.224"); // IP of my home router (it does port forwarding)
    //hostAddress.setAddress("37.201.192.228");
    //hostAddress.setAddress("84.116.198.88"); // IP of my home router (it does port forwarding)
    //hostAddress.setAddress("192.168.0.10"); // IP of my home router (it does port forwarding)
    //hostAddress.setAddress("2a02:908:f61b:1400:3a59:f9ff:fe1a:a8b5"); // IP of my home router (it does port forwarding)

    message = "Hello";

    qDebug() << "Sending message: " << message << endl;
    solve(hostAddress, hostPort, message);
    qDebug() << "Received message: " << message << endl;

    return 0;
}
