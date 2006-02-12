// File "Manager.cpp"
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

#include <sstream>
#include <stdexcept>
#include "Manager.h"
#include "Parser.h"

using namespace std;

namespace blahtex
{

// String versions of the MathML mathvariant fonts.
// (See enum MathmlFont in LayoutTree.h.)
extern std::wstring gMathmlFontStrings[];

// This function tests whether two STL sets are disjoint, assuming T has a
// total ordering.
// (You'd think the STL would have this, but I couldn't find it.)
template<typename T> bool disjoint(
    const std::set<T>& x,
    const std::set<T>& y
) {
    if (x.empty() || y.empty())
        return true;

    typename std::set<T>::const_iterator p = x.begin(), q = y.begin();
    while (true)
    {
        if (*p < *q)
        {
            if (++p == x.end())
                return true;
        }
        else if (*q < *p)
        {
            if (++q == y.end())
                return true;
        }
        else
            return false;
    }
}

// I don't entirely trust the wide versions of isalpha etc, so this
// function does the job instead.
bool IsAlphabetic(wchar_t c)
{
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
}


// Tokenise() splits the given input into tokens, each represented by a
// string. The output is APPENDED to "output".
//
// There are several types of tokens:
// * single characters like "a", or "{", or single non-ASCII unicode
//   characters
// * alphabetic commands like "\frac"
// * commands like "\," which have a single nonalphabetic character
//   after the backslash
// * commands like "\   " which have their whitespace collapsed,
//   stored as "\ "
// * other consecutive whitespace characters which get collapsed to
//   just " "
// * the sequence "\begin   {  stuff  }" gets stored as the single token
//   "\begin{  stuff  }". Note that whitespace is preserved between the
//   braces but not between "\begin" and "{". Similarly for "\end".
void Tokenise(const wstring& input, vector<wstring>& output)
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
            // Disallow non-printable, non-whitespace ASCII
            if (*ptr < L' ' || *ptr == 0x7F)
                throw Exception(L"IllegalCharacter");
            output.push_back(wstring(1, *ptr++));
        }
        else
        {
            // tokens starting with backslash
            wstring token = L"\\";

            if (++ptr == input.end())
                throw Exception(L"IllegalFinalBackslash");
            if (IsAlphabetic(*ptr))
            {
                // plain alphabetic commands
                do
                    token += *ptr++;
                while (ptr != input.end() && IsAlphabetic(*ptr));

                // Special treatment for "\begin" and "\end"; need to
                // collapse "\begin  {xyz}" to "\begin{xyz}", and store it
                // as a single token.
                if (token == L"\\begin" || token == L"\\end")
                {
                    while (ptr != input.end() && iswspace(*ptr))
                        ptr++;
                    if (ptr == input.end() || *ptr != L'{')
                        throw Exception(L"MissingOpenBraceAfter", token);
                    token += *ptr++;
                    while (ptr != input.end() && *ptr != L'}')
                        token += *ptr++;
                    if (ptr == input.end())
                        throw Exception(L"UnmatchedOpenBrace");
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
            // commands like "\," and "\;"
            else
                token += *ptr++;

            output.push_back(token);
        }
    }
}


