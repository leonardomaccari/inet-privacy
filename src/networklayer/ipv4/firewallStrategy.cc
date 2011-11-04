
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
