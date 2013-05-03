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

package nodearch.examples.wireless04_adhocRouting;

import nodearch.examples.CSystemOmnet;

// import org.mf2.contrib.mobility.ConstSpeedMobility;
import org.mf2.contrib.nic.Nic80211;
import org.mf2.core.blackboard.Blackboard;
import org.mf2.core.channelControl.ChannelControl;
import org.mf2.contrib.mobility.BonnMotionMobility;

//
// Mobile node with an IP-WLAN interface running the Node Architecture Daemon
//
module MobileNode
{
    parameters:
        @node;
        @display("bgb=295,239");

    gates:
        input radioIn @directIn;

    submodules:
        blackboard: Blackboard {
            parameters:
                @display("p=170,48");
        }

        nodeArchDaemon: CSystemOmnet {
            parameters:
                @display("p=58,48");
            gates:
                eth[1]; // one gate only
        }

        nic: Nic80211 {
            parameters:
                @display("p=58,185");
        }

	bonn: BonnMotionMobility {
	    parameters:
		@display("p=170,185");
	}

//        mobility: ConstSpeedMobility {
//           parameters:
//                @display("p=260,48");
//        }

    connections allowunconnected:
        radioIn --> nic.radioIn;
        nic.uppergateOut --> nodeArchDaemon.eth$i[0];
        nic.uppergateIn <-- nodeArchDaemon.eth$o[0];
        nic.upperControlOut --> nodeArchDaemon.lowerControlIn;
}

//
// Wireless ad-hoc scenario
// (using the mobility framework framework).
//
network Net1_MFWifi
{
    parameters:
        double playgroundSizeX = default(1000);
        double playgroundSizeY = default(1000);

    submodules:
	host[14]: MobileNode {
            @display("p=500,177");
        }

        channelcontrol: ChannelControl {
            playgroundSizeX = playgroundSizeX;
            playgroundSizeY = playgroundSizeY;
            @display("p=500,35");
        }
}
