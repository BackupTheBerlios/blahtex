/*
File "layout.cpp"

blahtex (version 0.2.1): a LaTeX to MathML converter designed specifically for MediaWiki
Copyright (C) 2005, David Harvey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "blahtex.h"

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Some lookup tables. All these are implemented as hash tables. */

/* This table lists all latex commands that should be interpreted as identifiers.
The boolean column indicates whether the default font is italic. */

dictionary_item<identifier_info> identifier_data[] = {
	{L"\\alpha",          {true,    L"&alpha;"}},
	{L"\\beta",           {true,    L"&beta;"}},
	{L"\\gamma",          {true,    L"&gamma;"}},
	{L"\\delta",          {true,    L"&delta;"}},
	{L"\\epsilon",        {true,    L"&epsilon;"}},
	{L"\\varepsilon",     {true,    L"&varepsilon;"}},
	{L"\\zeta",           {true,    L"&zeta;"}},
	{L"\\eta",            {true,    L"&eta;"}},
	{L"\\theta",          {true,    L"&theta;"}},
	{L"\\vartheta",       {true,    L"&vartheta;"}},
	{L"\\iota",           {true,    L"&iota;"}},
	{L"\\kappa",          {true,    L"&kappa;"}},
	{L"\\varkappa",       {true,    L"&varkappa;"}},
	{L"\\lambda",         {true,    L"&lambda;"}},
	{L"\\mu",             {true,    L"&mu;"}},
	{L"\\nu",             {true,    L"&nu;"}},
	{L"\\pi",             {true,    L"&pi;"}},
	{L"\\varpi",          {true,    L"&varpi;"}},
	{L"\\rho",            {true,    L"&rho;"}},
	{L"\\varrho",         {true,    L"&varrho;"}},
	{L"\\sigma",          {true,    L"&sigma;"}},
	{L"\\varsigma",       {true,    L"&varsigma;"}},
	{L"\\tau",            {true,    L"&tau;"}},
	{L"\\upsilon",        {true,    L"&upsilon;"}},
	{L"\\phi",            {true,    L"&phi;"}},
	{L"\\varphi",         {true,    L"&varphi;"}},
	{L"\\chi",            {true,    L"&chi;"}},
	{L"\\psi",            {true,    L"&psi;"}},
	{L"\\omega",          {true,    L"&omega;"}},
	{L"\\xi",             {true,    L"&xi;"}},
	{L"\\digamma",        {true,    L"&digamma;"}},
		
	{L"\\Alpha",          {false,   L"&Alpha;"}},
	{L"\\Beta",           {false,   L"&Beta;"}},
	{L"\\Gamma",          {false,   L"&Gamma;"}},
	{L"\\Delta",          {false,   L"&Delta;"}},
	{L"\\Epsilon",        {false,   L"&Epsilon;"}},
	{L"\\Zeta",           {false,   L"&Zeta;"}},
	{L"\\Eta",            {false,   L"&Eta;"}},
	{L"\\Theta",          {false,   L"&Theta;"}},
	{L"\\Iota",           {false,   L"&Iota;"}},
	{L"\\Kappa",          {false,   L"&Kappa;"}},
	{L"\\Lambda",         {false,   L"&Lambda;"}},
	{L"\\Mu",             {false,   L"&Mu;"}},
	{L"\\Nu",             {false,   L"&Nu;"}},
	{L"\\Pi",             {false,   L"&Pi;"}},
	{L"\\Rho",            {false,   L"&Rho;"}},
	{L"\\Sigma",          {false,   L"&Sigma;"}},
	{L"\\Tau",            {false,   L"&Tau;"}},
	{L"\\Upsilon",        {false,   L"&Upsilon;"}},
	{L"\\Phi",            {false,   L"&Phi;"}},
	{L"\\Chi",            {false,   L"&Chi;"}},
	{L"\\Psi",            {false,   L"&Psi;"}},
	{L"\\Omega",          {false,   L"&Omega;"}},
	{L"\\Xi",             {false,   L"&Xi;"}},
		
	{L"\\aleph",          {false,   L"&aleph;"}},
	{L"\\beth",           {false,   L"&beth;"}},
	{L"\\gimel",          {false,   L"&gimel;"}},
	{L"\\daleth",         {false,   L"&daleth;"}},
		
	{L"\\wp",             {true,    L"&weierp;"}},
	{L"\\AA",             {true,    L"&Aring;"}},
	{L"\\ell",            {true,    L"&ell;"}},
	{L"\\P",              {true,    L"&para;"}},
	{L"\\imath",          {true,    L"&imath;"}},
	{L"\\partial",        {false,   L"&part;"}},
	{L"\\infty",          {false,   L"&infin;"}},
};

hash_table<identifier_info> identifier_table(
	identifier_data, identifier_data + sizeof(identifier_data)/sizeof(identifier_data[0]));

/* This table lists all "\log"-style functions. They get rendered without italics, and with
an "&ApplyFunction;" operator after them. */

dictionary_item<wstring> function_data[] = {
	{L"\\ker",       L"ker"},
	{L"\\Pr",        L"Pr"},
	{L"\\hom",       L"hom"},
	{L"\\dim",       L"dim"},
	{L"\\arg",       L"arg"},
	{L"\\sin",       L"sin"},
	{L"\\cos",       L"cos"},
	{L"\\sec",       L"sec"},
	{L"\\csc",       L"csc"},
	{L"\\tan",       L"tan"},
	{L"\\cot",       L"cot"},
	{L"\\arcsin",    L"arcsin"},
	{L"\\arccos",    L"arccos"},
	{L"\\arcsec",    L"arcsec"},
	{L"\\arccsc",    L"arccsc"},
	{L"\\arctan",    L"arctan"},
	{L"\\arccot",    L"arccot"},
	{L"\\sinh",      L"sinh"},
	{L"\\cosh",      L"cosh"},
	{L"\\tanh",      L"tanh"},
	{L"\\coth",      L"coth"},
	{L"\\log",       L"log"},
	{L"\\lg",        L"lg"},
	{L"\\ln",        L"ln"},
	{L"\\exp",       L"exp"},
	{L"\\sgn",       L"sgn"},
	{L"\\gcd",       L"gcd"},
	{L"\\deg",       L"deg"},
	{L"\\det",       L"det"},
	{L"\\Re",        L"&Re;"},
	{L"\\Im",        L"&Im;"},	
};

hash_table<wstring> function_table(
	function_data, function_data + sizeof(function_data)/sizeof(function_data[0]));

