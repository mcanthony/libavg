//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2014 Ulrich von Zadow
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at www.libavg.de
//

#include "Skeleton.h"

#include "../base/Exception.h"

using namespace std;

namespace avg {

Skeleton::Skeleton(int userID)
    : m_Status(DOWN),
      m_UserID(userID),
      m_bDownHasBeenSent(false)
{
}

void Skeleton::setStatus(SkelStatus status)
{
    m_Status = status;
}

void Skeleton::clearJoints()
{
    m_Joints.clear();
}

void Skeleton::addJoint(const glm::vec3& pos)
{
    m_Joints.push_back(pos);
    AVG_ASSERT(m_Joints.size() < 26);
}
    
void Skeleton::setDownSent()
{
    m_bDownHasBeenSent = true;
}

bool Skeleton::hasDownBeenSent() const
{
    return m_bDownHasBeenSent;
}

Event::Type Skeleton::getEventType() const
{
    switch (m_Status) {
        case DOWN:
            return Event::CURSOR_DOWN;
        case MOVE:
            return Event::CURSOR_MOTION;
        case UP:
            return Event::CURSOR_UP;
        default:
            AVG_ASSERT(false);
            return Event::UNKNOWN;
    }
}

Skeleton::SkelStatus Skeleton::getStatus() const
{
    return m_Status;
}

int Skeleton::getUserID() const
{
    return m_UserID;
}

const vector<glm::vec3>& Skeleton::getJoints() const
{
    return m_Joints;
}

}
