#include <QCoreApplication>
#include <QTextStream>
#include <QString>

#include <iostream>
#include <stdio.h>

#include "SchedServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QTextStream out(stdout);

    //qDebug() << QString(a.applicationDirPath()+"/test.txt").toAscii();

    SchedServer server;

    server.setObjectName("theSchedServer");

    QHostAddress host = QHostAddress::LocalHost;

    //host.setAddress("132.176.74.60");
    //host.setAddress("132.176.74.101");
    //host.setAddress("10.0.2.15");

    if (server.listen(host, 5555)){
        //qDebug() << "Started " << server.objectName();
        out << "Started " << server.objectName() << endl;
    }else{
        qDebug() << "Failed to start " << server.objectName();
        //out << "Failed to start " << server.objectName() << endl;
        a.exit();
    }

    return a.exec();
}