/* This table lists all characters and commands that get mapped to <mo>...</mo> operators.
The boolean column indicates whether they have scripts under/over by default (rather than sub/sup).
*/
dictionary_item<operator_info> operator_data[] = {
	{L"$",                     {false,   L"$"}},
	{L"%",                     {false,   L"%"}},
	{L",",                     {false,   L","}},
	{L".",                     {false,   L"."}},
	{L"+",                     {false,   L"+"}},
	{L"*",                     {false,   L"*"}},
	{L"!",                     {false,   L"!"}},
	{L"?",                     {false,   L"?"}},
	{L"'",                     {false,   L"&prime;"}},
	{L"-",                     {false,   L"-"}},
	{L":",                     {false,   L":"}},
	{L";",                     {false,   L";"}},
	{L"/",                     {false,   L"/"}},
	{L"=",                     {false,   L"="}},
	{L"\\_",                   {false,   L"_"}},
	{L"\\&",                   {false,   L"&amp;"}},
	{L"\\$",                   {false,   L"$"}},
	{L"\\#",                   {false,   L"#"}},
	{L"\\%",                   {false,   L"%"}},
	{L"\\{",                   {false,   L"{"}},
	{L"\\}",                   {false,   L"}"}},
	{L"(",                     {false,   L"("}},
	{L")",                     {false,   L")"}},
	{L"[",                     {false,   L"["}},
	{L"]",                     {false,   L"]"}},
	{L"<",                     {false,   L"&lt;"}},
	{L">",                     {false,   L"&gt;"}},
	{L"|",                     {false,   L"|"}},
	{L"\\vert",                {false,   L"|"}},
	{L"\\Vert",                {false,   L"&Vert;"}},
	{L"\\lfloor",              {false,   L"&lfloor;"}},
	{L"\\rfloor",              {false,   L"&rfloor;"}},
	{L"\\lceil",               {false,   L"&lceil;"}},
	{L"\\rceil",               {false,   L"&rceil;"}},
	{L"\\lbrace",              {false,   L"{"}},
	{L"\\rbrace",              {false,   L"}"}},
	{L"\\langle",              {false,   L"&lang;"}},
	{L"\\rangle",              {false,   L"&rang;"}},
	{L"\\lbrack",              {false,   L"["}},
	{L"\\rbrack",              {false,   L"]"}},
	{L"\\forall",              {false,   L"&forall;"}},
	{L"\\exists",              {false,   L"&exist;"}},
	{L"\\Finv",                {false,   L"&#x2132;"}},
	{L"\\Game",                {false,   L"&#x2141;"}},
	{L"\\leftarrow",           {false,   L"&larr;"}},
	{L"\\rightarrow",          {false,   L"&rarr;"}},
	{L"\\gets",                {true,    L"&larr;"}},
	{L"\\to",                  {true,    L"&rarr;"}},
	{L"\\longleftarrow",       {false,   L"&larr;"}},
	{L"\\longrightarrow",      {false,   L"&rarr;"}},
	{L"\\Leftarrow",           {false,   L"&lArr;"}},
	{L"\\Rightarrow",          {false,   L"&rArr;"}},
	{L"\\Longleftarrow",       {false,   L"&lArr;"}},
	{L"\\Longrightarrow",      {false,   L"&rArr;"}},
	{L"\\mapsto",              {false,   L"&map;"}},
	{L"\\longmapsto",          {false,   L"&map;"}},
	{L"\\leftrightarrow",      {false,   L"&harr;"}},
	{L"\\Leftrightarrow",      {false,   L"&hArr;"}},
	{L"\\longleftrightarrow",  {false,   L"&harr;"}},
	{L"\\Longleftrightarrow",  {false,   L"&hArr;"}},
	{L"\\uparrow",             {false,   L"&uarr;"}},
	{L"\\Uparrow",             {false,   L"&uArr;"}},
	{L"\\downarrow",           {false,   L"&darr;"}},
	{L"\\Downarrow",           {false,   L"&dArr;"}},
	{L"\\updownarrow",         {false,   L"&varr;"}},
	{L"\\Updownarrow",         {false,   L"&vArr;"}},
	{L"\\searrow",             {false,   L"&searr;"}},
	{L"\\nearrow",             {false,   L"&nearr;"}},
	{L"\\swarrow",             {false,   L"&swarr;"}},
	{L"\\nwarrow",             {false,   L"&nwarr;"}},
	{L"\\hookrightarrow",      {true,    L"&rarrhk;"}},
	{L"\\hookleftarrow",       {true,    L"&larrhk;"}},
	{L"\\upharpoonright",      {false,   L"&uharr;"}},
	{L"\\upharpoonleft",       {false,   L"&uharl;"}},
	{L"\\downharpoonright",    {false,   L"&dharr;"}},
	{L"\\downharpoonleft",     {false,   L"&dharl;"}},
	{L"\\rightharpoonup",      {false,   L"&rharu;"}},
	{L"\\rightharpoondown",    {false,   L"&rhard;"}},
	{L"\\leftharpoonup",       {false,   L"&lharu;"}},
	{L"\\leftharpoondown",     {false,   L"&lhard;"}},
	{L"\\nleftarrow",          {false,   L"&nlarr;"}},
	{L"\\nrightarrow",         {false,   L"&nrarr;"}},
	{L"\\lim",                 {true,    L"lim"}},
	{L"\\sup",                 {true,    L"sup"}},
	{L"\\inf",                 {true,    L"inf"}},
	{L"\\min",                 {true,    L"min"}},
	{L"\\max",                 {true,    L"max"}},
	{L"\\limsup",              {true,    L"lim sup"}},
	{L"\\liminf",              {true,    L"lim inf"}},
	{L"\\injlim",              {true,    L"inj lim"}},
	{L"\\projlim",             {true,    L"proj lim"}},
	{L"\\supset",              {false,   L"&sup;"}},
	{L"\\subset",              {false,   L"&sub;"}},
	{L"\\supseteq",            {false,   L"&supe;"}},
	{L"\\subseteq",            {false,   L"&sube;"}},
	{L"\\sqsupset",            {false,   L"&sqsup;"}},
	{L"\\sqsubset",            {false,   L"&sqsub;"}},
	{L"\\sqsupseteq",          {false,   L"&sqsupe;"}},
	{L"\\sqsubseteq",          {false,   L"&sqsube;"}},
	{L"\\supsetneq",           {false,   L"&supne;"}},
	{L"\\subsetneq",           {false,   L"&subne;"}},
	{L"\\in",                  {false,   L"&in;"}},
	{L"\\ni",                  {false,   L"&ni;"}},
	{L"\\notin",               {false,   L"&notin;"}},
	{L"\\iff",                 {false,   L"&iff;"}},
	{L"\\sim",                 {false,   L"&sim;"}},
	{L"\\simeq",               {false,   L"&simeq;"}},
	{L"\\approx",              {false,   L"&approx;"}},
	{L"\\propto",              {false,   L"&propto;"}},
	{L"\\equiv",               {false,   L"&equiv;"}},
	{L"\\cong",                {false,   L"&cong;"}},
	{L"\\neq",                 {false,   L"&ne;"}},
	{L"\\ll",                  {false,   L"&ll;"}},
	{L"\\gg",                  {false,   L"&gg;"}},
	{L"\\geq",                 {false,   L"&geq;"}},
	{L"\\leq",                 {false,   L"&leq;"}},
	{L"\\triangleleft",        {false,   L"&vltri;"}},
	{L"\\triangleright",       {false,   L"&vrtri;"}},
	{L"\\trianglelefteq",      {false,   L"&ltrie;"}},
	{L"\\trianglerighteq",     {false,   L"&rtrie;"}},
	{L"\\models",              {false,   L"&models;"}},
	{L"\\vdash",               {false,   L"&vdash;"}},
	{L"\\Vdash",               {false,   L"&Vdash;"}},
	{L"\\vDash",               {false,   L"&vDash;"}},
	{L"\\lesssim",             {false,   L"&lesssim;"}},
	{L"\\nless",               {false,   L"&nless;"}},
	{L"\\ngeq",                {false,   L"&nge;"}},
	{L"\\nleq",                {false,   L"&nle;"}},
	{L"\\times",               {false,   L"&times;"}},
	{L"\\div",                 {false,   L"&div;"}},
	{L"\\wedge",               {false,   L"&wedge;"}},
	{L"\\vee",                 {false,   L"&vee;"}},
	{L"\\oplus",               {false,   L"&oplus;"}},
	{L"\\otimes",              {false,   L"&otimes;"}},
	{L"\\cap",                 {false,   L"&cap;"}},
	{L"\\cup",                 {false,   L"&cup;"}},
	{L"\\sqcap",               {false,   L"&sqcap;"}},
	{L"\\sqcup",               {false,   L"&sqcup;"}},
	{L"\\smile",               {false,   L"&smile;"}},
	{L"\\frown",               {false,   L"&frown;"}},
	{L"\\smallsmile",          {false,   L"&smile;"}},
	{L"\\smallfrown",          {false,   L"&frown;"}},
	{L"\\setminus",            {false,   L"&setminus;"}},
	{L"\\smallsetminus",       {false,   L"&setminus;"}},
	{L"\\And",                 {false,   L"&amp;"}},
	{L"\\sum",                 {true,    L"&sum;"}},
	{L"\\prod",                {true,    L"&prod;"}},
	{L"\\int",                 {false,   L"&int;"}},
	{L"\\iint",                {false,   L"&Int;"}},
	{L"\\iiint",               {false,   L"&tint;"}},
	{L"\\iiiint",              {false,   L"&qint;"}},
	{L"\\oint",                {false,   L"&oint;"}},
	{L"\\bigcap",              {true,    L"&bigcap;"}},
	{L"\\bigodot",             {true,    L"&bigodot;"}},
	{L"\\bigcup",              {true,    L"&bigcup;"}},
	{L"\\bigotimes",           {true,    L"&bigotimes;"}},
	{L"\\coprod",              {true,    L"&coprod;"}},
	{L"\\bigsqcup",            {true,    L"&bigsqcup;"}},
	{L"\\bigoplus",            {true,    L"&bigoplus;"}},
	{L"\\bigvee",              {true,    L"&bigvee;"}},
	{L"\\biguplus",            {true,    L"&biguplus;"}},
	{L"\\bigwedge",            {true,    L"&bigwedge;"}},
	{L"\\star",                {false,   L"&star;"}},
	{L"\\triangle",            {false,   L"&#x25b3;"}},
	{L"\\wr",                  {false,   L"&wr;"}},
	{L"\\textvisiblespace",    {false,   L"&bbrk;"}},
	{L"\\circ",                {false,   L"&SmallCircle;"}},
	{L"\\hbar",                {false,   L"&hslash;"}},		// can't find the right glyph
	{L"\\lnot",                {false,   L"&not;"}},
	{L"\\nabla",               {false,   L"&nabla;"}},
	{L"\\prime",               {false,   L"&prime;"}},
	{L"\\backslash",           {false,   L"&Backslash;"}},
	{L"\\pm",                  {false,   L"&pm;"}},
	{L"\\mp",                  {false,   L"&mp;"}},
	{L"\\emptyset",            {false,   L"&empty;"}},
	{L"\\varnothing",          {false,   L"&Oslash;"}},
	{L"\\O",                   {false,   L"&Oslash;"}},
	{L"\\S",                   {false,   L"&sect;"}},
	{L"\\angle",               {false,   L"&angle;"}},
	{L"\\colon",               {false,   L":"}},
	{L"\\Diamond",             {false,   L"&diamond;"}},
	{L"\\nmid",                {false,   L"&nmid;"}},
	{L"\\square",              {false,   L"&square;"}},
	{L"\\Box",                 {false,   L"&square;"}},
	{L"\\checkmark",           {false,   L"&check;"}},
	{L"\\complement",          {false,   L"&comp;"}},
	{L"\\eth",                 {false,   L"&eth;"}},
	{L"\\hslash",              {false,   L"&hslash;"}},
	{L"\\mho",                 {false,   L"&mho;"}},
	{L"\\flat",                {false,   L"&flat;"}},
	{L"\\sharp",               {false,   L"&sharp;"}},
	{L"\\natural",             {false,   L"&natural;"}},
	{L"\\bullet",              {false,   L"&bullet;"}},
	{L"\\dagger",              {false,   L"&dagger;"}},
	{L"\\ddagger",             {false,   L"&Dagger;"}},
	{L"\\clubsuit",            {false,   L"&clubs;"}},
	{L"\\spadesuit",           {false,   L"&spades;"}},
	{L"\\heartsuit",           {false,   L"&hearts;"}},
	{L"\\diamondsuit",         {false,   L"&diams;"}},
	{L"\\top",                 {false,   L"&top;"}},
	{L"\\perp",                {false,   L"&perp;"}},
	{L"\\ldots",               {false,   L"&#x2026;"}},		// what about just "..."?
	{L"\\cdot",                {false,   L"&sdot;"}},
	{L"\\cdots",               {false,   L"&sdot;&sdot;&sdot;"}},	// maybe &#x22ef;?
	{L"\\vdots",               {false,   L"&#x22ee;"}},
	{L"\\ddots",               {false,   L"&#x22f1;"}},
};

