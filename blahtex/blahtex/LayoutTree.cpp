// File "LayoutTree.cpp"
// 
// blahtex (version 0.3.2): a LaTeX to MathML converter designed with MediaWiki in mind
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
namespace LayoutTree
{

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

int GetMathmlScriptLevel(Style style)
{
    switch (style)
    {
        case cStyleDisplay:
        case cStyleText:           return 0;
        case cStyleScript:         return 1;
        case cStyleScriptScript:   return 2;
    }
    throw logic_error("Unexpected style value in GetMathmlScriptLevel");
}

auto_ptr<XmlNode> InsertMstyle(auto_ptr<XmlNode> node, Style sourceStyle, Style targetStyle)
{
    if (sourceStyle == targetStyle)
        return node;
    
    auto_ptr<XmlNode> style(new XmlNode(XmlNode::cTag, L"mstyle"));
    style->mChildren.push_back(node.release());

    if (sourceStyle == cStyleDisplay)
        style->mAttributes[L"displaystyle"] = L"false";
    else if (targetStyle == cStyleDisplay)
        style->mAttributes[L"displaystyle"] = L"true";

    int targetSize = GetMathmlScriptLevel(targetStyle);
        
    if (targetSize != GetMathmlScriptLevel(sourceStyle))
    {
        wostringstream os;
        os << targetSize;
        style->mAttributes[L"scriptlevel"] = os.str();
    }
    
    return style;
}

// This function obtains the nucleus of a MathML expression. (See "embellished operators" in the MathML spec.)
// This is used to find any <mo> node which should have its "lspace" and/or "rspace" attributes set.
XmlNode* GetNucleus(XmlNode* node)
{
    if (!node || node->mType == XmlNode::cString)
        return node;
    return (node->mText == L"msup" || node->mText == L"msub" || node->mText == L"msubsup" ||
            node->mText == L"mover" || node->mText == L"munder" || node->mText == L"munderover")
        ? GetNucleus(node->mChildren.front()) : node;
}

auto_ptr<XmlNode> Row::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    // FIX: what will happen if the TOPmost node is not a Row? Maybe should just force a Row in there for safety.

    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mrow"));

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
            currentNode = (*child)->BuildMathmlTree(options, mStyle);

        bool merged = false;
        
