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


#include "AddressGenerator.h"
#include "RoutingTable.h"


#include "IPvXAddressResolver.h"

Define_Module(AddressGenerator);

simsignal_t AddressGenerator::listSizeSignal = SIMSIGNAL_NULL;

void AddressGenerator::initialize(int stage)
{
	// because of IPvXAddressResolver, we need to wait until interfaces are registered,
	// address auto-assignment takes place etc.
	if (stage != 3)
		return;

	listSizeSignal = registerSignal("listSize");
	listSize = par("listSize").longValue();
	updateTime = par("updateTime").longValue();

	avgHopCount.setName("Average hop count in Address list");
	changedTargetsRatio.setName("Fraction of changed addresses from one generation to another");

	routingTablePath = this->getParentModule()->getFullPath();

	generatorMode = par("generatorMode").stdstringValue();
	WATCH_MAP(destAddresses);

	if (generatorMode.compare(std::string("RoutingTable")) == 0){
		keepSetConsistent = par("keepSetConsistent");
		keepSetBalanced = par("keepSetBalanced");
		if (listSize == -1){
			EV << "Ignoring  keepSetConsistent/keepSetBalanced option since you did not specify a listSize option";
			keepSetConsistent = false;
			keepSetBalanced = false;
		}
		if (keepSetBalanced && keepSetConsistent)
			error("Can not keep the set consistent and balanced at the same time, see keepSetConsistent/keepSetBalanced options in ned");

		scheduleAt(simTime() + par("waitTime").longValue(), new cMessage("refresh Address list"));
		if (par("routingTablePath").stdstringValue().compare(std::string("")) != 0)
			routingTablePath.append(".").append(par("routingTablePath").stdstringValue());
		else
			routingTablePath.append(".").append(std::string("routingTable"));
		WATCH(routingTablePath);
		cModule * module = simulation.getModuleByPath(routingTablePath.c_str());
		if (module == 0)
			opp_error("The routing table path is wrong!");
		routingTable = static_cast<RoutingTable*>(module);

		destAddresses = routingTable->gatherRoutes(routingTable->getRouterId());
		if (par("nodeType").stdstringValue().compare(std::string("")) != 0){
			nodeType = par("nodeType").stdstringValue();
			chooseNodesByType();
			typePurge(destAddresses);
		}
	}
	else if (generatorMode.compare(std::string("FixedList")) == 0 ){
		if (par("addressList").stdstringValue().compare(std::string("")) == 0)
			opp_error("You need to configure an address list if you use mode FixedList");
		const char *destAddrs = par("addressList").stringValue();

		cStringTokenizer tokenizer(destAddrs);
		const char *token;

		while ((token = tokenizer.nextToken()) != 0 )
		{
			if ( strstr (token,"Broadcast")!=NULL)
				destAddresses[IPv4Address::ALLONES_ADDRESS] = 0; // FIXME what if this is IPv6?
			else
				destAddresses[IPvXAddressResolver().resolve(token).get4()] = 0;
		}
	}
	else if (generatorMode.compare(std::string("NodeType")) == 0 ){
		if (par("nodeType").stdstringValue().compare(std::string("")) == 0)
			opp_error("You need to specify a NodeType option (e.g. 'host')  if you use mode NodeType");
		nodeType = par("nodeType").stdstringValue();
		chooseNodesByType();
		destAddresses = chosenTypeList;

	} else
		opp_error("Wrong nodeType chose, see ned file for right values");

	if (listSize != -1)
		randomPurge(listSize);
}

void AddressGenerator::chooseNodesByType(){
	int myId = (this)->getParentModule()->getIndex();
	std::string myType = std::string((this)->getName());
	cModule * typeArray = (this)->getParentModule()->getParentModule()->getSubmodule(nodeType.c_str(),0);
	if (typeArray != 0 && typeArray->isVector()){
		for (int i=0; i<typeArray->getVectorSize(); i++)
		{
			if (!(i == myId && (myType.compare(nodeType) == 0))){ // if it's not this module
				std::stringstream nodeId;
				nodeId << (this)->getParentModule()->getParentModule()->getFullPath() << "." << nodeType << "[" << i << "]";
				chosenTypeList[IPvXAddressResolver().resolve(nodeId.str().c_str()).get4()] = 0;
			}
		}
	} else {
		std::stringstream tmp;
		tmp <<  "This module expects a node structure of the kind network.node[].addressgenerator. There is no array with path network." << nodeType <<"[]";
		error(tmp.str().c_str());
	}
}

void AddressGenerator::finish()
{
}

// remove elements from the map in random order
void AddressGenerator::randomPurge(int size){
	if (destAddresses.size() <= size)
		return;
	int rnd, i;
	std::map<IPv4Address, int>::iterator ii = destAddresses.begin();
	while (destAddresses.size() > size)
	{
		rnd = intuniform(0,destAddresses.size()-1);
		ii = destAddresses.begin();
		for (i = 0 ; i < rnd; i++)
			ii++;
		destAddresses.erase(ii);
	}
}

