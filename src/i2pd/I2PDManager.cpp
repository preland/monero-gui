// Copyright (c) 2014-2022, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "I2PDManager.h"

#include "../../external/i2pd/libi2pd/Config.h"
#include "../../external/i2pd/libi2pd/FS.h"
#include "../../external/i2pd/libi2pd/Base.h"
#include "../../external/i2pd/libi2pd/version.h"
#include "../../external/i2pd/libi2pd/Transports.h"
#include "../../external/i2pd/libi2pd/RouterInfo.h"
#include "../../external/i2pd/libi2pd/RouterContext.h"
#include "../../external/i2pd/libi2pd/Tunnel.h"
#include "../../external/i2pd/libi2pd/HTTP.h"
#include "../../external/i2pd/libi2pd/NetDb.hpp"
#include "../../external/i2pd/libi2pd/Garlic.h"
#include "../../external/i2pd/libi2pd/Streaming.h"
#include "../../external/i2pd/libi2pd/Destination.h"
#include "../../external/i2pd/libi2pd_client/ClientContext.h"
#include "../../external/i2pd/libi2pd/Crypto.h"

#include "../../external/i2pd/daemon/Daemon.h"

#include <thread>
#include <vector>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include <boost/thread.hpp>

I2PDManager::I2PDManager(QObject *parent)
    : QObject(parent)
{
    std::cout << QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString() << "\n";
#ifdef Q_OS_WIN
    m_i2pdDataDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/i2pd";
    if (!QDir(m_i2pdDataDir).exists()) {
        QDir().mkpath(m_i2pdDataDir);
    }
#elif defined(Q_OS_UNIX)
    m_i2pdDataDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/i2pd";
    std::cout << m_i2pdDataDir.toStdString() << "\n";
    if (!QDir(m_i2pdDataDir).exists()) {
        QDir().mkpath(m_i2pdDataDir);
    }
#endif
    if (m_i2pdDataDir.length() == 0) {
        qCritical() << "I2PD not compatible with current platform";
    }

    std::string dataDir = "--datadir=" + m_i2pdDataDir.toStdString();
    std::vector<const char*> argv({"i2pd", dataDir.data()});
    init(argv.size(), (char**) argv.data());
}

I2PDManager::~I2PDManager()
{

}

