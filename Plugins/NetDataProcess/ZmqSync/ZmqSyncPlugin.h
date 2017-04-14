#ifndef ZMQSYNC_H
#define ZMQSYNC_H

#include "ZmqSync_Global.h"

#include <PluginSystem/iplugin.h>
#include <PSFW_Global.h>

namespace ZmqSync
{

class ZmqSyncPlugin : public PluginSystem::IPlugin
{
    Q_OBJECT

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.PSFW.Plugin" FILE "ZmqSync.json")
#endif
    INSTANCE(ZmqSyncPlugin)
    public:
        ZmqSyncPlugin();
    ~ZmqSyncPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    bool canHotPlug();
};

} // namespace ZmqSync

#endif // ZMQSYNC_H
