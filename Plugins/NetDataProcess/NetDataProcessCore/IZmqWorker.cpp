#include <zmq.hpp>
#include "IZmqWorker.h"
#include <NetDataProcessCore/NetDataManager.h>
#include <Core/ConfigManager/ConfigManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <Core/Redis/RedisClient.h>

using namespace NetDataProcessCore;

bool NetDataProcessCore::IZmqWorker::s_bHasTopicsKey = false;

IZmqWorker::IZmqWorker(zmq::context_t *context, QObject *parent)
    : QThread(parent)
    , m_index(-1)
    , m_bStarted(false)
    , m_redis(0)
    , m_context(context)
    , m_bSendToZMQ(false)
    , m_workerPush(0)
    , m_workerurl(NETDATA_MANAGER->getPullUrl().toUtf8())
    , m_nTopicIndex(-1)
    , m_nTimeMaskIndex(-1)
    , m_nDataIndex(-1)
    , m_bDoSyncRedisSet(false)
{
    setObjectName("IZmqWorker");
    connect(this, &IZmqWorker::finished, this, &IZmqWorker::deleteLater);


    QString group = CONFIG_GETVALUE2(PLUGIN_NAME, PLUGIN_NAME, "CurrentPackageDefine", "PackageDefine-BigDataServer").toString();
    setPackageDefine(group);
}

IZmqWorker::~IZmqWorker()
{
    SAVE_DELETE(m_redis);
}

void IZmqWorker::setWorkerIndex(int index)
{
    m_index = index;
}

int IZmqWorker::getWorkerIndex() const
{
    return m_index;
}

void IZmqWorker::setSendKeyToZMQ(bool bSendToZMQ)
{
    m_bSendToZMQ = bSendToZMQ;
    if(m_bSendToZMQ)
    {
        m_workerPush = new zmq::socket_t(*m_context, ZMQ_PUSH);
        //连接，用于推送获取到的实时数据
        m_workerPush->bind(qPrintable(m_workerurl));
    }
    else
    {
        if(m_workerPush)
        {
            delete m_workerPush;
            m_workerPush = NULL;
        }
    }
}

bool IZmqWorker::sendNetDataKey(const QByteArray &key, const QByteArray &topic, qint64 datasize)
{
    if(m_bSendToZMQ && m_workerPush)
    {
        try
        {
            //send reply to netdatacore: The new key
            m_workerPush->send(topic.constData(), topic.size(), ZMQ_SNDMORE);
            m_workerPush->send(key.constData(), key.size());
        }
        catch(zmq::error_t e)
        {
            qCWarning(netDataProcess) << e.what();
            return false;
        }
    }
    else
    {
        NETDATA_MANAGER->gotData(key, m_timeMask, topic, datasize);
    }
//    qCInfo(netDataProcess) << this << "Data received(key, topic, datasize): " << key << topic << datasize;
    return true;
}

void IZmqWorker::setPackageDefine(const QString &groupName)
{
    if(groupName.isEmpty())
        return;
    m_nTopicIndex = CONFIG_GETVALUE2(PLUGIN_NAME, groupName, "TopicIndex", -1).toInt();
    m_nTimeMaskIndex = CONFIG_GETVALUE2(PLUGIN_NAME, groupName, "TimeMaskIndex", -1).toInt();
    m_nDataIndex = CONFIG_GETVALUE2(PLUGIN_NAME, groupName, "DataIndex", -1).toInt();

    PSFW_ASSERT(m_nTopicIndex != -1, qCWarning(netDataProcess)<<"m_nTopicIndex config error!");
    PSFW_ASSERT(m_nTimeMaskIndex != -1, qCWarning(netDataProcess)<<"m_nTimeMaskIndex config error!");
    PSFW_ASSERT(m_nDataIndex != -1, qCWarning(netDataProcess)<<"m_nDataIndex config error!");
}

bool IZmqWorker::isSendKeyToZMQ() const
{
    return m_bSendToZMQ;
}

bool IZmqWorker::isWorkerStarted() const
{
    return m_bStarted;
}

bool IZmqWorker::addKeyValue(const QByteArray &key,
                             qint64 timeMask,
                             const QByteArray &topic,
                             const QByteArray &data)
{
    Q_UNUSED(timeMask)
    //检测是否包含 topics key
    if(!m_topics.contains(topic))
    {
        if(!s_bHasTopicsKey)
        {
            QList<QByteArray> cmd;
            cmd << "EXISTS" << "topics";
            QVariant var = m_redis->runCommandSync(cmd);
            if(var.toInt() == 1)
                s_bHasTopicsKey = true;
        }
        if(s_bHasTopicsKey)
        {
            //如果包含 topics 的 key, 则检验是否已经添加过了
            QByteArray topics = m_redis->getValueSync("topics");
            if(!topics.contains(topic))
            { //如果没有添加过则添加
                QList<QByteArray> cmd;
                cmd << "APPEND" << "topics" << (";" + topic);
                m_redis->runCommandSync(cmd);
            }
        }
        else
        {
            //如果不包含, 则添加
            m_redis->addKeyValue("topics", topic);
        }
        m_topics.append(topic);
    }
    //添加到redis中

    m_redis->addKeyValueExpireSync(key, data, 60*60*24*7);//一周
//    m_redis->setExpire(key, 60*60*24*7);//一周
//    m_redis->doZAdd(topic, timeMask, key);
    return true;
}

