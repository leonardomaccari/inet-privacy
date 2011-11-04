/*
 * This class implements various ways to reduce the ruleset size when using
 * firewall with distributed environments.
 * @FIXME add license here.
 */

#ifndef FIREWALL_S
#define FIREWALL_S

#include "IPv4RouteRule.h"
#include "IPv4Route.h"


class INET_API firewallStrategy: public cObject{

public:
	virtual int reorganizeRules() = 0;
	void setMaxSize(int size);
	void setMPRThreshold(int th);
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
