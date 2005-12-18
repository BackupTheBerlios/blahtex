// File "Instance.cpp"
// 
// blahtex (version 0.3.4): a LaTeX to MathML converter designed with MediaWiki in mind
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
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iterator>

using namespace std;

namespace blahtex
{

// The arrays 'gAmsmathCommands', 'gAmsfontsCommands' and 'gAmssymbCommands' list all the latex commands
// which require additional latex packages to be loaded.
// This information is used by Instance::GeneratePurifiedTex'.
wstring gAmsmathCommandsArray[] =
{
    L"\\text",
    L"\\binom",
    L"\\cfrac",
    L"\\begin{matrix}",
    L"\\begin{pmatrix}",
    L"\\begin{bmatrix}",
    L"\\begin{Bmatrix}",
    L"\\begin{vmatrix}",
    L"\\begin{Vmatrix}",
    L"\\begin{cases}",
    L"\\begin{aligned}",
    L"\\begin{smallmatrix}",
    L"\\overleftrightarrow",
    L"\\boldsymbol",
    L"\\And",
    L"\\iint",
    L"\\iiint",
    L"\\iiiint",
    L"\\varinjlim",
    L"\\varprojlim"
    L"\\dotsb",
    L"\\operatorname",
    L"\\operatornamewithlimits",
    L"\\lvert",
    L"\\rvert",
    L"\\lVert",
    L"\\rVert",

    // The following commands are all defined in regular latex, but amsmath redefines them to have
    // slightly different properties:
    //  * The text commands are modified so that the font size does not change if they are used
    //    inside a formula.
    //  * The "\dots" command adjusts the height of the dots depending on the surrounding symbols.
    //  * The "\colon" command gets some spacing adjustments.
    // Therefore for consistency we include 'amsmath' when these commands appear.
    // (FIX: there are probably others that need to be here that I haven't put here yet.)
    L"\\emph",
    L"\\textit",
    L"\\textbf",
    L"\\textrm",
    L"\\texttt",
    L"\\textsf",
    L"\\dots",
    L"\\colon"
};

wstring gAmsfontsCommandsArray[] =
{
    L"\\mathbb",
    L"\\mathfrak"
};

wstring gAmssymbCommandsArray[] =
{
    L"\\varkappa",
    L"\\digamma",
    L"\\beth",
    L"\\gimel",
    L"\\daleth",
    L"\\Finv",
    L"\\Game",
    L"\\upharpoonright",
    L"\\upharpoonleft",
    L"\\downharpoonright",
    L"\\downharpoonleft",
    L"\\nleftarrow",
    L"\\nrightarrow",
    L"\\sqsupset",
    L"\\sqsubset",
    L"\\supsetneq",
    L"\\subsetneq",
    L"\\trianglelefteq",
    L"\\trianglerighteq",
    L"\\Vdash",
    L"\\vDash",
    L"\\lesssim",
    L"\\nless",
    L"\\ngeq",
    L"\\nleq",
    L"\\smallsmile",
    L"\\smallfrown",
    L"\\smallsetminus",
    L"\\varnothing",
    L"\\Diamond",
    L"\\nmid",
    L"\\square",
    L"\\Box",
    L"\\checkmark",
    L"\\complement",
    L"\\eth",
    L"\\hslash",
    L"\\mho"
};

set<wstring> gAmsmathCommands (gAmsmathCommandsArray,  END_ARRAY(gAmsmathCommandsArray));
set<wstring> gAmsfontsCommands(gAmsfontsCommandsArray, END_ARRAY(gAmsfontsCommandsArray));
set<wstring> gAmssymbCommands (gAmssymbCommandsArray,  END_ARRAY(gAmssymbCommandsArray));

wstring Instance::gTexvcCompatibilityMacros =
    // First we have some macros which are not part of tex/latex/amslatex (i.e. amsmath, amsfonts, amssymb
    // packages), but which texvc recognises, so for backward compatibility we define them here too.
    // Most of these are apparently intended to cater for those more familiar with HTML entities.
    L"\\newcommand{\\R}{{\\mathbb R}}"
    L"\\newcommand{\\Reals}{\\R}"
    L"\\newcommand{\\reals}{\\R}"
    L"\\newcommand{\\Z}{{\\mathbb Z}}"
    L"\\newcommand{\\N}{{\\mathbb N}}"
    L"\\newcommand{\\natnums}{\\N}"
    L"\\newcommand{\\Complex}{{\\mathbb C}}"
    L"\\newcommand{\\cnums}{\\Complex}"
    L"\\newcommand{\\alefsym}{\\aleph}"
    L"\\newcommand{\\alef}{\\aleph}"
    L"\\newcommand{\\larr}{\\leftarrow}"
    L"\\newcommand{\\rarr}{\\rightarrow}"
    L"\\newcommand{\\Larr}{\\Leftarrow}"
    L"\\newcommand{\\lArr}{\\Leftarrow}"
    L"\\newcommand{\\Rarr}{\\Rightarrow}"
    L"\\newcommand{\\rArr}{\\Rightarrow}"
    L"\\newcommand{\\uarr}{\\uparrow}"
    L"\\newcommand{\\uArr}{\\Uparrow}"
    L"\\newcommand{\\Uarr}{\\Uparrow}"
    L"\\newcommand{\\darr}{\\downarrow}"
    L"\\newcommand{\\dArr}{\\Downarrow}"
    L"\\newcommand{\\Darr}{\\Downarrow}"
    L"\\newcommand{\\lrarr}{\\leftrightarrow}"
    L"\\newcommand{\\harr}{\\leftrightarrow}"
    L"\\newcommand{\\Lrarr}{\\Leftrightarrow}"
    L"\\newcommand{\\Harr}{\\Leftrightarrow}"
    L"\\newcommand{\\lrArr}{\\Leftrightarrow}"
    L"\\newcommand{\\hAar}{\\Leftrightarrow}" // this one looks like a typo in the texvc source code
    L"\\newcommand{\\sub}{\\subset}"
    L"\\newcommand{\\supe}{\\supseteq}"
    L"\\newcommand{\\sube}{\\subseteq}"
    L"\\newcommand{\\infin}{\\infty}"
    L"\\newcommand{\\lang}{\\langle}"
    L"\\newcommand{\\rang}{\\rangle}"
    L"\\newcommand{\\real}{\\Re}"
    L"\\newcommand{\\image}{\\Im}"
    L"\\newcommand{\\bull}{\\bullet}"
    L"\\newcommand{\\weierp}{\\wp}"
    L"\\newcommand{\\isin}{\\in}"
    L"\\newcommand{\\plusmn}{\\pm}"
    L"\\newcommand{\\Dagger}{\\ddagger}"
    L"\\newcommand{\\exist}{\\exists}"
    L"\\newcommand{\\sect}{\\S}"
    L"\\newcommand{\\clubs}{\\clubsuit}"
    L"\\newcommand{\\spades}{\\spadesuit}"
    L"\\newcommand{\\hearts}{\\heartsuit}"
    L"\\newcommand{\\diamonds}{\\diamondsuit}"
    L"\\newcommand{\\sdot}{\\cdot}"
    L"\\newcommand{\\ang}{\\angle}"
    L"\\newcommand{\\thetasym}{\\theta}"
    L"\\newcommand{\\sgn}{\\operatorname{sgn}}"
    L"\\newcommand{\\Alpha}{A}"
    L"\\newcommand{\\Beta}{B}"
    L"\\newcommand{\\Epsilon}{E}"
    L"\\newcommand{\\Zeta}{Z}"
    L"\\newcommand{\\Eta}{H}"
    L"\\newcommand{\\Iota}{I}"
    L"\\newcommand{\\Kappa}{K}"
    L"\\newcommand{\\Mu}{M}"
    L"\\newcommand{\\Nu}{N}"
    L"\\newcommand{\\Rho}{P}"
    L"\\newcommand{\\Tau}{T}"
    L"\\newcommand{\\Chi}{X}"