void I2PDManager::init(int argc, char* argv[])
{
    Daemon.init(argc,argv);
    //i2p::util::DaemonLinux::Instance()
    /*i2p::config::Init();
    i2p::config::ParseCmdline(argc, argv);
    std::string datadir;
    i2p::config::GetOption("datadir", datadir);
    i2p::fs::DetectDataDir(datadir, false);
    i2p::fs::Init();
    i2p::config::ParseConfig(datadir + "/i2pd.conf");
    i2p::config::Finalize();

    std::string certsdir; i2p::config::GetOption("certsdir", certsdir);
    i2p::fs::SetCertsDir(certsdir);
    certsdir = i2p::fs::GetCertsDir();//todo: ask if this actually makes sense
    std::cout << "certsdir: " << certsdir << "\n";
    

    bool precomputation; i2p::config::GetOption("precomputation.elgamal", precomputation);
    bool aesni; i2p::config::GetOption("cpuext.aesni", aesni);
    bool avx; i2p::config::GetOption("cpuext.avx", avx);
    bool forceCpuExt; i2p::config::GetOption("cpuext.force", forceCpuExt);
    bool ssu; i2p::config::GetOption("ssu", ssu);
    if (!ssu && i2p::config::IsDefault ("precomputation.elgamal"))
        precomputation = false; // we don't elgamal table if no ssu, unless it's specified explicitly
    i2p::crypto::InitCrypto(precomputation, aesni, avx, forceCpuExt);

    i2p::transport::InitAddressFromIface(); // get address4/6 from interfaces

    int netID; i2p::config::GetOption("netid", netID);
    i2p::context.SetNetID(netID);
    i2p::context.Init();

    i2p::transport::InitTransports();

    bool isFloodfill; i2p::config::GetOption("floodfill", isFloodfill);
    if (isFloodfill)
    {
        i2p::context.SetFloodfill (true);
    }
    else
        i2p::context.SetFloodfill (false);

    bool transit; i2p::config::GetOption("notransit", transit);
    i2p::context.SetAcceptsTunnels (!transit);
    uint16_t transitTunnels; i2p::config::GetOption("limits.transittunnels", transitTunnels);
    if (isFloodfill && i2p::config::IsDefault ("limits.transittunnels"))
        transitTunnels *= 2; // double default number of transit tunnels for floodfill
    i2p::tunnel::tunnels.SetMaxNumTransitTunnels (transitTunnels);

    // this section also honors 'floodfill' flag, if set above 
    std::string bandwidth; i2p::config::GetOption("bandwidth", bandwidth);
    if (bandwidth.length () > 0)
    {
        if (bandwidth[0] >= 'K' && bandwidth[0] <= 'X')
        {
            i2p::context.SetBandwidth (bandwidth[0]);
        }
        else
        {
            auto value = std::atoi(bandwidth.c_str());
            if (value > 0)
            {
                i2p::context.SetBandwidth (value);
            }
            else
            {
                i2p::context.SetBandwidth (i2p::data::CAPS_FLAG_LOW_BANDWIDTH2);
            }
        }
    }
    else if (isFloodfill)
    {
        i2p::context.SetBandwidth (i2p::data::CAPS_FLAG_EXTRA_BANDWIDTH2);
    }
    else
    {
        i2p::context.SetBandwidth (i2p::data::CAPS_FLAG_LOW_BANDWIDTH2);
    }

    int shareRatio; i2p::config::GetOption("share", shareRatio);
    i2p::context.SetShareRatio (shareRatio);

    std::string family; i2p::config::GetOption("family", family);
    i2p::context.SetFamily(family);
    m_isRunning = false;*/
}
void I2PDManager::start()
{
    m_daemon.reset(new QProcess());

    m_daemon->startDetached(m_monerod, "");
        //std::cout << "we here\n";
        /*boost::thread i2pd ([a = &Daemon](){
            if(a->start()) {
                a->run();
            }
        });*/
        //Daemon.run();
    /*
    if (!isRunning()) {
        i2p::log::Log().Start();
		i2p::data::netdb.Start();

		bool nettime; i2p::config::GetOption("nettime.enabled", nettime);
		if (nettime)
		{
			m_NTPSync = std::unique_ptr<i2p::util::NTPTimeSync>(new i2p::util::NTPTimeSync);
			m_NTPSync->Start();
		}

		bool ntcp2; i2p::config::GetOption("ntcp2.enabled", ntcp2);
		bool ssu2; i2p::config::GetOption("ssu2.enabled", ssu2);
		bool checkInReserved; i2p::config::GetOption("reservedrange", checkInReserved);

		i2p::transport::transports.SetCheckReserved(checkInReserved);
		i2p::transport::transports.Start(ntcp2, ssu2);
		if (!(i2p::transport::transports.IsBoundSSU2() || i2p::transport::transports.IsBoundNTCP2()))
		{
			i2p::transport::transports.Stop();
			i2p::data::netdb.Stop();
			return;
		}

		i2p::tunnel::tunnels.Start();
		i2p::context.Start();
		i2p::client::context.Start();
        m_isRunning = true;
    }*/
}

void I2PDManager::stop()
{
    Daemon.stop();
    /*if (isRunning()) {
		i2p::client::context.Stop();
		i2p::context.Stop();
		i2p::tunnel::tunnels.Stop();

		if (m_NTPSync)
		{
			m_NTPSync->Stop ();
			m_NTPSync = nullptr;
		}

		i2p::transport::transports.Stop();
		i2p::data::netdb.Stop();
		i2p::crypto::TerminateCrypto();
        m_isRunning = false;
    }*/
}

bool I2PDManager::isRunning() const
{
    return m_isRunning;
    //return false;//todo: implement
}