        // FIX: it's possible for a nasty user to generate quadratic length input by
        // inputting a table with one long row and lots of empty rows. This sucks. Not sure
        // what to do. Maybe put a hard-coded limit.
        
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
                currentNode->mAttributes[L"mathvariant"] == L"normal")
            {
                previousNode->mChildren.front()->mText += currentNode->mChildren.front()->mText;
                merged = true;
            }
        }

        // FIX: what to do about negative spaces?
        
        // Now decide about spacing. We use <mspace>, unless we have available <mo> nodes on either side,
        // in which case we use "lspace" and/or "rspace" attributes.
        if (!merged)
        {
            XmlNode*  currentNucleus = GetNucleus(currentNode.get());
            XmlNode* previousNucleus = GetNucleus(previousNode);

            bool isPreviousMo = (previousNucleus && previousNucleus->mText == L"mo");
            bool isCurrentMo  = ( currentNucleus &&  currentNucleus->mText == L"mo");

            // We assume that MathML renderers like to put space between <mo> nodes and other nodes,
            // but don't like to put space between non-<mo> nodes.
            
            // FIX: this is not quite right yet... e.g. MathML renderers probably don't put space around "(" etc.
            // Need to look in the operator dictionary...        
            
            bool doSpace = false;

            if (options.mSpacingExplicitness == 0 || isUserRequested)
                doSpace = true;
            else if (options.mSpacingExplicitness == 1)
                doSpace = (isPreviousMo || isCurrentMo) ? (spaceWidth == 0) : (spaceWidth != 0);

            if (doSpace)
            {
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
                    if (spaceWidth != 0)
                    {
                        XmlNode* spaceNode = new XmlNode(XmlNode::cTag, L"mspace");
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
    
    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> Space::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    // FIX: this seems to happen on input like "x^{\,}", putting "\," in a matrix entry etc.
    throw logic_error("Unexpectedly arrived in Space::BuildMathmlTree");
}

auto_ptr<XmlNode> SymbolPlain::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag,
        (mText.size() == 1 && mText[0] >= L'0' && mText[0] <= '9') ? L"mn" : L"mi"));
        // FIX: what about merging commas, decimal points into <mn> nodes? Might need to special-case it.

    if (options.mFancyFontSubstitution && mText.size() == 1)
    {
        wchar_t replacement = 0;
        wchar_t baseUppercase = 0, baseLowercase = 0;
        
        switch (mFont)
        {
            case cMathmlFontBoldScript:     baseUppercase = L'\U0001D4D0'; break;
            case cMathmlFontScript:         baseUppercase = L'\U0001D49C'; break;
            case cMathmlFontFraktur:        baseUppercase = L'\U0001D504';
                                            baseLowercase = L'\U0001D51E'; break;
            case cMathmlFontBoldFraktur:    baseUppercase = L'\U0001D56C';
                                            baseLowercase = L'\U0001D586'; break;
            case cMathmlFontDoubleStruck:   baseUppercase = L'\U0001D538'; break;
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
            return InsertMstyle(node, currentStyle, mStyle);
        }
    }

    node->mChildren.push_back(new XmlNode(XmlNode::cString, mText));
    
    // We put in the font explicitly here. This makes it easier (in Row::BuildMathmlTree) to work out
    // which adjacent nodes to merge. In most cases this "mathvariant" attribute will subsequently get
    // removed. e.g. it might start as "<mi mathvariant="italic">x</mi>", but then the mathvariant can
    // get removed because it is the MathML default.
    // FIX: move this comment to Row::BuildMathmlTree.
    node->mAttributes[L"mathvariant"] = gMathmlFontStrings[mFont];

    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> SymbolOperator::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    static wstring stretchyArray[] =
    {
        L"(",
        L")",
        L"[",
        L"]",
        L"{",
        L"}",
        L"|",
        L"/",
        L"&Backslash;",
        L"&lang;",
        L"&rang;",
        L"&lceil;",
        L"&rceil;",
        L"&lfloor;",
        L"&rfloor;",
        // FIX: look these up in MathML documentation to see if they are stretchy:...
        L"&sum;",
        L"&prod;",
        L"&int;",
        L"&Int;",
        L"&tint;",
        L"&qint;",
        L"&oint;",
        L"&bigcap;",
        L"&bigodot;",
        L"&bigotimes;",
        L"&coprod;",
        L"&bigsqcup;",
        L"&bigoplus;",
        L"&bigvee;",
        L"&biguplus;",
        L"&Wedge;"
    };
    static wishful_hash_set<wstring> stretchyTable(stretchyArray, END_ARRAY(stretchyArray));
    
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mo"));
    node->mChildren.push_back(new XmlNode(XmlNode::cString, mText));

    if (mLimits == cLimitsLimits)
        node->mAttributes[L"movablelimits"] = L"false";

    if (mFont != cMathmlFontNormal)
        node->mAttributes[L"mathvariant"] = gMathmlFontStrings[mFont];
    
    if (mIsStretchy)
    {
        node->mAttributes[L"stretchy"] = L"true";
        if (!mSize.empty())
            node->mAttributes[L"minsize"] = node->mAttributes[L"maxsize"] = mSize;
    }
    else if (stretchyTable.count(mText))
        node->mAttributes[L"stretchy"] = L"false";
    
    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> SymbolText::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mtext"));
    node->mAttributes[L"mathvariant"] = gMathmlFontStrings[mFont];
    node->mChildren.push_back(new XmlNode(XmlNode::cString, mText));
    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> Scripts::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    Style smallerStyle = mStyle;
    switch (smallerStyle)
    {
        case cStyleDisplay:
        case cStyleText:          smallerStyle = cStyleScript; break;
        case cStyleScript:
        case cStyleScriptScript:  smallerStyle = cStyleScriptScript; break;
    }

    auto_ptr<XmlNode> base;
    if (mBase.get())
        base = mBase->BuildMathmlTree(options, mStyle);
    else
        // An empty base gets represented by "<mrow/>"
        base.reset(new XmlNode(XmlNode::cTag, L"mrow"));
    
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L""));
    node->mChildren.push_back(base.release());
    
    if (mUpper.get())
    {
        if (mLower.get())
        {
            node->mText = (mPlacement == cPlacementSideset) ? L"msubsup" : L"munderover";
            node->mChildren.push_back(mLower->BuildMathmlTree(options, smallerStyle).release());
            node->mChildren.push_back(mUpper->BuildMathmlTree(options, smallerStyle).release());
        }
        else
        {
            node->mText = (mPlacement == cPlacementSideset) ? L"msup" : L"mover";
            node->mChildren.push_back(mUpper->BuildMathmlTree(options, smallerStyle).release());
        }
    }
    else
    {
        node->mText = (mPlacement == cPlacementSideset) ? L"msub" : L"munder";
        node->mChildren.push_back(mLower->BuildMathmlTree(options, smallerStyle).release());
    }
   
    if (mPlacement == cPlacementAccent)
    {
        if (mUpper.get())
            node->mAttributes[L"accentover"] = L"true";
        if (mLower.get())
            node->mAttributes[L"accentunder"] = L"true";
        
        // FIX: accents are still very not right... all kind of extra mstyle tags get inserted.
        // FIX: also, need to completely rewrite this stuff so that the buildmathmltree functions
        // get passed a current "scriptLevel" and "displayStyle" setting, not a "currentStyle".
    }

    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> Fraction::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    Style smallerStyle = mStyle;
    switch (smallerStyle)
    {
        case cStyleDisplay:    smallerStyle = cStyleText; break;
        case cStyleText:       smallerStyle = cStyleScript; break;
        case cStyleScript:     smallerStyle = cStyleScriptScript; break;
    }

    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mfrac"));
    node->mChildren.push_back(mNumerator  ->BuildMathmlTree(options, smallerStyle).release());
    node->mChildren.push_back(mDenominator->BuildMathmlTree(options, smallerStyle).release());
    if (!mIsLineVisible)
        node->mAttributes[L"linethickness"] = L"0";

    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> Fenced::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    auto_ptr<XmlNode> inside = mChild->BuildMathmlTree(options, mStyle);

    if (mLeftDelimiter.empty() && mRightDelimiter.empty())
        return inside;

    if (inside->mText != L"mrow")
    {
        auto_ptr<XmlNode> temp(new XmlNode(XmlNode::cTag, L"mrow"));
        temp->mChildren.push_back(inside.release());
        inside = temp;
    }

    auto_ptr<XmlNode> output(new XmlNode(XmlNode::cTag, L"mrow"));

    if (!mLeftDelimiter.empty())
    {
        auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mo"));
        node->mChildren.push_back(new XmlNode(XmlNode::cString, mLeftDelimiter));
        node->mAttributes[L"stretchy"] = L"true";
        output->mChildren.push_back(node.release());
    }
    
    output->mChildren.push_back(inside.release());

    if (!mRightDelimiter.empty())
    {
        auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mo"));
        node->mChildren.push_back(new XmlNode(XmlNode::cString, mRightDelimiter));
        node->mAttributes[L"stretchy"] = L"true";
        output->mChildren.push_back(node.release());
    }

    return InsertMstyle(output, currentStyle, mStyle);
}

