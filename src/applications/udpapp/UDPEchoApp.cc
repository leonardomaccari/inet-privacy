//
// Copyright (C) 2005 Andras Babos
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


#include "UDPEchoApp.h"

#include "UDPEchoAppMsg_m.h"
#include "UDPControlInfo_m.h"


Define_Module(UDPEchoApp);

simsignal_t UDPEchoApp::roundTripTimeSignal = SIMSIGNAL_NULL;
simsignal_t UDPEchoApp::sentAnswerBytesSignal = SIMSIGNAL_NULL;
int UDPEchoApp::requestsSent;
int UDPEchoApp::repliesReceived;
int UDPEchoApp::repliesSent;
int UDPEchoApp::globalUnsent;

void UDPEchoApp::initialize(int stage)
{
    UDPBasicApp::initialize(stage);

    roundTripTimeSignal = registerSignal("roundTripTime");
//    sentAnswerBytesSignal = registerSignal("sentAnswerBytes");
    requestsSent = 0;
    repliesReceived = 0;
    repliesSent = 0;
    globalUnsent = 0;
    copyReceived = 0;
    WATCH_SET(receivedReplies);
    WATCH_SET(repliedRequests);

}


/*
 *
 *IPvXAddress UDPEchoApp::chooseDestAddr()
{
    if (destAddresses.size() == 1)
        return destAddresses[0];
    return destAddresses[intrand(destAddresses.size())];
}
*/

cPacket *UDPEchoApp::createPacket()
{
    char msgName[32];
    sprintf(msgName, "UDPEcho-%d", counter++);
    UDPEchoAppMsg *message = new UDPEchoAppMsg(msgName);
    message->setCounter(counter);
    message->setByteLength(par("messageLength").longValue());
    requestsSent++;
    return message;
}

void UDPEchoApp::processPacket(cPacket *msg)
{
    if (msg->getKind() == UDP_I_ERROR)
    {
        delete msg;
        return;
    }

    UDPEchoAppMsg *packet = check_and_cast<UDPEchoAppMsg *>(msg);
    emit(rcvdPkSignal, packet);

    if (packet->getIsRequest())
    {
        // send back as response

    	if (repliedRequests.find(packet->getCounter()) != repliedRequests.end()){
    		copyReceived++;
    		delete packet;
    		return;
    	}
    	repliedRequests.insert(packet->getCounter());
    	if (repliedRequests.size() > CACHE_SIZE)
    		repliedRequests.erase(repliedRequests.begin());
        packet->setIsRequest(false);
        UDPDataIndication *ctrl = check_and_cast<UDPDataIndication *>(packet->removeControlInfo());
        IPvXAddress srcAddr = ctrl->getSrcAddr();
        int srcPort = ctrl->getSrcPort();
        delete ctrl;
        std::string newName(packet->getName());
        newName.append("-Reply");
        packet->setName(newName.c_str());
        emit(sentAnswerBytesSignal, (long)(packet->getByteLength()));
        repliesSent++; // static
        numSent++; // update also UDPBasicApp counter
        emit(sentPkSignal, packet);
        socket.sendTo(packet, srcAddr, srcPort);
    }
    else
    {
        // response packet: compute round-trip time
    	if (receivedReplies.find(packet->getCounter()) != receivedReplies.end()){
    	    		delete packet;
    	    		return;
    	    	}
    	receivedReplies.insert(packet->getCounter());
      	if (receivedReplies.size() > CACHE_SIZE)
        		receivedReplies.erase(receivedReplies.begin());
        simtime_t rtt = simTime() - packet->getCreationTime();
        EV << "RTT: " << rtt << "\n";
        emit(roundTripTimeSignal, rtt);
        repliesReceived++; // static
        delete msg;
    }
    numReceived++;
}
void UDPEchoApp::finish()
{
	UDPBasicApp::finish();
    recordScalar("RequestsSent", requestsSent);
    recordScalar("RepliesSent", repliesSent);
    recordScalar("Copies received", copyReceived);
    recordScalar("RepliesReceived", repliesReceived);
    recordScalar("Global packets arrived ratio", (double)repliesSent/(double)requestsSent);
    recordScalar("Global packets acked ratio", (double)repliesReceived/(double)requestsSent);
    globalUnsent += numUnsent;
    recordScalar("RequestsUnSent", globalUnsent);
}

