#ifndef ZMQWORKERS_H
#define ZMQWORKERS_H

#include "ZmqWorkers_Global.h"

#include <PluginSystem/iplugin.h>
#include <PSFW_Global.h>

namespace ZmqWorkers
{

class ZmqWorkersPlugin : public PluginSystem::IPlugin
{
    Q_OBJECT

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.PSFW.Plugin" FILE "ZmqWorkers.json")
#endif
    INSTANCE(ZmqWorkersPlugin)
    public:
        ZmqWorkersPlugin();
    ~ZmqWorkersPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    bool canHotPlug();
};

} // namespace ZmqWorkers

#endif // ZMQWORKERS_H
