#ifndef SCHEDCLIENTDLLINTERFACE_H
#define SCHEDCLIENTDLLINTERFACE_H

#include <QtCore>
#include <QtGlobal>
#include <QtNetwork>

#include "PlanSchedClientDefinitions.h"

// Entry point which can be called out of the DLL
extern "C" SCHEDCLIENTSHARED_EXPORT void solve(const QHostAddress&, const qint32&, QByteArray&);
#endif // SCHEDCLIENTDLLINTERFACE_H
