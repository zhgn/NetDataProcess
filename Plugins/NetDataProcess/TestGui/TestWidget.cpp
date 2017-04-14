#include <zmq.hpp>
#include "TestWidget.h"
#include "ui_TestWidget.h"
#include <NetDataProcessCore/NetDataManager.h>
#include <Core/NetworkService/NetworkService.h>
#include <Core/ActionManager/ActionManager.h>
#include <ZmqSync/ZmqSyncWorker.h>
#include <PluginSystem/pluginmanager.h>
#include "TestGui_Global.h"

TestWidget::TestWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestWidget),
    updateTimer(new QTimer(this) ),
    recvTimer(new QTimer(this) ),
//    subTimer(new QTimer(this)),
    simulateTimer(new QTimer(this)),
    testSocket(new zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_REQ)),
//    testSub(new zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_SUB)),
    simulateThread(0),
    bDevMode(true),
    qredis(new Core::RedisClient(this))
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tbwCount->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 通过 Ctrl+D 切换到开发模式
    idDevMode = grabShortcut(QKeySequence("Ctrl+D") );
    switchDevMode();

    connect(ui->btnSend, &QPushButton::clicked, this, &TestWidget::send);
    connect(ui->btnSimulateStart, &QPushButton::clicked, this, &TestWidget::simulateStart);
    connect(ui->btnSimulateStop, &QPushButton::clicked, this, &TestWidget::simulateStop);
    connect(ui->btnBrowserFile, &QPushButton::clicked, this, &TestWidget::browserFile);
    connect(ui->btnSaveKey, &QPushButton::clicked, this, &TestWidget::saveRedisKey);
    connect(ui->btnStartWorkers, &QPushButton::clicked,
            NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::startWorkers);
    connect(ui->btnStopWorker, &QPushButton::clicked,
            NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::stopWorkers);
    connect(NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::sigGotData,
            this, &TestWidget::slotGotData, Qt::QueuedConnection);
    connect(NETDATA_MANAGER, &NetDataProcessCore::NetDataManager::sigMonitor,
            this, &TestWidget::slotMonitor, Qt::QueuedConnection);
    connect(ui->btnAddKey, &QPushButton::clicked, this, &TestWidget::addResisKey);
    connect(updateTimer, &QTimer::timeout, this, &TestWidget::updateNumbers);
    updateTimer->start(500);

    testSocket->connect(qPrintable(NETDATA_MANAGER->getRouterUrl()));
//    testSub->setsockopt(ZMQ_SUBSCRIBE, "", 0);
//    testSub->connect(NETDATA_MANAGER->getPubUrl().toStdString());

    recvTimer->setInterval(NETDATA_MANAGER->getZmqRcvTimer());
    connect(recvTimer, &QTimer::timeout, this, &TestWidget::recvReply);
//    subTimer->setInterval(NETDATA_MANAGER->getZmqRcvTimer());
//    connect(subTimer, &QTimer::timeout, this, &TestWidget::recvSub);
//    subTimer->start();

    simulateTimer->setInterval(NETDATA_MANAGER->getZmqRcvTimer());
    connect(simulateTimer, &QTimer::timeout, this, &TestWidget::simulateData);

    setEnabled(false);

    connect(qredis, &Core::RedisClient::connectStatusChanged, this, &TestWidget::qredisConnectStatusChanged);
    QTimer::singleShot(0, qredis, SLOT(connectServer()) );
}

TestWidget::~TestWidget()
{
//    try{
//        testSocket->close();
//        testSub->close();
//    }
//    catch(zmq::error_t e)
//    {
//        qWarning() << e.what();
//    }

    updateTimer->stop();
    recvTimer->stop();
//    subTimer->stop();

    SAVE_DELETE(testSocket);
//    SAVE_DELETE(testSub);
    simulateStop();

    qCDebug(testgui) << "zmq in TestGui have been destroyed";

    SAVE_DELETE(qredis);

    delete ui;
}

void TestWidget::upateFileData()
{
    if(fileData.isEmpty())
    {
        QFile file(ui->leFilePath->text());
        if(file.open(QFile::ReadOnly) )
        {
            fileData = file.readAll();
        }
        else
        {
            ui->listWidget->addItem(file.errorString());
            return;
        }
    }
}

bool TestWidget::event(QEvent *e)
{
    if(e->type() == QEvent::Shortcut)
    {
        if(idDevMode == ((QShortcutEvent*)e)->shortcutId())
        {
            switchDevMode();
            return true;
        }
    }
    return QWidget::event(e);
}

void TestWidget::send()
{
    QByteArray cmd = ui->leReq->text().toUtf8();
    if(cmd.isEmpty())
    {
        ui->listWidget->addItem("Command is empty!");
        return;
    }

    try{
        zmq::message_t msg(cmd.constData(), cmd.size());
        testSocket->send(msg);
    } catch(zmq::error_t e) {
        qWarning() << e.what();
    }

    recvTimer->start();

    ui->listWidget->addItem(ui->leReq->text());
}

