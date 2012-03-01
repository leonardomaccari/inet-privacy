//
// ObstacleControl - models obstacles that block radio transmissions
// Copyright (C) 2010 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include <set>
#include "world/obstacles/Obstacle.h"

Obstacle::Obstacle(std::string id, double attenuationPerWall, double attenuationPerMeter) :
    visualRepresentation(0),
    id(id),
    attenuationPerWall(attenuationPerWall),
    attenuationPerMeter(attenuationPerMeter) {
}

void Obstacle::setShape(Coords shape) {
    coords = shape;
    bboxP1 = Coord(1e7, 1e7);
    bboxP2 = Coord(-1e7, -1e7);
    for (Coords::const_iterator i = coords.begin(); i != coords.end(); ++i) {
        bboxP1.x = std::min(i->x, bboxP1.x);
        bboxP1.y = std::min(i->y, bboxP1.y);
        bboxP2.x = std::max(i->x, bboxP2.x);
        bboxP2.y = std::max(i->y, bboxP2.y);
    }
}

const Obstacle::Coords& Obstacle::getShape() const {
    return coords;
}

const Coord Obstacle::getBboxP1() const {
    return bboxP1;
}

const Coord Obstacle::getBboxP2() const {
    return bboxP2;
}



double Obstacle::calculateReceivedPower(double pSend, double carrierFrequency, const Coord& senderPos, double senderAngle, const Coord& receiverPos, double receiverAngle) const {

    // if obstacles has neither walls nor matter: bail.
    if (getShape().size() < 2) return pSend;

    // get a list of points (in [0, 1]) along the line between sender and receiver where the beam intersects with this obstacle
    std::multiset<double> intersectAt;
    bool doesIntersect = false;
    const Obstacle::Coords& shape = getShape();
    Obstacle::Coords::const_iterator i = shape.begin();
    Obstacle::Coords::const_iterator j = (shape.rbegin()+1).base();
    DEBUG_OUT<< senderPos << " - " << receiverPos << std::endl;
    for (; i != shape.end(); j = i++) {
        Coord c1 = *i;
        Coord c2 = *j;
        DEBUG_OUT<< c2 << c1;

        double i = segmentsIntersectAt(senderPos, receiverPos, c1, c2);
        if (i != -1) {
            DEBUG_OUT<< "OK" << std::endl;
            doesIntersect = true;
            intersectAt.insert(i);
        }
        DEBUG_OUT<< std::endl;

    }

    // if beam interacts with neither walls nor matter: bail.
    bool senderInside = isPointInObstacle(senderPos, *this);
    bool receiverInside = isPointInObstacle(receiverPos, *this);
    if (!doesIntersect && !senderInside && !receiverInside) return pSend;

    // make sure every other pair of points marks transition through matter and void, respectively.
    if (senderInside) intersectAt.insert(0);
    if (receiverInside) intersectAt.insert(1);
        // there are many border conditions that generate odd interceptions, we discard them
	//     // ASSERT((intersectAt.size() % 2) == 0);
	         if (intersectAt.size() %2 != 0)
	             	return -1;

    // sum up distances in matter.
    double fractionInObstacle = 0;
    for (std::multiset<double>::const_iterator i = intersectAt.begin(); i != intersectAt.end(); ) {
        double p1 = *(i++);
        double p2 = *(i++);
        fractionInObstacle += (p2 - p1);
    }

    // calculate attenuation
    double numWalls = intersectAt.size();
    double totalDistance = senderPos.distance(receiverPos);
    double attenuation = (attenuationPerWall * numWalls) + (attenuationPerMeter * fractionInObstacle * totalDistance);
    return pSend * pow(10.0, -attenuation/10.0);
}


Coord Obstacle::getTurningPoint(Coord * from, Coord * to, Coord * across) const{
	// 1) use segmentsIntersectAt to get the intersection between my movement vector and
	// the sides of the shape.
	//
	// 2) of that side, return the edge that is closer to the target

	//1) copied from calculateReceivedPower
	bool doesIntersect = false;
	const Obstacle::Coords& shape = getShape();
	Obstacle::Coords::const_iterator i = shape.begin();
	Obstacle::Coords::const_iterator j = (shape.rbegin()+1).base();
	Coord p1,p2;
	int count = 0;
	for (; i != shape.end(); j = i++) {
		Coord c1 = *i;
		Coord c2 = *j;
		double i = segmentsIntersectAt(from, across, c1, c2, false);
		if (i != -1) {
			doesIntersect = true;
			count++;
			p1 = c1;
			p2 = c2;
   		DEBUG_OUT << "going to " << *to << ", intersecting from " << *from << " to "
   				<< *across << " with line " << c1
   				<< c2 << " % of side length " << i << std::endl;
		}
	}
	// we actually may "fly" over a very sharp angle crossing two lines,
	// so do not check this
	//	ASSERT(count == 1);

	// 2)
	Coord target;
	if (p1.sqrdist(to) < p2.sqrdist(to))
		target =  p1;
	else
		target =  p2;

	return target;
}
