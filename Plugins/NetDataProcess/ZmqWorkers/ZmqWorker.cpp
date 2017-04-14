#include <zmq.h>
#include <zmq.hpp>
#include "ZmqWorker.h"
#include <NetDataProcessCore/NetDataManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <Core/ConfigManager/ConfigManager.h>
#include <PSFW_Global.h>

Q_LOGGING_CATEGORY(zmqUnsync, "NetDataProcess.ZmqWorker")

using namespace ZmqWorkers;

QMap<QString, ZmqPairThread*> ZmqWorker::gMapPairThread;
QMutex ZmqWorker::gMapPairThreadMutex;

ZmqWorker::ZmqWorker(zmq::context_t *context, QObject *parent) :
    IZmqWorker(context, parent),
    m_workerRep(0)
{
    m_workerurl = NETDATA_MANAGER->getDealerUrl().toUtf8();
}

ZmqWorker::~ZmqWorker()
{
}

void ZmqWorker::addThread(ZmqPairThread *thread)
{
    QMutexLocker locker(&ZmqWorker::gMapPairThreadMutex);
    ZmqWorker::gMapPairThread.insert(thread->requestUrlRoot(), thread);
}

ZmqPairThread *ZmqWorker::getThread(const QString &key)
{
    QMutexLocker locker(&ZmqWorker::gMapPairThreadMutex);
    return ZmqWorker::gMapPairThread.value(key, NULL);
}

void ZmqWorker::removeThread(const QString &key)
{
    QMutexLocker locker(&ZmqWorker::gMapPairThreadMutex);
    ZmqWorker::gMapPairThread.remove(key);
}

void ZmqWorker::_startWorker(QString url)
{
    if(m_bStarted)
        return;

    //检查本插件的配置文件中是否指定了远端数据 PUB 端的地址
    url = CONFIG_GETVALUE2(PLUGIN_NAME, PLUGIN_NAME, "ExternalReqUrl", url).toString();
    //如果远端地址为空, 则返回
    if(url.isEmpty() )
    {
        deleteLater();
        return;
    }
    //通知底层启动worker
    IZmqWorker::_startWorker(url);
}

void ZmqWorker::_stopWorker()
{
    IZmqWorker::_stopWorker();

    QMutexLocker locker(&ZmqWorker::gMapPairThreadMutex);
    foreach(ZmqPairThread *thread, ZmqWorker::gMapPairThread)
    {
        thread->stopWork();
    }
    ZmqWorker::gMapPairThread.clear();
}

bool ZmqWorker::initThread()
{
    PSFW_ASSERT(IZmqWorker::initThread(), return false);
    try
    {
        //准备接收框架进程的获取数据的请求
        m_workerRep = new zmq::socket_t(*m_context, ZMQ_REP);
        m_workerRep->setsockopt<int>(ZMQ_LINGER, 100);
        m_workerRep->connect(qPrintable(m_workerurl));

        //设置接收数据的超时时间
        //        m_workerRep->setsockopt<int>(ZMQ_RCVTIMEO, NETDATA_MANAGER->getZmqRcvTimeout());
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqUnsync) << e.what() << endl
                             << " m_workerurl: " <<m_workerurl << endl
                             << " m_serverurl: " <<m_serverurl << endl;
        stopWorker();
        return false;
    }

    return true;
}

bool ZmqWorker::recvData()
{
    try{
        //还没有接收到客户端的请求, 则接收请求
        //如果数据已经下载过，并存储与redis中，则程序不会运行到这里
        m_recvedData = NETWORK_SERVICE->recvZmqData(m_workerRep, 0); //不需要设置recv timeout, 如果没有请求过来就一直等待
        PSFW_ASSERT(!m_recvedData.isEmpty(), return false);

        //没有具体含义只是为了完成通讯流程
        m_workerRep->send("worker start to work", 20);
        //send reply to dealer, 完成了完整的REQ-REP
    }
    catch(zmq::error_t e)
    {
        qWarning(zmqUnsync) << e.what();
        return false;
    }
    return true;
}

bool ZmqWorker::procData()
{
    //保存请求地址
    ZmqData url = m_recvedData[0];
    QString strUrl = QString::fromUtf8(*url);
    Core::NetDataRequest request(strUrl);

    if(m_recvedData.size() == 1)
    {
        //打印通知
        qCInfo(zmqUnsync) << "Receive request: " << strUrl;

        //判断是不是有已经在进行的请求
        if(ZmqPairThread *oldThread = ZmqWorker::getThread(request.url.toString() ) )
        {
            //停止并删除旧的线程
            oldThread->stopWork();
            oldThread->deleteLater();
            ZmqWorker::removeThread(request.url.toString() );
        }

        //开启Pair线程
        ZmqPairThread *thread = new ZmqPairThread(m_context, this, m_serverurl);
        thread->setRequestUrl(strUrl);
        //加入Map
        ZmqWorker::addThread(thread);
        //启动线程
        thread->start();
    }

    //判断是否包含命令
    if(m_recvedData.size() > 1)
    {
        QString cmd = QString::fromUtf8(m_recvedData[1]->toLower());
        //打印通知
        qCInfo(zmqUnsync) << "Receive request: " << strUrl << " cmd: " << m_recvedData[1];

        if(cmd != "stop")
            return false;

        //判断是不是有已经在进行的请求
        if(ZmqPairThread *oldThread = ZmqWorker::getThread(request.url.toString() ) )
        {
            //打印通知
            qCInfo(zmqUnsync) << "Remove old thread: " << oldThread->requestUrl();
            //停止并删除旧的线程
            oldThread->stopWork();
            oldThread->deleteLater();
            ZmqWorker::removeThread(request.url.toString() );
        }
    }
    return true;
}

