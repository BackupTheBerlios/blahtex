// File "ParseTree3.cpp"
//
// blahtex (version 0.4.2)
// a TeX to MathML converter designed with MediaWiki in mind
// Copyright (C) 2006, David Harvey
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#include <stdexcept>
#include <set>
#include <iomanip>
#include <sstream>
#include "ParseTree.h"

using namespace std;

namespace blahtex
{

MathmlFont TexMathFont::GetMathmlApproximation() const
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

    throw logic_error("Unexpected TexMathFont data");
}

MathmlFont TexTextFont::GetMathmlApproximation() const
{
    switch (mFamily)
    {
        case cFamilyRm:
            return mIsBold
                ? (mIsItalic ? cMathmlFontBoldItalic : cMathmlFontBold)
                : (mIsItalic ? cMathmlFontItalic     : cMathmlFontNormal);

        case cFamilySf:
            return mIsBold
                ? (
                    mIsItalic
                        ? cMathmlFontSansSerifBoldItalic
                        : cMathmlFontBoldSansSerif
                )
                : (
                    mIsItalic
                        ? cMathmlFontSansSerifItalic
                        : cMathmlFontSansSerif
                );

        case cFamilyTt:   return cMathmlFontMonospace;
    }

    throw logic_error("Unexpected TexTextFont data");
}


namespace ParseTree
{

// A couple of destructors that implement ownership conventions.

MathList::~MathList()
{
    for (vector<MathNode*>::iterator
        p = mChildren.begin(); p != mChildren.end(); p++
    )
        delete *p;
}

MathTableRow::~MathTableRow()
{
    for (vector<MathNode*>::iterator
        p = mEntries.begin(); p != mEntries.end(); p++
    )
        delete *p;
}

MathTable::~MathTable()
{
    for (vector<MathTableRow*>::iterator
        p = mRows.begin(); p != mRows.end(); p++
    )
        delete *p;
}

TextList::~TextList()
{
    for (vector<TextNode*>::iterator
        p = mChildren.begin(); p != mChildren.end(); p++
    )
        delete *p;
}

// =========================================================================
// Implementations of ParseTree::Node::GetPurifiedTex()

void MathSymbol::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << L" " << mCommand;
}


void MathCommand1Arg::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    if (mCommand == L"\\not")
    {
        // FIX: This is a nasty hack to make sure that e.g. "\not <"
        // gets sent as "\not <" instead of "\not{<}" which is broken.
        os << mCommand << L" ";
        mChild->GetPurifiedTex(os, options);
    }
    else
    {
        os << mCommand << L"{";
        mChild->GetPurifiedTex(os, options);
        os << L"}";
    }
}


void MathStyleChange::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << mCommand;
    mChild->GetPurifiedTex(os, options);
}


void MathCommand2Args::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    if (mIsInfix)
    {
        os << L"{";
        mChild1->GetPurifiedTex(os, options);
        os << L"}" << mCommand << L"{";
        mChild2->GetPurifiedTex(os, options);
        os << L"}";
    }
    else
    {
        if (mCommand == L"\\rootReserved")
        {
            os << L"\\sqrt[{";
            mChild1->GetPurifiedTex(os, options);
            os << L"}]{";
            mChild2->GetPurifiedTex(os, options);
            os << L"}";
        }
        else
        {
            os << mCommand << L"{";
            mChild1->GetPurifiedTex(os, options);
            os << L"}{";
            mChild2->GetPurifiedTex(os, options);
            os << L"}";
        }
    }
}


void MathGroup::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << L"{";
    mChild->GetPurifiedTex(os, options);
    os << L"}";
}


void MathList::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    for (vector<MathNode*>::const_iterator
        ptr = mChildren.begin();
        ptr != mChildren.end();
        ptr++
    )
        (*ptr)->GetPurifiedTex(os, options);
}


void MathScripts::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    if (mBase.get())
        mBase->GetPurifiedTex(os, options);
    if (mUpper.get())
    {
        os << L"^{";
        mUpper->GetPurifiedTex(os, options);
        os << L"}";
    }
    if (mLower.get())
    {
        os << L"_{";
        mLower->GetPurifiedTex(os, options);
        os << L"}";
    }
}


void MathLimits::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    mChild->GetPurifiedTex(os, options);
    os << mCommand;
}


void MathDelimited::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << L"\\left" << mLeftDelimiter;
    mChild->GetPurifiedTex(os, options);
    os << L"\\right" << mRightDelimiter;
}


void MathBig::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << mCommand << mDelimiter;
}


void MathTableRow::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    for (vector<MathNode*>::const_iterator
        ptr = mEntries.begin();
        ptr != mEntries.end();
        ptr++
    )
    {
        if (ptr != mEntries.begin())
            os << L" &";
        (*ptr)->GetPurifiedTex(os, options);
    }
}


void MathTable::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    for (vector<MathTableRow*>::const_iterator
        ptr = mRows.begin();
        ptr != mRows.end();
        ptr++
    )
    {
        if (ptr != mRows.begin())
            os << L" \\\\";
        (*ptr)->GetPurifiedTex(os, options);
    }
}


