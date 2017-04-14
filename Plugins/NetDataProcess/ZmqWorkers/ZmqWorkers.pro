DEFINES += ZMQWORKERS_LIBRARY
PLUGIN_VERSION = 1.0

include(../../../../Include/Plugin.pri)
QMAKE_EXTRA_TARGETS -= BBPY_Include
POST_TARGETDEPS -= BBPY_Include
include($$PSFW_3RDPARTYPATH/ZeroMQ/zmq.pri)

# ZmqWorkers files

SOURCES += ZmqWorkersPlugin.cpp \
    ZmqWorker.cpp

HEADERS += ZmqWorkersPlugin.h \
        ZmqWorkers_Global.h \
        ZmqWorkersConstants.h \
    ZmqWorker.h
