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


package inet.applications.addressgenerator;



// This module implements a pool of addresses that can be used by the
// application modules to select targets for their purpuses. It interacts with
// the routing table or can be configured with static address lists.

simple AddressGenerator
{
    parameters:
        string generatorMode = default("RoutingTable"); 
            // The way addressess are gathered: 
                // - RoutingTable: get them from your routing table (useful in
                // ad-hoc networks)
                // - FixedList: get them from the configuration
                // - NodeType: specify a node type and get any of them
        int listSize = default(-1); 
            // size of the list to be returned.
        string addressList = default(""); 
                // needed for FixedList mode, may be "random(host)" where host is the nodetype  
        string nodeType = default(""); 
                // needed for NodeType mode
        string routingTablePath = default("");
                // it it is unset, the default is host[].routingTable
                
        int waitTime @unit(s) = default(1s); 
        // when using routingTable mode do not start at time==0 but wait an amount of time
        // this is useful when routing tables take some time to be filled and keepSetConsistent is used
        
        bool keepSetConsistent = default(false);
        // try to keep time consistency of the list among updates (not just
        // random IP taken by routes, safeguard the ones already present in the
        // list at previous iteration), 
        bool keepSetBalanced = default(false);  
        // try to keep topology consistency of the list among updates. In group mobility models
        // the keepSetConsistent will probably produce sets with a lot of one-hop nodes
        // since they are always reachable. This tries to proportionally spread the target set
        // over the possible hopcount in the network.
        
        int updateTime @unit(s) = default(1s);

        @display("i=abstract/people");
        @statistic[listSize](title="Size of targetlist"; record=vector);
        @statistic[list](title="Composition of of targetlist"; record=histogram);
}

// tests tbd:
// routingtable - listSize -1: check the list size is always equal to the routing table size
//  routingtable - listSize x: check the list size is always x < to the routing table size and there is consistency with time
// NodeType : check that there are all the nodetype destinations
// addressList: check the list, test broadcast
