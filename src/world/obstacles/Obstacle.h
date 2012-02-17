//
// ObstacleControl - models obstacles that block radio transmissions
// Copyright (C) 2006 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
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


#ifndef WORLD_OBSTACLE_OBSTACLE_H
#define WORLD_OBSTACLE_OBSTACLE_H

#include <vector>
#include "Coord.h"
#include "world/annotations/AnnotationManager.h"

#ifndef DEBUG

#include <ostream>
struct nullstream : std::ostream {
	nullstream() : std::ios(0), std::ostream(0) {}
};
static nullstream logstream;
#define DEBUG_OUT if(0) logstream

#else

#define DEBUG_OUT std::cout
#endif


/**
 * stores information about an Obstacle for ObstacleControl
 */
class Obstacle {
    public:
        typedef std::vector<Coord> Coords;

        Obstacle(std::string id, double attenuationPerWall, double attenuationPerMeter);

        void setShape(Coords shape);
        const Coords& getShape() const;
        const Coord getBboxP1() const;
        const Coord getBboxP2() const;
        Coord getTurningPoint(Coord * from, Coord * to, Coord * across) const;

        double calculateReceivedPower(double pSend, double carrierFrequency, const Coord& senderPos, double senderAngle, const Coord& receiverPos, double receiverAngle) const;

        AnnotationManager::Annotation* visualRepresentation;

    protected:
        std::string id;
        double attenuationPerWall; /**< in dB. Consumer Wi-Fi vs. an exterior wall will give approx. 50 dB */
        double attenuationPerMeter; /**< in dB / m. Consumer Wi-Fi vs. an interior hollow wall will give approx. 5 dB */
        Coords coords;
        Coord bboxP1;
        Coord bboxP2;
};

namespace {

    bool isPointInObstacle(Coord point, const Obstacle& o) {
        bool isInside = false;
        const Obstacle::Coords& shape = o.getShape();
        Obstacle::Coords::const_iterator i = shape.begin();
        Obstacle::Coords::const_iterator j = (shape.rbegin()+1).base();
        for (; i != shape.end(); j = i++) {
            bool inYRangeUp = (point.y >= i->y) && (point.y < j->y);
            bool inYRangeDown = (point.y >= j->y) && (point.y < i->y);
            bool inYRange = inYRangeUp || inYRangeDown;
            if (!inYRange) continue;
            bool intersects = point.x < (i->x + ((point.y - i->y) * (j->x - i->x) / (j->y - i->y)));
            if (!intersects) continue;
            isInside = !isInside;
        }
        return isInside;
    }


    double segmentsIntersectAt(Coord p1From, Coord p1To, Coord p2From, Coord p2To, bool strict) {
        Coord p1Vec = p1To - p1From;
        Coord p2Vec = p2To - p2From;
        Coord p1p2 = p1From - p2From;

        double D = (p1Vec.x * p2Vec.y - p1Vec.y * p2Vec.x);
        if (D == 0) // segments are aligned and overlaping, the border is not the object
	       	return -1;

        double p1Frac = (p2Vec.x * p1p2.y - p2Vec.y * p1p2.x) / D;
        if (strict){
        	if (p1Frac < 0 || p1Frac > 1) return -1;
        	double p2Frac = (p1Vec.x * p1p2.y - p1Vec.y * p1p2.x) / D;
        	if (p2Frac < 0 || p2Frac > 1) return -1;
        } else {
        	if (p1Frac <= 0 || p1Frac >= 1) return -1;
        	double p2Frac = (p1Vec.x * p1p2.y - p1Vec.y * p1p2.x) / D;
        	if (p2Frac <= 0 || p2Frac >= 1) return -1;
        }
        return p1Frac;
    }

    double segmentsIntersectAt(Coord p1From, Coord p1To, Coord p2From, Coord p2To) {
      	return segmentsIntersectAt(p1From, p1To, p2From, p2To, false);
      }

}
#endif
