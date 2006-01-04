// File "LayoutTree.cpp"
// 
// blahtex (version 0.3.5): a TeX to MathML converter designed with MediaWiki in mind
// Copyright (C) 2005, David Harvey
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "blahtex.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace blahtex
{

static pair<wstring, wstring> gDelimiterArray[] =
{
    make_pair(L".",             L""),
    make_pair(L"[",             L"["),
    make_pair(L"]",             L"]"),
    make_pair(L"\\lbrack",      L"["),
    make_pair(L"\\rbrack",      L"]"),
    make_pair(L"(",             L"("),
    make_pair(L")",             L")"),
    make_pair(L"<",             L"\U00002329"),
    make_pair(L">",             L"\U0000232A"),
    make_pair(L"\\langle",      L"\U00002329"),
    make_pair(L"\\rangle",      L"\U0000232A"),
    make_pair(L"/",             L"/"),
    make_pair(L"\\backslash",   L"\U00002216"),
    make_pair(L"\\{",           L"{"),
    make_pair(L"\\}",           L"}"),
    make_pair(L"\\lbrace",      L"{"),
    make_pair(L"\\rbrace",      L"}"),
    make_pair(L"|",             L"|"),
    make_pair(L"\\vert",        L"|"),
    make_pair(L"\\lvert",       L"|"),
    make_pair(L"\\rvert",       L"|"),
    make_pair(L"\\Vert",        L"\U00002225"),
    make_pair(L"\\lVert",       L"\U00002225"),
    make_pair(L"\\rVert",       L"\U00002225"),
    make_pair(L"\\uparrow",     L"\U00002191"),
    make_pair(L"\\downarrow",   L"\U00002193"),
    make_pair(L"\\updownarrow", L"\U00002195"),
    make_pair(L"\\Uparrow",     L"\U000021D1"),
    make_pair(L"\\Downarrow",   L"\U000021D3"),
    make_pair(L"\\Updownarrow", L"\U000021D5"),
    make_pair(L"\\lfloor",      L"\U0000230A"),
    make_pair(L"\\rfloor",      L"\U0000230B"),
    make_pair(L"\\lceil",       L"\U00002308"),
    make_pair(L"\\rceil",       L"\U00002309")
};

wishful_hash_map<wstring, wstring> gDelimiterTable(gDelimiterArray, END_ARRAY(gDelimiterArray));

wstring gMathmlFontStrings[] =
{
    L"normal",
    L"bold",
    L"italic",
    L"bold-italic",
    L"double-struck",
    L"bold-fraktur",
    L"script",
    L"bold-script",
    L"fraktur",
    L"sans-serif",
    L"bold-sans-serif",
    L"sans-serif-italic",
    L"sans-serif-bold-italic",
    L"monospace"
};

namespace LayoutTree
{

Row::~Row()
{
    for (list<Node*>::iterator p = mChildren.begin(); p != mChildren.end(); p++)
        delete *p;
}

Table::~Table()
{
    for (vector<vector<Node*> >::iterator p = mRows.begin(); p != mRows.end(); p++)
        for (vector<Node*>::iterator q = p->begin(); q != p->end(); q++)
            delete *q;
}

MathmlEnvironment::MathmlEnvironment(Style style)
{
    mDisplayStyle = (style == cStyleDisplay);

    switch (style)
    {
        case cStyleDisplay:
        case cStyleText:             mScriptLevel = 0; break;
        case cStyleScript:           mScriptLevel = 1; break;
        case cStyleScriptScript:     mScriptLevel = 2; break;
        default:
            throw logic_error("Unexpected style value in MathmlEnvironment::MathmlEnvironment");
    }
}

// FIX: doc this. Remember to mention it only works for nodes which can have either zero or one child.
void CollapseMrow(XmlNode& node)
{
    if (!node.mChildren.empty() && node.mChildren.front()->mText == L"mrow")
    {
        list<XmlNode*> temp;
        temp.swap(node.mChildren.front()->mChildren);
        delete node.mChildren.front();
        node.mChildren.clear();
        node.mChildren.swap(temp);
    }
}

void IncrementNodeCount(unsigned& nodeCount)
{
    if (++nodeCount >= cMaxMathmlNodeCount)
        throw Exception(L"TooManyMathmlNodes");
}

auto_ptr<XmlNode> InsertMstyle(auto_ptr<XmlNode> node, MathmlEnvironment sourceEnvironment, MathmlEnvironment targetEnvironment)
{
    if (sourceEnvironment.mDisplayStyle == targetEnvironment.mDisplayStyle &&
        sourceEnvironment.mScriptLevel == targetEnvironment.mScriptLevel)
        return node;
    
    auto_ptr<XmlNode> newNode(new XmlNode(XmlNode::cTag, L"mstyle"));
    newNode->mChildren.push_back(node.release());

    if (sourceEnvironment.mDisplayStyle != targetEnvironment.mDisplayStyle)
        newNode->mAttributes[L"displaystyle"] = (targetEnvironment.mDisplayStyle) ? L"true" : L"false";

    if (sourceEnvironment.mScriptLevel != targetEnvironment.mScriptLevel)
    {
        wostringstream os;
        os << targetEnvironment.mScriptLevel;
        newNode->mAttributes[L"scriptlevel"] = os.str();
    }
    
    CollapseMrow(*newNode);
    return newNode;
}

// This function obtains the core of a MathML expression. (See "embellished operators" in the MathML spec.)
// This is used to find any <mo> node which should have its "lspace" and/or "rspace" attributes set.
XmlNode* GetCore(XmlNode* node)
{
    // FIX: this code is not quite right. It doesn't handle situations where <mrow> or <mstyle>
    // or something similar contain a single node which is an embellished operator.
    
    if (!node || node->mType == XmlNode::cString)
        return node;

    return (node->mText == L"msup"  || node->mText == L"msub"   || node->mText == L"msubsup"    ||
            node->mText == L"mover" || node->mText == L"munder" || node->mText == L"munderover" ||
            node->mText == L"mfrac")
        ? GetCore(node->mChildren.front()) : node;
}

auto_ptr<XmlNode> Row::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mrow"));
    IncrementNodeCount(nodeCount);

    if (mChildren.empty())
        return node;

    for (list<Node*>::const_iterator child = mChildren.begin(); true; child++)
    {
        XmlNode* previousNode = node->mChildren.empty() ? NULL : node->mChildren.back();

        int spaceWidth = 0;
        bool isUserRequested = false;

        for (; child != mChildren.end(); child++)
        {
            const Space* spaceNode = dynamic_cast<const Space*>(*child);
            if (!spaceNode)
                break;
            spaceWidth += spaceNode->mWidth;
            if (spaceNode->mIsUserRequested)
                isUserRequested = true;
        }

        auto_ptr<XmlNode> currentNode;
        if (child != mChildren.end())
            currentNode = (*child)->BuildMathmlTree(options, MathmlEnvironment(mStyle), nodeCount);

        bool merged = false;
        
        // If there is no space, we need to consider all the possible ways of merging adjacent nodes.

        // FIX: one day, when I could be bothered, need to also consider merging into bases of sub/superscripts....
        // that's going to be quite a painful piece of code.
        if (spaceWidth == 0 && previousNode && currentNode.get())
        {
            if ((previousNode->mText == L"mn" || previousNode->mText == L"mtext") &&
                currentNode->mText == previousNode->mText &&
                previousNode->mAttributes[L"mathvariant"] == currentNode->mAttributes[L"mathvariant"])
            {
                previousNode->mChildren.front()->mText += currentNode->mChildren.front()->mText;
                merged = true;
            }
            else if (previousNode->mText == L"mi" && currentNode->mText == L"mi" &&
                previousNode->mAttributes[L"mathvariant"] == L"normal" &&
                currentNode ->mAttributes[L"mathvariant"] == L"normal")
            {
                previousNode->mChildren.front()->mText += currentNode->mChildren.front()->mText;
                merged = true;
            }
        }

        // FIX: what to do about negative spaces?
        
        if (!merged)
        {
            // Now decide about spacing.
            
            XmlNode*  currentNucleus = GetCore(currentNode.get());
            XmlNode* previousNucleus = GetCore(previousNode);

            bool isPreviousMo = false, isCurrentMo = false;
            bool isPreviousMi = false, isCurrentMi = false;
            
            if (previousNucleus)
            {
                if (previousNucleus->mText == L"mo")
                    isPreviousMo = true;
                else if (previousNucleus->mText == L"mi")
                    isPreviousMi = true;
            }

            if (currentNucleus)
            {
                if (currentNucleus->mText == L"mo")
                    isCurrentMo = true;
                else if (currentNucleus->mText == L"mi")
                    isCurrentMi = true;
            }

            bool doSpace = false;

            if (options.mSpacingControl == cSpacingControlStrict || isUserRequested)
                doSpace = true;
            else if (options.mSpacingControl == cSpacingControlModerate)
            {
                // The user has asked for "moderate" spacing mode, so we need to give the MathML renderer
                // a helping hand with spacing decisions, without being *too* pushy.

                // Special-casing for neighbouring <mi> nodes, since Firefox sometimes likes to put
                // extra space between them:
                // FIX: this could be more discriminating with respect to fontstyle attributes...
                if (isPreviousMi && isCurrentMi)
                    doSpace = true;
                
                else if (!isPreviousMo && !isCurrentMo)
                    doSpace = (spaceWidth != 0);

                // Special-casing for the "&times;" operator (since Firefox doesn't know that it's a
                // binary operator):
                else if (
                    (isPreviousMo && previousNucleus->mChildren.front()->mText == L"\U000000D7") ||
                    (isCurrentMo  &&  currentNucleus->mChildren.front()->mText == L"\U000000D7"))
                {
                    doSpace = true;
                }
                
                else if (spaceWidth == 0)
                {
                    // Special-casing for zero space after ",", e.g. in a situation like "65{,}536"
                    if (isPreviousMo && previousNucleus->mChildren.front()->mText == L",")
                        doSpace = true;
                    // Special-casing for neighbouring &prime; operators
                    else if (
                        (isPreviousMo && previousNucleus->mChildren.front()->mText == L"\U00002032") &&
                        (isCurrentMo  &&  currentNucleus->mChildren.front()->mText == L"\U00002032"))
                    {
                        doSpace = false;
                    }
                    else
                    {
                        static wstring fenceArray[] = {L"(", L")", L"[", L"]", L"{", L"}"};
                        static wishful_hash_set<wstring> fenceTable(fenceArray, END_ARRAY(fenceArray));
                        
                        if (isPreviousMo && isCurrentMo)
                        {
                            if (!fenceTable.count(previousNucleus->mChildren.front()->mText) &&
                                !fenceTable.count( currentNucleus->mChildren.front()->mText))
                            {
                                // This handles the situation where there are two operators, neither of which
                                // are fences, which have no space between them: e.g. "a := b".
                                doSpace = true;
                            }
                        }
                    }
                }
            }

            if (doSpace)
            {
                // We use <mspace>, unless we have an <mo> node on either side (or both sides),
                // in which case we use "lspace" and/or "rspace" attributes.
            
                wstring widthAsString;
                if (spaceWidth == 0)
                    widthAsString = L"0";
                else
                {
                    wostringstream wos;
                    wos << fixed << setprecision(3) << (spaceWidth / 18.0) << L"em";
                    widthAsString = wos.str();
                }

                if (isPreviousMo)
                {
                    previousNucleus->mAttributes[L"rspace"] = widthAsString;
                    if (isCurrentMo)
                        currentNucleus->mAttributes[L"lspace"] = L"0";
                }
                else if (isCurrentMo)
                    currentNucleus->mAttributes[L"lspace"] = widthAsString;
                else
                {
                    if (spaceWidth != 0 || (isPreviousMi && isCurrentMi))
                    {
                        XmlNode* spaceNode = new XmlNode(XmlNode::cTag, L"mspace");
                        IncrementNodeCount(nodeCount);
                        spaceNode->mAttributes[L"width"] = widthAsString;
                        node->mChildren.push_back(spaceNode);
                    }
                }
            }

            if (currentNode.get())
                node->mChildren.push_back(currentNode.release());
        }

        if (child == mChildren.end())
            break;
    }

    // If only one child left, return it without enclosing <mrow> tags.
    if (!node->mChildren.empty() && (++node->mChildren.begin() == node->mChildren.end()))
    {
        auto_ptr<XmlNode> singleton(node->mChildren.back());
        node->mChildren.pop_back();     // required to relinquish ownership
        node = singleton;
    }
    
    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Space::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    if (!mIsUserRequested)
        throw logic_error("Unexpected lonely automatic space in Space::BuildMathmlTree");
    
    // FIX: what happens with negative space?
    
    wostringstream wos;
    wos << fixed << setprecision(3) << (mWidth / 18.0) << L"em";

    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mspace"));
    IncrementNodeCount(nodeCount);
    node->mAttributes[L"width"] = wos.str();

    return node;
}

auto_ptr<XmlNode> SymbolPlain::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag,
        (mText.size() == 1 && mText[0] >= L'0' && mText[0] <= '9') ? L"mn" : L"mi"));
        // FIX: what about merging commas, decimal points into <mn> nodes? Might need to special-case it.
    IncrementNodeCount(nodeCount);

    if (options.mMathmlVersion1Fonts && mText.size() == 1)
    {
        wchar_t replacement = 0;
        wchar_t baseUppercase = 0, baseLowercase = 0;

        switch (mFont)
        {
            case cMathmlFontBoldScript:
                if (options.mAllowPlane1)
                {
                    baseUppercase = L'\U0001D4D0';
                    break;
                }
                else
                {
                    node->mAttributes[L"mathvariant"] = L"bold";
                    baseUppercase = L'\U0001D49C';
                    break;
                }
                
            case cMathmlFontScript:
                baseUppercase = L'\U0001D49C';
                break;

            case cMathmlFontBoldFraktur:   
                if (options.mAllowPlane1)
                {
                    baseUppercase = L'\U0001D56C';
                    baseLowercase = L'\U0001D586'; 
                    break;
                }
                else
                {
                    node->mAttributes[L"mathvariant"] = L"bold";
                    baseUppercase = L'\U0001D504';
                    baseLowercase = L'\U0001D51E';
                    break;
                }
                
            case cMathmlFontFraktur:
                baseUppercase = L'\U0001D504';
                baseLowercase = L'\U0001D51E';
                break;

            case cMathmlFontDoubleStruck:
                baseUppercase = L'\U0001D538';
                break;
        }
        
        if (baseUppercase && mText[0] >= 'A' && mText[0] <= 'Z')
            replacement = baseUppercase + (mText[0] - 'A');
        if (baseLowercase && mText[0] >= 'a' && mText[0] <= 'z')
            replacement = baseLowercase + (mText[0] - 'a');
        
        switch (replacement)
        {
            case L'\U0001D49D':   replacement = L'\U0000212C'; break;       // script B
            case L'\U0001D4A0':   replacement = L'\U00002130'; break;       // script E
            case L'\U0001D4A1':   replacement = L'\U00002131'; break;       // script F
            case L'\U0001D4A3':   replacement = L'\U0000210B'; break;       // script H
            case L'\U0001D4A4':   replacement = L'\U00002110'; break;       // script I
            case L'\U0001D4A7':   replacement = L'\U00002112'; break;       // script L
            case L'\U0001D4A8':   replacement = L'\U00002133'; break;       // script M
            case L'\U0001D4AD':   replacement = L'\U0000211B'; break;       // script R

            case L'\U0001D53A':   replacement = L'\U00002102'; break;       // double struck C
            case L'\U0001D53F':   replacement = L'\U0000210D'; break;       // double struck H
            case L'\U0001D545':   replacement = L'\U00002115'; break;       // double struck N
            case L'\U0001D547':   replacement = L'\U00002119'; break;       // double struck P
            case L'\U0001D548':   replacement = L'\U0000211A'; break;       // double struck Q
            case L'\U0001D549':   replacement = L'\U0000211D'; break;       // double struck R
            case L'\U0001D551':   replacement = L'\U00002124'; break;       // double struck Z

            case L'\U0001D506':   replacement = L'\U0000212D'; break;       // fraktur C
            case L'\U0001D50B':   replacement = L'\U0000210C'; break;       // fraktur H
            case L'\U0001D50C':   replacement = L'\U00002111'; break;       // fraktur I
            case L'\U0001D515':   replacement = L'\U0000211C'; break;       // fraktur R
            case L'\U0001D51D':   replacement = L'\U00002128'; break;       // fraktur Z
        }
        
        if (replacement)
        {
            node->mChildren.push_back(new XmlNode(XmlNode::cString, wstring(1, replacement)));
            IncrementNodeCount(nodeCount);
            return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
        }
    }

    node->mChildren.push_back(new XmlNode(XmlNode::cString, mText));
    IncrementNodeCount(nodeCount);
    node->mAttributes[L"mathvariant"] = gMathmlFontStrings[mFont];

    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> SymbolOperator::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    static wchar_t stretchyByDefaultArray[] =
    {
        L'(',
        L')',
        L'[',
        L']',
        L'{',
        L'}',
        L'|',
        L'/',
        L'\U000002DC',      // DiacriticalTilde
        L'\U000002C7',      // Hacek
        L'\U000002D8',      // Breve
        L'\U00002216',      // Backslash
        L'\U00002329',      // LeftAngleBracket
        L'\U0000232A',      // RightAngleBracket
        L'\U00002308',      // LeftCeiling
        L'\U00002309',      // RightCeiling
        L'\U0000230A',      // LeftFloor
        L'\U0000230B',      // RightFloor
        L'\U00002211',      // Sum
        L'\U0000220F',      // Product
        L'\U0000222B',      // Integral
        L'\U0000222C',      // Int
        L'\U0000222D',      // iiint
        L'\U00002A0C',      // iiiint
        L'\U0000222E',      // ContourIntegral
        L'\U000022C2',      // Intersection
        L'\U00002A00',      // bigodot
        L'\U00002A02',      // bigotimes
        L'\U00002210',      // Coproduct
        L'\U00002A06',      // bigsqcup
        L'\U00002A01',      // bigoplus
        L'\U000022C1',      // Vee
        L'\U00002A04',      // biguplus
        L'\U000022C0'       // Wedge
    };
    static wishful_hash_set<wchar_t> stretchyByDefaultTable(stretchyByDefaultArray, END_ARRAY(stretchyByDefaultArray));

    static wchar_t accentByDefaultArray[] =
    {
        L'\U0000FE37',
        L'\U0000FE38'
    };
    static wishful_hash_set<wchar_t> accentByDefaultTable(accentByDefaultArray, END_ARRAY(accentByDefaultArray));
    
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mo"));
    IncrementNodeCount(nodeCount);
    node->mChildren.push_back(new XmlNode(XmlNode::cString, mText));
    IncrementNodeCount(nodeCount);

    if (mFont != cMathmlFontNormal)
        node->mAttributes[L"mathvariant"] = gMathmlFontStrings[mFont];
    
    if (mIsStretchy)
    {
        node->mAttributes[L"stretchy"] = L"true";
        if (!mSize.empty())
            node->mAttributes[L"minsize"] = node->mAttributes[L"maxsize"] = mSize;
    }
    else if (mText.size() == 1 && stretchyByDefaultTable.count(mText[0]))
        node->mAttributes[L"stretchy"] = L"false";
    
    if (mIsAccent)
    {
        node->mAttributes[L"accent"] = L"true";
        return node;
    }
    else if (mText.size() == 1 && accentByDefaultTable.count(mText[0]))
        node->mAttributes[L"accent"] = L"false";
    
    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> SymbolText::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mtext"));
    IncrementNodeCount(nodeCount);
    node->mAttributes[L"mathvariant"] = gMathmlFontStrings[mFont];
    node->mChildren.push_back(new XmlNode(XmlNode::cString, mText));
    IncrementNodeCount(nodeCount);
    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Scripts::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    MathmlEnvironment scriptEnvironment = MathmlEnvironment(mStyle);
    scriptEnvironment.mDisplayStyle = false;
    scriptEnvironment.mScriptLevel++;

    auto_ptr<XmlNode> base;
    if (mBase.get())
        base = mBase->BuildMathmlTree(options, MathmlEnvironment(mStyle), nodeCount);
    else
    {
        // An empty base gets represented by "<mrow/>"
        base.reset(new XmlNode(XmlNode::cTag, L"mrow"));
        IncrementNodeCount(nodeCount);
    }

    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L""));
    IncrementNodeCount(nodeCount);
    node->mChildren.push_back(base.release());

    if (mUpper.get())
    {
        if (mLower.get())
        {
            node->mText = mIsSideset ? L"msubsup" : L"munderover";
            node->mChildren.push_back(mLower->BuildMathmlTree(options, scriptEnvironment, nodeCount).release());
            node->mChildren.push_back(mUpper->BuildMathmlTree(options, scriptEnvironment, nodeCount).release());
        }
        else
        {
            node->mText = mIsSideset ? L"msup" : L"mover";
            node->mChildren.push_back(mUpper->BuildMathmlTree(options, scriptEnvironment, nodeCount).release());
        }
    }
    else
    {
        node->mText = mIsSideset ? L"msub" : L"munder";
        node->mChildren.push_back(mLower->BuildMathmlTree(options, scriptEnvironment, nodeCount).release());
    }

    if (!mIsSideset && mStyle != cStyleDisplay)
    {
        // This situation should be quite unusual, since the user would have to force things using
        // "\limits". If there's an operator in the core, we need to set movablelimits just to be safe.

        // FIX: this code might let the user induce quadratic time, with something like this:
        // "\textstyle \mathop{\mathop{\mathop{\mathop{x}\limits^x}\limits^x}\limits^x}\limits^x" etc
        
        // FIX: we could add a table to check whether the operator inside is likely to need movablelimits
        // adjusted because of the operator dictionary.
        
        XmlNode* core = GetCore(node->mChildren.front());
        if (core->mText == L"mo")
            core->mAttributes[L"movablelimits"] = L"false";
    }
       
    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Fraction::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    MathmlEnvironment smallerEnvironment = MathmlEnvironment(mStyle);
    
    if (smallerEnvironment.mDisplayStyle)
        smallerEnvironment.mDisplayStyle = false;
    else
        smallerEnvironment.mScriptLevel++;
    
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mfrac"));
    IncrementNodeCount(nodeCount);
    node->mChildren.push_back(mNumerator  ->BuildMathmlTree(options, smallerEnvironment, nodeCount).release());
    node->mChildren.push_back(mDenominator->BuildMathmlTree(options, smallerEnvironment, nodeCount).release());
    if (!mIsLineVisible)
        node->mAttributes[L"linethickness"] = L"0";

    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Fenced::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> inside = mChild->BuildMathmlTree(options, MathmlEnvironment(mStyle), nodeCount);

    if (mLeftDelimiter.empty() && mRightDelimiter.empty())
        return inside;

    if (inside->mText != L"mrow")
    {
        auto_ptr<XmlNode> temp(new XmlNode(XmlNode::cTag, L"mrow"));
        IncrementNodeCount(nodeCount);
        temp->mChildren.push_back(inside.release());
        inside = temp;
    }

    auto_ptr<XmlNode> output(new XmlNode(XmlNode::cTag, L"mrow"));
    IncrementNodeCount(nodeCount);

    if (!mLeftDelimiter.empty())
    {
        auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mo"));
        IncrementNodeCount(nodeCount);
        node->mChildren.push_back(new XmlNode(XmlNode::cString, mLeftDelimiter));
        IncrementNodeCount(nodeCount);
        node->mAttributes[L"stretchy"] = L"true";
        output->mChildren.push_back(node.release());
    }
    
    output->mChildren.push_back(inside.release());

    if (!mRightDelimiter.empty())
    {
        auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mo"));
        IncrementNodeCount(nodeCount);
        node->mChildren.push_back(new XmlNode(XmlNode::cString, mRightDelimiter));
        IncrementNodeCount(nodeCount);
        node->mAttributes[L"stretchy"] = L"true";
        output->mChildren.push_back(node.release());
    }

    return InsertMstyle(output, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Sqrt::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"msqrt"));
    IncrementNodeCount(nodeCount);
    node->mChildren.push_back(mChild->BuildMathmlTree(options, MathmlEnvironment(mStyle), nodeCount).release());
    CollapseMrow(*node);
    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Root::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mroot"));
    IncrementNodeCount(nodeCount);
    node->mChildren.push_back(mInside ->BuildMathmlTree(options, MathmlEnvironment(mStyle), nodeCount).release());
    node->mChildren.push_back(mOutside->BuildMathmlTree(options, MathmlEnvironment(false, 2), nodeCount).release());
    return InsertMstyle(node, inheritedEnvironment, MathmlEnvironment(mStyle));
}