hash_table<operator_info> operator_table(
	operator_data, operator_data + sizeof(operator_data)/sizeof(operator_data[0]));

// This table lists all spacing commands valid in math mode, and how much space they are worth.

dictionary_item<wstring> space_data[] = {
	{L"\\,",       L"0.1em"},
	{L"\\!",       L"-0.1667em"},
	{L"\\ ",       L"0.3333em"},
	{L"\\;",       L"0.2778em"},
	{L"\\quad",    L"1.em"},
	{L"\\qquad",   L"2.em"},
	{L"~",         L"1.em"},
};

hash_table<wstring> space_table(
	space_data, space_data + sizeof(space_data)/sizeof(space_data[0]));

// This table lists all atoms valid in text mode (in addition to single character atoms).

dictionary_item<wstring> text_atom_data[] = {
	{L"\\_",                L"_"},
	{L"\\&",                L"&amp;"},
	{L"\\$",                L"$"},
	{L"\\textbackslash",    L"\\"},
	{L"\\{",                L"{"},
	{L"\\}",                L"}"},
	{L"~",                  L"&nbsp;"},
	{L"\\,",                L"&nbsp;"},
	{L"\\!",                L""},
	{L"\\ ",                L"&nbsp;"},
	{L"\\;",                L"&nbsp;"},
	{L"\\quad",             L"&nbsp;&nbsp;"},
	{L"\\qquad",            L"&nbsp;&nbsp;&nbsp;&nbsp;"},
	{L"<",                  L"&lt;"},
	{L">",                  L"&gt;"},
	{L" ",                  L"&nbsp;"},
	{L"\\textvisiblespace", L"&bbrk;"},		// not quite right but will do for now
	{L"\\O",                L"&Oslash;"},
	{L"\\S",                L"&sect;"},
};

