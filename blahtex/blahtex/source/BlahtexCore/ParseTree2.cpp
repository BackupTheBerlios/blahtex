// File "ParseTree2.cpp"
//
// blahtex (version 0.4)
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
#include "ParseTree.h"

using namespace std;

namespace blahtex
{

pair<wstring, wchar_t> lowercaseGreekArray[] =
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
wishful_hash_map<wstring, wchar_t> lowercaseGreekTable(
    lowercaseGreekArray,
    END_ARRAY(lowercaseGreekArray)
);


pair<wstring, wchar_t> uppercaseGreekArray[] =
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
wishful_hash_map<wstring, wchar_t> uppercaseGreekTable(
    uppercaseGreekArray,
    END_ARRAY(uppercaseGreekArray)
);


pair<wstring, int> spaceArray[] =
{
    make_pair(L"\\!",       -3),
    make_pair(L"\\,",       3),
    make_pair(L"\\>",       4),
    make_pair(L"\\;",       5),
    make_pair(L"\\quad",    18),
    make_pair(L"\\qquad",   36),
    // These last two aren't quite right, but hopefully they're close
    // enough. TeX's rules are too complicated for me to care :-)
    make_pair(L"~",         6),
    make_pair(L"\\ ",       6)
};
wishful_hash_map<wstring, int> spaceTable(
    spaceArray,
    END_ARRAY(spaceArray)
);


struct OperatorInfo
{
    wstring mText;
    LayoutTree::Node::Flavour mFlavour;
    LayoutTree::Node::Limits mLimits;

