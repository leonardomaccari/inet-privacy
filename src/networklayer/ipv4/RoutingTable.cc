//
// Copyright (C) 2004-2006 Andras Varga
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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


//  Cleanup and rewrite: Andras Varga, 2004

#include <algorithm>
#include <sstream>

#include "RoutingTable.h"

#include "IInterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "IPv4InterfaceData.h"
#include "IPv4Route.h"
#include "NotificationBoard.h"
#include "NotifierConsts.h"
#include "RoutingTableParser.h"


Define_Module(RoutingTable);

#define LOGTIME 1
simsignal_t RoutingTable::routingTableSize = SIMSIGNAL_NULL;
simsignal_t RoutingTable::firewallTableSize = SIMSIGNAL_NULL;
simsignal_t RoutingTable::hopCount = SIMSIGNAL_NULL;
simsignal_t RoutingTable::ruleSetDistribution = SIMSIGNAL_NULL;
simsignal_t RoutingTable::inputRulesMatch = SIMSIGNAL_NULL;
simsignal_t RoutingTable::outputRulesMatch = SIMSIGNAL_NULL;
simsignal_t RoutingTable::outputRulesMiss= SIMSIGNAL_NULL;
simsignal_t RoutingTable::forwardRulesMiss= SIMSIGNAL_NULL;
simsignal_t RoutingTable::forwardRulesMatch= SIMSIGNAL_NULL;
simsignal_t RoutingTable::postRoutingMiss= SIMSIGNAL_NULL;
simsignal_t RoutingTable::postRoutingMatch= SIMSIGNAL_NULL;

int RoutingTable::inputMatched;
int RoutingTable::forwardMissed;
int RoutingTable::forwardMatched;
int RoutingTable::outputMissed;
int RoutingTable::outputMatched;
int RoutingTable::globalSent;
int RoutingTable::globalReceived;
int RoutingTable::postRoutingMatched;
int RoutingTable::postRoutingMissed;
int RoutingTable::toBeFiltered;
int RoutingTable::numLogged;

Define_Module(RoutingTable);


std::ostream& operator<<(std::ostream& os, const IPv4Route& e)
{
    os << e.info();
    return os;
};

std::ostream& operator<<(std::ostream& os, const IPv4RouteRule& e)
{
    os << e.info();
    return os;
};


RoutingTable::RoutingTable()
{
 // DSDV
    timetolive_routing_entry = timetolive_routing_entry.getMaxTime();
}

RoutingTable::~RoutingTable()
{
    for (unsigned int i=0; i<routes.size(); i++)
        delete routes[i];
    for (unsigned int i=0; i<multicastRoutes.size(); i++)
        delete multicastRoutes[i];
    while (!inputRules.empty())
    {
        delete inputRules.back();
        inputRules.pop_back();
    }
    while (!outputRules.empty())
    {
        delete outputRules.back();
        outputRules.pop_back();
    }
    if(strategy){
    	delete strategy;
    }
    cancelAndDelete(updateStats);

}

void RoutingTable::initialize(int stage)
{
    if (stage==0)
    {
        // get a pointer to the NotificationBoard module and IInterfaceTable
        nb = NotificationBoardAccess().get();
        ift = InterfaceTableAccess().get();

        IPForward = par("IPForward").boolValue();

        nb->subscribe(this, NF_INTERFACE_CREATED);
        nb->subscribe(this, NF_INTERFACE_DELETED);
        nb->subscribe(this, NF_INTERFACE_STATE_CHANGED);
        nb->subscribe(this, NF_INTERFACE_CONFIG_CHANGED);
        nb->subscribe(this, NF_INTERFACE_IPv4CONFIG_CHANGED);
        inputMatched = 0;
        outputMatched = 0;
        outputMissed = 0;
        forwardMissed = 0;
        forwardMatched = 0;
        globalSent = 0;
        globalReceived = 0;
        postRoutingMatched = 0;
        postRoutingMissed = 0;
        toBeFiltered = 0;
        numLogged = 0;
        routeChanged = false;
        rSetSize.setName("Enforced ruleset size");
        strategy = firewallStrategy::getStrategy(par("firewallStrategy"), routes, outputRules);
        strategy->setMaxSize(par("firewallRuleSetSize"));
        strategy->routingTable = this;
        strategy->setMPRThreshold(par("firewallMPRThreshold"));
        perf = strategy->stringToPerf(par("nodePerformance").stdstringValue());
        perfArray = par("perfArray").xmlValue();
        strategy->fillPerfMap(perfArray, par("nodePerformance").stdstringValue());
        cacheHit = 0;
        cacheMiss = 0;

        WATCH_PTRVECTOR(routes);
        WATCH_PTRVECTOR(multicastRoutes);
        WATCH_PTRVECTOR(outputRules);
        WATCH_PTRVECTOR(inputRules);
        WATCH_PTRVECTOR(forwardRules);
        WATCH_PTRVECTOR(globalRules);
        WATCH_PTRVECTOR(postRoutingRules);

        WATCH(IPForward);
        WATCH(routerId);
        WATCH(globalReceived);
        routingTableSize = registerSignal("routingTableSize");
        firewallTableSize = registerSignal("firewallTableSize");
        hopCount= registerSignal("hopCount");
        ruleSetDistribution= registerSignal("ruleSetDistribution");
        inputRulesMatch = registerSignal("inputRulesMatch");
        outputRulesMatch = registerSignal("outputRulesMatch");
        outputRulesMiss = registerSignal("outputRulesMiss");
        postRoutingMatch = registerSignal("postRoutingRulesMatch");
        postRoutingMiss = registerSignal("postRoutingRulesMiss");

    }
    else if (stage==1)
    {
        // L2 modules register themselves in stage 0, so we can only configure
        // the interfaces in stage 1.
        const char *filename = par("routingFile");

        // At this point, all L2 modules have registered themselves (added their
        // interface entries). Create the per-interface IPv4 data structures.
        IInterfaceTable *interfaceTable = InterfaceTableAccess().get();
        for (int i=0; i<interfaceTable->getNumInterfaces(); ++i)
            configureInterfaceForIPv4(interfaceTable->getInterface(i));
        configureLoopbackForIPv4();

        // read routing table file (and interface configuration)
        RoutingTableParser parser(ift, this);
        if (*filename && parser.readRoutingTableFromFile(filename)==-1)
            error("Error reading routing table file %s", filename);

        // set routerId if param is not "" (==no routerId) or "auto" (in which case we'll
        // do it later in stage 3, after network configurators configured the interfaces)
        const char *routerIdStr = par("routerId").stringValue();
        if (strcmp(routerIdStr, "") && strcmp(routerIdStr, "auto"))
            routerId = IPv4Address(routerIdStr);
    }
    else if (stage==3)
    {
        // routerID selection must be after stage==2 when network autoconfiguration
        // assigns interface addresses
        configureRouterId();

        // we don't use notifications during initialize(), so we do it manually.
        // Should be in stage=3 because autoconfigurator runs in stage=2.
        updateNetmaskRoutes();
        const char *filename = par("firewallFile");
        double probFirewallOn = par("probFirewallOn");
        if (uniform(0,1) > probFirewallOn)
        	firewallOn = false;
        else
        	firewallOn = true;


        RoutingTableParser parser(ift, this);
        if (*filename && parser.readRoutingTableFromFile(filename)==-1)
            error("Error reading firewall rules file %s", filename);
        for(RuleSetMap::iterator ii = ruleMap.begin();
        		ii != ruleMap.end(); ii++){
        	RoutingRule &RuleSet =  ii->second; // this is just to have more meaningful
        									   // names in the GUI for the watched vectors
        	WATCH_PTRVECTOR(RuleSet);
        }

        updateStats = new cMessage("updateStats");
        scheduleAt(simTime()+1, updateStats);
        //printRoutingTable();
    }
}

