//
// Copyright (C) 2005 Andras Varga
// Copyright (C) 2010 Alfonso Ariza
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


#include <omnetpp.h>
#include "Ieee80211eClassifier.h"
#ifdef WITH_IPv4
  #include "IPv4Datagram.h"
  #include "ICMPMessage_m.h"
#endif
#ifdef WITH_IPv6
  #include "IPv6Datagram.h"
  #include "ICMPv6Message_m.h"
#endif
#ifdef WITH_UDP
  #include "UDPPacket_m.h"
#endif
#ifdef WITH_TCP_COMMON
  #include "TCPSegment.h"
#endif

Register_Class(Ieee80211eClassifier);

#define DEFAULT 3

Ieee80211eClassifier::Ieee80211eClassifier()
{
    defaultAC=DEFAULT;
}


int Ieee80211eClassifier::getNumQueues()
{
    return 4;
}

int Ieee80211eClassifier::classifyPacket(cMessage *msg)
{
    cPacket *ipData = NULL;  // must be initialized in case neither IPv4 nor IPv6 is present

#ifdef WITH_IPv4
    ipData = dynamic_cast<IPv4Datagram *>(PK(msg)->getEncapsulatedPacket());
    if (ipData && dynamic_cast<ICMPMessage *>(ipData->getEncapsulatedPacket()))
        return 1;  // ICMP class
#endif

#ifdef WITH_IPv6
    if (!ipData) {
        ipData = dynamic_cast<IPv6Datagram *>(PK(msg)->getEncapsulatedPacket());
        if (ipData && dynamic_cast<ICMPv6Message *>(ipData->getEncapsulatedPacket()))
            return 1; // ICMPv6 class
    }
#endif

    if (!ipData)
        return 3; // neither IPv4 nor IPv6 packet = class 3

#ifdef WITH_UDP
    UDPPacket *udp = dynamic_cast<UDPPacket *>(ipData->getEncapsulatedPacket());
    if (udp)
    {
        if (udp->getDestinationPort() == 5000 || udp->getSourcePort() == 5000)  //voice
           return 3;
        if (udp->getDestinationPort() == 4000 || udp->getSourcePort() == 4000)  //video
           return 2;
        if (udp->getDestinationPort() == 80 || udp->getSourcePort() == 80)  //voice
            return 1;
        if (udp->getDestinationPort() == 21 || udp->getSourcePort() == 21)  //voice
            return 0;
    }
#endif

#ifdef WITH_TCP_COMMON
    TCPSegment *tcp = dynamic_cast<TCPSegment *>(ipData->getEncapsulatedPacket());
    if (tcp)
    {
        if (tcp->getDestPort() == 80 || tcp->getSrcPort() == 80)
            return 1;
        if (tcp->getDestPort() == 21 || tcp->getSrcPort() == 21)
            return 0;
        if (tcp->getDestPort() == 4000 || tcp->getSrcPort() == 4000)
            return 2;
        if (tcp->getDestPort() == 5000 || tcp->getSrcPort() == 5000)
            return 3;
    }
#endif

    return defaultAC;
}


