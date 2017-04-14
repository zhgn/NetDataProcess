DEFINES += TESTGUI_LIBRARY
PLUGIN_VERSION = 1.0

include(../../../../Include/Plugin.pri)
QMAKE_EXTRA_TARGETS -= BBPY_Include
POST_TARGETDEPS -= BBPY_Include
include($$PSFW_3RDPARTYPATH/ZeroMQ/zmq.pri)

# TestGui files

SOURCES += \
    TestGuiPlugin.cpp \
    TestWidget.cpp

HEADERS += \
    TestGuiPlugin.h \
    TestGui_Global.h \
    TestGuiConstants.h \
    TestWidget.h

FORMS += \
    TestWidget.ui