void RoutingTable::configureRouterId()
{
    if (routerId.isUnspecified())  // not yet configured
    {
        const char *routerIdStr = par("routerId").stringValue();
        if (!strcmp(routerIdStr, "auto"))  // non-"auto" cases already handled in stage 1
        {
            // choose highest interface address as routerId
            for (int i=0; i<ift->getNumInterfaces(); ++i)
            {
                InterfaceEntry *ie = ift->getInterface(i);
                if (!ie->isLoopback() && ie->ipv4Data()->getIPAddress().getInt() > routerId.getInt())
                    routerId = ie->ipv4Data()->getIPAddress();
            }
        }
    }
    else // already configured
    {
        // if there is no interface with routerId yet, assign it to the loopback address;
        // TODO find out if this is a good practice, in which situations it is useful etc.
        if (getInterfaceByAddress(routerId)==NULL)
        {
            InterfaceEntry *lo0 = ift->getFirstLoopbackInterface();
            lo0->ipv4Data()->setIPAddress(routerId);
            lo0->ipv4Data()->setNetmask(IPv4Address::ALLONES_ADDRESS);
        }
    }
}

void RoutingTable::updateDisplayString()
{
    if (!ev.isGUI())
        return;

    char buf[80];
    if (routerId.isUnspecified())
        sprintf(buf, "%d+%d routes", (int)routes.size(), (int)multicastRoutes.size());
    else
        sprintf(buf, "routerId: %s\n%d+%d routes", routerId.str().c_str(), (int)routes.size(), (int)multicastRoutes.size());
    getDisplayString().setTagArg("t", 0, buf);
}

void RoutingTable::handleMessage(cMessage *msg)
{
	if (msg->isSelfMessage())
	{
	    emit(routingTableSize,getNumRoutes());
	    scheduleAt(simTime()+LOGTIME, msg);

	    if(strncmp(par("firewallStrategy").stringValue(),"NONE",strlen("NONE")) != 0 ){
	    	for (RoutingRule::iterator ii = outputRules.begin();
	    			ii!=outputRules.end(); ii++){
	    		(*ii)->hitAvg = 0;
	    		for (std::vector<int>::iterator jj = (*ii)->hitQueue.begin();
	    				jj != (*ii)->hitQueue.end(); jj++)
	    		{
	    			(*ii)->hitAvg += *jj;
	    		}
	    		(*ii)->hitAvg = (*ii)->hitAvg/HITQUEUE_SIZE;
	    		(*ii)->hitQueue.erase((*ii)->hitQueue.begin());
	    		(*ii)->hitQueue.push_back(0);
	    	}
	    }
	    for (int i=0; i<getNumRoutes(); i++){
	    	if(!getRoute(i)->getInterface()->isLoopback()){ // exclude loopback
	    		emit(hopCount, getRoute(i)->getMetric());
//#define DEBUG
#ifdef DEBUG
	    		std::cout << "XXX " << getRoute(i)->getHost() <<  " " << (int)(getRoute(i)->getRuleSet()) << std::endl;
	        	emit(ruleSetDistribution, (int)(getRoute(i)->getRuleSet()));
	        	// use some script-fu to assure that ruleSet distribution is as expected,
	        	// somwthing like: ./run -c TestFirewall -u Cmdenv | grep XXX | sort | uniq
#endif
	    	}
	    }
	}else
		throw cRuntimeError(this, "This module doesn't process messages");
}