void IZmqWorker::startWorker(QString url)
{
    _startWorker(url);
}

void IZmqWorker::stopWorker()
{
    _stopWorker();
}

void IZmqWorker::run()
{
    PSFW_ASSERT(m_context, return);
    PSFW_ASSERT(initThread(), return);

    while(m_bStarted)
    {
        //清理
        m_currKey.clear();
        m_recvedData.clear();
        m_timeMask = -1;

        if(!recvData())
            continue;

        PSFW_ASSERT(procData(), continue);
        PSFW_ASSERT(saveData(), continue);
        PSFW_ASSERT(notifyData(), continue);
    }

    PSFW_ASSERT(endDataProc(), return);
}

bool IZmqWorker::initThread()
{
    m_redis = new Core::RedisClient;
    while(!m_redis->isConnected())
    {
        m_redis->connectServer();
        this->msleep(100);
    }
    return true;
}

bool IZmqWorker::procData()
{
    PSFW_ASSERT(!m_recvedData.isEmpty(), return false);
    m_timeMask = -1;
    qint64 utcTime = QDateTime::fromMSecsSinceEpoch(0).toMSecsSinceEpoch();
    if(m_recvedData.size() > 1)
    {
        m_timeMask = m_recvedData[1]->toLongLong();
    }

    //如果 size 不等于3, 则代表数据格式错误, 直接丢弃后, 读取下一条
    PSFW_ASSERT(m_recvedData.size() == 3, return false);

    m_currKey = *m_recvedData[0] + ':' + *m_recvedData[1];
    //运行到这里说明数据格式满足要求, 则跳出循环
    qreal size = (qreal)m_recvedData[2]->size() / 1024.;
    if(m_timeMask <= utcTime)
    {
        qCWarning(netDataProcess) << "Key: " << m_currKey
                                  << "TimeMask's format is wrong: " << m_timeMask
                                  << "Data Size(KB): " << size;
    }
    else
    {
        qCDebug(netDataProcess) << "Key: " << m_currKey
                                << QDateTime::fromMSecsSinceEpoch(m_timeMask).toString("yyyy-M-d_H:m:s.z")
                                << "Data Size: " << size <<"KB";
    }
    return true;
}

bool IZmqWorker::endDataProc()
{
    //清理上传数据的 zmq socket
    try
    {
        if(m_workerPush)
            m_workerPush->close();
        SAVE_DELETE(m_workerPush);
    }
    catch(zmq::error_t e)
    {
        qCWarning(netDataProcess) << e.what();
    }
    //清理保存数据的 redis client
    SAVE_DELETE(m_redis);
    //打印信息
    qCInfo(netDataProcess) << this << "Worker has been stopped, server url: " << m_serverurl << "; in thread: " << QThread::currentThread();

    return true;
}

bool IZmqWorker::_recvData(zmq::socket_t *socket)
{
    PSFW_ASSERT(socket, return false);
    ZmqDataList receivedDataList;
    try{
        zmq::message_t msg;
        do
        {
            if(socket->recv(&msg) )
            {
                QByteArray *data = new QByteArray((const char *)msg.data(), (int)msg.size());
                receivedDataList.append(ZmqData(data) );
            }
            else
            {
                return false;
            }
        }
        while(msg.more());//判断是否还有后续数据
    }
    catch(zmq::error_t e)
    {
        qCWarning(netDataProcess) << e.what();
        return false;
    }

    m_recvedData.append(receivedDataList.value(m_nTopicIndex) );
    m_recvedData.append(receivedDataList.value(m_nTimeMaskIndex) );
    m_recvedData.append(receivedDataList.value(m_nDataIndex) );

    return true;
}

void IZmqWorker::_startWorker(QString url)
{
    if(m_bStarted)
        return;

    m_serverurl = url.toUtf8();
    m_bStarted = true;
    //开启工作线程
    start();
    qCInfo(netDataProcess) << this << "Worker has been started, server url: " << m_serverurl;
}

void IZmqWorker::_stopWorker()
{
    m_bStarted = false;
}

QString IZmqWorker::getWorkerUrl() const
{
    return CONFIG_GETVALUE(PLUGIN_NAME, "Worker", "WorkerUrl" + QString::number(m_index)).toString();
}

QString IZmqWorker::getWorkerValue(const QString &key) const
{
    return CONFIG_GETVALUE(PLUGIN_NAME, "Worker", key + QString::number(m_index)).toString();
}