    // The commands in this next group are defined in tex+latex+amslatex, but they don't get mapped to
    // what texvc thinks (e.g. "\part" is used in typesetting books to mean a unit somewhat larger than
    // a chapter, like "Part IV").
    // We'll stick to the way texvc does it, especially since wikipedia has quite a number of equations
    // using them.
    L"\\newcommand{\\empty}{\\emptyset}"
    L"\\newcommand{\\and}{\\wedge}"
    L"\\newcommand{\\or}{\\vee}"
    L"\\newcommand{\\part}{\\partial}"
;

wstring Instance::gStandardMacros =
    // The next group are standard TeX synonyms.
    L"\\newcommand{\\|}{\\Vert}"
    L"\\newcommand{\\implies}{\\;\\Longrightarrow\\;}"
    L"\\newcommand{\\neg}{\\lnot}"
    L"\\newcommand{\\ne}{\\neq}"
    L"\\newcommand{\\ge}{\\geq}"
    L"\\newcommand{\\le}{\\leq}"
    L"\\newcommand{\\land}{\\wedge}"
    L"\\newcommand{\\lor}{\\vee}"
    L"\\newcommand{\\gets}{\\leftarrow}"
    L"\\newcommand{\\to}{\\rightarrow}"
    
    // The amsfonts package accepts the following two commands, but warns that they are obsolete, so let's
    // just quietly replace them.
    L"\\newcommand{\\Bbb}{\\mathbb}"
    L"\\newcommand{\\bold}{\\mathbf}"
    
