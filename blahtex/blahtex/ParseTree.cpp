// File "ParseTree.cpp"
// 
// blahtex (version 0.3.3): a LaTeX to MathML converter designed with MediaWiki in mind
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
    // FIX: test this one:
    make_pair(L"\\Vert",        L"\U00002225"),
    make_pair(L"\\uparrow",     L"\U00002191"),
    make_pair(L"\\downarrow",   L"\U00002193"),
    // FIX: test this one:
    make_pair(L"\\updownarrow", L"\U00002195"),
    make_pair(L"\\Uparrow",     L"\U000021D1"),
    make_pair(L"\\Downarrow",   L"\U000021D3"),
    // FIX: test this one:
    make_pair(L"\\Updownarrow", L"\U000021D5"),
    make_pair(L"\\lfloor",      L"\U0000230A"),
    make_pair(L"\\rfloor",      L"\U0000230B"),
    make_pair(L"\\lceil",       L"\U00002308"),
    make_pair(L"\\rceil",       L"\U00002309")
// FIX: what about \lvert, \rvert... do these need to be here?
};

wishful_hash_map<wstring, wstring> gDelimiterTable(gDelimiterArray, END_ARRAY(gDelimiterArray));

namespace ParseTree
{

MathList::~MathList()
{
    for (vector<MathNode*>::iterator p = mChildren.begin(); p != mChildren.end(); p++)
        delete *p;
}

MathTableRow::~MathTableRow()
{
    for (vector<MathNode*>::iterator p = mEntries.begin(); p != mEntries.end(); p++)
        delete *p;
}

MathTable::~MathTable()
{
    for (vector<MathTableRow*>::iterator p = mRows.begin(); p != mRows.end(); p++)
        delete *p;
}

TextList::~TextList()
{
    for (vector<TextNode*>::iterator p = mChildren.begin(); p != mChildren.end(); p++)
        delete *p;
}

MathmlFont LatexMathFont::GetMathmlApproximation() const
{
    if (mIsBoldsymbol)
    {
        switch (mFamily)
        {
            case cFamilyRm:    return cMathmlFontBold;
            case cFamilyIt:    return cMathmlFontBoldItalic;
            case cFamilyBf:    return cMathmlFontBold;
            case cFamilyBb:    return cMathmlFontDoubleStruck;
            case cFamilySf:    return cMathmlFontBoldSansSerif;
            case cFamilyCal:   return cMathmlFontBoldScript;
            case cFamilyTt:    return cMathmlFontMonospace;
            case cFamilyFrak:  return cMathmlFontBoldFraktur;
        }
    }
    else
    {
        switch (mFamily)
        {
            case cFamilyRm:    return cMathmlFontNormal;
            case cFamilyIt:    return cMathmlFontItalic;
            case cFamilyBf:    return cMathmlFontBold;
            case cFamilyBb:    return cMathmlFontDoubleStruck;
            case cFamilySf:    return cMathmlFontSansSerif;
            case cFamilyCal:   return cMathmlFontScript;
            case cFamilyTt:    return cMathmlFontMonospace;
            case cFamilyFrak:  return cMathmlFontFraktur;
        }
    }
    
    throw logic_error("Unexpected LatexMathFont data");
}

MathmlFont LatexTextFont::GetMathmlApproximation() const
{
    switch (mFamily)
    {
        case cFamilyRm:
            return mIsBold
                ? (mIsItalic ? cMathmlFontBoldItalic : cMathmlFontBold)
                : (mIsItalic ? cMathmlFontItalic     : cMathmlFontNormal);
            
        case cFamilySf:
            return mIsBold
                ? (mIsItalic ? cMathmlFontSansSerifBoldItalic : cMathmlFontBoldSansSerif)
                : (mIsItalic ? cMathmlFontSansSerifItalic     : cMathmlFontSansSerif);
            
        case cFamilyTt:   return cMathmlFontMonospace;
    }
    
    throw logic_error("Unexpected LatexTextFont data");
}

auto_ptr<LayoutTree::Node> MathList::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    auto_ptr<LayoutTree::Row> output(new LayoutTree::Row(currentStyle));
    list<LayoutTree::Node*>& targetList = output->mChildren;

    for (vector<MathNode*>::const_iterator node = mChildren.begin(); node != mChildren.end(); node++)
        targetList.push_back((*node)->BuildLayoutTree(currentFont, currentStyle).release());

    // First pass: modify atom flavours according to TeX's rules.
    for (list<LayoutTree::Node*>::iterator node = targetList.begin(); node != targetList.end(); node++)
    {
        switch ((*node)->mFlavour)
        {

            case cFlavourBin:
            {
                if (node == targetList.begin())
                    (*node)->mFlavour = cFlavourOrd;
                else
                {
                    list<LayoutTree::Node*>::iterator previous = node;
                    previous--;
                    switch ((*previous)->mFlavour)
                    {
                        case cFlavourBin:
                        case cFlavourOp:
                        case cFlavourRel:
                        case cFlavourOpen:
                        case cFlavourPunct:
                            (*node)->mFlavour = cFlavourOrd;
                            break;
                    }
                }
                break;
            }
            
            case cFlavourRel:
            case cFlavourClose:
            case cFlavourPunct:
            {
                if (node != targetList.begin())
                {
                    list<LayoutTree::Node*>::iterator previous = node;
                    previous--;
                    if ((*previous)->mFlavour == cFlavourBin)
                        (*previous)->mFlavour = cFlavourOrd;
                }
                
                break;
            }
        }
    }
    if (!targetList.empty() && targetList.back()->mFlavour == cFlavourBin)
        targetList.back()->mFlavour = cFlavourOrd;

    // Second pass: insert inter-atomic spacing according to TeX's rules.
    
    // spaceTable[i][j] gives the amount of space that should be inserted between nodes of
    // flavour i and flavour j.

    // ignoreSpaceTable[i][j] is nonzero whenever the space between i and j should be ignored while in
    // script style or scriptscript style.

    static int spaceTable[8][8] =
    {
        //                     RIGHT
        // ord   op    bin   rel   open  close punct inner
           {0,    3,    4,    5,    0,    0,    0,    3},    // ord
           {3,    3,    0,    5,    0,    0,    0,    3},    // op
           {4,    4,    0,    0,    4,    0,    0,    4},    // bin
           {5,    5,    0,    0,    5,    0,    0,    5},    // rel
           {0,    0,    0,    0,    0,    0,    0,    0},    // open        // LEFT
           {0,    3,    4,    5,    0,    0,    0,    3},    // close
           {3,    3,    0,    3,    3,    3,    3,    3},    // punct
           {3,    3,    4,    5,    3,    0,    3,    3}     // inner
    };

    static int ignoreSpaceTable[8][8] =
    {
        //                     RIGHT
        // ord   op    bin   rel   open  close punct inner
           {0,    0,    1,    1,    0,    0,    0,    1},    // ord
           {0,    0,    0,    1,    0,    0,    0,    1},    // op
           {1,    1,    0,    0,    1,    0,    0,    1},    // bin
           {1,    1,    0,    0,    1,    0,    0,    1},    // rel
           {0,    0,    0,    0,    0,    0,    0,    0},    // open        // LEFT
           {0,    0,    1,    1,    0,    0,    0,    1},    // close
           {1,    1,    0,    1,    1,    1,    1,    1},    // punct
           {1,    0,    1,    1,    1,    0,    1,    1}     // inner
    };

    list<LayoutTree::Node*>::iterator currentAtom = targetList.begin();
    list<LayoutTree::Node*>::iterator previousAtom;
    bool foundFirst = false;
    while (true)
    {
        while (currentAtom != targetList.end() && dynamic_cast<LayoutTree::Space*>(*currentAtom))
            currentAtom++;
        if (currentAtom == targetList.end())
            break;
        
        if (!foundFirst)
            foundFirst = true;
        else
        {
            Flavour  leftFlavour = dynamic_cast<LayoutTree::Fenced*>(*previousAtom) ? cFlavourClose : (*previousAtom)->mFlavour;
            Flavour rightFlavour = dynamic_cast<LayoutTree::Fenced*>(*currentAtom)  ? cFlavourOpen  : (*currentAtom)->mFlavour;

            int width = 

                (ignoreSpaceTable[leftFlavour][rightFlavour] &&
                    (currentStyle == cStyleScript || currentStyle == cStyleScriptScript))

                ? 0 : spaceTable[leftFlavour][rightFlavour];

            targetList.insert(currentAtom, new LayoutTree::Space(width,
                false       // indicates non-user-specified space
                ));
        }
        
        previousAtom = currentAtom;
        currentAtom++;
    }
    
    // Third pass: splice any children Rows into this Row.
    // The idea is that no Row node should have any Rows as children.
    for (list<LayoutTree::Node*>::iterator child = targetList.begin(); child != targetList.end(); child++)
    {
        LayoutTree::Row* childAsRow = dynamic_cast<LayoutTree::Row*>(*child);
        if (childAsRow && (childAsRow->mStyle == currentStyle))
        {
            targetList.splice(child, childAsRow->mChildren);
            delete childAsRow;
            child = targetList.erase(child);
        }
    }
    
    // If there's only one node left, return it by itself (without the enclosing Row object).
    // (NOTE: we don't use list::size() since that's O(n))
    if (!output->mChildren.empty() && output->mChildren.front() == output->mChildren.back())
    {
        auto_ptr<LayoutTree::Node> singleton(output->mChildren.back());
        output->mChildren.pop_back();       // relinquish ownership
        return singleton;
    }
    
    return static_cast< auto_ptr<LayoutTree::Node> >(output);
}

// For each operator, this struct stores (static) info about how it should be translated.
struct OperatorInfo
{
    wstring mText;
    Flavour mFlavour;
    Limits mLimits;
    
    OperatorInfo(const wstring& text, Flavour flavour, Limits limits = cLimitsDisplayLimits) :
        mText(text), mFlavour(flavour), mLimits(limits) { }
};

struct IdentifierInfo
{
    bool mIsItalicDefault;
    wstring mText;
    Flavour mFlavour;
    
    IdentifierInfo(bool isItalicDefault, const wstring& text, Flavour flavour) :
        mIsItalicDefault(isItalicDefault), mText(text), mFlavour(flavour) { }
};

