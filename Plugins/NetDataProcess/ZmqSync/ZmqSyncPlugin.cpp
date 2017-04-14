#include "ZmqSyncPlugin.h"
#include "ZmqSyncConstants.h"
#include "ZmqSyncWorker.h"
#include <NetDataProcessCore/NetDataManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <Core/ConfigManager/ConfigManager.h>
#include <PluginSystem/pluginmanager.h>
INSTANCE_CPP(ZmqSync::ZmqSyncPlugin)

namespace ZmqSync
{

    ZmqSyncPlugin::ZmqSyncPlugin()
    {
    }

    ZmqSyncPlugin::~ZmqSyncPlugin()
    {
    }

    bool ZmqSyncPlugin::initialize(const QStringList &arguments, QString *errorString)
    {
        Q_UNUSED(arguments)
        Q_UNUSED(errorString)

        //获取所有的group, group内配置了 Topics, ServerUrl
        QSettings *settings = CONFIG_MANAGER->getQSettings(PLUGIN_NAME);
        PSFW_ASSERT(settings, return false);

        QStringList groups = settings->childGroups();
        groups.removeAll(PLUGIN_NAME); //移除总的配制组

        //根据Group的数量, 添加所有的worker
        for(QString group : groups)
        {
            //获取 Topics 支持单个或者多个
            QStringList topics;
            QVariant varTopics = settings->value(group%"/Topics");
            if(varTopics.type() == QVariant::StringList)
            {
                topics = varTopics.toStringList();
            }
            else if(varTopics.type() == QVariant::String)
            {
                topics.append(varTopics.toString() );
            }
            else
            {
                continue;
            }
            //远端的 zmq 地址
            QString url = settings->value(group%"/ServerUrl").toString();
            //Package 定义
            QString packageDefineGroup = settings->value(group%"/PackageDefine", QString()).toString();

            //在线程中运行，只需要一个实例
            ZmqSyncWorker *worker = new ZmqSyncWorker(NETWORK_SERVICE->getZmqContext());
            worker->setTopics(topics);
            worker->setUrl(url);
            if(!packageDefineGroup.isEmpty())
                worker->setPackageDefine(packageDefineGroup);
            //连接必要的信号槽
            connect(NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::startSyncWorker,
                    worker, &ZmqSyncWorker::startWorker);
            connect(NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::stop,
                    worker, &ZmqSyncWorker::stopWorker);

            qCDebug(zmqSync) << "ZmqSyncWorker have been created, Topics: " << topics
                             << "; ServerUrl: " <<url;
        }

        return true;
    }

    void ZmqSyncPlugin::extensionsInitialized()
    {
    }

    PluginSystem::IPlugin::ShutdownFlag ZmqSyncPlugin::aboutToShutdown()
    {
        return SynchronousShutdown;
    }

    bool ZmqSyncPlugin::canHotPlug()
    {
        return false;
    }

}