void RoutingTable::receiveChangeNotification(int category, const cObject *details)
{
    if (simulation.getContextType()==CTX_INITIALIZE)
        return;  // ignore notifications during initialize

    Enter_Method_Silent();
    printNotificationBanner(category, details);

    if (category==NF_INTERFACE_CREATED)
    {
        // add netmask route for the new interface
        updateNetmaskRoutes();
    }
    else if (category==NF_INTERFACE_DELETED)
    {
        // remove all routes that point to that interface
        InterfaceEntry *entry = check_and_cast<InterfaceEntry*>(details);
        deleteInterfaceRoutes(entry);
    }
    else if (category==NF_INTERFACE_STATE_CHANGED)
    {
        invalidateCache();
    }
    else if (category==NF_INTERFACE_CONFIG_CHANGED)
    {
        invalidateCache();
    }
    else if (category==NF_INTERFACE_IPv4CONFIG_CHANGED)
    {
        // if anything IPv4-related changes in the interfaces, interface netmask
        // based routes have to be re-built.
        updateNetmaskRoutes();
    }
}

void RoutingTable::deleteInterfaceRoutes(InterfaceEntry *entry)
{
    RouteVector::iterator it = routes.begin();
    while (it != routes.end())
    {
        IPv4Route *route = *it;
        if (route->getInterface() == entry)
        {
            deleteRoute(route);
            it = routes.begin();  // iterator became invalid -- start over
        }
        else
        {
            ++it;
        }
    }
}

void RoutingTable::invalidateCache()
{
    routingCache.clear();
    localAddresses.clear();
}

void RoutingTable::printRoutingTable() const
{
    EV << "-- Routing table --\n";
    ev.printf("%-16s %-16s %-16s %-3s %s\n",
              "Destination", "Gateway", "Netmask", "Iface");

    for (int i=0; i<getNumRoutes(); i++)
        EV << getRoute(i)->detailedInfo() << "\n";
    EV << "\n";
}

std::vector<IPv4Address> RoutingTable::gatherAddresses() const
{
    std::vector<IPv4Address> addressvector;

    for (int i=0; i<ift->getNumInterfaces(); ++i)
        addressvector.push_back(ift->getInterface(i)->ipv4Data()->getIPAddress());
    return addressvector;
}


std::map<IPv4Address, int> RoutingTable::gatherRoutes(IPv4Address myAddr) const
{

	std::map<IPv4Address,int> targetSet;
	for (RouteVector::const_iterator i=routes.begin(); i!=routes.end(); ++i){

		// get a list of unique reachable addresses that are not local and are
		// not the one specified as a parameter (sometimes, manet routing creates
		// loops, so that in the routing table there is a route to this node that
		// is not DIRECT).
		if((*i)->getType() != IPv4Route::DIRECT && !(*i)->getHost().isUnspecified()
				&& (!myAddr.isUnspecified() && myAddr != (*i)->getHost() )){
			std::map<IPv4Address,int>::iterator jj = targetSet.find((*i)->getHost());
			if(jj == targetSet.end() || (jj->second > (*i)->getMetric()))
				targetSet[(*i)->getHost()] = (*i)->getMetric();
		}
	}
	return targetSet;
}

//---

void RoutingTable::configureInterfaceForIPv4(InterfaceEntry *ie)
{
    IPv4InterfaceData *d = new IPv4InterfaceData();
    ie->setIPv4Data(d);

    // metric: some hints: OSPF cost (2e9/bps value), MS KB article Q299540, ...
    d->setMetric((int)ceil(2e9/ie->getDatarate())); // use OSPF cost as default
}

InterfaceEntry *RoutingTable::getInterfaceByAddress(const IPv4Address& addr) const
{
    Enter_Method("getInterfaceByAddress(%u.%u.%u.%u)", addr.getDByte(0), addr.getDByte(1), addr.getDByte(2), addr.getDByte(3)); // note: str().c_str() too slow here

    if (addr.isUnspecified())
        return NULL;
    for (int i=0; i<ift->getNumInterfaces(); ++i)
    {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->ipv4Data()->getIPAddress()==addr)
            return ie;
    }
    return NULL;
}


void RoutingTable::configureLoopbackForIPv4()
{
    InterfaceEntry *ie = ift->getFirstLoopbackInterface();

    // add IPv4 info. Set 127.0.0.1/8 as address by default --
    // we may reconfigure later it to be the routerId
    IPv4InterfaceData *d = new IPv4InterfaceData();
    d->setIPAddress(IPv4Address::LOOPBACK_ADDRESS);
    d->setNetmask(IPv4Address::LOOPBACK_NETMASK);
    d->setMetric(1);
    ie->setIPv4Data(d);
}

//---

bool RoutingTable::isLocalAddress(const IPv4Address& dest) const
{
    Enter_Method("isLocalAddress(%u.%u.%u.%u)", dest.getDByte(0), dest.getDByte(1), dest.getDByte(2), dest.getDByte(3)); // note: str().c_str() too slow here

    if (localAddresses.empty())
    {
        // collect interface addresses if not yet done
        for (int i=0; i<ift->getNumInterfaces(); i++)
        {
            IPv4Address interfaceAddr = ift->getInterface(i)->ipv4Data()->getIPAddress();
            localAddresses.insert(interfaceAddr);
        }
    }

    AddressSet::iterator it = localAddresses.find(dest);
    return it!=localAddresses.end();
}

