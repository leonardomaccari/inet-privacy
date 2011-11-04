//
// Copyright (C) 2004 Andras Varga
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


package inet.networklayer.ipv4;

//
// Stores the routing table. (Per-interface configuration is stored in
// ~InterfaceTable.)
//
// For hosts, the routerId module parameter should be empty (""),
// in which case the loopback IPv4 address will be set to 127.0.0.1/8.
// For routers which have a routerId set, the loopback address will be set to
// routerId. (See also RFC 2072 "Router Renumbering Guide".)
//
// RouterId may also be set to the string <tt>"auto"</tt>, in which case the
// highest interface address will be chosen as routerId; the loopback address
// will remain 127.0.0.1/8.
//
// The routing table is read from a file (parameter routingFile);
// the file can also fill in or overwrite interface settings.
// The file format is documented <a href="irt.html">here</a>.
//
// Note that many protocols don't require routerId to be routable, but some
// others do -- so it is probably a good idea to set up routable routerIds.
//
// This module has no gates; all functionality can be accessed via member
// functions of the C++ module class. For detailed info, please see the C++
// documentation of the class (Doxygen).
//
// @see ~InterfaceTable
// @author: Jochen Reber, Andras Varga
//
simple RoutingTable
{
    parameters:
        string routerId = default("auto"); // for routers, the router id using IPv4 address dotted
                          // notation; specify "auto" to select the highest
                          // interface address; should be left empty ("") for hosts
        bool IPForward = default(true);  // turns IP forwarding on/off
        string routingFile = default("");  // routing table file name
                string firewallFile = default("");
        string firewallStrategy = default("NONE");
        int firewallRuleSetSize = default(1000000000); // keep all rules by default 
        int firewallMPRThreshold = default(-1);
        
        double probFirewallOn = default(1); // probability to have firewall On, needed for batch simulations
        @statistic[routingTableSize](title="Routing table size"; record=vector); // routing table size logging
        @statistic[firewallTableSize](title="Firewall table size"; record=vector); // routing table size logging
        @statistic[hopCount](title="Route hopCount"; record=histogram); // routing table hopcount
        @statistic[ruleSetDistribution](title="Rule Set Distribution"; record=histogram;); // routing table hopcount
        @statistic[inputRulesMatch](title="Input rules matches"; record=histogram;); // input filter table matches
        @statistic[outputRulesMatch](title="Output rules matches statistics"; record=histogram;); // output rules match statistics
        @statistic[outputRulesMiss](title="Output rules missed statistics"; record=histogram;); // output rules match statistics
        
        
        @display("i=block/table");
}