void TestWidget::recvReply()
{
    QList<QSharedPointer<QByteArray> > list = NETWORK_SERVICE->recvZmqData(testSocket);
    if(list.isEmpty())
        return;

    recvTimer->stop();
    foreach(QSharedPointer<QByteArray> data, list)
        ui->listWidget->addItem(*data);
}
///hrdac/TracePoints?startdate=1460968822&enddate=1460968833
void TestWidget::recvSub()
{
//    QList<QSharedPointer<QByteArray> > list = NETWORK_SERVICE->recvZmqData(testSub);
//    if(list.isEmpty())
//        return;

//    qint64 oldtime = list[1]->split(':')[1].toLongLong();
//    qint64 usedtime = QDateTime::currentMSecsSinceEpoch() - oldtime;
//    ui->listWidget_2->addItem("topic: " + *list[0] + "; key: " + *list[1] + " - " + QString::number(usedtime) );
}

void TestWidget::updateNumbers()
{
    qreal onesize = NETDATA_MANAGER->getLastDataSize();
    onesize = onesize / 1024. / 1024.; //MB
    qreal totalsize = NETDATA_MANAGER->totalDataReceived();
    totalsize = totalsize / 1024. / 1024.; //MB

    ui->lbSingle->setText(QString::number(onesize));
    ui->lbTotal->setText(QString::number(totalsize));
    ui->lbAvgRate->setText(QString::number(NETDATA_MANAGER->averateTransRate()));
    ui->lbCurrRate->setText(QString::number(NETDATA_MANAGER->currentTransRate()));
    ui->lbNumbers->setText(QString::number(NETDATA_MANAGER->totalDataNumbers()));
    ui->lbNumRate->setText(QString::number(NETDATA_MANAGER->getDataNumRate()));
}

void TestWidget::simulateData()
{
    if(simulatePub)
    {
        upateFileData();
        QStringList topics = ui->leTopics->text().split(';');
        try{
            foreach (QString topic, topics) {
                zmq::message_t msg(qPrintable(topic), topic.size());
                QByteArray utc = QByteArray::number(QDateTime::currentMSecsSinceEpoch());
                simulatePub->send(msg, ZMQ_SNDMORE);
                simulatePub->send(utc.constData(), utc.size(), ZMQ_SNDMORE);
                simulatePub->send(fileData.constData(), fileData.size());
            }
        }catch(zmq::error_t e){
            qWarning()<< e.what();
        }
    }
}

void TestWidget::simulateStart()
{
    if(ui->leTopics->text().isEmpty() || ui->leFilePath->text().isEmpty() ||
            !QFile::exists(ui->leFilePath->text()))
    {
        return;
    }

    simulateTimer->start(ui->spinBox->value());
    ui->btnSimulateStop->setEnabled(true);
    ui->btnBrowserFile->setEnabled(false);
    ui->btnSimulateStart->setEnabled(false);

    if(!simulateThread)
    {
        try{
            simulatePub = new zmq::socket_t(*NETWORK_SERVICE->getZmqContext(), ZMQ_PUB);
            simulatePub->bind("inproc://zmq_simulatesyncworker");
        }catch(zmq::error_t e){
            qWarning()<< e.what();
        }

        simulateThread = new QThread;
        simulateThread->setObjectName("simulateThread");
        //在线程中运行，只需要一个实例
        simulateWorker = new ZmqSync::ZmqSyncWorker(NETWORK_SERVICE->getZmqContext());
        simulateWorker->moveToThread(simulateThread);
        simulateWorker->setObjectName("SimulateZmqSyncWorker");
        //连接必要的信号槽
        connect(simulateWorker, &ZmqSync::ZmqSyncWorker::destroyed, simulateThread, &QThread::terminate);
        connect(simulateWorker, &ZmqSync::ZmqSyncWorker::destroyed, simulateThread, &QThread::deleteLater);
        connect(simulateWorker, &ZmqSync::ZmqSyncWorker::destroyed, this, &TestWidget::testDestroyed);
        connect(simulateThread, &QThread::destroyed, this, &TestWidget::testDestroyed);
        //启动线程
        simulateThread->start();
        QMetaObject::invokeMethod(simulateWorker, "startWorker",
                                  Qt::QueuedConnection, Q_ARG(QString, "inproc://zmq_simulatesyncworker"));
    }
}

void TestWidget::simulateStop()
{
    ui->btnSimulateStart->setEnabled(true);
    ui->btnSimulateStop->setEnabled(false);
    ui->btnBrowserFile->setEnabled(true);

    if(simulateThread)
    {
        simulatePub->close();
        simulateTimer->stop();
        SAVE_DELETE(simulatePub);

        qCDebug(testgui)<<simulateWorker<<simulateThread;
        QMetaObject::invokeMethod(simulateWorker, "stopWorker", Qt::QueuedConnection);
        simulateWorker->deleteLater();
        simulateWorker = 0;

//        simulateThread->terminate();
//        simulateThread->wait(500);
//        simulateThread->deleteLater();
        simulateThread = 0;
    }
}

void TestWidget::browserFile()
{
    QString path = QFileDialog::getOpenFileName();
    ui->leFilePath->setText(path);

    fileData.clear();
    upateFileData();
}

