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

#include "OGLTexture.h"
#include "SDLDisplayEngine.h"
#include "../graphics/VertexArray.h"
#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"

#include <iostream>
#include <string>

namespace avg {

using namespace std;
    
OGLTexture::OGLTexture(IntRect TexExtent, IntPoint TexSize, IntPoint TileSize,
        IntRect TileIndexExtent, PixelFormat pf, SDLDisplayEngine * pEngine) 
    : m_TexExtent(TexExtent),
      m_TexSize(TexSize),
      m_TileSize(TileSize),
      m_TileIndexExtent(TileIndexExtent),
      m_pf(pf),
      m_pEngine(pEngine)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    createTextures();
    int numTiles = m_TileIndexExtent.width()*m_TileIndexExtent.height();
    m_pVertexes = new VertexArray(numTiles*4, numTiles*6);
    calcTexCoords();
}

OGLTexture::~OGLTexture()
{
    delete m_pVertexes;
    deleteTextures();
    ObjectCounter::get()->decRef(&typeid(*this));
}

void OGLTexture::resize(IntRect TexExtent, IntPoint TexSize, IntPoint TileSize)
{
    deleteTextures();
    m_TexExtent = TexExtent;
    m_TexSize = TexSize;
    m_TileSize = TileSize;
    createTextures();
    calcTexCoords();
}

int OGLTexture::getTexID(int i) const
{
    return m_TexID[i];
}

OGLShaderPtr OGLTexture::getFragmentShader() {
    OGLShaderPtr pShader;
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        if (m_pf == YCbCr420p) {
            pShader = m_pEngine->getYCbCr420pShader();
        } else {
            pShader = m_pEngine->getYCbCrJ420pShader();
        }
        //cerr<<"Please?!? (()) "<<pShader<<"\n";
    }
    //cerr<<"Texture::getFrag "<<pShader<<" "<<this<<"\n";
    return pShader;
}

void OGLTexture::blt(const VertexGrid* pVertexes) const
{
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        OGLProgramPtr pShader = m_pEngine->getActiveShader();
        assert(pShader);
        //pShader->activate();
        //glproc::ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_TexID[0]);
        pShader->setUniformIntParam("YTexture", 0);
        glproc::ActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_TexID[1]);
        pShader->setUniformIntParam("CbTexture", 1);
        glproc::ActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_TexID[2]);
        pShader->setUniformIntParam("CrTexture", 2);
    } else {
        glproc::ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_TexID[0]);
        //Igor: Now handled in a more general manner
        //if (m_pEngine->getYCbCrMode() == OGL_SHADER) {
        //    glproc::UseProgramObject(0);
        //}
    }
    if (pVertexes) {
        m_pVertexes->reset();
        for (int y=m_TileIndexExtent.tl.y; y<m_TileIndexExtent.br.y; y++) {
            for (int x=m_TileIndexExtent.tl.x; x<m_TileIndexExtent.br.x; x++) {
                int xoffset = x-m_TileIndexExtent.tl.x;
                int yoffset = y-m_TileIndexExtent.tl.y;
                int curVertex=m_pVertexes->getCurVert();
                m_pVertexes->appendPos((*pVertexes)[y][x], 
                        m_TexCoords[yoffset][xoffset]); 
                m_pVertexes->appendPos((*pVertexes)[y][x+1], 
                        m_TexCoords[yoffset][xoffset+1]); 
                m_pVertexes->appendPos((*pVertexes)[y+1][x+1],
                        m_TexCoords[yoffset+1][xoffset+1]); 
                m_pVertexes->appendPos((*pVertexes)[y+1][x],
                        m_TexCoords[yoffset+1][xoffset]); 
                m_pVertexes->appendQuadIndexes(
                        curVertex+1, curVertex, curVertex+2, curVertex+3);
            }
        }
    }

    m_pVertexes->draw();
    
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        glproc::ActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glproc::ActiveTexture(GL_TEXTURE2);
        glDisable(GL_TEXTURE_2D);
        glproc::ActiveTexture(GL_TEXTURE0);
        glproc::UseProgramObject(0);
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTexture::blt: glDisable(GL_TEXTURE_2D)");
    }
}

void OGLTexture::createTextures()
{
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        createTexture(0, m_TexSize, I8);
        createTexture(1, m_TexSize/2, I8);
        createTexture(2, m_TexSize/2, I8);
    } else {
        createTexture(0, m_TexSize, m_pf);
    }
}

