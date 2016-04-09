#include "SchedClientDLLInterface.h"

#include "SchedClient.h"

// Entry point which can be called out of the DLL
void getSched(const QHostAddress& hostAddress, const qint32& hostPort, QByteArray& message){
    //QTextStream out(stdout);

    //out << "Hello from SchedClient" << endl;

    SchedClient client;

    client.connectTo(hostAddress, hostPort);

    qDebug() << hostAddress.toString() << ":" << QString::number(hostPort) << endl;

    client.sendMessage(message); // Send the data to the Scheduler

    client.waitForReply(); // Non-busy waiting until the result comes

    message = client.getMessage(); // Return the obtained message

}