hash_table<wstring> text_atom_table(
	text_atom_data, text_atom_data + sizeof(text_atom_data)/sizeof(text_atom_data[0]));

/* The following table lists negated forms of various operators. This is used whenever the
"\not" command needs to be applied. */

dictionary_item<wstring> negated_data[] = {
	{L"&in;",             L"&notin;"},
	{L"&equiv;",          L"&nequiv;"},
	{L"&exist;",          L"&nexist;"},
	{L"=",                L"&ne;"},
	{L"&rarr;",           L"&nrarr;"},
	{L"&sube;",           L"&nsube;"},
	{L"&sim;",            L"&nsim;"},
	{L"&Vdash;",          L"&nVdash;"},
	{L"&harr;",           L"&nharr;"},
};

hash_table<wstring> negated_table(
	negated_data, negated_data + sizeof(negated_data)/sizeof(negated_data[0]));

// The following table lists all recognised environments, and their delimiting operators.

dictionary_item<environment_info> environment_data[] = {
	{L"matrix",       {L"",       L""}},
	{L"pmatrix",      {L"(",      L")"}},
	{L"bmatrix",      {L"[",      L"]"}},
	{L"Bmatrix",      {L"{",      L"}"}},
	{L"vmatrix",      {L"|",      L"|"}},
	{L"Vmatrix",      {L"&Vert;", L"&Vert;"}},
	{L"cases",        {L"{",      L""}},
};

hash_table<environment_info> environment_table(
	environment_data, environment_data + sizeof(environment_data)/sizeof(environment_data[0]));

/* The following table lists all recognised accent commands.
The first boolean column indicates whether this is a "stretchy" accent.
The second boolean column indicates that this accent goes above (rather than below) the base. */

dictionary_item<accent_info> accent_data[] = {
	{L"\\hat",                 {L"&Hat;",               false,    true}},
	{L"\\widehat",             {L"&Hat;",               true,     true}},
	{L"\\bar",                 {L"&macr;",              false,    true}},
	{L"\\overline",            {L"&macr;",              true,     true}},
	{L"\\underline",           {L"&macr;",              true,     false}},
	{L"\\tilde",               {L"&tilde;",             false,    true}},
	{L"\\widetilde",           {L"&tilde;",             true,     true}},
	{L"\\overbrace",           {L"&OverBrace;",         true,     true}},
	{L"\\underbrace",          {L"&UnderBrace;",        true,     false}},
	{L"\\overleftarrow",       {L"&larr;",              true,     true}},
	{L"\\vec",                 {L"&rarr;",              false,    true}},
	{L"\\overrightarrow",      {L"&rarr;",              true,     true}},
	{L"\\overleftrightarrow",  {L"&harr;",              true,     true}},
	{L"\\dot",                 {L"&middot;",            false,    true}},
	{L"\\ddot",                {L"&middot;&middot;",    false,    true}},
	{L"\\check",               {L"&caron;",             false,    true}},
	{L"\\acute",               {L"&acute;",             false,    true}},
	{L"\\grave",               {L"&grave;",             false,    true}},
	{L"\\breve",               {L"&breve;",             false,    true}},
};

hash_table<accent_info> accent_table(
	accent_data, accent_data + sizeof(accent_data)/sizeof(accent_data[0]));

/* The following table lists all delimiters, and their translations when preceded by a command
like "\left". For example, "<" gets translated as "&rang;" rather than "&lt;" in this context. */

dictionary_item<wstring> delimiter_data[] = {
	{L".",             L""},
	{L"[",             L"["},
	{L"]",             L"]"},
	{L"\\lbrack",      L"["},
	{L"\\rbrack",      L"]"},
	{L"(",             L"("},
	{L")",             L")"},
	{L"<",             L"&lang;"},
	{L">",             L"&rang;"},
	{L"\\langle",      L"&lang;"},
	{L"\\rangle",      L"&rang;"},
	{L"/",             L"/"},
	{L"\\{",           L"{"},
	{L"\\}",           L"}"},
	{L"\\lbrace",      L"{"},
	{L"\\rbrace",      L"}"},
	{L"|",             L"|"},
	{L"\\vert",        L"|"},
	{L"\\Vert",        L"&Vert;"},
	{L"\\uparrow",     L"&uarr;"},
	{L"\\downarrow",   L"&darr;"},
	{L"\\lfloor",      L"&lfloor;"},
	{L"\\rfloor",      L"&rfloor;"},
	{L"\\lceil",       L"&lceil;"},
	{L"\\rceil",       L"&rceil;"},
};

