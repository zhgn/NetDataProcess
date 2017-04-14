#ifndef NETDATAMANAGER_P_H
#define NETDATAMANAGER_P_H

#include <zmq.hpp>
#include <QtCore>
#include <private/qobject_p.h>
#include "IZmqWorker.h"

namespace NetDataProcessCore
{
class NetDataManager;
class NetDataManagerPrivate : public QObjectPrivate
{
    Q_DISABLE_COPY(NetDataManagerPrivate)

public:
    Q_DECLARE_PUBLIC(NetDataManager)

    NetDataManagerPrivate();
    virtual ~NetDataManagerPrivate();
    void init();
    /*!
     * \brief 协调多个工作者共同处理多个请求的线程
     * 相当于负载均衡
     */
    static void startProxy();

    QMap<QString, IZmqWorkerFactory*> mapFactory;

    QString routerUrl;
    QString pubUrl;
    QString dealerUrl;
    QString pullUrl;
    QString externalPubUrl;
    QString externalReqUrl;

    zmq::socket_t *pull;
    zmq::socket_t *pub;

    QTimer *pullTimer;

    int zmqRcvTimeout;
    int zmqRcvTimer;
    QElapsedTimer *totalElapsedTimer;
    QElapsedTimer *lastElapsedTimer;
    qint64 totalDataSize;
    int lastDataSize;
    qreal averateRate;
    qreal currentRate;
    qint64 dataNumbers;
    qreal dataNumRate;
    qreal currentDataNumRate;

    QMutex gotDataMutex;
};
}

#endif