// JcM add: check if the dest addr is local network broadcast
bool RoutingTable::isLocalBroadcastAddress(const IPv4Address& dest) const
{
    Enter_Method("isLocalBroadcastAddress(%x)", dest.getInt()); // note: str().c_str() too slow here

    if (localBroadcastAddresses.empty())
    {
        // collect interface addresses if not yet done
        for (int i=0; i<ift->getNumInterfaces(); i++)
        {
            IPv4Address interfaceAddr = ift->getInterface(i)->ipv4Data()->getIPAddress();
            IPv4Address broadcastAddr = interfaceAddr.getBroadcastAddress(ift->getInterface(i)->ipv4Data()->getNetmask());
            if (!broadcastAddr.isUnspecified())
            {
                 localBroadcastAddresses.insert(broadcastAddr);
            }
        }
    }

    AddressSet::iterator it = localBroadcastAddresses.find(dest);
    return it!=localBroadcastAddresses.end();
}

bool RoutingTable::isLocalMulticastAddress(const IPv4Address& dest) const
{
    Enter_Method("isLocalMulticastAddress(%u.%u.%u.%u)", dest.getDByte(0), dest.getDByte(1), dest.getDByte(2), dest.getDByte(3)); // note: str().c_str() too slow here

    for (int i=0; i<ift->getNumInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->ipv4Data()->isMemberOfMulticastGroup(dest))
            return true;
    }
    return false;
}

void RoutingTable::dsdvTestAndDelete()
{
     if (timetolive_routing_entry==timetolive_routing_entry.getMaxTime())
           return;
     for (RouteVector::iterator i=routes.begin(); i!=routes.end(); ++i)
     {

           IPv4Route *e = *i;
           if (this->isLocalAddress(e->getHost()))
                     continue;
           if (((e->getHost()).str() != "*") && ((e->getHost()).str() != "<unspec>") && ((e->getHost()).str() != "127.0.0.1") && (simTime()-(e->getInstallTime()))>timetolive_routing_entry){
                //EV << "Routes ends at" << routes.end() <<"\n";
                deleteRoute(e);
                //EV << "After deleting Routes ends at" << routes.end() <<"\n";
                EV << "Deleting entry ip=" << e->getHost().str() <<"\n";
                i--;
           }
      }

}

const bool RoutingTable::testValidity(const IPv4Route *entry) const
{
     if (timetolive_routing_entry==timetolive_routing_entry.getMaxTime())
           return true;
     if (this->isLocalAddress(entry->getHost()))
           return true;
     if (((entry->getHost()).str() != "*") && ((entry->getHost()).str() != "<unspec>") && ((entry->getHost()).str() != "127.0.0.1") && (simTime()-(entry->getInstallTime()))>timetolive_routing_entry){
                return false;
      }
      return true;

}

const IPv4Route *RoutingTable::findBestMatchingRoute(const IPv4Address& dest) const
{
    Enter_Method("findBestMatchingRoute(%u.%u.%u.%u)", dest.getDByte(0), dest.getDByte(1), dest.getDByte(2), dest.getDByte(3)); // note: str().c_str() too slow here

    RoutingCache::iterator it = routingCache.find(dest);
    if (it != routingCache.end())
    {
        if (it->second==NULL)
        {
              routingCache.clear();
              localAddresses.clear();
        }
        else if (testValidity(it->second))
        {
            if (it->second->getSource()==IPv4Route::MANET)
            {
                if (IPv4Address::maskedAddrAreEqual(dest, it->second->getHost(), IPv4Address::ALLONES_ADDRESS))
        return it->second;
            }
            else
                return it->second;
        }
    }
    // find best match (one with longest prefix)
    // default route has zero prefix length, so (if exists) it'll be selected as last resort
    const IPv4Route *bestRoute = NULL;
    uint32 longestNetmask = 0;
    for (RouteVector::const_iterator i=routes.begin(); i!=routes.end(); ++i)
    {
        IPv4Route *e = *i;
        if (testValidity(e))
        {
            if (IPv4Address::maskedAddrAreEqual(dest, e->getHost(), e->getNetmask()) && // match
                (!bestRoute || e->getNetmask().getInt() > longestNetmask))  // longest so far
            {
                bestRoute = e;
                longestNetmask = e->getNetmask().getInt();
            }
    }
    }

    if (bestRoute && bestRoute->getSource()==IPv4Route::MANET && bestRoute->getHost()!=dest)
    {
        bestRoute = NULL;
        /* in this case we must find the mask must be 255.255.255.255 route */
        for (RouteVector::const_iterator i=routes.begin(); i!=routes.end(); ++i)
        {
           IPv4Route *e = *i;
           if (testValidity(e))
           {
              if (IPv4Address::maskedAddrAreEqual(dest, e->getHost(), IPv4Address::ALLONES_ADDRESS) && // match
               (!bestRoute || e->getNetmask().getInt()>longestNetmask))  // longest so far
              {
                 bestRoute = e;
                 longestNetmask = e->getNetmask().getInt();
              }
           }
        }
    }

    routingCache[dest] = bestRoute;
    return bestRoute;
}

InterfaceEntry *RoutingTable::getInterfaceForDestAddr(const IPv4Address& dest) const
{
    Enter_Method("getInterfaceForDestAddr(%u.%u.%u.%u)", dest.getDByte(0), dest.getDByte(1), dest.getDByte(2), dest.getDByte(3)); // note: str().c_str() too slow here

    const IPv4Route *e = findBestMatchingRoute(dest);
    return e ? e->getInterface() : NULL;
}

IPv4Address RoutingTable::getGatewayForDestAddr(const IPv4Address& dest) const
{
    Enter_Method("getGatewayForDestAddr(%u.%u.%u.%u)", dest.getDByte(0), dest.getDByte(1), dest.getDByte(2), dest.getDByte(3)); // note: str().c_str() too slow here

    const IPv4Route *e = findBestMatchingRoute(dest);
    return e ? e->getGateway() : IPv4Address();
}