    // The following macros are implemented in (ams)latex in such a way that they get completely
    // surrounded by "safety braces". For example, "x^\frac yz" becomes "x^{y \over z}". Therefore we
    // surround them by safety braces too.
    // The blahtex parser correspondingly recognises the tokens with the "Blahtex" suffix.
    // FIX: re-doc this
    L"\\newcommand{\\mbox}             [1]{{\\hbox{#1}}}"
    L"\\newcommand{\\textReserved}     [1]{{\\text{#1}}}"
    L"\\newcommand{\\textitReserved}   [1]{{\\textit{#1}}}"
    L"\\newcommand{\\textrmReserved}   [1]{{\\textrm{#1}}}"
    L"\\newcommand{\\textbfReserved}   [1]{{\\textbf{#1}}}"
    L"\\newcommand{\\textsfReserved}   [1]{{\\textsf{#1}}}"
    L"\\newcommand{\\textttReserved}   [1]{{\\texttt{#1}}}"
    L"\\newcommand{\\emphReserved}     [1]{{\\emph{#1}}}"
    L"\\newcommand{\\fracReserved}     [2]{{\\frac{#1}{#2}}}"
    L"\\newcommand{\\mathrmReserved}   [1]{{\\mathrm{#1}}}"
    L"\\newcommand{\\mathbfReserved}   [1]{{\\mathbf{#1}}}"
    L"\\newcommand{\\mathbbReserved}   [1]{{\\mathbb{#1}}}"
    L"\\newcommand{\\mathitReserved}   [1]{{\\mathit{#1}}}"
    L"\\newcommand{\\mathcalReserved}  [1]{{\\mathcal{#1}}}"
    L"\\newcommand{\\mathfrakReserved} [1]{{\\mathfrak{#1}}}"
    L"\\newcommand{\\mathttReserved}   [1]{{\\mathtt{#1}}}"
    L"\\newcommand{\\mathsfReserved}   [1]{{\\mathsf{#1}}}"
    L"\\newcommand{\\bigReserved}      [1]{{\\big#1}}"
    L"\\newcommand{\\biggReserved}     [1]{{\\bigg#1}}"
    L"\\newcommand{\\BigReserved}      [1]{{\\Big#1}}"
    L"\\newcommand{\\BiggReserved}     [1]{{\\Bigg#1}}"
;

vector<wstring> Instance::gStandardMacrosTokenised;
vector<wstring> Instance::gTexvcCompatibilityMacrosTokenised;

Instance::Instance()
{
    if (gTexvcCompatibilityMacrosTokenised.empty())
        Tokenise(gTexvcCompatibilityMacros, gTexvcCompatibilityMacrosTokenised);

    if (gStandardMacrosTokenised.empty())
        Tokenise(gStandardMacros, gStandardMacrosTokenised);

    mStrictSpacingRequested = false;
}

bool IsAlphabetic(wchar_t c) 
{
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
}

void Instance::Tokenise(const wstring& input, vector<wstring>& output)
{
    wstring::const_iterator ptr = input.begin();

    while (ptr != input.end())
    {
        // merge adjacent whitespace
        if (iswspace(*ptr))
        {
            output.push_back(L" ");
            do
                ptr++;
            while (ptr != input.end() && iswspace(*ptr));
        }
        // boring single character tokens
        else if (*ptr != L'\\')
        {
            // FIX: test this:
            // Disallow non-printable, non-whitespace ASCII
            if (*ptr < L' ')
                throw Exception(Exception::cIllegalCharacter);
            output.push_back(wstring(1, *ptr++));
        }
        else
        {
            // tokens starting with backslash
            wstring token = L"\\";
            
            if (++ptr == input.end())
                throw Exception(Exception::cIllegalFinalBackslash);
            if (IsAlphabetic(*ptr))
            {
                // plain alphabetic commands
                do
                    token += *ptr++;
                while (ptr != input.end() && IsAlphabetic(*ptr));
                
                // Special treatment for "\begin" and "\end"; need to collapse "\begin  {xyz}" to
                // "\begin{xyz}", and store it as a single token.
                if (token == L"\\begin" || token == L"\\end")
                {
                    while (ptr != input.end() && iswspace(*ptr))
                        ptr++;
                    if (ptr == input.end() || *ptr != L'{')
                        throw Exception(Exception::cMissingOpenBraceAfter, token);
                    token += *ptr++;
                    while (ptr != input.end() && *ptr != L'}')
                        token += *ptr++;
                    if (ptr == input.end())
                        throw Exception(Exception::cUnmatchedOpenBrace);
                    token += *ptr++;
                }
            }
            else if (iswspace(*ptr))
            {
                // commands like "\    "
                token += L" ";
                do
                    ptr++;
                while (ptr != input.end() && iswspace(*ptr));
            }
            // commands like "\, and "\;"
            else
                token += *ptr++;
            
            output.push_back(token);
        }
    }
}

void Instance::ProcessInput(const wstring& input, bool texvcCompatibility)
{
    static wstring reservedCommandArray[] =
    {
        L"\\sqrt",
        L"\\text",
        L"\\textit",
        L"\\textrm",
        L"\\textbf",
        L"\\textsf",
        L"\\texttt",
        L"\\emph",
        L"\\frac",
        L"\\mathrm",
        L"\\mathbf",
        L"\\mathbb",
        L"\\mathit",
        L"\\mathcal",
        L"\\mathfrak",
        L"\\mathtt",
        L"\\mathsf",
        L"\\big",
        L"\\bigg",
        L"\\Big",
        L"\\Bigg"
    };
    static wishful_hash_set<wstring> reservedCommandTable(reservedCommandArray, END_ARRAY(reservedCommandArray));

    vector<wstring> inputTokens;
    Tokenise(input, inputTokens);

    // Check that the user hasn't supplied any input directly containing the "Blahtex" suffix,
    // and add Blahtex suffix to certain selected commands, and search for magic commands
    // (currently the only magic command is "\strictspacing")
    for (vector<wstring>::iterator ptr = inputTokens.begin(); ptr != inputTokens.end(); ptr++)
    {
        if (reservedCommandTable.count(*ptr))
            *ptr += L"Reserved";
        else if (ptr->size() >= 8 && ptr->substr(ptr->size() - 8, 8) == L"Reserved")
            throw Exception(Exception::cReservedCommand, *ptr);
        else if (*ptr == L"\\strictspacing")
        {
            mStrictSpacingRequested = true;
            *ptr = L" ";
        }
    }
    
    vector<wstring> tokens;
    
    if (texvcCompatibility)
        tokens = gTexvcCompatibilityMacrosTokenised;

    copy(gStandardMacrosTokenised.begin(), gStandardMacrosTokenised.end(), back_inserter(tokens));
    copy(inputTokens.begin(), inputTokens.end(), back_inserter(tokens));

    Parser P;
    mParseTree = P.DoParse(tokens);
    mLayoutTree = mParseTree->BuildLayoutTree(LatexMathFont(), cStyleText);
}

struct Version1FontInfo
{
    wstring mFamily;
    bool mIsItalic;
    bool mIsBold;
    
