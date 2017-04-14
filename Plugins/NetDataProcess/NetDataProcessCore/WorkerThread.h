#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QtCore>
#include <NetDataProcessCore/NetDataProcessCore_Global.h>
#include <NetDataProcessCore/IZmqWorker.h>

namespace NetDataProcessCore
{

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(const QString &workername, QObject *parent = 0);

protected:
    void run();

private:
    QString _workername;
    IZmqWorker *_worker;
};

}
Q_DECLARE_LOGGING_CATEGORY(networkerThread)
#endif // WORKERTHREAD_H
