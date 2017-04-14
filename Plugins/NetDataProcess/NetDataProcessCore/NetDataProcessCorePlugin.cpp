#include "NetDataProcessCorePlugin.h"
#include "NetDataProcessCoreConstants.h"
#include "NetDataManager.h"
#include <Core/Redis/RedisClient.h>

INSTANCE_CPP(NetDataProcessCore::NetDataProcessCorePlugin)

using namespace NetDataProcessCore;

NetDataProcessCorePlugin::NetDataProcessCorePlugin()
{
}

NetDataProcessCorePlugin::~NetDataProcessCorePlugin()
{
    NetDataManager::destroySelf();
}

bool NetDataProcessCorePlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // 初始化 NetDataManager 等待插件添加 工厂类
    NETDATA_MANAGER->init();

    //手动启动 或者 通过插件组的配置文件指定为 自动启动
//    Core::RedisClient::startRedisServer();

    return true;
}

void NetDataProcessCorePlugin::extensionsInitialized()
{
    //NetDataManager 初始化
    NETDATA_MANAGER->extensionsInitialized();
}

PluginSystem::IPlugin::ShutdownFlag NetDataProcessCorePlugin::aboutToShutdown()
{
    NETDATA_MANAGER->stopWorkers();
    return SynchronousShutdown;
}

bool NetDataProcessCorePlugin::canHotPlug()
{
    return false;
}
