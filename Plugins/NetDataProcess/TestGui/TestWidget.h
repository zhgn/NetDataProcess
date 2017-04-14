#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QtWidgets>
#include <Core/Redis/RedisClient.h>

namespace zmq {
class context_t;
class socket_t;
}

namespace Ui {
class TestWidget;
}

namespace ZmqSync
{
class ZmqSyncWorker;
}

class TestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestWidget(QWidget *parent = 0);
    ~TestWidget();
    void upateFileData();

protected:
    bool event(QEvent *e);

public slots:
    void send();
    void recvReply();
    void recvSub();
    void updateNumbers();
    void simulateData();
    void simulateStart();
    void simulateStop();
    void browserFile();
    void testDestroyed(QObject *obj);
    void saveRedisKey();
    void addResisKey();
    void qredislog(const QString& log);
    void qredisConnectStatusChanged(bool status);

    void slotGotData(const QByteArray &key, qint64 timeMask, qint64 datasize);
    void slotMonitor(qint64 totalDataSize,
                     int lastDataSize,
                     qreal averateRate,
                     qreal currentRate,
                     qint64 dataNumbers,
                     qreal dataNumRate,
                     qreal currentDataNumRate);
    void switchDevMode();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::TestWidget *ui;
    QTimer *updateTimer;
    QTimer *recvTimer;
    QTimer *subTimer;
    QTimer *simulateTimer;

    zmq::socket_t *testSocket;
    zmq::socket_t *testSub;
    zmq::socket_t *simulatePub;

    QThread *simulateThread;
    ZmqSync::ZmqSyncWorker *simulateWorker;
    QByteArray fileData;
    QMap<QString, qint64> mapOldTopic;
    int idDevMode;
    bool bDevMode;

    Core::RedisClient *qredis;
    QAction *actDevMode;
};

#endif // TESTWIDGET_H
