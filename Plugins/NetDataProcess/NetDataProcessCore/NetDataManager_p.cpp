//参考qlineedit_p.cpp，必须把NetDataManager.h放在私有的头文件的前面，否则无法通过编译。所有基于qobjectprivate的私有类都需要如此处理
#include "NetDataManager.h"
#include "NetDataManager_p.h"
#include <Core/NetworkService/NetworkService.h>
#include <Core/ConfigManager/ConfigManager.h>
#include <QtConcurrent>

using namespace NetDataProcessCore;

NetDataManagerPrivate::NetDataManagerPrivate()
    : pull(0)
    , pub(0)
    , pullTimer(0)
    , zmqRcvTimeout(500)
    , zmqRcvTimer(200)
    , totalElapsedTimer(0)
    , lastElapsedTimer(0)
    , totalDataSize(0)
    , lastDataSize(0)
    , averateRate(0)
    , currentRate(0)
    , dataNumbers(0)
    , dataNumRate(0)
{

}

NetDataManagerPrivate::~NetDataManagerPrivate()
{
}

void NetDataManagerPrivate::init()
{
    Q_Q(NetDataManager);
    pullTimer = new QTimer(q);
    totalElapsedTimer = new QElapsedTimer;
    lastElapsedTimer = new QElapsedTimer;
    QObject::connect(pullTimer, SIGNAL(timeout()), q, SLOT(recvPull()));

    routerUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "RouterUrl").toString();
    dealerUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "DealerUrl").toString();
    pullUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "PullUrl").toString();
    pubUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "PubUrl").toString();
    externalPubUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "ExternalPubUrl").toString();
    externalReqUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "ExternalReqUrl").toString();
    if(QSettings *settings = CONFIG_MANAGER->getQSettings(PLUGIN_NAME))
    {
        settings->beginGroup(PLUGIN_NAME);
        if(settings->contains("ZMQ_RcvTimeout"))
        {
            zmqRcvTimeout = settings->value("ZMQ_RcvTimeout").toInt();
        }
        if(settings->contains("ZMQ_RcvTimer"))
        {
            zmqRcvTimer = settings->value("ZMQ_RcvTimer").toInt();
        }
        settings->endGroup();
    }

    QtConcurrent::run(NetDataManagerPrivate::startProxy);

    try{
        pull = new zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_PULL);
        pull->setsockopt<int>(ZMQ_LINGER, 100);
        pub = new zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_PUB);
        pub->setsockopt<int>(ZMQ_SNDHWM, 100);
        pull->bind(pullUrl.toStdString());
        pub->bind(pubUrl.toStdString());
    }
    catch(zmq::error_t e)
    {
        qWarning(netDataProcess)<<e.what();
    }

    pullTimer->start(zmqRcvTimer);
}

void NetDataManagerPrivate::startProxy()
{
    try{
        QString routerUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "RouterUrl").toString();
        QString dealerUrl = CONFIG_MANAGER->getValue(PLUGIN_NAME, PLUGIN_NAME, "DealerUrl").toString();
        zmq::socket_t router = zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_ROUTER);
        zmq::socket_t dealer = zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_DEALER);
        router.bind(routerUrl.toStdString());
        dealer.bind(dealerUrl.toStdString());
        //queue proxy
        zmq::proxy((void*)router, (void*)dealer, NULL);
    }
    catch(zmq::error_t e)
    {
        qWarning(netDataProcess)<<e.what();
    }

    qCInfo(netDataProcess) << "NetDataManager proxy had been destroyed.";
}
