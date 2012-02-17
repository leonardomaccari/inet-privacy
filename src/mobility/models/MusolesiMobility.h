

/*
 * Mobilty based on the code released by Mirco Musolesi:
 * http://portal.acm.org/citation.cfm?id=1132983.1132990 
 * "A community based mobility model for ad hoc network research"
 * Ported to Omnet++ by Leonardo Maccari for the PAF-FPE project, see pervacy.eu
 * leonardo.maccari@unitn.it
 
Mobility Patterns Generator for ns-2 simulator
Based on a Community Based Mobility Model
	
Copyright (C) Mirco Musolesi University College London
              m.musolesi@cs.ucl.ac.uk
Mirco Musolesi
Department of Computer Science - University College London
Gower Street London WC1E 6BT United Kingdom
Email: m.musolesi@cs.ucl.ac.uk
Phone: +44 20 7679 0391 Fax: +44 20 7387 1397
Web: http://www.cs.ucl.ac.uk/staff/m.musolesi


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

The contact details of the author are the following:

 *
 */

#ifndef MUSOLESI_MOBILITY_H
#define MUSOLESI_MOBILITY_H

#include <omnetpp.h>
#include "LineSegmentsMobilityBase.h"



/*  helper structs  */
typedef struct hostsItem{
    double currentX;
    double currentY;
    double relativeX;
    double relativeY;
    double goalRelativeX;
    double goalRelativeY;
    double goalCurrentX;
    double goalCurrentY;
    int cellIdX;
    int cellIdY;
    int myCellIdX;
    int myCellIdY;
    double speed;
    double absSpeed;
    double af;
    bool isATraveller;
    double nextStopTime;
} hostsItem;

typedef struct cellsItem{
    double minX;
    double minY;
    int numberOfHosts;
} cellsItem;
	
class INET_API MusolesiMobility : public LineSegmentsMobilityBase
{
  protected: // mixed arguments from Musolesi code
	bool obstacleAvoidance;
    double minHostSpeed;
    double maxHostSpeed;
    int numberOfRows; 
    int numberOfColumns;
    double rewiringProb;
    int numberOfGroups; 
    bool girvanNewmanOn; 
    int targetChoice;
    double sideLength_WholeAreaX;
    double sideLength_WholeAreaY;
	double sideLengthX;
	double sideLengthY;
	double threshold;
    int numHosts;
    double hcmm;
    int rewiringPeriod;
    int reshufflePeriod;
    int initialrewiringPeriod;
    int initialreshufflePeriod;
	double recordStartTime;
    std::map<int,std::pair<double,double> > nodesInMyBlock;  
    static std::map<int, int> intervalDistribution;
    static std::map<int, int> interContactDistribution;
    bool RWP;
    double drift;
    double expmean;
    bool reshufflePositionsOnly;
    int myGroup;
	bool recordStatistics;
    Coord areaTopLeft, areaBottomRight;
    
	protected: // parameters from Musolesi code

    static hostsItem * hosts;
    static cellsItem ** cells;
    static int * numberOfMembers;
    static int **adjacency;
	static double **interaction;
	static int **groups;

	static simsignal_t blocksHistogram;
	static simsignal_t walkedMeters;
	static simsignal_t blockChanges;


	
    double ** CA;
    double **a;
    
    int nodeId;

    cMessage * moveMessage;
  protected:
    /** @brief Initializes mobility model parameters.*/
    virtual void initialize(int);
    virtual void finish();
    virtual void initializePosition();
    /* @brief Configure the initial group matrix */
    void setInitialPosition(bool rewire);
    /* @brief Define and allocate static members with original Musolesi code*/
    void DefineStaticMembers();
    /* @brief Redefined from LineSegmentsMobility */
    virtual void setTargetPosition();
    /* @brief Redefined from LineSegmentsMobility */
    virtual void fixIfHostGetsOutside();
    virtual void handleSelfMsg(cMessage* msg);
    virtual void handleMessage(cMessage* msg);
    void move();
    Coord getRandomPointWithObstacles(int minX, int minY);


};

// global useful functions from original code

void print_int_array(int **array, int array_size);

void print_double_array(double **array, int array_size);

int **initialise_int_array(int array_size);

double **initialise_double_array(int array_size);

void rewire(double **weight, int array_size, double probRewiring, double threshold, int ** groups, int *numberOfMembers, int numberOfGroups);

void refresh_weight_array_ingroups(double ** weight, int array_size, int numberOfGroups, 
        double nRewiring, double threshold, int** groups, int* numberofMembers);

void generate_adjacency (double** weightMat, int** adjacencyMat, double threshold, int array_size);

bool areInTheSameGroup (int node1, int node2, int** groups, int numberOfGroups, int* numberOfMembers);

bool isInGroup(int node, int* group, int numberOfMembers);



#endif

