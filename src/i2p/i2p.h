//todos:
//
//function to check version of i2p
//this should be as simple as running i2pd --version and grabbing the output

//
//function to download and verify the most recent i2pd version
//stepz:
//figure out the platform
//figure out the most recent i2pd version (automagically)
//^base url to use is https://github.com/PurpleI2P/i2pd/releases/recent; recent autoresolves to tag/<version>
//download i2pd version matching the platform and version
//^take base url and add "i2pd_<version><platform tag>"
//^should support win64, macx86/macarm, linx64
//download checksum and compare if applicable
//
//function to launch i2pd with some given flags
//some sort of async thread thingy to run i2pd headless---should probably remove browser viewer by default
//
//function to check if i2pd is installed
//
//...is that it? that might be it.
#include <QMutex>
#include <QObject>
#include <QUrl>
#include <QProcess>
#include <QVariantMap>

#include "qt/FutureScheduler.h"
#include "NetworkType.h"

class I2PManager : public QObject
{
  Q_OBJECT
public:
     explicit I2PManager(QObject *parent = 0);
    ~I2PManager();

    Q_INVOKABLE bool start(const QString &flags);
    Q_INVOKABLE void stopAsync();

    //Q_INVOKABLE bool noSync() const noexcept;
    // return true if daemon process is started
    Q_INVOKABLE void runningAsync() const;
    // Send daemon command from qml and prints output in console window.
    Q_INVOKABLE void exit();
    Q_INVOKABLE QString checkI2PVersion();
    Q_INVOKABLE bool checkI2PInstalled();
    Q_INVOKABLE bool I2PInstall();
private:
    bool startWatcher() const;
    bool stopWatcher() const;
signals:
    void daemonStarted() const;
    void daemonStopped() const;
    void daemonStartFailure(const QString &error) const;
public slots:
    void printOutput();
    void printError();
    void stateChanged(QProcess::ProcessState state);
private:
    std::unique_ptr<QProcess> m_daemon;
    QMutex m_daemonMutex;
    QString m_i2pd_dir;
    QString m_i2pd_executable;
    QString m_i2pd_config;
    QString m_i2pd_tunconf;
    QString m_i2pd_tunnelsdir;
    QString m_i2pd_certsdir;
    QString m_version;
    mutable FutureScheduler m_scheduler;
};