// The arrays gAmsmathCommands, gAmsfontsCommands and gAmssymbCommands list
// all the commands which require additional latex packages to be loaded.
// This information is used by Manager::GeneratePurifiedTex.

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
    L"\\varlimsup",
    L"\\varliminf",
    L"\\varinjlim",
    L"\\varprojlim",
    L"\\injlim",
    L"\\projlim",
    L"\\dotsb",
    L"\\operatorname",
    L"\\operatornamewithlimits",
    L"\\lvert",
    L"\\rvert",
    L"\\lVert",
    L"\\rVert",
    L"\\substack",
    L"\\overset",
    L"\\underset",
    L"\\mod",

    // The following commands are all defined in regular latex, but amsmath
    // redefines them to have slightly different properties:
    //
    //  * The text commands are modified so that the font size does not
    //    change if they are used inside a formula.
    //  * The "\dots" command adjusts the height of the dots depending on
    //    the surrounding symbols.
    //  * The "\colon" command gets some spacing adjustments.
    //
    // Therefore for consistency we include amsmath when these commands
    // appear.
    //
    // (FIX: there are probably others that need to be here that I haven't
    // put here yet.)
    L"\\emph",
    L"\\textit",
    L"\\textbf",
    L"\\textrm",
    L"\\texttt",
    L"\\textsf",
    L"\\dots",
    L"\\dotsb",
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
    L"\\nmid",
    L"\\square",
    L"\\Box",
    L"\\checkmark",
    L"\\complement",
    L"\\eth",
    L"\\hslash",
    L"\\mho",
    L"\\circledR",
    L"\\yen",
    L"\\maltese",
    L"\\ulcorner",
    L"\\urcorner",
    L"\\llcorner",
    L"\\lrcorner",
    L"\\dashrightarrow",
    L"\\dasharrow",
    L"\\dashleftarrow",
    L"\\backprime",
    L"\\vartriangle",
    L"\\blacktriangle",
    L"\\triangledown",
    L"\\blacktriangledown",
    L"\\blacksquare",
    L"\\lozenge",
    L"\\blacklozenge",
    L"\\circledS",
    L"\\bigstar",
    L"\\sphericalangle",
    L"\\measuredangle",
    L"\\diagup",
    L"\\diagdown",
    L"\\Bbbk",
    L"\\dotplus",
    L"\\ltimes",
    L"\\rtimes",
    L"\\Cap",
    L"\\leftthreetimes",
    L"\\rightthreetimes",
    L"\\Cup",
    L"\\barwedge",
    L"\\curlywedge",
    L"\\veebar",
    L"\\curlyvee",
    L"\\doublebarwedge",
    L"\\boxminus",
    L"\\circleddash",
    L"\\boxtimes",
    L"\\circledast",
    L"\\boxdot",
    L"\\circledcirc",
    L"\\boxplus",
    L"\\centerdot",
    L"\\divideontimes",
    L"\\intercal",
    L"\\leqq",
    L"\\geqq",
    L"\\leqslant",
    L"\\geqslant",
    L"\\eqslantless",
    L"\\eqslantgtr",
    L"\\gtrsim",
    L"\\lessapprox",
    L"\\gtrapprox",
    L"\\approxeq",
    L"\\eqsim",
    L"\\lessdot",
    L"\\gtrdot",
    L"\\lll",
    L"\\ggg",
    L"\\lessgtr",
    L"\\gtrless",
    L"\\lesseqgtr",
    L"\\gtreqless",
    L"\\lesseqqgtr",
    L"\\gtreqqless",
    L"\\doteqdot",
    L"\\eqcirc",
    L"\\risingdotseq",
    L"\\circeq",
    L"\\fallingdotseq",
    L"\\triangleq",
    L"\\backsim",
    L"\\thicksim",
    L"\\backsimeq",
    L"\\thickapprox",
    L"\\subseteqq",
    L"\\supseteqq",
    L"\\Subset",
    L"\\Supset",
    L"\\preccurlyeq",
    L"\\succcurlyeq",
    L"\\curlyeqprec",
    L"\\curlyeqsucc",
    L"\\precsim",
    L"\\succsim",
    L"\\precapprox",
    L"\\succapprox",
    L"\\vartriangleleft",
    L"\\vartriangleright",
    L"\\Vvdash",
    L"\\shortmid",
    L"\\shortparallel",
    L"\\bumpeq",
    L"\\between",
    L"\\Bumpeq",
    L"\\varpropto",
    L"\\backepsilon",
    L"\\blacktriangleleft",
    L"\\blacktriangleright",
    L"\\therefore",
    L"\\because",
    L"\\ngtr",
    L"\\nleqslant",
    L"\\ngeqslant",
    L"\\nleqq",
    L"\\ngeqq",
    L"\\lneqq",
    L"\\gneqq",
    L"\\lvertneqq",
    L"\\gvertneqq",
    L"\\lnsim",
    L"\\gnsim",
    L"\\lnapprox",
    L"\\gnapprox",
    L"\\nprec",
    L"\\nsucc",
    L"\\npreceq",
    L"\\nsucceq",
    L"\\precneqq",
    L"\\succneqq",
    L"\\precnsim",
    L"\\succnsim",
    L"\\precnapprox",
    L"\\succnapprox",
    L"\\nsim",
    L"\\ncong",
    L"\\nshortmid",
    L"\\nshortparallel",
    L"\\nmid",
    L"\\nparallel",
    L"\\nvdash",
    L"\\nvDash",
    L"\\nVdash",
    L"\\nVDash",
    L"\\ntriangleleft",
    L"\\ntriangleright",
    L"\\ntrianglelefteq",
    L"\\ntrianglerighteq",
    L"\\nsubseteq",
    L"\\nsupseteq",
    L"\\nsubseteqq",
    L"\\nsupseteqq",
    L"\\subsetneq",
    L"\\supsetneq",
    L"\\varsubsetneq",
    L"\\varsupsetneq",
    L"\\subsetneqq",
    L"\\supsetneqq",
    L"\\varsubsetneqq",
    L"\\varsupsetneqq",
    L"\\leftleftarrows",
    L"\\rightrightarrows",
    L"\\leftrightarrows",
    L"\\rightleftarrows",
    L"\\Lleftarrow",
    L"\\Rrightarrow",
    L"\\twoheadleftarrow",
    L"\\twoheadrightarrow",
    L"\\leftarrowtail",
    L"\\rightarrowtail",
    L"\\looparrowleft",
    L"\\looparrowright",
    L"\\leftrightharpoons",
    L"\\rightleftharpoons",
    L"\\curvearrowleft",
    L"\\curvearrowright",
    L"\\circlearrowleft",
    L"\\circlearrowright",
    L"\\Lsh",
    L"\\Rsh",
    L"\\upuparrows",
    L"\\downdownarrows",
    L"\\multimap",
    L"\\rightsquigarrow",
    L"\\leftrightsquigarrow",
    L"\\nLeftarrow",
    L"\\nRightarrow",
    L"\\nleftrightarrow",
    L"\\nLeftrightarrow",
    L"\\pitchfork",
    L"\\nexists",
    L"\\lhd",
    L"\\rhd",
    L"\\unlhd",
    L"\\unrhd",
    L"\\Join",
    L"\\leadsto"    
};

