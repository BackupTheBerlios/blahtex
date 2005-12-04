// File "Parser.cpp"
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
#include <stdexcept>

using namespace std;

// FIX : blahtex interprets "a \rm \over b" incorrectly. Mention this in the documentation.
// texvc messes it up too, arguably not as badly. Compare with "a \displaystyle \over b".

// FIX: blahtex can't handle "\mathop\sum"... basically because \sum should expand as a macro, but
// regular latex can figure out what \mathop applies to... I can't!
// This is no big deal though.. e.g. \mathrel\sum fails on latex+amslatex.

// FIX: need to think about illegal UNICODE characters, like FFFE or whatever it is...

namespace blahtex {

// Commands ending with Blahtex are used internally, and are not allowed in the input supplied by the user.
// See gStandardMacros for more information on this.

pair<wstring, Parser::TokenCode> gMathTokenArray[] =
{    
    make_pair(L"",                         Parser::cEndOfInput),
    make_pair(L" ",                        Parser::cWhitespace),
    make_pair(L"\\newcommand",             Parser::cNewcommand),
        
    make_pair(L"$",                        Parser::cIllegal),
    make_pair(L"%",                        Parser::cIllegal),
    make_pair(L"#",                        Parser::cIllegal),
    make_pair(L"`",                        Parser::cIllegal),
    make_pair(L"\"",                       Parser::cIllegal),
    
    make_pair(L"{",                        Parser::cBeginGroup),
    make_pair(L"}",                        Parser::cEndGroup),

    make_pair(L"&",                        Parser::cNextCell),
    make_pair(L"\\\\",                     Parser::cNextRow),

    make_pair(L"^",                        Parser::cSuperscript),
    make_pair(L"_",                        Parser::cSubscript),
    make_pair(L"'",                        Parser::cPrime),

    make_pair(L"\\hbox",                   Parser::cEnterTextMode),
    make_pair(L"\\emphBlahtex",            Parser::cEnterTextMode),
    make_pair(L"\\textBlahtex",            Parser::cEnterTextMode),
    make_pair(L"\\textitBlahtex",          Parser::cEnterTextMode),
    make_pair(L"\\textbfBlahtex",          Parser::cEnterTextMode),
    make_pair(L"\\textrmBlahtex",          Parser::cEnterTextMode),
    make_pair(L"\\textttBlahtex",          Parser::cEnterTextMode),
    make_pair(L"\\textsfBlahtex",          Parser::cEnterTextMode),

    make_pair(L"\\sqrtBlahtex",            Parser::cCommand1Arg),
    make_pair(L"\\pmod",                   Parser::cCommand1Arg),
    make_pair(L"\\operatorname",           Parser::cCommand1Arg),
    make_pair(L"\\operatornamewithlimits", Parser::cCommand1Arg),

    make_pair(L"\\rootBlahtex",            Parser::cCommand2Args),
    make_pair(L"\\binom",                  Parser::cCommand2Args),
    make_pair(L"\\fracBlahtex",            Parser::cCommand2Args),
    make_pair(L"\\cfrac",                  Parser::cCommand2Args),

    make_pair(L"\\over",                   Parser::cCommandInfix),
    make_pair(L"\\choose",                 Parser::cCommandInfix),
    make_pair(L"\\atop",                   Parser::cCommandInfix),

    make_pair(L"\\left",                   Parser::cLeft),
    make_pair(L"\\right",                  Parser::cRight),

    make_pair(L"\\bigBlahtex",             Parser::cBig),
    make_pair(L"\\bigl",                   Parser::cBig),
    make_pair(L"\\bigr",                   Parser::cBig),
    make_pair(L"\\BigBlahtex",             Parser::cBig),
    make_pair(L"\\Bigl",                   Parser::cBig),
    make_pair(L"\\Bigr",                   Parser::cBig),
    make_pair(L"\\biggBlahtex",            Parser::cBig),
    make_pair(L"\\biggl",                  Parser::cBig),
    make_pair(L"\\biggr",                  Parser::cBig),
    make_pair(L"\\BiggBlahtex",            Parser::cBig),
    make_pair(L"\\Biggl",                  Parser::cBig),
    make_pair(L"\\Biggr",                  Parser::cBig),

    make_pair(L"\\mathop",                 Parser::cCommand1Arg),
    make_pair(L"\\mathrel",                Parser::cCommand1Arg),
    make_pair(L"\\mathord",                Parser::cCommand1Arg),
    make_pair(L"\\mathbin",                Parser::cCommand1Arg),
    make_pair(L"\\mathopen",               Parser::cCommand1Arg),
    make_pair(L"\\mathclose",              Parser::cCommand1Arg),
    make_pair(L"\\mathpunct",              Parser::cCommand1Arg),
    make_pair(L"\\mathinner",              Parser::cCommand1Arg),

    // FIX: write about implementation of "\not".
    make_pair(L"\\not",                    Parser::cCommand1Arg),

    make_pair(L"\\limits",                 Parser::cLimits),
    make_pair(L"\\nolimits",               Parser::cLimits),
    make_pair(L"\\displaylimits",          Parser::cLimits),

    make_pair(L"\\_",                      Parser::cSymbol),
    make_pair(L"\\&",                      Parser::cSymbol),
    make_pair(L"\\$",                      Parser::cSymbol),
    make_pair(L"\\#",                      Parser::cSymbol),
    make_pair(L"\\%",                      Parser::cSymbol),
    make_pair(L"\\{",                      Parser::cSymbol),
    make_pair(L"\\}",                      Parser::cSymbol),

    make_pair(L"\\mod",                    Parser::cSymbolUnsafe),
    make_pair(L"\\bmod",                   Parser::cSymbolUnsafe),
        
    make_pair(L"\\begin{matrix}",          Parser::cBeginEnvironment),
    make_pair(L"\\begin{pmatrix}",         Parser::cBeginEnvironment),
    make_pair(L"\\begin{bmatrix}",         Parser::cBeginEnvironment),
    make_pair(L"\\begin{Bmatrix}",         Parser::cBeginEnvironment),
    make_pair(L"\\begin{vmatrix}",         Parser::cBeginEnvironment),
    make_pair(L"\\begin{Vmatrix}",         Parser::cBeginEnvironment),
    make_pair(L"\\begin{cases}",           Parser::cBeginEnvironment),
    make_pair(L"\\begin{aligned}",         Parser::cBeginEnvironment),
    make_pair(L"\\begin{smallmatrix}",     Parser::cBeginEnvironment),
        
    make_pair(L"\\end{matrix}",            Parser::cEndEnvironment),
    make_pair(L"\\end{pmatrix}",           Parser::cEndEnvironment),
    make_pair(L"\\end{bmatrix}",           Parser::cEndEnvironment),
    make_pair(L"\\end{Bmatrix}",           Parser::cEndEnvironment),
    make_pair(L"\\end{vmatrix}",           Parser::cEndEnvironment),
    make_pair(L"\\end{Vmatrix}",           Parser::cEndEnvironment),
    make_pair(L"\\end{cases}",             Parser::cEndEnvironment),
    make_pair(L"\\end{aligned}",           Parser::cEndEnvironment),
    make_pair(L"\\end{smallmatrix}",       Parser::cEndEnvironment),
        
    make_pair(L"~",                        Parser::cSymbolUnsafe),
    make_pair(L"\\,",                      Parser::cSymbolUnsafe),
    make_pair(L"\\!",                      Parser::cSymbolUnsafe),
    make_pair(L"\\ ",                      Parser::cSymbolUnsafe),
    make_pair(L"\\;",                      Parser::cSymbolUnsafe),
    make_pair(L"\\>",                      Parser::cSymbolUnsafe),
    make_pair(L"\\quad",                   Parser::cSymbolUnsafe),
    make_pair(L"\\qquad",                  Parser::cSymbolUnsafe),

    make_pair(L"(",                        Parser::cSymbol),
    make_pair(L")",                        Parser::cSymbol),
    make_pair(L"[",                        Parser::cSymbol),
    make_pair(L"]",                        Parser::cSymbol),
    make_pair(L"<",                        Parser::cSymbol),
    make_pair(L">",                        Parser::cSymbol),
    make_pair(L"+",                        Parser::cSymbol),
    make_pair(L"-",                        Parser::cSymbol),
    make_pair(L"=",                        Parser::cSymbol),
    make_pair(L"|",                        Parser::cSymbol),
    make_pair(L";",                        Parser::cSymbol),
    make_pair(L":",                        Parser::cSymbol),
    make_pair(L",",                        Parser::cSymbol),
    make_pair(L".",                        Parser::cSymbol),
    make_pair(L"/",                        Parser::cSymbol),
    make_pair(L"?",                        Parser::cSymbol),
    make_pair(L"!",                        Parser::cSymbol),
    make_pair(L"@",                        Parser::cSymbol),
    make_pair(L"*",                        Parser::cSymbol),
        
    make_pair(L"\\vert",                   Parser::cSymbol),
    make_pair(L"\\lvert",                  Parser::cSymbol),
    make_pair(L"\\rvert",                  Parser::cSymbol),
    make_pair(L"\\Vert",                   Parser::cSymbol),
    make_pair(L"\\lVert",                  Parser::cSymbol),
    make_pair(L"\\rVert",                  Parser::cSymbol),
    make_pair(L"\\lfloor",                 Parser::cSymbol),
    make_pair(L"\\rfloor",                 Parser::cSymbol),
    make_pair(L"\\lceil",                  Parser::cSymbol),
    make_pair(L"\\rceil",                  Parser::cSymbol),
    make_pair(L"\\lbrace",                 Parser::cSymbol),
    make_pair(L"\\rbrace",                 Parser::cSymbol),
    make_pair(L"\\langle",                 Parser::cSymbol),
    make_pair(L"\\rangle",                 Parser::cSymbol),
    make_pair(L"\\lbrack",                 Parser::cSymbol),
    make_pair(L"\\rbrack",                 Parser::cSymbol),
        
    make_pair(L"\\hat",                    Parser::cCommand1Arg),
    make_pair(L"\\widehat",                Parser::cCommand1Arg),
    make_pair(L"\\dot",                    Parser::cCommand1Arg),
    make_pair(L"\\ddot",                   Parser::cCommand1Arg),
    make_pair(L"\\bar",                    Parser::cCommand1Arg),
    make_pair(L"\\overline",               Parser::cCommand1Arg),
    make_pair(L"\\underline",              Parser::cCommand1Arg),
    make_pair(L"\\overbrace",              Parser::cCommand1Arg),
    make_pair(L"\\underbrace",             Parser::cCommand1Arg),
    make_pair(L"\\overleftarrow",          Parser::cCommand1Arg),
    make_pair(L"\\overrightarrow",         Parser::cCommand1Arg),
    make_pair(L"\\overleftrightarrow",     Parser::cCommand1Arg),
    make_pair(L"\\check",                  Parser::cCommand1Arg),
    make_pair(L"\\acute",                  Parser::cCommand1Arg),
    make_pair(L"\\grave",                  Parser::cCommand1Arg),
    make_pair(L"\\vec",                    Parser::cCommand1Arg),
    make_pair(L"\\breve",                  Parser::cCommand1Arg),
    make_pair(L"\\tilde",                  Parser::cCommand1Arg),
    make_pair(L"\\widetilde",              Parser::cCommand1Arg),
    
    make_pair(L"\\mathbfBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\mathbbBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\mathrmBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\mathitBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\mathcalBlahtex",         Parser::cCommand1Arg),
    make_pair(L"\\mathfrakBlahtex",        Parser::cCommand1Arg),
    make_pair(L"\\mathsfBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\mathttBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\boldsymbol",             Parser::cCommand1Arg),

    make_pair(L"\\rm",                     Parser::cStyleChange),
    make_pair(L"\\bf",                     Parser::cStyleChange),
    make_pair(L"\\it",                     Parser::cStyleChange),
    make_pair(L"\\cal",                    Parser::cStyleChange),
    make_pair(L"\\tt",                     Parser::cStyleChange),
    make_pair(L"\\sf",                     Parser::cStyleChange),

    make_pair(L"\\displaystyle",           Parser::cStyleChange),
    make_pair(L"\\textstyle",              Parser::cStyleChange),
    make_pair(L"\\scriptstyle",            Parser::cStyleChange),
    make_pair(L"\\scriptscriptstyle",      Parser::cStyleChange),
    
    make_pair(L"\\varlimsup",              Parser::cSymbolUnsafe),
    make_pair(L"\\varliminf",              Parser::cSymbolUnsafe),
    make_pair(L"\\lim",                    Parser::cSymbolUnsafe),
    make_pair(L"\\sup",                    Parser::cSymbolUnsafe),
    make_pair(L"\\inf",                    Parser::cSymbolUnsafe),
    make_pair(L"\\limsup",                 Parser::cSymbolUnsafe),
    make_pair(L"\\liminf",                 Parser::cSymbolUnsafe),
    make_pair(L"\\injlim",                 Parser::cSymbolUnsafe),
    make_pair(L"\\projlim",                Parser::cSymbolUnsafe),
    make_pair(L"\\min",                    Parser::cSymbolUnsafe),
    make_pair(L"\\max",                    Parser::cSymbolUnsafe),
    make_pair(L"\\gcd",                    Parser::cSymbolUnsafe),
    make_pair(L"\\det",                    Parser::cSymbolUnsafe),
    make_pair(L"\\Pr",                     Parser::cSymbolUnsafe),

    make_pair(L"\\ker",                    Parser::cSymbolUnsafe),
    make_pair(L"\\hom",                    Parser::cSymbolUnsafe),
    make_pair(L"\\dim",                    Parser::cSymbolUnsafe),
    make_pair(L"\\arg",                    Parser::cSymbolUnsafe),
    make_pair(L"\\sin",                    Parser::cSymbolUnsafe),
    make_pair(L"\\cos",                    Parser::cSymbolUnsafe),
    make_pair(L"\\sec",                    Parser::cSymbolUnsafe),
    make_pair(L"\\csc",                    Parser::cSymbolUnsafe),
    make_pair(L"\\tan",                    Parser::cSymbolUnsafe),
    make_pair(L"\\cot",                    Parser::cSymbolUnsafe),
    make_pair(L"\\arcsin",                 Parser::cSymbolUnsafe),
    make_pair(L"\\arccos",                 Parser::cSymbolUnsafe),
    make_pair(L"\\arcsec",                 Parser::cSymbolUnsafe),
    make_pair(L"\\arccsc",                 Parser::cSymbolUnsafe),
    make_pair(L"\\arctan",                 Parser::cSymbolUnsafe),
    make_pair(L"\\arccot",                 Parser::cSymbolUnsafe),
    make_pair(L"\\sinh",                   Parser::cSymbolUnsafe),
    make_pair(L"\\cosh",                   Parser::cSymbolUnsafe),
    make_pair(L"\\tanh",                   Parser::cSymbolUnsafe),
    make_pair(L"\\coth",                   Parser::cSymbolUnsafe),
    make_pair(L"\\log",                    Parser::cSymbolUnsafe),
    make_pair(L"\\lg",                     Parser::cSymbolUnsafe),
    make_pair(L"\\ln",                     Parser::cSymbolUnsafe),
    make_pair(L"\\exp",                    Parser::cSymbolUnsafe),
    make_pair(L"\\deg",                    Parser::cSymbolUnsafe),

    make_pair(L"\\alpha",                  Parser::cSymbol),
    make_pair(L"\\beta",                   Parser::cSymbol),
    make_pair(L"\\gamma",                  Parser::cSymbol),
    make_pair(L"\\delta",                  Parser::cSymbol),
    make_pair(L"\\epsilon",                Parser::cSymbol),
    make_pair(L"\\varepsilon",             Parser::cSymbol),
    make_pair(L"\\zeta",                   Parser::cSymbol),
    make_pair(L"\\eta",                    Parser::cSymbol),
    make_pair(L"\\theta",                  Parser::cSymbol),
    make_pair(L"\\vartheta",               Parser::cSymbol),
    make_pair(L"\\iota",                   Parser::cSymbol),
    make_pair(L"\\kappa",                  Parser::cSymbol),
    make_pair(L"\\varkappa",               Parser::cSymbol),
    make_pair(L"\\lambda",                 Parser::cSymbol),
    make_pair(L"\\mu",                     Parser::cSymbol),
    make_pair(L"\\nu",                     Parser::cSymbol),
    make_pair(L"\\pi",                     Parser::cSymbol),
    make_pair(L"\\varpi",                  Parser::cSymbol),
    make_pair(L"\\rho",                    Parser::cSymbol),
    make_pair(L"\\varrho",                 Parser::cSymbol),
    make_pair(L"\\sigma",                  Parser::cSymbol),
    make_pair(L"\\varsigma",               Parser::cSymbol),
    make_pair(L"\\tau",                    Parser::cSymbol),
    make_pair(L"\\upsilon",                Parser::cSymbol),
    make_pair(L"\\phi",                    Parser::cSymbol),
    make_pair(L"\\varphi",                 Parser::cSymbol),
    make_pair(L"\\chi",                    Parser::cSymbol),
    make_pair(L"\\psi",                    Parser::cSymbol),
    make_pair(L"\\omega",                  Parser::cSymbol),
    make_pair(L"\\xi",                     Parser::cSymbol),
    make_pair(L"\\digamma",                Parser::cSymbol),
        
    make_pair(L"\\Gamma",                  Parser::cSymbol),
    make_pair(L"\\Delta",                  Parser::cSymbol),
    make_pair(L"\\Theta",                  Parser::cSymbol),
    make_pair(L"\\Lambda",                 Parser::cSymbol),
    make_pair(L"\\Pi",                     Parser::cSymbol),
    make_pair(L"\\Sigma",                  Parser::cSymbol),
    make_pair(L"\\Upsilon",                Parser::cSymbol),
    make_pair(L"\\Phi",                    Parser::cSymbol),
    make_pair(L"\\Psi",                    Parser::cSymbol),
    make_pair(L"\\Omega",                  Parser::cSymbol),
    make_pair(L"\\Xi",                     Parser::cSymbol),

    make_pair(L"\\aleph",                  Parser::cSymbol),
    make_pair(L"\\beth",                   Parser::cSymbol),
    make_pair(L"\\gimel",                  Parser::cSymbol),
    make_pair(L"\\daleth",                 Parser::cSymbol),
        
    make_pair(L"\\wp",                     Parser::cSymbol),
    make_pair(L"\\ell",                    Parser::cSymbol),
    make_pair(L"\\P",                      Parser::cSymbol),
    make_pair(L"\\imath",                  Parser::cSymbol),
    make_pair(L"\\forall",                 Parser::cSymbol),
    make_pair(L"\\exists",                 Parser::cSymbol),
    make_pair(L"\\Finv",                   Parser::cSymbol),
    make_pair(L"\\Game",                   Parser::cSymbol),
    make_pair(L"\\partial",                Parser::cSymbol),
    make_pair(L"\\Re",                     Parser::cSymbol),
    make_pair(L"\\Im",                     Parser::cSymbol),
        
    make_pair(L"\\leftarrow",              Parser::cSymbol),
    make_pair(L"\\rightarrow",             Parser::cSymbol),
    make_pair(L"\\longleftarrow",          Parser::cSymbolUnsafe),
    make_pair(L"\\longrightarrow",         Parser::cSymbolUnsafe),    
    make_pair(L"\\Leftarrow",              Parser::cSymbol),
    make_pair(L"\\Rightarrow",             Parser::cSymbol),
    make_pair(L"\\Longleftarrow",          Parser::cSymbolUnsafe),
    make_pair(L"\\Longrightarrow",         Parser::cSymbolUnsafe),    
    make_pair(L"\\mapsto",                 Parser::cSymbolUnsafe),
    make_pair(L"\\longmapsto",             Parser::cSymbolUnsafe),
    make_pair(L"\\leftrightarrow",         Parser::cSymbol),
    make_pair(L"\\Leftrightarrow",         Parser::cSymbol),
    make_pair(L"\\longleftrightarrow",     Parser::cSymbolUnsafe),
    make_pair(L"\\Longleftrightarrow",     Parser::cSymbolUnsafe),
    
    make_pair(L"\\uparrow",                Parser::cSymbol),
    make_pair(L"\\Uparrow",                Parser::cSymbol),
    make_pair(L"\\downarrow",              Parser::cSymbol),
    make_pair(L"\\Downarrow",              Parser::cSymbol),
    make_pair(L"\\updownarrow",            Parser::cSymbol),
    make_pair(L"\\Updownarrow",            Parser::cSymbol),
    
    make_pair(L"\\searrow",                Parser::cSymbol),
    make_pair(L"\\nearrow",                Parser::cSymbol),
    make_pair(L"\\swarrow",                Parser::cSymbol),
    make_pair(L"\\nwarrow",                Parser::cSymbol),
    
    make_pair(L"\\hookrightarrow",         Parser::cSymbolUnsafe),
    make_pair(L"\\hookleftarrow",          Parser::cSymbolUnsafe),
    make_pair(L"\\upharpoonright",         Parser::cSymbol),
    make_pair(L"\\upharpoonleft",          Parser::cSymbol),
    make_pair(L"\\downharpoonright",       Parser::cSymbol),
    make_pair(L"\\downharpoonleft",        Parser::cSymbol),
    make_pair(L"\\rightharpoonup",         Parser::cSymbol),
    make_pair(L"\\rightharpoondown",       Parser::cSymbol),
    make_pair(L"\\leftharpoonup",          Parser::cSymbol),
    make_pair(L"\\leftharpoondown",        Parser::cSymbol),
    
    make_pair(L"\\nleftarrow",             Parser::cSymbol),
    make_pair(L"\\nrightarrow",            Parser::cSymbol),
        
    make_pair(L"\\supset",                 Parser::cSymbol),
    make_pair(L"\\subset",                 Parser::cSymbol),
    make_pair(L"\\supseteq",               Parser::cSymbol),
    make_pair(L"\\subseteq",               Parser::cSymbol),
    make_pair(L"\\sqsupset",               Parser::cSymbol),
    make_pair(L"\\sqsubset",               Parser::cSymbol),
    make_pair(L"\\sqsupseteq",             Parser::cSymbol),
    make_pair(L"\\sqsubseteq",             Parser::cSymbol),
    make_pair(L"\\supsetneq",              Parser::cSymbol),
    make_pair(L"\\subsetneq",              Parser::cSymbol),
        
    make_pair(L"\\in",                     Parser::cSymbol),
    make_pair(L"\\ni",                     Parser::cSymbol),
    make_pair(L"\\notin",                  Parser::cSymbolUnsafe),

    make_pair(L"\\iff",                    Parser::cSymbolUnsafe),
        
    make_pair(L"\\mid",                    Parser::cSymbol),
    make_pair(L"\\sim",                    Parser::cSymbol),
    make_pair(L"\\simeq",                  Parser::cSymbol),
    make_pair(L"\\approx",                 Parser::cSymbol),
    make_pair(L"\\propto",                 Parser::cSymbol),
    make_pair(L"\\equiv",                  Parser::cSymbol),
    make_pair(L"\\cong",                   Parser::cSymbolUnsafe),
    make_pair(L"\\neq",                    Parser::cSymbolUnsafe),
        
    make_pair(L"\\ll",                     Parser::cSymbol),
    make_pair(L"\\gg",                     Parser::cSymbol),
    make_pair(L"\\geq",                    Parser::cSymbol),
    make_pair(L"\\leq",                    Parser::cSymbol),
    make_pair(L"\\triangleleft",           Parser::cSymbol),
    make_pair(L"\\triangleright",          Parser::cSymbol),
    make_pair(L"\\trianglelefteq",         Parser::cSymbol),
    make_pair(L"\\trianglerighteq",        Parser::cSymbol),
        
    make_pair(L"\\models",                 Parser::cSymbolUnsafe),
    make_pair(L"\\vdash",                  Parser::cSymbol),
    make_pair(L"\\Vdash",                  Parser::cSymbol),
    make_pair(L"\\vDash",                  Parser::cSymbol),
        
    make_pair(L"\\lesssim",                Parser::cSymbol),
    make_pair(L"\\nless",                  Parser::cSymbol),
    make_pair(L"\\ngeq",                   Parser::cSymbol),
    make_pair(L"\\nleq",                   Parser::cSymbol),
        
    make_pair(L"\\times",                  Parser::cSymbol),
    make_pair(L"\\div",                    Parser::cSymbol),
    make_pair(L"\\wedge",                  Parser::cSymbol),
    make_pair(L"\\vee",                    Parser::cSymbol),
    make_pair(L"\\oplus",                  Parser::cSymbol),
    make_pair(L"\\otimes",                 Parser::cSymbol),
    make_pair(L"\\cap",                    Parser::cSymbol),
    make_pair(L"\\cup",                    Parser::cSymbol),
    make_pair(L"\\sqcap",                  Parser::cSymbol),
    make_pair(L"\\sqcup",                  Parser::cSymbol),
    make_pair(L"\\smile",                  Parser::cSymbol),
    make_pair(L"\\frown",                  Parser::cSymbol),
    make_pair(L"\\smallsmile",             Parser::cSymbol),
    make_pair(L"\\smallfrown",             Parser::cSymbol),

    make_pair(L"\\setminus",               Parser::cSymbol),
    make_pair(L"\\smallsetminus",          Parser::cSymbol),
        
    make_pair(L"\\And",                    Parser::cSymbolUnsafe),

    // This next group of "large operators" are all "safe" in ordinary latex, but NOT when AMS-latex is
    // loaded, so we'll call them unsafe (just to be safe :-)
    make_pair(L"\\sum",                    Parser::cSymbolUnsafe),
    make_pair(L"\\prod",                   Parser::cSymbolUnsafe),
    make_pair(L"\\int",                    Parser::cSymbolUnsafe),
    make_pair(L"\\iint",                   Parser::cSymbolUnsafe),
    make_pair(L"\\iiint",                  Parser::cSymbolUnsafe),
    make_pair(L"\\iiiint",                 Parser::cSymbolUnsafe),
    make_pair(L"\\oint",                   Parser::cSymbolUnsafe),
    make_pair(L"\\bigcap",                 Parser::cSymbolUnsafe),
    make_pair(L"\\bigodot",                Parser::cSymbolUnsafe),
    make_pair(L"\\bigcup",                 Parser::cSymbolUnsafe),
    make_pair(L"\\bigotimes",              Parser::cSymbolUnsafe),
    make_pair(L"\\coprod",                 Parser::cSymbolUnsafe),
    make_pair(L"\\bigsqcup",               Parser::cSymbolUnsafe),
    make_pair(L"\\bigoplus",               Parser::cSymbolUnsafe),
    make_pair(L"\\bigvee",                 Parser::cSymbolUnsafe),
    make_pair(L"\\biguplus",               Parser::cSymbolUnsafe),
    make_pair(L"\\bigwedge",               Parser::cSymbolUnsafe),
        
    make_pair(L"\\star",                   Parser::cSymbol),
    make_pair(L"\\triangle",               Parser::cSymbol),
    make_pair(L"\\wr",                     Parser::cSymbol),
    make_pair(L"\\infty",                  Parser::cSymbol),
    make_pair(L"\\circ",                   Parser::cSymbol),
    make_pair(L"\\hbar",                   Parser::cSymbol),
    make_pair(L"\\lnot",                   Parser::cSymbol),
    make_pair(L"\\nabla",                  Parser::cSymbol),
    make_pair(L"\\prime",                  Parser::cSymbol),
    make_pair(L"\\backslash",              Parser::cSymbol),
    make_pair(L"\\pm",                     Parser::cSymbol),
    make_pair(L"\\mp",                     Parser::cSymbol),
    make_pair(L"\\emptyset",               Parser::cSymbol),
    make_pair(L"\\varnothing",             Parser::cSymbol),
    make_pair(L"\\S",                      Parser::cSymbol),
    make_pair(L"\\angle",                  Parser::cSymbol),
    make_pair(L"\\colon",                  Parser::cSymbolUnsafe),
    make_pair(L"\\Diamond",                Parser::cSymbol),
    make_pair(L"\\nmid",                   Parser::cSymbol),    
    make_pair(L"\\square",                 Parser::cSymbol),
    make_pair(L"\\Box",                    Parser::cSymbol),
    make_pair(L"\\checkmark",              Parser::cSymbol),
    make_pair(L"\\complement",             Parser::cSymbol),
    make_pair(L"\\eth",                    Parser::cSymbol),
    make_pair(L"\\hslash",                 Parser::cSymbol),
    make_pair(L"\\mho",                    Parser::cSymbol),
        
    make_pair(L"\\flat",                   Parser::cSymbol),
    make_pair(L"\\sharp",                  Parser::cSymbol),
    make_pair(L"\\natural",                Parser::cSymbol),
    make_pair(L"\\bullet",                 Parser::cSymbol),
    make_pair(L"\\dagger",                 Parser::cSymbol),
    make_pair(L"\\ddagger",                Parser::cSymbol),
        
    make_pair(L"\\clubsuit",               Parser::cSymbol),
    make_pair(L"\\spadesuit",              Parser::cSymbol),
    make_pair(L"\\heartsuit",              Parser::cSymbol),
    make_pair(L"\\diamondsuit",            Parser::cSymbol),
        
    make_pair(L"\\top",                    Parser::cSymbol),
    make_pair(L"\\bot",                    Parser::cSymbol),
    make_pair(L"\\perp",                   Parser::cSymbol),
        
    make_pair(L"\\ldots",                  Parser::cSymbolUnsafe),
    make_pair(L"\\cdot",                   Parser::cSymbol),
    make_pair(L"\\cdots",                  Parser::cSymbolUnsafe),
    make_pair(L"\\vdots",                  Parser::cSymbolUnsafe),
    make_pair(L"\\ddots",                  Parser::cSymbolUnsafe),
    
    // AMSLaTeX is pretty clever with "\dots" and "\dotsb"; it adjusts the height of the
    // dots based on the surrounding symbols. Blahtex currently doesn't do this.
    make_pair(L"\\dots",                   Parser::cSymbolUnsafe),
    make_pair(L"\\dotsb",                  Parser::cSymbolUnsafe),
                
    make_pair(L"\\varinjlim",              Parser::cSymbolUnsafe),
    make_pair(L"\\varprojlim",             Parser::cSymbolUnsafe)
};

pair<wstring, Parser::TokenCode> gTextTokenArray[] =
{
    make_pair(L"",                         Parser::cEndOfInput),
    make_pair(L" ",                        Parser::cWhitespace),
    make_pair(L"\\newcommand",             Parser::cNewcommand),

    make_pair(L"{",                        Parser::cBeginGroup),
    make_pair(L"}",                        Parser::cEndGroup),

    make_pair(L"$",                        Parser::cIllegal),
    make_pair(L"%",                        Parser::cIllegal),
    make_pair(L"#",                        Parser::cIllegal),
    make_pair(L"&",                        Parser::cIllegal),
    make_pair(L"\\\\",                     Parser::cIllegal),
    make_pair(L"^",                        Parser::cIllegal),
    make_pair(L"_",                        Parser::cIllegal),

    make_pair(L"\\&",                      Parser::cSymbol),
    make_pair(L"\\_",                      Parser::cSymbol),
    make_pair(L"\\$",                      Parser::cSymbol),
    make_pair(L"\\#",                      Parser::cSymbol),
    make_pair(L"\\%",                      Parser::cSymbol),
    make_pair(L"\\{",                      Parser::cSymbol),
    make_pair(L"\\}",                      Parser::cSymbol),
    make_pair(L"\\textbackslash",          Parser::cSymbol),
    make_pair(L"\\textvisiblespace",       Parser::cSymbol),
    make_pair(L"\\O",                      Parser::cSymbol),
    make_pair(L"\\S",                      Parser::cSymbol),

    make_pair(L"!",                        Parser::cSymbol),
    make_pair(L"@",                        Parser::cSymbol),
    make_pair(L"*",                        Parser::cSymbol),
    make_pair(L"(",                        Parser::cSymbol),
    make_pair(L")",                        Parser::cSymbol),
    make_pair(L"-",                        Parser::cSymbol),
    make_pair(L"=",                        Parser::cSymbol),
    make_pair(L"+",                        Parser::cSymbol),
    make_pair(L"[",                        Parser::cSymbol),
    make_pair(L"]",                        Parser::cSymbol),
    make_pair(L"|",                        Parser::cSymbol),
    make_pair(L";",                        Parser::cSymbol),
    make_pair(L":",                        Parser::cSymbol),
    make_pair(L"<",                        Parser::cSymbol),
    make_pair(L">",                        Parser::cSymbol),
    make_pair(L",",                        Parser::cSymbol),
    make_pair(L".",                        Parser::cSymbol),
    make_pair(L"/",                        Parser::cSymbol),
    make_pair(L"?",                        Parser::cSymbol),
    make_pair(L"\"",                       Parser::cSymbol),

    make_pair(L"~",                        Parser::cSymbolUnsafe),
    make_pair(L"\\,",                      Parser::cSymbolUnsafe),
    make_pair(L"\\!",                      Parser::cSymbolUnsafe),
    make_pair(L"\\ ",                      Parser::cSymbolUnsafe),
    make_pair(L"\\;",                      Parser::cSymbolUnsafe),
    make_pair(L"\\quad",                   Parser::cSymbolUnsafe),
    make_pair(L"\\qquad",                  Parser::cSymbolUnsafe),

    make_pair(L"\\hbox",                   Parser::cCommand1Arg),
    make_pair(L"\\emphBlahtex",            Parser::cCommand1Arg),
    make_pair(L"\\textBlahtex",            Parser::cCommand1Arg),
    make_pair(L"\\textitBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\textbfBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\textrmBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\textttBlahtex",          Parser::cCommand1Arg),
    make_pair(L"\\textsfBlahtex",          Parser::cCommand1Arg),

    make_pair(L"\\rm",                     Parser::cStyleChange),
    make_pair(L"\\it",                     Parser::cStyleChange),
    make_pair(L"\\bf",                     Parser::cStyleChange),
    make_pair(L"\\tt",                     Parser::cStyleChange),
    make_pair(L"\\sf",                     Parser::cStyleChange),
};

wishful_hash_map<wstring, Parser::TokenCode> Parser::gMathTokenTable(gMathTokenArray, END_ARRAY(gMathTokenArray));
wishful_hash_map<wstring, Parser::TokenCode> Parser::gTextTokenTable(gTextTokenArray, END_ARRAY(gTextTokenArray));

Parser::TokenCode Parser::GetMathTokenCode(const wstring& token) const
{
    wishful_hash_map<wstring, TokenCode>::const_iterator output = gMathTokenTable.find(token);
    if (output != gMathTokenTable.end())
    {
        if (output->second != cIllegal)
            return output->second;
            
        // Give the user some helpful hints if they try to use certain illegal commands.
        if (token == L"%" || token == L"#" || token == L"$")
            throw Exception(Exception::cIllegalCommandInMathModeWithHint, token, L"\\" + token);
        else if (token == L"`" || token == L"\"")
            throw Exception(Exception::cIllegalCommandInMathMode, L"`");

        throw logic_error("Unexpected illegal character in Parser::GetMathTokenCode");
    }

    if (token[0] == L'\\')
    {
        if (gTextTokenTable.count(token))
            throw Exception(Exception::cIllegalCommandInMathMode, StripBlahtexSuffix(token));
        else
            throw Exception(Exception::cUnrecognisedCommand, token);
    }

    if ((token[0] >= L'a' && token[0] <= L'z') ||
        (token[0] >= L'A' && token[0] <= L'Z') ||
        (token[0] >= L'0' && token[0] <= L'9') ||
        (token[0] > 0x7F))
        return cSymbol;

    throw Exception(Exception::cUnrecognisedCommand, token);
}

Parser::TokenCode Parser::GetTextTokenCode(const wstring& token) const
{
    wishful_hash_map<wstring, TokenCode>::const_iterator output = gTextTokenTable.find(token);
    if (output != gTextTokenTable.end())
    {
        if (output->second != cIllegal)
            return output->second;
            
        // Give the user some helpful hints if they try to use certain illegal commands.
        if (token == L"&" || token == L"_" || token == L"%" || token == L"#" || token == L"$")
            throw Exception(Exception::cIllegalCommandInTextModeWithHint, token, L"\\" + token);
        else if (token == L"\\\\")
            throw Exception(Exception::cIllegalCommandInTextModeWithHint, L"\\\\", L"\\textbackslash");
        else
            throw Exception(Exception::cIllegalCommandInTextMode, StripBlahtexSuffix(token));
    }

    if (token[0] == L'\\')
    {
        if (gMathTokenTable.count(token))
            throw Exception(Exception::cIllegalCommandInTextMode, StripBlahtexSuffix(token));
        else
            throw Exception(Exception::cUnrecognisedCommand, token);
    }
    
    if ((token[0] >= L'a' && token[0] <= L'z') ||
        (token[0] >= L'A' && token[0] <= L'Z') ||
        (token[0] >= L'0' && token[0] <= L'9') ||
        (token[0] > 0x7F))
        return cSymbol;

    throw Exception(Exception::cUnrecognisedCommand, token);
}

auto_ptr<ParseTree::MathNode> Parser::DoParse(const vector<wstring>& input)
{
    mTokenSource.reset(new MacroProcessor(input));
    auto_ptr<ParseTree::MathNode> output = ParseMathList();
    
    switch (GetMathTokenCode(mTokenSource->Peek()))
    {
        case cEndOfInput:     return output;
        case cEndGroup:       throw Exception(Exception::cUnmatchedCloseBrace);
        case cRight:          throw Exception(Exception::cUnmatchedRight);
        case cNextCell:       throw Exception(Exception::cUnexpectedNextCell);
        case cNextRow:        throw Exception(Exception::cUnexpectedNextRow);
        case cEndEnvironment: throw Exception(Exception::cUnmatchedEnd);
    }

    throw logic_error("Unexpected token code in Parser::DoParse");
}

auto_ptr<ParseTree::MathNode> Parser::ParseMathField()
{
    mTokenSource->SkipWhitespace();
    wstring command = mTokenSource->Get();

    switch (GetMathTokenCode(command))
    {
        case cSymbol:
            return auto_ptr<ParseTree::MathNode>(new ParseTree::MathSymbol(command));
        
        case cBeginGroup:
        {
            auto_ptr<ParseTree::MathNode> field(new ParseTree::MathGroup(ParseMathList()));
            if (mTokenSource->Peek() != L"}")
                throw Exception(Exception::cUnmatchedOpenBrace);
            mTokenSource->Advance();
            return field;
        }

        // FIX: make sure this code gets duplicated for ParseTextField:
        case cEndOfInput:
            throw Exception(Exception::cMissingOpenBraceAtEnd);
    }

    throw Exception(Exception::cMissingOpenBraceBefore, StripBlahtexSuffix(command));
}

auto_ptr<ParseTree::MathTable> Parser::ParseMathTable()
{
    auto_ptr<ParseTree::MathTable> table(new ParseTree::MathTable);
    auto_ptr<ParseTree::MathTableRow> row(new ParseTree::MathTableRow);
    
    while (true)
    {
        auto_ptr<ParseTree::MathNode> entry = ParseMathList();
        
        switch (GetMathTokenCode(mTokenSource->Peek()))
        {
            case cNextCell:
            {
                mTokenSource->Advance();
                row->mEntries.push_back(entry.release());
                break;
            }

            case cNextRow:
            {
                mTokenSource->Advance();
                row->mEntries.push_back(entry.release());
                table->mRows.push_back(row.release());
                row.reset(new ParseTree::MathTableRow);
                break;
            }
            
            case cEndGroup:
            case cRight:
            case cEndOfInput:
            case cEndEnvironment:
            {
                // We only include the last row if it isn't blank,
                // e.g. "\begin{matrix} a \\ \end{matrix}" should only result in a single row.
                ParseTree::MathList* check = dynamic_cast<ParseTree::MathList*>(entry.get());
                if (!check || !check->mChildren.empty() || !row->mEntries.empty())
                {
                    row->mEntries.push_back(entry.release());
                    table->mRows.push_back(row.release());
                }
                
                return table;
            }
            
            default:
                throw logic_error("Unexpected token code in Parser::ParseMathTable");
        }
    }

    // Hmmm... gcc seems to think the control flow can reach here; I beg to differ.
    throw logic_error("Unexpected control flow in Parser::ParseMathTable");
}

ParseTree::MathScripts* Parser::PrepareScripts(ParseTree::MathList* output)
{
    ParseTree::MathScripts* target;

    if (output->mChildren.empty())
    {
        target = new ParseTree::MathScripts;
        output->mChildren.push_back(target);
    }
    else
    {
        target = dynamic_cast<ParseTree::MathScripts*>(output->mChildren.back());
        if (!target)
        {
            target = new ParseTree::MathScripts;
            target->mBase.reset(output->mChildren.back());
            output->mChildren.back() = target;
        }
    }

    return target;
}

auto_ptr<ParseTree::MathNode> Parser::ParseMathList()
{
    auto_ptr<ParseTree::MathList> output(new ParseTree::MathList);

    // infixNumerator temporarily holds the numerator of an infix command (like "\over"), while we are
    // waiting for the denominator to be fully built up.
    auto_ptr<ParseTree::MathList> infixNumerator;
    wstring infixCommand;
    
    while (true)
    {
        switch (GetMathTokenCode(mTokenSource->Peek()))
        {
            case cEndGroup:
            case cRight:
            case cNextCell:
            case cNextRow:
            case cEndEnvironment:
            case cEndOfInput:
            {
                // It's a little strange that the following static_casts should be needed, but gcc 3.3
                // seems to require them. Don't know about later versions.
                if (!infixCommand.empty())
                    return auto_ptr<ParseTree::MathNode>(
                        new ParseTree::MathCommand2Args(infixCommand,
                        static_cast<auto_ptr<ParseTree::MathNode> >(infixNumerator),
                        static_cast<auto_ptr<ParseTree::MathNode> >(output),
                        true   // "true" specifies this is an infix command rather than a two-argument command
                        ));
                else
                {
                    if (output->mChildren.size() == 1)
                    {
                        // If there's only node in the list, return just that single node.
                        // (We need to actually remove it from output->mChildren so that output's destructor
                        // doesn't delete it *again*).
                        auto_ptr<ParseTree::MathNode> temp(output->mChildren.back());
                        output->mChildren.clear();
                        return temp;
                    }
                    else
                        return static_cast<auto_ptr<ParseTree::MathNode> >(output);
                }
            }
            
            case cNewcommand:
            {
                mTokenSource->HandleNewcommand();
                break;
            }

            case cWhitespace:
            {
                // Skip whitespace.
                mTokenSource->Advance();
                break;
            }

            case cSymbol:
            case cSymbolUnsafe:
            {
                output->mChildren.push_back(new ParseTree::MathSymbol(mTokenSource->Get()));
                break;
            }
            
            case cBeginGroup:
            {
                mTokenSource->Advance();
                output->mChildren.push_back(new ParseTree::MathGroup(ParseMathList()));
                if (mTokenSource->Peek() != L"}")
                    throw Exception(Exception::cUnmatchedOpenBrace);
                mTokenSource->Advance();
                break;
            }                

            case cBeginEnvironment:
            {
                // extract e.g. "matrix" from "\begin{matrix}"
                wstring beginCommand = mTokenSource->Get();
                wstring name = beginCommand.substr(7, beginCommand.size() - 8);
                
                auto_ptr<ParseTree::MathTable> table = ParseMathTable();
                
                wstring endCommand = mTokenSource->Get();
                if (GetMathTokenCode(endCommand) != cEndEnvironment)
                    throw Exception(Exception::cUnmatchedBegin, beginCommand);
                
                if (name != endCommand.substr(5, endCommand.size() - 6))
                    throw Exception(Exception::cMismatchedBeginAndEnd, beginCommand, endCommand);
                
                if (name == L"cases")
                {
                    // check that none of the rows have more than two entries
                    for (vector<ParseTree::MathTableRow*>::iterator row = table->mRows.begin();
                        row != table->mRows.end(); row++)

                        if ((*row)->mEntries.size() > 2)
                            throw Exception(Exception::cCasesRowTooBig);
                }

                output->mChildren.push_back(new ParseTree::MathEnvironment(name, table));
                break;
            }

            case cEnterTextMode:
            {
                wstring command = mTokenSource->Get();
                
                mTokenSource->SkipWhitespace();
                if (mTokenSource->Peek() != L"{")
                    throw Exception(Exception::cMissingOpenBraceAfter, StripBlahtexSuffix(command));

                output->mChildren.push_back(new ParseTree::EnterTextMode(command, ParseTextField()));                        
                break;
            }
            
            case cLeft:
            {
                mTokenSource->Advance();
                mTokenSource->SkipWhitespace();
                wstring left = mTokenSource->Get();
                if (left.empty())
                    throw Exception(Exception::cMissingDelimiter, L"\\left");
                else if (!gDelimiterTable.count(left))
                    throw Exception(Exception::cIllegalDelimiter, L"\\left");

                auto_ptr<ParseTree::MathNode> child = ParseMathList();

                if (mTokenSource->Peek() != L"\\right")
                    throw Exception(Exception::cUnmatchedLeft);

                mTokenSource->Advance();
                mTokenSource->SkipWhitespace();
                wstring right = mTokenSource->Get();
                if (right.empty())
                    throw Exception(Exception::cMissingDelimiter, L"\\right");
                else if (!gDelimiterTable.count(right))
                    throw Exception(Exception::cIllegalDelimiter, L"\\right");
                
                output->mChildren.push_back(new ParseTree::MathDelimited(child, left, right));
                break;
            }

            case cBig:
            {
                wstring command = mTokenSource->Get();
                mTokenSource->SkipWhitespace();
                wstring delimiter = mTokenSource->Get();
                if (delimiter.empty())
                    throw Exception(Exception::cMissingDelimiter, StripBlahtexSuffix(command));
                else if (!gDelimiterTable.count(delimiter))
                    throw Exception(Exception::cIllegalDelimiter, StripBlahtexSuffix(command));
                
                output->mChildren.push_back(new ParseTree::MathBig(command, delimiter));
                break;
            }
            
            case cSuperscript:
            {
                mTokenSource->Advance();
                ParseTree::MathScripts* target = PrepareScripts(output.get());
                if (target->mUpper.get())
                    throw Exception(Exception::cDoubleSuperscript);
                target->mUpper = ParseMathField();
                break;
            }
            
            case cSubscript:
            {
                mTokenSource->Advance();
                ParseTree::MathScripts* target = PrepareScripts(output.get());
                if (target->mLower.get())
                    throw Exception(Exception::cDoubleSubscript);
                target->mLower = ParseMathField();
                break;
            }
            
            case cPrime:
            {
                auto_ptr<ParseTree::MathList> superscript(new ParseTree::MathList);

                while (mTokenSource->Peek() == L"'")
                {
                    superscript->mChildren.push_back(new ParseTree::MathSymbol(L"\\prime"));
                    mTokenSource->Advance();
                }
                
                ParseTree::MathScripts* target = PrepareScripts(output.get());
                if (target->mUpper.get())
                    throw Exception(Exception::cDoubleSuperscript);
                
                if (mTokenSource->Peek() == L"^")
                {
                    mTokenSource->Advance();
                    superscript->mChildren.push_back(ParseMathField().release());
                }
                
                target->mUpper.reset(new ParseTree::MathGroup(static_cast<auto_ptr<ParseTree::MathNode> >(superscript)));
                break;
            }

            case cLimits:
            {
                wstring command = mTokenSource->Get();
                if (output->mChildren.empty())
                    throw Exception(Exception::cMisplacedLimits, command);

                ParseTree::MathScripts* scripts = dynamic_cast<ParseTree::MathScripts*>(output->mChildren.back());
                if (scripts)
                    scripts->mBase.reset(new ParseTree::MathLimits(command, scripts->mBase));
                else
                    output->mChildren.back() =
                        new ParseTree::MathLimits(command, auto_ptr<ParseTree::MathNode>(output->mChildren.back()));
                break;
            }
            
            case cCommand1Arg:
            {
                wstring command = mTokenSource->Get();
                output->mChildren.push_back(new ParseTree::MathCommand1Arg(command, ParseMathField()));
                break;
            }
            
            case cStyleChange:
            {
                wstring command = mTokenSource->Get();
                output->mChildren.push_back(new ParseTree::MathStyleChange(command, ParseMathList()));
                break;
            }
            
            case cCommand2Args:
            {
                wstring command = mTokenSource->Get();
                auto_ptr<ParseTree::MathNode> child1 = ParseMathField();
                auto_ptr<ParseTree::MathNode> child2 = ParseMathField();
                output->mChildren.push_back(new ParseTree::MathCommand2Args(command, child1, child2, false));
                break;
            }
            
            case cCommandInfix:
            {
                if (!infixCommand.empty())
                    throw Exception(Exception::cAmbiguousInfix, mTokenSource->Peek());
                
                infixNumerator = output;
                infixCommand = mTokenSource->Get();
                output.reset(new ParseTree::MathList);
                break;
            }

            default:
                throw logic_error("Unexpected token code in Parser::ParseMathList");
        }
    }

    // Hmmm... gcc seems to think the control flow can reach here; I beg to differ.
    throw logic_error("Unexpected control flow in Parser::ParseMathList");
}
            
auto_ptr<ParseTree::TextNode> Parser::ParseTextField()
{
    mTokenSource->SkipWhitespace();
    wstring command = mTokenSource->Get();

    switch (GetTextTokenCode(command))
    {    
        case cSymbol:
            return auto_ptr<ParseTree::TextNode>(new ParseTree::TextSymbol(command));
        
        case cBeginGroup:
        {
            auto_ptr<ParseTree::TextNode> field(new ParseTree::TextGroup(ParseTextList()));
            if (mTokenSource->Peek() != L"}")
                throw Exception(Exception::cUnmatchedOpenBrace);
            mTokenSource->Advance();
            return field;
        }
        
        default:
            throw Exception(Exception::cMissingOpenBraceBefore, StripBlahtexSuffix(command));
    }
}

auto_ptr<ParseTree::TextNode> Parser::ParseTextList()
{
    auto_ptr<ParseTree::TextList> output(new ParseTree::TextList);

    while (true)
    {
        switch (GetTextTokenCode(mTokenSource->Peek()))
        {
            case cEndGroup:
            case cEndOfInput:
            {
                if (output->mChildren.size() == 1)
                {
                    // If there's only node in the list, return just that single node.
                    // (We need to actually remove it from output->mChildren so that output's destructor
                    // doesn't delete it *again*).
                    ParseTree::TextNode* temp = output->mChildren.back();
                    output->mChildren.pop_back();
                    return auto_ptr<ParseTree::TextNode>(temp);
                }
                else
                    return static_cast<auto_ptr<ParseTree::TextNode> >(output);
            }

            case cNewcommand:
            {
                mTokenSource->HandleNewcommand();
                break;
            }
        
            case cBeginGroup:
            {
                mTokenSource->Advance();
                output->mChildren.push_back(new ParseTree::TextGroup(ParseTextList()));
                if (mTokenSource->Peek() != L"}")
                    throw Exception(Exception::cUnmatchedOpenBrace);
                mTokenSource->Advance();
                break;
            }

            case cWhitespace:
            case cSymbol:
            case cSymbolUnsafe:
            {
                output->mChildren.push_back(new ParseTree::TextSymbol(mTokenSource->Get()));
                break;
            }

            case cCommand1Arg:
            {
                wstring command = mTokenSource->Get();
                output->mChildren.push_back(new ParseTree::TextCommand1Arg(command, ParseTextField()));
                break;
            }

            case cStyleChange:
            {
                wstring command = mTokenSource->Get();
                output->mChildren.push_back(new ParseTree::TextStyleChange(command, ParseTextList()));
                break;
            }
            
            default:
                throw logic_error("Unexpected token code in Parser::ParseTextList");
        }
    }

    // Hmmm... gcc seems to think the control flow can reach here; I beg to differ.
    throw logic_error("Unexpected control flow in Parser::ParseTextField");
}

wstring StripBlahtexSuffix(const wstring& input)
{
    if (input.size() >= 7 && input.substr(input.size() - 7, 7) == L"Blahtex")
        return input.substr(0, input.size() - 7);
    else
        return input;
}

}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@