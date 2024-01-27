// Copyright (c) 2014-2018, The Monero Project
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

import QtQuick 2.9
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2

import moneroComponents.I2PDManager 1.0
import moneroComponents.DaemonManager 1.0

import "../../components" as MoneroComponents

Rectangle {
    color: "transparent"
    Layout.fillWidth: true
    property alias networkHeight: networkLayout.height

    ColumnLayout {
        id: networkLayout
        Layout.fillWidth: true
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        anchors.topMargin: 0
        spacing: 30

        MoneroComponents.Label {
            id: networkTitleLabel
            fontSize: 24
            text: qsTr("Network traffic protection") + translationManager.emptyString
        }
        
        MoneroComponents.TextPlain {
            id: networkMainLabel
            text: qsTr("Your wallet communicates with a set node and other nodes on the\nMonero network. This communication can be used to identify you.\nUse the options below to protect your privacy. \n\nPlease check your local laws and internet policies before using I2P, a P2P distributed invisible internet. See more information at https://geti2p.net (http://i2p-projekt.i2p)") + translationManager.emptyString
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            font.family: MoneroComponents.Style.fontRegular.name
            font.pixelSize: 14
            color: MoneroComponents.Style.defaultFontColor
        }

        ColumnLayout {
            id: modeButtonsColumn
            Layout.topMargin: 8

            MoneroComponents.CheckBox {
                id: handleProtectButton
                text: qsTr("Protect my network connection") + translationManager.emptyString
                fontSize: 16
                checked: false
                onClicked: {
                    if (handleProtectButton.checked) {
                        startI2PD();
                    } else {
                        stopI2PD();
                    }
                }
            }
            MoneroComponents.CheckBox {
                id: handleProxyButton
                text: qsTr("Protect my network connection") + translationManager.emptyString
                fontSize: 16
                checked: false
                onClicked: {
                    if (handleProxyButton.checked) {
                        startI2PD();//todo: add functionality
                    } else {
                        startI2PD();
                    }
                }
            }
        }

        MoneroComponents.LineEditMulti{
            id: outproxyListLine
            visible: handleProxyButton.checked
            Layout.fillWidth: true
            labelFontSize: 14
            labelText: qsTr("Outproxy List") + translationManager.emptyString;
            placeholderFontSize: 16
            placeholderText: qsTr("list outproxies here") + translationManager.emptyString;
            readOnly: false
            wrapMode: Text.WrapAnywhere

        }
        MoneroComponents.LineEditMulti{
            id: nodeListLine
            visible: handleProxyButton.checked
            Layout.fillWidth: true
            labelFontSize: 14
            labelText: qsTr("Node List") + translationManager.emptyString;
            placeholderFontSize: 16
            placeholderText: qsTr("list nodes here") + translationManager.emptyString;
            readOnly: false
            wrapMode: Text.WrapAnywhere

        }

        RowLayout
        {
            MoneroComponents.Label {
                id: networkProtectionStatusLabel
                fontSize: 20
                text: qsTr("Status: ") + translationManager.emptyString
            }

            MoneroComponents.Label {
                id: networkProtectionStatus
                fontSize: 20
                text: qsTr("Unprotected") + translationManager.emptyString
            }
        }
    }

    function startI2PD()
    {
        //TODO: figure out how this is supposed to be used ??? 
        //var args = "--tx-proxy i2p,127.0.0.1:8060 --add-peer core5hzivg4v5ttxbor4a3haja6dssksqsmiootlptnsrfsgwqqa.b32.i2p --add-peer dsc7fyzzultm7y6pmx2avu6tze3usc7d27nkbzs5qwuujplxcmzq.b32.i2p --add-peer sel36x6fibfzujwvt4hf5gxolz6kd3jpvbjqg6o3ud2xtionyl2q.b32.i2p --add-peer yht4tm2slhyue42zy5p2dn3sft2ffjjrpuy7oc2lpbhifcidml4q.b32.i2p --anonymous-inbound XXXXXXXXXXXXXXXXXXXXXXXXXXXXX.b32.i2p,127.0.0.1:8061 --detach";
        var args = "";//note: args are applied to monerod; not i2pd
        if(handleProxyButton.checked){
            var args = "";//todo: figure out the actual args for this
        }else {
            var args = "";//and this
        }

        var noSync = false;
        //these args will be deleted because DaemonManager::start will re-add them later.
        //removing '--tx-proxy=i2p,...' lets us blindly add '--tx-proxy i2p,...' later without risking duplication.
        var defaultArgs = ["--detach","--data-dir","--bootstrap-daemon-address","--prune-blockchain","--no-sync","--check-updates","--non-interactive","--max-concurrency","--tx-proxy=i2p,127.0.0.1:8060"]
        var customDaemonArgsArray = args.split(' ');
        var flag = "";
        var allArgs = [];
        var i2pdArgs = ["--tx-proxy i2p,127.0.0.1:8060"];
        //create an array (allArgs) of ['--arg value','--arg2','--arg3']
        for (let i = 0; i < customDaemonArgsArray.length; i++) {
            if(!customDaemonArgsArray[i].startsWith("--")) {
                flag += " " + customDaemonArgsArray[i]
            } else {
                if(flag){
                    allArgs.push(flag)
                }
                flag = customDaemonArgsArray[i]
            }
        }
        allArgs.push(flag)
        //pop from allArgs if value is inside the deleteme array (defaultArgs)
        allArgs = allArgs.filter( ( el ) => !defaultArgs.includes( el.split(" ")[0] ) )
        //append required i2pd flags
        for (let i = 0; i < i2pdArgs.length; i++) {
            if(!allArgs.includes(i2pdArgs[i])) {
                allArgs.push(i2pdArgs[i])
                continue
            }
        }
        //daemonManager.stop();
        var success = true//daemonManager.start(allArgs.join(" "), persistentSettings.nettype, persistentSettings.blockchainDataDir, persistentSettings.bootstrapNodeAddress, noSync, persistentSettings.pruneBlockchain)
        if (success) {
            i2pdManager.start();
        }       
    }

    function stopI2PD()
    {
        i2pdManager.stop();
        //daemonManager.stop();
        //daemonManager.start("", persistentSettings.nettype, persistentSettings.blockchainDataDir, persistentSettings.bootstrapNodeAddress, noSync, persistentSettings.pruneBlockchain);//todo: get the "flag" value for parameter 1

    }

    function onI2PDStatus(isRunning)
    {
        
    }
    function debugI2PD() {
        i2pdManager.debug();
    }
    Component.onCompleted: {
        //i2pdManager.i2pdStatus.connect(onI2PDStatus);
    }
}
