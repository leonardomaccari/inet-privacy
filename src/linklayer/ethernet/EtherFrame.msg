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


cplusplus {{
#include "Ethernet.h"
#include "MACAddress.h"
#include "Ieee802Ctrl_m.h" // for enums
}}


class noncobject MACAddress;

class noncobject MessageId;

enum EtherType;

enum SAPCode;

// containing their common header fields.
//
// This class should never be instantiated by the models, only specific
// subclasses: EthernetJam, EtherIFG and submodules of ~EtherFrame.
packet EtherTraffic
{
}

//
// Represents jam on the Ethernet.
//
packet EtherJam extends EtherTraffic
{
}

//
// Represents padding when packet too short, and filled IFG on burst mode.
//
packet EtherPadding extends EtherTraffic
{
}

//
// Common base class for classes representing Ethernet II and 802.3 frame types,
// containing their common header fields.
//
// This class should never be instantiated by the models, only specific
// subclasses: ~EthernetIIFrame, ~EtherFrameWithLLC and ~EtherFrameWithSNAP.
//
// Source and destination MAC address are stored in data members.
// Other fields include:
//  - preamble: not stored (only contributes length)
//  - length: stored in cMessage length, not stored explicitly
//  - payload: stored a encapsulated packet (cMessage::encapsulate())
//  - crc: represented by cMessage::hasBitError()
//
packet EtherFrame extends EtherTraffic
{
    MACAddress dest;
    MACAddress src;
    int origByteLength;
    //MessageId id; -- uncomment for tracing frames
}


//
// Ethernet II headers contain a 16-bit ~EtherType to identify
// the encapsulated protocol.
//
// Header length: src(6)+dest(6)+etherType(2) = 14 bytes
//
packet EthernetIIFrame extends EtherFrame
{
    int etherType enum (EtherType);
}


//
// Ethernet frame with 802.3 LLC header.
//
// Header length: src(6)+dest(6)+length(2)+ssap(1)+dsap(1)+control(1) = 17 bytes
//
packet EtherFrameWithLLC extends EtherFrame
{
    int ssap;
    int dsap;
    int control;
}


//
// Ethernet frame with 802.3 LLC and SNAP headers.
//
// The fields ssap, dsap and control are not included in the class because
// they are always set to fixed values: 0xAA, 0xAA, 0x03.
//
// Header length: src(6)+dest(6)+length(2)+ssap(1)+dsap(1)+control(1)+
// orgCode(3)+localCode(2) = 22 bytes
//
packet EtherFrameWithSNAP extends EtherFrame
{
    long orgCode;  // SNAP organization code, 0 for IP and ARP
    int localcode; // SNAP local code (stores ~EtherType for IP and ARP)
}


//
// Ethernet frame used by the PAUSE protocol
//
packet EtherPauseFrame extends EtherFrame
{
    int pauseTime; // in 512 bit-time units
}
