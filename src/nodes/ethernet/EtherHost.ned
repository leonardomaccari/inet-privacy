//
// Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//



package inet.nodes.ethernet;

import inet.networklayer.queue.IOutputQueue;
import inet.linklayer.ethernet.EtherLLC;
import inet.linklayer.IEtherMAC;
import inet.applications.ethernet.EtherAppSrv;
import inet.applications.ethernet.EtherAppCli;


//
// An example host with one Ethernet port and traffic generators that
// generate Ethernet traffic directly. This host model does not contain
// higher layer protocols (IP, TCP). By default is configured to use
// half duplex MAC (CSMA/CD).
//
module EtherHost
{
    parameters:
        bool csmacdSupport = default(true);  // by default use CSMA/CD 
        string macType = default(csmacdSupport ? "EtherMAC" : "EtherMACFullDuplex"); // ~EtherMAC or ~EtherMACFullDuplex
        string queueType = default("NoQueue"); // DropTailQueue, DropTailQoSQueue, REDQueue, NoQueue
        @node();
        @labels(node,ethernet-node);
        @display("i=device/pc2");
    gates:
        inout ethg @labels(EtherFrame-conn);
    submodules:
        queue: <queueType> like IOutputQueue if queueType != "NoQueue" {
            parameters:
                @display("p=99,201;q=l2queue");
        }
        cli: EtherAppCli {
            parameters:
                registerSAP = true;
                @display("p=60,60,col");
        }
        srv: EtherAppSrv {
            parameters:
                registerSAP = true;
                @display("p=250,60,col");
        }
        llc: EtherLLC {
            parameters:
                @display("p=155,120");
            gates:
                upperLayerIn[2];
                upperLayerOut[2];
        }
        mac: <macType> like IEtherMAC {
            parameters:
                queueModule = (queueType == "NoQueue" ? "" : "queue");
                @display("p=155,272;q=queue");
        }
    connections:
        mac.phys <--> ethg;

        cli.out --> llc.upperLayerIn[0];
        cli.in <-- llc.upperLayerOut[0];

        srv.out --> llc.upperLayerIn[1];
        srv.in <-- llc.upperLayerOut[1];
        llc.lowerLayerOut --> queue.in if queueType != "NoQueue";
        queue.out --> mac.upperLayerIn if queueType != "NoQueue";
        llc.lowerLayerOut --> mac.upperLayerIn if queueType == "NoQueue";
        mac.upperLayerOut --> llc.lowerLayerIn;
}
