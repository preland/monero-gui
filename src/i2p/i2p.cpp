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
#include <cstddef>
#include <curl/curl.h>
#include <curl/easy.h>
#include <ostream>
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
std::cout << "url: [" << url << "]" << std::endl;
std::cout << "url size: " << url.length() << std::endl;
size_t last_slash = url.rfind('/');
std::cout << "lastpos: " << last_slash << std::endl;
std::string version = url.substr(last_slash + 1);
  if(version == "latest" || last_slash == std::string::npos){
    qCritical() << "getting i2pd version failed";
    return false;
  }
  std::cout << version << std::endl;
  std::string filename = "";
  std::string sep = "";
#ifdef Q_OS_WIN
  filename = "setup_i2pd_v" + version = ".exe"; //this is definitely wrong
  sep = "\\";
#endif
#ifdef Q_OS_MACOS
  filename = "i2pd_" + version + "_osx.tar.gz";
  sep = "/";
#endif
#ifdef Q_OS_LINUX
  filename = "i2pd_" + version + "-1bookworm1_amd64.deb";
  sep = "/";
#endif
  std::cout << "2\n";
  if(filename == ""){
    qCritical() << "unsupported platform";
    return false;
  }
  CURL *curl;
  FILE *fp;
  CURLcode res;
  std::string downloadurl = "https://www.github.com/PurpleI2P/i2pd/releases/download/"+ version + "/" + filename;
  std::cout << downloadurl << std::endl;
  std::string outfilename = QApplication::applicationDirPath().toStdString() + sep + filename;
  std::cout << outfilename << std::endl;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {

    /* Switch on full protocol/debug output while testing */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* disable progress meter, set to 0L to enable and disable debug output */ 
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_URL, downloadurl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    fp = fopen(outfilename.c_str(),"wb");
    if(fp) {
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      res = curl_easy_perform(curl);
      fclose(fp);
    }
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  std::cout << "we succeeded\n";

  return true;
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