hash_table<wstring> delimiter_table(
	delimiter_data, delimiter_data + sizeof(delimiter_data)/sizeof(delimiter_data[0]));

// This table lists all the "big" commands, and what vertical size they correspond to.

dictionary_item<wstring> big_command_data[] = {
	{L"\\big",     L"1.5em"},
	{L"\\Big",     L"2em"},
	{L"\\bigg",    L"2.5em"},
	{L"\\Bigg",    L"3em"},
};

hash_table<wstring> big_command_table(
	big_command_data, big_command_data + sizeof(big_command_data)/sizeof(big_command_data[0]));

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Font translation code */

// This function translates a latex text font into its best MathML equivalent.
mathml_font latex_text_font::get_mathml_approximation() const {
	switch (family) {
		case rm:
			return bold
				? (italic ? mathml_font_bold_italic : mathml_font_bold)
				: (italic ? mathml_font_italic      : mathml_font_normal);
			
		case sf:
			return bold
				? (italic ? mathml_font_sans_serif_bold_italic : mathml_font_bold_sans_serif)
				: (italic ? mathml_font_sans_serif_italic      : mathml_font_sans_serif);
			
		case tt:   return mathml_font_monospace;
		
		default: throw internal_error(L"Unexpected latex_text_font.");
	}
}

// This function translates a latex math font into its best MathML equivalent.
mathml_font latex_math_font::get_mathml_approximation() const {
	if (boldsymbol) {
		switch (type) {
			case rm:    return mathml_font_bold;
			case it:    return mathml_font_bold_italic;
			case bf:    return mathml_font_bold;
			case bb:    return mathml_font_double_struck;
			case sf:    return mathml_font_bold_sans_serif;
			case cal:   return mathml_font_bold_script;
			case tt:    return mathml_font_monospace;
			case frak:  return mathml_font_bold_fraktur;
		}
	}
	else {
		switch (type) {
			case rm:    return mathml_font_normal;
			case it:    return mathml_font_italic;
			case bf:    return mathml_font_bold;
			case bb:    return mathml_font_double_struck;
			case sf:    return mathml_font_sans_serif;
			case cal:   return mathml_font_script;
			case tt:    return mathml_font_monospace;
			case frak:  return mathml_font_fraktur;
		}
	}
	throw internal_error(L"Unexpected latex_math_font.");
}

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Layout tree building code,
i.e. implementations of "build_layout_tree" for each class derived from parse_node. */


/* The net effect of this function is to flatten a tree of text commands and atoms into a single
layout_textblock node with a sequence of layout_textchar children. */
layout_node* parse_text_join::build_layout_tree(const latex_text_font& current_font) const {
	if (dynamic_cast<parse_text_empty*>(child1)) return child2->build_layout_tree(current_font);
	if (dynamic_cast<parse_text_empty*>(child2)) return child1->build_layout_tree(current_font);

	layout_node* layout1 = child1->build_layout_tree(current_font);
	layout_node* layout2 = child2->build_layout_tree(current_font);
	
	layout_textblock* is_block1 = dynamic_cast<layout_textblock*>(layout1);
	layout_textchar*  is_char1  = dynamic_cast<layout_textchar *>(layout1);
	layout_textblock* is_block2 = dynamic_cast<layout_textblock*>(layout2);
	layout_textchar*  is_char2  = dynamic_cast<layout_textchar *>(layout2);
	
	if (is_block1 && is_block2) {
		is_block1->children.splice(is_block1->children.end(), is_block2->children);
		return is_block1;
	}
	else if (is_block1 && is_char2) {
		is_block1->children.push_back(is_char2);
		return is_block1;
	}
	else if (is_char1 && is_block2) {
		is_block2->children.push_front(is_char1);
		return is_block2;
	}
	else if (is_char1 && is_char2) {
		layout_textblock* new_node = new layout_textblock;
		new_node->children.push_back(is_char1);
		new_node->children.push_back(is_char2);
		return new_node;
	}
	else throw internal_error(L"Unexpected layout_node types in parse_text_join.");
}

layout_node* parse_text_empty::build_layout_tree(const latex_text_font& current_font) const {
	return new layout_textblock;
}

layout_node*
parse_text_command_1arg::build_layout_tree(const latex_text_font& current_font) const {
	latex_text_font new_font = current_font;
	
	if      (command == L"\\rm")  new_font = latex_text_font(latex_text_font::rm, false, false);
	else if (command == L"\\bf")  new_font = latex_text_font(latex_text_font::rm, true, false);
	else if (command == L"\\it")  new_font = latex_text_font(latex_text_font::rm, false, true);
	else if (command == L"\\tt")  new_font = latex_text_font(latex_text_font::tt, false, false);
	else if (command == L"\\sf")  new_font = latex_text_font(latex_text_font::sf, false, false);
	else if (command == L"\\textrm")    new_font.family = latex_text_font::rm;
	else if (command == L"\\emph")      new_font.italic = !current_font.italic;
	else if (command == L"\\textbf")    new_font.bold = true;
	else if (command == L"\\textit")    new_font.italic = true;
	else if (command == L"\\texttt")    new_font.family = latex_text_font::tt;
	else if (command == L"\\textsf")    new_font.family = latex_text_font::sf;
	else throw internal_error(L"Unexpected command for parse_text_command_1arg.");
	
	return argument->build_layout_tree(new_font);
}

layout_node* parse_enter_text_mode::build_layout_tree(const latex_math_font& current_font) const {
	latex_text_font new_font;
	
	if (command == L"\\textrm" || command == L"\\mbox")
		new_font = latex_text_font(latex_text_font::rm, false, false);
	else if (command == L"\\textbf")
		new_font = latex_text_font(latex_text_font::rm, true, false);
	else if (command == L"\\textit" || command == L"\\emph")
		new_font = latex_text_font(latex_text_font::rm, false, true);
	else if (command == L"\\texttt")
		new_font = latex_text_font(latex_text_font::tt, false, false);
	else if (command == L"\\textsf")
		new_font = latex_text_font(latex_text_font::sf, false, false);
	else throw internal_error(L"Unexpected command for parse_enter_text_mode.");

	/* Generate the argument's layout tree with the specified font; wrap it in a layout_textblock
	node if necessary. */
	layout_node* argument_layout = argument->build_layout_tree(new_font);
	if (dynamic_cast<layout_textblock*>(argument_layout)) return argument_layout;
	else {
		layout_textblock* new_node = new layout_textblock;
		new_node->children.push_back(dynamic_cast<layout_textchar*>(argument_layout));
		return new_node;
	}
}

