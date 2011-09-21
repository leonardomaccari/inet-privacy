#ifndef __DUALSLOPEMODEL_H
#define __DUALSLOPEMODEL_H

#include "FreeSpaceModel.h"

// this model implements a dual-slope pathloss model, as in the paper
// described in http://dl.acm.org/citation.cfm?id=1416323&bnc=1 and
// http://books.google.com/books?id=VbCCJhjO4OcC&lpg=PA79&dq=dual-slope%20path%20loss%20coefficients&pg=PA79#v=onepage&q&f=false
//
// Basically is a pathloss model with two 
// coefficients, one to be used before the breakpoint distance, and another
// to be used after. This mimics the possibility of having freespace up to 
// a certain distance and obstruction of some obstacles after that 
// distance. The second reference implements a correction factor to avoid
// sharp edges in the transition 

class INET_API DualSlopeModel : public FreeSpaceModel
{
	protected:
		// The Breakdown distance
		double breakpointDistance;
		// Coefficient before the breakpoint distance
		double alphaFreeSpace;
		// Coefficient after the breakpoint distance
		double alphaObstacle;
        // reference pathloss 1m
        double PL_d0_db;
        // do we use the smoothing of the dual slope?
        bool smoothStep;

        bool debugModel;

        cOutVector recvPower;
	public:
        // derived from IReceptionModel
		virtual void initializeFrom(cModule *radioModule);
        // derived from IReceptionModel
		virtual double calculateReceivedPower(double pSend, double carrierFrequency, double distance);
};

#endif

