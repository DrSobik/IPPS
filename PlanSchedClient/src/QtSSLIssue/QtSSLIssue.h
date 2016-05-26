#ifndef QTSSLISSUE_H
#define QTSSLISSUE_H

#include "PlanSchedClientDefinitions.h"

/* This is a temporary solution to avoid linking errors against SSL since SSLv2 was banned from SSL but Qt still seemt to refer to it (wrong configuraiton for MinGW64).*/
extern "C"  SCHEDCLIENTSHARED_EXPORT void SSLv2_client_method(void);
extern "C"  SCHEDCLIENTSHARED_EXPORT void SSLv2_server_method(void);

#endif // QTSSLISSUE_H
