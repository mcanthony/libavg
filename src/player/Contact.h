//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2008 Ulrich von Zadow
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

#ifndef _Contact_H_
#define _Contact_H_

#include "CursorEvent.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>

namespace avg {

class CursorEvent;
typedef boost::shared_ptr<class CursorEvent> CursorEventPtr;
class Contact;
typedef boost::shared_ptr<class Contact> ContactPtr;
typedef boost::weak_ptr<class Contact> ContactWeakPtr;

class Contact {
public:
    Contact(CursorEventPtr pEvent);
    virtual ~Contact();

    void setThis(ContactWeakPtr This);
    ContactPtr getThis() const;

    void pushEvent(CursorEventPtr pEvent);
    CursorEventPtr pollEvent();
    CursorEventPtr getLastEvent();

private:
    std::vector<CursorEventPtr> m_pEvents;
    std::vector<CursorEventPtr> m_pNewEvents;

    ContactWeakPtr m_This;    

    int m_CursorID;
};

}

#endif
