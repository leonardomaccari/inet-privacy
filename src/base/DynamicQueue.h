//
// Copyright (C) 2011 Leonardo Maccari
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU  General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#ifndef __INET_DynamicQueue_H
#define __INET_DynamicQueue_H

#include "AbstractQueue.h"

/**
 * comment here...
 */
class INET_API DynamicQueue : public AbstractQueue
{
  protected:
    cMessage * endServiceMsg;

  public:
    DynamicQueue() {}

  protected:
    virtual void arrival(cPacket *msg);
    virtual cPacket *arrivalWhenIdle(cPacket *msg);
    virtual simtime_t startService(cPacket *msg);
    virtual void endService(cPacket* msg);
};

#endif

