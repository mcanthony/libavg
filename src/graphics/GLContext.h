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
#ifndef _GLContext_H_
#define _GLContext_H_

#include "../api.h"

#include "GLBufferCache.h"
#include "GLConfig.h"

#include "../base/GLMHelper.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread/tss.hpp>

namespace avg {

class GLContext;
typedef boost::shared_ptr<GLContext> GLContextPtr;
class ShaderRegistry;
typedef boost::shared_ptr<ShaderRegistry> ShaderRegistryPtr;
class StandardShader;

class AVG_API GLContext
{
public:
    GLContext(const IntPoint& windowSize);
    virtual ~GLContext();

    virtual void activate()=0;
    ShaderRegistryPtr getShaderRegistry() const;
    StandardShader* getStandardShader();
    bool useGPUYUVConversion() const;
    GLConfig::ShaderUsage getShaderUsage() const;

    // GL Object caching.
    GLBufferCache& getPBOCache();
    unsigned genFBO();
    void returnFBOToCache(unsigned fboID);

    // GL state cache.
    void setBlendColor(const glm::vec4& color);
    enum BlendMode {BLEND_BLEND, BLEND_ADD, BLEND_MIN, BLEND_MAX, BLEND_COPY};
    void setBlendMode(BlendMode mode, bool bPremultipliedAlpha = false);
    bool isBlendModeSupported(BlendMode mode) const;
    void bindTexture(unsigned unit, unsigned texID);

    const GLConfig& getConfig();
    void logConfig();
    size_t getVideoMemInstalled();
    size_t getVideoMemUsed();
    int getMaxTexSize();
    bool usePOTTextures();
    bool arePBOsSupported();
    OGLMemoryMode getMemoryMode();
    bool isGLES() const;
    bool isVendor(const std::string& sWantedVendor) const;
    virtual bool useDepthBuffer() const;

    virtual bool initVBlank(int rate);
    virtual void swapBuffers() = 0;

    static void enableErrorChecks(bool bEnable);
    static void checkError(const char* pszWhere);
    static void mandatoryCheckError(const char* pszWhere);
    void ensureFullShaders(const std::string& sContext) const;

    static BlendMode stringToBlendMode(const std::string& s);

    static GLContext* getCurrent();

    static int nextMultiSampleValue(int curSamples);
    static void enableErrorLog(bool bEnable);

protected:
    void init(const GLConfig& glConfig, bool bOwnsContext);
    void deleteObjects();

    void getVersion(int& major, int& minor) const;
    bool ownsContext() const;

    void setCurrent();

private:
    void checkGPUMemInfoSupport();
    bool isDebugContextSupported() const;
    static void APIENTRY debugLogCallback(GLenum source, GLenum type, GLuint id, 
        GLenum severity, GLsizei length, const GLchar* message, void* userParam);

    bool m_bOwnsContext;
    
    ShaderRegistryPtr m_pShaderRegistry;
    StandardShader* m_pStandardShader;

    GLBufferCache m_PBOCache;
    std::vector<unsigned int> m_FBOIDs;

    int m_MaxTexSize;
    GLConfig m_GLConfig;
    bool m_bCheckedGPUMemInfoExtension;
    bool m_bGPUMemInfoSupported;
    bool m_bCheckedMemoryMode;
    OGLMemoryMode m_MemoryMode;

    // OpenGL state
    glm::vec4 m_BlendColor;
    BlendMode m_BlendMode;
    bool m_bPremultipliedAlpha;
    unsigned m_BoundTextures[16];

    int m_MajorGLVersion;
    int m_MinorGLVersion;

    static bool s_bErrorCheckEnabled;
    static bool s_bErrorLogEnabled;

    static GLContext* s_pCurrentContext;
};

}
#endif


