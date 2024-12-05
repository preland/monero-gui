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
#include "i2p.h"
#include "curl_effective_url.h"
#include <curl/curl.h>
#include <string>
#include <QElapsedTimer>
#include <QFile>
#include <QMutexLocker>
#include <QThread>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include <QProcess>
#include <QStorageInfo>
#include <QVariantMap>
#include <QVariant>
#include <QMap>

namespace {
    static const int DAEMON_START_TIMEOUT_SECONDS = 120;
}
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool I2PManager::start(const QString &flags){
  if(!checkI2PInstalled()) {
    return false;
  }

  QStringList arguments;
  //todo: figure out static flags here

  //custom flags
  foreach (const QString &str, flags.split(" ")) {
          qDebug() << QString(" [%1] ").arg(str);
          if (!str.isEmpty())
            arguments << str;
    }
  qDebug() << "starting i2pd " + m_i2pd;
  qDebug() << "With command line arguments " << arguments;
  QMutexLocker locker(&m_daemonMutex);
  m_daemon.reset(new QProcess());

    // Connect output slots
  connect(m_daemon.get(), SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
  connect(m_daemon.get(), SIGNAL(readyReadStandardError()), this, SLOT(printError()));

  // Start i2pd 
  bool started = m_daemon->startDetached(m_i2pd, arguments);

  // add state changed listener
  connect(m_daemon.get(), SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

  if (!started) {
      qDebug() << "i2pd start error: " + m_daemon->errorString();
      emit daemonStartFailure(m_daemon->errorString());
      return false;
  }

  // Start start watcher
  m_scheduler.run([this] {
      if (startWatcher()) {
          emit daemonStarted();
      } else {
          emit daemonStartFailure(tr("Timed out, local node is not responding after %1 seconds").arg(DAEMON_START_TIMEOUT_SECONDS));
      }
  });

  return true;
}
void I2PManager::stopAsync(){

}

void I2PManager::runningAsync() const {

}
void I2PManager::exit() {

}  
bool I2PManager::startWatcher() const {
  return false;
}
bool I2PManager::stopWatcher() const {
  return false;
}
void I2PManager::printOutput() {

}
void I2PManager::printError() {

}
void I2PManager::stateChanged(QProcess::ProcessState state) {

}

QString I2PManager::checkI2PVersion() {
  
  if(!checkI2PInstalled()) {
    return "Not Installed";
  }
  QStringList arguments = {"--version"};
  if(m_version.length() <= 0) {
    QProcess *proc = new QProcess();
    proc->start(m_i2pd, arguments);
    //if this takes longer than 5 seconds it will give up
    proc->waitForFinished(5000);

    m_version = proc->readAllStandardOutput();
    //todo: format above properly
  }
  return m_version;
}
bool I2PManager::checkI2PInstalled() {
  if(QFileInfo(m_i2pd).isFile()) {
    return true;
  }
  return false;
}
bool I2PManager::I2PInstall() {
  if(checkI2PInstalled()) {
    return false;
  }
std::string url = get_url_redirect("https://github.com/PurpleI2P/i2pd/releases/latest");
std::string version = url.substr(url.find_last_of('/')+1);
  if(version == "latest"){
    qCritical() << "getting i2pd version failed";
    return false;
  }
  std::string filename = "";
#ifdef Q_OS_WIN
  filename = "setup_i2pd_v" + version = ".exe"; //this is probably wrong
#endif
#ifdef Q_OS_MACOS
  filename = "i2pd_" + version + "_osx.tar.gz";
#endif
#ifdef Q_OS_LINUX
  filename = "i2pd_" + version + "-1bookworm1_amd64.deb";
#endif
  if(filename == ""){
    qCritical() << "unsupported platform";
    return false;
  }
    CURL *curl;
  FILE *fp;
  CURLcode res;
  std::string downloadurl = url + filename;
  std::string outfilename = QApplication::applicationDirPath().toStdString() + "/" + filename;
  curl = curl_easy_init();
  if (curl) {
    fp = fopen(outfilename.c_str(),"wb");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
  }
}
I2PManager::I2PManager(QObject *parent)
    : QObject(parent)
    , m_scheduler(this)
{

    // Platform dependent path to monerod
#ifdef Q_OS_WIN
    m_i2pd = QApplication::applicationDirPath() + "/i2pd.exe";
#endif
#ifdef Q_OS_UNIX
    m_i2pd = QApplication::applicationDirPath() + "/i2pd";
#endif

    if (m_i2pd.length() == 0) {
        qCritical() << "no i2p binary defined for current platform";
    }
}

I2PManager::~I2PManager()
{

}