auto_ptr<LayoutTree::Node> MathSymbol::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    // First check for certain easy-to-handle single character commands, like letters or numerals.
    if (mCommand.size() == 1)
    {
        bool good = false;
        bool fancyFontsIllegal = false;
        LatexMathFont::Family defaultFamily = LatexMathFont::cFamilyIt;
        LatexMathFont font = currentFont;
        
        if (mCommand[0] >= L'A' && mCommand[0] <= L'Z')
            good = true;
        else if (mCommand[0] >= L'a' && mCommand[0] <= L'z')
        {
            fancyFontsIllegal = true;
            good = true;
        }
        else if (mCommand[0] >= L'0' && mCommand[0] <= L'9')
        {
            fancyFontsIllegal = true;
            defaultFamily = LatexMathFont::cFamilyRm;
            good = true;
        }
        
        if (good)
        {
            if (font.mFamily == LatexMathFont::cFamilyDefault)
                font.mFamily = defaultFamily;

            if (fancyFontsIllegal && font.mFamily == LatexMathFont::cFamilyCal)
                throw Exception(Exception::cUnavailableSymbolFontCombination, mCommand, L"cal");
            if (fancyFontsIllegal && font.mFamily == LatexMathFont::cFamilyBb)
                throw Exception(Exception::cUnavailableSymbolFontCombination, mCommand, L"bb");
            
            return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolPlain(
                mCommand, font.GetMathmlApproximation(), currentStyle, cFlavourOrd));
        }
        
        // Non-ascii characters
        if (mCommand[0] > 0x7F)
            throw logic_error("Unexpected non-ascii character in MathSymbol::BuildLayoutTree");
    }

    static pair<wstring, wchar_t> lowercaseGreekArray[] =
    {
        make_pair(L"\\alpha",      L'\U000003B1'),
        make_pair(L"\\beta",       L'\U000003B2'),
        make_pair(L"\\gamma",      L'\U000003B3'),
        make_pair(L"\\delta",      L'\U000003B4'),
        make_pair(L"\\epsilon",    L'\U000003F5'),  // straightepsilon
        make_pair(L"\\varepsilon", L'\U000003B5'),  // varepsilon
        make_pair(L"\\zeta",       L'\U000003B6'),
        make_pair(L"\\eta",        L'\U000003B7'),
        make_pair(L"\\theta",      L'\U000003B8'),
        make_pair(L"\\vartheta",   L'\U000003D1'),
        make_pair(L"\\iota",       L'\U000003B9'),
        make_pair(L"\\kappa",      L'\U000003BA'),
        make_pair(L"\\varkappa",   L'\U000003F0'),
        make_pair(L"\\lambda",     L'\U000003BB'),
        make_pair(L"\\mu",         L'\U000003BC'),
        make_pair(L"\\nu",         L'\U000003BD'),
        make_pair(L"\\pi",         L'\U000003C0'),
        make_pair(L"\\varpi",      L'\U000003D6'),
        make_pair(L"\\rho",        L'\U000003C1'),
        make_pair(L"\\varrho",     L'\U000003F1'),
        make_pair(L"\\sigma",      L'\U000003C3'),
        make_pair(L"\\varsigma",   L'\U000003C2'),
        make_pair(L"\\tau",        L'\U000003C4'),
        make_pair(L"\\upsilon",    L'\U000003C5'),
        make_pair(L"\\phi",        L'\U000003D5'),  // straightphi
        make_pair(L"\\varphi",     L'\U000003C6'),
        make_pair(L"\\chi",        L'\U000003C7'),
        make_pair(L"\\psi",        L'\U000003C8'),
        make_pair(L"\\omega",      L'\U000003C9'),
        make_pair(L"\\xi",         L'\U000003BE'),
        make_pair(L"\\digamma",    L'\U000003DD')
    };
    static wishful_hash_map<wstring, wchar_t> lowercaseGreekTable(lowercaseGreekArray, END_ARRAY(lowercaseGreekArray));
    
    wishful_hash_map<wstring, wchar_t>::const_iterator lowercaseGreekLookup = lowercaseGreekTable.find(mCommand);
    if (lowercaseGreekLookup != lowercaseGreekTable.end())
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolPlain(
            wstring(1, lowercaseGreekLookup->second),
            // lowercase greek is only affected by the boldsymbol status, not the family.
            currentFont.mIsBoldsymbol ? cMathmlFontBoldItalic : cMathmlFontItalic,
            currentStyle, cFlavourOrd));
    }

    // FIX: for some reason firefox is selecting perhaps a sans-serif font to render
    // <mi mathvariant="normal">&Gamma;</mi>
    // and for other greek letters too.
    // Maybe should really follow Roger's suggestion re: mathml version 1 fonts

    static pair<wstring, wchar_t> uppercaseGreekArray[] =
    {
        make_pair(L"\\Gamma",     L'\U00000393'),
        make_pair(L"\\Delta",     L'\U00000394'),
        make_pair(L"\\Theta",     L'\U00000398'),
        make_pair(L"\\Lambda",    L'\U0000039B'),
        make_pair(L"\\Pi",        L'\U000003A0'),
        make_pair(L"\\Sigma",     L'\U000003A3'),
        make_pair(L"\\Upsilon",   L'\U000003A5'),
        make_pair(L"\\Phi",       L'\U000003A6'),
        make_pair(L"\\Psi",       L'\U000003A8'),
        make_pair(L"\\Omega",     L'\U000003A9'),
        make_pair(L"\\Xi",        L'\U0000039E')
    };
    static wishful_hash_map<wstring, wchar_t> uppercaseGreekTable(uppercaseGreekArray, END_ARRAY(uppercaseGreekArray));

    wishful_hash_map<wstring, wchar_t>::const_iterator uppercaseGreekLookup = uppercaseGreekTable.find(mCommand);
    if (uppercaseGreekLookup != uppercaseGreekTable.end())
    {
        LatexMathFont font = currentFont;
        if (font.mFamily == LatexMathFont::cFamilyCal)
            throw Exception(Exception::cUnavailableSymbolFontCombination, mCommand, L"cal");
        if (font.mFamily == LatexMathFont::cFamilyBb)
            throw Exception(Exception::cUnavailableSymbolFontCombination, mCommand, L"bb");
        if (font.mFamily == LatexMathFont::cFamilyFrak)
            throw Exception(Exception::cUnavailableSymbolFontCombination, mCommand, L"frak");

        if (font.mFamily == LatexMathFont::cFamilyDefault)
            font.mFamily = LatexMathFont::cFamilyRm;
    
        return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolPlain(
            wstring(1, uppercaseGreekLookup->second), font.GetMathmlApproximation(), currentStyle, cFlavourOrd));
    }

    static pair<wstring, int> spaceArray[] =
    {
        make_pair(L"\\!",       -3),
        make_pair(L"\\,",       3),
        make_pair(L"\\>",       4),
        make_pair(L"\\;",       5),
        make_pair(L"\\quad",    18),
        make_pair(L"\\qquad",   36),
        
        // These last two aren't quite right, but hopefully they're close enough.
        // TeX's rules are too complicated for me to care :-)
        make_pair(L"~",         6),
        make_pair(L"\\ ",       6),
    };
    static wishful_hash_map<wstring, int> spaceTable(spaceArray, END_ARRAY(spaceArray));
    
    wishful_hash_map<wstring, int>::const_iterator spaceLookup = spaceTable.find(mCommand);
    if (spaceLookup != spaceTable.end())
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::Space(spaceLookup->second,
            true      // indicates a user-requested space
            ));
    }

    static pair<wstring, OperatorInfo> operatorArray[] =
    {
        make_pair(L"(",                        OperatorInfo(L"(",             cFlavourOpen)),
        make_pair(L")",                        OperatorInfo(L")",             cFlavourClose)),
        make_pair(L"[",                        OperatorInfo(L"[",             cFlavourOpen)),
        make_pair(L"]",                        OperatorInfo(L"]",             cFlavourClose)),
        make_pair(L"<",                        OperatorInfo(L"<",             cFlavourRel)),
        make_pair(L">",                        OperatorInfo(L">",             cFlavourRel)),
        make_pair(L"+",                        OperatorInfo(L"+",             cFlavourBin)),
        make_pair(L"-",                        OperatorInfo(L"-",             cFlavourBin)),
        make_pair(L"=",                        OperatorInfo(L"=",             cFlavourRel)),
        make_pair(L"|",                        OperatorInfo(L"|",             cFlavourOrd)),
        make_pair(L";",                        OperatorInfo(L";",             cFlavourPunct)),
        make_pair(L":",                        OperatorInfo(L":",             cFlavourRel)),
        make_pair(L",",                        OperatorInfo(L",",             cFlavourPunct)),
        make_pair(L".",                        OperatorInfo(L".",             cFlavourOrd)),
        make_pair(L"/",                        OperatorInfo(L"/",             cFlavourOrd)),
        make_pair(L"?",                        OperatorInfo(L"?",             cFlavourClose)),
        make_pair(L"!",                        OperatorInfo(L"!",             cFlavourClose)),
        make_pair(L"@",                        OperatorInfo(L"@",             cFlavourOrd)),
        make_pair(L"*",                        OperatorInfo(L"*",             cFlavourBin)),
        make_pair(L"\\_",                      OperatorInfo(L"_",             cFlavourOrd)),
        make_pair(L"\\&",                      OperatorInfo(L"&",             cFlavourOrd)),
        make_pair(L"\\$",                      OperatorInfo(L"$",             cFlavourOrd)),
        make_pair(L"\\#",                      OperatorInfo(L"#",             cFlavourOrd)),
        make_pair(L"\\%",                      OperatorInfo(L"%",             cFlavourOrd)),
        make_pair(L"\\{",                      OperatorInfo(L"{",             cFlavourOpen)),
        make_pair(L"\\}",                      OperatorInfo(L"}",             cFlavourClose)),
        make_pair(L"\\lbrace",                 OperatorInfo(L"{",             cFlavourOpen)),
        make_pair(L"\\rbrace",                 OperatorInfo(L"}",             cFlavourClose)),
        make_pair(L"\\vert",                   OperatorInfo(L"|",             cFlavourOrd)),
        make_pair(L"\\lvert",                  OperatorInfo(L"|",             cFlavourOpen)),
        make_pair(L"\\rvert",                  OperatorInfo(L"|",             cFlavourClose)),
        make_pair(L"\\Vert",                   OperatorInfo(L"\U00002225",    cFlavourOrd)),
        make_pair(L"\\lVert",                  OperatorInfo(L"\U00002225",    cFlavourOpen)),
        make_pair(L"\\rVert",                  OperatorInfo(L"\U00002225",    cFlavourClose)),
        make_pair(L"\\lfloor",                 OperatorInfo(L"\U0000230A",    cFlavourOpen)),
        make_pair(L"\\rfloor",                 OperatorInfo(L"\U0000230B",    cFlavourClose)),
        make_pair(L"\\lceil",                  OperatorInfo(L"\U00002308",    cFlavourOpen)),
        make_pair(L"\\rceil",                  OperatorInfo(L"\U00002309",    cFlavourClose)),
        make_pair(L"\\langle",                 OperatorInfo(L"\U00002329",    cFlavourOpen)),
        make_pair(L"\\rangle",                 OperatorInfo(L"\U0000232A",    cFlavourClose)),
        make_pair(L"\\lbrack",                 OperatorInfo(L"[",             cFlavourOpen)),
        make_pair(L"\\rbrack",                 OperatorInfo(L"]",             cFlavourClose)),
        make_pair(L"\\forall",                 OperatorInfo(L"\U00002200",    cFlavourOrd)),
        make_pair(L"\\exists",                 OperatorInfo(L"\U00002203",    cFlavourOrd)),
        make_pair(L"\\leftarrow",              OperatorInfo(L"\U00002190",    cFlavourRel)),
        make_pair(L"\\rightarrow",             OperatorInfo(L"\U00002192",    cFlavourRel)),
        // FIX: for longleftarrow, longrightarrow, Longleftarrow, Longrightarrow, longmapsto, leftrightarrow, Leftrightarrow
        // can we do this using stretchiness? and specify a specific size?
        make_pair(L"\\longleftarrow",          OperatorInfo(L"\U00002190",    cFlavourRel)),
        make_pair(L"\\longrightarrow",         OperatorInfo(L"\U00002192",    cFlavourRel)),
        make_pair(L"\\Leftarrow",              OperatorInfo(L"\U000021D0",    cFlavourRel)),
        make_pair(L"\\Rightarrow",             OperatorInfo(L"\U000021D2",    cFlavourRel)),
        make_pair(L"\\Longleftarrow",          OperatorInfo(L"\U000021D0",    cFlavourRel)),
        make_pair(L"\\Longrightarrow",         OperatorInfo(L"\U000021D2",    cFlavourRel)),
        make_pair(L"\\mapsto",                 OperatorInfo(L"\U000021A6",    cFlavourRel)),
        make_pair(L"\\longmapsto",             OperatorInfo(L"\U000021A6",    cFlavourRel)),
        make_pair(L"\\leftrightarrow",         OperatorInfo(L"\U00002194",    cFlavourRel)),
        make_pair(L"\\Leftrightarrow",         OperatorInfo(L"\U000021D4",    cFlavourRel)),
        make_pair(L"\\longleftrightarrow",     OperatorInfo(L"\U00002194",    cFlavourRel)),
        make_pair(L"\\Longleftrightarrow",     OperatorInfo(L"\U000021D4",    cFlavourRel)),
        make_pair(L"\\uparrow",                OperatorInfo(L"\U00002191",    cFlavourRel)),
        make_pair(L"\\Uparrow",                OperatorInfo(L"\U000021D1",    cFlavourRel)),
        make_pair(L"\\downarrow",              OperatorInfo(L"\U00002193",    cFlavourRel)),
        make_pair(L"\\Downarrow",              OperatorInfo(L"\U000021D3",    cFlavourRel)),
        make_pair(L"\\updownarrow",            OperatorInfo(L"\U00002195",    cFlavourRel)),
        make_pair(L"\\Updownarrow",            OperatorInfo(L"\U000021D5",    cFlavourRel)),
        make_pair(L"\\searrow",                OperatorInfo(L"\U00002198",    cFlavourRel)),
        make_pair(L"\\nearrow",                OperatorInfo(L"\U00002197",    cFlavourRel)),
        make_pair(L"\\swarrow",                OperatorInfo(L"\U00002199",    cFlavourRel)),
        make_pair(L"\\nwarrow",                OperatorInfo(L"\U00002196",    cFlavourRel)),
        make_pair(L"\\hookrightarrow",         OperatorInfo(L"\U000021AA",    cFlavourRel)),
        make_pair(L"\\hookleftarrow",          OperatorInfo(L"\U000021A9",    cFlavourRel)),
        make_pair(L"\\upharpoonright",         OperatorInfo(L"\U000021BE",    cFlavourRel)),
        make_pair(L"\\upharpoonleft",          OperatorInfo(L"\U000021BF",    cFlavourRel)),
        make_pair(L"\\downharpoonright",       OperatorInfo(L"\U000021C2",    cFlavourRel)),
        make_pair(L"\\downharpoonleft",        OperatorInfo(L"\U000021C3",    cFlavourRel)),
        make_pair(L"\\rightharpoonup",         OperatorInfo(L"\U000021C0",    cFlavourRel)),
        make_pair(L"\\rightharpoondown",       OperatorInfo(L"\U000021C1",    cFlavourRel)),
        make_pair(L"\\leftharpoonup",          OperatorInfo(L"\U000021BC",    cFlavourRel)),
        make_pair(L"\\leftharpoondown",        OperatorInfo(L"\U000021BD",    cFlavourRel)),
        make_pair(L"\\nleftarrow",             OperatorInfo(L"\U0000219A",    cFlavourRel)),
        make_pair(L"\\nrightarrow",            OperatorInfo(L"\U0000219B",    cFlavourRel)),
        make_pair(L"\\supset",                 OperatorInfo(L"\U00002283",    cFlavourRel)),
        make_pair(L"\\subset",                 OperatorInfo(L"\U00002282",    cFlavourRel)),
        make_pair(L"\\supseteq",               OperatorInfo(L"\U00002287",    cFlavourRel)),
        make_pair(L"\\subseteq",               OperatorInfo(L"\U00002286",    cFlavourRel)),
        make_pair(L"\\sqsupset",               OperatorInfo(L"\U00002290",    cFlavourRel)),
        make_pair(L"\\sqsubset",               OperatorInfo(L"\U0000228F",    cFlavourRel)),
        make_pair(L"\\sqsupseteq",             OperatorInfo(L"\U00002292",    cFlavourRel)),
        make_pair(L"\\sqsubseteq",             OperatorInfo(L"\U00002291",    cFlavourRel)),
        make_pair(L"\\supsetneq",              OperatorInfo(L"\U0000228B",    cFlavourRel)),
        make_pair(L"\\subsetneq",              OperatorInfo(L"\U0000228A",    cFlavourRel)),
        make_pair(L"\\in",                     OperatorInfo(L"\U00002208",    cFlavourRel)),
        make_pair(L"\\ni",                     OperatorInfo(L"\U0000220B",    cFlavourRel)),
        make_pair(L"\\notin",                  OperatorInfo(L"\U00002209",    cFlavourRel)),
        // FIX: \iff needs 5 mu space on each side
        // FIX: \iff needs to be stretchy and have size set larger
        make_pair(L"\\iff",                    OperatorInfo(L"\U000021D4",    cFlavourRel)),
        make_pair(L"\\mid",                    OperatorInfo(L"|",             cFlavourRel)),
        make_pair(L"\\sim",                    OperatorInfo(L"\U0000223C",    cFlavourRel)),
        make_pair(L"\\simeq",                  OperatorInfo(L"\U00002243",    cFlavourRel)),
        make_pair(L"\\approx",                 OperatorInfo(L"\U00002248",    cFlavourRel)),
        make_pair(L"\\propto",                 OperatorInfo(L"\U0000221D",    cFlavourRel)),
        make_pair(L"\\equiv",                  OperatorInfo(L"\U00002208",    cFlavourRel)),
        make_pair(L"\\cong",                   OperatorInfo(L"\U00002245",    cFlavourRel)),
        make_pair(L"\\neq",                    OperatorInfo(L"\U00002260",    cFlavourRel)),
        make_pair(L"\\ll",                     OperatorInfo(L"\U0000226A",    cFlavourRel)),
        make_pair(L"\\gg",                     OperatorInfo(L"\U0000226B",    cFlavourRel)),
        make_pair(L"\\geq",                    OperatorInfo(L"\U00002265",    cFlavourRel)),
        make_pair(L"\\leq",                    OperatorInfo(L"\U00002264",    cFlavourRel)),
        make_pair(L"\\triangleleft",           OperatorInfo(L"\U000022B2",    cFlavourBin)),
        make_pair(L"\\triangleright",          OperatorInfo(L"\U000022B3",    cFlavourBin)),
        make_pair(L"\\trianglelefteq",         OperatorInfo(L"\U000022B4",    cFlavourRel)),
        make_pair(L"\\trianglerighteq",        OperatorInfo(L"\U000022B5",    cFlavourRel)),
        make_pair(L"\\models",                 OperatorInfo(L"\U000022A7",    cFlavourRel)),
        make_pair(L"\\vdash",                  OperatorInfo(L"\U000022A2",    cFlavourRel)),
        make_pair(L"\\Vdash",                  OperatorInfo(L"\U000022A9",    cFlavourRel)),
        make_pair(L"\\vDash",                  OperatorInfo(L"\U000022A8",    cFlavourRel)),
        make_pair(L"\\lesssim",                OperatorInfo(L"\U00002272",    cFlavourRel)),
        make_pair(L"\\nless",                  OperatorInfo(L"\U0000226E",    cFlavourRel)),
        make_pair(L"\\ngeq",                   OperatorInfo(L"\U00002271",    cFlavourRel)),
        make_pair(L"\\nleq",                   OperatorInfo(L"\U00002270",    cFlavourRel)),
        make_pair(L"\\times",                  OperatorInfo(L"\U000000D7",    cFlavourBin)),
        make_pair(L"\\div",                    OperatorInfo(L"\U000000F7",    cFlavourBin)),
        make_pair(L"\\wedge",                  OperatorInfo(L"\U00002227",    cFlavourBin)),
        make_pair(L"\\vee",                    OperatorInfo(L"\U00002228",    cFlavourBin)),
        make_pair(L"\\oplus",                  OperatorInfo(L"\U00002295",    cFlavourBin)),
        make_pair(L"\\otimes",                 OperatorInfo(L"\U00002297",    cFlavourBin)),
        make_pair(L"\\cap",                    OperatorInfo(L"\U00002229",    cFlavourBin)),
        make_pair(L"\\cup",                    OperatorInfo(L"\U0000222A",    cFlavourBin)),
        make_pair(L"\\sqcap",                  OperatorInfo(L"\U00002293",    cFlavourBin)),
        make_pair(L"\\sqcup",                  OperatorInfo(L"\U00002294",    cFlavourBin)),
        make_pair(L"\\smile",                  OperatorInfo(L"\U00002323",    cFlavourRel)),
        make_pair(L"\\frown",                  OperatorInfo(L"\U00002322",    cFlavourRel)),
        // FIX: how to make these smiles/frowns smaller?
        make_pair(L"\\smallsmile",             OperatorInfo(L"\U00002323",    cFlavourRel)),
        make_pair(L"\\smallfrown",             OperatorInfo(L"\U00002322",    cFlavourRel)),
        make_pair(L"\\setminus",               OperatorInfo(L"\U00002216",    cFlavourBin)),
        // FIX: how to make smallsetminus smaller?
        make_pair(L"\\smallsetminus",          OperatorInfo(L"\U00002216",    cFlavourBin)),
        // FIX: \And needs space of 5 mu on each side (in addition to flavour-based space)
        make_pair(L"\\And",                    OperatorInfo(L"&",             cFlavourRel)),
        make_pair(L"\\star",                   OperatorInfo(L"\U000022C6",    cFlavourBin)),
        make_pair(L"\\triangle",               OperatorInfo(L"\U000025B3",    cFlavourOrd)),
        make_pair(L"\\wr",                     OperatorInfo(L"\U00002240",    cFlavourBin)),
        make_pair(L"\\circ",                   OperatorInfo(L"\U00002218",    cFlavourBin)),
        make_pair(L"\\lnot",                   OperatorInfo(L"\U000000AC",    cFlavourOrd)),
        make_pair(L"\\nabla",                  OperatorInfo(L"\U00002207",    cFlavourOrd)),
        make_pair(L"\\prime",                  OperatorInfo(L"\U00002032",    cFlavourOrd)),
        make_pair(L"\\backslash",              OperatorInfo(L"\U00002216",    cFlavourOrd)),
        make_pair(L"\\pm",                     OperatorInfo(L"\U000000B1",    cFlavourBin)),
        make_pair(L"\\mp",                     OperatorInfo(L"\U00002213",    cFlavourBin)),
        make_pair(L"\\angle",                  OperatorInfo(L"\U00002220",    cFlavourOrd)),
        
        // FIX: amsmath redefines \colon, does interesting things with spacing... to think about...
        // FIX: needs 2mu before, and 6 afterwards (NOT in addition to flavour-based space)
        // FIX: is this the right flavour for "\colon"?
        make_pair(L"\\colon",                  OperatorInfo(L":",             cFlavourPunct)),

        make_pair(L"\\Diamond",                OperatorInfo(L"\U000022C4",    cFlavourBin)),
        make_pair(L"\\nmid",                   OperatorInfo(L"\U00002224",    cFlavourRel)),    
        make_pair(L"\\square",                 OperatorInfo(L"\U000025A1",    cFlavourOrd)),
        make_pair(L"\\Box",                    OperatorInfo(L"\U000025A1",    cFlavourOrd)),
        make_pair(L"\\checkmark",              OperatorInfo(L"\U00002713",    cFlavourOrd)),
        make_pair(L"\\complement",             OperatorInfo(L"\U00002201",    cFlavourOrd)),
        make_pair(L"\\flat",                   OperatorInfo(L"\U0000266D",    cFlavourOrd)),
        make_pair(L"\\sharp",                  OperatorInfo(L"\U0000266F",    cFlavourOrd)),
        make_pair(L"\\natural",                OperatorInfo(L"\U0000266E",    cFlavourOrd)),
        make_pair(L"\\bullet",                 OperatorInfo(L"\U00002022",    cFlavourBin)),
        make_pair(L"\\dagger",                 OperatorInfo(L"\U00002020",    cFlavourBin)),
        make_pair(L"\\ddagger",                OperatorInfo(L"\U00002021",    cFlavourBin)),
        make_pair(L"\\clubsuit",               OperatorInfo(L"\U00002663",    cFlavourOrd)),
        make_pair(L"\\spadesuit",              OperatorInfo(L"\U00002660",    cFlavourOrd)),
        make_pair(L"\\heartsuit",              OperatorInfo(L"\U00002665",    cFlavourOrd)),
        make_pair(L"\\diamondsuit",            OperatorInfo(L"\U00002666",    cFlavourOrd)),
        make_pair(L"\\top",                    OperatorInfo(L"\U000022A4",    cFlavourOrd)),
        make_pair(L"\\bot",                    OperatorInfo(L"\U000022A5",    cFlavourOrd)),
        make_pair(L"\\perp",                   OperatorInfo(L"\U000022A5",    cFlavourRel)),
        make_pair(L"\\cdot",                   OperatorInfo(L"\U000022C5",    cFlavourBin)),
        make_pair(L"\\vdots",                  OperatorInfo(L"\U000022EE",    cFlavourOrd)),
        make_pair(L"\\ddots",                  OperatorInfo(L"\U000022F1",    cFlavourInner)),
        // FIX: should get mapped to one of \cdots or \ldots...
        /*
        make_pair(L"\\dots",                   OperatorInfo(L"", cFlavourInner)),
        make_pair(L"\\dotsb",                  OperatorInfo(L"", cFlavourInner)),
        make_pair(L"\\ldots",                  OperatorInfo(L"", cFlavourInner)),
        make_pair(L"\\cdots",                  OperatorInfo(L"", cFlavourInner)),
        */

        make_pair(L"\\sum",                    OperatorInfo(L"\U00002211",    cFlavourOp)),
        make_pair(L"\\prod",                   OperatorInfo(L"\U0000220F",    cFlavourOp)),
        make_pair(L"\\int",                    OperatorInfo(L"\U0000222B",    cFlavourOp, cLimitsNoLimits)),
        make_pair(L"\\iint",                   OperatorInfo(L"\U0000222C",    cFlavourOp, cLimitsNoLimits)),
        make_pair(L"\\iiint",                  OperatorInfo(L"\U0000222D",    cFlavourOp, cLimitsNoLimits)),
        make_pair(L"\\iiiint",                 OperatorInfo(L"\U00002A0C",    cFlavourOp, cLimitsNoLimits)),
        make_pair(L"\\oint",                   OperatorInfo(L"\U0000222E",    cFlavourOp, cLimitsNoLimits)),
        make_pair(L"\\bigcap",                 OperatorInfo(L"\U000022C2",    cFlavourOp)),
        make_pair(L"\\bigodot",                OperatorInfo(L"\U00002A00",    cFlavourOp)),
        make_pair(L"\\bigcup",                 OperatorInfo(L"\U000022C3",    cFlavourOp)),
        make_pair(L"\\bigotimes",              OperatorInfo(L"\U00002A02",    cFlavourOp)),
        make_pair(L"\\coprod",                 OperatorInfo(L"\U00002210",    cFlavourOp)),
        make_pair(L"\\bigsqcup",               OperatorInfo(L"\U00002A06",    cFlavourOp)),
        make_pair(L"\\bigoplus",               OperatorInfo(L"\U00002A01",    cFlavourOp)),
        make_pair(L"\\bigvee",                 OperatorInfo(L"\U000022C1",    cFlavourOp)),
        make_pair(L"\\biguplus",               OperatorInfo(L"\U00002A04",    cFlavourOp)),
        make_pair(L"\\bigwedge",               OperatorInfo(L"\U000022C0",    cFlavourOp)),

        make_pair(L"\\lim",                    OperatorInfo(L"lim",           cFlavourOp)),
        make_pair(L"\\sup",                    OperatorInfo(L"sup",           cFlavourOp)),
        make_pair(L"\\inf",                    OperatorInfo(L"inf",           cFlavourOp)),
        make_pair(L"\\min",                    OperatorInfo(L"min",           cFlavourOp)),
        make_pair(L"\\max",                    OperatorInfo(L"max",           cFlavourOp)),
        make_pair(L"\\gcd",                    OperatorInfo(L"gcd",           cFlavourOp)),
        make_pair(L"\\det",                    OperatorInfo(L"det",           cFlavourOp)),
        make_pair(L"\\Pr",                     OperatorInfo(L"Pr",            cFlavourOp)),
        // FIX: is this how we want to do these? with nbsp?
        make_pair(L"\\limsup",                 OperatorInfo(L"lim sup",       cFlavourOp)),
        make_pair(L"\\liminf",                 OperatorInfo(L"lim inf",       cFlavourOp)),
        make_pair(L"\\injlim",                 OperatorInfo(L"inj lim",       cFlavourOp)),
        make_pair(L"\\projlim",                OperatorInfo(L"proj lim",      cFlavourOp)),
    };
    static wishful_hash_map<wstring, OperatorInfo> operatorTable(operatorArray, END_ARRAY(operatorArray));

    wishful_hash_map<wstring, OperatorInfo>::const_iterator operatorLookup = operatorTable.find(mCommand);
    if (operatorLookup != operatorTable.end())
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolOperator(
            false, L"",     // not stretchy
            false,          // not an accent
            operatorLookup->second.mText,
            // operators are only affected by the boldsymbol status, not the family.
            currentFont.mIsBoldsymbol ? cMathmlFontBold : cMathmlFontNormal,
            currentStyle, operatorLookup->second.mFlavour, operatorLookup->second.mLimits));
    }

    static pair<wstring, IdentifierInfo> identifierArray[] =
    {
        make_pair(L"\\ker",              IdentifierInfo(false, L"ker",        cFlavourOp)),
        make_pair(L"\\deg",              IdentifierInfo(false, L"deg",        cFlavourOp)),
        make_pair(L"\\hom",              IdentifierInfo(false, L"hom",        cFlavourOp)),
        make_pair(L"\\dim",              IdentifierInfo(false, L"dim",        cFlavourOp)),
        make_pair(L"\\arg",              IdentifierInfo(false, L"arg",        cFlavourOp)),
        make_pair(L"\\sin",              IdentifierInfo(false, L"sin",        cFlavourOp)),
        make_pair(L"\\cos",              IdentifierInfo(false, L"cos",        cFlavourOp)),
        make_pair(L"\\sec",              IdentifierInfo(false, L"sec",        cFlavourOp)),
        make_pair(L"\\csc",              IdentifierInfo(false, L"csc",        cFlavourOp)),
        make_pair(L"\\tan",              IdentifierInfo(false, L"tan",        cFlavourOp)),
        make_pair(L"\\cot",              IdentifierInfo(false, L"cot",        cFlavourOp)),
        make_pair(L"\\arcsin",           IdentifierInfo(false, L"arcsin",     cFlavourOp)),
        make_pair(L"\\arccos",           IdentifierInfo(false, L"arccos",     cFlavourOp)),
        make_pair(L"\\arcsec",           IdentifierInfo(false, L"arcsec",     cFlavourOp)),
        make_pair(L"\\arccsc",           IdentifierInfo(false, L"arccsc",     cFlavourOp)),
        make_pair(L"\\arctan",           IdentifierInfo(false, L"arctan",     cFlavourOp)),
        make_pair(L"\\arccot",           IdentifierInfo(false, L"arccot",     cFlavourOp)),
        make_pair(L"\\sinh",             IdentifierInfo(false, L"sinh",       cFlavourOp)),
        make_pair(L"\\cosh",             IdentifierInfo(false, L"cosh",       cFlavourOp)),
        make_pair(L"\\tanh",             IdentifierInfo(false, L"tanh",       cFlavourOp)),
        make_pair(L"\\coth",             IdentifierInfo(false, L"coth",       cFlavourOp)),
        make_pair(L"\\log",              IdentifierInfo(false, L"log",        cFlavourOp)),
        make_pair(L"\\lg",               IdentifierInfo(false, L"lg",         cFlavourOp)),
        make_pair(L"\\ln",               IdentifierInfo(false, L"ln",         cFlavourOp)),
        make_pair(L"\\exp",              IdentifierInfo(false, L"exp",        cFlavourOp)),

        make_pair(L"\\aleph",            IdentifierInfo(false, L"\U00002135", cFlavourOrd)),
        make_pair(L"\\beth",             IdentifierInfo(false, L"\U00002136", cFlavourOrd)),
        make_pair(L"\\gimel",            IdentifierInfo(false, L"\U00002137", cFlavourOrd)),
        make_pair(L"\\daleth",           IdentifierInfo(false, L"\U00002138", cFlavourOrd)),
        make_pair(L"\\wp",               IdentifierInfo(true,  L"\U00002118", cFlavourOrd)),
        make_pair(L"\\ell",              IdentifierInfo(true,  L"\U00002113", cFlavourOrd)),
        make_pair(L"\\P",                IdentifierInfo(true,  L"\U000000B6", cFlavourOrd)),
        make_pair(L"\\imath",            IdentifierInfo(true,  L"\U00000131", cFlavourOrd)),
        make_pair(L"\\Finv",             IdentifierInfo(false, L"\U00002132", cFlavourOrd)),
        make_pair(L"\\Game",             IdentifierInfo(false, L"\U00002141", cFlavourOrd)),
        make_pair(L"\\partial",          IdentifierInfo(false, L"\U00002202", cFlavourOrd)),
        make_pair(L"\\Re",               IdentifierInfo(false, L"\U0000211C", cFlavourOrd)),
        make_pair(L"\\Im",               IdentifierInfo(false, L"\U00002111", cFlavourOrd)),
        make_pair(L"\\infty",            IdentifierInfo(false, L"\U0000221E", cFlavourOrd)),
        make_pair(L"\\hbar",             IdentifierInfo(false, L"\U00000127", cFlavourOrd)),
        make_pair(L"\\emptyset",         IdentifierInfo(false, L"\U00002205", cFlavourOrd)),
        make_pair(L"\\varnothing",       IdentifierInfo(false, L"\U000000D8", cFlavourOrd)),
        make_pair(L"\\S",                IdentifierInfo(false, L"\U000000A7", cFlavourOrd)),
        make_pair(L"\\eth",              IdentifierInfo(false, L"\U000000F0", cFlavourOrd)),
        make_pair(L"\\hslash",           IdentifierInfo(false, L"\U0000210F", cFlavourOrd)),
        make_pair(L"\\mho",              IdentifierInfo(false, L"\U00002127", cFlavourOrd))
    };
    static wishful_hash_map<wstring, IdentifierInfo> identifierTable(identifierArray, END_ARRAY(identifierArray));

    wishful_hash_map<wstring, IdentifierInfo>::const_iterator identifierLookup = identifierTable.find(mCommand);
    if (identifierLookup != identifierTable.end())
    {
        LatexMathFont font = currentFont;
        font.mFamily = identifierLookup->second.mIsItalicDefault ? LatexMathFont::cFamilyIt : LatexMathFont::cFamilyRm;

        return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolPlain(
            identifierLookup->second.mText, font.GetMathmlApproximation(), currentStyle,
            identifierLookup->second.mFlavour,
            // For all the "\sin"-like functions:
            (identifierLookup->second.mFlavour == cFlavourOp) ? cLimitsNoLimits : cLimitsDisplayLimits));
    }

    throw logic_error("Not implemented yet.");

