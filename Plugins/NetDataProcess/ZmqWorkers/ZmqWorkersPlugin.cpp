#include "ZmqWorkersPlugin.h"
#include "ZmqWorkersConstants.h"
#include "ZmqWorker.h"
#include <NetDataProcessCore/NetDataManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <PluginSystem/pluginmanager.h>
INSTANCE_CPP(ZmqWorkers::ZmqWorkersPlugin)

namespace ZmqWorkers
{

    ZmqWorkersPlugin::ZmqWorkersPlugin()
    {
    }

    ZmqWorkersPlugin::~ZmqWorkersPlugin()
    {
    }

    bool ZmqWorkersPlugin::initialize(const QStringList &arguments, QString *errorString)
    {
        Q_UNUSED(arguments)
        Q_UNUSED(errorString)

        //在线程中运行，暂定4个实例
        for(int i = 0; i < 1; ++i)
        {
            ZmqWorker *worker = new ZmqWorker(NETWORK_SERVICE->getZmqContext());
            //连接必要的信号槽
            connect(NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::startUnsyncWorker,
                    worker, &ZmqWorker::startWorker);
            connect(NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::stop,
                    worker, &ZmqWorker::stopWorker);
        }

        return true;
    }

    void ZmqWorkersPlugin::extensionsInitialized()
    {
    }

    PluginSystem::IPlugin::ShutdownFlag ZmqWorkersPlugin::aboutToShutdown()
    {
        return SynchronousShutdown;
    }

    bool ZmqWorkersPlugin::canHotPlug()
    {
        return false;
    }

}