layout_node* parse_text_atom::build_layout_tree(const latex_text_font& current_font) const {
	const wstring* atom = text_atom_table.lookup(text);
	return new layout_textchar((atom == NULL) ? text : *atom,
		current_font.get_mathml_approximation());
}

/* The net effect of this function is to flatten a tree of math join nodes into a single
layout_join node with a sequence of children. (It respects the "prohibit_merge" flag on child
join nodes.) */
layout_node* parse_math_join::build_layout_tree(const latex_math_font& current_font) const {
	layout_node* layout1 = child1->build_layout_tree(current_font);
	layout_node* layout2 = child2->build_layout_tree(current_font);

	layout_join* is_join1 = dynamic_cast<layout_join*>(layout1);
	layout_join* is_join2 = dynamic_cast<layout_join*>(layout2);
	
	layout_join* output;
	
	if (is_join1 && !is_join1->prohibit_merge) {
		if (is_join2 && !is_join2->prohibit_merge) {
			is_join1->children.splice(is_join1->children.end(), is_join2->children);
			output = is_join1;
		}
		else {
			is_join1->children.push_back(layout2);
			output = is_join1;
		}
	}
	else {
		if (is_join2 && !is_join2->prohibit_merge) {
			is_join2->children.push_front(layout1);
			output = is_join2;
		}
		else {
			output = new layout_join;
			output->children.push_back(layout1);
			output->children.push_back(layout2);
		}
	}

	if (output->children.size() == 1) return output->children.front();
	else return output;
}

layout_node* parse_math_empty::build_layout_tree(const latex_math_font& current_font) const {
	return new layout_join;
}

layout_node* parse_math_atom::build_layout_tree(const latex_math_font& current_font) const {
	// Handle one character atoms
	if (text.size() == 1) {
		latex_math_font new_font = current_font;
		
		if (is_plain_alpha(text[0])) {
			// Default font for single alphabetic characters is italic
			if (new_font.type == latex_math_font::none)
				new_font.type = latex_math_font::it;
			return new layout_identifier(text, new_font.get_mathml_approximation());
		}
		else if (text[0] > 0x7F) {
			// Default font for non-ASCII characters is roman
			if (new_font.type == latex_math_font::none)
				new_font.type = latex_math_font::rm;
			return new layout_identifier(text, new_font.get_mathml_approximation());
		}
		else if (text[0] >= '0' && text[0] <= '9') {
			// Default font for numerals is roman
			if (new_font.type == latex_math_font::none)
				new_font.type = latex_math_font::rm;
			return new layout_numeral(text, new_font.get_mathml_approximation());
		}
	}

	// Backslash-prefixed atom commands
	
	const identifier_info* is_identifier = identifier_table.lookup(text);
	if (is_identifier)
		return new layout_identifier(is_identifier->text,
			current_font.boldsymbol
			? (is_identifier->italic ? mathml_font_bold_italic : mathml_font_bold)
			: (is_identifier->italic ? mathml_font_italic      : mathml_font_normal));
	
	const operator_info* is_operator = operator_table.lookup(text);
	if (is_operator) {
		layout_simple_operator* new_node = new layout_simple_operator(is_operator->text,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);
		new_node->underover = is_operator->underover;
		return new_node;
	}

	const wstring* is_function = function_table.lookup(text);
	if (is_function)
		return new layout_identifier(*is_function,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal, true);
	
	const wstring* is_space = space_table.lookup(text);
	if (is_space)
		return new layout_space(*is_space);
		
	throw internal_error(L"Unexpected math atom.");
}

/* This function examines the superscripts, subscripts and primes attached to a base, checks that
they are legal, and constructs an appropriate layout_scripts node. The rules latex uses are:

(1) There can be at most one "^" and at most one "_".
(2) If there are any primes, they must all be consecutive, and must appear immediately before
    any "^" clause that appears.

For example:
- "a^b^c" is illegal (double superscript)
- "a_b_c" is illegal (double subscript)
- "a''^b" is legal, and its superscript is "<mo>&prime;</mo> <mo>&prime;</mo> <mi>b</mi>"
- "a^b''" is illegal (double superscript)
- "a'_b" is legal, superscript is "<mo>&prime;</mo>", subscript is "<mi>b</mi>"
- "a_b'" is equivalent to "a'_b"
- "a'_b'" is illegal (double superscript)
- "a'^b_c" is legal
- "a_b'^c" is legal
- "a'_b^c" is illegal (double superscript)
*/

layout_node* parse_math_scripts::build_layout_tree(const latex_math_font& current_font) const {
	layout_node* sub = NULL;
	layout_node* sup = NULL;

	list<parse_math_script*>::const_iterator ptr = scripts.begin();
	while (ptr != scripts.end()) {
		parse_math_subscript* is_subscript = dynamic_cast<parse_math_subscript*>(*ptr);
		if (is_subscript) {
			if (sub) throw user_error(L"Double subscript.");
			sub = is_subscript->subscript->build_layout_tree(current_font);
			ptr++;
		}
		else {
			layout_join* new_node = new layout_join;

			for (; ptr != scripts.end() && dynamic_cast<parse_math_prime*>(*ptr); ptr++)
				new_node->children.push_back(new layout_simple_operator(L"&prime;",
					current_font.boldsymbol ? mathml_font_bold : mathml_font_normal));
			
			parse_math_superscript* is_superscript = NULL;
			if (ptr != scripts.end()) is_superscript = dynamic_cast<parse_math_superscript*>(*ptr);
				
			if (is_superscript) {
				new_node->children.push_back(
					is_superscript->superscript->build_layout_tree(current_font));
				ptr++;
			}
			
			if (new_node->children.empty()) throw internal_error(L"Unexpected script type.");
			if (sup) throw user_error(L"Double superscript.");
			if (new_node->children.size() == 1) sup = new_node->children.front();
			else sup = new_node;
		}
	}
	
	layout_node* base_layout = base->build_layout_tree(current_font);
	layout_operator* is_operator = dynamic_cast<layout_operator*>(base_layout);

	if (limits.empty())
		return new layout_scripts(base_layout, sub, sup,
			is_operator ? is_operator->underover : false);
	else {
		/* We are cheating here... LaTeX doesn't allow limit commands to apply to baby
		operators like "+" or "\times", but we do.
		Also we just interpret \displaylimits as \limits for the time being. */
		if (!is_operator)
			throw user_error(L"Commands like \"" + limits +
				L"\" may only be applied to operators. (Consider using \"\\mathop\".)");

		if (limits == L"\\limits" || limits == L"\\displaylimits")
			return new layout_scripts(base_layout, sub, sup, true);
		else if (limits == L"\\nolimits")
			return new layout_scripts(base_layout, sub, sup, false);
		else throw internal_error(L"Unexpected limits command.");
	}
}