/*

// FIX: need to think about the "largeop" attribute, and font sizes for these operators.
// In particular need to work out how browsers size these things depending on scriptsize.

        make_pair(L"\\varinjlim",              ),
        make_pair(L"\\varprojlim",             ),
        make_pair(L"\\varlimsup",              ),
        make_pair(L"\\varliminf",              ),
        make_pair(L"\\mod",                    ),
        make_pair(L"\\bmod",                   ),


*/
}

// Stores info about accent commands (like "\hat", "\overrightarrow", etc)
struct AccentInfo {
    wstring mText;
    bool mIsStretchy;
    
    AccentInfo(const wstring& text, bool isStretchy) :
        mText(text), mIsStretchy(isStretchy) { }
};

auto_ptr<LayoutTree::Node> MathCommand1Arg::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    if (mCommand == L"\\sqrtBlahtex")
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::Sqrt(mChild->BuildLayoutTree(currentFont, currentStyle)));
    }

// FIX: implement these: (NOT as accents I think...)
//        make_pair(L"\\overbrace",           AccentInfo(L"\U0000FE37",           true)),
//        make_pair(L"\\underbrace",          AccentInfo(L"\U0000FE38",           true)),
    
    if (mCommand == L"\\pmod")
    {
        auto_ptr<LayoutTree::Row> row(new LayoutTree::Row(currentStyle));
        
        MathmlFont font = currentFont.mIsBoldsymbol ? cMathmlFontBold : cMathmlFontNormal;

        row->mChildren.push_back(new LayoutTree::Space(18, true));      // "true" means user-requested, non-negotiable space
        row->mChildren.push_back(new LayoutTree::SymbolOperator(false, L"", false, L"(", font, currentStyle, cFlavourOpen));
        row->mChildren.push_back(new LayoutTree::SymbolOperator(false, L"", false, L"mod", font, currentStyle, cFlavourOrd));
        row->mChildren.push_back(new LayoutTree::Space(6, false));      // "false" means possibly negotiable space
        row->mChildren.push_back(mChild->BuildLayoutTree(currentFont, currentStyle).release());
        row->mChildren.push_back(new LayoutTree::SymbolOperator(false, L"", false, L")", font, currentStyle, cFlavourClose));
        
        return static_cast<auto_ptr<LayoutTree::Node> >(row);
    }

    if (mCommand == L"\\operatorname" || mCommand == L"\\operatornamewithlimits")
    {
        // FIX: DOC the business with merging adjacent roman font characters in <mi> nodes
        
        LatexMathFont font = currentFont;
        font.mFamily = LatexMathFont::cFamilyRm;        
        auto_ptr<LayoutTree::Node> node = mChild->BuildLayoutTree(font, currentStyle);
        node->mFlavour = cFlavourOp;
        node->mLimits = (mCommand == L"\\operatorname") ? cLimitsNoLimits : cLimitsDisplayLimits;
        return node;
    }
    
    static pair<wstring, wstring> negationArray[] =
    {
        // FIX: add more entries to this table
        // FIX: label these entries so I know what the hell they are
        // FIX: Why is 2208 listed twice???
        make_pair(L"\U00002208",       L"\U00002209"),
        make_pair(L"\U00002208",       L"\U00002262"),
        make_pair(L"\U00002203",       L"\U00002204"),
        make_pair(L"=",                L"\U00002260"),
        make_pair(L"\U00002192",       L"\U0000219B"),
        make_pair(L"\U00002286",       L"\U00002288"),
        make_pair(L"\U0000223C",       L"\U00002241"),
        make_pair(L"\U000022A9",       L"\U000022AE"),
        make_pair(L"\U00002194",       L"\U000021AE"),
    };
    static wishful_hash_map<wstring, wstring> negationTable(negationArray, END_ARRAY(negationArray));
    
    if (mCommand == L"\\not")
    {
        auto_ptr<LayoutTree::Node> child = mChild->BuildLayoutTree(currentFont, currentStyle);
        LayoutTree::SymbolOperator* childAsOperator = dynamic_cast<LayoutTree::SymbolOperator*>(child.get());
        if (!childAsOperator)
            throw Exception(Exception::cInvalidNegation);

        wishful_hash_map<wstring, wstring>::const_iterator negationLookup = negationTable.find(childAsOperator->mText);
        if (negationLookup == negationTable.end())
            throw Exception(Exception::cInvalidNegation);

        childAsOperator->mText = negationLookup->second;
        return child;
    }
    
    static pair<wstring, Flavour> flavourCommandArray[] =
    {
        make_pair(L"\\mathop",        cFlavourOp),
        make_pair(L"\\mathrel",       cFlavourRel),
        make_pair(L"\\mathbin",       cFlavourBin),
        make_pair(L"\\mathord",       cFlavourOrd),
        make_pair(L"\\mathopen",      cFlavourOpen),
        make_pair(L"\\mathclose",     cFlavourClose),
        make_pair(L"\\mathpunct",     cFlavourPunct),
        make_pair(L"\\mathinner",     cFlavourInner)
    };
    static wishful_hash_map<wstring, Flavour> flavourCommandTable(flavourCommandArray, END_ARRAY(flavourCommandArray));
    
    wishful_hash_map<wstring, Flavour>::const_iterator flavourCommand = flavourCommandTable.find(mCommand);
    if (flavourCommand != flavourCommandTable.end())
    {
        auto_ptr<LayoutTree::Node> node = mChild->BuildLayoutTree(currentFont, currentStyle);
        node->mFlavour = flavourCommand->second;
        if (node->mFlavour == cFlavourOp)
            node->mLimits = cLimitsDisplayLimits;
        return node;
    }
    
    static pair<wstring, LatexMathFont::Family> fontCommandArray[] =
    {
        make_pair(L"\\mathbfBlahtex",         LatexMathFont::cFamilyBf),
        make_pair(L"\\mathbbBlahtex",         LatexMathFont::cFamilyBb),
        make_pair(L"\\mathitBlahtex",         LatexMathFont::cFamilyIt),
        make_pair(L"\\mathrmBlahtex",         LatexMathFont::cFamilyRm),
        make_pair(L"\\mathsfBlahtex",         LatexMathFont::cFamilySf),
        make_pair(L"\\mathttBlahtex",         LatexMathFont::cFamilyTt),
        make_pair(L"\\mathcalBlahtex",        LatexMathFont::cFamilyCal),
        make_pair(L"\\mathfrakBlahtex",       LatexMathFont::cFamilyFrak)
    };
    static wishful_hash_map<wstring, LatexMathFont::Family> fontCommandTable(fontCommandArray, END_ARRAY(fontCommandArray));

    wishful_hash_map<wstring, LatexMathFont::Family>::const_iterator fontCommand = fontCommandTable.find(mCommand);
    if (fontCommand != fontCommandTable.end())
    {
        LatexMathFont font = currentFont;
        font.mFamily = fontCommand->second;
        return mChild->BuildLayoutTree(font, currentStyle);
    }
    
    if (mCommand == L"\\boldsymbol")
    {
        LatexMathFont font = currentFont;
        font.mIsBoldsymbol = true;
        return mChild->BuildLayoutTree(font, currentStyle);
    }
    
    static pair<wstring, AccentInfo> accentCommandArray[] =
    {                                                                       // stretchy?
        // FIX: review these...
        make_pair(L"\\hat",                 AccentInfo(L"^",                    false)),
        make_pair(L"\\widehat",             AccentInfo(L"^",                    true)),
        make_pair(L"\\bar",                 AccentInfo(L"\U000000AF",           false)),
        make_pair(L"\\overline",            AccentInfo(L"\U000000AF",           true)),
        make_pair(L"\\underline",           AccentInfo(L"\U000000AF",           true)),
        make_pair(L"\\tilde",               AccentInfo(L"\U000002DC",           false)),
        make_pair(L"\\widetilde",           AccentInfo(L"\U000002DC",           true)),
        make_pair(L"\\overleftarrow",       AccentInfo(L"\U00002190",           true)),
        make_pair(L"\\vec",                 AccentInfo(L"\U000020D7",           true)),
        make_pair(L"\\overrightarrow",      AccentInfo(L"\U00002192",           true)),
        make_pair(L"\\overleftrightarrow",  AccentInfo(L"\U00002194",           true)),
        make_pair(L"\\dot",                 AccentInfo(L"\U000000B7",           false)),
        make_pair(L"\\ddot",                AccentInfo(L"\U000000B7\U000000B7", false)),
        make_pair(L"\\check",               AccentInfo(L"\U000002C7",           false)),
        make_pair(L"\\acute",               AccentInfo(L"\U000000B4",           false)),
        make_pair(L"\\grave",               AccentInfo(L"\U00000060",           false)),
        make_pair(L"\\breve",               AccentInfo(L"\U000002D8",           false))
    };

    static wishful_hash_map<wstring, AccentInfo> accentCommandTable(accentCommandArray, END_ARRAY(accentCommandArray));

    wishful_hash_map<wstring, AccentInfo>::const_iterator accentCommand = accentCommandTable.find(mCommand);
    if (accentCommand != accentCommandTable.end())
    {
        auto_ptr<LayoutTree::Node> base = mChild->BuildLayoutTree(currentFont, currentStyle), lower, upper;

        auto_ptr<LayoutTree::Node> accent(new LayoutTree::SymbolOperator(
            accentCommand->second.mIsStretchy, L"",
            true,       // is an accent
            accentCommand->second.mText,
            currentFont.mIsBoldsymbol ? cMathmlFontBold : cMathmlFontNormal,
            // Note: we don't need to decrement the style here, because LayoutTree::SymbolOperator
            // knows not to insert style changes for accent operators
            currentStyle,
            cFlavourOrd));
        
        if (mCommand == L"\\underline")
            lower = accent;
        else
            upper = accent;

        return auto_ptr<LayoutTree::Node>(new LayoutTree::Scripts(
            currentStyle, cFlavourOrd, cLimitsDisplayLimits,
            false,      // not sideset
            base, upper, lower));
    }
    
    // FIX: change this:
    throw logic_error("Not implemented yet.");
}

