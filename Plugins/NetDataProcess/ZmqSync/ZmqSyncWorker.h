#ifndef ZMQSYNCWORKER_H
#define ZMQSYNCWORKER_H

#include <NetDataProcessCore/IZmqWorker.h>
#include <ZmqSync/ZmqSync_Global.h>

namespace zmq {
class context_t;
class socket_t;
}

namespace Core{
class RedisClient;
}

namespace ZmqSync {

class ZMQSYNCSHARED_EXPORT ZmqSyncWorker : public NetDataProcessCore::IZmqWorker
{
    Q_OBJECT

public:
    explicit ZmqSyncWorker(zmq::context_t *context, QObject *parent = 0);
    ~ZmqSyncWorker();
    void setTopics(const QStringList &topics);

    QString url() const;
    void setUrl(const QString &url);

protected:
    virtual bool initThread();
    virtual bool recvData();
//    virtual bool procData();
    virtual bool saveData();
    virtual bool notifyData();
    virtual bool endDataProc();

    void _startWorker(QString url);

private:
    //internet
    QStringList m_topics;
    QString m_url;
    zmq::socket_t *_socket;
};

}
Q_DECLARE_LOGGING_CATEGORY(zmqSync)

#endif // ZMQSYNCWORKER_H