auto_ptr<XmlNode> Table::BuildMathmlTree(const MathmlOptions& options, MathmlEnvironment inheritedEnvironment, unsigned& nodeCount) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mtable"));
    IncrementNodeCount(nodeCount);
    
    int tableWidth = 0;
    for (vector<vector<Node*> >::const_iterator row = mRows.begin(); row != mRows.end(); row++)
    {
        if (tableWidth < row->size())
            tableWidth = row->size();
    }

    if (mAlign == cAlignLeft)
        node->mAttributes[L"columnalign"] = L"left";
    else if (mAlign == cAlignRightLeft)
    {
        wstring alignString = L"right";
        for (int i = 1; i < tableWidth; i++)
            alignString += (i % 2) ? L" left" : L" right";
        node->mAttributes[L"columnalign"] = alignString;
    }
    
    for (vector<vector<Node*> >::const_iterator inRow = mRows.begin(); inRow != mRows.end(); inRow++)
    {
        auto_ptr<XmlNode> outRow(new XmlNode(XmlNode::cTag, L"mtr"));
        IncrementNodeCount(nodeCount);
        int count = 0;
        for (vector<Node*>::const_iterator inEntry = inRow->begin(); inEntry != inRow->end(); inEntry++, count++)
        {
            auto_ptr<XmlNode> outEntry(new XmlNode(XmlNode::cTag, L"mtd"));
            IncrementNodeCount(nodeCount);
            outEntry->mChildren.push_back((*inEntry)->BuildMathmlTree(options, MathmlEnvironment(mStyle), nodeCount).release());
            // FIX: this next line should be uncommented, except that Firefox has a bug (#236963) where it
            // doesn't correctly put an "inferred mrow" inside a <mtd> block, so stuff inside the <mtd>
            // doesn't stretch properly unless we put in an mrow.
//            CollapseMrow(*outEntry);
            outRow->mChildren.push_back(outEntry.release());
        }
        for (; count < tableWidth; count++)
        {
            outRow->mChildren.push_back(new XmlNode(XmlNode::cTag, L"mtd"));
            IncrementNodeCount(nodeCount);
        }

        node->mChildren.push_back(outRow.release());
    }

    // Here we would prefer to use the InsertMstyle function, but the MathML spec says that the displaystyle
    // attribute needs to be set on the <mtable> element itself, since the default "false" overrides any
    // enclosing <mstyle>. So we only put the scriptlevel in an mstyle (if necessary).
    MathmlEnvironment tableEnvironment(mStyle);
    if (tableEnvironment.mDisplayStyle)
        node->mAttributes[L"displaystyle"] = L"true";
    if (tableEnvironment.mScriptLevel != inheritedEnvironment.mScriptLevel)
    {
        auto_ptr<XmlNode> styleNode(new XmlNode(XmlNode::cTag, L"mstyle"));
        IncrementNodeCount(nodeCount);
        styleNode->mChildren.push_back(node.release());

        wostringstream os;
        os << tableEnvironment.mScriptLevel;
        styleNode->mAttributes[L"scriptlevel"] = os.str();
        
        return styleNode;
    }
    else
        return node;
}

