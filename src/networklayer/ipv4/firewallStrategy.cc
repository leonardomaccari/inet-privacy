
/*
  Copyright Leonardo Maccari, 2011. This software has been developed
  for the PAF-FPE Project financed by EU and Provincia di Trento.
  See www.pervacy.eu or contact me at leonardo.maccari@unitn.it

  I'd be greatful if:
  - you keep the above copyright notice
  - you cite pervacy.eu if you reuse this code


  firewallStrategy is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  firewallStrategy is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with firewallStrategy.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "firewallStrategy.h"
#include "OLSR.h"

firewallStrategy * firewallStrategy::getStrategy(std::string strategy,
		std::vector<IPv4Route *> & table, std::vector<IPv4RouteRule *> &rSet){
	if(strategy == "hitBased")
		return new hitBasedStrategy(table, rSet);
	if(strategy == "hopBased")
		return new hopBasedStrategy(table, rSet);
	if(strategy == "OLSRBased")
		return new OLSRBasedStrategy(table, rSet);
	if(strategy == "NONE")
		return new noStrategy(table, rSet);
	opp_error("Unknown firewalling strategy chosen");
	return NULL; // just to avoid the compilator warning
}
firewallStrategy::firewallStrategy(std::vector<IPv4Route *> & table,
		std::vector<IPv4RouteRule *> &rSet):ruleSetSize(-1), rt(table), ruleSet(rSet){
}

void firewallStrategy::setMaxSize(int size){
	ruleSetSize =  size;
}

void firewallStrategy::setMPRThreshold(int th){
	MPRThreshold =  th;
}

// maps must have a (0,x) value and must have at least 2 values
// @FIXME this code is horrible... I should find a better way to initialize this

void firewallStrategy::fillPerfMap(cXMLElement * xml, std::string perf){
	posTimeMap myPerf;
	cXMLElement * myXML;
	if (!xml && !perf.compare(""))
		opp_error("You hav chosen a non null performance value but no performance array file");

	if (!xml)
		return;

	std::stringstream tmp;
	tmp << "./root/node[@type='" << perf <<"\']";
	myXML = xml->getFirstChildWithAttribute("node","type", perf.c_str());
	if(myXML == 0)
		opp_error("You have chosen a performance array but no default node type");

	for (cXMLElement *child=myXML->getFirstChildWithAttribute("rset", "size");
			child; child = child->getNextSiblingWithTag("rset"))
	{
		perfMap[atoi(child->getAttribute("size"))] = atof(child->getFirstChild()->getNodeValue());
	}
}

firewallStrategy::nodePerformance firewallStrategy::stringToPerf(std::string perf_s){
if (perf_s.compare("linksys") == 0)
	return linksys;
else if (perf_s.compare("moko") == 0)
	return moko;
else if (perf_s.compare("n900") == 0)
	return n900;
else if (perf_s.compare("atom") == 0)
	return atom;
else
	return NOPERF;
}

double firewallStrategy::getDelay(nodePerformance perf, int pos){
	posTimeMap::iterator ii = perfMap.upper_bound(pos);
	if (ii == perfMap.begin()){
		return ii->second;
	}
	else if (ii == perfMap.end()){
		return (--ii)->second;
	}
}


int hitBasedStrategy::reorganizeRules(){

	std::multimap<double,IPv4RouteRule*> hitMap;
	for(std::vector<IPv4RouteRule*>::iterator ii = ruleSet.begin(); ii != ruleSet.end();ii++){
		std::pair<double, IPv4RouteRule*> mapEl((*ii)->hitAvg, *ii);
		hitMap.insert(mapEl);
	}
	ruleSet.clear();
	int i = 1;
	std::vector<IPv4RouteRule*> ruleBuffer;
	for (std::multimap<double,IPv4RouteRule*>::reverse_iterator jj = hitMap.rbegin();
			jj != hitMap.rend(); jj++){

		if (jj->second->isActive() && i <= ruleSetSize){
			jj->second->enforce();
			i++;
			ruleSet.push_back(jj->second);
		}
		else {
			ruleBuffer.push_back(jj->second);
			jj->second->unenforce();
		}
	}
	if(!ruleBuffer.empty())
		ruleSet.insert(ruleSet.end(), ruleBuffer.begin(), ruleBuffer.end());
	return (ruleSet.size() < ruleSetSize) ? ruleSet.size(): ruleSetSize;
}

int hopBasedStrategy::reorganizeRules(){
	int size = 0;

	for(std::vector<IPv4RouteRule*>::iterator ii = ruleSet.begin(); ii != ruleSet.end();ii++)
	{
		if((*ii)->hc < minHopCount){
			if (!reverse)
				(*ii)->unenforce();
			else{
				(*ii)->enforce();
				size++;
			}
		}
		else{
			if(!reverse){
				(*ii)->enforce();
				size++;
			}
			else
				(*ii)->unenforce();
		}
	}
	return size;
}

int OLSRBasedStrategy::reorganizeRules(){
    cModule* manetrouting = routingTable->getParentModule()->getModuleByRelativePath("manetrouting");

   std::cout << "aggiorna OLSR con isMpr";
   exit(1);
//	if(static_cast<OLSR*>(manetrouting)->isMpr() >= MPRThreshold)
//		return noStrategy::reorganizeRules();
//	else
//		return hopBasedStrategy::reorganizeRules();


}
int noStrategy::reorganizeRules(){
	int size = 0;

	for(std::vector<IPv4RouteRule*>::iterator ii = ruleSet.begin(); ii != ruleSet.end();ii++)
	{
		if((*ii)->isActive()){
			(*ii)->enforce();
			size++;
		}
	}
	return size;
}
