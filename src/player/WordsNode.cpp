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

#include "WordsNode.h"
#include "SDLDisplayEngine.h"
#include "OGLSurface.h"
#include "NodeDefinition.h"
#include "TextEngine.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ScopeTimer.h"
#include "../base/XMLHelper.h"
#include "../base/StringHelper.h"
#include "../base/MathHelper.h"
#include "../base/ObjectCounter.h"

#include "../graphics/Filterfill.h"

#include <pango/pangoft2.h>

#include <iostream>
#include <algorithm>

using namespace std;

namespace avg {

NodeDefinition WordsNode::createDefinition()
{
    static const string sDTDElements = 
        "<!ELEMENT span (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ATTLIST span\n"
        "  font_desc CDATA #IMPLIED\n"
        "  font_family CDATA #IMPLIED\n"
        "  face CDATA #IMPLIED\n"
        "  size CDATA #IMPLIED\n"
        "  style CDATA #IMPLIED\n"
        "  weight CDATA #IMPLIED\n"
        "  variant CDATA #IMPLIED\n"
        "  stretch CDATA #IMPLIED\n"
        "  foreground CDATA #IMPLIED\n"
        "  background CDATA #IMPLIED\n"
        "  underline CDATA #IMPLIED\n"
        "  rise CDATA #IMPLIED\n"
        "  strikethrough CDATA #IMPLIED\n"
        "  fallback CDATA #IMPLIED\n"
        "  lang CDATA #IMPLIED\n"
        "  letter_spacing CDATA #IMPLIED\n"
        "  rawtextmode CDATA #IMPLIED >\n"

        "<!ELEMENT b (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT big (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT i (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT s (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT sub (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT sup (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT small (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT tt (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT u (#PCDATA|span|b|big|i|s|sub|sup|small|tt|u)*>\n"
        "<!ELEMENT br (#PCDATA)*>\n";

    string sChildArray[] = {"#PCDATA", "span", "b", "big", "i", "s", "sup", "sub", 
            "small", "tt", "u", "br"};
    vector<string> sChildren = vectorFromCArray(sizeof(sChildArray)/sizeof(*sChildArray),
            sChildArray); 
    return NodeDefinition("words", Node::buildNode<WordsNode>)
        .extendDefinition(RasterNode::createDefinition())
        .addChildren(sChildren)
        .addDTDElements(sDTDElements)
        .addArg(Arg<string>("font", "arial", false, offsetof(WordsNode, m_sFontName)))
        .addArg(Arg<string>("variant", "", false, offsetof(WordsNode, m_sFontVariant)))
        .addArg(Arg<UTF8String>("text", ""))
        .addArg(Arg<string>("color", "FFFFFF", false, offsetof(WordsNode, m_sColorName)))
        .addArg(Arg<double>("fontsize", 15, false, offsetof(WordsNode, m_FontSize)))
        .addArg(Arg<int>("indent", 0, false, offsetof(WordsNode, m_Indent)))
        .addArg(Arg<double>("linespacing", -1, false, offsetof(WordsNode, m_LineSpacing)))
        .addArg(Arg<string>("alignment", "left"))
        .addArg(Arg<string>("wrapmode", "word"))
        .addArg(Arg<bool>("justify", false, false, offsetof(WordsNode, m_bJustify)))
        .addArg(Arg<bool>("rawtextmode", false, false, 
                offsetof(WordsNode, m_bRawTextMode)))
        .addArg(Arg<double>("letterspacing", 0, false, 
                offsetof(WordsNode, m_LetterSpacing)))
        .addArg(Arg<bool>("hint", true, false, offsetof(WordsNode, m_bHint)))
        ;
}

WordsNode::WordsNode(const ArgList& Args)
    : m_StringExtents(0,0),
      m_pFontDescription(0),
      m_pLayout(0),
      m_bFontChanged(true),
      m_bDrawNeeded(true)
{
    m_bParsedText = false;

    Args.setMembers(this);
    setAlignment(Args.getArgVal<string>("alignment"));
    setWrapMode(Args.getArgVal<string>("wrapmode"));
    setText(Args.getArgVal<UTF8String>("text"));
    m_Color = colorStringToColor(m_sColorName);
    setViewport(-32767, -32767, -32767, -32767);
    ObjectCounter::get()->incRef(&typeid(*this));
}

WordsNode::~WordsNode()
{
    if (m_pFontDescription) {
        pango_font_description_free(m_pFontDescription);
    }
    if (m_pLayout) {
        g_object_unref(m_pLayout);
    }
    ObjectCounter::get()->decRef(&typeid(*this));
}

void WordsNode::setTextFromNodeValue(const string& sText)
{
//    cerr << "NODE VALUE: " << sText << "|" << endl;
    // Gives priority to Node Values only if they aren't empty
    UTF8String sTemp = removeExcessSpaces(sText);
    if (sTemp.length() != 0) {
        setText(sText);
    }
}

void WordsNode::setRenderingEngines(DisplayEngine * pDisplayEngine, 
        AudioEngine * pAudioEngine)
{
    m_bFontChanged = true;
    m_bDrawNeeded = true;
    RasterNode::setRenderingEngines(pDisplayEngine, pAudioEngine);
}

void WordsNode::connect()
{
    RasterNode::connect();
    checkReload();
}

void WordsNode::disconnect(bool bKill)
{
    if (m_pFontDescription) {
        pango_font_description_free(m_pFontDescription);
        m_pFontDescription = 0;
        m_bFontChanged = true;
    }
    RasterNode::disconnect(bKill);
}

string WordsNode::getAlignment() const
{
    switch(m_Alignment) {
        case PANGO_ALIGN_LEFT:
            return "left";
        case PANGO_ALIGN_CENTER:
            return "center";
        case PANGO_ALIGN_RIGHT:
            return "right";
        default:
            assert(false);
            return "";
    }
}

void WordsNode::setAlignment(const string& sAlign)
{
    if (sAlign == "left") {
        m_Alignment = PANGO_ALIGN_LEFT;
    } else if (sAlign == "center") {
        m_Alignment = PANGO_ALIGN_CENTER;
    } else if (sAlign == "right") {
        m_Alignment = PANGO_ALIGN_RIGHT;
    } else {
        throw(Exception(AVG_ERR_UNSUPPORTED, 
                "WordsNode alignment "+sAlign+" not supported."));
    }

    m_bDrawNeeded = true;
}

bool WordsNode::getJustify() const
{
    return m_bJustify;
}

void WordsNode::setJustify(bool bJustify)
{
    m_bJustify = bJustify;
    m_bDrawNeeded = true;
}

double WordsNode::getLetterSpacing() const
{
    return m_LetterSpacing;
}

void WordsNode::setLetterSpacing(double letterSpacing)
{
    m_LetterSpacing = letterSpacing;
    m_bDrawNeeded = true;
}

bool WordsNode::getHint() const
{
    return m_bHint;
}

void WordsNode::setHint(bool bHint)
{
    m_bHint = bHint;
}


double WordsNode::getWidth() 
{
    drawString();
    return AreaNode::getWidth();
}

double WordsNode::getHeight()
{
    drawString();
    return AreaNode::getHeight();
}

NodePtr WordsNode::getElementByPos(const DPoint & pos)
{
    drawString();
    DPoint relPos = pos-DPoint(m_PosOffset);
    return AreaNode::getElementByPos(relPos);
}

const std::string& WordsNode::getFont() const
{
    return m_sFontName;
}

void WordsNode::setFont(const std::string& sName)
{
    m_sFontName = sName;
    m_bFontChanged = true;
    m_bDrawNeeded = true;
}

const std::string& WordsNode::getFontVariant() const
{
    return m_sFontVariant;
}

void WordsNode::addFontDir(const std::string& sDir)
{
    TextEngine::get(true).addFontDir(sDir);
    TextEngine::get(false).addFontDir(sDir);
}

void WordsNode::setFontVariant(const std::string& sVariant)
{
    m_sFontVariant = sVariant;
    m_bFontChanged = true;
    m_bDrawNeeded = true;
}

const UTF8String& WordsNode::getText() const 
{
    return m_sRawText;
}

void WordsNode::setText(const UTF8String& sText)
{
    if (sText.length() > 32767) {
        throw(Exception(AVG_ERR_INVALID_ARGS, 
                string("WordsNode::setText: string too long (") 
                        + toString(sText.length()) + ")"));
    }
//    cerr << "setText(): " << sText << "|" << endl;
    if (m_sRawText != sText) {
        m_sRawText = sText;
        m_sText = m_sRawText;
        if (m_bRawTextMode) {
            m_bParsedText = false;
        } else {
            setParsedText(sText);
        }
        m_bDrawNeeded = true;
    }
}

const std::string& WordsNode::getColor() const
{
    return m_sColorName;
}

void WordsNode::setColor(const std::string& sColor)
{
    m_sColorName = sColor;
    m_Color = colorStringToColor(m_sColorName);
    m_bDrawNeeded = true;
}

double WordsNode::getFontSize() const
{
    return m_FontSize;
}

void WordsNode::setFontSize(double Size)
{
    m_FontSize = Size;
    m_bFontChanged = true;
    m_bDrawNeeded = true;
}

int WordsNode::getIndent() const
{
    return m_Indent;
}

void WordsNode::setIndent(int Indent)
{
    m_Indent = Indent;
    m_bDrawNeeded = true;
}

double WordsNode::getLineSpacing() const
{
    return m_LineSpacing;
}

void WordsNode::setLineSpacing(double LineSpacing)
{
    m_LineSpacing = LineSpacing;
    m_bDrawNeeded = true;
}

bool WordsNode::getRawTextMode() const
{
    return m_bRawTextMode;
}

void WordsNode::setRawTextMode(bool RawTextMode)
{
    if (RawTextMode != m_bRawTextMode) {
        m_sText = m_sRawText;
        if (RawTextMode) {
            m_bParsedText = false;
        } else {
            setParsedText(m_sText);
        }
        m_bRawTextMode = RawTextMode;
        m_bDrawNeeded = true;
    }
}

DPoint WordsNode::getGlyphPos(int i)
{
    PangoRectangle rect = getGlyphRect(i);
    return DPoint(double(rect.x)/PANGO_SCALE, double(rect.y)/PANGO_SCALE);
}

DPoint WordsNode::getGlyphSize(int i)
{
    PangoRectangle rect = getGlyphRect(i);
    return DPoint(double(rect.width)/PANGO_SCALE, double(rect.height)/PANGO_SCALE);
}

void WordsNode::setWrapMode(const string& sWrapMode)
{
    if (sWrapMode == "word") {
        m_WrapMode = PANGO_WRAP_WORD;
    } else if (sWrapMode == "char") {
        m_WrapMode = PANGO_WRAP_CHAR;
    } else if (sWrapMode == "wordchar") {
        m_WrapMode = PANGO_WRAP_WORD_CHAR;
    } else {
        throw(Exception(AVG_ERR_UNSUPPORTED, 
                "WordsNode wrapping mode "+sWrapMode+" not supported."));
    }

    m_bDrawNeeded = true;
}

string WordsNode::getWrapMode() const
{
    switch(m_WrapMode) {
        case PANGO_WRAP_WORD:
            return "word";
        case PANGO_WRAP_CHAR:
            return "char";
        case PANGO_WRAP_WORD_CHAR:
            return "wordchar";
        default:
            assert(false);
            return "";
    }
}

void WordsNode::parseString(PangoAttrList** ppAttrList, char** ppText)
{
    UTF8String sTextWithoutBreaks = applyBR(m_sText);
    bool bOk;
    GError * pError = 0;
    bOk = (pango_parse_markup(sTextWithoutBreaks.c_str(), 
            int(sTextWithoutBreaks.length()), 0,
            ppAttrList, ppText, 0, &pError) != 0);
    if (!bOk) {
        string sError;
        if (getID() != "") {
            sError = string("Can't parse string in node with id '")+getID()+"' ("
                    +pError->message+")";
        } else {
            sError = string("Can't parse string '")+m_sRawText+"' ("+pError->message+")";
        }
        throw Exception(AVG_ERR_CANT_PARSE_STRING, sError);
    }

}

void WordsNode::setMaterialMask(MaterialInfo& material, const DPoint& pos, 
        const DPoint& size, const DPoint& mediaSize)
{
    DPoint maskSize;
    DPoint maskPos;
    if (size == DPoint(0,0)) {
        maskSize = DPoint(1,1);
        maskPos = DPoint(pos.x/mediaSize.x, pos.y/mediaSize.y);
    } else {
        maskSize = DPoint(size.x/mediaSize.x, size.y/mediaSize.y);
        maskPos = DPoint(pos.x/size.x, pos.y/size.y);
    }
    material.setMask(true);
    material.setMaskCoords(maskPos, maskSize);
}

static ProfilingZone DrawStringProfilingZone("  WordsNode::drawString");

void WordsNode::drawString()
{
    assert (m_sText.length() < 32767);
    if (!m_bDrawNeeded) {
        return;
    }
    ScopeTimer Timer(DrawStringProfilingZone);
    if (m_sText.length() == 0) {
        m_StringExtents = IntPoint(0,0);
    } else {
        if (m_bFontChanged) {
            if (m_pFontDescription) {
                pango_font_description_free(m_pFontDescription);
            }
            m_pFontDescription = TextEngine::get(m_bHint).getFontDescription(m_sFontName, 
                    m_sFontVariant);
            pango_font_description_set_absolute_size(m_pFontDescription,
                    (int)(m_FontSize * PANGO_SCALE));

            m_bFontChanged = false;
        }
        PangoContext* pContext = TextEngine::get(m_bHint).getPangoContext();
        pango_context_set_font_description(pContext, m_pFontDescription);

        if (m_pLayout) {
            g_object_unref(m_pLayout);
        }
        m_pLayout = pango_layout_new(pContext);

        PangoAttrList * pAttrList = 0;
#if PANGO_VERSION > PANGO_VERSION_ENCODE(1,18,2) 
        PangoAttribute * pLetterSpacing = pango_attr_letter_spacing_new
                (int(m_LetterSpacing*1024));
#endif
        if (m_bParsedText) {
            char * pText = 0;
            parseString(&pAttrList, &pText);
#if PANGO_VERSION > PANGO_VERSION_ENCODE(1,18,2) 
            // Workaround for pango bug.
            pango_attr_list_insert_before(pAttrList, pLetterSpacing);
#endif            
            pango_layout_set_text(m_pLayout, pText, -1);
            g_free (pText);
        } else {
            pAttrList = pango_attr_list_new();
#if PANGO_VERSION > PANGO_VERSION_ENCODE(1,18,2) 
            pango_attr_list_insert_before(pAttrList, pLetterSpacing);
#endif
            pango_layout_set_text(m_pLayout, m_sText.c_str(), -1);
        }
        pango_layout_set_attributes(m_pLayout, pAttrList);
        pango_attr_list_unref(pAttrList);

        pango_layout_set_wrap(m_pLayout, m_WrapMode);
        pango_layout_set_alignment(m_pLayout, m_Alignment);
        pango_layout_set_justify(m_pLayout, m_bJustify);
        if (getUserSize().x != 0) {
            pango_layout_set_width(m_pLayout, int(getUserSize().x * PANGO_SCALE));
        }
        pango_layout_set_indent(m_pLayout, m_Indent * PANGO_SCALE);
        if (m_Indent < 0) {
            // For hanging indentation, we add a tabstop to support lists
            PangoTabArray* pTabs = pango_tab_array_new_with_positions(1, false,
                    PANGO_TAB_LEFT, -m_Indent * PANGO_SCALE);
            pango_layout_set_tabs(m_pLayout, pTabs);
            pango_tab_array_free(pTabs);
        }
        if (m_LineSpacing != -1) {
            pango_layout_set_spacing(m_pLayout, (int)(m_LineSpacing*PANGO_SCALE));
        }
        PangoRectangle logical_rect;
        PangoRectangle ink_rect;
        pango_layout_get_pixel_extents(m_pLayout, &ink_rect, &logical_rect);
        assert (logical_rect.width < 4096);
        assert (logical_rect.height < 4096);
//        cerr << "Ink: " << ink_rect.x << ", " << ink_rect.y << ", " 
//                << ink_rect.width << ", " << ink_rect.height << endl;
//        cerr << "Logical: " << logical_rect.x << ", " << logical_rect.y << ", " 
//                << logical_rect.width << ", " << logical_rect.height << endl;
        m_StringExtents.y = logical_rect.height+2;
        if (getUserSize().x == 0) {
            m_StringExtents.x = logical_rect.width;
            if (m_Alignment == PANGO_ALIGN_LEFT) {
                // This makes sure we have enough room for italic text.
                // XXX: We should really do some calculations based on ink_rect here.
                m_StringExtents.x += int(m_FontSize/6+1);
            }
        } else {
            m_StringExtents.x = int(getUserSize().x);
        }
        if (m_StringExtents.x == 0) {
            m_StringExtents.x = 1;
        }
        if (m_StringExtents.y == 0) {
            m_StringExtents.y = 1;
        }
//        cerr << "libavg Extents: " << m_StringExtents << endl;
        if (getState() == NS_CANRENDER) {
            getSurface()->create(m_StringExtents, I8);

            BitmapPtr pBmp = getSurface()->lockBmp();
            FilterFill<unsigned char>(0).applyInPlace(pBmp);
            FT_Bitmap bitmap;
            bitmap.rows = m_StringExtents.y;
            bitmap.width = m_StringExtents.x;
            unsigned char * pLines = pBmp->getPixels();
            bitmap.pitch = pBmp->getStride();
            bitmap.buffer = pLines;
            bitmap.num_grays = 256;
            bitmap.pixel_mode = ft_pixel_mode_grays;

            m_PosOffset = IntPoint(0,0);
            if (ink_rect.y < 0) {
                m_PosOffset.y = ink_rect.y;
            }
            if (ink_rect.x < 0) {
                m_PosOffset.x = ink_rect.x;
            }
            pango_ft2_render_layout(&bitmap, m_pLayout, -m_PosOffset.x, -m_PosOffset.y);
            if (m_Alignment == PANGO_ALIGN_CENTER) {
                m_PosOffset.x -= m_StringExtents.x/2;
            } else if (m_Alignment == PANGO_ALIGN_RIGHT) {
                m_PosOffset.x -= m_StringExtents.x;
            }

            getSurface()->unlockBmps();

            getSurface()->bind();
            if (m_LineSpacing == -1) {
                m_LineSpacing = pango_layout_get_spacing(m_pLayout)/PANGO_SCALE;
            }
        }
    }
    if (getState() == NS_CANRENDER) {
        m_bDrawNeeded = false;
        setViewport(-32767, -32767, -32767, -32767);
    }
}

void WordsNode::preRender()
{
    Node::preRender();
    drawString();
}

static ProfilingZone RenderProfilingZone("WordsNode::render");

void WordsNode::render(const DRect& Rect)
{
    ScopeTimer Timer(RenderProfilingZone);
    if (m_sText.length() != 0 && getEffectiveOpacity() > 0.001) {
        if (m_PosOffset != IntPoint(0,0)) {
            getDisplayEngine()->pushTransform(DPoint(m_PosOffset), 0, DPoint(0,0));
        }
        getSurface()->blta8(DPoint(getMediaSize()), getEffectiveOpacity(), m_Color, 
                getBlendMode());
        if (m_PosOffset != IntPoint(0,0)) {
            getDisplayEngine()->popTransform();
        }
    }
}

IntPoint WordsNode::getMediaSize()
{
    drawString();
    return m_StringExtents;
}

const vector<string>& WordsNode::getFontFamilies()
{
    return TextEngine::get(true).getFontFamilies();
}

const vector<string>& WordsNode::getFontVariants(const string& sFontName)
{
    return TextEngine::get(true).getFontVariants(sFontName);
}

string WordsNode::removeExcessSpaces(const string & sText)
{
    string s = sText;
    string::size_type lastPos = s.npos;
    string::size_type pos = s.find_first_of(" \n\r");
    while (pos != s.npos) {
        s[pos] = ' ';
        if (pos == lastPos+1) {
            s.erase(pos, 1);
            pos--;
        }
        lastPos = pos;
        pos = s.find_first_of(" \n\r", pos+1);
    }
    return s;
}
        
PangoRectangle WordsNode::getGlyphRect(int i)
{
    
    if (i >= int(g_utf8_strlen(m_sText.c_str(), -1)) || i < 0) {
        throw(Exception(AVG_ERR_INVALID_ARGS, 
                string("getGlyphRect: Index ") + toString(i) + " out of range."));
    }
    char * pChar = g_utf8_offset_to_pointer(m_sText.c_str(), i);
    int byteOffset = pChar-m_sText.c_str();
    drawString();
    PangoRectangle rect;
    
    if (m_pLayout) {
        pango_layout_index_to_pos(m_pLayout, byteOffset, &rect);
    } else {
        rect.x = 0;
        rect.y = 0;
        rect.width = 0;
        rect.height = 0;
    }
    return rect;
}

void WordsNode::setParsedText(const UTF8String& sText)
{
    m_sText = removeExcessSpaces(sText);
    m_bDrawNeeded = true;

    // This just does a syntax check and throws an exception if appropriate.
    // The results are discarded.
    PangoAttrList * pAttrList = 0;
    char * pText = 0;
    parseString(&pAttrList, &pText);
    pango_attr_list_unref (pAttrList);
    g_free (pText);
    m_bParsedText = true;
}

UTF8String WordsNode::applyBR(const UTF8String& sText)
{
    UTF8String sResult(sText);
    UTF8String sLowerText = tolower(sResult); 
    string::size_type pos=sLowerText.find("<br/>");
    while (pos != string::npos) {
        sResult.replace(pos, 5, "\n");
        sLowerText.replace(pos, 5, "\n");
        if (sLowerText[pos+1] == ' ') {
            sLowerText.erase(pos+1, 1);
            sResult.erase(pos+1, 1);
        }
        pos=sLowerText.find("<br/>");
    }
    return sResult;
}

}