auto_ptr<XmlNode> Sqrt::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"msqrt"));
    node->mChildren.push_back(mChild->BuildMathmlTree(options, mStyle).release());
    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> Root::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mroot"));
    // FIX: check these are around the right away
    node->mChildren.push_back(mInside ->BuildMathmlTree(options, mStyle).release());
    node->mChildren.push_back(mOutside->BuildMathmlTree(options, cStyleScriptScript).release());
    return InsertMstyle(node, currentStyle, mStyle);
}

auto_ptr<XmlNode> Table::BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const
{
    // FIX: remember can kill <mrow> tags inside <mtd> tags i.e. make them implied....
    // and NOT JUST HERE... this happens for many types of tags.

    auto_ptr<XmlNode> node(new XmlNode(XmlNode::cTag, L"mtable"));
    
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
        int count = 0;
        for (vector<Node*>::const_iterator inEntry = inRow->begin(); inEntry != inRow->end(); inEntry++, count++)
        {
            auto_ptr<XmlNode> outEntry(new XmlNode(XmlNode::cTag, L"mtd"));
            outEntry->mChildren.push_back((*inEntry)->BuildMathmlTree(options, mStyle).release());
            outRow->mChildren.push_back(outEntry.release());
        }
        for (; count < tableWidth; count++)
            outRow->mChildren.push_back(new XmlNode(XmlNode::cTag, L"mtd"));

        node->mChildren.push_back(outRow.release());
    }

    if (mStyle == cStyleDisplay)
        node->mAttributes[L"displaystyle"] = L"true";
    
    int size = GetMathmlScriptLevel(mStyle);
    if (size != GetMathmlScriptLevel(currentStyle))
    {
        auto_ptr<XmlNode> style(new XmlNode(XmlNode::cTag, L"mstyle"));
        style->mChildren.push_back(node.release());

        wostringstream os;
        os << size;
        style->mAttributes[L"scriptlevel"] = os.str();
        
        return style;
    }
    
    return node;
}

