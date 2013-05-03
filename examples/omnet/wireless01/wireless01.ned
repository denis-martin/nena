//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package nodearch.examples.wireless01;

import nodearch.examples.CSystemOmnet;

import inet.base.NotificationBoard;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.RoutingTable;
import inet.nodes.adhoc.MobileHost;
import inet.nodes.inet.NetworkLayer;
import inet.mobility.NullMobility;
import inet.mobility.BasicMobility;
import inet.world.ChannelControl;

import inet.linklayer.ieee80211.Ieee80211NicAdhoc;

//
// Mobile node with an IP-WLAN interface running the Node Architecture Daemon
//
module MobileNode
{
    parameters:
        @node;
        @display("bgb=295,239");
        bool IPForward = default(false);
        string routingFile = default("");
        string mobilityType = default("NullMobility");

    gates:
        input radioIn @directIn;

    submodules:
        notificationBoard: NotificationBoard {
            parameters:
                @display("p=170,48");
        }

        interfaceTable: InterfaceTable {
            parameters:
                @display("p=170,116");
        }

        routingTable: RoutingTable {
            parameters:
                IPForward = IPForward;
                routerId = "";
                routingFile = routingFile;
                @display("p=170,185");
        }

        nodeArchDaemon: CSystemOmnet {
            parameters:
                @display("p=58,48");
            gates:
                eth[1]; // one gate only
        }

        networkLayer: NetworkLayer {
            parameters:
                proxyARP = false;
                @display("p=58,116;q=queue");
            gates:
                ifIn[1+1]; //1 for nodeArchDaemon.eth and 1 for wlan
                ifOut[1+1];
        }

        wlan: Ieee80211NicAdhoc {
            parameters:
                @display("p=58,185");
        }

        mobility: <mobilityType> like BasicMobility {
            parameters:
                @display("p=260,48");
        }

    connections allowunconnected:
        radioIn --> wlan.radioIn;
        // TODO: we might want to interface directly with the MAC layer
        wlan.uppergateOut --> networkLayer.ifIn[1];
        wlan.uppergateIn <-- networkLayer.ifOut[1];
        networkLayer.udpOut --> nodeArchDaemon.eth$i[0];
        networkLayer.udpIn <-- nodeArchDaemon.eth$o[0];
}

//
// Node Architecture Daemon running on an Access Point
//
//module BaseStation
//{
//}

//
// Wireless scenario with one Access Point and several mobile nodes
// (using the inet framework).
//
network Net1_INetWifi
{
    parameters:
        double playgroundSizeX = default(600);
        double playgroundSizeY = default(400);

    submodules:
        MobileNodes[3]: MobileNode {
            @display("p=71,177");
        }
        //        AccessPoint: BaseStation {
        //            @display("p=71,100");
        //        }
        channelcontrol: ChannelControl {
            playgroundSizeX = playgroundSizeX;
            playgroundSizeY = playgroundSizeY;
            @display("p=71,35");
        }
}
