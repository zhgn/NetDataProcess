#ifndef IZMQWORKER_H
#define IZMQWORKER_H

#include <QtCore>
#include <NetDataProcessCore/NetDataProcessCore_Global.h>
#include <Core/NetworkService/NetworkService.h>
#include <Core/Factory/Factory.h>
#include <Core/CoreConstants_p.h>

namespace zmq {
class context_t;
class socket_t;
}

namespace Core{
class RedisClient;
}

namespace NetDataProcessCore
{

class NETDP_EXPORT IZmqWorker : public QThread
{
    Q_OBJECT
public:
    explicit IZmqWorker(zmq::context_t *context, QObject *parent = 0);
    virtual ~IZmqWorker();

    void setWorkerIndex(int index);
    int getWorkerIndex() const;
    /*!
     * \brief setSendKeyToZMQ 设置将数据的KEY通过ZMQ还是函数调用发送到NetDataProcessCore
     * 建议多进程方式通过ZMQ，同进程通过函数调用
     * \param bSendToZMQ true为使用ZMQ发送；默认为不使用ZMQ
     */
    virtual void setSendKeyToZMQ(bool bSendToZMQ);
    bool isSendKeyToZMQ() const;
    bool isWorkerStarted() const;
    virtual bool addKeyValue(const QByteArray &key,
                             qint64 timeMask,
                             const QByteArray &topic,
                             const QByteArray &data);

    virtual bool sendNetDataKey(const QByteArray &key, const QByteArray &topic, qint64 datasize);

    void setPackageDefine(const QString &groupName);

public slots:
    void startWorker(QString url);
    void stopWorker();

protected:
    void run();

    virtual bool initThread();
    /*!
     * \brief 处理接收到的订阅数据，储存入Redis数据库
     * \param list 根据和远端服务器的约定，list的数据长度为3；第一个条为类型；第二条为时间；第三条为数据
     */
    virtual bool recvData() = 0;
    virtual bool procData();
    virtual bool saveData() = 0;
    virtual bool notifyData() = 0;
    virtual bool endDataProc();

    virtual bool _recvData(zmq::socket_t *socket);
    virtual void _startWorker(QString url);
    virtual void _stopWorker();
    virtual QString getWorkerUrl() const;
    virtual QString getWorkerValue(const QString &key) const;

    int m_index;                //worker 的 index
    bool m_bStarted;            //worker 是否启动
    QList<QByteArray> m_topics;        //当前的 Topic 是否已经加入到 topics 中了
    static bool s_bHasTopicsKey;//是否包含 topics
    Core::RedisClient *m_redis; //redis客户端

    zmq::context_t *m_context;  //zmq的主context

    bool m_bSendToZMQ;          //是否通过zmq传递通知
    zmq::socket_t *m_workerPush;//用于传递通知的zmq socket

    qint64 m_timeMask;
    ZmqDataList m_recvedData;
    QByteArray m_currKey;
    /*!
     * \brief m_serverurl 用于启动 ZMQ 的 Socket 的地址
     * 有具体的子类确定自己的 socket 的类型以及使用 bind 还是 connect
     */
    QByteArray m_serverurl;
    QByteArray m_workerurl;

    int m_nTopicIndex;
    int m_nTimeMaskIndex;
    int m_nDataIndex;

    bool m_bDoSyncRedisSet;
};

class NETDP_EXPORT IZmqWorkerFactory : public Core::Factory
{
public:
    virtual ~IZmqWorkerFactory() {}
    virtual IZmqWorker *create(QObject *) {return NULL;}
};

}
#endif // IZMQWORKER_H