    OperatorInfo(
        const wstring& text,
        LayoutTree::Node::Flavour flavour,
        LayoutTree::Node::Limits limits =
            LayoutTree::Node::cLimitsDisplayLimits
    ) :
        mText(text),
        mFlavour(flavour),
        mLimits(limits)
    { }
};

// Here is a list of all commands that get translated as operators,
// together with their MathML translation and flavour.
pair<wstring, OperatorInfo> operatorArray[] =
{
    make_pair(L"(",
        OperatorInfo(L"(", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L")",
        OperatorInfo(L")", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"[",
        OperatorInfo(L"[", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"]",
        OperatorInfo(L"]", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"<",
        OperatorInfo(L"<", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L">",
        OperatorInfo(L">", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"+",
        OperatorInfo(L"+", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"-",
        OperatorInfo(L"-", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"=",
        OperatorInfo(L"=", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"|",
        OperatorInfo(L"|", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L";",
        OperatorInfo(L";", LayoutTree::Node::cFlavourPunct)
    ),
    make_pair(L":",
        OperatorInfo(L":", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L",",
        OperatorInfo(L",", LayoutTree::Node::cFlavourPunct)
    ),
    make_pair(L".",
        OperatorInfo(L".", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"/",
        OperatorInfo(L"/", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"?",
        OperatorInfo(L"?", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"!",
        OperatorInfo(L"!", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"@",
        OperatorInfo(L"@", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"*",
        OperatorInfo(L"*", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\_",
        OperatorInfo(L"_", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\&",
        OperatorInfo(L"&", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\$",
        OperatorInfo(L"$", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\#",
        OperatorInfo(L"#", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\%",
        OperatorInfo(L"%", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\{",
        OperatorInfo(L"{", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\}",
        OperatorInfo(L"}", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\lbrace",
        OperatorInfo(L"{", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rbrace",
        OperatorInfo(L"}", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\vert",
        OperatorInfo(L"|", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\lvert",
        OperatorInfo(L"|", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rvert",
        OperatorInfo(L"|", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\Vert",
        OperatorInfo(L"\U00002225", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\lVert",
        OperatorInfo(L"\U00002225", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rVert",
        OperatorInfo(L"\U00002225", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\lfloor",
        OperatorInfo(L"\U0000230A", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rfloor",
        OperatorInfo(L"\U0000230B", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\lceil",
        OperatorInfo(L"\U00002308", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rceil",
        OperatorInfo(L"\U00002309", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\langle",
        OperatorInfo(L"\U00002329", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rangle",
        OperatorInfo(L"\U0000232A", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\lbrack",
        OperatorInfo(L"[", LayoutTree::Node::cFlavourOpen)
    ),
    make_pair(L"\\rbrack",
        OperatorInfo(L"]", LayoutTree::Node::cFlavourClose)
    ),
    make_pair(L"\\forall",
        OperatorInfo(L"\U00002200", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\exists",
        OperatorInfo(L"\U00002203", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\leftarrow",
        OperatorInfo(L"\U00002190", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\rightarrow",
        OperatorInfo(L"\U00002192", LayoutTree::Node::cFlavourRel)
    ),

    // FIX: for longleftarrow, longrightarrow, Longleftarrow,
    // Longrightarrow, longmapsto, leftrightarrow, Leftrightarrow
    // can we do this using stretchiness? and specify a specific size?
    // Problem is that firefox doesn't repect stretchy commands
    // horizontally in a lot of cases.
    // FIX: should also make regular \leftarrow and \rightarrow (etc)
    // NON-STRETCHY, because they are default by stretchy in the
    // normative operator dictionary.

    make_pair(L"\\longleftarrow",
        OperatorInfo(L"\U00002190", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\longrightarrow",
        OperatorInfo(L"\U00002192", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Leftarrow",
        OperatorInfo(L"\U000021D0", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Rightarrow",
        OperatorInfo(L"\U000021D2", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Longleftarrow",
        OperatorInfo(L"\U000021D0", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Longrightarrow",
        OperatorInfo(L"\U000021D2", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\mapsto",
        OperatorInfo(L"\U000021A6", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\longmapsto",
        OperatorInfo(L"\U000021A6", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\leftrightarrow",
        OperatorInfo(L"\U00002194", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Leftrightarrow",
        OperatorInfo(L"\U000021D4", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\longleftrightarrow",
        OperatorInfo(L"\U00002194", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Longleftrightarrow",
        OperatorInfo(L"\U000021D4", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\uparrow",
        OperatorInfo(L"\U00002191", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Uparrow",
        OperatorInfo(L"\U000021D1", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\downarrow",
        OperatorInfo(L"\U00002193", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Downarrow",
        OperatorInfo(L"\U000021D3", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\updownarrow",
        OperatorInfo(L"\U00002195", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Updownarrow",
        OperatorInfo(L"\U000021D5", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\searrow",
        OperatorInfo(L"\U00002198", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\nearrow",
        OperatorInfo(L"\U00002197", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\swarrow",
        OperatorInfo(L"\U00002199", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\nwarrow",
        OperatorInfo(L"\U00002196", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\hookrightarrow",
        OperatorInfo(L"\U000021AA", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\hookleftarrow",
        OperatorInfo(L"\U000021A9", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\upharpoonright",
        OperatorInfo(L"\U000021BE", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\upharpoonleft",
        OperatorInfo(L"\U000021BF", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\downharpoonright",
        OperatorInfo(L"\U000021C2", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\downharpoonleft",
        OperatorInfo(L"\U000021C3", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\rightharpoonup",
        OperatorInfo(L"\U000021C0", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\rightharpoondown",
        OperatorInfo(L"\U000021C1", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\leftharpoonup",
        OperatorInfo(L"\U000021BC", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\leftharpoondown",
        OperatorInfo(L"\U000021BD", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\nleftarrow",
        OperatorInfo(L"\U0000219A", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\nrightarrow",
        OperatorInfo(L"\U0000219B", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\supset",
        OperatorInfo(L"\U00002283", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\subset",
        OperatorInfo(L"\U00002282", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\supseteq",
        OperatorInfo(L"\U00002287", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\subseteq",
        OperatorInfo(L"\U00002286", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\sqsupset",
        OperatorInfo(L"\U00002290", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\sqsubset",
        OperatorInfo(L"\U0000228F", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\sqsupseteq",
        OperatorInfo(L"\U00002292", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\sqsubseteq",
        OperatorInfo(L"\U00002291", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\supsetneq",
        OperatorInfo(L"\U0000228B", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\subsetneq",
        OperatorInfo(L"\U0000228A", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\in",
        OperatorInfo(L"\U00002208", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\ni",
        OperatorInfo(L"\U0000220B", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\notin",
        OperatorInfo(L"\U00002209", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\mid",
        OperatorInfo(L"|", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\sim",
        OperatorInfo(L"\U0000223C", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\simeq",
        OperatorInfo(L"\U00002243", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\approx",
        OperatorInfo(L"\U00002248", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\propto",
        OperatorInfo(L"\U0000221D", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\equiv",
        OperatorInfo(L"\U00002261", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\cong",
        OperatorInfo(L"\U00002245", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\neq",
        OperatorInfo(L"\U00002260", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\ll",
        OperatorInfo(L"\U0000226A", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\gg",
        OperatorInfo(L"\U0000226B", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\geq",
        OperatorInfo(L"\U00002265", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\leq",
        OperatorInfo(L"\U00002264", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\triangleleft",
        OperatorInfo(L"\U000022B2", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\triangleright",
        OperatorInfo(L"\U000022B3", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\trianglelefteq",
        OperatorInfo(L"\U000022B4", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\trianglerighteq",
        OperatorInfo(L"\U000022B5", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\models",
        OperatorInfo(L"\U000022A7", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\vdash",
        OperatorInfo(L"\U000022A2", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\Vdash",
        OperatorInfo(L"\U000022A9", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\vDash",
        OperatorInfo(L"\U000022A8", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\lesssim",
        OperatorInfo(L"\U00002272", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\nless",
        OperatorInfo(L"\U0000226E", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\ngeq",
        OperatorInfo(L"\U00002271", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\nleq",
        OperatorInfo(L"\U00002270", LayoutTree::Node::cFlavourRel)
    ),

    // FIX: the fonts shipped with Firefox 1.5 don't know about
    // 0x2a2f (&Cross;). So I'm mapping it to 0xd7 (&times;) for now.
    // The only downside of this is that the default operator
    // dictionary in Firefox knows that &Cross; is a binary operator,
    // but it doesn't know this for &times;, so I need to special-case
    // the spacing for &times; (see LayoutTree::Row::BuildMathmlTree).

    make_pair(L"\\times",
#if 0
        OperatorInfo(L"\U00002A2F", LayoutTree::Node::cFlavourBin)
#else
        OperatorInfo(L"\U000000D7", LayoutTree::Node::cFlavourBin)
#endif
    ),
    make_pair(L"\\div",
        OperatorInfo(L"\U000000F7", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\wedge",
        OperatorInfo(L"\U00002227", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\vee",
        OperatorInfo(L"\U00002228", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\oplus",
        OperatorInfo(L"\U00002295", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\otimes",
        OperatorInfo(L"\U00002297", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\cap",
        OperatorInfo(L"\U00002229", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\cup",
        OperatorInfo(L"\U0000222A", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\sqcap",
        OperatorInfo(L"\U00002293", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\sqcup",
        OperatorInfo(L"\U00002294", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\smile",
        OperatorInfo(L"\U00002323", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\frown",
        OperatorInfo(L"\U00002322", LayoutTree::Node::cFlavourRel)
    ),
    // FIX: how to make these smiles/frowns smaller?
    make_pair(L"\\smallsmile",
        OperatorInfo(L"\U00002323", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\smallfrown",
        OperatorInfo(L"\U00002322", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\setminus",
        OperatorInfo(L"\U00002216", LayoutTree::Node::cFlavourBin)
    ),
    // FIX: how to make smallsetminus smaller?
    make_pair(L"\\smallsetminus",
        OperatorInfo(L"\U00002216", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\star",
        OperatorInfo(L"\U000022C6", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\triangle",
        OperatorInfo(L"\U000025B3", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\wr",
        OperatorInfo(L"\U00002240", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\circ",
        OperatorInfo(L"\U00002218", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\lnot",
        OperatorInfo(L"\U000000AC", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\nabla",
        OperatorInfo(L"\U00002207", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\prime",
        OperatorInfo(L"\U00002032", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\backslash",
        OperatorInfo(L"\U00002216", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\pm",
        OperatorInfo(L"\U000000B1", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\mp",
        OperatorInfo(L"\U00002213", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\angle",
        OperatorInfo(L"\U00002220", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\Diamond",
        OperatorInfo(L"\U000022C4", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\nmid",
        OperatorInfo(L"\U00002224", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\square",
        OperatorInfo(L"\U000025A1", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\Box",
        OperatorInfo(L"\U000025A1", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\checkmark",
        OperatorInfo(L"\U00002713", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\complement",
        OperatorInfo(L"\U00002201", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\flat",
        OperatorInfo(L"\U0000266D", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\sharp",
        OperatorInfo(L"\U0000266F", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\natural",
        OperatorInfo(L"\U0000266E", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\bullet",
        OperatorInfo(L"\U00002022", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\dagger",
        OperatorInfo(L"\U00002020", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\ddagger",
        OperatorInfo(L"\U00002021", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\clubsuit",
        OperatorInfo(L"\U00002663", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\spadesuit",
        OperatorInfo(L"\U00002660", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\heartsuit",
        OperatorInfo(L"\U00002665", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\diamondsuit",
        OperatorInfo(L"\U00002666", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\top",
        OperatorInfo(L"\U000022A4", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\bot",
        OperatorInfo(L"\U000022A5", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\perp",
        OperatorInfo(L"\U000022A5", LayoutTree::Node::cFlavourRel)
    ),
    make_pair(L"\\cdot",
        OperatorInfo(L"\U000022C5", LayoutTree::Node::cFlavourBin)
    ),
    make_pair(L"\\vdots",
        OperatorInfo(L"\U000022EE", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\ddots",
        OperatorInfo(L"\U000022F1", LayoutTree::Node::cFlavourInner)
    ),
    make_pair(L"\\cdots",
        OperatorInfo(L"\U000022EF", LayoutTree::Node::cFlavourInner)
    ),
    make_pair(L"\\ldots",
        OperatorInfo(L"\U00002026", LayoutTree::Node::cFlavourInner)
    ),
    // FIX: these next two aren't right. The amsmath package does tricky
    // things so that the dots change their vertical position depending
    // on the surrounding operators. We chicken out and just map them
    // to the same as \cdots and \ldots respectively.
    make_pair(L"\\dotsb",
        OperatorInfo(L"\U000022EF", LayoutTree::Node::cFlavourInner)
    ),
    make_pair(L"\\dots",
        OperatorInfo(L"\U00002026", LayoutTree::Node::cFlavourInner)
    ),
    make_pair(L"\\sum",
        OperatorInfo(L"\U00002211", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\prod",
        OperatorInfo(L"\U0000220F", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\int",
        OperatorInfo(
            L"\U0000222B",
            LayoutTree::Node::cFlavourOp,
            LayoutTree::Node::cLimitsNoLimits
        )
    ),
    make_pair(L"\\iint",
        OperatorInfo(
            L"\U0000222C",
            LayoutTree::Node::cFlavourOp,
            LayoutTree::Node::cLimitsNoLimits
        )
    ),
    make_pair(L"\\iiint",
        OperatorInfo(
            L"\U0000222D",
            LayoutTree::Node::cFlavourOp,
            LayoutTree::Node::cLimitsNoLimits
        )
    ),
    make_pair(L"\\iiiint",
        OperatorInfo(
            L"\U00002A0C",
            LayoutTree::Node::cFlavourOp,
            LayoutTree::Node::cLimitsNoLimits
        )
    ),
    make_pair(L"\\oint",
        OperatorInfo(L"\U0000222E",
            LayoutTree::Node::cFlavourOp,
            LayoutTree::Node::cLimitsNoLimits
        )
    ),
    make_pair(L"\\bigcap",
        OperatorInfo(L"\U000022C2", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigodot",
        OperatorInfo(L"\U00002A00", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigcup",
        OperatorInfo(L"\U000022C3", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigotimes",
        OperatorInfo(L"\U00002A02", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\coprod",
        OperatorInfo(L"\U00002210", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigsqcup",
        OperatorInfo(L"\U00002A06", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigoplus",
        OperatorInfo(L"\U00002A01", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigvee",
        OperatorInfo(L"\U000022C1", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\biguplus",
        OperatorInfo(L"\U00002A04", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\bigwedge",
        OperatorInfo(L"\U000022C0", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\lim",
        OperatorInfo(L"lim", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\sup",
        OperatorInfo(L"sup", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\inf",
        OperatorInfo(L"inf", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\min",
        OperatorInfo(L"min", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\max",
        OperatorInfo(L"max", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\gcd",
        OperatorInfo(L"gcd", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\det",
        OperatorInfo(L"det", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\Pr",
        OperatorInfo(L"Pr", LayoutTree::Node::cFlavourOp)
    ),
    // FIX: the space between the words in these operators is a
    // tiny bit too big.
    make_pair(L"\\limsup",
        OperatorInfo(L"lim sup", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\liminf",
        OperatorInfo(L"lim inf", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\injlim",
        OperatorInfo(L"inj lim", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\projlim",
        OperatorInfo(L"proj lim", LayoutTree::Node::cFlavourOp)
    )
};
wishful_hash_map<wstring, OperatorInfo> operatorTable(
    operatorArray,
    END_ARRAY(operatorArray)
);


struct IdentifierInfo
{
    bool mIsItalicDefault;
    wstring mText;
    LayoutTree::Node::Flavour mFlavour;

    IdentifierInfo(
        bool isItalicDefault,
        const wstring& text,
        LayoutTree::Node::Flavour flavour
    ) :
        mIsItalicDefault(isItalicDefault),
        mText(text),
        mFlavour(flavour)
    { }
};

// A list of all commands that get translated as identifiers,
// their MathML translations, flavour, and whether they should be
// rendered in italic font.
pair<wstring, IdentifierInfo> identifierArray[] =
{
    make_pair(L"\\ker",
        IdentifierInfo(false, L"ker", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\deg",
        IdentifierInfo(false, L"deg", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\hom",
        IdentifierInfo(false, L"hom", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\dim",
        IdentifierInfo(false, L"dim", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\arg",
        IdentifierInfo(false, L"arg", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\sin",
        IdentifierInfo(false, L"sin", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\cos",
        IdentifierInfo(false, L"cos", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\sec",
        IdentifierInfo(false, L"sec", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\csc",
        IdentifierInfo(false, L"csc", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\tan",
        IdentifierInfo(false, L"tan", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\cot",
        IdentifierInfo(false, L"cot", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\arcsin",
        IdentifierInfo(false, L"arcsin", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\arccos",
        IdentifierInfo(false, L"arccos", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\arctan",
        IdentifierInfo(false, L"arctan", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\sinh",
        IdentifierInfo(false, L"sinh", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\cosh",
        IdentifierInfo(false, L"cosh", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\tanh",
        IdentifierInfo(false, L"tanh", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\coth",
        IdentifierInfo(false, L"coth", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\log",
        IdentifierInfo(false, L"log", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\lg",
        IdentifierInfo(false, L"lg", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\ln",
        IdentifierInfo(false, L"ln", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\exp",
        IdentifierInfo(false, L"exp", LayoutTree::Node::cFlavourOp)
    ),
    make_pair(L"\\aleph",
        IdentifierInfo(false, L"\U00002135", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\beth",
        IdentifierInfo(false, L"\U00002136", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\gimel",
        IdentifierInfo(false, L"\U00002137", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\daleth",
        IdentifierInfo(false, L"\U00002138", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\wp",
        IdentifierInfo(true, L"\U00002118", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\ell",
        IdentifierInfo(true, L"\U00002113", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\P",
        IdentifierInfo(true, L"\U000000B6", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\imath",
        IdentifierInfo(true, L"\U00000131", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\Finv",
        IdentifierInfo(false, L"\U00002132", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\Game",
        IdentifierInfo(false, L"\U00002141", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\partial",
        IdentifierInfo(false, L"\U00002202", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\Re",
        IdentifierInfo(false, L"\U0000211C", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\Im",
        IdentifierInfo(false, L"\U00002111", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\infty",
        IdentifierInfo(false, L"\U0000221E", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\hbar",
        IdentifierInfo(false, L"\U00000127", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\emptyset",
        IdentifierInfo(false, L"\U00002205", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\varnothing",
        IdentifierInfo(false, L"\U000000D8", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\S",
        IdentifierInfo(false, L"\U000000A7", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\eth",
        IdentifierInfo(false, L"\U000000F0", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\hslash",
        IdentifierInfo(false, L"\U0000210F", LayoutTree::Node::cFlavourOrd)
    ),
    make_pair(L"\\mho",
        IdentifierInfo(false, L"\U00002127", LayoutTree::Node::cFlavourOrd)
    )
};
wishful_hash_map<wstring, IdentifierInfo> identifierTable(
    identifierArray,
    END_ARRAY(identifierArray)
);


namespace ParseTree
{

auto_ptr<LayoutTree::Node> MathSymbol::BuildLayoutTree(
    const TexMathFont& currentFont,
    LayoutTree::Node::Style currentStyle
) const
{
    // First check for certain easy-to-handle single character commands,
    // like letters or numerals.
    if (mCommand.size() == 1)
    {
        bool good = false;
        // fancyFontsIllegal is set for characters which can't be
        // displayed in frak, cal or bb fonts.
        bool fancyFontsIllegal = false;
        TexMathFont::Family defaultFamily = TexMathFont::cFamilyIt;
        TexMathFont font = currentFont;

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
            defaultFamily = TexMathFont::cFamilyRm;
            good = true;
        }

        if (good)
        {
            if (font.mFamily == TexMathFont::cFamilyDefault)
                font.mFamily = defaultFamily;

            if (fancyFontsIllegal &&
                font.mFamily == TexMathFont::cFamilyCal
            )
                throw Exception(
                    L"UnavailableSymbolFontCombination", mCommand, L"cal"
                );

            if (fancyFontsIllegal &&
                font.mFamily == TexMathFont::cFamilyBb
            )
                throw Exception(
                    L"UnavailableSymbolFontCombination", mCommand, L"bb"
                );

            return auto_ptr<LayoutTree::Node>(
                new LayoutTree::SymbolPlain(
                    mCommand,
                    font.GetMathmlApproximation(),
                    currentStyle,
                    LayoutTree::Node::cFlavourOrd
                )
            );
        }

        // Non-ascii characters
        if (mCommand[0] > 0x7F)
            throw logic_error(
                "Unexpected non-ASCII character "
                "in MathSymbol::BuildLayoutTree"
            );
    }

    wishful_hash_map<wstring, wchar_t>::const_iterator
        lowercaseGreekLookup = lowercaseGreekTable.find(mCommand);

    if (lowercaseGreekLookup != lowercaseGreekTable.end())
    {
        return auto_ptr<LayoutTree::Node>(
            new LayoutTree::SymbolPlain(
                wstring(1, lowercaseGreekLookup->second),
                // lowercase greek is only affected by the boldsymbol
                // status, not the family.
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBoldItalic : cMathmlFontItalic,
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
    }

    wishful_hash_map<wstring, wchar_t>::const_iterator
        uppercaseGreekLookup = uppercaseGreekTable.find(mCommand);

    if (uppercaseGreekLookup != uppercaseGreekTable.end())
    {
        TexMathFont font = currentFont;
        if (font.mFamily == TexMathFont::cFamilyCal)
            throw Exception(
                L"UnavailableSymbolFontCombination", mCommand, L"cal"
            );

        if (font.mFamily == TexMathFont::cFamilyBb)
            throw Exception(
                L"UnavailableSymbolFontCombination", mCommand, L"bb"
            );

        if (font.mFamily == TexMathFont::cFamilyFrak)
            throw Exception(
                L"UnavailableSymbolFontCombination", mCommand, L"frak"
            );

        if (font.mFamily == TexMathFont::cFamilyDefault)
            font.mFamily = TexMathFont::cFamilyRm;

        return auto_ptr<LayoutTree::Node>(
            new LayoutTree::SymbolPlain(
                wstring(1, uppercaseGreekLookup->second),
                font.GetMathmlApproximation(),
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
    }

    wishful_hash_map<wstring, int>::const_iterator
        spaceLookup = spaceTable.find(mCommand);

    if (spaceLookup != spaceTable.end())
    {
        return auto_ptr<LayoutTree::Node>(
            new LayoutTree::Space(
                spaceLookup->second,
                true      // true = indicates a user-requested space
            )
        );
    }

    wishful_hash_map<wstring, OperatorInfo>::const_iterator
        operatorLookup = operatorTable.find(mCommand);

    if (operatorLookup != operatorTable.end())
    {
        return auto_ptr<LayoutTree::Node>(
            new LayoutTree::SymbolOperator(
                false, L"",     // not stretchy
                false,          // not an accent
                operatorLookup->second.mText,
                // operators are only affected by the boldsymbol status,
                // not the family.
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBold : cMathmlFontNormal,
                currentStyle,
                operatorLookup->second.mFlavour,
                operatorLookup->second.mLimits
            )
        );
    }

    wishful_hash_map<wstring, IdentifierInfo>::const_iterator
        identifierLookup = identifierTable.find(mCommand);

    if (identifierLookup != identifierTable.end())
    {
        TexMathFont font = currentFont;
        font.mFamily =
            identifierLookup->second.mIsItalicDefault
                ? TexMathFont::cFamilyIt : TexMathFont::cFamilyRm;

        return auto_ptr<LayoutTree::Node>(
            new LayoutTree::SymbolPlain(
                identifierLookup->second.mText,
                font.GetMathmlApproximation(),
                currentStyle,
                identifierLookup->second.mFlavour,
                // For all the "\sin"-like functions:
                (
                    identifierLookup->second.mFlavour ==
                    LayoutTree::Node::cFlavourOp
                )
                    ? LayoutTree::Node::cLimitsNoLimits
                    : LayoutTree::Node::cLimitsDisplayLimits
            )
        );
    }

    if (mCommand == L"\\And")
    {
        auto_ptr<LayoutTree::Row> row(new LayoutTree::Row(currentStyle));
        row->mFlavour = LayoutTree::Node::cFlavourRel;
        row->mChildren.push_back(new LayoutTree::Space(5, true));
        row->mChildren.push_back(
            new LayoutTree::SymbolOperator(
                false,
                L"",
                false,
                L"&",
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBold : cMathmlFontNormal,
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
        row->mChildren.push_back(new LayoutTree::Space(5, true));
        return static_cast<auto_ptr<LayoutTree::Node> >(row);
    }

    if (mCommand == L"\\iff")
    {
        auto_ptr<LayoutTree::Row> row(new LayoutTree::Row(currentStyle));
        row->mFlavour = LayoutTree::Node::cFlavourRel;
        row->mChildren.push_back(new LayoutTree::Space(5, true));
        // FIX: I would like to make this stretchy and set a particular
        // size, but for some reason firefox doesn't stretch things
        // horizontally like this. It DOES do it if the element is in a
        // <mover> or <munder> etc, but not when it's just by itself.
        // Very strange. This is mozilla bug 320303.
        row->mChildren.push_back(
            new LayoutTree::SymbolOperator(
                false,
                L"",
                false,
                L"\U000021D4",
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBold : cMathmlFontNormal,
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
        row->mChildren.push_back(new LayoutTree::Space(5, true));
        return static_cast<auto_ptr<LayoutTree::Node> >(row);
    }

    if (mCommand == L"\\colon")
    {
        // FIX: this spacing stuff isn't quite right, but it will hopefully
        // do. The amsmath package does all kinds of interesting things with
        // \colon's spacing.
        auto_ptr<LayoutTree::Row> row(new LayoutTree::Row(currentStyle));
        row->mChildren.push_back(new LayoutTree::Space(2, true));
        row->mChildren.push_back(
            new LayoutTree::SymbolOperator(
                false,
                L"",
                false,
                L":",
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBold : cMathmlFontNormal,
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
        row->mChildren.push_back(new LayoutTree::Space(6, true));
        return static_cast<auto_ptr<LayoutTree::Node> >(row);
    }

    if (mCommand == L"\\bmod")
    {
        auto_ptr<LayoutTree::Row> row(new LayoutTree::Row(currentStyle));
        row->mFlavour = LayoutTree::Node::cFlavourBin;
        row->mChildren.push_back(new LayoutTree::Space(1, true));
        row->mChildren.push_back(
            new LayoutTree::SymbolOperator(
                false,
                L"",
                false,
                L"mod",
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBold : cMathmlFontNormal,
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
        row->mChildren.push_back(new LayoutTree::Space(1, true));
        return static_cast<auto_ptr<LayoutTree::Node> >(row);
    }

    if (mCommand == L"\\mod")
    {
        auto_ptr<LayoutTree::Row> row(new LayoutTree::Row(currentStyle));
        row->mChildren.push_back(new LayoutTree::Space(18, true));
        row->mChildren.push_back(
            new LayoutTree::SymbolOperator(
                false,
                L"",
                false,
                L"mod",
                currentFont.mIsBoldsymbol
                    ? cMathmlFontBold : cMathmlFontNormal,
                currentStyle,
                LayoutTree::Node::cFlavourOrd
            )
        );
        row->mChildren.push_back(new LayoutTree::Space(6, true));
        return static_cast<auto_ptr<LayoutTree::Node> >(row);
    }

    if (mCommand == L"\\varinjlim" || mCommand == L"\\varprojlim" ||
        mCommand == L"\\varlimsup" || mCommand == L"\\varliminf")
    {
        MathmlFont font =
            currentFont.mIsBoldsymbol ? cMathmlFontBold : cMathmlFontNormal;

        auto_ptr<LayoutTree::Node> base(
            new LayoutTree::SymbolOperator(
                false,
                L"",
                false,
                L"lim",
                font,
                currentStyle,
                LayoutTree::Node::cFlavourOp,
                LayoutTree::Node::cLimitsLimits
            )
        );

        auto_ptr<LayoutTree::Scripts> node(
            new LayoutTree::Scripts(
                currentStyle,
                LayoutTree::Node::cFlavourOp,
                LayoutTree::Node::cLimitsDisplayLimits,
                false,
                base,
                auto_ptr<LayoutTree::Node>(),
                auto_ptr<LayoutTree::Node>()
            )
        );

        if (mCommand == L"\\varinjlim")
            node->mLower = auto_ptr<LayoutTree::Node>(
                new LayoutTree::SymbolOperator(
                    false,
                    L"",
                    true,
                    L"\U00002192",
                    font,
                    currentStyle,
                    LayoutTree::Node::cFlavourOrd
                )
            );

        else if (mCommand == L"\\varprojlim")
            node->mLower = auto_ptr<LayoutTree::Node>(
                new LayoutTree::SymbolOperator(
                    false,
                    L"",
                    true,
                    L"\U00002190",
                    font,
                    currentStyle,
                    LayoutTree::Node::cFlavourOrd
                )
            );

        else if (mCommand == L"\\varliminf")
            node->mLower = auto_ptr<LayoutTree::Node>(
                new LayoutTree::SymbolOperator(
                    true,
                    L"",
                    true,
                    L"\U000000AF",
                    font,
                    currentStyle,
                    LayoutTree::Node::cFlavourOrd
                )
            );

        else if (mCommand == L"\\varlimsup")
            node->mUpper = auto_ptr<LayoutTree::Node>(
                new LayoutTree::SymbolOperator(
                    true,
                    L"",
                    true,
                    L"\U000000AF",
                    font,
                    currentStyle,
                    LayoutTree::Node::cFlavourOrd
                )
            );

        return static_cast<auto_ptr<LayoutTree::Node> >(node);
    }

    throw logic_error("Unexpected command in MathSymbol::BuildLayoutTree");
}

}
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
