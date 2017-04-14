#ifndef TESTGUI_H
#define TESTGUI_H

#include "TestGui_Global.h"

#include <PluginSystem/iplugin.h>
#include <PSFW_Global.h>

class TestWidget;

namespace TestGui
{

class TestGuiPlugin : public PluginSystem::IPlugin
{
    Q_OBJECT

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.PSFW.Plugin" FILE "TestGui.json")
#endif
    INSTANCE(TestGuiPlugin)
    public:
        TestGuiPlugin();
    ~TestGuiPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    bool canHotPlug();
public slots:
    void showTestWidget();

private:
    TestWidget *m_testWidget;
};

} // namespace TestGui

#endif // TESTGUI_H
