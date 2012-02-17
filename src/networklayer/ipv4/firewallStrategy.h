/*
 * This class implements various ways to reduce the ruleset size when using
 * firewall with distributed environments.
 */

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

#ifndef FIREWALL_S
#define FIREWALL_S

#include "IPv4RouteRule.h"
#include "IPv4Route.h"




class INET_API firewallStrategy: public cObject{

public:
	typedef enum NodePerformance {
		linksys,
		moko,
		n900,
		atom,
		NOPERF
	} nodePerformance;

	typedef std::map<int,double> posTimeMap;
	posTimeMap perfMap;

public:
	virtual int reorganizeRules() = 0;
	void setMaxSize(int size);
	void setMPRThreshold(int th);
	void fillPerfMap(cXMLElement * xml, std::string perf);
	nodePerformance stringToPerf(std::string);
	double getDelay(nodePerformance, int);
	static firewallStrategy * getStrategy(std::string strategy,
			std::vector<IPv4Route *> & table, std::vector<IPv4RouteRule *> &rSet);
	firewallStrategy(std::vector<IPv4Route *> & table,
			std::vector<IPv4RouteRule *> &rSet);

	cModule * routingTable;

protected:
	int ruleSetSize;
	int MPRThreshold;
	std::vector<IPv4Route*> & rt;
	std::vector<IPv4RouteRule*> & ruleSet;
};

class hitBasedStrategy:  public firewallStrategy{
public:
	int reorganizeRules();
	hitBasedStrategy(std::vector<IPv4Route *> & table,
			std::vector<IPv4RouteRule *> &rSet): firewallStrategy(table, rSet){};
};


class hopBasedStrategy: virtual public firewallStrategy{
protected:
	bool reverse;
	int minHopCount;

public:
	virtual int reorganizeRules();
	hopBasedStrategy(std::vector<IPv4Route *> & table,
			std::vector<IPv4RouteRule *> &rSet): firewallStrategy(table, rSet), reverse(false), minHopCount(2){};
};


/*
 * Do nothing strategy, keep the whole ruleset
 */
class noStrategy: virtual public firewallStrategy{
public:
	virtual int reorganizeRules();
	noStrategy(std::vector<IPv4Route *> & table,
			std::vector<IPv4RouteRule *> &rSet): firewallStrategy(table, rSet){};
	//friend class firewallStrategy;
};

class OLSRBasedStrategy:  public noStrategy,  public hopBasedStrategy{
public:
	int reorganizeRules();
	OLSRBasedStrategy(std::vector<IPv4Route *> & table,
			std::vector<IPv4RouteRule *> &rSet): firewallStrategy(table, rSet), noStrategy(table, rSet), hopBasedStrategy(table, rSet){
		minHopCount = 3;
		reverse = true;
	};
private:
	cModule* manetrouting;
};


#endif