MulticastRoutes RoutingTable::getMulticastRoutesFor(const IPv4Address& dest) const
{
    Enter_Method("getMulticastRoutesFor(%u.%u.%u.%u)", dest.getDByte(0), dest.getDByte(1), dest.getDByte(2), dest.getDByte(3)); // note: str().c_str() too slow here here

    MulticastRoutes res;
    res.reserve(16);
    for (RouteVector::const_iterator i=multicastRoutes.begin(); i!=multicastRoutes.end(); ++i)
    {
        const IPv4Route *e = *i;
        if (IPv4Address::maskedAddrAreEqual(dest, e->getHost(), e->getNetmask()))
        {
            MulticastRoute r;
            r.interf = e->getInterface();
            r.gateway = e->getGateway();
            res.push_back(r);
        }
    }
    return res;
}


int RoutingTable::getNumRoutes() const
{
    return routes.size()+multicastRoutes.size();
}

const IPv4Route *RoutingTable::getRoute(int k) const
{
    if (k < (int)routes.size())
        return routes[k];
    k -= routes.size();
    if (k < (int)multicastRoutes.size())
        return multicastRoutes[k];
    return NULL;
}

const IPv4Route *RoutingTable::getDefaultRoute() const
{
    int n = (int)routes.size();
    for (int i=0; i<n; i++)
        if (routes[i]->getNetmask().isUnspecified())
            return routes[i];
    return NULL;
}

const IPv4Route *RoutingTable::findRoute(const IPv4Address& target, const IPv4Address& netmask,
    const IPv4Address& gw, int metric, const char *dev) const
{
    int n = getNumRoutes();
    for (int i=0; i<n; i++)
        if (routeMatches(getRoute(i), target, netmask, gw, metric, dev))
            return getRoute(i);
    return NULL;
}

void RoutingTable::addRoute(const IPv4Route *entry)
{
    Enter_Method("addRoute(...)");
    routeChanged = true;

    // check for null address and default route
    if (entry->getHost().isUnspecified() != entry->getNetmask().isUnspecified())
        error("addRoute(): to add a default route, set both host and netmask to zero");

    if (entry->getHost().doAnd(entry->getNetmask().isUnspecified()).getInt() != 0)
        error("addRoute(): suspicious route: host %s has 1-bits outside netmask %s",
              entry->getHost().str().c_str(), entry->getNetmask().str().c_str());

    // check that the interface exists
    if (!entry->getInterface())
        error("addRoute(): interface cannot be NULL");

    // if this is a default route, remove old default route (we're replacing it)
    if (entry->getNetmask().isUnspecified() && getDefaultRoute()!=NULL)
        deleteRoute(getDefaultRoute());

    // add to tables
    if (!entry->getHost().isMulticast())
        routes.push_back(const_cast<IPv4Route*>(entry));
    else
        multicastRoutes.push_back(const_cast<IPv4Route*>(entry));

    if (firewallOn == false)
    	return;
    RuleSetMap::iterator newSet = ruleMap.find(entry->getRuleSet());
    if (newSet == ruleMap.end() && entry->getRuleSet() != 0)
    	error("This node must use ruleset %d that it doesn't know", entry->getRuleSet());
    else if (entry->getRuleSet() != 0){
    	for (RoutingRule::iterator jj = newSet->second.begin(); jj!=newSet->second.end(); jj++){
    		(*jj)->setDestAddress(entry->getHost());
    		(*jj)->setDestNetmask(entry->getNetmask());
    		(*jj)->hc = entry->getMetric();
    		addRule(POSTROUTING, *jj); // rulesets add rules only in postrouting
    	}
    }


    invalidateCache();
    updateDisplayString();

    nb->fireChangeNotification(NF_IPv4_ROUTE_ADDED, entry);
}


bool RoutingTable::deleteRoute(const IPv4Route *entry)
{
    Enter_Method("deleteRoute(...)");
    routeChanged = true;
    RouteVector::iterator i = std::find(routes.begin(), routes.end(), entry);
    if (i!=routes.end())
    {
        nb->fireChangeNotification(NF_IPv4_ROUTE_DELETED, entry); // rather: going to be deleted
        routes.erase(i);
        delRule(POSTROUTING, entry->getHost()); // delete all associated rules
        delete entry;
        invalidateCache();
        updateDisplayString();
        return true;
    }
    i = std::find(multicastRoutes.begin(), multicastRoutes.end(), entry);
    if (i!=multicastRoutes.end())
    {
        nb->fireChangeNotification(NF_IPv4_ROUTE_DELETED, entry); // rather: going to be deleted
        multicastRoutes.erase(i);
        delete entry;
        invalidateCache();
        updateDisplayString();
        return true;
    }
    return false;
}


bool RoutingTable::routeMatches(const IPv4Route *entry,
    const IPv4Address& target, const IPv4Address& nmask,
    const IPv4Address& gw, int metric, const char *dev) const
{
    if (!target.isUnspecified() && !target.equals(entry->getHost()))
        return false;
    if (!nmask.isUnspecified() && !nmask.equals(entry->getNetmask()))
        return false;
    if (!gw.isUnspecified() && !gw.equals(entry->getGateway()))
        return false;
    if (metric && metric!=entry->getMetric())
        return false;
    if (dev && strcmp(dev, entry->getInterfaceName()))
        return false;

    return true;
}

