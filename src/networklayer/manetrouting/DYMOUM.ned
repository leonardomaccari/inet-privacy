//
//   Copyright (C) 2009 by Alfonso Ariza
//   Malaga University 
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

package inet.networklayer.manetrouting;

import inet.networklayer.manetrouting.base.BaseRouting;
import inet.networklayer.IManetRouting;


//
// Dynamic MANET On-demand (DYMO) Routing
//
// Implements draft-ietf-manet-dymo-05.
//
// NOTE:
//  - The initialization of the routing component is done in stage=4
//  - This implementation is a bit outdated. As of 2010-09-16 the 
//    current draft number is 21 
//
simple DYMOUM extends BaseRouting like IManetRouting
{
    parameters:
        @class(DYMOUM);
        @reactive;                             // IP module will send control messages when no route is present to the destination 
        bool no_path_acc_ = default(false);  // Disable path accumulation
        bool reissue_rreq_ = default(false); // Try more route discoveries when it fails the first time
        bool s_bit_ = default(false);        // Set S-bit of DYMO header
        int hello_ival_ = default(0);        // (s) hello sending interval. 0 means no hello and uses link layer feedback instead
        int MaxPktSec = default(10);         // limit the rate of dymo packets per sec (max 50/sec)
        int NetDiameter = default(10);       // diameter of the network
        int RouteTimeOut = default(5000);    // timeout for routing entries (ms)
        int RouteDeleteTimeOut = default(5000*5); // 5*RouteTimeOut
        int RREQWaitTime = default(1000);    // (ms) max wait time for a route request response
        int RREQTries = default(3);          // Rreq reties before fail and sending unreachable

        // Experimental
        bool promiscuous = default(false); // experimental: promiscuous mode is used to discover neighborhood nodes 
        int noRouteBehaviour = default(1); // experimental: 0:send ICMP(unreachable) 1:send RErr 2: Send both RErr and ICMP 3:initiate local route discovery
        bool intermediateRREP = default(false); // experimental: allow intermediate nodes to reply instead of taget node (see draft7 sect 5.3.3)
        bool RREQattachPacket = default(false); // experimental: whether to attach data packet to the RReq message

        bool EqualDelay = default(true);       // add a bit of delay (jitter) tor brocast operations (RREQ, HELLO broadcast) (non RFC - omnetpp specific)
        volatile double broadCastDelay @unit("s") = default(uniform(0s,0.005s));  // the delay added to broadcast operations if EqualDelay is set (used to model processing time) 
        volatile double uniCastDelay @unit("s") = default(0s);  // a delay added to unicast messaged (i.e. data packet forwarding) (used to model processing time)
        bool UseIndex = default(false);         // use the interface index instead the ip to identify the interface
    gates:
        input from_ip;
        output to_ip;
}