// ===========================================================================================================
// Now all the LayoutTree debugging code

// FIX: make this stuff entity encode for my sanity

wstring gFlavourStrings[] =
{
    L"Ord",
    L"Op",
    L"Bin",
    L"Rel",
    L"Open",
    L"Close",
    L"Punct",
    L"Inner"
};

// This function generates the indents used by various debugging Print functions.
wstring indent(int depth)
{
    return wstring(2 * depth, L' ');
}

void Row::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Row" << endl;
    for (list<Node*>::const_iterator ptr = mChildren.begin(); ptr != mChildren.end(); ptr++)
        (*ptr)->Print(os, depth+1);
}

void SymbolPlain::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"SymbolPlain \"" << mText << L"\" (font = " << gMathmlFontStrings[mFont] << L", flavour = " << gFlavourStrings[mFlavour] << L")" << endl;
}

void SymbolText::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"SymbolText \"" << mText << L"\" (font = " << gMathmlFontStrings[mFont] << L", flavour = " << gFlavourStrings[mFlavour] << L")" << endl;
}

void SymbolOperator::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"SymbolOperator \"" << mText << L"\" (font = " << gMathmlFontStrings[mFont]
        << L", stretchy = " << (mIsStretchy ? L"true" : L"false");
    if (!mSize.empty())
        os << L", size = " << mSize;
    os << L", flavour = " << gFlavourStrings[mFlavour] << L")" << endl;
}

void Space::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Space (width = " << mWidth << L", user requested = " <<
        (mIsUserRequested ? L"true" : L"false") << L")" << endl;
}

// FIX: should print out flavour for all of these too....:

void Scripts::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Scripts ";
    switch (mPlacement)
    {
        case cPlacementSideset:     os << L"(sideset)"; break;
        case cPlacementUnderOver:   os << L"(under/over)"; break;
        case cPlacementAccent:      os << L"(accented)"; break;
    }
    os << endl;

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
    os << indent(depth) << L"Fraction (visible line = " << (mIsLineVisible ? L"true" : L"false") << L")" << endl;
    mNumerator->Print(os, depth+1);
    mDenominator->Print(os, depth+1);
}

void Fenced::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Fenced (left = \"" << mLeftDelimiter << L"\", right = \"" << mRightDelimiter << L"\")" << endl;
    mChild->Print(os, depth+1);
}

void Sqrt::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Sqrt" << endl;
    mChild->Print(os, depth+1);
}

void Root::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Root" << endl;
    mInside->Print(os, depth+1);
    mOutside->Print(os, depth+1);
}

void Table::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"Table" << endl;
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