void RoutingTable::updateNetmaskRoutes()
{
    // first, delete all routes with src=IFACENETMASK
    for (unsigned int k=0; k<routes.size(); k++)
        if (routes[k]->getSource()==IPv4Route::IFACENETMASK)
            routes.erase(routes.begin()+(k--));  // '--' is necessary because indices shift down

    // then re-add them, according to actual interface configuration
    for (int i=0; i<ift->getNumInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->ipv4Data()->getNetmask()!=IPv4Address::ALLONES_ADDRESS)
        {
            IPv4Route *route = new IPv4Route();
            route->setType(IPv4Route::DIRECT);
            route->setSource(IPv4Route::IFACENETMASK);
            route->setHost(ie->ipv4Data()->getIPAddress().doAnd(ie->ipv4Data()->getNetmask()));
            route->setNetmask(ie->ipv4Data()->getNetmask());
            route->setGateway(IPv4Address());
            route->setMetric(ie->ipv4Data()->getMetric());
            route->setInterface(ie);
            routes.push_back(route);
        }
    }

    invalidateCache();
    updateDisplayString();
}



void RoutingTable::addRule(CHAIN chain, IPv4RouteRule *entry)
{
// first, find the rule if exist
	IPv4RouteRule * rule;
    if((rule = findRule(chain, entry->getProtocol(), entry->getSrcPort(),
    		entry->getSrcAddress(), entry->getDestPort(),
    		entry->getDestAddress(), entry->getInterface(), 0, false)) != 0){
    	(rule)->activate();
    	if(strncmp(par("firewallStrategy").stringValue(),"NONE",strlen("NONE")) == 0 )
    		(rule)->enforce();
    	(rule)->hc = entry->hc;
    	return;
    }

    IPv4RouteRule * newRule = new IPv4RouteRule;
    memcpy(newRule, entry, sizeof(IPv4RouteRule));
    newRule->activate();
	// needed to enforce output rules in the initialization when not using firewall-aware OLSR
    if(strncmp(par("firewallStrategy").stringValue(),"NONE",strlen("NONE")) == 0 ){
		newRule->enforce();
    }
    for (int i=0; i < HITQUEUE_SIZE; i++)
        	newRule->hitQueue.push_back(0);

    if (chain == OUTPUT)
        outputRules.push_back(newRule);
    else if (chain == INPUT)
        inputRules.push_back(newRule);
    else if (chain == POSTROUTING)
        postRoutingRules.push_back(newRule);
    else if (chain == FORWARD)
         forwardRules.push_back(newRule);
    else if (chain == GLOBAL)
         globalRules.push_back(newRule);
    else
    	opp_error("Unknown chain chosen");
}


// rules are never really deleted, we deactivate them. This is
// not really the case of a generic firewall, it is needed
// for simulations in manet case

void RoutingTable::delRule(IPv4RouteRule *entry)
{
	for (unsigned int i=0; i<outputRules.size(); i++)
		if (outputRules[i] == entry)
		{
			entry->deactivate();
			// do not unenforce(), if the rule is put back it must be running, only firewallingStrategy changes enforcement
			return;
		}
	for (unsigned int i=0; i<inputRules.size(); i++)
		if (inputRules[i] == entry)
		{
			entry->deactivate();
			return;
		}
	for (unsigned int i=0; i<forwardRules.size(); i++)
		if (forwardRules[i] == entry)
		{
			entry->deactivate();
			return;
		}
}
void RoutingTable::delRule(CHAIN chain, IPv4Address destAddress)
{
	if (destAddress.isUnspecified())
		return;
	if(chain == OUTPUT)
	{
		for (unsigned int i=0; i<outputRules.size(); i++)
			if (outputRules[i]->getDestAddress() == destAddress)
			{
				outputRules[i]->deactivate();
				break;
			}
	} else if (chain == INPUT ){
		for (unsigned int i=0; i<inputRules.size(); i++)
			if (inputRules[i]->getDestAddress() == destAddress)
			{
				outputRules[i]->deactivate();
				break;
			}
	} else if (chain == FORWARD){
		for (unsigned int i=0; i<forwardRules.size(); i++)
				if (forwardRules[i]->getDestAddress() == destAddress)
				{
					forwardRules[i]->deactivate();
					break;
				}
	} else if (chain == POSTROUTING){
			for (unsigned int i=0; i<postRoutingRules.size(); i++)
					if (postRoutingRules[i]->getDestAddress() == destAddress)
					{
						postRoutingRules[i]->deactivate();
						break;
					}
	} else if (chain == GLOBAL){
		opp_error("Why are you trying to delete a global rule?");
	}
}

void RoutingTable::storeRule(IPv4RouteRule *entry, uint8_t rulesetCode){
	ruleMap[rulesetCode].push_back(entry);
}

void RoutingTable::delStoredRuleSet(uint8_t rSet){
	for(RoutingRule::iterator ii=ruleMap[rSet].begin();
			ii != ruleMap[rSet].end(); ii++)
		delete *ii;
	ruleMap.erase(rSet);
}

void RoutingTable::enforceRuleSet(CHAIN chain, int rSet){

	if (ruleMap.find(rSet) == ruleMap.end() && rSet != 0)
		error("Trying to enforce an unknown `%d' ruleset", rSet);
	for(RoutingRule::iterator ii=ruleMap[rSet].begin();
			ii != ruleMap[rSet].end(); ii++){
	    IPv4RouteRule * newRule = new IPv4RouteRule;
	    memcpy(newRule, (*ii), sizeof(IPv4RouteRule));
		newRule->setDestAddress(routerId);
		newRule->activate();
	    if(strncmp(par("firewallStrategy").stringValue(),"NONE",strlen("NONE")) == 0 )
			newRule->enforce();
		addRule(chain,newRule);
	}
}
const IPv4RouteRule * RoutingTable::getRule(CHAIN chain, int index) const
{
    if (chain == OUTPUT)
    {
        if (index < (int)outputRules.size())
            return outputRules[index];
        else
            return NULL;
    }
    else if( chain == INPUT)
    {
        if (index < (int)inputRules.size())
            return inputRules[index];
        else
            return NULL;
    }
    else if( chain == FORWARD)
    {
        if (index < (int)forwardRules.size())
            return forwardRules[index];
        else
            return NULL;
    }
    else if( chain == POSTROUTING)
    {
        if (index < (int)postRoutingRules.size())
            return postRoutingRules[index];
        else
            return NULL;
    }
    else if( chain == GLOBAL)
    {
        if (index < (int)globalRules.size())
            return globalRules[index];
        else
            return NULL;
    }
	opp_error("Inexistent chain selected");

}

