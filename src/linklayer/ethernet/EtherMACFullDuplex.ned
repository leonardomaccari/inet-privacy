//
// Copyright (C) 2006 Levente Meszaros
// Copyright (C) 2010 Zoltan Bojthe
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


package inet.linklayer.ethernet;

import inet.linklayer.IEtherMAC;


//
// Ethernet MAC which supports full duplex operation ONLY.
// See the ~IEtherMAC for general informations.
//
// Most of today's Ethernet networks are switched, and operate
// in full duplex mode. Full-duplex transmission can be used for
// point-to-point connections only. Since full-duplex connections
// cannot be shared, collisions are eliminated. This setup eliminates
// most of the need for the CSMA/CD access control mechanism because
// there is no need to determine whether the connection is already
// being used. This allows for a much simpler simulation model
// for MAC. (In "traditional" Ethernet simulations, most of the code
// deals with the shared medium and the CSMA/CD mechanism.)
// ~EtherMACFullDuplex implements Ethernet without shared medium and CSMA/CD.
// (If you need half-duplex operation, see ~EtherMAC which is for a full-blown
// and therefore more complicated Ethernet MAC model.)
//
// ~EtherMACFullDuplex performs transmission and reception of frames.
// It does not do encapsulation/decapsulation; see ~EtherLLC and ~EtherEncap
// for that.
//
// Supported variations:
// - 10Mb Ethernet (full duplex mode)
// - 100Mb Ethernet (full duplex mode)
// - 1Gb Ethernet (full duplex mode)
//
// Supports all three Ethernet frame types. (It handles ~EtherFrame message class;
// specific frame classes (Ethernet-II, IEEE 802.3) are subclassed from that one.)
// RAW mode (only used by the IPX protocol) is not supported.
//
// <b>Operation</b>
//
// Processing of frames received from higher layers:
// - if src address in the frame is empty, fill it out
// - frames get queued up until transmission
// - transmit according to the CSMA/CD protocol
// - can send PAUSE message if requested by higher layers (PAUSE protocol,
//   used in switches).
//
// Processing of frames incoming from the network:
// - receive according to the CSMA/CD protocol
// - CRC checking (frames with the error bit set are discarded).
// - respond to PAUSE frames
// - in promiscuous mode, pass up all received frames;
//   otherwise, only frames with matching MAC addresses and
//   broadcast frames are passed up.
//
// The module does not perform encapsulation or decapsulation of frames --
// this is done by higher layers (~EtherLLC or ~EtherEncap).
//
// When a frame is received from the higher layers, it must be an ~EtherFrame,
// and with all protocol fields filled out
// (including the destination MAC address). The source address, if left empty,
// will be filled in. Then frame is queued and transmitted according
// to the CSMA/CD protocol.
//
// Data frames received from the network are EtherFrames. They are passed to
// the higher layers without modification.
// Also, the module properly responds to PAUSE frames, but never sends them
// by itself -- however, it transmits PAUSE frames received from upper layers.
// See <a href="ether-pause.html">PAUSE handling</a> for more info.
//
// <b>Disabling and disconnecting</b>
//
// If the MAC is not connected to the network ("cable unplugged"), it will
// start up in "disabled" mode. A disabled MAC simply discards any messages
// it receives. It is currently not supported to dynamically connect/disconnect
// a MAC.
//
//
// <b>Queueing</b>
//
// In routers, MAC relies on an external queue module (see ~IOutputQueue)
// to model finite buffer, implement QoS and/or RED, and requests packets
// from this external queue one-by-one.
//
// In hosts, no such queue is used, so MAC contains an internal
// queue named txQueue to queue up packets waiting for transmission.
// Conceptually, txQueue is of infinite size, but for better diagnostics
// one can specify a hard limit in the txQueueLimit parameter -- if this is
// exceeded, the simulation stops with an error.
//
//
// <b>Physical layer messaging</b>
//
// Please see <a href="physical.html">Messaging on the physical layer</a>.
//
// <b>Statistics</b>
//
// Output vectors:
// - txPkBytes: bytes sent, including Ethernet frame fields (but excluding preamble and SFD)
// - rxPkBytesOK: total bytes received  without collision or CRC error,
//   including Ethernet frame fields (but excluding preamble and SFD)
// - passedUpPkBytes: number of bytes of frames actually passed up to higher layer
// - txPausePkUnits: number of PAUSE frames sent out
// - rxPausePkUnits: number of PAUSE frames received from network
// - rxPkBytesFromHL: number of bytes of frames received from higher layer
// - droppedPkBytesIfaceDown: number of bytes of frames from higher layer dropped
// - droppedPkBytesBitError: number of bytes of frames dropped because of bit errors
// - droppedPkBytesNotForUs: number of bytes of frames dropped because destination address didn't match
//
// Output scalars (written in the finish() function) include the final values of
// the above variables and throughput.
//
// @see ~EthernetInterface, ~EthernetInterface, ~IOutputQueue, ~EtherEncap, ~EtherLLC
// @see ~EtherFrame, ~EthernetIIFrame, ~EtherFrameWithLLC, ~Ieee802Ctrl
//
simple EtherMACFullDuplex like IEtherMAC
{
    parameters:
        bool promiscuous = default(false);     // if true, all packets are received, otherwise only the
                                // ones with matching destination MAC address
        string address = default("auto");       // MAC address as hex string (12 hex digits), or
                                // "auto". "auto" values will be replaced by
                                // a generated MAC address in init stage 0.
        bool duplexEnabled = true;   // duplex mode can be enabled;
        int txQueueLimit = default(1000); // maximum number of frames queued up for transmission;
                                // additional frames are dropped. Only used if queueModule==""
        string queueModule = default("");    // name of optional external queue module
        bool frameBursting = default(true);     // enable/disable frame bursting mode in Gigabit Ethernet
        int mtu = default(1500);
        @display("i=block/rxtx");
        @statistic[txPkBytes](title="Sent bytes"; record=sum,count,vector);
        @statistic[rxPkBytesOk](title="Received bytes"; record=sum,count,vector);
        @statistic[droppedPkBytesIfaceDown](title="Dropped msg Iface down"; record=sum,count,vector);
        @statistic[droppedPkBytesBitError](title="Dropped bytes bit error"; record=sum,count,vector);
        @statistic[droppedPkBytesNotForUs](title="Dropped bytes not for us"; record=sum,count,vector);
        @statistic[passedUpPkBytes](title="Passed bytes to higher layer"; record=sum,count,vector);
        @statistic[rxPausePkUnits](title="Received pause"; record=count,stats,vector);
        @statistic[txPausePkUnits](title="Sent pause"; record=count,stats,vector);
        @statistic[rxPkBytesFromHL](title="Received bytes from higher layer"; record=sum,count,vector);
    gates:
        input upperLayerIn @labels(EtherFrame);    // to ~EtherLLC or ~MACRelayUnitPP
        output upperLayerOut @labels(EtherFrame);  // to ~EtherLLC or ~MACRelayUnitPP
        inout phys @labels(EtherFrame);            // to physical layer or the network
}
