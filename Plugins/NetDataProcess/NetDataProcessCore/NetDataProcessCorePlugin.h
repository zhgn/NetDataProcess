#ifndef NETDATAPROCESSCORE_H
#define NETDATAPROCESSCORE_H

#include <QtWidgets>
#include <PluginSystem/iplugin.h>
#include <PSFW_Global.h>
#include <NetDataProcessCore/NetDataProcessCore_Global.h>

#define NETDATA_CORE NetDataProcessCore::NetDataProcessCorePlugin::instance()

namespace NetDataProcessCore
{

class NETDP_EXPORT NetDataProcessCorePlugin : public PluginSystem::IPlugin
{
    Q_OBJECT

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.PSFW.Plugin" FILE "NetDataProcessCore.json")
#endif
    INSTANCE(NetDataProcessCorePlugin)
    public:
        NetDataProcessCorePlugin();
    ~NetDataProcessCorePlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    bool canHotPlug();
};

} // namespace NetDataProcessCore

#endif // NETDATAPROCESSCORE_H