auto_ptr<LayoutTree::Node> MathStyleChange::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    static pair<wstring, Style> styleCommandArray[] =
    {
        make_pair(L"\\displaystyle",          cStyleDisplay),
        make_pair(L"\\textstyle",             cStyleText),
        make_pair(L"\\scriptstyle",           cStyleScript),
        make_pair(L"\\scriptscriptstyle",     cStyleScriptScript)
    };
    static wishful_hash_map<wstring, Style> styleCommandTable(styleCommandArray, END_ARRAY(styleCommandArray));

    wishful_hash_map<wstring, Style>::const_iterator styleCommand = styleCommandTable.find(mCommand);
    if (styleCommand != styleCommandTable.end())
    {
        return mChild->BuildLayoutTree(currentFont, styleCommand->second);
    }
    
    static pair<wstring, LatexMathFont::Family> fontCommandArray[] =
    {
        make_pair(L"\\rm",     LatexMathFont::cFamilyRm),
        make_pair(L"\\bf",     LatexMathFont::cFamilyBf),
        make_pair(L"\\it",     LatexMathFont::cFamilyIt),
        make_pair(L"\\cal",    LatexMathFont::cFamilyCal),
        make_pair(L"\\tt",     LatexMathFont::cFamilyTt),
        make_pair(L"\\sf",     LatexMathFont::cFamilySf)
    };
    static wishful_hash_map<wstring, LatexMathFont::Family> fontCommandTable(fontCommandArray, END_ARRAY(fontCommandArray));

    wishful_hash_map<wstring, LatexMathFont::Family>::const_iterator fontCommand = fontCommandTable.find(mCommand);
    if (fontCommand != fontCommandTable.end())
    {
        LatexMathFont font = currentFont;
        font.mFamily = fontCommand->second;
        return mChild->BuildLayoutTree(font, currentStyle);
    }
    
    throw logic_error("Not implemented yet.");
}

