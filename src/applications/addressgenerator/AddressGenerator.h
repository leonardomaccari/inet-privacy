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
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#ifndef __INET_ADDRESSGENERATOR_H
#define __INET_ADDRESSGENERATOR_H

#include "omnetpp.h"
#include "IPvXAddress.h"
#include "RoutingTable.h"


class  AddressGenerator : public cSimpleModule
{
protected:

	std::string generatorMode;
	// The way addressess are gathered:
	// - RoutingTable: get them from your routing table (useful in
	// ad-hoc networks)
	// - FixedList: get them from the configuration
	// - NodeType: specify a node type and get any of them
	int listSize;
	// size of the list to be returned.
	bool keepSetConsistent;
	// try to keep time consistency of the list among updates (not just random IP taken by routes),
	bool keepSetBalanced;
	// try to keep topology consistency of the list among updates. In group mobility models
	// the keepSetConsistent will probably produce sets with a lot of one-hop nodes
	// since they are always reachable. This tries to proportionally spread the target set
	// over the possible hopcount in the network.

	std::string addressList;
	// needed for FixedList mode
	std::string nodeType;
	// needed for NodeType mode
	std::string routingTablePath;
	bool refreshList;


	int waitTime;
	int updateTime;
	RoutingTable * routingTable;
	static simsignal_t listSizeSignal;
	std::map<IPv4Address, int> destAddresses;
	std::map<IPv4Address, int> tmpList;

	cOutVector changedTargetsRatio;
	cOutVector avgHopCount;


protected:
	virtual int numInitStages() const {return 4;}
	virtual void initialize(int stage);
	virtual void handleMessage(cMessage *msg);
	void randomPurge(int size);
	void consistentPurge(int size);
	void balancedPurge(int size);
	void statsGenerator(std::map<IPv4Address, int>& oldList);
	virtual void finish();
};

#endif

