DEFINES += ZMQSYNC_LIBRARY
PLUGIN_VERSION = 1.0

include(../../../../Include/Plugin.pri)
QMAKE_EXTRA_TARGETS -= BBPY_Include
POST_TARGETDEPS -= BBPY_Include
include($$PSFW_3RDPARTYPATH/ZeroMQ/zmq.pri)

# ZmqSync files

SOURCES += ZmqSyncPlugin.cpp \
    ZmqSyncWorker.cpp

HEADERS += ZmqSyncPlugin.h \
        ZmqSync_Global.h \
        ZmqSyncConstants.h \
    ZmqSyncWorker.h

DISTFILES += \
    configuration/Configuration.ini
