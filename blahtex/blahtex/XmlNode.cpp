// File "XmlNode.cpp"
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

using namespace std;

namespace blahtex
{

struct UnicodeNameInfo
{
    wstring mShortName;
    wstring mLongName;
    
    UnicodeNameInfo() { }
    UnicodeNameInfo(const wstring& shortName) :
        mShortName(shortName) { }
    UnicodeNameInfo(const wstring& shortName, const wstring& longName) :
        mShortName(shortName), mLongName(longName) { }
};

pair<wchar_t, UnicodeNameInfo> gUnicodeNameArray[] =
{
    make_pair(L'\U00000060', UnicodeNameInfo(L"grave",     L"DiacriticalGrave")),
    make_pair(L'\U000000A0', UnicodeNameInfo(L"nbsp",      L"NonBreakingSpace")),
    make_pair(L'\U000000A7', UnicodeNameInfo(L"sect")),
    make_pair(L'\U000000AC', UnicodeNameInfo(L"not")),
    make_pair(L'\U000000AF', UnicodeNameInfo(L"macr",      L"OverBar")),
    make_pair(L'\U000000B1', UnicodeNameInfo(L"pm",        L"PlusMinus")),
    make_pair(L'\U000000B4', UnicodeNameInfo(L"acute",     L"DiacriticalAcute")),
    make_pair(L'\U000000B6', UnicodeNameInfo(L"para")),
    make_pair(L'\U000000B7', UnicodeNameInfo(L"middot",    L"CenterDot")),
    make_pair(L'\U000000D7', UnicodeNameInfo(L"times")),
    make_pair(L'\U000000D8', UnicodeNameInfo(L"Oslash")),
    make_pair(L'\U000000F0', UnicodeNameInfo(L"eth")),
    make_pair(L'\U000000F7', UnicodeNameInfo(L"div",       L"divide")),
    make_pair(L'\U00000127', UnicodeNameInfo(L"hstrok")),
    make_pair(L'\U00000131', UnicodeNameInfo(L"imath")),
    make_pair(L'\U000002C7', UnicodeNameInfo(L"caron")),
    make_pair(L'\U000002D8', UnicodeNameInfo(L"breve")),
    make_pair(L'\U000002DC', UnicodeNameInfo(L"tilde",     L"DiacriticalTilde")),
    make_pair(L'\U00000393', UnicodeNameInfo(L"Gamma")),
    make_pair(L'\U00000394', UnicodeNameInfo(L"Delta")),
    make_pair(L'\U00000398', UnicodeNameInfo(L"Theta")),
    make_pair(L'\U0000039B', UnicodeNameInfo(L"Lambda")),
    make_pair(L'\U0000039E', UnicodeNameInfo(L"Xi")),
    make_pair(L'\U000003A0', UnicodeNameInfo(L"Pi")),
    make_pair(L'\U000003A3', UnicodeNameInfo(L"Sigma")),
    make_pair(L'\U000003A5', UnicodeNameInfo(L"Upsilon")),
    make_pair(L'\U000003A6', UnicodeNameInfo(L"Phi")),
    make_pair(L'\U000003A8', UnicodeNameInfo(L"Psi")),
    make_pair(L'\U000003A9', UnicodeNameInfo(L"Omega")),
    make_pair(L'\U000003B1', UnicodeNameInfo(L"alpha")),
    make_pair(L'\U000003B2', UnicodeNameInfo(L"beta")),
    make_pair(L'\U000003B3', UnicodeNameInfo(L"gamma")),
    make_pair(L'\U000003B4', UnicodeNameInfo(L"delta")),
    make_pair(L'\U000003B5', UnicodeNameInfo(L"epsiv",     L"varepsilon")),
    make_pair(L'\U000003B6', UnicodeNameInfo(L"zeta")),
    make_pair(L'\U000003B7', UnicodeNameInfo(L"eta")),
    make_pair(L'\U000003B8', UnicodeNameInfo(L"theta")),
    make_pair(L'\U000003B9', UnicodeNameInfo(L"iota")),
    make_pair(L'\U000003BA', UnicodeNameInfo(L"kappa")),
    make_pair(L'\U000003BB', UnicodeNameInfo(L"lambda")),
    make_pair(L'\U000003BC', UnicodeNameInfo(L"mu")),
    make_pair(L'\U000003BD', UnicodeNameInfo(L"nu")),
    make_pair(L'\U000003BE', UnicodeNameInfo(L"xi")),
    make_pair(L'\U000003C0', UnicodeNameInfo(L"pi")),
    make_pair(L'\U000003C1', UnicodeNameInfo(L"rho")),
    make_pair(L'\U000003C2', UnicodeNameInfo(L"sigmav",    L"varsigma")),
    make_pair(L'\U000003C3', UnicodeNameInfo(L"sigma")),
    make_pair(L'\U000003C4', UnicodeNameInfo(L"tau")),
    make_pair(L'\U000003C5', UnicodeNameInfo(L"upsi",      L"upsilon")),
    make_pair(L'\U000003C6', UnicodeNameInfo(L"phiv",      L"varphi")),
    make_pair(L'\U000003C7', UnicodeNameInfo(L"chi")),
    make_pair(L'\U000003C8', UnicodeNameInfo(L"psi")),
    make_pair(L'\U000003C9', UnicodeNameInfo(L"omega")),
    make_pair(L'\U000003D1', UnicodeNameInfo(L"thetav",    L"vartheta")),
    make_pair(L'\U000003D5', UnicodeNameInfo(L"phi",       L"straightphi")),
    make_pair(L'\U000003D6', UnicodeNameInfo(L"piv",       L"varpi")),
    make_pair(L'\U000003DD', UnicodeNameInfo(L"gammad",    L"digamma")),
    make_pair(L'\U000003F0', UnicodeNameInfo(L"kappav",    L"varkappa")),
    make_pair(L'\U000003F1', UnicodeNameInfo(L"rhov",      L"varrho")),
    make_pair(L'\U000003F5', UnicodeNameInfo(L"epsi",      L"straightepsilon")),
    make_pair(L'\U00002020', UnicodeNameInfo(L"dagger")),
    make_pair(L'\U00002021', UnicodeNameInfo(L"Dagger",    L"ddagger")),
    make_pair(L'\U00002022', UnicodeNameInfo(L"bull",      L"bullet")),
    make_pair(L'\U00002032', UnicodeNameInfo(L"prime")),
    make_pair(L'\U000020D7', UnicodeNameInfo(L"WRONG",     L"WRONG")),
    make_pair(L'\U0000210F', UnicodeNameInfo(L"hbar",      L"planck")),
    make_pair(L'\U00002111', UnicodeNameInfo(L"Im",        L"imagpart")),
    make_pair(L'\U00002113', UnicodeNameInfo(L"ell")),
    make_pair(L'\U00002118', UnicodeNameInfo(L"wp",        L"weierp")),
    make_pair(L'\U0000211C', UnicodeNameInfo(L"Re",        L"realpart")),
    make_pair(L'\U00002127', UnicodeNameInfo(L"mho")),
    make_pair(L'\U00002135', UnicodeNameInfo(L"aleph")),
    make_pair(L'\U00002136', UnicodeNameInfo(L"beth")),
    make_pair(L'\U00002137', UnicodeNameInfo(L"gimel")),
    make_pair(L'\U00002138', UnicodeNameInfo(L"daleth")),
    make_pair(L'\U00002190', UnicodeNameInfo(L"larr",      L"LeftArrow")),
    make_pair(L'\U00002191', UnicodeNameInfo(L"uarr",      L"UpArrow")),
    make_pair(L'\U00002192', UnicodeNameInfo(L"rarr",      L"RightArrow")),
    make_pair(L'\U00002193', UnicodeNameInfo(L"darr",      L"DownArrow")),
    make_pair(L'\U00002194', UnicodeNameInfo(L"harr",      L"LeftRightArrow")),
    make_pair(L'\U00002195', UnicodeNameInfo(L"varr",      L"UpDownArrow")),
    make_pair(L'\U00002196', UnicodeNameInfo(L"nwarr",     L"UpperLeftArrow")),
    make_pair(L'\U00002197', UnicodeNameInfo(L"nearr",     L"UpperRightArrow")),
    make_pair(L'\U00002198', UnicodeNameInfo(L"searr",     L"LowerRightArrow")),
    make_pair(L'\U00002199', UnicodeNameInfo(L"swarr",     L"LowerLeftArrow")),
    make_pair(L'\U0000219A', UnicodeNameInfo(L"nlarr",     L"nleftarrow")),
    make_pair(L'\U0000219B', UnicodeNameInfo(L"nrarr",     L"nrightarrow")),
    make_pair(L'\U000021A6', UnicodeNameInfo(L"map",       L"RightTeeArrow")),
    make_pair(L'\U000021A6', UnicodeNameInfo(L"map",       L"RightTeeArrow")),
    make_pair(L'\U000021A9', UnicodeNameInfo(L"larrhk",    L"hookleftarrow")),
    make_pair(L'\U000021AA', UnicodeNameInfo(L"rarrhk",    L"hookrightarrow")),
    make_pair(L'\U000021AE', UnicodeNameInfo(L"nharr",     L"nleftrightarrow")),
    make_pair(L'\U000021BC', UnicodeNameInfo(L"lharu",     L"leftharpoonup")),
    make_pair(L'\U000021BD', UnicodeNameInfo(L"lhard",     L"leftharpoondown")),
    make_pair(L'\U000021BE', UnicodeNameInfo(L"uharr",     L"upharpoonright")),
    make_pair(L'\U000021BF', UnicodeNameInfo(L"uharl",     L"upharpoonleft")),
    make_pair(L'\U000021C0', UnicodeNameInfo(L"rharu",     L"rightharpoonup")),
    make_pair(L'\U000021C1', UnicodeNameInfo(L"rhard",     L"rightharpoondown")),
    make_pair(L'\U000021C2', UnicodeNameInfo(L"dharr",     L"downharpoonright")),
    make_pair(L'\U000021C3', UnicodeNameInfo(L"dharl",     L"downharpoonleft")),
    make_pair(L'\U000021D0', UnicodeNameInfo(L"lArr",      L"DoubleLeftArrow")),
    make_pair(L'\U000021D1', UnicodeNameInfo(L"uArr",      L"DoubleUpArrow")),
    make_pair(L'\U000021D2', UnicodeNameInfo(L"rArr",      L"DoubleRightArrow")),
    make_pair(L'\U000021D3', UnicodeNameInfo(L"dArr",      L"DoubleDownArrow")),
    make_pair(L'\U000021D4', UnicodeNameInfo(L"hArr",      L"DoubleLeftRightArrow")),
    make_pair(L'\U000021D5', UnicodeNameInfo(L"vArr",      L"DoubleUpDownArrow")),
    make_pair(L'\U00002200', UnicodeNameInfo(L"forall",    L"ForAll")),
    make_pair(L'\U00002201', UnicodeNameInfo(L"comp",      L"complement")),
    make_pair(L'\U00002202', UnicodeNameInfo(L"part",      L"PartialD")),
    make_pair(L'\U00002203', UnicodeNameInfo(L"exist",     L"Exists")),
    make_pair(L'\U00002204', UnicodeNameInfo(L"nexist",    L"NotExists")),
    make_pair(L'\U00002205', UnicodeNameInfo(L"empty",     L"emptyset")),
    make_pair(L'\U00002207', UnicodeNameInfo(L"nabla",     L"Del")),
    make_pair(L'\U00002208', UnicodeNameInfo(L"in",        L"Element")),
    make_pair(L'\U00002209', UnicodeNameInfo(L"notin",     L"NotElement")),
    make_pair(L'\U0000220B', UnicodeNameInfo(L"ni",        L"ReverseElement")),
    make_pair(L'\U0000220F', UnicodeNameInfo(L"prod",      L"Product")),
    make_pair(L'\U00002210', UnicodeNameInfo(L"coprod",    L"Coproduct")),
    make_pair(L'\U00002211', UnicodeNameInfo(L"sum",       L"Sum")),
    make_pair(L'\U00002213', UnicodeNameInfo(L"mp",        L"MinusPlus")),
    make_pair(L'\U00002216', UnicodeNameInfo(L"setmn",     L"Backslash")),
    make_pair(L'\U00002218', UnicodeNameInfo(L"compfn",    L"SmallCircle")),
    make_pair(L'\U0000221D', UnicodeNameInfo(L"prop",      L"Proportional")),
    make_pair(L'\U0000221E', UnicodeNameInfo(L"infin")),
    make_pair(L'\U00002220', UnicodeNameInfo(L"ang",       L"angle")),
    make_pair(L'\U00002224', UnicodeNameInfo(L"nmid",      L"NotVerticalBar")),
    make_pair(L'\U00002225', UnicodeNameInfo(L"par",       L"DoubleVerticalBar")),
    make_pair(L'\U00002227', UnicodeNameInfo(L"and",       L"wedge")),
    make_pair(L'\U00002228', UnicodeNameInfo(L"or",        L"vee")),
    make_pair(L'\U00002229', UnicodeNameInfo(L"cap")),
    make_pair(L'\U0000222A', UnicodeNameInfo(L"cup")),
    make_pair(L'\U0000222B', UnicodeNameInfo(L"int",       L"Integral")),
    make_pair(L'\U0000222C', UnicodeNameInfo(L"Int")),
    make_pair(L'\U0000222D', UnicodeNameInfo(L"tint",      L"iiint")),
    make_pair(L'\U0000222E', UnicodeNameInfo(L"conint",    L"ContourIntegral")),
    make_pair(L'\U0000223C', UnicodeNameInfo(L"sim",       L"Tilde")),
    make_pair(L'\U00002240', UnicodeNameInfo(L"wr",        L"VerticalTilde")),
    make_pair(L'\U00002241', UnicodeNameInfo(L"nsim",      L"NotTilde")),
    make_pair(L'\U00002243', UnicodeNameInfo(L"sime",      L"TildeEqual")),
    make_pair(L'\U00002245', UnicodeNameInfo(L"cong",      L"TildeFullEqual")),
    make_pair(L'\U00002248', UnicodeNameInfo(L"ap",        L"TildeTilde")),
    make_pair(L'\U00002260', UnicodeNameInfo(L"ne",        L"NotEqual")),
    make_pair(L'\U00002262', UnicodeNameInfo(L"nequiv",    L"NotCongruent")),
    make_pair(L'\U00002264', UnicodeNameInfo(L"le",        L"leq")),
    make_pair(L'\U00002265', UnicodeNameInfo(L"ge",        L"GreaterEqual")),
    make_pair(L'\U0000226A', UnicodeNameInfo(L"Lt",        L"NestedLessLess")),
    make_pair(L'\U0000226B', UnicodeNameInfo(L"Gt",        L"NestedGreaterGreater")),
    make_pair(L'\U0000226E', UnicodeNameInfo(L"nlt",       L"NotLess")),
    make_pair(L'\U00002270', UnicodeNameInfo(L"nle",       L"NotLessEqual")),
    make_pair(L'\U00002271', UnicodeNameInfo(L"nge",       L"NotGreaterEqual")),
    make_pair(L'\U00002272', UnicodeNameInfo(L"lsim",      L"LessTilde")),
    make_pair(L'\U00002282', UnicodeNameInfo(L"sub",       L"subset")),
    make_pair(L'\U00002283', UnicodeNameInfo(L"sup",       L"supset")),
    make_pair(L'\U00002286', UnicodeNameInfo(L"sube",      L"SubsetEqual")),
    make_pair(L'\U00002287', UnicodeNameInfo(L"supe",      L"SupersetEqual")),
    make_pair(L'\U00002288', UnicodeNameInfo(L"nsube",     L"NotSubsetEqual")),
    make_pair(L'\U0000228A', UnicodeNameInfo(L"subne",     L"subsetneq")),
    make_pair(L'\U0000228B', UnicodeNameInfo(L"supne",     L"supsetneq")),
    make_pair(L'\U0000228F', UnicodeNameInfo(L"sqsub",     L"SquareSubset")),
    make_pair(L'\U00002290', UnicodeNameInfo(L"sqsup",     L"SquareSuperset")),
    make_pair(L'\U00002291', UnicodeNameInfo(L"sqsube",    L"SquareSubsetEqual")),
    make_pair(L'\U00002292', UnicodeNameInfo(L"sqsupe",    L"SquareSupersetEqual")),
    make_pair(L'\U00002293', UnicodeNameInfo(L"sqcap",     L"SquareIntersection")),
    make_pair(L'\U00002294', UnicodeNameInfo(L"sqcup",     L"SquareUnion")),
    make_pair(L'\U00002295', UnicodeNameInfo(L"oplus",     L"CirclePlus")),
    make_pair(L'\U00002297', UnicodeNameInfo(L"otimes",    L"CircleTimes")),
    make_pair(L'\U000022A2', UnicodeNameInfo(L"vdash",     L"RightTee")),
    make_pair(L'\U000022A4', UnicodeNameInfo(L"top",       L"DownTee")),
    make_pair(L'\U000022A5', UnicodeNameInfo(L"bot",       L"UpTee")),
    make_pair(L'\U000022A5', UnicodeNameInfo(L"bot",       L"UpTee")),
    make_pair(L'\U000022A7', UnicodeNameInfo(L"models")),
    make_pair(L'\U000022A8', UnicodeNameInfo(L"vDash",     L"DoubleRightTee")),
    make_pair(L'\U000022A9', UnicodeNameInfo(L"Vdash")),
    make_pair(L'\U000022AE', UnicodeNameInfo(L"nVdash")),
    make_pair(L'\U000022B2', UnicodeNameInfo(L"vltri",     L"LeftTriangle")),
    make_pair(L'\U000022B3', UnicodeNameInfo(L"vrtri",     L"RightTriangle")),
    make_pair(L'\U000022B4', UnicodeNameInfo(L"ltrie",     L"LeftTriangleEqual")),
    make_pair(L'\U000022B5', UnicodeNameInfo(L"rtrie",     L"RightTriangleEqual")),
    make_pair(L'\U000022C0', UnicodeNameInfo(L"xwedge",    L"Wedge")),
    make_pair(L'\U000022C1', UnicodeNameInfo(L"xvee",      L"Vee")),
    make_pair(L'\U000022C2', UnicodeNameInfo(L"xcap",      L"Intersection")),
    make_pair(L'\U000022C3', UnicodeNameInfo(L"xcup",      L"Union")),
    make_pair(L'\U000022C4', UnicodeNameInfo(L"diam",      L"Diamond")),
    make_pair(L'\U000022C5', UnicodeNameInfo(L"sdot")),
    make_pair(L'\U000022C6', UnicodeNameInfo(L"Star")),
    make_pair(L'\U000022EE', UnicodeNameInfo(L"vellip")),
    make_pair(L'\U000022F1', UnicodeNameInfo(L"dtdot")),
    make_pair(L'\U00002308', UnicodeNameInfo(L"lceil",     L"LeftCeiling")),
    make_pair(L'\U00002309', UnicodeNameInfo(L"rceil",     L"RightCeiling")),
    make_pair(L'\U0000230A', UnicodeNameInfo(L"lfloor",    L"LeftFloor")),
    make_pair(L'\U0000230B', UnicodeNameInfo(L"rfloor",    L"RightFloor")),
    make_pair(L'\U00002322', UnicodeNameInfo(L"frown",     L"sfrown")),
    make_pair(L'\U00002323', UnicodeNameInfo(L"smile",     L"ssmile")),
    make_pair(L'\U00002329', UnicodeNameInfo(L"lang",      L"LeftAngleBracket")),
    make_pair(L'\U0000232A', UnicodeNameInfo(L"rang",      L"RightAngleBracket")),
    make_pair(L'\U000023B5', UnicodeNameInfo(L"bbrk",      L"UnderBracket")),
    make_pair(L'\U000025A1', UnicodeNameInfo(L"squ",       L"Square")),
    make_pair(L'\U000025B3', UnicodeNameInfo(L"xutri",     L"bigtriangleup")),
    make_pair(L'\U00002660', UnicodeNameInfo(L"spades",    L"spadesuit")),
    make_pair(L'\U00002663', UnicodeNameInfo(L"clubs",     L"clubsuit")),
    make_pair(L'\U00002665', UnicodeNameInfo(L"hearts",    L"heartsuit")),
    make_pair(L'\U00002666', UnicodeNameInfo(L"diams",     L"diamondsuit")),
    make_pair(L'\U0000266D', UnicodeNameInfo(L"flat")),
    make_pair(L'\U0000266E', UnicodeNameInfo(L"natur",     L"natural")),
    make_pair(L'\U0000266F', UnicodeNameInfo(L"sharp")),
    make_pair(L'\U00002713', UnicodeNameInfo(L"check",     L"checkmark")),
    make_pair(L'\U00002A00', UnicodeNameInfo(L"xodot",     L"bigodot")),
    make_pair(L'\U00002A01', UnicodeNameInfo(L"xoplus",    L"bigoplus")),
    make_pair(L'\U00002A02', UnicodeNameInfo(L"xotime",    L"bigotimes")),
    make_pair(L'\U00002A04', UnicodeNameInfo(L"xuplus",    L"biguplus")),
    make_pair(L'\U00002A06', UnicodeNameInfo(L"xsqcup",    L"bigsqcup")),
    make_pair(L'\U00002A0C', UnicodeNameInfo(L"qint",      L"iiiint")),
    make_pair(L'\U0000FE37', UnicodeNameInfo(L"OverBrace")),
    make_pair(L'\U0000FE38', UnicodeNameInfo(L"UnderBrace")),
    
    // Mathematical bold script capital letters:
    make_pair(L'\U0001D4D0', UnicodeNameInfo()),
    make_pair(L'\U0001D4D1', UnicodeNameInfo()),
    make_pair(L'\U0001D4D2', UnicodeNameInfo()),
    make_pair(L'\U0001D4D3', UnicodeNameInfo()),
    make_pair(L'\U0001D4D4', UnicodeNameInfo()),
    make_pair(L'\U0001D4D5', UnicodeNameInfo()),
    make_pair(L'\U0001D4D6', UnicodeNameInfo()),
    make_pair(L'\U0001D4D7', UnicodeNameInfo()),
    make_pair(L'\U0001D4D8', UnicodeNameInfo()),
    make_pair(L'\U0001D4D9', UnicodeNameInfo()),
    make_pair(L'\U0001D4DA', UnicodeNameInfo()),
    make_pair(L'\U0001D4DB', UnicodeNameInfo()),
    make_pair(L'\U0001D4DC', UnicodeNameInfo()),
    make_pair(L'\U0001D4DD', UnicodeNameInfo()),
    make_pair(L'\U0001D4DE', UnicodeNameInfo()),
    make_pair(L'\U0001D4DF', UnicodeNameInfo()),
    make_pair(L'\U0001D4E0', UnicodeNameInfo()),
    make_pair(L'\U0001D4E1', UnicodeNameInfo()),
    make_pair(L'\U0001D4E2', UnicodeNameInfo()),
    make_pair(L'\U0001D4E3', UnicodeNameInfo()),
    make_pair(L'\U0001D4E4', UnicodeNameInfo()),
    make_pair(L'\U0001D4E5', UnicodeNameInfo()),
    make_pair(L'\U0001D4E6', UnicodeNameInfo()),
    make_pair(L'\U0001D4E7', UnicodeNameInfo()),
    make_pair(L'\U0001D4E8', UnicodeNameInfo()),
    make_pair(L'\U0001D4E9', UnicodeNameInfo()),
    
    make_pair(L'\U0001D49C', UnicodeNameInfo(L"Ascr")),
    make_pair(L'\U0000212C', UnicodeNameInfo(L"Bscr", L"Bernoullis")),
    make_pair(L'\U0001D49E', UnicodeNameInfo(L"Cscr")),
    make_pair(L'\U0001D49F', UnicodeNameInfo(L"Dscr")),
    make_pair(L'\U00002130', UnicodeNameInfo(L"Escr", L"expectation")),
    make_pair(L'\U00002131', UnicodeNameInfo(L"Fscr", L"Fouriertrf")),
    make_pair(L'\U0001D4A2', UnicodeNameInfo(L"Gscr")),
    make_pair(L'\U0000210B', UnicodeNameInfo(L"Hscr", L"HilbertSpace")),
    make_pair(L'\U00002110', UnicodeNameInfo(L"Iscr", L"imagline")),
    make_pair(L'\U0001D4A5', UnicodeNameInfo(L"Jscr")),
    make_pair(L'\U0001D4A6', UnicodeNameInfo(L"Kscr")),
    make_pair(L'\U00002112', UnicodeNameInfo(L"Lscr", L"Laplacetrf")),
    make_pair(L'\U00002133', UnicodeNameInfo(L"Mscr", L"Mellintrf")),
    make_pair(L'\U0001D4A9', UnicodeNameInfo(L"Nscr")),
    make_pair(L'\U0001D4AA', UnicodeNameInfo(L"Oscr")),
    make_pair(L'\U0001D4AB', UnicodeNameInfo(L"Pscr")),
    make_pair(L'\U0001D4AC', UnicodeNameInfo(L"Qscr")),
    make_pair(L'\U0000211B', UnicodeNameInfo(L"Rscr", L"realine")),
    make_pair(L'\U0001D4AE', UnicodeNameInfo(L"Sscr")),
    make_pair(L'\U0001D4AF', UnicodeNameInfo(L"Tscr")),
    make_pair(L'\U0001D4B0', UnicodeNameInfo(L"Uscr")),
    make_pair(L'\U0001D4B1', UnicodeNameInfo(L"Vscr")),
    make_pair(L'\U0001D4B2', UnicodeNameInfo(L"Wscr")),
    make_pair(L'\U0001D4B3', UnicodeNameInfo(L"Xscr")),
    make_pair(L'\U0001D4B4', UnicodeNameInfo(L"Yscr")),
    make_pair(L'\U0001D4B5', UnicodeNameInfo(L"Zscr")),
    
    make_pair(L'\U0001D504', UnicodeNameInfo(L"Afr")),
    make_pair(L'\U0001D505', UnicodeNameInfo(L"Bfr")),
    make_pair(L'\U0000212D', UnicodeNameInfo(L"Cfr", L"Cayleys")),
    make_pair(L'\U0001D507', UnicodeNameInfo(L"Dfr")),
    make_pair(L'\U0001D508', UnicodeNameInfo(L"Efr")),
    make_pair(L'\U0001D509', UnicodeNameInfo(L"Ffr")),
    make_pair(L'\U0001D50A', UnicodeNameInfo(L"Gfr")),
    make_pair(L'\U0000210C', UnicodeNameInfo(L"Hfr", L"Poincareplane")),
//  make_pair(L'\U00002111', UnicodeNameInfo(L"Ifr")),      // already included as "Im"
    make_pair(L'\U0001D50D', UnicodeNameInfo(L"Jfr")),
    make_pair(L'\U0001D50E', UnicodeNameInfo(L"Kfr")),
    make_pair(L'\U0001D50F', UnicodeNameInfo(L"Lfr")),
    make_pair(L'\U0001D510', UnicodeNameInfo(L"Mfr")),
    make_pair(L'\U0001D511', UnicodeNameInfo(L"Nfr")),
    make_pair(L'\U0001D512', UnicodeNameInfo(L"Ofr")),
    make_pair(L'\U0001D513', UnicodeNameInfo(L"Pfr")),
    make_pair(L'\U0001D514', UnicodeNameInfo(L"Qfr")),
//  make_pair(L'\U0000211C', UnicodeNameInfo(L"Rfr")),      // already included as "Re"
    make_pair(L'\U0001D516', UnicodeNameInfo(L"Sfr")),
    make_pair(L'\U0001D517', UnicodeNameInfo(L"Tfr")),
    make_pair(L'\U0001D518', UnicodeNameInfo(L"Ufr")),
    make_pair(L'\U0001D519', UnicodeNameInfo(L"Vfr")),
    make_pair(L'\U0001D51A', UnicodeNameInfo(L"Wfr")),
    make_pair(L'\U0001D51B', UnicodeNameInfo(L"Xfr")),
    make_pair(L'\U0001D51C', UnicodeNameInfo(L"Yfr")),
    make_pair(L'\U00002128', UnicodeNameInfo(L"Zfr", L"zeetrf")),
    
    make_pair(L'\U0001D51E', UnicodeNameInfo(L"afr")),
    make_pair(L'\U0001D51F', UnicodeNameInfo(L"bfr")),
    make_pair(L'\U0001D520', UnicodeNameInfo(L"cfr")),
    make_pair(L'\U0001D521', UnicodeNameInfo(L"dfr")),
    make_pair(L'\U0001D522', UnicodeNameInfo(L"efr")),
    make_pair(L'\U0001D523', UnicodeNameInfo(L"ffr")),
    make_pair(L'\U0001D524', UnicodeNameInfo(L"gfr")),
    make_pair(L'\U0001D525', UnicodeNameInfo(L"hfr")),
    make_pair(L'\U0001D526', UnicodeNameInfo(L"ifr")),
    make_pair(L'\U0001D527', UnicodeNameInfo(L"jfr")),
    make_pair(L'\U0001D528', UnicodeNameInfo(L"kfr")),
    make_pair(L'\U0001D529', UnicodeNameInfo(L"lfr")),
    make_pair(L'\U0001D52A', UnicodeNameInfo(L"mfr")),
    make_pair(L'\U0001D52B', UnicodeNameInfo(L"nfr")),
    make_pair(L'\U0001D52C', UnicodeNameInfo(L"ofr")),
    make_pair(L'\U0001D52D', UnicodeNameInfo(L"pfr")),
    make_pair(L'\U0001D52E', UnicodeNameInfo(L"qfr")),
    make_pair(L'\U0001D52F', UnicodeNameInfo(L"rfr")),
    make_pair(L'\U0001D530', UnicodeNameInfo(L"sfr")),
    make_pair(L'\U0001D531', UnicodeNameInfo(L"tfr")),
    make_pair(L'\U0001D532', UnicodeNameInfo(L"ufr")),
    make_pair(L'\U0001D533', UnicodeNameInfo(L"vfr")),
    make_pair(L'\U0001D534', UnicodeNameInfo(L"wfr")),
    make_pair(L'\U0001D535', UnicodeNameInfo(L"xfr")),
    make_pair(L'\U0001D536', UnicodeNameInfo(L"yfr")),
    make_pair(L'\U0001D537', UnicodeNameInfo(L"zfr")),
    
    make_pair(L'\U0001D538', UnicodeNameInfo(L"Aopf")),
    make_pair(L'\U0001D539', UnicodeNameInfo(L"Bopf")),
    make_pair(L'\U00002102', UnicodeNameInfo(L"Copf", L"complexes")),
    make_pair(L'\U0001D53B', UnicodeNameInfo(L"Dopf")),
    make_pair(L'\U0001D53C', UnicodeNameInfo(L"Eopf")),
    make_pair(L'\U0001D53D', UnicodeNameInfo(L"Fopf")),
    make_pair(L'\U0001D53E', UnicodeNameInfo(L"Gopf")),
    make_pair(L'\U0000210D', UnicodeNameInfo(L"Hopf", L"quaternions")),
    make_pair(L'\U0001D540', UnicodeNameInfo(L"Iopf")),
    make_pair(L'\U0001D541', UnicodeNameInfo(L"Jopf")),
    make_pair(L'\U0001D542', UnicodeNameInfo(L"Kopf")),
    make_pair(L'\U0001D543', UnicodeNameInfo(L"Lopf")),
    make_pair(L'\U0001D544', UnicodeNameInfo(L"Mopf")),
    make_pair(L'\U0001D546', UnicodeNameInfo(L"Oopf")),
    make_pair(L'\U00002119', UnicodeNameInfo(L"Popf", L"primes")),
    make_pair(L'\U0000211A', UnicodeNameInfo(L"Qopf", L"rationals")),
    make_pair(L'\U0000211D', UnicodeNameInfo(L"Ropf", L"reals")),
    make_pair(L'\U0001D54A', UnicodeNameInfo(L"Sopf")),
    make_pair(L'\U0001D54B', UnicodeNameInfo(L"Topf")),
    make_pair(L'\U0001D54C', UnicodeNameInfo(L"Uopf")),
    make_pair(L'\U0001D54D', UnicodeNameInfo(L"Vopf")),
    make_pair(L'\U0001D54E', UnicodeNameInfo(L"Wopf")),
    make_pair(L'\U0001D54F', UnicodeNameInfo(L"Xopf")),
    make_pair(L'\U0001D550', UnicodeNameInfo(L"Yopf")),
    make_pair(L'\U00002124', UnicodeNameInfo(L"Zopf", L"integers"))
};

wishful_hash_map<wchar_t, UnicodeNameInfo> gUnicodeNameTable(gUnicodeNameArray, END_ARRAY(gUnicodeNameArray));

XmlNode::~XmlNode()
{
    for (list<XmlNode*>::iterator p = mChildren.begin(); p != mChildren.end(); p++)
        delete *p;
}

void WriteIndent(wostream& os, int depth)
{
    os << L'\n';
    for (int i = 0; i < depth; i++)
        os << L"  ";
}

void XmlNode::Print(wostream& os, const MathmlOptions& options, bool indent, int depth) const {
    if (mType == cString)
    {
        for (wstring::const_iterator ptr = mText.begin(); ptr != mText.end(); ptr++)
        {
            if (*ptr == L'&')
                os << L"&amp;";
            else if (*ptr == L'<')
                os << L"&lt;";
            else if (*ptr == L'>')
                os << L"&gt;";
            else if (*ptr <= 0x7F)
                os << *ptr;
            else
            {
                wishful_hash_map<wchar_t, UnicodeNameInfo>::const_iterator search = gUnicodeNameTable.find(*ptr);
                if (search == gUnicodeNameTable.end())
                {
                    if (options.mOtherEncodingRaw)
                        os << *ptr;
                    else
                        os << L"&#x" << hex << static_cast<unsigned>(*ptr) << L";";
                }
                else
                {
                    switch (options.mMathmlEncoding)
                    {
                        case cMathmlEncodingLong:
                            if (!search->second.mLongName.empty())
                            {
                                os << L"&" << search->second.mLongName << L";";
                                break;
                            }

                        case cMathmlEncodingShort:
                            if (!search->second.mShortName.empty())
                            {
                                os << L"&" << search->second.mShortName << L";";
                                break;
                            }
                            
                        case cMathmlEncodingNumeric:
                            os << L"&#x" << hex << static_cast<unsigned>(*ptr) << L";";
                            break;

                        case cMathmlEncodingRaw:
                            os << *ptr;
                            break;                            
                    }
                    
                }
            }
        }

    }
    else
    {
        os << L"<" << mText;
        for (map<wstring, wstring>::const_iterator attribute = mAttributes.begin(); attribute != mAttributes.end(); attribute++)
            os << L" " << attribute->first << L"=\"" << attribute->second << L"\"";
        
        if (mChildren.empty())
        {
            os << L"/>";
            return;
        }
        
        os << L">";
        
        bool justWroteString = false;
        for (list<XmlNode*>::const_iterator child = mChildren.begin(); child != mChildren.end(); child++)
        {
            bool isTag = ((*child)->mType == cTag);
            if (isTag && !justWroteString && indent)
                WriteIndent(os, depth + 1);
            (*child)->Print(os, options, indent, depth + 1);
            justWroteString = !isTag;
        }
        
        if (!justWroteString && indent)
            WriteIndent(os, depth);
        
        os << L"</" << mText << L">";
    }
}

}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