void TestWidget::testDestroyed(QObject *obj)
{
    qCDebug(testgui)<<(void*)obj;
}

void TestWidget::saveRedisKey()
{
    QByteArray key = ui->leKey->text().toUtf8();
    QByteArray data = qredis->getValueSync(key);
    if(data.isEmpty())
        return;

    QString type = ui->cbFileType->currentText();
    QDir dir = QDir::home();
    QString path = dir.filePath(key + "." + type);
    QFile file(path);
    if(!file.open(QFile::WriteOnly | QFile::Truncate))
        return;

    file.write(data);
    file.close();
}

void TestWidget::addResisKey()
{
    QByteArray key = ui->leKey->text().toUtf8();
    upateFileData();
    qredis->addKeyValue(key, fileData);
}

void TestWidget::qredislog(const QString &log)
{
    ui->listWidget->addItem(log);
}

void TestWidget::qredisConnectStatusChanged(bool status)
{
    if(!status)
    {
        QTimer::singleShot(0, qredis, SLOT(connectServer()) );
    }
    setEnabled(status);
}

void TestWidget::slotGotData(const QByteArray &key, qint64 timeMask, qint64 datasize)
{
    setUpdatesEnabled(false);
//    QElapsedTimer testTimer;
//    testTimer.start();

    QString topic(key.left(key.indexOf(':')));
    qint64 interval = 0;
    QTime communicationTime(0,0,0,0);
    communicationTime = communicationTime.addMSecs(QDateTime::currentDateTime().toMSecsSinceEpoch() - timeMask);
    if(mapOldTopic.contains(topic))
        interval = timeMask - mapOldTopic[topic];

    QTableWidgetItem *item0 = new QTableWidgetItem(topic);
    QTableWidgetItem *item1 = new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(timeMask).time().toString("H:m:s.z"));
    QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(datasize/1024.));
    QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(interval));
    QTableWidgetItem *item4 = new QTableWidgetItem(communicationTime.toString("H:m:s.z"));
    item0->setTextAlignment(Qt::AlignCenter);
    item1->setTextAlignment(Qt::AlignCenter);
    item2->setTextAlignment(Qt::AlignCenter);
    item3->setTextAlignment(Qt::AlignCenter);
    item4->setTextAlignment(Qt::AlignCenter);

    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, item0);
    ui->tableWidget->setItem(row, 1, item1);
    ui->tableWidget->setItem(row, 2, item2);
    ui->tableWidget->setItem(row, 3, item3);
    ui->tableWidget->setItem(row, 4, item4);
//速度很慢, 消耗40毫秒
//    if(!ui->tableWidget->hasFocus())
//        ui->tableWidget->scrollToBottom();

    QList<QTableWidgetItem *> list = ui->tbwCount->findItems(topic, Qt::MatchFixedString);
    if(list.isEmpty())
    {
        row = ui->tbwCount->rowCount();
        ui->tbwCount->insertRow(row);
        QTableWidgetItem *itemTopic = new QTableWidgetItem(topic);
        itemTopic->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *itemCount = new QTableWidgetItem("1");
        itemCount->setTextAlignment(Qt::AlignCenter);
        ui->tbwCount->setItem(row, 0, itemTopic);
        ui->tbwCount->setItem(row, 1, itemCount);
    }
    else
    {
        QTableWidgetItem *itemTopic = list.first();
        QTableWidgetItem *itemCount = ui->tbwCount->item(itemTopic->row(), 1);
        int currCount = itemCount->text().toInt() + 1;
        itemCount->setText(QString::number(currCount));
    }

    mapOldTopic.insert(topic, timeMask);

    setUpdatesEnabled(true);
}

void TestWidget::slotMonitor(qint64 totalDataSize,
                             int lastDataSize,
                             qreal averateRate,
                             qreal currentRate,
                             qint64 dataNumbers,
                             qreal dataNumRate,
                             qreal currentDataNumRate)
{
    qreal onesize = lastDataSize / 1024. / 1024.; //MB
    qreal totalsize = totalDataSize / 1024. / 1024.; //MB

    ui->lbSingle->setText(QString::number(onesize));
    ui->lbTotal->setText(QString::number(totalsize));
    ui->lbAvgRate->setText(QString::number(averateRate) );
    ui->lbCurrRate->setText(QString::number(currentRate) );
    ui->lbNumbers->setText(QString::number(dataNumbers) );
    ui->lbNumRate->setText(QString::number(dataNumRate) );
    ui->lbCurrDataNumRate->setText(QString::number(currentDataNumRate) );
}

void TestWidget::switchDevMode()
{
    bDevMode = !bDevMode;
    ui->btnStartWorkers->setVisible(bDevMode);
    ui->btnStopWorker->setVisible(bDevMode);
    ui->groupBox->setVisible(bDevMode);
    ui->groupBox_3->setVisible(bDevMode);
    ui->groupBox_2->setVisible(bDevMode);
    ui->label_7->setVisible(bDevMode);
    ui->listWidget->setVisible(bDevMode);
}

void TestWidget::closeEvent(QCloseEvent *event)
{
//    deleteLater();
    QWidget::closeEvent(event);
}