bool ZmqWorker::saveData()
{
    return true;
}

bool ZmqWorker::notifyData()
{
    return true;
}

bool ZmqWorker::endDataProc()
{
    try {
        m_workerRep->close();
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqUnsync) << e.what();
    }
    SAVE_DELETE(m_workerRep);

    return IZmqWorker::endDataProc();
}

ZmqPairThread::ZmqPairThread(zmq::context_t *context, ZmqWorker *worker, const QString &url, QObject *parent)
    : QThread(parent)
    , m_context(context)
    , m_netReq(0)
    , m_pair(0)
    , m_worker(worker)
    , m_netReqUrl(url)
    , m_stopped(false)
{
}

ZmqPairThread::~ZmqPairThread()
{
    m_context = NULL;
    m_pair = NULL;
}

void ZmqPairThread::stopWork()
{
    sendFinished();
    m_stopped = true;
}

void ZmqPairThread::run()
{
    if(!m_context)
        m_context = NETWORK_SERVICE->getZmqContext();

    PSFW_ASSERT(m_context, return);
    PSFW_ASSERT(m_worker, return);
    PSFW_ASSERT(!m_pair, return);
    PSFW_ASSERT(!m_netReqUrl.isEmpty(), return);

    try
    {
        //连接至远端REP端口，发送数据请求
        m_netReq = new zmq::socket_t(*m_context, ZMQ_REQ);
        m_netReq->setsockopt<int>(ZMQ_LINGER, 100);
        m_netReq->connect(qPrintable(m_netReqUrl) );

        //已经接收到客户端的请求, 则转发请求到远端服务器, 并获取接收具体数据的 pair 连接
        //发送Req到远端服务器, msg 和 url 内容一致
        m_netReq->send(qPrintable(m_request.reqstr), m_request.reqstr.size());

        //接收请求返回的pair连接地址
        zmq::message_t msg;
        m_netReq->recv(&msg);//TODO 单独设置recv timeout, 避免服务器问题卡死服务
        //没有后续数据了，仅仅接收PAIR连接的地址
        QByteArray pairUrl = QByteArray::fromRawData((const char*)msg.data(), (int)msg.size());
        //判断pair连接是否正确
        if(pairUrl.isEmpty() || !pairUrl.startsWith("tcp://") || pairUrl.count(':') != 2)
        {
            qCWarning(zmqUnsync) << "Error: " << pairUrl << " ; request: " << m_netReqUrl;
            m_stopped = true;
        }
        else
        {
            //开启Pair连接，用于接收数据
            m_pair = new zmq::socket_t(*m_context, ZMQ_PAIR);
            m_pair->setsockopt<int>(ZMQ_LINGER, 100);
            m_pair->connect(qPrintable(pairUrl) );

            //发送Ready信号给远端服务器
            zmq::message_t readyMsg("Ready", 5);
            m_pair->send(readyMsg);
            qCInfo(zmqUnsync) << this << "PAIR has been connected; url: " << pairUrl;
        }
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqUnsync) << e.what();
        m_stopped = true;
    }

    //连接建立完毕，开启接收
    while(!m_stopped)
    {
        ZmqDataList received = NETWORK_SERVICE->recvZmqData(m_pair);
        if(received.isEmpty())
            continue;

        if(received[0]->toLower() == "send finished")
        {
            sendFinished();

            //打印信息
            qCInfo(zmqUnsync) << this << "Request: " << m_request.reqstr << " finished.";

            //发送通知到客户端, 表示此条请求已经处理完成
            NETDATA_MANAGER->gotData(QByteArray("receive finished"), -1, m_request.reqstr.toUtf8(), 0);

            //处理结束
            break;
        }

        qint64 timeMask = -1;
        if(received.size() > 1)
        {
            timeMask = received[0]->toLongLong();
        }
        //运行到这里说明数据格式满足要求, 则跳出循环
        qreal size = received[1]->size() / 1024; //KB
        QByteArray key = m_request.mainkey % ":" % *received[0];
        if(timeMask <= 999999999999) //timeMask不是13位
        {
            qCWarning(zmqUnsync) << "Key: " << key
                                 << "TimeMask's format is wrong: " << timeMask
                                 << "Data Size(KB): " << size;
        }
        else
        {
            qCDebug(zmqUnsync) << "Key: " << key
                               << QDateTime::fromMSecsSinceEpoch(timeMask)
                               << "Data Size(KB): " << size;
        }
        //saveData
        m_worker->addKeyValue(key, timeMask, m_request.mainkey, *received[1]);
        //发送通知到客户端, 表示此条请求已经处理完成
        NETDATA_MANAGER->gotData(key, timeMask, m_request.reqstr.toUtf8(), received[1]->size());
    }

    try {
        if(m_pair)
            m_pair->close();
        if(m_netReq)
            m_netReq->close();
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqUnsync) << e.what();
    }
    SAVE_DELETE(m_pair);
    SAVE_DELETE(m_netReq);
}

void ZmqPairThread::sendFinished()
{
    PSFW_ASSERT(m_pair, return);
    //发送接收完毕信号给远端服务器
    zmq::message_t msgFinished("receive finished", 16);

    //数据接收完毕，解除连接状态
    try {
        m_pair->send(msgFinished);
    }
    catch(zmq::error_t e)
    {
        qCWarning(zmqUnsync) << e.what();
    }
}

QString ZmqPairThread::requestUrlRoot() const
{
    return m_request.url.toString();
}

QString ZmqPairThread::requestUrl() const
{
    return m_request.reqstr;
}

void ZmqPairThread::setRequestUrl(const QString &requestUrl)
{
    m_request = Core::NetDataRequest(requestUrl);
}
