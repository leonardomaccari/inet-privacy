//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 Andras Varga
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


#include "UDPBasicApp.h"
#include "UDPControlInfo_m.h"
#include "IPvXAddressResolver.h"


Define_Module(UDPBasicApp);

int UDPBasicApp::counter;
simsignal_t UDPBasicApp::sentPkSignal = SIMSIGNAL_NULL;
simsignal_t UDPBasicApp::rcvdPkSignal = SIMSIGNAL_NULL;

void UDPBasicApp::initialize(int stage)
{
    // because of IPvXAddressResolver, we need to wait until interfaces are registered,
    // address auto-assignment takes place etc.
    if (stage != 3)
        return;

    counter = 0;
    numSent = 0;
    numReceived = 0;
    numUnsent = 0;
    WATCH(numSent);
    WATCH(numReceived);
    sentPkSignal = registerSignal("sentPk");
    rcvdPkSignal = registerSignal("rcvdPk");

    localPort = par("localPort");
    destPort = par("destPort");

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    IPvXAddress myAddr = IPvXAddressResolver().resolve(this->getParentModule()->getFullPath().c_str());
    while ((token = tokenizer.nextToken()) != NULL)
    {
        if (strstr(token, "Broadcast") != NULL)
            destAddresses.push_back(IPv4Address::ALLONES_ADDRESS);
        else
        {
            IPvXAddress addr = IPvXAddressResolver().resolve(token);
            if (addr != myAddr)
                destAddresses.push_back(addr);
        }
    }
    if (destAddresses.size() == 0){
    	addressGeneratorModule = dynamic_cast<AddressGenerator*>(getParentModule()->getModuleByRelativePath("addressGenerator"));
    	if (addressGeneratorModule == 0)
    		error("Wrong path to the address generator!?!?");
    }

    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort);

    stopTime = par("stopTime").doubleValue();
    simtime_t startTime = par("startTime").doubleValue();
    if (stopTime != 0 && stopTime <= startTime)
        error("Invalid startTime/stopTime parameters");

    cMessage *timerMsg = new cMessage("sendTimer");
    if (startTime >= 0) // do not start if startTime < 0
    	scheduleAt(startTime, timerMsg);
}

void UDPBasicApp::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    recordScalar("packets unsent", numUnsent);

}

IPvXAddress UDPBasicApp::chooseDestAddr()
{
	if (destAddresses.size() == 1)
	        return destAddresses[0];
	else if(destAddresses.size() != 0){
		int k = intrand(destAddresses.size());
		return destAddresses[k];
	}

	std::map<IPv4Address, int> currentList = addressGeneratorModule->gatherAddresses();
	if (currentList.empty())
		return IPvXAddress();
	int rnd = uniform(0,currentList.size());
	std::map<IPv4Address, int>::iterator ii = currentList.begin();
	while (rnd > 0){
		ii++;
		rnd--;
	}
	return ii->first;
}

cPacket *UDPBasicApp::createPacket()
{
    char msgName[32];
    sprintf(msgName, "UDPBasicAppData-%d", counter++);

    cPacket *payload = new cPacket(msgName);
    payload->setByteLength(par("messageLength").longValue());
    return payload;
}

void UDPBasicApp::sendPacket()
{
    IPvXAddress destAddr = chooseDestAddr();
    if (destAddr.isUnspecified()){
    	numUnsent++;
    	return;
    }
    cPacket *payload = createPacket();
    emit(sentPkSignal, payload);
    socket.sendTo(payload, destAddr, par("destPort").longValue());
    numSent++;
}

void UDPBasicApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // send, then reschedule next sending
        sendPacket();
        simtime_t d = simTime() + par("sendInterval").doubleValue();
        if (stopTime == 0 || d < stopTime)
            scheduleAt(d, msg);
        else
            delete msg;
    }
    else if (msg->getKind() == UDP_I_DATA)
    {
        // process incoming packet
        processPacket(PK(msg));
    }
    else if (msg->getKind() == UDP_I_ERROR)
    {
        EV << "Ignoring UDP error report\n";
        delete msg;
    }
    else
    {
        error("Unrecognized message (%s)%s", msg->getClassName(), msg->getName());
    }

    if (ev.isGUI())
    {
        char buf[40];
        sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
        getDisplayString().setTagArg("t", 0, buf);
    }
}

void UDPBasicApp::processPacket(cPacket *pk)
{
    emit(rcvdPkSignal, pk);
    EV << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;
    delete pk;
    numReceived++;
}