set<wstring> gAmsmathCommands(
    gAmsmathCommandsArray,
    END_ARRAY(gAmsmathCommandsArray)
);

set<wstring> gAmsfontsCommands(
    gAmsfontsCommandsArray,
    END_ARRAY(gAmsfontsCommandsArray)
);

set<wstring> gAmssymbCommands(
    gAmssymbCommandsArray,
    END_ARRAY(gAmssymbCommandsArray)
);

wstring Manager::gTexvcCompatibilityMacros =

    // First we have some macros which are not part of tex/latex/amslatex
    // but which texvc recognises, so for backward compatibility we define
    // them here too. Most of these are apparently intended to cater for
    // those more familiar with HTML entities.

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
    // The next one looks like a typo in the texvc source code:
    L"\\newcommand{\\hAar}{\\Leftrightarrow}"
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
    L"\\newcommand{\\arccot}{\\operatorname{arccot}}"
    L"\\newcommand{\\arcsec}{\\operatorname{arcsec}}"
    L"\\newcommand{\\arccsc}{\\operatorname{arccsc}}"
    L"\\newcommand{\\sgn}{\\operatorname{sgn}}"

    // The commands in this next group are defined in tex/latex/amslatex,
    // but they don't get mapped to what texvc thinks (e.g. "\part" is used
    // in typesetting books to mean a unit somewhat larger than a chapter,
    // like "Part IV").
    //
    // We'll stick to the way texvc does it, especially since wikipedia has
    // quite a number of equations using them.
    L"\\newcommand{\\empty}{\\emptyset}"
    L"\\newcommand{\\and}{\\wedge}"
    L"\\newcommand{\\or}{\\vee}"
    L"\\newcommand{\\part}{\\partial}"
;

