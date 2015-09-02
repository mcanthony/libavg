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

#ifndef _SkeletonEvent_H_
#define _SkeletonEvent_H_

#include "../api.h"
#include "Event.h"
#include "Skeleton.h"

#include "../base/GLMHelper.h"

#include <math.h>
#include <boost/weak_ptr.hpp>

namespace avg {

class SkeletonEvent;
typedef boost::shared_ptr<class SkeletonEvent> SkeletonEventPtr;
typedef boost::weak_ptr<class SkeletonEvent> SkeletonEventWeakPtr;

class AVG_API SkeletonEvent: public Event 
{
    public:
        SkeletonEvent(SkeletonPtr pSkeleton);
        virtual ~SkeletonEvent();

        float getUserID() const;
        const glm::vec3& getJoint(int i) const;

    private:
        SkeletonPtr m_pSkeleton;
};

}
#endif