// ===========================================================================================================
// Now all the LayoutTree debugging code

wstring gFlavourStrings[] =
{
    L"ord",
    L"op",
    L"bin",
    L"rel",
    L"open",
    L"close",
    L"punct",
    L"inner"
};

wstring gLimitsStrings[] =
{
    L"displaylimits",
    L"limits",
    L"nolimits"
};

wstring gStyleStrings[] =
{
    L"displaystyle",
    L"textstyle",
    L"scriptstyle",
    L"scriptscriptstyle"
};

wstring gAlignStrings[] =
{
    L"left",
    L"centre",
    L"rightleft"
};

// This function generates the indents used by various debugging Print functions.
wstring indent(int depth)
{
    return wstring(2 * depth, L' ');
}

wstring Node::PrintFields() const
{
    wstring output = gFlavourStrings[mFlavour];
    if (mFlavour == cFlavourOp)
        output += L" " + gLimitsStrings[mLimits];
    output += L" " + gStyleStrings[mStyle];
    return output;
}

void Row::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Row " << PrintFields() << endl;
    for (list<Node*>::const_iterator ptr = mChildren.begin(); ptr != mChildren.end(); ptr++)
        (*ptr)->Print(os, depth+1);
}

void SymbolPlain::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"SymbolPlain \"" << mText << L"\" " << gMathmlFontStrings[mFont] << L" " << PrintFields() << endl;
}