wstring Manager::gStandardMacros =

    // The next group are standard TeX/LaTeX/AMS-LaTeX synonyms.
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
    L"\\newcommand{\\doublecap}{\\Cap}"
    L"\\newcommand{\\restriction}{\\upharpoonright}"
    L"\\newcommand{\\llless}{\\lll}"
    L"\\newcommand{\\gggtr}{\\ggg}"
    L"\\newcommand{\\Doteq}{\\doteqdot}"
    L"\\newcommand{\\doublecup}{\\Cup}"
    L"\\newcommand{\\dasharrow}{\\dashleftarrow}"
    L"\\newcommand{\\vartriangleleft}{\\lhd}"
    L"\\newcommand{\\vartriangleright}{\\rhd}"
    L"\\newcommand{\\trianglelefteq}{\\unlhd}"
    L"\\newcommand{\\trianglerighteq}{\\unrhd}"
    L"\\newcommand{\\Join}{\\bowtie}"
    L"\\newcommand{\\Diamond}{\\lozenge}"

    // The amsfonts package accepts the following two commands, but warns
    // that they are obsolete, so let's just quietly replace them.
    L"\\newcommand{\\Bbb}{\\mathbb}"
    L"\\newcommand{\\bold}{\\mathbf}"

    // Now we come to the xxxReserved commands. These are all implemented
    // as macros in TeX, so for maximum compatibility, we want to treat
    // their arguments the way a TeX macro does. The strategy is the
    // following. First, in Manager::ProcessInput, we convert e.g. "\mbox"
    // into "\mboxReserved". Then, the MacroProcessor object sees e.g.
    // "\mboxReserved A" and converts it to "\mbox{A}". This simplifies
    // things enormously for the parser, since now it can treat "\mbox"
    // and "\hbox" in the same way. ("\hbox" requires braces around its
    // argument, even if it's just a single character.) This strategy also
    // keeps TeX happy when we send off the purified TeX, since TeX doesn't
    // care about the extra braces.

    L"\\newcommand{\\mboxReserved}     [1]{\\mbox{#1}}"
    L"\\newcommand{\\substackReserved} [1]{\\substack{#1}}"
    L"\\newcommand{\\oversetReserved}  [2]{\\overset{#1}{#2}}"
    L"\\newcommand{\\undersetReserved} [2]{\\underset{#1}{#2}}"

    // The following are all similar, but they get extra "safety braces"
    // placed around them. For example, "x^\frac yz" is legal, because it
    // becomes "x^{y \over z}".

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

vector<wstring> Manager::gStandardMacrosTokenised;
vector<wstring> Manager::gTexvcCompatibilityMacrosTokenised;

Manager::Manager()
{
    // Tokenise the standard macros if it hasn't been done already.

    if (gTexvcCompatibilityMacrosTokenised.empty())
        Tokenise(
            gTexvcCompatibilityMacros,
            gTexvcCompatibilityMacrosTokenised
        );

    if (gStandardMacrosTokenised.empty())
        Tokenise(gStandardMacros, gStandardMacrosTokenised);

    mStrictSpacingRequested = false;
}

void Manager::ProcessInput(const wstring& input, bool texvcCompatibility)
{
    // Here are all the commands which get "Reserved" tacked on the end
    // before the MacroProcessor sees them:

    static wstring reservedCommandArray[] =
    {
        L"\\sqrt",
        L"\\mbox",
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
        L"\\Bigg",
        L"\\overset",
        L"\\underset"
        L"\\substack"
    };
    static wishful_hash_set<wstring> reservedCommandTable(
        reservedCommandArray,
        END_ARRAY(reservedCommandArray)
    );

    vector<wstring> inputTokens;
    Tokenise(input, inputTokens);

    mStrictSpacingRequested = false;

    // Check that the user hasn't supplied any input directly containing the
    // "Reserved" suffix, and add Reserved suffixes appropriately.
    //
    // Also search for magic commands (currently the only magic command is
    // "\strictspacing")
    for (vector<wstring>::iterator
        ptr = inputTokens.begin();
        ptr != inputTokens.end();
        ptr++
    )
    {
        if (reservedCommandTable.count(*ptr))
            *ptr += L"Reserved";

        else if (
            ptr->size() >= 8 &&
            ptr->substr(ptr->size() - 8, 8) == L"Reserved"
        )
            throw Exception(L"ReservedCommand", *ptr);

        else if (*ptr == L"\\strictspacing")
        {
            mStrictSpacingRequested = true;
            *ptr = L" ";
        }
    }

    vector<wstring> tokens;

    // Append the texvc-compatibility and standard macros where appropriate.

    if (texvcCompatibility)
        tokens = gTexvcCompatibilityMacrosTokenised;

    copy(
        gStandardMacrosTokenised.begin(),
        gStandardMacrosTokenised.end(),
        back_inserter(tokens)
    );
    copy(inputTokens.begin(), inputTokens.end(), back_inserter(tokens));

    // Generate the parse tree and the layout tree.
    Parser P;
    mParseTree = P.DoParse(tokens);
    mHasDelayedMathmlError = false;
    
    try
    {
        mLayoutTree = mParseTree->BuildLayoutTree(
            TexMathFont(),
            LayoutTree::Node::cStyleText
        );
    }
    catch (Exception& e)
    {
        // Some types of error need to returned as MathML errors, not
        // parsing errors.
        if (
            e.GetCode() == L"InvalidNegation" ||
            e.GetCode() == L"UnavailableSymbolFontCombination"
        )
        {
            mHasDelayedMathmlError = true;
            mDelayedMathmlError = e;
            mLayoutTree.reset(NULL);
        }
        else
            throw e;
    }
}

