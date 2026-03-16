// Copyright (c) 2014-2024, The Monero Project
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
//    used to endorse or promote products derived from this software or specific
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

#include "I2PManager.h"

#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QDebug>

#include "api.h"
#include "ClientContext.h"
#include "Identity.h"
#include "common/util.h"

static const char I2PD_CONF[] =
    "[sam]\n"
    "enabled = true\n"
    "port = 7656\n"
    "\n"
    "[socksproxy]\n"
    "enabled = true\n"
    "port = 4447\n"
    "\n"
    "[httpproxy]\n"
    "enabled = false\n"
    "\n"
    "[http]\n"
    "enabled = false\n"
    "\n"
    "[log]\n"
    "enabled = false\n";

// Dedicated port for i2pd to forward incoming I2P P2P connections to monerod.
// Must not conflict with clearnet P2P (18080), RPC (18081), or ZMQ (18083).
// Must match the port in --anonymous-inbound passed to monerod (see main.qml).
static const char TUNNELS_CONF[] =
    "[monerod]\n"
    "type = server\n"
    "host = 127.0.0.1\n"
    "port = 18085\n"
    "keys = monerod.dat\n";

I2PManager::I2PManager(QObject *parent)
    : QObject(parent)
{
}

I2PManager::~I2PManager()
{
    stop();
}

void I2PManager::initDataDir(const QString &i2pDir) const
{
    // i2pd.conf: write only if absent — users may add custom settings.
    if (!QFile::exists(i2pDir + "/i2pd.conf")) {
        QFile f(i2pDir + "/i2pd.conf");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
            f.write(I2PD_CONF);
    }
    // tunnels.conf: always overwrite — entirely managed by monero-gui.
    // This ensures the port stays in sync with --anonymous-inbound (18085).
    QFile t(i2pDir + "/tunnels.conf");
    if (t.open(QIODevice::WriteOnly | QIODevice::Text))
        t.write(TUNNELS_CONF);
}

QString I2PManager::loadOrCreateKeypair(const QString &i2pDir) const
{
    const QString keyPath = i2pDir + "/monerod.dat";
    i2p::data::PrivateKeys keys;

    QFile f(keyPath);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        const QByteArray data = f.readAll();
        f.close();
        if (keys.FromBuffer(reinterpret_cast<const uint8_t *>(data.constData()), data.size()) == 0) {
            qWarning() << "I2PManager: failed to parse existing keypair, regenerating";
            f.remove();
        } else {
            return QString::fromStdString(keys.GetPublic()->GetIdentHash().ToBase32()) + ".b32.i2p";
        }
    }

    keys = i2p::data::PrivateKeys::CreateRandomKeys(i2p::data::SIGNING_KEY_TYPE_EDDSA_SHA512_ED25519);
    const size_t len = keys.GetFullLen();
    std::vector<uint8_t> buf(len);
    keys.ToBuffer(buf.data(), len);

    if (f.open(QIODevice::WriteOnly)) {
        f.write(reinterpret_cast<const char *>(buf.data()), static_cast<qint64>(len));
        f.close();
    } else {
        qCritical() << "I2PManager: failed to write keypair to" << keyPath;
    }

    return QString::fromStdString(keys.GetPublic()->GetIdentHash().ToBase32()) + ".b32.i2p";
}

bool I2PManager::start(const QString &dataDir)
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_running) {
            return true;
        }
    }

    const QString base = dataDir.isEmpty()
        ? QString::fromStdString(tools::get_default_data_dir())
        : dataDir;
    const QString i2pDir = base + "/i2pd";
    if (!QDir().mkpath(i2pDir)) {
        emit startFailure(tr("Failed to create I2P data directory: %1").arg(i2pDir));
        return false;
    }

    initDataDir(i2pDir);
    const QString b32 = loadOrCreateKeypair(i2pDir);

    // Keep strings alive for the duration of InitI2P
    const std::string dir     = i2pDir.toStdString();
    const std::string conf    = dir + "/i2pd.conf";
    const std::string tunconf = dir + "/tunnels.conf";
    const char *argv[] = { "monero-gui", "--datadir", dir.c_str(), "--conf", conf.c_str(), "--tunconf", tunconf.c_str(), nullptr };
    i2p::api::InitI2P(7, const_cast<char **>(argv), "monero-gui");
    i2p::api::StartI2P();
    i2p::client::context.Start(); // starts SAM bridge, SOCKS5 proxy, and server tunnels

    {
        QMutexLocker locker(&m_mutex);
        m_running = true;
        m_b32 = b32;
    }
    emit b32AddressChanged(b32);
    emit runningChanged(true);
    return true;
}

void I2PManager::stop()
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_running) {
            return;
        }
        m_running = false;
    }
    i2p::client::context.Stop(); // stops SAM bridge, SOCKS5 proxy, and server tunnels
    i2p::api::StopI2P();
    i2p::api::TerminateI2P();
    emit runningChanged(false);
}

bool I2PManager::isRunning() const
{
    QMutexLocker locker(&m_mutex);
    return m_running;
}

QString I2PManager::b32Address() const
{
    QMutexLocker locker(&m_mutex);
    return m_b32;
}
