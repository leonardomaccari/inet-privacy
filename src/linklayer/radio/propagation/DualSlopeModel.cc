
#include "DualSlopeModel.h"
#include <omnetpp.h>


Register_Class(DualSlopeModel);

void DualSlopeModel::initializeFrom(cModule *radioModule)
{

    EV << "Initializing Dual Slope pathloss model. " << std::endl;
    initializeFreeSpace(radioModule);
    recvPower.setName("Dual Slope Received Power");
    WATCH(recvPower);

    alphaFreeSpace = radioModule->par("alphaFreeSpace");
    alphaObstacle = radioModule->par("alphaObstacle");
    breakpointDistance = radioModule->par("breakpointDistance");
    smoothStep = radioModule->par("smoothStep").boolValue();


    if (alphaObstacle < alphaFreeSpace or alphaFreeSpace < 2)
        opp_error("You must set alphaObstacle to a value  greater than alphaFreeSpace \
                and alphaFreeSpace to a value greater than 2, now they are %d, %d",
                alphaObstacle, alphaFreeSpace);

}

double DualSlopeModel::calculateReceivedPower(double pSend, double carrierFrequency, double distance)
{
    double receivedPower = 0; 
    double waveLength = SPEED_OF_LIGHT / carrierFrequency;

    double PL_d0 = freeSpace(Gt, Gr, L, pSend, waveLength, 1.0, alphaFreeSpace);
    PL_d0_db = 10.0 * log10(pSend / PL_d0);

    double PL_db;
    if(distance < 1)// nodes are closer than 1m, round to 1m
        PL_db = PL_d0;
    else{
        if (smoothStep) 
            PL_db = PL_d0_db +  // reference pathloss
                10*alphaFreeSpace*log10(distance) + // decay from freespace coefficient
                10*(alphaObstacle-alphaFreeSpace)*log10(1+(distance/breakpointDistance)); // decay
        // from obstacle coefficient
        // adjusted to avoid a discrete step
        else {
            if(distance < breakpointDistance)// decay from freespace coefficient
                PL_db = PL_d0_db +  // reference pathloss
                    10*alphaFreeSpace*log10(distance);
            else // decay from freespace coefficient
                PL_db = PL_d0_db + 
                    10*alphaFreeSpace*log10(breakpointDistance) // loss up to the breakpoint
                    + 10*alphaObstacle*log10(distance/breakpointDistance); // lossa after the breakpoint
        }

    }
     // Reception power = Tx Power - Pathloss
    double Prx_db = (10.0 * log10(pSend)) - PL_db;

    if(debugModel){
    	recvPower.record(Prx_db);
    }
    receivedPower = pow(10, Prx_db/10.0);
#ifdef DEBUGMODEL
    
    // compare with freespace model
      std::cerr << simTime() << " " << Prx_db << " " <<
        10*log10(FreeSpaceModel::calculateReceivedPower(pSend, carrierFrequency, distance)) 
        << std::endl; 
#endif

    // adjust fulctuations. This comes from LogNormal Shadowing model, in
    // reality with dual slope it cannot happen, so I'll just put an error here
    if (receivedPower > pSend)
        opp_error("Dual slope error, received power seems to be higher \
                than transmitted power??? (%f,%f, distance=%f)"
                ,receivedPower, pSend, distance);

    return receivedPower;
}