auto_ptr<LayoutTree::Node> MathCommand2Args::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    bool isFractionCommand = false;
    bool hasParentheses;
    bool isLineVisible;
    
    if (mCommand == L"\\fracBlahtex" || mCommand == L"\\over")
    {
        isFractionCommand = true;
        isLineVisible = true;
        hasParentheses = false;
    }
    else if (mCommand == L"\\atop")
    {
        isFractionCommand = true;
        isLineVisible = false;
        hasParentheses = false;
    }
    else if (mCommand == L"\\binom" || mCommand == L"\\choose")
    {
        isFractionCommand = true;
        isLineVisible = false;
        hasParentheses = true;
    }
    
    if (isFractionCommand)
    {
        Style smallerStyle = currentStyle;
        switch (currentStyle)
        {
            case cStyleDisplay:    smallerStyle = cStyleText; break;
            case cStyleText:       smallerStyle = cStyleScript; break;
            case cStyleScript:     smallerStyle = cStyleScriptScript; break;
        }
        
        auto_ptr<LayoutTree::Node> inside(new LayoutTree::Fraction(
            currentStyle,
            mChild1->BuildLayoutTree(currentFont, smallerStyle),
            mChild2->BuildLayoutTree(currentFont, smallerStyle),
            isLineVisible));

        if (hasParentheses)
            return auto_ptr<LayoutTree::Node>(new LayoutTree::Fenced(currentStyle, L"(", L")", inside));
        else
            return inside;
    }

    if (mCommand == L"\\rootBlahtex")
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::Root(
            mChild1->BuildLayoutTree(currentFont, currentStyle),
            mChild2->BuildLayoutTree(currentFont, cStyleScriptScript)));            
    }
    
    if (mCommand == L"\\cfrac")
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::Fraction(
            cStyleDisplay,
            mChild1->BuildLayoutTree(currentFont, cStyleText),
            mChild2->BuildLayoutTree(currentFont, cStyleText),
            true        // i.e. there should be a visible fraction line
            ));
    }

    throw logic_error("Not implemented yet.");
}

