#include <zmq.h>
#include <zmq.hpp>
#include "ZmqSyncWorker.h"
#include <NetDataProcessCore/NetDataManager.h>
#include <Core/ConfigManager/ConfigManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <Core/CoreConstants_p.h>

Q_LOGGING_CATEGORY(zmqSync, "NetDataProcess.ZmqSyncWorker")

using namespace ZmqSync;

ZmqSyncWorker::ZmqSyncWorker(zmq::context_t *context, QObject *parent) :
    IZmqWorker(context, parent),
    _socket(0)
{
}

ZmqSyncWorker::~ZmqSyncWorker()
{
}

void ZmqSyncWorker::setTopics(const QStringList &topics)
{
    m_topics = topics;
}

QString ZmqSyncWorker::url() const
{
    return m_url;
}

void ZmqSyncWorker::setUrl(const QString &url)
{
    m_url = url;
}

void ZmqSyncWorker::_startWorker(QString url)
{
    if(m_bStarted)
        return;

    //如果没有配置url, 则使用统一提供的url
    if(m_url.isEmpty())
        m_url = url;

    //如果远端地址为空, 则返回
    if(m_url.isEmpty() )
    {
        deleteLater();
        return;
    }
    //通知底层启动worker
    IZmqWorker::_startWorker(m_url);
    qCDebug(zmqSync) << "Topics: " << m_topics;
}

bool ZmqSyncWorker::initThread()
{
    PSFW_ASSERT(IZmqWorker::initThread(), return false);
    try
    {
        _socket = new zmq::socket_t(*m_context, ZMQ_SUB);
        //设置接收数据的超时时间
        _socket->setsockopt<int>(ZMQ_RCVTIMEO, NETDATA_MANAGER->getZmqRcvTimeout());
        //connect
        _socket->connect(m_serverurl.constData());
        //订阅所有消息
        for(QString topic : m_topics)
            _socket->setsockopt(ZMQ_SUBSCRIBE, qPrintable(topic), topic.size());
        qCDebug(zmqSync) << "ZMQ_SUBSCRIBE: " << m_topics;
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqSync) << e.what();
        stopWorker();
        return false;
    }
    return true;
}

bool ZmqSyncWorker::recvData()
{
    return IZmqWorker::_recvData(_socket);
}

bool ZmqSyncWorker::saveData()
{
    //do insert to redis
    //通知外部
    return IZmqWorker::addKeyValue(m_currKey, m_timeMask, *m_recvedData[0], *m_recvedData[2]);
}

bool ZmqSyncWorker::notifyData()
{
    return sendNetDataKey(m_currKey,"PUB", m_recvedData[2]->size() );
}

bool ZmqSyncWorker::endDataProc()
{
    //清理接收数据的 zmq socket
    try
    {
        _socket->close();
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqSync) << e.what();
    }
    SAVE_DELETE(_socket);

    return IZmqWorker::endDataProc();
}
