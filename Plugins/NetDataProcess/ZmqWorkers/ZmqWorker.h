#ifndef ZMQWORKER_H
#define ZMQWORKER_H

#include <NetDataProcessCore/IZmqWorker.h>
#include <ZmqWorkers/ZmqWorkers_Global.h>
#include <Core/NetworkService/NetDataRequest.h>

namespace zmq {
class context_t;
class socket_t;
}

namespace ZmqWorkers
{

class ZmqPairThread;

class ZMQWORKERSSHARED_EXPORT ZmqWorker : public NetDataProcessCore::IZmqWorker
{
    Q_OBJECT

public:
    explicit ZmqWorker(zmq::context_t *context, QObject *parent = 0);
    ~ZmqWorker();
    /*!
     * \brief m_mapPairThread 全局变量
     * key为请求的url不包含query部分，每个请求保证只有一个 ZmqPairThread 的实例
     * 重复请求会清除前面的
     */
    static QMap<QString, ZmqPairThread*> gMapPairThread;
    static QMutex gMapPairThreadMutex;
    static void addThread(ZmqPairThread *thread);
    static ZmqPairThread *getThread(const QString &key);
    static void removeThread(const QString &key);

protected:
    virtual bool initThread();
    virtual bool recvData();
    virtual bool procData();
    virtual bool saveData();
    virtual bool notifyData();
    virtual bool endDataProc();

    void _startWorker(QString url);
    void _stopWorker();

private:

    //inproc
    zmq::socket_t *m_workerRep;//用于接收数据请求，连接到Dealer

    /*!
     * \brief m_workerurl worker工作的zmq的socket的地址
     * 这里是用于连接 NetDataProcessCore 中 Dealer 的 ZmqSocket 的地址
     */
    QByteArray m_workerurl;
};

/*!
 * \brief The ZmqPairThread class
 * 用于从服务器接收具体的数据，接收完毕后发送信号 finished()
 */
class ZmqPairThread : public QThread
{
    Q_OBJECT
public:
    explicit ZmqPairThread(zmq::context_t *context, ZmqWorker *worker, const QString &url, QObject *parent = 0);
    ~ZmqPairThread();
    void stopWork();

    QString requestUrlRoot() const;
    QString requestUrl() const;
    void setRequestUrl(const QString &requestUrl);

protected:
    void run();
    void sendFinished();

    zmq::context_t *m_context;
    //internet
    zmq::socket_t *m_netReq; //用于发送数据请求，连接到远端Rep
    zmq::socket_t *m_pair;   //用于接收具体数据，连接到远端Pair
    ZmqWorker *m_worker;
    QString m_netReqUrl;
    Core::NetDataRequest m_request;
    bool m_stopped;
};
}
Q_DECLARE_LOGGING_CATEGORY(zmqUnsync)
#endif // ZMQWORKER_H
