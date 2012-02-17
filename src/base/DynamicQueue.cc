//
// Copyright (C) 2011 Leonardo Maccari
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
// You should have received a copy of the GNU  General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#include "DynamicQueue.h"
#include "IPv4Datagram.h"

/*
 * This queue model receives messages together with a parameter
 * specifying how long they have to be processed
 */

Define_Module(DynamicQueue);

cPacket *DynamicQueue::arrivalWhenIdle(cPacket *msg)
{
    return msg;
}

simtime_t DynamicQueue::startService(cPacket *msg)
{//	aggiungi il tempo di elaborazione dei dati in coda

	IPv4Datagram * datag = dynamic_cast<IPv4Datagram*>(msg);
	if (datag == 0)
		return 0;
	cArray parlist = datag->getParList();
	if (parlist["FirewallDelay"] == 0)
		return 0;
	else
		return simtime_t(check_and_cast<cMsgPar*>(parlist["FirewallDelay"])->doubleValue()); // seconds
}


void DynamicQueue::endService(cPacket * msg){
	send(msg, "queueOut");
}

// this function is called when a packet arrives and the queue is not empty

void DynamicQueue::arrival(cPacket * msg){
		queue.insert(msg);
}

