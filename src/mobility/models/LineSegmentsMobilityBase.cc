//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//
// Copyright 2012 Leonardo Maccari for the modifications to avoid obstacles (www.pervacy.eu)

#include "LineSegmentsMobilityBase.h"
#include "FWMath.h"





LineSegmentsMobilityBase::LineSegmentsMobilityBase()
{
    targetPosition = Coord::ZERO;
    obstacles = 0;
    deviating = 0;
    obstacleAvoidance = false;
}

void LineSegmentsMobilityBase::initialize(int stage, bool o){
	obstacleAvoidance = o;
	LineSegmentsMobilityBase::initialize(stage);
}
void LineSegmentsMobilityBase::initialize(int stage)
{
    if (stage == 1)
    	if (obstacleAvoidance)
    		obstacles = ObstacleControlAccess().getIfExists();

    MovingMobilityBase::initialize(stage);
    EV << "initializing LineSegmentsMobilityBase stage " << stage << endl;

    if (stage == 2)
    {
#ifdef DEBUG
        WATCH(deviating);
        WATCH(targetPosition);
#endif

        // set this in the configuration file, it is normally unset

        if (!stationary) {
        	int counter;
        	for (counter = 0; counter < 100; counter++){
        		setTargetPosition();
        		if (obstacleAvoidance && obstacles->isInObstacle(targetPosition))
        			continue;
        		else
        			break;
        	}
        	if (counter == 100)
        		error("I've been trying 100 next target destination without finding"
        				" anyone outside an obstacle, your topology may be malformed");
            lastSpeed = (targetPosition - lastPosition) / (nextChange - simTime()).dbl();
        }
    }
}


/* Changes to introduce obstacle avoidance (see pervacy.eu for details):
 * 1) generate an obstacle topology. Only squared/rectangular obstacles are tested.
 * 2) Whenever the nextPosition given to a move() command points into an obstacle do:
 *   - verify which is the obstacle side you're going to crash with
 *   - find the corners of the obstacle that belong to that side
 *   - of the two corners find the turningPoint, the one closer to you final destination (targetPosition)
 *   - save the targetPosition in a backup variable
 *   - get a random point *close* to the  turningPoint that is not in the obstacle
 *   - set targetPosition to the turning point
 * 3) If you hit another obstacle do the same, but not overwrite the backup variable
 * 4) When you reach the turningPoint, re-set targetPosition to the backup value
 *
 * WARNING:
 * - this strategy is very simple, it does not allow the node to avoid concave obstacles.
 *   Your obstacles must have spaces between them in order for the node to find a corridor to
 *   move around them, or nodes will get stuck.
 * - getting a random point helps not getting stuck in some pitfalls (for instance,
 *   you are exactly on a corner) but it is not deterministic.
 * - You can not place your nodes in their initial position into an obstacle, and you can't
 *   chose a targetPosition into an obstacle. To avoid this, use getRandomPositionWithObstacles
 *   instead of getRandomPosition().
 * - If your obstacles are not well-placed there can be deadlocks that let the random
 *   functions go in infinite loops. I added some counters to avoid this, if you have errors
 *   about the topology assure that your obstacles:
 *    - do not touch each other (leave at least 2m, see ##README comment below)
 *    - do not touch the margin
 * - Speed is not changed, that is, we do not compensate the increase in distance to reach
 *   a waypoint with a higher speed (this may change the statistic properties of
 *   your mobility model, but adding obstacles has this inconvenience :-) )
 */