    Version1FontInfo(const wstring& family, bool isItalic, bool isBold) :
        mFamily(family), mIsItalic(isItalic), mIsBold(isBold) { }
};

// FIX: doc this
// remove extraneous ones and replace by version 1 fonts if required
void CleanupFontAttributes(XmlNode* node, bool mathmlVersion1Fonts)
{
    static pair<wstring, Version1FontInfo> version1Array[] =
    {
        make_pair(L"normal",                    Version1FontInfo(L"",           false, false)),
        make_pair(L"bold",                      Version1FontInfo(L"",           false, true )),
        make_pair(L"italic",                    Version1FontInfo(L"",           true,  false)),
        make_pair(L"bold-italic",               Version1FontInfo(L"",           true,  true )),
        make_pair(L"sans-serif",                Version1FontInfo(L"sans-serif", false, false)),
        make_pair(L"bold-sans-serif",           Version1FontInfo(L"sans-serif", false, true )),
        make_pair(L"sans-serif-italic",         Version1FontInfo(L"sans-serif", true,  false)),
        make_pair(L"sans-serif-bold-italic",    Version1FontInfo(L"sans-serif", true,  true )),
        make_pair(L"monospace",                 Version1FontInfo(L"monospace",  false, false))
    };
    static wishful_hash_map<wstring, Version1FontInfo> version1Table(version1Array, END_ARRAY(version1Array));
    
    if (node->mType == XmlNode::cTag)
    {
        map<wstring, wstring>::iterator search = node->mAttributes.find(L"mathvariant");
        if (search != node->mAttributes.end())
        {
            if (search->second == L"")
                node->mAttributes.erase(search);
            else
            {
                MathmlFont defaultMathmlFont;
                bool defaultItalic;
                
                if (node->mText == L"mi" && node->mChildren.front()->mText.size() == 1)
                {
                    defaultMathmlFont = cMathmlFontItalic;
                    defaultItalic = true;
                }
                else
                {
                    defaultMathmlFont = cMathmlFontNormal;
                    defaultItalic = false;
                }
                
                if (mathmlVersion1Fonts)
                {
                    wishful_hash_map<wstring, Version1FontInfo>::const_iterator lookup = version1Table.find(search->second);
                    if (lookup == version1Table.end())
                        throw logic_error("Unexpected mathvariant value in CleanupFontAttributes");
                
                    node->mAttributes.erase(search);
                    if (!lookup->second.mFamily.empty())
                        node->mAttributes[L"fontfamily"] = lookup->second.mFamily;
                    if (lookup->second.mIsItalic != defaultItalic)
                        node->mAttributes[L"fontstyle"] = lookup->second.mIsItalic ? L"italic" : L"normal";
                    if (lookup->second.mIsBold)
                        node->mAttributes[L"fontweight"] = L"bold";
                }
                else
                {
                    if (gMathmlFontStrings[defaultMathmlFont] == search->second)
                        node->mAttributes.erase(search);
                }
            }
        }
        
        for (list<XmlNode*>::iterator child = node->mChildren.begin(); child != node->mChildren.end(); child++)
            CleanupFontAttributes(*child, mathmlVersion1Fonts);
    }
}

auto_ptr<XmlNode> Instance::GenerateMathml(const MathmlOptions& options)
{
    if (!mLayoutTree.get())
        throw logic_error("Layout tree not yet built in Instance::GenerateMathml");
    
    MathmlOptions optionsCopy = options;
    if (mStrictSpacingRequested)
        optionsCopy.mSpacingControl = cSpacingControlStrict;
    
    auto_ptr<XmlNode> root = mLayoutTree->BuildMathmlTree(optionsCopy, cStyleText);
    CleanupFontAttributes(root.get(), options.mMathmlVersion1Fonts);

    return root;
}

auto_ptr<XmlNode> Instance::GenerateHtml()
{
    if (!mLayoutTree.get())
        throw logic_error("Layout tree not yet built in Instance::GenerateHtml");
        
    return auto_ptr<XmlNode>(new XmlNode(XmlNode::cTag, L"html"));
}

wstring Instance::GeneratePurifiedTex(const PurifiedTexOptions& options)
{
    if (!mParseTree.get())
        throw logic_error("Parse tree not yet built in Instance::GeneratePurifiedTex");

    wostringstream os;
    mParseTree->GetPurifiedTex(os, options);
    wstring latex = os.str();
    
    // Work out which commands appeared in the purified TeX output, so we can work out which packages
    // need to be included.
    
    vector<wstring> tokens;
    Tokenise(latex, tokens);

    set<wstring> commands;
    for (vector<wstring>::iterator p = tokens.begin(); p != tokens.end(); p++)
        if (!p->empty() && (*p)[0] == L'\\')
            commands.insert(*p);
    
    wstring output = 
        L"\\nonstopmode\n"
        L"\\documentclass[12pt]{article}\n";

    if (!disjoint(commands, gAmsmathCommands))
        output += L"\\usepackage{amsmath}\n";
    if (!disjoint(commands, gAmsfontsCommands))
        output += L"\\usepackage{amsfonts}\n";
    if (!disjoint(commands, gAmssymbCommands))
        output += L"\\usepackage{amssymb}\n";
    if (commands.count(L"\\unichar"))
        output += L"\\usepackage{ucs}\n";
        
    output +=
        L"\\pagestyle{empty}\n"
        L"\\begin{document}\n"        
        L"$\n";

    output += latex;
    
    output += 
        L"\n$\n"
        L"\\end{document}\n";

    return output;
}
    
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
