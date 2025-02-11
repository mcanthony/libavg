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

#include "TexInfo.h"

#include "../base/Exception.h"
#include "../base/StringHelper.h"
#include "../base/MathHelper.h"
#include "../base/ObjectCounter.h"

#include "GLContext.h"

#include <string.h>
#include <iostream>

namespace avg {

using namespace std;

TexCompression string2TexCompression(const std::string& s)
{
    if (s == "none") {
        return TEXCOMPRESSION_NONE;
    } else if (s == "B5G6R5") {
        return TEXCOMPRESSION_B5G6R5;
    } else {
        throw(Exception(AVG_ERR_UNSUPPORTED, "Texture compression "+s+" not supported."));
    }
}

std::string texCompression2String(TexCompression compression)
{
    switch(compression) {
        case TEXCOMPRESSION_NONE:
            return "none";
        case TEXCOMPRESSION_B5G6R5:
            return "B5G6R5";
        default:
            AVG_ASSERT(false);
            return 0;
    }
}


TexInfo::TexInfo(const IntPoint& size, PixelFormat pf, bool bMipmap, bool bUsePOT,
        int potBorderColor)
    : m_Size(size),
      m_pf(pf),
      m_bMipmap(bMipmap),
      m_bUsePOT(bUsePOT),
      m_POTBorderColor(potBorderColor)
{
    if (m_bUsePOT) {
        m_GLSize.x = nextpow2(m_Size.x);
        m_GLSize.y = nextpow2(m_Size.y);
    } else {
        m_GLSize = m_Size;
    }

    int maxTexSize = GLContext::getCurrent()->getMaxTexSize();
    if (size.x > maxTexSize || size.y > maxTexSize) {
        throw Exception(AVG_ERR_VIDEO_GENERAL, "Texture too large ("  + toString(size)
                + "). Maximum supported by graphics card is "
                + toString(maxTexSize));
    }

    if (getGLType(m_pf) == GL_FLOAT && !isFloatFormatSupported()) {
        throw Exception(AVG_ERR_UNSUPPORTED, 
                "Float textures not supported by OpenGL configuration.");
    }
}

TexInfo::~TexInfo()
{
}

const IntPoint& TexInfo::getSize() const
{
    return m_Size;
}

const IntPoint& TexInfo::getGLSize() const
{
    return m_GLSize;
}

PixelFormat TexInfo::getPF() const
{
    return m_pf;
}
    
int TexInfo::getMemNeeded() const
{
    return m_GLSize.x*m_GLSize.y*getBytesPerPixel(m_pf);
}

IntPoint TexInfo::getMipmapSize(int level) const
{
    IntPoint size = m_Size;
    for (int i=0; i<level; ++i) {
        size.x = max(1, size.x >> 1);
        size.y = max(1, size.y >> 1);
    }
    return size;
}

bool TexInfo::isFloatFormatSupported()
{
    return queryOGLExtension("GL_ARB_texture_float");
}

int TexInfo::getGLFormat(PixelFormat pf)
{
    switch (pf) {
        case I8:
        case I32F:
            return GL_LUMINANCE;
        case A8:
            return GL_ALPHA;
        case R8:
            return GL_RED;
        case R8G8B8A8:
        case R8G8B8X8:
            return GL_RGBA;
        case B8G8R8A8:
        case B8G8R8X8:
            AVG_ASSERT(!GLContext::getCurrent()->isGLES());
            return GL_BGRA;
#ifndef AVG_ENABLE_EGL
        case R32G32B32A32F:
            return GL_BGRA;
#endif
        case R8G8B8:
        case B5G6R5:
            return GL_RGB;
        default:
            AVG_ASSERT(false);
            return 0;
    }
}

int TexInfo::getGLType(PixelFormat pf)
{
    switch (pf) {
        case I8:
        case A8:
        case R8:
            return GL_UNSIGNED_BYTE;
        case R8G8B8A8:
        case R8G8B8X8:
        case B8G8R8A8:
        case B8G8R8X8:
#ifdef __APPLE__
            return GL_UNSIGNED_INT_8_8_8_8_REV;
#else
            return GL_UNSIGNED_BYTE;
#endif
        case R32G32B32A32F:
        case I32F:
            return GL_FLOAT;
        case R8G8B8:
            return GL_UNSIGNED_BYTE;
        case B5G6R5:
            return GL_UNSIGNED_SHORT_5_6_5;
        default:
            AVG_ASSERT(false);
            return 0;
    }
}

int TexInfo::getGLInternalFormat() const
{
    switch (m_pf) {
        case I8:
            return GL_LUMINANCE;
        case A8:
            return GL_ALPHA;
        case R8:
            return GL_R8;
        case R8G8B8A8:
        case R8G8B8X8:
            return GL_RGBA;
        case B8G8R8A8:
        case B8G8R8X8:
            AVG_ASSERT(!GLContext::getCurrent()->isGLES());
            return GL_RGBA;
#ifndef AVG_ENABLE_EGL            
        case R32G32B32A32F:
            return GL_RGBA32F_ARB;
        case I32F:
            return GL_LUMINANCE32F_ARB;
#endif
        case R8G8B8:
        case B5G6R5:
            return GL_RGB;
        default:
            AVG_ASSERT(false);
            return 0;
    }
}

void TexInfo::dump() const
{
    cerr << "TexInfo" << endl;
    cerr << "m_Size: " << m_Size << endl;
    cerr << "m_GLSize: " << m_GLSize << endl;
    cerr << "m_pf: " << m_pf << endl;
    cerr << "m_bMipmap: " << m_bMipmap << endl;
}

bool TexInfo::getUseMipmap() const
{
    return m_bMipmap;
}

bool TexInfo::getUsePOT() const
{
    return m_bUsePOT;
}

int TexInfo::getPOTBorderColor() const
{
    return m_POTBorderColor;
}

bool TexInfo::usePOT(bool bForcePOT, bool bMipmap)
{
    GLContext* pGLContext = GLContext::getCurrent();
    if (pGLContext->usePOTTextures() || bForcePOT || (pGLContext->isGLES() && bMipmap)) {
        return true;
    } else {
        return false;
    }
}

}
