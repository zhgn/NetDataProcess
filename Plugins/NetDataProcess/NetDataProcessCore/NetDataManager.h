#ifndef NETDATAMANAGER_H
#define NETDATAMANAGER_H

#define NETDATA_MANAGER NetDataProcessCore::NetDataManager::instance()

#include <QObject>
#include <QProcess>
#include <QLoggingCategory>
#include <Core/EventService/PluginNode.h>
#include <NetDataProcessCore/NetDataProcessCore_Global.h>

namespace zmq {
class context_t;
class socket_t;
}

namespace NetDataProcessCore
{
class NetDataManagerPrivate;
class IZmqWorkerFactory;
class IZmqWorker;
class NETDP_EXPORT NetDataManager : public QObject, public Core::PluginNode
{
    Q_OBJECT

    Q_DISABLE_COPY(NetDataManager)
    Q_DECLARE_PRIVATE(NetDataManager)
    INSTANCE(NetDataManager)

    NetDataManager();
public:
    ~NetDataManager();
    /*!
     * \brief init 执行初始化工作
     */
    void init();
    void extensionsInitialized();

    void regWorkerFactory(IZmqWorkerFactory *factory);
    IZmqWorker *createWorker(const QString &name, QObject *parent);

    /*!
     * \brief getRouterUrl 用于接收多个FrameWork进程发送的REQ
     */
    QString getRouterUrl() const;
    /*!
     * \brief getPubUrl 用于对外发布订阅，内容为缓存到的数据的KEY
     */
    QString getPubUrl() const;
    /*!
     * \brief getDealerUrl 用于连接多个扩展的工作者，执行REQ并返回REP
     */
    QString getDealerUrl() const;
    /*!
     * \brief getPullUrl 用于接收工作者完成的工作
     */
    QString getPullUrl() const;
    /*!
     * \brief getExternalPubUrl 远端服务器的订阅地址
     */
    QString getExternalPubUrl() const;
    /*!
     * \brief getExternalReqUrl 远端服务器的REQ地址
     */
    QString getExternalReqUrl() const;
    /*!
     * \brief getZmqRcvTimeout 获取ZMQ接收数据的超时时间
     * 用于在接收数据时，如果添加了ZMQ_DNOTWAIT标志后能够在超过超时时间后快速返回，不至于产生阻塞
     * 默认为500ms，可以通过配置文件 ZMQ_RcvTimeout 指定
     */
    int getZmqRcvTimeout() const;
    /*!
     * \brief getZmqRcvTimer 获取用于接收数据的Timer的触发时间
     * 默认 200ms，可以通过配置文件 ZMQ_RcvTimer 指定
     */
    int getZmqRcvTimer() const;

    //以下为统计数据用的函数
    /*!
     * \brief totalDataReceived 获取总计收到的数据量，单位字节
     */
    qint64 totalDataReceived() const;
    /*!
     * \brief getLastDataSize 获取上一次缓存到的数据的尺寸，单位字节
     */
    int getLastDataSize() const;
    /*!
     * \brief currentTransRate 当前速率，单位 MB/s
     */
    qreal currentTransRate() const;
    /*!
     * \brief averateTransRate 平均速率，单位 MB/s
     */
    qreal averateTransRate() const;
    /*!
     * \brief totalDataNumbers 总的数据包个数
     */
    qint64 totalDataNumbers() const;
    /*!
     * \brief getDataNumRate 数据包个数的速率
     */
    qreal getDataNumRate() const;
    /*!
     * \brief getDataNumRate 数据包个数的速率
     */
    qreal getCurrentDataNumRate() const;

signals:
    /*!
     * \brief startSyncWorker 启动同步工作的Worker，用于缓存外部服务器的实时数据
     * \param serverUrl 外部服务器的地址
     */
    void startSyncWorker(QString serverUrl);
    /*!
     * \brief startUnsyncWorker 启动异步同坐的Worker，用于根据请求从外部服务器获取指定的数据
     * \param serverUrl 外部服务器中执行请求的地址
     */
    void startUnsyncWorker(QString serverUrl);
    /*!
     * \brief stop 通知所有的 Worker 停止工作
     */
    void stop();

    void sigGotData(const QByteArray &key, qint64 timeMask, qint64 datasize);
    void sigMonitor(qint64 totalDataSize,
                    int lastDataSize,
                    qreal averateRate,
                    qreal currentRate,
                    qint64 dataNumbers,
                    qreal dataNumRate,
                    qreal currentDataNumRate);

public slots:
    /*!
     * \brief gotData 通知缓存了数据
     * \param key 数据的存储KEY
     * \param topic 数据的Topic：ReqUrl、PUB
     * \param datasize 数据收到了 datasize 个字节
     */
    void gotData(const QByteArray &key, qint64 timeMask, const QByteArray &topic, qint64 datasize);
//    void gotData2(const QByteArray &key, qint64 timeMask, const QByteArray &topic, const QByteArray &data);
    /*!
     * \brief startWorkers 启动所有的Workers
     */
    void startWorkers();
    /*!
     * \brief stopWorkers 程序要退出了，开始关闭所有的 Workers
     */
    void stopWorkers();

private slots:
    void recvPull();
    void recvPub();
};

}

Q_DECLARE_LOGGING_CATEGORY(netDataProcess)
#endif // NETDATAMANAGER_H