layout_node* parse_math_command_1arg::build_layout_tree(const latex_math_font& current_font) const {
	
	const accent_info* accent = accent_table.lookup(command);
	if (accent != NULL) {
		layout_simple_operator* accent_node = new layout_simple_operator(accent->text,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);
		accent_node->stretchy = accent->stretchy;
		
		layout_node* argument_layout = argument->build_layout_tree(current_font);
		layout_scripts* new_node;
		if (accent->over)
			new_node = new layout_scripts(argument_layout, NULL, accent_node, false);
		else
			new_node = new layout_scripts(argument_layout, accent_node, NULL, false);
		
		new_node->accent = true;
		return new_node;
	}

	const wstring* big = big_command_table.lookup(command);
	if (big != NULL) {
		parse_math_atom* is_atom = dynamic_cast<parse_math_atom*>(argument);
		if (!is_atom) throw user_error(
			L"The command following \"" + command + L"\" is not a valid delimiter.");
		const wstring* delimiter = delimiter_table.lookup(is_atom->text);
		if (delimiter == NULL) throw user_error(
			L"The command following \"" + command + L"\" is not a valid delimiter.");				
		
		layout_simple_operator* new_node = new layout_simple_operator(*delimiter,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);
		new_node->stretchy = true;
		new_node->size = *big;
		return new_node;
	}

	else if (command == L"\\mathop") {
		layout_node* new_node = argument->build_layout_tree(current_font);
		// wrap up the arugment as a compound operator if it's not an operator already
		return (dynamic_cast<layout_operator*>(new_node))
			? new_node : new layout_compound_operator(new_node);
	}

	else if (command == L"\\operatorname") {
		/* If the only things in the argument are alphabetic, numerals and spaces, we do it
		just like "\sin". Otherwise we just apply roman font and hope for the best. */
		
		try {
			wstring name;
			argument->make_operatorname(name);
			return new layout_identifier(name,
				current_font.boldsymbol ? mathml_font_bold : mathml_font_normal, true);
		}
		catch (operatorname_too_hard) {
			latex_math_font new_font = current_font;
			new_font.type = latex_math_font::rm;
			return argument->build_layout_tree(new_font);
		}
	}

	else if (command == L"\\sqrt")
		return new layout_sqrt(argument->build_layout_tree(current_font));

	/* FIX: one day when we support macros with arguments, these mod commands would be better
	implemented using such macros. It's a bit painful this way. */

	else if (command == L"\\pmod" || command == L"\\bmod" || command == L"\\mod") {
		bool use_parentheses = (command == L"\\pmod");
		layout_join* new_node = new layout_join;
		
		new_node->children.push_back(
			parse_math_atom((command == L"\\bmod") ? L"\\," : L"\\quad")
							  .build_layout_tree(current_font));
		
		if (use_parentheses)
			new_node->children.push_back(parse_math_atom(L"(").build_layout_tree(current_font));
		
		new_node->children.push_back(new layout_simple_operator(L"mod",
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal));
		
		new_node->children.push_back(parse_math_atom(L"\\,").build_layout_tree(current_font));

		new_node->children.push_back(argument->build_layout_tree(current_font));
		
		if (use_parentheses)
			new_node->children.push_back(parse_math_atom(L")").build_layout_tree(current_font));
		
		return new_node;				
	}

	else if (command == L"\\not") {
		layout_simple_operator* is_operator =
			dynamic_cast<layout_simple_operator*>(argument->build_layout_tree(current_font));
		
		if (!is_operator)
			throw user_error(L"The command \"\\not\" must be followed by an operator.");
		
		const wstring* negated = negated_table.lookup(is_operator->text);
		
		if (negated == NULL)
			throw user_error(L"The operator following \"\\not\" cannot be negated."
				L" (If you disagree, contact the blahtex developers, "
				L"we'll see what we can do about it.)");
		else {
			is_operator->text = *negated;
			return is_operator;
		}
	}

	else {
		latex_math_font new_font = current_font;

		if (command == L"\\mathrm" || command == L"\\rm")
			new_font.type = latex_math_font::rm;
		else if (command == L"\\mathbf" || command == L"\\bf")
			new_font.type = latex_math_font::bf;
		else if (command == L"\\mathbb")
			new_font.type = latex_math_font::bb;
		else if (command == L"\\mathit" || command == L"\\it")
			new_font.type = latex_math_font::it;
		else if (command == L"\\mathcal" || command == L"\\cal")
			new_font.type = latex_math_font::cal;
		else if (command == L"\\mathfrak")
			new_font.type = latex_math_font::frak;
		else if (command == L"\\mathtt" || command == L"\\tt")
			new_font.type = latex_math_font::tt;
		else if (command == L"\\mathsf" || command == L"\\sf")
			new_font.type = latex_math_font::sf;
		else if (command == L"\\boldsymbol")
			new_font.boldsymbol = true;
		else throw internal_error(L"Unexpected command_1arg command.");

		return argument->build_layout_tree(new_font);
	}
}

layout_node*
parse_math_command_2args::build_layout_tree(const latex_math_font& current_font) const {
	
	if (command == L"\\frac" || command == L"\\over")
		return new layout_fraction(argument1->build_layout_tree(current_font),
			argument2->build_layout_tree(current_font));

	else if (command == L"\\atop")
		return new layout_fraction(argument1->build_layout_tree(current_font),
			argument2->build_layout_tree(current_font), false);
	
	else if (command == L"\\binom" || command == L"\\choose") {
		layout_simple_operator* left  = new layout_simple_operator(L"(", mathml_font_normal);
		layout_simple_operator* right = new layout_simple_operator(L")", mathml_font_normal);
		left->stretchy = right->stretchy = true;
		
		layout_join* new_node = new layout_join;
		new_node->children.push_back(left);
		new_node->children.push_back(
			new layout_fraction(argument1->build_layout_tree(current_font),
				argument2->build_layout_tree(current_font), false));
		new_node->children.push_back(right);
		
		new_node->prohibit_merge = true;
		return new_node;
	}
	
	else if (command == L"\\sqrt")
		return new layout_root(argument2->build_layout_tree(current_font),
			argument1->build_layout_tree(current_font));

	else throw internal_error(L"Unexpected command_2args command.");
}

