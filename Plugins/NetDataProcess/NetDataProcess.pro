TEMPLATE  = subdirs
CONFIG   += ordered

exists(NetDataProcessCore/NetDataProcessCore.pro) : SUBDIRS *= NetDataProcessCore

SUBDIRS +=  \
    ZmqWorkers \
    ZmqSync \
    TestGui
