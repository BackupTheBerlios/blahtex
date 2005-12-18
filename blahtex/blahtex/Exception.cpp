// File "Exception.cpp"
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

using namespace std;

namespace blahtex {

pair<Exception::Code, wstring> gCodesAsTextArray[] = {
    make_pair( Exception::cNonAsciiInMathMode,                     L"NonAsciiInMathMode" ),
    make_pair( Exception::cIllegalCharacter,                       L"IllegalCharacter" ),
    make_pair( Exception::cPngIncompatibleCharacter,               L"PngIncompatibleCharacter" ),
    make_pair( Exception::cReservedCommand,                        L"ReservedCommand" ),
    make_pair( Exception::cTooManyTokens,                          L"TooManyTokens" ),
    make_pair( Exception::cIllegalFinalBackslash,                  L"IllegalFinalBackslash" ),
    make_pair( Exception::cUnrecognisedCommand,                    L"UnrecognisedCommand" ),
    make_pair( Exception::cIllegalCommandInMathMode,               L"IllegalCommandInMathMode" ),
    make_pair( Exception::cIllegalCommandInMathModeWithHint,       L"IllegalCommandInMathModeWithHint" ),
    make_pair( Exception::cIllegalCommandInTextMode,               L"IllegalCommandInTextMode" ),
    make_pair( Exception::cIllegalCommandInTextModeWithHint,       L"IllegalCommandInTextModeWithHint" ),
    make_pair( Exception::cMissingOpenBraceBefore,                 L"MissingOpenBraceBefore" ),
    make_pair( Exception::cMissingOpenBraceAfter,                  L"MissingOpenBraceAfter" ),
    make_pair( Exception::cMissingOpenBraceAtEnd,                  L"MissingOpenBraceAtEnd" ),
    make_pair( Exception::cNotEnoughArguments,                     L"NotEnoughArguments" ),
    make_pair( Exception::cMissingCommandAfterNewcommand,          L"MissingCommandAfterNewcommand" ),
    make_pair( Exception::cIllegalRedefinition,                    L"IllegalRedefinition" ),
    make_pair( Exception::cMissingOrIllegalParameterCount,         L"MissingOrIllegalParameterCount" ),
    make_pair( Exception::cMissingOrIllegalParameterIndex,         L"MissingOrIllegalParameterIndex" ),
    make_pair( Exception::cUnmatchedOpenBracket,                   L"UnmatchedOpenBracket" ),
    make_pair( Exception::cUnmatchedOpenBrace,                     L"UnmatchedOpenBrace" ),
    make_pair( Exception::cUnmatchedCloseBrace,                    L"UnmatchedCloseBrace" ),
    make_pair( Exception::cUnmatchedLeft,                          L"UnmatchedLeft" ),
    make_pair( Exception::cUnmatchedRight,                         L"UnmatchedRight" ),
    make_pair( Exception::cUnmatchedBegin,                         L"UnmatchedBegin" ),
    make_pair( Exception::cUnmatchedEnd,                           L"UnmatchedEnd" ),
    make_pair( Exception::cUnexpectedNextCell,                     L"UnexpectedNextCell" ),
    make_pair( Exception::cUnexpectedNextRow,                      L"UnexpectedNextRow" ),
    make_pair( Exception::cMismatchedBeginAndEnd,                  L"MismatchedBeginAndEnd" ),
    make_pair( Exception::cCasesRowTooBig,                         L"CasesRowTooBig" ),
    make_pair( Exception::cMissingDelimiter,                       L"MissingDelimiter" ),
    make_pair( Exception::cIllegalDelimiter,                       L"IllegalDelimiter" ),
    make_pair( Exception::cMisplacedLimits,                        L"MisplacedLimits" ),
    make_pair( Exception::cDoubleSuperscript,                      L"DoubleSuperscript" ),
    make_pair( Exception::cDoubleSubscript,                        L"DoubleSubscript" ),
    make_pair( Exception::cAmbiguousInfix,                         L"AmbiguousInfix" ),
    make_pair( Exception::cUnavailableSymbolFontCombination,       L"UnavailableSymbolFontCombination" ),
    make_pair( Exception::cInvalidNegation,                        L"InvalidNegation" )
};

wishful_hash_map<Exception::Code, wstring> Exception::gCodesAsTextTable(gCodesAsTextArray, END_ARRAY(gCodesAsTextArray));

pair<Exception::Code, wstring> gEnglishMessagesArray[] = {

    make_pair(Exception::cNonAsciiInMathMode,
        L"Non-ASCII characters may only be used in text mode (try enclosing the problem characters in \"\\text{...}\")"),

    make_pair(Exception::cIllegalCharacter,
        L"Illegal character in input"),

    make_pair(Exception::cPngIncompatibleCharacter,
        L"Blahtex is unable to generate a correct PNG for the character $0"),

    make_pair(Exception::cReservedCommand,
        L"The command \"$0\" is reserved for internal use by blahtex"),

    make_pair(Exception::cTooManyTokens,
        L"The input is too long"),

    make_pair(Exception::cIllegalFinalBackslash,
        L"Illegal backslash \"\\\" at end of input"),

    make_pair(Exception::cUnrecognisedCommand,
        L"Unrecognised command \"$0\""),

    make_pair(Exception::cIllegalCommandInMathMode,
        L"The command \"$0\" is illegal in math mode"),

    make_pair(Exception::cIllegalCommandInMathModeWithHint,
        L"The command \"$0\" is illegal in math mode (perhaps you intended to use \"$1\" instead?)"),

    make_pair(Exception::cIllegalCommandInTextMode,
        L"The command \"$0\" is illegal in text mode"),

    make_pair(Exception::cIllegalCommandInTextModeWithHint,
        L"The command \"$0\" is illegal in text mode (perhaps you intended to use \"$1\" instead?)"),

    make_pair(Exception::cMissingOpenBraceBefore,
        L"Missing open brace \"{\" before \"$0\""),

    make_pair(Exception::cMissingOpenBraceAfter,
        L"Missing open brace \"{\" after \"$0\""),

    make_pair(Exception::cMissingOpenBraceAtEnd,
        L"Missing open brace \"{\" at end of input"),

    make_pair(Exception::cNotEnoughArguments,
        L"Not enough arguments were supplied for \"$0\""),

    make_pair(Exception::cMissingCommandAfterNewcommand,
        L"Missing or illegal new command name after \"\\newcommand\" "   
        L"(there must be precisely one command defined; it must begin with a backslash \"\\\" "   
        L"and contain only alphabetic characters)"),

    make_pair(Exception::cIllegalRedefinition,
        L"The command \"$0\" has already been defined; you cannot redefine it"),

    make_pair(Exception::cMissingOrIllegalParameterCount,
        L"Missing or illegal parameter count in definition of \"$0\" (must be a single digit between 1 and 9 inclusive)"),

    make_pair(Exception::cMissingOrIllegalParameterIndex,
        L"Missing or illegal parameter index in definition of \"$0\""),

    make_pair(Exception::cUnmatchedOpenBracket,
        L"Encountered open bracket \"[\" without matching close bracket \"]\""),

    make_pair(Exception::cUnmatchedOpenBrace,
        L"Encountered open brace \"{\" without matching close brace \"}\""),

    make_pair(Exception::cUnmatchedCloseBrace,
        L"Encountered close brace \"{\" without matching open brace \"}\""),

    make_pair(Exception::cUnmatchedLeft,
        L"Encountered \"\\left\" without matching \"\\right\""),

    make_pair(Exception::cUnmatchedRight,
        L"Encountered \"\\right\" without matching \"\\left\""),

    make_pair(Exception::cUnmatchedBegin,
        L"Encountered \"\\begin\" without matching \"\\end\""),

    make_pair(Exception::cUnmatchedEnd,
        L"Encountered \"\\end\" without matching \"\\begin\""),

    make_pair(Exception::cUnexpectedNextCell,
        L"The command \"&\" may only appear inside a \"\\begin ... \\end\" block"),

    make_pair(Exception::cUnexpectedNextRow,
        L"The command \"\\\\\" may only appear inside a \"\\begin ... \\end\" block"),

    make_pair(Exception::cMismatchedBeginAndEnd,
        L"Commands \"$0\" and \"$1\" do not match"),

    make_pair(Exception::cCasesRowTooBig,
        L"There can only be two entries in each row of a \"cases\" block"),

    make_pair(Exception::cMissingDelimiter,
        L"Missing delimiter after \"$0\""),

    make_pair(Exception::cIllegalDelimiter,
        L"Illegal delimiter following \"$0\""),

    make_pair(Exception::cMisplacedLimits,
        L"The command \"$0\" can only appear after a math operator (consider using \"\\mathop\")"),

    make_pair(Exception::cDoubleSuperscript,
        L"Encountered two superscripts attached to the same base (only one is allowed)"),

    make_pair(Exception::cDoubleSubscript,
        L"Encountered two subscripts attached to the same base (only one is allowed)"),

    make_pair(Exception::cAmbiguousInfix,
        L"Ambiguous placement of \"$0\" (try using additional braces \"{ ... }\" to disambiguate)"),

    make_pair(Exception::cUnavailableSymbolFontCombination,
        L"The symbol \"$0\" is not available in the font \"$1\""),

    make_pair(Exception::cInvalidNegation,
        L"No negative version of the symbol(s) following \"\\not\" is available. (If you disagree, contact the developers!)"),
};

wishful_hash_map<Exception::Code, wstring> Exception::gEnglishMessagesTable(gEnglishMessagesArray, END_ARRAY(gEnglishMessagesArray));

Exception::Exception(Code code)    
    : mCode(code)
{
}

Exception::Exception(Code code, const wstring& arg1)
    : mCode(code)
{
    mArgs.push_back(arg1);
}

Exception::Exception(Code code, const wstring& arg1, const wstring& arg2)
    : mCode(code)
{
    mArgs.push_back(arg1);
    mArgs.push_back(arg2);
}

/*
Exception::Exception(Code code, const wstring& arg1, int arg2)
    : mCode(code)
{
    mArgs.push_back(arg1);
    wostringstream s;
    s << arg2;
    mArgs.push_back(s.str());
}
*/

wstring Exception::GetCodeAsText() const
{
    wishful_hash_map<Code, wstring>::const_iterator search = gCodesAsTextTable.find(mCode);
    if (search == gCodesAsTextTable.end())
        return L"UnknownExceptionCode";
    else
        return search->second;
}

wstring XmlEncode(wstring input, bool encodingRaw)
{
    wostringstream wos;
    for (wstring::const_iterator ptr = input.begin(); ptr != input.end(); ptr++)
    {
        if (*ptr == L'&')
            wos << L"&amp;";
        else if (*ptr == L'<')
            wos << L"&lt;";
        else if (*ptr == L'>')
            wos << L"&gt;";
        else if (*ptr < 0x7F)
            wos << *ptr;
        else if (!encodingRaw)
            wos << *ptr;
        else
            wos << L"&#x" << hex << static_cast<unsigned>(*ptr) << L";";
    }
    
    return wos.str();
}

wstring Exception::GetXml(bool encodingRaw) const
{
    wostringstream wos;

    wos << L"<inputError>";
    
    wos << L"<id>" << GetCodeAsText() << L"</id>";

    for (vector<wstring>::const_iterator arg = mArgs.begin(); arg != mArgs.end(); arg++)
        wos << L"<arg>" << XmlEncode(*arg, encodingRaw) << "</arg>";

    wos << L"<message>";
    wishful_hash_map<Code, wstring>::const_iterator search = gEnglishMessagesTable.find(mCode);
    if (search == gEnglishMessagesTable.end())
        wos << L"An exception with an unknown code occurred";
    else
    {
        const wstring& source = search->second;
        wstring message;

        // Perform argument substitution on error message, e.g. "$2" gets replaced with contents of mArgs[2]
        for (wstring::const_iterator ptr = source.begin(); ptr != source.end(); ptr++) {
            if (*ptr == L'$') {
                ptr++;
                int n = (*ptr) - L'0';
                if (n >= 0 && n < mArgs.size())
                    message += mArgs[n];
                else
                    message += L"?";
            }
            else
                message += *ptr;
        }

        wos << XmlEncode(message, encodingRaw);
    }

    wos << L"</message>";
    wos << L"</inputError>";
    return wos.str();
}

}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
