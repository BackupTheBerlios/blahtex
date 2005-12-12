// File "Instance.cpp"
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
#include <sstream>
#include <stdexcept>

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

wstring Instance::gStandardMacros =
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
    L"\\newcommand{\\mbox}[1]{{\\hbox{#1}}}"
    L"\\newcommand{\\text}[1]{{\\textBlahtex{#1}}}"
    L"\\newcommand{\\textit}[1]{{\\textitBlahtex{#1}}}"
    L"\\newcommand{\\textrm}[1]{{\\textrmBlahtex{#1}}}"
    L"\\newcommand{\\textbf}[1]{{\\textbfBlahtex{#1}}}"
    L"\\newcommand{\\textsf}[1]{{\\textsfBlahtex{#1}}}"
    L"\\newcommand{\\texttt}[1]{{\\textttBlahtex{#1}}}"
    L"\\newcommand{\\emph}[1]{{\\emphBlahtex{#1}}}"
    L"\\newcommand{\\frac}[2]{{\\fracBlahtex{#1}{#2}}}"
    L"\\newcommand{\\mathrm}[1]{{\\mathrmBlahtex{#1}}}"
    L"\\newcommand{\\mathbf}[1]{{\\mathbfBlahtex{#1}}}"
    L"\\newcommand{\\mathbb}[1]{{\\mathbbBlahtex{#1}}}"
    L"\\newcommand{\\mathit}[1]{{\\mathitBlahtex{#1}}}"
    L"\\newcommand{\\mathcal}[1]{{\\mathcalBlahtex{#1}}}"
    L"\\newcommand{\\mathfrak}[1]{{\\mathfrakBlahtex{#1}}}"
    L"\\newcommand{\\mathtt}[1]{{\\mathttBlahtex{#1}}}"
    L"\\newcommand{\\mathsf}[1]{{\\mathsfBlahtex{#1}}}"
    L"\\newcommand{\\big}[1]{{\\bigBlahtex#1}}"
    L"\\newcommand{\\bigg}[1]{{\\biggBlahtex#1}}"
    L"\\newcommand{\\Big}[1]{{\\BigBlahtex#1}}"
    L"\\newcommand{\\Bigg}[1]{{\\BiggBlahtex#1}}"
;

vector<wstring> Instance::gStandardMacrosTokenised;

Instance::Instance()
{
    if (gStandardMacrosTokenised.empty())
        Tokenise(gStandardMacros, gStandardMacrosTokenised);
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

void Instance::ProcessInput(const wstring& input)
{
    vector<wstring> tokens = gStandardMacrosTokenised;
    Tokenise(input, tokens);
    
    // Check that the user hasn't supplied any input directly containing the "Blahtex" suffix
    for (vector<wstring>::const_iterator ptr = tokens.begin() + gStandardMacrosTokenised.size();
        ptr != tokens.end(); ptr++)
    {
        if (ptr->size() >= 7 && ptr->substr(ptr->size() - 7, 7) == L"Blahtex")
            throw Exception(Exception::cIllegalBlahtexSuffix);
    }

    Parser P;
    mParseTree = P.DoParse(tokens);
    mLayoutTree = mParseTree->BuildLayoutTree(LatexMathFont(), cStyleText);
}

// This function recursively traverses a MathML tree, and removes "extraneous" font attributes.
// The following are examples of mathvariant tags that would get removed:
//    <mi mathvariant="italic"> x </mi>
//    <mi mathvariant="normal"> sin </mi>
//    <mn mathvariant="normal> 1 </mn>
// In each case, the mathvariant value is the MathML default so doesn't need to be specified.
// It wouldn't hurt for it to be there, except that we don't want output that is too bloated.
// The reason we do this so late in the processing is that it makes BuildMathmlTree() (earlier in the
// codepath) much simpler.
void RemoveExtraneousFontAttributes(XmlNode* node)
{
    if (node->mType == XmlNode::cTag)
    {
        map<wstring, wstring>::iterator search = node->mAttributes.find(L"mathvariant");
        if (search != node->mAttributes.end())
        {
            if (search->second == L"")
                node->mAttributes.erase(search);
            else if (node->mText == L"mi")
            {
                int count = node->mChildren.front()->mText.size();
                if ((count == 1 && search->second == L"italic") || (count > 1 && search->second == L"normal"))
                    node->mAttributes.erase(search);
            }
            else if (node->mText == L"mn" || node->mText == L"mtext")
            {
                if (search->second == L"normal")
                    node->mAttributes.erase(search);
            }
        }
    
        for (list<XmlNode*>::iterator child = node->mChildren.begin(); child != node->mChildren.end(); child++)
            RemoveExtraneousFontAttributes(*child);
    }
}

auto_ptr<XmlNode> Instance::GenerateMathml(const MathmlOptions& options)
{
    // FIX: Check the layout tree has actually been generated
    auto_ptr<XmlNode> root = mLayoutTree->BuildMathmlTree(options, cStyleText);
    RemoveExtraneousFontAttributes(root.get());
    return root;
}

auto_ptr<XmlNode> Instance::GenerateHtml()
{
    // FIX: Check the layout tree has actually been generated
    return auto_ptr<XmlNode>(new XmlNode(XmlNode::cTag, L"html"));
}

wstring Instance::GeneratePurifiedTex()
{
    // Check the parse tree has actually been generated
    if (!mParseTree.get())
        throw logic_error("Parse tree not built yet in call to Instance::GeneratePurifiedTex");

    wostringstream os;
    mParseTree->GetPurifiedTex(os);
    wstring latex = os.str();
    
    // Work out which commands appeared in the purified latex output, so we can work out which packages
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
        
    output +=
        L"\\pagestyle{empty}\n"
        L"\\begin{document}\n"        
        L"$$\n";

    output += latex;
    
    output += 
        L"\n$$\n"
        L"\\end{document}\n";

    return output;
}
    
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
