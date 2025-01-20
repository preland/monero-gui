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
#include "common/util.h"
#include "curl_effective_url.h"
#include <cstddef>
#include <curl/curl.h>
#include <curl/easy.h>
#include<filesystem>
#include <fstream>
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
    static const int RETRY_DELAY = 1;
}
size_t write_data_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
size_t write_data_str(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t totalSize = size * nmemb;
    std::string *buffer = static_cast<std::string *>(stream);
    buffer->append(static_cast<char *>(ptr), totalSize);
    return totalSize;
}

/*bool I2PManager::enableI2P(bool enable, const QString &flags, int mode) {*/
/*  QStringList monerod_args;*/
/*  //for now we are just going to shutdown ungracefully*/
/*  exit();*/
/*  if(enable){*/
/*    if(!start(flags))*/
/*      return false;*/
/*    //stop monerod here*/
/*    switch (mode) {*/
/*      case 0: //simple*/
/*        break;*/
/*      case 1: //simple (bootstrap)*/
/*        break;*/
/*      case 2: //advanced*/
/*        monerod_args = QStringList ()*/
/*        << "--proxy=127.0.0.1:4447"*/
/*        << "--daemon-address=" << getAddress().c_str()*/
/*        << "--trusted-daemon";*/
/*        break;*/
/*      default:*/
/*        break;*/
/*    }*/
/*  }*/
/*  //restart monerod here*/
/*  return true;*/
/*}*/
std::string I2PManager::getFlags(int mode) {
  std::string monerod_args("");
  switch (mode) {
    case 0: //simple
      break;
    case 1: //simple (bootstrap)
      break;
    case 2: //advanced
      monerod_args += std::string("--proxy=127.0.0.1:4447") + "--daemon-address=" + getAddress(5) + "--trusted-daemon";
      break;
    default:
      break;
  }
  return monerod_args;
}
bool I2PManager::start(const QString &flags){
          qDebug() << getAddress(1).c_str();
  if(!checkI2PInstalled()) {
    return false;
  }

  exit();
  sleep(1); //wait for exit before continuing

  QStringList arguments = QStringList() 
    << "--conf" << m_i2pd_config 
    << "--tunconf" << m_i2pd_tunconf
    << "--tunnelsdir" << m_i2pd_tunnelsdir
    << "--certsdir" << m_i2pd_certsdir;

//custom flags
  foreach (const QString &str, flags.split(" ")) {
          if (!str.isEmpty())
            arguments << str;
    }
  qDebug() << "starting i2pd " + m_i2pd_executable;
  qDebug() << "With command line arguments " << arguments;
  QMutexLocker locker(&m_daemonMutex);
  m_daemon.reset(new QProcess());

    // Connect output slots
  connect(m_daemon.get(), SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
  connect(m_daemon.get(), SIGNAL(readyReadStandardError()), this, SLOT(printError()));

  // Start i2pd 
  bool started = m_daemon->startDetached(m_i2pd_executable, arguments);

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
  //should use SIGINT to stop i2pd gracefully; use SIGKILL in other cases
  //not sure how to do this in windows though...
  qDebug() << "Killing i2pd";
#ifdef Q_OS_WIN
  QProcess::execute("taskkill",  {"/F", "/IM", "i2pd.exe"});
#else
  QProcess::execute("pkill", {"i2pd"});
#endif

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
    qDebug() << "STATE CHANGED: " << state;
    if (state == QProcess::NotRunning) {
        emit daemonStopped();
    }
}

void I2PManager::checkI2PVersion() {
  if(!checkI2PInstalled()) {
    m_version = "Not Installed";
    emit versionTextChanged();
    return;
  }
  QStringList arguments = {"--version"};
  /*if(m_version.length() <= 0) {*/
    QProcess *proc = new QProcess();
    proc->start(m_i2pd_executable, arguments);
    //if this takes longer than 5 seconds it will give up
    proc->waitForFinished(5000);
    m_version = proc->readAllStandardOutput().mid(13, 6);
    qDebug() << m_version << "!!!";
  /*}*/
  emit versionTextChanged();
  return;
}
bool I2PManager::checkI2PInstalled() {
  if(QFileInfo(m_i2pd_executable).isFile()) {
    return true;
  }
  return false;
}
bool I2PManager::I2PInstall() {
  //todo: fix this below
  std::string download_folder = QApplication::applicationDirPath().toStdString() + "/i2pd/downloaded";
  QDir().mkdir(download_folder.c_str());
  /*if(checkI2PInstalled()) {*/
    //todo: add reinstall functionality
    /*return false;*/
  /*}*/
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
  std::string outfilename = download_folder + sep + filename;
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_file);

    /*fp = fopen(outfilename.c_str(),"wb");*/
    /*if(fp) {*/
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      res = curl_easy_perform(curl);
      /*fclose(fp);*/
    /*}*/
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  
  //extract data.tar.xz

  std::string arCmd = "ar x " + outfilename + " data.tar.xz";
  if (std::system(arCmd.c_str()) != 0) {
    qCritical() << "Error extracting data.tar.xz";
    return false;
  }
  //extract from data.tar.xz
  std::string tarCmd = "tar -xf data.tar.xz -C " + QApplication::applicationDirPath().toStdString() + "/i2pd/downloaded/";
  if (std::system(tarCmd.c_str()) != 0) {
    qCritical() << "Error extracting files from data.tar.xz";
    return false;
  }
  //move executable to main directory
  tools::copy_file(QApplication::applicationDirPath().toStdString() + "/i2pd/downloaded/usr/bin/i2pd", QApplication::applicationDirPath().toStdString() + "/i2pd/i2pd");
  //add config files: could be downloaded from repo?
  write_file(QApplication::applicationDirPath().toStdString() + "/i2pd/i2pd.conf", "\
loglevel = none\n\
ipv4 = true\n\
ipv6 = false\n\
[ntcp2]\n\
enabled = true\n\
[ssu2]\n\
enabled = true\n\
[httpproxy]\n\
enabled = false\n\
[sam]\n\
enabled = false");
  write_file(QApplication::applicationDirPath().toStdString() + "/i2pd/tunnels.conf", "\
[monero-node]\n\
type = server\n\
host = 127.0.0.1\n\
# Anonymous inbound port\n\
port = 18085\n\
inport = 0\n\
keys = monero-mainnet.dat\n\
\n\
[monero-rpc]\n\
type = server\n\
host = 127.0.0.1\n\
# Restricted RPC port\n\
port = 18089\n\
keys = monero-mainnet.dat");
  /*write_file(QApplication::applicationDirPath().toStdString() + "/i2pd/i2pd.conf", "\*/
  /*    ");*/
  /*std::cout << "we succeeded\n";*/
  //update version info
  checkI2PVersion(); 

  return true;
}
int is_server_online(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize libcurl\n");
        return 0;
    }

    CURLcode res;
    int online = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Connection timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Overall timeout
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); // Treat HTTP errors as failures

    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        online = 1; // Server is online
    } else {
        fprintf(stderr, "Server check failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return online;
}
std::string I2PManager::getAddress(int max_retries) {
  int retries = 0;
  CURL *curl;
  FILE *fp;
  CURLcode res;
  std::string readBuffer = "";
  std::string tunnelurl = "http://127.0.0.1:7070/?page=i2p_tunnels";
  std::cout << tunnelurl << std::endl;
  //std::string outfilename = m_i2pd_download_dir.toStdString() + m_sep.toStdString() + "idkwhatthisis";
  //std::cout << outfilename << std::endl;

  curl_global_init(CURL_GLOBAL_ALL);
  while(retries < max_retries) {
    if (is_server_online(tunnelurl.c_str())) {
      printf("Server is online! Proceeding with the request.\n");
      break;
    }
    printf("Server is offline. Retrying in %d second(s)...\n", RETRY_DELAY);
    sleep(RETRY_DELAY); // Delay before retrying
    retries++;
  }
  curl = curl_easy_init();
  if (curl) {

    /* Switch on full protocol/debug output while testing */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* disable progress meter, set to 0L to enable and disable debug output */ 
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Transfer timeout: 10 seconds
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Connection timeout: 5 seconds

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_URL, tunnelurl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_str);

    //fp = fopen(outfilename.c_str(),"wb");
    //if(fp) {
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    /*  fclose(fp);*/
    /*}*/
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  
/*"<div class="listitem"><a href="/?page=local_destin*/
/*ation&b32={URL}">SOCKS Proxy<"}*/
  const std::string prefix = "page=local_destination&b32=";
  const std::string suffix = "\">SOCKS Proxy";
  size_t startPos = readBuffer.find(prefix);
  if (startPos == std::string::npos) return "failed@curl_parse1";
  startPos += prefix.length();
  size_t endPos = readBuffer.find(suffix);
  if (endPos == std::string::npos) return "failed@curl_parse2";

  std::string addr_front = readBuffer.substr(startPos, endPos - startPos);

  //todo: ensure url is valid b32 address

  std::string addr = addr_front + ".b32.i2p:18089";
  //QString address = QString(addr.c_str());
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return "failed@curl_easy_perform";
  } else {
    std::cout << addr << "\n";
    return addr;
  }
}
I2PManager::I2PManager(QObject *parent)
    : QObject(parent)
    , m_scheduler(this)
{

    // Platform dependent path to i2pd files
    // todo: is the tunneldir and certsdir necessary
#ifdef Q_OS_WIN
    m_i2pd_dir = QApplication::applicationDirPath() + "\\i2pd";
    m_i2pd_executable = m_i2pd_dir + "TODOOOOO";
    m_i2pd_config = m_i2pd_dir + "TODOOOOO";
    m_i2pd_tunconf = m_i2pd_dir + "TODOOOOO";
    //m_i2pd_tunnelsdir = m_i2pd_dir + "TODOOOO";
    //m_i2pd_certsdir = m_i2pd_dir + "TODOOOO";
#endif
#ifdef Q_OS_UNIX
    m_sep = "/";
    m_i2pd_dir = QApplication::applicationDirPath() + "/i2pd";
    m_i2pd_download_dir = m_i2pd_dir + "/download";
    m_i2pd_executable = m_i2pd_dir + "/i2pd";
    m_i2pd_config = m_i2pd_dir + "/i2pd.conf";
    m_i2pd_tunconf = m_i2pd_dir + "/tunnels.conf";
    //m_i2pd_tunnelsdir = m_i2pd_dir + "/etc/i2pd/tunnels.conf.d";
    //m_i2pd_certsdir = m_i2pd_dir + "/usr/share/i2pd/certificates";
#endif

    if (m_i2pd_dir.length() == 0) {
        qCritical() << "no i2p binary defined for current platform";
    }
}

I2PManager::~I2PManager()
{

}