void LineSegmentsMobilityBase::move()
{
    simtime_t now = simTime();

    // new waypoint, no obstacle avoidance active
    if (now >= nextChange && ! deviating) {
        lastPosition = targetPosition;
        EV << "destination reached. x = " << lastPosition.x
        		<< " y = " << lastPosition.y << " z = " << lastPosition.z << endl;
        int counter;
        // we have to find a new target, so try to find one that
        // is not inside an obstacle. If we can't do it in a reasonable time,
        // raise an error
        for (counter = 0; counter < 1000; counter++){
        	setTargetPosition();
        	//OPTIMIZE: check if the new trajectory crosses
        	// an obstacle, then apply all the rest
        	if (obstacleAvoidance && obstacles->isInObstacle(targetPosition))
        		continue;
        	else
        		break;
        }
        if (counter == 1000)
        	error("I've been trying 1000 next target destination "
        			"without finding anyone outside an obstacle, "
        			"your topology may be malformed");
        lastSpeed = (targetPosition - lastPosition) / (nextChange - simTime()).dbl();

    } // we reached the turning point! new waypoint is the old one
     else  if (now >= nextChange && deviating) {
        lastPosition = targetPosition;
        DEBUG_OUT << this->getParentModule()->getFullPath()
        		<< " obstacle deviation reached. x = " << lastPosition.x
        		<< " y = " << lastPosition.y << " z = " << lastPosition.z << endl;

        nextChange = now + (targetPositionBak - targetPosition).length()/lastSpeed.length();
        targetPosition = targetPositionBak;
        deviating = 0;
        targetPositionBak.x = -1;
        targetPositionBak.y = -1;
        lastSpeed = (targetPosition - lastPosition) / (nextChange - simTime()).dbl();

    } // we just make a move in the direction of the waypoint
    else if (now > lastUpdate) {
    	Coord previousPosition = lastPosition; // position before this update
        ASSERT(nextChange == -1 || now < nextChange);
        // theoretical next position before we check if we get into an object
        Coord nextPosition = lastPosition + lastSpeed * (now - lastUpdate).dbl();
		const Obstacle * o;
        if(obstacleAvoidance)
        	o = obstacles->isInObstacle(nextPosition);
        else
        	o = 0;
        if (deviating)
        	DEBUG_OUT << this->getParentModule()->getFullPath() << " still deviating " << std::endl;
        // next move is inside an obstacle!
        if (o != 0)
        {
        	DEBUG_OUT <<  this->getParentModule()->getFullPath()
        			<< " found myself headed to an obstacle from "
        			<< previousPosition << " to " << nextPosition
        			<< " with target " << targetPosition << std::endl;

        	// get the turning point
        	Coord routeDeviationT = o->getTurningPoint(&previousPosition,
        			&targetPosition, &nextPosition );
        	Coord initialCoord = routeDeviationT;
        	shufflePoint(&routeDeviationT);
        	int count = 0;
        	// get a random point around the turning point
        	while (obstacles->isInObstacle(routeDeviationT)
        			|| ( routeDeviationT.x < 0 || routeDeviationT.y < 0 ||
        	             routeDeviationT.x > constraintAreaMax.x ||
        	             routeDeviationT.y > constraintAreaMax.y )
        	        || ( routeDeviationT == previousPosition)){
        		routeDeviationT = initialCoord;
        		shufflePoint(&routeDeviationT);
        		count++;
        		if (count >= 100){
        			error("Looping! you probably have an error in your"
        					" topology (double check that the obstacles"
        					" you generated completely fit in the scenario)");
        		}
        	}

        	// check if we head to the corner
        	DEBUG_OUT << " deviating to " <<  routeDeviationT << std::endl;

        	// OPTIMIZE: use a stack of past targets instead of 1-step memory.
        	// Else, we can not keep track of original speed when crossing
        	// multiple obstacles.

        	// if we were already deviating, do not save the previous target,
        	// since obstacles are convex, this should not lead to a deadlock
        	if  ( deviating == 0){
        		targetPositionBak = targetPosition;
        		deviating = true;
        	}

        	targetPosition = routeDeviationT;


        	DEBUG_OUT << "now is " << now << " nextChange " << nextChange
        			<< " updated to ";
        	nextChange = now+(targetPosition - previousPosition).length()/(lastSpeed.length());
        	DEBUG_OUT << nextChange << std::endl;

            lastSpeed = (targetPosition - previousPosition) / (nextChange - now).dbl();

            lastPosition = lastPosition + lastSpeed * (now - lastUpdate).dbl();
        } else{
        	lastPosition = nextPosition;
            DEBUG_OUT << "going forward. x = " << lastPosition.x
            		<< " y = " << lastPosition.y << " z = " << lastPosition.z
            		<< " lastSpeed "<< lastSpeed << " lastUpdate " << lastUpdate << endl;
        }
    }
}

void LineSegmentsMobilityBase::shufflePoint(Coord * c ){
	// add a random +/- delta to the point
	int x = intuniform(-5, 5); // ##README:
	int y = intuniform(-5, 5); // ##README:
	// when a node hits a wall we send him to the corner of the obstacle,
	// and add a random distance from the corner in order for it to
	// overcome the corner. x/y is the distance on axis. Obviously the new waypoint
	// must not be inside an obstacle, so the range of j is correlated to
	// the minimum distance you have to keep between obstacles.
	// if you need to have obstacles closer than this distance, change the range

	c->x += x;
	c->y += y;
/*
	if (i == 1)
		c->x += j;
	else if (i == 2)
		c->x -= j;
	else if (i == 3)
		c->y += j;
	else if (i == 4)
		c->y -= j;
		*/
	DEBUG_OUT  << this->getParentModule()->getFullPath() << " looping " << *c << std::endl;
}

Coord LineSegmentsMobilityBase::getRandomPositionWithObstacles(){
	Coord upLeft, downRight;
	upLeft.x = constraintAreaMin.x;
	upLeft.y = constraintAreaMin.y;
	upLeft.z = constraintAreaMin.z;

	downRight.x = constraintAreaMax.x;
	downRight.y = constraintAreaMax.y;
	downRight.z = constraintAreaMax.z;
	return getRandomPositionWithObstacles(upLeft, downRight);

}
Coord LineSegmentsMobilityBase::getRandomPositionWithObstacles(Coord upLeft, Coord downRight){
	Coord randomPoint = getRandomPosition(upLeft, downRight);
	int counter = 0;
	if (obstacles == 0){
		DEBUG_OUT <<   this->getParentModule()->getFullPath()
				<<  randomPoint <<  " OK. no obstacles " << std::endl;

		return randomPoint;
	}
	while (obstacles->isInObstacle(randomPoint) && counter < 1000){
		randomPoint = getRandomPosition(upLeft, downRight);
		counter++;
	}
	if (counter == 1000)
		error("I've been trying 100 random points without finding "
				"anyone outside an obstacle, your topology may be malformed"
				"(if you're using Musolesi mobility double check your obstacles"
				"are not bigger than the grid blocks you've chosen)");
	DEBUG_OUT <<  this->getParentModule()->getFullPath() <<  randomPoint
			<< " OK at " << counter << " loop with obstacles" << std::endl;
	return randomPoint;
}