void MathEnvironment::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    if (mIsShort)
    {
        os << L"\\" << mName << L"{";
        mTable->GetPurifiedTex(os, options);
        os << L"}";
    }
    else
    {
        os << L"\\begin{" << mName << L"}";
        mTable->GetPurifiedTex(os, options);
        os << L"\\end{" << mName << L"}";
    }
}


void TextList::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    for (vector<TextNode*>::const_iterator
        ptr = mChildren.begin();
        ptr != mChildren.end();
        ptr++
    )
        (*ptr)->GetPurifiedTex(os, options);
}


void TextGroup::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << L"{";
    mChild->GetPurifiedTex(os, options);
    os << L"}";
}


void TextSymbol::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    // These are all the non-ASCII unicode characters that we permit inside
    // text mode.
    static wchar_t gAllowedUnicodeArray[] = {
        161, 163, 167, 169, 172, 174, 176, 181, 182, 191, 192, 193, 194,
        195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
        223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
        236, 237, 238, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
        251, 252, 253, 255, 256, 257, 258, 259, 262, 263, 264, 265, 266,
        267, 268, 269, 270, 271, 274, 275, 276, 277, 278, 279, 282, 283,
        284, 285, 286, 287, 288, 289, 290, 292, 293, 296, 297, 298, 299,
        300, 301, 304, 305, 308, 309, 310, 311, 313, 314, 315, 316, 317,
        318, 321, 322, 323, 324, 325, 326, 327, 328, 332, 333, 334, 335,
        336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348,
        349, 350, 351, 352, 353, 354, 355, 356, 357, 360, 361, 362, 363,
        364, 365, 366, 367, 368, 369, 372, 373, 374, 375, 376, 377, 378,
        379, 380, 381, 382, 461, 462, 463, 464, 465, 466, 467, 468, 482,
        483, 486, 487, 488, 489, 496, 500, 501, 504, 505, 508, 509, 510,
        511, 536, 537, 538, 539, 542, 543, 550, 551, 552, 553, 558, 559,
        562, 563
    };

    static set<wchar_t> gAllowedUnicodeTable(
        gAllowedUnicodeArray,
        END_ARRAY(gAllowedUnicodeArray)
    );

    if (mCommand.size() == 1 && mCommand[0] > 0x7F)
    {
        if (
            options.mUseUcsPackage &&
            gAllowedUnicodeTable.count(mCommand[0])
        )
            os << L"\\unichar{"
                << static_cast<unsigned>(mCommand[0]) << L"}";
        else
        {
            wostringstream code;
            code << hex << setfill(L'0') << uppercase << setw(8)
                << static_cast<unsigned>(mCommand[0]);
            throw Exception(
                L"PngIncompatibleCharacter", L"U+" + code.str()
            );
        }
    }
    else
        os << mCommand;
}


void TextStyleChange::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << mCommand;
    mChild->GetPurifiedTex(os, options);
}


void TextCommand1Arg::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << mCommand << L"{";
    mChild->GetPurifiedTex(os, options);
    os << L"}";
}


void EnterTextMode::GetPurifiedTex(
    wostream& os,
    const PurifiedTexOptions& options
) const
{
    os << mCommand << L"{";
    mChild->GetPurifiedTex(os, options);
    os << L"}";
}

// =========================================================================
// Now all the ParseTree debugging code.

// This function generates the indents used by various debugging Print()
// functions.
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
    os << indent(depth) << L"MathCommand1Arg \""
        << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathCommand2Args::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathCommand2Args \""
        << mCommand << L"\"" << endl;
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
    for (vector<MathNode*>::const_iterator
        ptr = mChildren.begin(); ptr != mChildren.end(); ptr++
    )
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
    os << indent(depth) << L"MathStyleChange \""
        << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathDelimited::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathDelimited \"" << mLeftDelimiter
        << L"\" \"" << mRightDelimiter << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void MathBig::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathBig \"" << mCommand << L"\" \""
        << mDelimiter << L"\"" << endl;
}

void MathTableRow::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathTableRow" << endl;
    for (vector<MathNode*>::const_iterator
        ptr = mEntries.begin(); ptr != mEntries.end(); ptr++
    )
        (*ptr)->Print(os, depth+1);
}

void MathTable::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathTable" << endl;
    for (vector<MathTableRow*>::const_iterator
        ptr = mRows.begin(); ptr != mRows.end(); ptr++
    )
        (*ptr)->Print(os, depth+1);
}

void MathEnvironment::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"MathEnvironment \"" << mName << L"\"";
    if (mIsShort)
        os << " (short)";
    os << endl;
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
    for (vector<TextNode*>::const_iterator
        ptr = mChildren.begin(); ptr != mChildren.end(); ptr++
    )
        (*ptr)->Print(os, depth+1);
}

void TextSymbol::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextSymbol \"" << mCommand << L"\"" << endl;
}

void TextCommand1Arg::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextCommand1Arg \""
        << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void TextStyleChange::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextStyleChange \""
        << mCommand << L"\"" << endl;
    mChild->Print(os, depth+1);
}

void TextGroup::Print(wostream& os, int depth) const
{
    os << indent(depth) << L"TextGroup" << endl;
    mChild->Print(os, depth+1);
}

}
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