auto_ptr<LayoutTree::Node> MathGroup::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    // TeX treates any group enclosed by curly braces as an "ordinary" atom.
    // This is why e.g. "123{,}456" looks different to "123,456"
    auto_ptr<LayoutTree::Node> node = mChild->BuildLayoutTree(currentFont, currentStyle);
    node->mFlavour = cFlavourOrd;
    return node;
}

auto_ptr<LayoutTree::Node> MathScripts::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    auto_ptr<LayoutTree::Node> base, upper, lower;
    
    Flavour flavour = cFlavourOrd;
    Limits limits = cLimitsDisplayLimits;
    
    if (mBase.get())
    {
        base = mBase->BuildLayoutTree(currentFont, currentStyle);
        flavour = base->mFlavour;
        
        // FIX: The next line is a slightly nasty hack.
        // We propagate the limits setting of the base to the LayoutTree::Scripts node. Then the Scripts
        // node can just examine its OWN limits setting to decide where to place the scripts. This probably
        // isn't the best way to do this, because strictly speaking the limits setting doesn't apply to the
        // scripts node itself. But it doesn't break, because the limits field on a scripts node should
        // never actually be used for anything else (at least I can't think of a counterexample).
        limits = base->mLimits;
    }
    
    Style smallerStyle = currentStyle;
    switch (smallerStyle)
    {
        case cStyleDisplay:
        case cStyleText:          smallerStyle = cStyleScript; break;
        case cStyleScript:
        case cStyleScriptScript:  smallerStyle = cStyleScriptScript; break;
    }

    if (mUpper.get())
        upper = mUpper->BuildLayoutTree(currentFont, smallerStyle);
    if (mLower.get())
        lower = mLower->BuildLayoutTree(currentFont, smallerStyle);
    
    bool isSideset = (flavour != cFlavourOp) ||
        (limits != cLimitsLimits && (limits != cLimitsDisplayLimits || currentStyle != cStyleDisplay));
    
    return auto_ptr<LayoutTree::Node>(new LayoutTree::Scripts(
        currentStyle, flavour, limits, isSideset, base, upper, lower));
}

