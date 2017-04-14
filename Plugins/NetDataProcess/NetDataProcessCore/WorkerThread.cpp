#include "WorkerThread.h"
#include "NetDataManager.h"
#include <Core/NetworkService/NetworkService.h>

using namespace NetDataProcessCore;

WorkerThread::WorkerThread(const QString &workername, QObject *parent)
    : QThread(parent)
    , _workername(workername)
{
    setObjectName("NetData WorkerThread:" + workername);
}

void WorkerThread::run()
{
    _worker = NETDATA_MANAGER->createWorker(_workername, 0);
    if(!_worker)
    {
        qCCritical(netDataProcess) << "Create worker failed: " << _workername;
        return;
    }


}
