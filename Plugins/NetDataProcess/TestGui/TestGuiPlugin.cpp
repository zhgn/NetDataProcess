#include "TestGuiPlugin.h"
#include "TestGuiConstants.h"
#include "TestWidget.h"
#include <QtWidgets>
#include <Core/ConfigManager/ConfigManager.h>
#include <Core/CrossPlatform/CrossPlatform.h>

INSTANCE_CPP(TestGui::TestGuiPlugin)

Q_LOGGING_CATEGORY(testgui, "NetDataProcess.TestGui")

namespace TestGui
{

    TestGuiPlugin::TestGuiPlugin()
        : m_testWidget(0)
    {
    }

    TestGuiPlugin::~TestGuiPlugin()
    {

    }

    bool TestGuiPlugin::initialize(const QStringList &arguments, QString *errorString)
    {
        Q_UNUSED(arguments)
        Q_UNUSED(errorString)

        return true;
    }

    void TestGuiPlugin::extensionsInitialized()
    {

        bool bUseSysTrayIcon = CONFIG_GETVALUE2(PLUGIN_NAME, PLUGIN_NAME, "UseSysTrayIcon", false).toBool();
        if(bUseSysTrayIcon)
        {
            qApp->setQuitOnLastWindowClosed(false);
            QMenu *menu = new QMenu;
            menu->addAction(tr("Show TestWidget"), this, SLOT(showTestWidget()));
            menu->addAction(tr("Quit"), qApp, SLOT(quit()));

            QSystemTrayIcon *tray = new QSystemTrayIcon(this);
            tray->setIcon(qApp->windowIcon());
            tray->setContextMenu(menu);
            tray->show();
        }
        else
        {
            showTestWidget();
        }
    }

    PluginSystem::IPlugin::ShutdownFlag TestGuiPlugin::aboutToShutdown()
    {
        SAVE_DELETE(m_testWidget);
        return SynchronousShutdown;
    }

    bool TestGuiPlugin::canHotPlug()
    {
        return false;
    }

    void TestGuiPlugin::showTestWidget()
    {
        TestWidget *widget = new TestWidget;
        widget->setAttribute(Qt::WA_DeleteOnClose, true);
        widget->show();
    }

}
