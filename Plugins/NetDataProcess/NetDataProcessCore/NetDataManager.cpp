#include "NetDataManager.h"
#include "NetDataManager_p.h"

#include <Core/ConfigManager/ConfigManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <PluginSystem/pluginmanager.h>

Q_LOGGING_CATEGORY(netDataProcess, "NetDataProcess.Core")
INSTANCE_CPP(NetDataProcessCore::NetDataManager)

using namespace NetDataProcessCore;

NetDataManager::NetDataManager()
    : QObject(*new NetDataManagerPrivate(), 0)
    , PluginNode(metaObject()->className())
{
}

NetDataManager::~NetDataManager()
{
    Q_D(NetDataManager);
    SAVE_DELETE(d->totalElapsedTimer);
    SAVE_DELETE(d->lastElapsedTimer);
}

void NetDataManager::init()
{
    Q_D(NetDataManager);
    d->init();
}

void NetDataManager::extensionsInitialized()
{
    bool bAutoStartWorkers = CONFIG_GETVALUE(PLUGIN_NAME, PLUGIN_NAME, "AUTO_StartWorkers").toBool();
    if(bAutoStartWorkers)
    {
        startWorkers();
    }
}

IZmqWorker *NetDataManager::createWorker(const QString &name, QObject *parent)
{
    Q_D(NetDataManager);
    IZmqWorkerFactory *factory = d->mapFactory.value(name, NULL);
    if(factory)
    {
        return factory->create(parent);
    }
    else
        return NULL;
}

void NetDataManager::regWorkerFactory(IZmqWorkerFactory *factory)
{
    Q_D(NetDataManager);
    if(d->mapFactory.contains(factory->typeString()))
        return;
    d->mapFactory.insert(factory->typeString(), factory);
}

void NetDataManager::stopWorkers()
{
    emit stop();
    Q_D(NetDataManager);
    if(d->pull && d->pub)
    {
        try {
            d->pull->close();
            d->pub->close();
        }
        catch(zmq::error_t e)
        {
            qCWarning(netDataProcess) << e.what();
        }

        d->pullTimer->stop();

        SAVE_DELETE(d->pull);
        SAVE_DELETE(d->pub);
    }

    qCInfo(netDataProcess) << "zmq in NetDataManager have been destroyed";
}

QString NetDataManager::getRouterUrl() const
{
    Q_D(const NetDataManager);
    return d->routerUrl;
}

QString NetDataManager::getPubUrl() const
{
    Q_D(const NetDataManager);
    return d->pubUrl;
}

QString NetDataManager::getDealerUrl() const
{
    Q_D(const NetDataManager);
    return d->dealerUrl;
}

QString NetDataManager::getPullUrl() const
{
    Q_D(const NetDataManager);
    return d->pullUrl;
}

QString NetDataManager::getExternalPubUrl() const
{
    Q_D(const NetDataManager);
    return d->externalPubUrl;
}

QString NetDataManager::getExternalReqUrl() const
{
    Q_D(const NetDataManager);
    return d->externalReqUrl;
}

int NetDataManager::getZmqRcvTimeout() const
{
    Q_D(const NetDataManager);
    return d->zmqRcvTimeout;
}

int NetDataManager::getZmqRcvTimer() const
{
    Q_D(const NetDataManager);
    return d->zmqRcvTimer;
}

void NetDataManager::gotData(const QByteArray &key, qint64 timeMask, const QByteArray &topic, qint64 datasize)
{
//    qCInfo(netDataProcess) << topic << key << datasize;

    Q_D(NetDataManager);
    //正常情况 pub 在初始化的时候就一定已经初始化完成了
    PSFW_ASSERT(d->pub, return);
    //使用 mutex 确保多线程时,此函数可以正常工作,设计 zmq socket 和 下面的计算工作
    QMutexLocker locker(&d->gotDataMutex);

    //通过 pub zmq socket 对外部发布通过网络接收到的各种数据,通过topic区分不同类型的数据
    try{
        zmq::message_t msgtopic(topic.constData(), topic.size());
        zmq::message_t msgbody(key.constData(), key.size());
        //通过PUB接口广播出去
        d->pub->send(msgtopic, ZMQ_SNDMORE);
        d->pub->send(msgbody);
    } catch(zmq::error_t e) {
        qCWarning(netDataProcess)<< e.what();
    }

    //对外通过信号发布 接收到的数据 的信息, 主要用于本进程内的其他需求
    emit sigGotData(key, timeMask, datasize);

    //计算数据的相关统计信息
    if(datasize == 0)
        return;
    d->totalDataSize += datasize;
    d->lastDataSize = datasize;
    d->dataNumbers++;
    if(!d->totalElapsedTimer->isValid() )
    {
        d->totalElapsedTimer->start();
    }
    else
    {
        d->dataNumRate = (qreal)d->dataNumbers / (qreal)(d->totalElapsedTimer->elapsed()) * 1000.; // 个每秒
        d->averateRate = (qreal)d->totalDataSize / (qreal)(d->totalElapsedTimer->elapsed()) * 1000. / 1024. / 1024.;// MB/s
        if(d->lastElapsedTimer->isValid())
        {
            d->currentRate = (qreal)datasize / (qreal)(d->lastElapsedTimer->elapsed()) * 1000. / 1024. / 1024.; // MB/s
            d->currentDataNumRate = (qreal)1. / (qreal)(d->lastElapsedTimer->elapsed()) * 1000.; // 个每秒
        }
    }

    //对外通过信号发布统计信息
    emit sigMonitor(d->totalDataSize,
                    d->lastDataSize,
                    d->averateRate,
                    d->currentRate,
                    d->dataNumbers,
                    d->dataNumRate,
                    d->currentDataNumRate);

    d->lastElapsedTimer->start();
}

void NetDataManager::startWorkers()
{
    Q_D(NetDataManager);
    emit startSyncWorker(d->externalPubUrl);
    emit startUnsyncWorker(d->externalReqUrl);
}

void NetDataManager::recvPull()
{
    Q_D(NetDataManager);
    //接收通过非REQ-REP方式获取到的网络数据
    QList<QSharedPointer<QByteArray> > list = NETWORK_SERVICE->recvZmqData(d->pull);
    if(list.isEmpty())
        return;

    //通过PUB接口广播出去
    int i = 0;
    for(; i < list.size() - 1; ++i)
    {
        d->pub->send(list[i]->constData(), list[i]->size(), ZMQ_SNDMORE);
    }
    d->pub->send(list[i]->constData(), list[i]->size());
}

void NetDataManager::recvPub()
{

}

qint64 NetDataManager::totalDataReceived() const
{
    Q_D(const NetDataManager);
    return d->totalDataSize;
}

int NetDataManager::getLastDataSize() const
{
    Q_D(const NetDataManager);
    return d->lastDataSize;
}

qreal NetDataManager::currentTransRate() const
{
    Q_D(const NetDataManager);
    return d->currentRate;
}

qreal NetDataManager::averateTransRate() const
{
    Q_D(const NetDataManager);
    return d->averateRate;
}

qint64 NetDataManager::totalDataNumbers() const
{
    Q_D(const NetDataManager);
    return d->dataNumbers;
}

qreal NetDataManager::getDataNumRate() const
{
    Q_D(const NetDataManager);
    return d->dataNumRate;
}

qreal NetDataManager::getCurrentDataNumRate() const
{
    Q_D(const NetDataManager);
    return d->currentDataNumRate;
}