// remove elements from the map in random order
void AddressGenerator::typePurge(std::map<IPv4Address, int> &destMap){
	if (nodeType.compare(std::string("")) == 0)
		return;

	std::map<IPv4Address, int>::iterator ii = destMap.begin();
	for (;ii != destMap.end(); )
		{
			if(chosenTypeList.find(ii->first) != chosenTypeList.end())
				ii++;
			else
				destMap.erase(ii++); // previous destination is not available anymore
		}
}
// try to keep the list consistent with time, not randomic at every refresh
void AddressGenerator::consistentPurge(int size){
	int rnd, i;
	std::map<IPv4Address, int>::iterator ii = destAddresses.begin();
	for (;ii != destAddresses.end(); )
	{
		std::map<IPv4Address, int>::iterator jj = tmpList.find(ii->first);

		if (jj != tmpList.end()){
			ii->second = jj->second; // update metric
			ii++;
		} else {
			destAddresses.erase(ii++); // previous destination is not available anymore
		}
	}
	while (destAddresses.size() < listSize){ // fill the remaining list space with other nodes, different from present ones
		rnd = intuniform(0,tmpList.size()-1);
		ii = tmpList.begin();
		for (i = 0 ; i < rnd; i++)
			ii++;
		destAddresses[ii->first] = ii->second;
		tmpList.erase(ii);
	}

	// insert here code to make the address list evolve (randomically change one of the targets once in a while)

}
void AddressGenerator::balancedPurge(int size){
	std::map<IPv4Address, int>::iterator ii = tmpList.begin();
	float ratio = (float)size/tmpList.size();

	if(ratio < 1){
		std::multimap<int,IPv4Address> tmpMap;
		std::map<int,std::vector<IPv4Address> > hCountCounter; // map with the hosts for any hcount

		for(; ii!= tmpList.end(); ii++){
			// this is reordering the reachable
			// hosts for their hopcount from this node, which randomizes
			// enough the set
			hCountCounter[ii->second].push_back(ii->first);
		}
		// now rescale the target set to a new map with the same proportion
		// between hcounts than the visible network (as much as possible). Since rescaling with
		// little integers may be problematic (i.e. scaling from 100 nodes grouped into 7 sets to
		// only, for instance, 3 nodes makes the proportion wrong anyway and will just fill the set
		// with the first ones you scan), we start filling by the largest sets.

		int hCountSetMax = 0;
		int hCountSetMaxIndex = 0;

		while(hCountCounter.size() != 0 && tmpMap.size() < size){

			std::map<int,std::vector<IPv4Address> >::iterator jj = hCountCounter.begin();
			for (; jj!= hCountCounter.end(); jj++) // just find the biggest set among sets of n-hop neighbors
				if (jj->second.size() > hCountSetMax)
					hCountSetMaxIndex = jj->first;
			int reducedSize = ceil((float)hCountCounter[hCountSetMaxIndex].size()*ratio); // howmuch the set gets shrinked
			hCountCounter[hCountSetMaxIndex].resize(reducedSize); // resize to new size
			for (std::vector<IPv4Address>::iterator kk = hCountCounter[hCountSetMaxIndex].begin();
					kk != hCountCounter[hCountSetMaxIndex].end(); kk++){
				tmpMap.insert(std::make_pair<int,IPv4Address>(hCountSetMaxIndex, *kk));
			}
			hCountCounter.erase(hCountSetMaxIndex);

		}
		destAddresses.clear();
		std::multimap<int,IPv4Address>::iterator hh = tmpMap.begin();
		for(;hh != tmpMap.end(); hh++)
			destAddresses[hh->second] = hh->first;

		// now the set is reduced to the correct size ane proportional to the
		// original set. This code below is added to produce a bias
		// for a certain hopcount
		/*
			float fourth = (float)reduceTargets/(float)4;

			int k = normal(fourth*3,1);

			// the targets are ordered by their hopcount. Get one of them with standard gaussian distribution
			// centered in the 3/4 of the array. (bias far connections)


			std::map<int, IPv4Address>::iterator hh = tmpMap.begin();
				for (int i=0; i<k; i++){
						hh++;
				}
//				std::cout << " -- Target " << hh->second <<std::endl;
				return hh->second;

		}
		 */

	}
}

void AddressGenerator::statsGenerator(std::map<IPv4Address, int>& oldList)
{
	if (oldList.empty()  || destAddresses.empty())
		return;
    std::map<IPv4Address, int>::iterator ii = destAddresses.begin();
    double changedEntries = 0;
	double hopAvg = 0;
	for (; ii!=destAddresses.end(); ii++){
		if (oldList.find(ii->first) == oldList.end())
			changedEntries++;
		hopAvg += ii->second;
	}
	changedTargetsRatio.record(changedEntries/oldList.size());
	avgHopCount.record(hopAvg/destAddresses.size());
}

void AddressGenerator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        scheduleAt(simTime()+updateTime, msg);
    tmpList = routingTable->gatherRoutes(routingTable->getRouterId());
    std::map<IPv4Address, int> oldList = destAddresses;
	typePurge(tmpList);
    if (keepSetConsistent && tmpList.size() >= listSize)
    	consistentPurge(listSize);
    else if (keepSetBalanced && tmpList.size() >= listSize)
    	balancedPurge(listSize);
    else
    	destAddresses = tmpList;
    statsGenerator(oldList);
}