layout_node* parse_math_delimited::build_layout_tree(const latex_math_font& current_font) const {
	const wstring*  left_delimiter = delimiter_table.lookup(left->text);
	const wstring* right_delimiter = delimiter_table.lookup(right->text);
	if (left_delimiter == NULL)
		throw user_error(L"The command following \"\\left\" is not a valid delimiter.");
	if (right_delimiter == NULL)
		throw user_error(L"The command following \"\\right\" is not a valid delimiter.");
	
	layout_join* new_node = new layout_join;
	new_node->prohibit_merge = true;
	
	if (*left_delimiter != L"") {
		layout_simple_operator* new_child = new layout_simple_operator(*left_delimiter,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);				
		new_child->stretchy = true;
		new_node->children.push_back(new_child);
	}
	
	layout_node* enclosed_layout = enclosed->build_layout_tree(current_font);
	layout_join* is_join = dynamic_cast<layout_join*>(enclosed_layout);
	if (is_join && !is_join->prohibit_merge)
		new_node->children.splice(new_node->children.end(), is_join->children);
	else new_node->children.push_back(enclosed_layout);
	
	if (*right_delimiter != L"") {
		layout_simple_operator* new_child = new layout_simple_operator(*right_delimiter,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);				
		new_child->stretchy = true;
		new_node->children.push_back(new_child);
	}
	
	return new_node;
}

layout_node* parse_math_environment::build_layout_tree(const latex_math_font& current_font) const {
	const environment_info* environment = environment_table.lookup(name);
	if (environment == NULL) throw internal_error(L"Unexpected environment name.");
	
	layout_join* new_node = new layout_join;
	new_node->prohibit_merge = true;
	
	if (environment->left_delimiter != L"") {
		layout_simple_operator* new_child = new layout_simple_operator(environment->left_delimiter,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);				
		new_child->stretchy = true;
		new_node->children.push_back(new_child);
	}
	
	layout_table* table = dynamic_cast<layout_table*>(child->build_layout_tree(current_font));
	
	if (name == L"cases") {
		table->align = L"left left";
		// check there are at most two columns in each row
		for (list<layout_table_row*>::iterator ptr = table->rows.begin();
			ptr != table->rows.end(); ptr++)
			
			if ((*ptr)->entries.size() > 2)
				throw user_error(L"Each line of a \"cases\" block can have at most two entries.");
	}			
	
	new_node->children.push_back(table);
	
	if (environment->right_delimiter != L"") {
		layout_simple_operator* new_child = new layout_simple_operator(environment->right_delimiter,
			current_font.boldsymbol ? mathml_font_bold : mathml_font_normal);				
		new_child->stretchy = true;
		new_node->children.push_back(new_child);
	}
	
	return new_node;
}

layout_node* parse_math_table::build_layout_tree(const latex_math_font& current_font) const {
	layout_table* new_node = new layout_table;
	for (list<parse_math_table_row*>::const_iterator ptr = rows.begin(); ptr != rows.end(); ptr++)
		new_node->rows.push_back(
			dynamic_cast<layout_table_row*>((*ptr)->build_layout_tree(current_font)));
			
	/* Get rid of last row if it's empty, i.e. the latex "\pmatrix{a \\ b \\}" should be
	equivalent to "\pmatrix{a \\ b}". */
	if (!rows.empty() && rows.back()->entries.size() == 1 &&
		dynamic_cast<const parse_math_empty*>(rows.back()->entries.front()))
			new_node->rows.pop_back();
	
	return new_node;
}

layout_node* parse_math_table_row::build_layout_tree(const latex_math_font& current_font) const {
	layout_table_row* new_node = new layout_table_row;
	for (list<parse_math*>::const_iterator ptr = entries.begin(); ptr != entries.end(); ptr++)
		new_node->entries.push_back((*ptr)->build_layout_tree(current_font));
	return new_node;
}

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Stuff to merge adjacent numerals */

layout_node* layout_join::merge_numerals() {
	list<layout_node*>::iterator ptr = children.begin();
	while (ptr != children.end()) {
		bool should_continue = true;
		layout_numeral* is_numeral = dynamic_cast<layout_numeral*>(*ptr);
		if (is_numeral) {
			list<layout_node*>::iterator next = ptr;
			if (++next != children.end()) {
				layout_numeral* is_numeral_next = dynamic_cast<layout_numeral*>(*next);
				if (is_numeral_next && is_numeral->font == is_numeral_next->font) {
					is_numeral->text += is_numeral_next->text;
					children.erase(next);
					should_continue = false;
				}
			}
		}
		else *ptr = (*ptr)->merge_numerals();
		if (should_continue) ptr++;
	}
	
	if (children.size() == 1 && !prohibit_merge) return children.front();
	else return this;
}

layout_node* layout_compound_operator::merge_numerals() {
	child = child->merge_numerals();
	return this;
}

layout_node* layout_scripts::merge_numerals() {
	base = base->merge_numerals();
	if (lower) lower = lower->merge_numerals();
	if (upper) upper = upper->merge_numerals();
	return this;
}

layout_node* layout_fraction::merge_numerals() {
	numerator = numerator->merge_numerals();
	denominator = denominator->merge_numerals();
	return this;
}

layout_node* layout_sqrt::merge_numerals() {
	inside = inside->merge_numerals();
	return this;
}

layout_node* layout_root::merge_numerals() {
	inside = inside->merge_numerals();
	outside = outside->merge_numerals();
	return this;
}

layout_node* layout_table_row::merge_numerals() {
	for (list<layout_node*>::iterator ptr = entries.begin(); ptr != entries.end(); ptr++)
		*ptr = (*ptr)->merge_numerals();
	return this;
}

layout_node* layout_table::merge_numerals() {
	for (list<layout_table_row*>::iterator ptr = rows.begin(); ptr != rows.end(); ptr++)
		*ptr = dynamic_cast<layout_table_row*>((*ptr)->merge_numerals());
	return this;
}


/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
end of file */
