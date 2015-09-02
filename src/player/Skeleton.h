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

#ifndef _Skeleton_H_
#define _Skeleton_H_

#include "../api.h"
#include "Event.h"
#include "../base/GLMHelper.h"

#include <boost/shared_ptr.hpp>
#include <vector>

namespace avg {

class AVG_API Skeleton
{
public:
    enum SkelStatus {DOWN, MOVE, UP};

    Skeleton(int userID);

    void setStatus(SkelStatus status);
    void clearJoints();
    void addJoint(const glm::vec3& pos);

    Event::Type getEventType() const;
    SkelStatus getStatus() const;
    int getUserID() const;
    const glm::vec3& getJoint(int i) const;

private:
    SkelStatus m_Status;
    int m_UserID;
    std::vector<glm::vec3> m_Joints;
};

typedef boost::shared_ptr<Skeleton> SkeletonPtr;

}

#endif