// This is a helper struct used in CleanupFontAttributes (see below).
struct Version1FontInfo
{
    wstring mFamily;
    bool mIsItalic;
    bool mIsBold;

    Version1FontInfo(
        const wstring& family,
        bool isItalic,
        bool isBold
    ) :
        mFamily(family),
        mIsItalic(isItalic),
        mIsBold(isBold)
    { }
};

// This function walks an XML (MathML) tree, doing two things:
//
// (1) Removes extraneous font attributes. For example, if it sees something
//     like
//           <mi mathvariant="italic">X</mi>
//     then the mathvariant="italic" is redundant, so it gets removed.
//
// (2) Converts MathML version 2 font attributes to version 1 attributes
//     (if the mathmlVersion1FontAttributes flag is set).
//
//     Rationale: it's much easier to just insert all the mathvariant
//     attributes without thinking about MathML defaults or about MathML
//     versions while we're actually building the MathML tree, and then
//     coming back to fix it all up later. It's a bit inefficient, but hey.

void CleanupFontAttributes(XmlNode* node, bool mathmlVersion1FontAttributes)
{
    // This array describes how to translate each mathvariant setting
    // into MathML version 1 attributes.

    static pair<wstring, Version1FontInfo> version1Array[] =
    {
        make_pair(L"normal",
            Version1FontInfo(L"", false, false)),

        make_pair(L"bold",
            Version1FontInfo(L"", false, true)),

        make_pair(L"italic",
            Version1FontInfo(L"", true,  false)),

        make_pair(L"bold-italic",
            Version1FontInfo(L"", true,  true)),

        make_pair(L"sans-serif",
            Version1FontInfo(L"sans-serif", false, false)),

        make_pair(L"bold-sans-serif",
            Version1FontInfo(L"sans-serif", false, true)),

        make_pair(L"sans-serif-italic",
            Version1FontInfo(L"sans-serif", true,  false)),

        make_pair(L"sans-serif-bold-italic",
            Version1FontInfo(L"sans-serif", true,  true)),

        make_pair(L"monospace",
            Version1FontInfo(L"monospace", false, false))
    };

    static wishful_hash_map<wstring, Version1FontInfo> version1Table(
        version1Array,
        END_ARRAY(version1Array)
    );

    if (node->mType == XmlNode::cTag)
    {
        map<wstring, wstring>::iterator
            search = node->mAttributes.find(L"mathvariant");
        if (search != node->mAttributes.end())
        {
            if (search->second == L"")
                node->mAttributes.erase(search);
            else
            {
                MathmlFont defaultMathmlFont;
                bool defaultItalic;

                // Work out what the default mathvariant and fontstyle
                // settings would be for this node.
                if (node->mText == L"mi"
                    && node->mChildren.front()->mText.size() == 1
                )
                {
                    defaultMathmlFont = cMathmlFontItalic;
                    defaultItalic = true;
                }
                else
                {
                    defaultMathmlFont = cMathmlFontNormal;
                    defaultItalic = false;
                }

                if (mathmlVersion1FontAttributes)
                {
                    wishful_hash_map
                        <wstring, Version1FontInfo>::const_iterator
                        lookup = version1Table.find(search->second);

                    if (lookup == version1Table.end())
                    {
                        // FIX: the only time we might end up here is when
                        // we have a fraktur digit. TeX has decent fraktur
                        // digits, but unicode doesn't seem to list them.
                        // Therefore we can't access them with version 1
                        // font attributes, so let's just map it to bold
                        // instead.
                        lookup = version1Table.find(L"bold");
                    }

                    node->mAttributes.erase(search);

                    if (!lookup->second.mFamily.empty())
                        node->mAttributes[L"fontfamily"]
                            = lookup->second.mFamily;

                    if (lookup->second.mIsItalic != defaultItalic)
                        node->mAttributes[L"fontstyle"]
                            = lookup->second.mIsItalic
                            ? L"italic" : L"normal";

                    if (lookup->second.mIsBold)
                        node->mAttributes[L"fontweight"] = L"bold";
                }
                else
                {
                    if (gMathmlFontStrings[defaultMathmlFont]
                        == search->second
                    )
                        // Erase "mathvariant" if its redundant.
                        node->mAttributes.erase(search);
                }
            }
        }

        // Recursively traverse the tree.
        for (list<XmlNode*>::iterator
            child = node->mChildren.begin();
            child != node->mChildren.end();
            child++
        )
            CleanupFontAttributes(*child, mathmlVersion1FontAttributes);
    }
}