void OGLTexture::createTexture(int i, IntPoint Size, PixelFormat pf)
{
    glGenTextures(1, &m_TexID[i]);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTexture::createTexture: glGenTextures()");
    glproc::ActiveTexture(GL_TEXTURE0+i);
    glBindTexture(GL_TEXTURE_2D, m_TexID[i]);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTexture::createTexture: glBindTexture()");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTexture::createTexture: glTexParameteri()");
    glPixelStorei(GL_UNPACK_ROW_LENGTH, Size.x);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTexture::createTexture: glPixelStorei(GL_UNPACK_ROW_LENGTH)");
    
    GLenum DestMode = m_pEngine->getOGLDestMode(pf);
    char * pPixels = 0;
    if (m_pEngine->usePOTTextures()) {
        // Make sure the texture is transparent and black before loading stuff 
        // into it to avoid garbage at the borders.
        int TexMemNeeded = Size.x*Size.y*Bitmap::getBytesPerPixel(pf);
        pPixels = new char[TexMemNeeded];
        memset(pPixels, 0, TexMemNeeded);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, DestMode, Size.x, Size.y, 0,
            m_pEngine->getOGLSrcMode(pf), m_pEngine->getOGLPixelType(pf), pPixels);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTexture::createTexture: glTexImage2D()");
    if (m_pEngine->usePOTTextures()) {
        free(pPixels);
    }
}

void OGLTexture::deleteTextures()
{
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        glDeleteTextures(3, m_TexID);
    } else {
        glDeleteTextures(1, m_TexID);
    }
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTexture::~OGLTexture: glDeleteTextures()");    
}

static ProfilingZone TexSubImageProfilingZone("OGLTexture::texture download");

void OGLTexture::downloadTexture(int i, BitmapPtr pBmp, int stride, 
                OGLMemoryMode MemoryMode) const
{
    PixelFormat pf;
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        pf = I8;
    } else {
        pf = m_pf;
    }
    IntRect Extent = m_TexExtent;
    if (i != 0) {
        stride /= 2;
        Extent = IntRect(m_TexExtent.tl/2.0, m_TexExtent.br/2.0);
    }
    glBindTexture(GL_TEXTURE_2D, m_TexID[i]);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTexture::downloadTexture: glBindTexture()");
    int bpp = Bitmap::getBytesPerPixel(pf);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTexture::downloadTexture: glPixelStorei(GL_UNPACK_ROW_LENGTH)");
//    cerr << "OGLTexture::downloadTexture(" << pBmp << ", stride=" << stride 
//        << ", Extent= " << m_TexExtent << ", pf= " << Bitmap::getPixelFormatString(m_pf)
//        << ", bpp= " << bpp << endl;
    unsigned char * pStartPos = (unsigned char *)
            (ptrdiff_t)(Extent.tl.y*stride*bpp + Extent.tl.x*bpp);
    if (MemoryMode == OGL) {
        pStartPos += (ptrdiff_t)(pBmp->getPixels());
    }
    {
        ScopeTimer Timer(TexSubImageProfilingZone);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Extent.width(), Extent.height(),
                m_pEngine->getOGLSrcMode(pf), m_pEngine->getOGLPixelType(pf), 
                pStartPos);
    }
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTexture::downloadTexture: glTexSubImage2D()");
}

void OGLTexture::calcTexCoords()
{
    double TexWidth;
    double TexHeight;
    double TexWidthPerTile;
    double TexHeightPerTile;
/*   
    cerr << "----calcTexCoords" << endl;
    cerr << "m_TexExtent: " << m_TexExtent << endl;
    cerr << "m_TileSize: " << m_TileSize << endl;
    cerr << "m_TexSize: " << m_TexSize << endl;
*/    
    TexWidth = double(m_TexExtent.width())/m_TexSize.x;
    TexHeight = double(m_TexExtent.height())/m_TexSize.y;
    TexWidthPerTile=double(m_TileSize.x)/m_TexSize.x;
    TexHeightPerTile=double(m_TileSize.y)/m_TexSize.y;
/*
    cerr << "TexSize: " << TexWidth << ", " << TexHeight << endl;
    cerr << "TexTileSize: " << TexWidthPerTile << ", " << TexHeightPerTile << endl;
*/
    vector<DPoint> TexCoordLine(m_TileIndexExtent.width()+1);
    m_TexCoords = std::vector<std::vector<DPoint> > 
            (m_TileIndexExtent.height()+1, TexCoordLine);
    for (unsigned int y=0; y<m_TexCoords.size(); y++) {
        for (unsigned int x=0; x<m_TexCoords[y].size(); x++) {
            if (y == m_TexCoords.size()-1) {
                m_TexCoords[y][x].y = TexHeight;
            } else {
                m_TexCoords[y][x].y = TexHeightPerTile*y;
            }
            if (x == m_TexCoords[y].size()-1) {
                m_TexCoords[y][x].x = TexWidth;
            } else {
                m_TexCoords[y][x].x = TexWidthPerTile*x;
            }
        }
    }
//    cerr << endl;
}

const IntRect& OGLTexture::getTileIndexExtent() const
{
    return m_TileIndexExtent;
}

const int OGLTexture::getTexMemDim()
{
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        return int(m_TexSize.x * m_TexSize.y * 1.5);
    }
    else {
        return m_TexSize.x * m_TexSize.y * Bitmap::getBytesPerPixel(m_pf);
    }
}

const PixelFormat OGLTexture::getPixelFormat() const
{
    return m_pf;
}

}