void SymbolText::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"SymbolText \"" << mText << L"\" " << gMathmlFontStrings[mFont] << L" " << PrintFields() << endl;
}

void SymbolOperator::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"SymbolOperator \"" << mText << L"\" " << gMathmlFontStrings[mFont]
        << (mIsStretchy ? L" stretchy" : L" non-stretchy") << (mIsAccent ? L" accent" : L"");
    if (!mSize.empty())
        os << L" size=\"" << mSize << L"\"";
    os << L" " << PrintFields() << endl;
}

void Space::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Space " << mWidth;
    if (mIsUserRequested)
        os << L" (user requested)";
    os << endl;
}

void Scripts::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Scripts " << (mIsSideset ? L"sideset" : L"underover") << L" " << PrintFields() << endl;

    if (mBase.get())
    {
        os << indent(depth+1) << L"base" << endl;
        mBase->Print(os, depth+2);
    }
    if (mUpper.get())
    {
        os << indent(depth+1) << L"upper" << endl;
        mUpper->Print(os, depth+2);
    }
    if (mLower.get())
    {
        os << indent(depth+1) << L"lower" << endl;
        mLower->Print(os, depth+2);
    }
}

void Fraction::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Fraction ";
    if (!mIsLineVisible)
        os << L"(no visible line) ";
    os << PrintFields() << endl;
    mNumerator->Print(os, depth+1);
    mDenominator->Print(os, depth+1);
}

void Fenced::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Fenced \"" << mLeftDelimiter << L"\" \"" << mRightDelimiter << L"\" "
        << PrintFields() << endl;
    mChild->Print(os, depth+1);
}

void Sqrt::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Sqrt " << PrintFields() << endl;
    mChild->Print(os, depth+1);
}

void Root::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Root " << PrintFields() << endl;
    mInside->Print(os, depth+1);
    mOutside->Print(os, depth+1);
}

void Table::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Table " << PrintFields() << L" " << gAlignStrings[mAlign] << endl;
    for (vector<vector<Node*> >::const_iterator row = mRows.begin(); row != mRows.end(); row++)
    {
        os << indent(depth+1) << L"Table row" << endl;
        for (vector<Node*>::const_iterator entry = row->begin(); entry != row->end(); entry++)
            (*entry)->Print(os, depth+2);
    }
}


}
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
