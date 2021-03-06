/*
 * Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include "StaticGridMobility.h"


Define_Module(StaticGridMobility);


StaticGridMobility::StaticGridMobility()
{
    marginX = 0;
    marginY = 0;
    numHosts = 0;
}

void StaticGridMobility::initialize(int stage)
{
    MobilityBase::initialize(stage);
    EV << "initializing StaticGridMobility stage " << stage << endl;
    if (stage == 0)
    {
        numHosts = par("numHosts");
        marginX = par("marginX");
        marginY = par("marginY");
        horizSize = par("horizontalSize");
        vertSize = 0;
    }
}

void StaticGridMobility::initializePosition()
{
    int index = visualRepresentation->getIndex();

    double realSize = sqrt((double)numHosts);
    bool isSquared;
    if(horizSize == 0)
        horizSize = (int)ceil(sqrt((double)numHosts));

    if ((realSize - horizSize) == 0 )
        isSquared = 1;
    else
        isSquared = 0;

    if(!isSquared)
        vertSize = (int)ceil((double)numHosts/horizSize);
    else
        vertSize = horizSize;

    int col = (index) % horizSize;
    double row = floor((index) / (double)horizSize);

    int roundedInt;
    if (horizSize == 1)
    	roundedInt = 1;
    else
    	roundedInt = horizSize - 1;
    lastPosition.x = constraintAreaMin.x + marginX
        + col * ((constraintAreaMax.x - constraintAreaMin.x) - 2 * marginX) / roundedInt;

    if (lastPosition.x >= constraintAreaMax.x)
        lastPosition.x -= 1;
    if (vertSize == 1)
    	roundedInt = 1;
    else
    	roundedInt = vertSize - 1;
    lastPosition.y = constraintAreaMin.y + marginY
        + row * ((constraintAreaMax.y - constraintAreaMin.y) - 2 * marginY) / roundedInt;

    if (lastPosition.y >= constraintAreaMax.y)
        lastPosition.y -= 1;
    lastPosition.z = 0;


/*  

    int size = (int)ceil(sqrt((double)numHosts));
    int row = (int)floor((double)index / (double)size);
    int col = index % size;
    lastPosition.x = constraintAreaMin.x + marginX
            + col * ((constraintAreaMax.x - constraintAreaMin.x) - 2 * marginX) / (size - 1);
    if (lastPosition.x >= constraintAreaMax.x)
        lastPosition.x -= 1;
    lastPosition.y = constraintAreaMin.y + marginY
            + row * ((constraintAreaMax.y - constraintAreaMin.y) - 2 * marginY) / (size - 1);
    if (lastPosition.y >= constraintAreaMax.y)
        lastPosition.y -= 1;
    lastPosition.z = 0;
    */
}

void StaticGridMobility::finish()
{
    MobilityBase::finish();
    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
}