int RoutingTable::getNumRules(CHAIN chain)
{
	if (chain == OUTPUT)
		return outputRules.size();
	else if( chain == INPUT)
		return inputRules.size();
	else if( chain == FORWARD)
		return forwardRules.size();
	else if( chain == POSTROUTING)
		return postRoutingRules.size();
	else if( chain == GLOBAL)
		return globalRules.size();
	opp_error("Inexistent chain selected");
	return -1; // stop bothering with warnings
}

IPv4RouteRule * RoutingTable::findRule(CHAIN chain, int prot,
		int sPort, const IPv4Address &srcAddr, int dPort,
		const IPv4Address &destAddr, const InterfaceEntry *iface,
		int ttl, bool activeOnly){
	double dummy;
	double & dummyref = dummy;
	return findRule(chain, prot, sPort, srcAddr, dPort,
			destAddr, iface,ttl, activeOnly, dummy);
}

 IPv4RouteRule * RoutingTable::findRule(CHAIN chain, int prot,
		int sPort, const IPv4Address &srcAddr, int dPort,
		const IPv4Address &destAddr, const InterfaceEntry *iface,
		int ttl, bool activeOnly, double & delay)
{
    std::vector<IPv4RouteRule *>::const_iterator it;
    std::vector<IPv4RouteRule *>::const_iterator endIt;


    int pos = 0;
    if (chain == OUTPUT)
    {
        it = outputRules.begin();
        endIt = outputRules.end();
    }
    else if (chain == INPUT)
    {
        it = inputRules.begin();
        endIt = inputRules.end();
    }
    else if (chain == FORWARD)
    {
    	it = forwardRules.begin();
    	endIt = forwardRules.end();
    }
    else if (chain == POSTROUTING)
    {
    	it = postRoutingRules.begin();
    	endIt = postRoutingRules.end();
    } else if (chain == GLOBAL)
    {
    	it = globalRules.begin();
    	endIt = globalRules.end();
    }

    while (it!=endIt)
    {

       IPv4RouteRule *e = (*it);
       if(activeOnly && !e->isActive()){
    	   /*
       	   active rules are rules that are
    	   corresponding to routes are known. Unactive rules
    	   relate to rules that were known in the past but now
    	   are unavailable. If a rule is unactive it means we do not
    	   have an available route for it, so there is no miss to be
    	   logged.
    	   */
    	   it++;
    	   continue;
       }

       if (!srcAddr.isUnspecified() && !e->getSrcAddress().isUnspecified())
       {
           if (!IPv4Address::maskedAddrAreEqual(srcAddr, e->getSrcAddress(), e->getSrcNetmask()))
           {
               it++;
               if (e->getTarget() != IPv4RouteRule::LOG)
            	   pos++;
               continue;
           }
       }
       if (!destAddr.isUnspecified() && !e->getDestAddress().isUnspecified())
       {
    	   if (!e->getDestNetmask().isUnspecified()){
    		   if (!IPv4Address::maskedAddrAreEqual(destAddr, e->getDestAddress(), e->getDestNetmask()))
    		   {
    			   it++;
    			   if (e->getTarget() != IPv4RouteRule::LOG)
    				   pos++;
    			   continue;
    		   }
    	   }
    	   else if(e->getDestAddress() != destAddr) {
    			   it++;
    			   if (e->getTarget() != IPv4RouteRule::LOG)
    				   pos++;
    			   continue;
    	   }

       }
       if ((prot!=IP_PROT_NONE) && (e->getProtocol()!=IP_PROT_NONE) && (prot!=e->getProtocol()))
       {
           it++;
           if (e->getTarget() != IPv4RouteRule::LOG)
             				   pos++;
           continue;
       }
       if ((sPort!=-1) && (e->getSrcPort()!=-1) && (sPort!=e->getSrcPort()))
       {
           it++;
           if (e->getTarget() != IPv4RouteRule::LOG)
             				   pos++;
           continue;
       }
       if ((dPort!=-1) && (e->getDestPort()!=-1) && (dPort!=e->getDestPort()))
       {
           it++;
           if (e->getTarget() != IPv4RouteRule::LOG)
             				   pos++;
           continue;
       }
       if ((iface!=NULL) && (e->getInterface()!=NULL) && (iface!=e->getInterface()))
       {
           it++;
           if (e->getTarget() != IPv4RouteRule::LOG)
             				   pos++;
           continue;
       }
       // found valid src address, dest address src port and dest port

       if (chain == GLOBAL) // global is dummy, just needed to make statistics of packets to be filtered
    	   return e;
       if (e->getTarget() == IPv4RouteRule::LOG){
    	   numLogged++;
    	   it++;
    	   continue;
       }
       if (!firewallOn){
    	   delay = 0;
    	   return 0;
       }
       if (activeOnly) // we want to filter and matched something (active/enforced or not), log
    	   logFirewallStats(chain,  e,  ttl);
       if (chain == POSTROUTING){ // only postrouting has dynamic firewalling (enforced/actived rules)
    	   if(activeOnly){ // we are looking for active rules
    		   if (!e->isEnforced()){
    			   /*
    			    * enforced rules are rules that the firewall strategy has chosen to
    			    * be enforced now. A rule can be active&&(enforced/!enforced) or it can
    			    * be non active. Only active rules can be enforced. If a rule is
    			    * active and not enforced there is a miss logged.
    			    * Non actived rules have no real application, they are rules that the
    			    * node has never received, I need them to make statistics against a
    			    * perfectly working firewall (a firewall that always receives all the rules) in case
    			    * of distribution of partial rule-sets.
    			    * the active/inactive state is meaningful only in postrouting chains that are dynamic.
    			    *
    			    * the routing protocol only inteferes with the POSTROUTING chain
    			    */
    			   it++;
    			   continue; //matched but not enforced, keep looking
    		   } else { // rule is enforced (so it must be active)
    			   delay = strategy->getDelay(perf,pos);
    			   return e;
    		   }
    	   } else{ /* we look for a rule not to filter, but to manage the rule-sets (add/delete)
    	   	   	   	   so return the rule even if it may be unenforced
    		    */
    		   delay = 0;
    		   return e;
    	   }
       }
       if(chain == INPUT || chain == OUTPUT || chain == FORWARD){ // always filter traffic if we have rules
    	   	   	   	   	   	   	   	   	     // since these chains are not dynamic (they are always active)
    	   delay = strategy->getDelay(perf,pos);
    	   return e;
       }

       opp_error("Should never arrive here"); // all chains should be already matched
    }
    return 0;
}