auto_ptr<XmlNode> Manager::GenerateMathml(
    const MathmlOptions& options
) const
{
    if (mHasDelayedMathmlError)
        throw mDelayedMathmlError;
    
    if (!mLayoutTree.get())
        throw logic_error(
            "Layout tree not yet built in Manager::GenerateMathml"
        );

    MathmlOptions optionsCopy = options;
    if (mStrictSpacingRequested)
        // Override the spacing control setting if the "\strictspacing"
        // command appeared somewhere in the input.
        optionsCopy.mSpacingControl = MathmlOptions::cSpacingControlStrict;

    // Build the MathML tree. The nodeCount variables counts the number
    // of nodes being generated; if too many appear, an exception is thrown.
    unsigned nodeCount = 0;
    auto_ptr<XmlNode> root = mLayoutTree->BuildMathmlTree(
        optionsCopy,
        LayoutTree::Node::cStyleText,
        nodeCount
    );

    CleanupFontAttributes(root.get(), options.mUseVersion1FontAttributes);

    return root;
}

wstring Manager::GeneratePurifiedTex(
    const PurifiedTexOptions& options
) const
{
    if (!mParseTree.get())
        throw logic_error(
            "Parse tree not yet built in Manager::GeneratePurifiedTex"
        );

    wostringstream os;
    mParseTree->GetPurifiedTex(os, options);
    wstring latex = os.str();

    // Work out which commands appeared in the purified TeX output, so we
    // can work out which LaTeX packages need to be included.

    vector<wstring> tokens;
    Tokenise(latex, tokens);

    set<wstring> commands;
    for (vector<wstring>::iterator
        p = tokens.begin();
        p != tokens.end();
        p++
    )
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

    if (options.mComputeVerticalShift)
    {
        output +=
            L"\\pagestyle{empty}\n"
            L"\\begin{document}\n"
            // The idea here is to add a single dark pixel to the left of
            // the output PNG, which is located on the baseline. Then we
            // can scan for that pixel to determine the vertical alignment
            // (<png><vshift> output field).
            L"\\hbox{\\vrule height 0.4pt depth 0pt width 0.5pt"
            L"\\hbox to 0.7pt{}$";
        
        output += latex;
        
        output +=
            L"\n$}\n"
            L"\\end{document}\n";
    }
    else
    {
        output +=
            L"\\pagestyle{empty}\n"
            L"\\begin{document}\n"
            L"$\n";

        output += latex;

        output +=
            L"\n$\n"
            L"\\end{document}\n";
    }

    return output;
}

}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
