//
// Copyright (C) 2011 Leonardo Maccari, derived from Net80211.ned by Andras
// Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

package inet.examples.firewall;

import inet.networklayer.autorouting.ipv4.FlatNetworkConfigurator;
import inet.nodes.inet.AdhocHost;
import inet.world.radio.ChannelControl;
import inet.world.obstacles.ObstacleControl;
import inet.world.annotations.AnnotationManager;


network FirewallMeshNet
{
    parameters:
        int numHosts;
        int numMeshNodes = default(0);
        int gridHorizSize = default(0);
        int numAreas = default(0);
        bool placeMeshNodes = default(true);
//        @display("bgi=maps/povo600");        
    submodules:
        host[numHosts]: AdhocHost {
            parameters:
                @display("p=250,58");
                udpApp[*].routingTableRelativePath = "routingTable"; //  .
        }
        meshNode[numMeshNodes]: AdhocHost {
            parameters:
                @display("i=device/accesspoint");
                @display("p=350,58");
                mobilityType = placeMeshNodes ? "StaticGridMobility" : "StationaryMobility" ; 
                mobility.horizontalSize = gridHorizSize;
                mobility.marginX = default(75m);
				mobility.marginY = default(75m);
				mobility.numHosts = numMeshNodes;

        }
        channelControl: ChannelControl {
            parameters:
                @display("p=60,50");
        }
        configurator: FlatNetworkConfigurator {
            @display("p=140,50");
        }
        obstacles: ObstacleControl {
            @display("p=200,50");
        } 
        annotations: AnnotationManager { }

}