void RoutingTable::logFirewallStats(CHAIN chain, IPv4RouteRule * e, int ttl)
{
	   if(ttl){ // needed only for statistics
	    	   if (chain == POSTROUTING && e->isEnforced()){
	    		   if (ttl == 32) // this packet has been generated by us.
	    			   toBeFiltered++;
	    		   postRoutingMatched++;
	    		   cacheHit++;
	    		   this->emit(postRoutingMatch, 32-ttl);
	        	   e->hitQueue.back()++; // increase last second usage statistic
	    	   }
	    	   else if (chain == POSTROUTING && !e->isEnforced()){
	    		   if (ttl == 32) // this packet has been generated by us.
	    			   toBeFiltered++;
	    		   postRoutingMissed++;
	    		   cacheMiss++;
	    		   this->emit(postRoutingMiss, 32-ttl);
	        	   e->hitQueue.back()++; // increase last second usage statistic
	    	   }
	    	   else if (chain == OUTPUT) { // non postrouting chains are not dynamic, so toBeFiltered is not applied here
	    		   outputMatched++;
	    	   }  else if (chain == FORWARD) {
	    		   forwardMatched++;
	    	   }
	    	   else if (chain == INPUT){
	    		   inputMatched++;
	    		   this->emit(inputRulesMatch, 32-ttl);
	    	   } // no GLOBAL here, to keep findRule clean it is in IPv4.cc
	       }
}

void RoutingTable::refreshRuleset(){
	if (routeChanged){
		routeChanged = false;
	    if(strncmp(par("firewallStrategy").stringValue(),"NONE",strlen("NONE")) != 0 )
	    	rSetSize.record(strategy->reorganizeRules());
	}
}

void RoutingTable::finish(){
	double totalPostrouting = postRoutingMatched+postRoutingMissed;

	if (totalPostrouting != 0)
	// match/miss at a local filter scale.
		recordScalar("Global PostRouting match ratio", postRoutingMatched/totalPostrouting);
	else
		recordScalar("Global PostRouting match ratio", -1);
	// all the packets that matched a GLOBAL rule in output
	recordScalar("Global chain output match", globalSent);
	// all the packets that matched a GLOBAL rule in input
	recordScalar("Global chain input match", globalReceived);
	// all the packets that were sent by a node but that matched a disabled rule in the
	// sender's postrouting chain. Works also without active/enabled rules
	recordScalar("To be Filtered", toBeFiltered);
	if (globalSent != 0)
		recordScalar("Globals missed", (float)globalReceived/(float)globalSent);
	else
		recordScalar("Globals missed", -1);

	recordScalar("numLogged", numLogged);

	// this metric makes sense with generic rules of the kind ''iptables -A POSTROUTING -p udp --dport 8000'', without explicit destination if:
	// you set in the non-dynamic rule-set the generic rules for the nodes that will apply filtering in POSTROUTING with target DROP
	// set in the non-dynamic rule-set the generic rules for the nodes that will NOT apply filtering in POSTROUTING with target LOG
	// the metric gives you the number of packets you were able to match
	// against the total number of packets you should have matched

	if (numLogged != 0 || postRoutingMatched != 0)
		recordScalar("Logged rules match", (float)postRoutingMatched/(float)(numLogged+postRoutingMatched));
	else
		recordScalar("Logged rules match", -1);

	if (0 != (cacheHit+cacheMiss))
		recordScalar("Cache hit ratio", cacheHit/(cacheHit+cacheMiss));

	// for this metric, every node should implement as GLOBAL every rule it exports to other and
	// it is useful with deactivated rules
	if (toBeFiltered!= 0)
		recordScalar("Global ruleset missed packets", (float)globalReceived/(float)toBeFiltered);
	//recordScalar("inputMatchedRatio", inputMatched/(double)toBeFiltered); // with this, I need to know the number of sent packets per run that should be filtered
	//recordScalar("inputMatchedGlobal", inputMatched); // with this I will compare inputmatch without firewall with inputmatch with more nodes filtering
}