auto_ptr<LayoutTree::Node> MathLimits::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    auto_ptr<LayoutTree::Node> node = mChild->BuildLayoutTree(currentFont, currentStyle);

    if (node->mFlavour != cFlavourOp)
        throw Exception(Exception::cMisplacedLimits, mCommand);
    
    if (mCommand == L"\\limits")
        node->mLimits = cLimitsLimits;
    else if (mCommand == L"\\nolimits")
        node->mLimits = cLimitsNoLimits;
    else if (mCommand == L"\\displaylimits")
        node->mLimits = cLimitsDisplayLimits;
    else
        throw logic_error("Unexpected command in MathLimits::BuildLayoutTree.");
    
    return node;
}

auto_ptr<LayoutTree::Node> MathDelimited::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    return auto_ptr<LayoutTree::Node>(new LayoutTree::Fenced(
        currentStyle, gDelimiterTable[mLeftDelimiter], gDelimiterTable[mRightDelimiter],
        mChild->BuildLayoutTree(currentFont, currentStyle)));
}

struct BigInfo
{
    Flavour mFlavour;
    wstring mSize;
    
    BigInfo(Flavour flavour, const wstring& size) :
        mFlavour(flavour), mSize(size) { }
};

auto_ptr<LayoutTree::Node> MathBig::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    static pair<wstring, BigInfo> bigCommandArray[] =
    {
        // FIX: I have no idea if these sizes are correct.
        make_pair(L"\\bigBlahtex",      BigInfo(cFlavourOrd,   L"1.5em")),
        make_pair(L"\\bigl",            BigInfo(cFlavourOpen,  L"1.5em")),
        make_pair(L"\\bigr",            BigInfo(cFlavourClose, L"1.5em")),

        make_pair(L"\\BigBlahtex",      BigInfo(cFlavourOrd,   L"2em"  )),
        make_pair(L"\\Bigl",            BigInfo(cFlavourOpen,  L"2em"  )),
        make_pair(L"\\Bigr",            BigInfo(cFlavourClose, L"2em"  )),

        make_pair(L"\\biggBlahtex",     BigInfo(cFlavourOrd,   L"2.5em")),
        make_pair(L"\\biggl",           BigInfo(cFlavourOpen,  L"2.5em")),
        make_pair(L"\\biggr",           BigInfo(cFlavourClose, L"2.5em")),

        make_pair(L"\\BiggBlahtex",     BigInfo(cFlavourOrd,   L"3em"  )),
        make_pair(L"\\Biggl",           BigInfo(cFlavourOpen,  L"3em"  )),
        make_pair(L"\\Biggr",           BigInfo(cFlavourClose, L"3em"  )),
    };
    static wishful_hash_map<wstring, BigInfo> bigCommandTable(bigCommandArray, END_ARRAY(bigCommandArray));
    
    wishful_hash_map<wstring, BigInfo>::const_iterator bigCommand = bigCommandTable.find(mCommand);
    if (bigCommand != bigCommandTable.end())
    {
        Style newStyle = currentStyle;
        if (currentStyle != cStyleDisplay && currentStyle != cStyleText)
            newStyle = cStyleText;

        // FIX: latex allows "\big."... do we?
        return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolOperator(
            true,       // indicates stretchy="true"
            bigCommand->second.mSize,
            false,      // not an accent
            gDelimiterTable[mDelimiter], cMathmlFontNormal,
            newStyle, bigCommand->second.mFlavour));
    }
    
    throw logic_error("Unknown command in MathBig::BuildLayoutTree");
}

auto_ptr<LayoutTree::Node> MathTableRow::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    throw logic_error("Arrived unexpectedly in MathTableRow::BuildLayoutTree");
}

auto_ptr<LayoutTree::Node> MathTable::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    auto_ptr<LayoutTree::Table> table(new LayoutTree::Table(currentStyle));
    table->mRows.reserve(mRows.size());
    
    for (vector<MathTableRow*>::const_iterator inRow = mRows.begin(); inRow != mRows.end(); inRow++)
    {
        table->mRows.push_back(vector<LayoutTree::Node*>());
        vector<LayoutTree::Node*>& outRow = table->mRows.back();
        for (vector<MathNode*>::const_iterator entry = (*inRow)->mEntries.begin(); entry != (*inRow)->mEntries.end(); entry++)
            outRow.push_back((*entry)->BuildLayoutTree(currentFont, currentStyle).release());
    }
        
    return static_cast<auto_ptr<LayoutTree::Node> >(table);
}

struct EnvironmentInfo
{
    wstring mLeftDelimiter, mRightDelimiter;
    
    EnvironmentInfo(const wstring& leftDelimiter, const wstring& rightDelimiter) :
        mLeftDelimiter(leftDelimiter), mRightDelimiter(rightDelimiter) { }
};

auto_ptr<LayoutTree::Node> MathEnvironment::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    static pair<wstring, EnvironmentInfo> environmentArray[] =
    {
        make_pair(L"matrix",       EnvironmentInfo(L"",           L"")),
        make_pair(L"pmatrix",      EnvironmentInfo(L"(",          L")")),
        make_pair(L"bmatrix",      EnvironmentInfo(L"[",          L"]")),
        make_pair(L"Bmatrix",      EnvironmentInfo(L"{",          L"}")),
        make_pair(L"vmatrix",      EnvironmentInfo(L"|",          L"|")),
        make_pair(L"Vmatrix",      EnvironmentInfo(L"\U00002225", L"\U00002225")),      // DoubleVerticalBar
        make_pair(L"cases",        EnvironmentInfo(L"{",          L"")),
        make_pair(L"aligned",      EnvironmentInfo(L"",           L"")),
        make_pair(L"smallmatrix",  EnvironmentInfo(L"(",          L")")),
    };
    static wishful_hash_map<wstring, EnvironmentInfo> environmentTable(environmentArray, END_ARRAY(environmentArray));
    
    wishful_hash_map<wstring, EnvironmentInfo>::const_iterator environmentName = environmentTable.find(mName);
    if (environmentName == environmentTable.end())
        throw logic_error("Unexpected environment name in MathEnvironment::BuildLayoutTree");

    // For reasons I don't fully comprehend, the "boldsymbol" persists into environments,
    // but the math font doesn't.
    LatexMathFont font;
    font.mIsBoldsymbol = currentFont.mIsBoldsymbol;

    Style tableStyle, fencedStyle;
    if (mName == L"smallmatrix")
        tableStyle = fencedStyle = cStyleScript;
    else if (mName == L"aligned")
        tableStyle = cStyleDisplay;
    else
    {
        tableStyle = cStyleText;
        fencedStyle = (currentStyle == cStyleDisplay) ? cStyleDisplay : cStyleText;
    }
    
    auto_ptr<LayoutTree::Node> table = mTable->BuildLayoutTree(font, tableStyle);

    if (mName == L"aligned")
        dynamic_cast<LayoutTree::Table*>(table.get())->mAlign = cAlignRightLeft;
    else if (mName == L"cases")
        dynamic_cast<LayoutTree::Table*>(table.get())->mAlign = cAlignLeft;

    if (mName == L"aligned" || mName == L"matrix")
        return table;
        
    return auto_ptr<LayoutTree::Node>(new LayoutTree::Fenced(fencedStyle,
        environmentName->second.mLeftDelimiter, environmentName->second.mRightDelimiter, table));
}

auto_ptr<LayoutTree::Node> EnterTextMode::BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const
{
    static pair<wstring, LatexTextFont> textCommandArray[] =
    {                                                                        //  bold?  italic?
        make_pair(L"\\hbox",             LatexTextFont(LatexTextFont::cFamilyRm, false, false)),
        make_pair(L"\\textBlahtex",      LatexTextFont(LatexTextFont::cFamilyRm, false, false)),
        make_pair(L"\\textrmBlahtex",    LatexTextFont(LatexTextFont::cFamilyRm, false, false)),
        make_pair(L"\\textbfBlahtex",    LatexTextFont(LatexTextFont::cFamilyRm, true,  false)),
        make_pair(L"\\emphBlahtex",      LatexTextFont(LatexTextFont::cFamilyRm, false, true)),
        make_pair(L"\\textitBlahtex",    LatexTextFont(LatexTextFont::cFamilyRm, false, true)),
        make_pair(L"\\textsfBlahtex",    LatexTextFont(LatexTextFont::cFamilySf, false, false)),
        make_pair(L"\\textttBlahtex",    LatexTextFont(LatexTextFont::cFamilyTt, false, false))
    };
    static wishful_hash_map<wstring, LatexTextFont> textCommandTable(textCommandArray, END_ARRAY(textCommandArray));
    
    wishful_hash_map<wstring, LatexTextFont>::iterator textCommand = textCommandTable.find(mCommand);
    if (textCommand == textCommandTable.end())
        throw logic_error("Unexpected command in EnterTextMode::BuildLayoutTree");

    Style style = (mCommand == L"\\hbox") ? cStyleText : currentStyle;
    return mChild->BuildLayoutTree(textCommand->second, style);
}

auto_ptr<LayoutTree::Node> TextList::BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const
{
    auto_ptr<LayoutTree::Row> node(new LayoutTree::Row(currentStyle));
    
    // Recursively build layout trees for children, merge Rows to obtain a single Row.
    for (vector<TextNode*>::const_iterator child = mChildren.begin(); child != mChildren.end(); child++)
    {
        auto_ptr<LayoutTree::Node> newNode = (*child)->BuildLayoutTree(currentFont, currentStyle);
        LayoutTree::Row* isRow = dynamic_cast<LayoutTree::Row*>(newNode.get());
        if (isRow)
            node->mChildren.splice(node->mChildren.end(), isRow->mChildren);
        else
            node->mChildren.push_back(newNode.release());
    }
    
    return static_cast<auto_ptr<LayoutTree::Node> >(node);
}

