DEFINES += NETDATAPROCESSCORE_LIBRARY
PLUGIN_VERSION = 1.0

include(../../../../Include/Plugin.pri)
QMAKE_EXTRA_TARGETS -= BBPY_Include
POST_TARGETDEPS -= BBPY_Include
include($$PSFW_3RDPARTYPATH/ZeroMQ/zmq.pri)
QT += concurrent

# NetDataProcessCore files

SOURCES += NetDataProcessCorePlugin.cpp \
    NetDataManager.cpp \
    NetDataManager_p.cpp \
    IZmqWorker.cpp \
    WorkerThread.cpp

HEADERS += NetDataProcessCorePlugin.h \
        NetDataProcessCore_Global.h \
        NetDataProcessCoreConstants.h \
    NetDataManager.h \
    NetDataManager_p.h \
    IZmqWorker.h \
    WorkerThread.h

DISTFILES += \
    configuration/Configuration.ini \
    configuration/Menu.xml