auto_ptr<LayoutTree::Node> TextSymbol::BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const
{
    static pair<wstring, wstring> textCommandArray[] =
    {
        make_pair(L"\\!",                      L""),
        make_pair(L" ",                        L"\U000000A0"),      // NonBreakingSpace
        make_pair(L"~",                        L"\U000000A0"),
        make_pair(L"\\,",                      L"\U000000A0"),
        make_pair(L"\\ ",                      L"\U000000A0"),
        make_pair(L"\\;",                      L"\U000000A0"),
        make_pair(L"\\quad",                   L"\U000000A0\U000000A0"),
        make_pair(L"\\qquad",                  L"\U000000A0\U000000A0\U000000A0\U000000A0"),
        
        make_pair(L"\\&",                      L"&"),
        make_pair(L"<",                        L"<"),
        make_pair(L">",                        L">"),
        make_pair(L"\\_",                      L"_"),
        make_pair(L"\\$",                      L"$"),
        make_pair(L"\\#",                      L"#"),
        make_pair(L"\\%",                      L"%"),
        make_pair(L"\\{",                      L"{"),
        make_pair(L"\\}",                      L"}"),
        make_pair(L"\\textbackslash",          L"\\"),
        make_pair(L"\\textvisiblespace",       L"\U000023B5"),
        make_pair(L"\\O",                      L"\U000000D8"),
        make_pair(L"\\S",                      L"\U000000A7")
    };
    static wishful_hash_map<wstring, wstring> textCommandTable(textCommandArray, END_ARRAY(textCommandArray));
    
    wishful_hash_map<wstring, wstring>::iterator textCommand = textCommandTable.find(mCommand);
    if (textCommand != textCommandTable.end())
    {
        return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolText(
            textCommand->second, currentFont.GetMathmlApproximation(), currentStyle));
    }

    return auto_ptr<LayoutTree::Node>(new LayoutTree::SymbolText(
        mCommand, currentFont.GetMathmlApproximation(), currentStyle));
}

auto_ptr<LayoutTree::Node> TextGroup::BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const
{
    return mChild->BuildLayoutTree(currentFont, currentStyle);
}

auto_ptr<LayoutTree::Node> TextStyleChange::BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const
{
    static pair<wstring, LatexTextFont> textCommandArray[] =
    {                                                             //  bold?  italic?
        make_pair(L"\\rm",    LatexTextFont(LatexTextFont::cFamilyRm, false, false)),
        make_pair(L"\\it",    LatexTextFont(LatexTextFont::cFamilyRm, false, true )),
        make_pair(L"\\bf",    LatexTextFont(LatexTextFont::cFamilyRm, true,  false)),
        make_pair(L"\\sf",    LatexTextFont(LatexTextFont::cFamilySf, false, false)),
        make_pair(L"\\tt",    LatexTextFont(LatexTextFont::cFamilyTt, false, false)),
    };
    static wishful_hash_map<wstring, LatexTextFont> textCommandTable(textCommandArray, END_ARRAY(textCommandArray));
    
    wishful_hash_map<wstring, LatexTextFont>::iterator textCommand = textCommandTable.find(mCommand);
    if (textCommand == textCommandTable.end())
        throw logic_error("Unexpected command in TextStyleChange::BuildLayoutTree");

    return mChild->BuildLayoutTree(textCommand->second, currentStyle);
}

auto_ptr<LayoutTree::Node> TextCommand1Arg::BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const
{
    LatexTextFont font = currentFont;

    if (mCommand == L"\\textrmBlahtex")
        font.mFamily = LatexTextFont::cFamilyRm;
    else if (mCommand == L"\\textttBlahtex")
        font.mFamily = LatexTextFont::cFamilyTt;
    else if (mCommand == L"\\textsfBlahtex")
        font.mFamily = LatexTextFont::cFamilySf;
    else if (mCommand == L"\\textitBlahtex")
        font.mIsItalic = true;
    else if (mCommand == L"\\emphBlahtex")
        font.mIsItalic = !font.mIsItalic;
    else if (mCommand == L"\\textbfBlahtex")
        font.mIsBold = true;
    else if (mCommand == L"\\textBlahtex" || mCommand == L"\\hbox")
        // do nothing!
        { }
    else
        throw logic_error("Unexpected command in TextCommand1Arg::BuildLayoutTree");

    return mChild->BuildLayoutTree(font, currentStyle);
}


// This stream insertion operator exists solely to simplify the code for GetPurifiedTex (below)
wostream& operator<<(wostream& os, const ParseTree::Node& source)
{
    source.GetPurifiedTex(os);
    return os;
}

void MathSymbol::GetPurifiedTex(wostream& os) const
{
    os << L" " << StripBlahtexSuffix(mCommand);
}

void MathCommand1Arg::GetPurifiedTex(wostream& os) const
{
    os << StripBlahtexSuffix(mCommand) << L"{" << *mChild << L"}";
}

void MathStyleChange::GetPurifiedTex(wostream& os) const
{
    os << StripBlahtexSuffix(mCommand) << *mChild;
}

void MathCommand2Args::GetPurifiedTex(wostream& os) const
{
    if (mIsInfix)
    {
        os << L"{" << *mChild1 << L"}" << mCommand << L"{" << *mChild2 << L"}";
    }
    else
    {
        if (mCommand == L"\\rootBlahtex")
            os << L"\\sqrt[{" << *mChild1 << L"}]{" << *mChild2 << L"}";
        else
            os << StripBlahtexSuffix(mCommand) << L"{" << *mChild1 << L"}{" << *mChild2 << L"}";
    }
}

void MathGroup::GetPurifiedTex(wostream& os) const
{
    // We remove nested braces here just for fun.
    if (dynamic_cast<MathGroup*>(mChild.get()))
        os << *mChild;
    else
        os << L"{" << *mChild << L"}";
}

void MathList::GetPurifiedTex(wostream& os) const
{
    for (vector<MathNode*>::const_iterator ptr = mChildren.begin(); ptr != mChildren.end(); ptr++)
        os << **ptr;
}

void MathScripts::GetPurifiedTex(wostream& os) const
{
    if (mBase.get())
        os << *mBase;
    if (mUpper.get())
        os << L"^{" << *mUpper << L"}";
    if (mLower.get())
        os << L"_{" << *mLower << L"}";
}

void MathLimits::GetPurifiedTex(wostream& os) const
{
    os << *mChild << mCommand;
}

void MathDelimited::GetPurifiedTex(wostream& os) const
{
    os << L"\\left" << mLeftDelimiter << *mChild << L"\\right" << mRightDelimiter;
}

void MathBig::GetPurifiedTex(wostream& os) const
{
    os << StripBlahtexSuffix(mCommand) << mDelimiter;
}

void MathTableRow::GetPurifiedTex(wostream& os) const
{
    for (vector<MathNode*>::const_iterator ptr = mEntries.begin(); ptr != mEntries.end(); ptr++)
    {
        if (ptr != mEntries.begin())
            os << L" &";
        os << **ptr;
    }
}

void MathTable::GetPurifiedTex(wostream& os) const
{
    for (vector<MathTableRow*>::const_iterator ptr = mRows.begin(); ptr != mRows.end(); ptr++)
    {
        if (ptr != mRows.begin())
            os << L" \\\\";
        os << **ptr;
    }
}

void MathEnvironment::GetPurifiedTex(wostream& os) const
{
    os << L"\\begin{" << mName << L"}" << *mTable << L"\\end{" << mName << L"}";
}

void TextList::GetPurifiedTex(wostream& os) const
{
    for (vector<TextNode*>::const_iterator ptr = mChildren.begin(); ptr != mChildren.end(); ptr++)
        os << **ptr;
}

void TextGroup::GetPurifiedTex(wostream& os) const
{
    // Let's remove nested braces, just for fun.
    if (dynamic_cast<TextGroup*>(mChild.get()))
        os << *mChild;
    else
        os << L"{" << *mChild << L"}";
}

void TextSymbol::GetPurifiedTex(wostream& os) const
{
    if (mCommand.size() == 1 && mCommand[0] > 0x7F)
        // Replace non-ascii characters by "?".
        os << L"?";
    else
        os << mCommand;
}

void TextStyleChange::GetPurifiedTex(wostream& os) const
{
    os << StripBlahtexSuffix(mCommand) << *mChild;
}

void TextCommand1Arg::GetPurifiedTex(wostream& os) const
{
    os << StripBlahtexSuffix(mCommand) << L"{" << *mChild << L"}";
}

void EnterTextMode::GetPurifiedTex(wostream& os) const
{
    os << StripBlahtexSuffix(mCommand) << L"{" << *mChild << L"}";
}

// ===========================================================================================================
// Now all the ParseTree debugging code

// This function generates the indents used by various debugging Print functions.
wstring indent(int depth)
{
    return wstring(2 * depth, L' ');
}

void MathSymbol::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathSymbol \"" << mCommand << L"\"" << endl;
}

void MathCommand1Arg::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathCommand1Arg \"" << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathCommand2Args::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathCommand2Args \"" << mCommand << L"\"" << endl;
    mChild1->Print(os, depth+1);
    mChild2->Print(os, depth+1);
}

void MathGroup::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathGroup" << endl;
    mChild->Print(os, depth+1);
}

void MathList::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathList" << endl;
    for (vector<MathNode*>::const_iterator ptr = mChildren.begin(); ptr != mChildren.end(); ptr++)
        (*ptr)->Print(os, depth+1);
}

void MathScripts::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathScripts" << endl;
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

void MathLimits::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathLimits \"" << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathStyleChange::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathStyleChange \"" << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathDelimited::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathDelimited \"" << mLeftDelimiter << L"\" \"" << mRightDelimiter << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathBig::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathBig \"" << mCommand << L"\" \"" << mDelimiter << L"\"" << endl;
}

void MathTableRow::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathTableRow" << endl;
    for (vector<MathNode*>::const_iterator ptr = mEntries.begin(); ptr != mEntries.end(); ptr++)
        (*ptr)->Print(os, depth+1);
}

void MathTable::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathTable" << endl;
    for (vector<MathTableRow*>::const_iterator ptr = mRows.begin(); ptr != mRows.end(); ptr++)
        (*ptr)->Print(os, depth+1);
}

void MathEnvironment::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathEnvironment \"" << mName << L"\"" << endl;
    mTable->Print(os, depth+1);
}

void EnterTextMode::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"EnterTextMode \"" << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void TextList::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextList" << endl;
    for (vector<TextNode*>::const_iterator ptr = mChildren.begin(); ptr != mChildren.end(); ptr++)
        (*ptr)->Print(os, depth+1);
}

void TextSymbol::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextSymbol \"" << mCommand << L"\"" << endl;
}

void TextCommand1Arg::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextCommand1Arg \"" << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void TextStyleChange::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextStyleChange \"" << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void TextGroup::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextGroup" << endl;
    mChild->Print(os, depth+1);
}

}
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
