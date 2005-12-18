// File "MacroProcessor.cpp"
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
#include <algorithm>

using namespace std;

namespace blahtex
{

wstring StripReservedSuffix(const wstring& input)
{
    if (input.size() >= 8 && input.substr(input.size() - 8, 8) == L"Reserved")
        return input.substr(0, input.size() - 8);
    else
        return input;
}

MacroProcessor::MacroProcessor(const vector<wstring>& input)
{
    copy(input.rbegin(), input.rend(), inserter(mTokens, mTokens.begin()));
    mCostIncurred = input.size();
    mIsTokenReady = false;
}

void MacroProcessor::Advance()
{
    if (!mTokens.empty())
    {
        mTokens.pop_back();
        mCostIncurred++;
        mIsTokenReady = false;
    }
}

void MacroProcessor::SkipWhitespace()
{
    while (Peek() == L" ")
        Advance();
}

void MacroProcessor::SkipWhitespaceRaw()
{
    while (!mTokens.empty() && mTokens.back() == L" ")
        Advance();
}

bool MacroProcessor::ReadArgument(vector<wstring>& output)
{
    SkipWhitespaceRaw();
    if (mTokens.empty())
        // Missing argument
        return false;
    
    wstring token = mTokens.back();
    mTokens.pop_back();
    mCostIncurred++;
    if (token == L"}")
        // Argument can't start with "}"
        return false;
        
    if (token == L"{")
    {
        int braceDepth = 1;
        while (!mTokens.empty())
        {
            mCostIncurred++;
            wstring token = mTokens.back();
            mTokens.pop_back();
            if (token == L"{")
                braceDepth++;
            else if (token == L"}" && --braceDepth == 0)
                break;
            output.push_back(token);
        }
        if (braceDepth > 0)
            throw Exception(Exception::cUnmatchedOpenBrace);
    }
    else
    {
        output.push_back(token);
    }

    mIsTokenReady = false;
    return true;
}

wstring MacroProcessor::Get()
{
    wstring token = Peek();
    Advance();
    return token;
}

void MacroProcessor::HandleNewcommand()
{
    // pop the "\newcommand" command:
    mTokens.pop_back();
    mCostIncurred++;

    // gobble opening brace
    SkipWhitespaceRaw();
    if (mTokens.empty() || mTokens.back() != L"{")
        throw Exception(Exception::cMissingOpenBraceAfter, L"\\newcommand");
    mTokens.pop_back();

    // grab new command being defined
    SkipWhitespaceRaw();
    if (mTokens.empty() || mTokens.back().empty() || mTokens.back()[0] != L'\\')
        throw Exception(Exception::cMissingCommandAfterNewcommand);
    wstring newCommand = mTokens.back();
    if (mMacros.count(newCommand) || gMathTokenTable.count(newCommand) || gTextTokenTable.count(newCommand))
        throw Exception(Exception::cIllegalRedefinition, StripReservedSuffix(newCommand));
    mTokens.pop_back();
    
    // gobble close brace
    SkipWhitespaceRaw();
    if (mTokens.empty())
        throw Exception(Exception::cUnmatchedOpenBrace);
    if (mTokens.back() != L"}")
        throw Exception(Exception::cMissingCommandAfterNewcommand);
    mTokens.pop_back();
    
    Macro& macro = mMacros[newCommand];

    SkipWhitespaceRaw();
    // Determine the number of arguments, if specified.
    if (!mTokens.empty() && mTokens.back() == L"[")
    {
        mTokens.pop_back();
        
        SkipWhitespaceRaw();
        if (mTokens.empty() || mTokens.back().size() != 1)
            throw Exception(Exception::cMissingOrIllegalParameterCount, newCommand);
        macro.mParameterCount = static_cast<int>(mTokens.back()[0] - L'0');
        if (macro.mParameterCount <= 0 || macro.mParameterCount > 9)
            throw Exception(Exception::cMissingOrIllegalParameterCount, newCommand);
        mTokens.pop_back();
        
        SkipWhitespaceRaw();
        if (mTokens.empty() || mTokens.back() != L"]")
            throw Exception(Exception::cUnmatchedOpenBracket);
        mTokens.pop_back();
    }
    
    if (newCommand == L"\\pxx")
        wcout << L"hello" << endl;
    
    // Read and store the tokens which make up the macro replacement.
    if (!ReadArgument(macro.mReplacement))
        throw Exception(Exception::cNotEnoughArguments, L"\\newcommand");
}

wstring MacroProcessor::Peek()
{
    while (!mTokens.empty())
    {
        if (mTokens.size() + (++mCostIncurred) >= cMaxParseCost)
            throw Exception(Exception::cTooManyTokens);

        if (mIsTokenReady)
            return mTokens.back();

        // "\sqrt" needs special handling due to its optional argument.
        // Something like "\sqrtReserved{x}" gets converted to "\sqrt{x}".
        // Something like "\sqrtReserved[y]{x}" gets converted to "\rootReserved{y}{x}".
        // 
        // (Blahtex doesn't handle grouping of [...] the same way as texvc; it does it the TeX way.
        // For example, "\sqrt[\sqrt[2]{3}]{4}" generates an error, whereas it is valid in texvc.)
        // 
        // We need to take into account grouping braces, e.g. "\sqrt[{]}]{2}" is valid.
        if (mTokens.back() == L"\\sqrtReserved")
        {
            mTokens.pop_back();

            SkipWhitespaceRaw();
            if (!mTokens.empty() && mTokens.back() == L"[")
            {
                mTokens.back() = L"{";

                vector<wstring>::reverse_iterator ptr = mTokens.rbegin();
                ptr++;
                
                int braceDepth = 0;
                while (ptr != mTokens.rend() && (braceDepth > 0 || *ptr != L"]"))
                {
                    mCostIncurred++;
                    if (*ptr == L"{")
                        braceDepth++;
                    else if (*ptr == L"}")
                    {
                        if (--braceDepth < 0)
                            throw Exception(Exception::cUnmatchedCloseBrace);
                    }
                    ptr++;
                }
                if (ptr == mTokens.rend())
                    throw Exception(Exception::cUnmatchedOpenBracket);
                if (*ptr != L"]")
                    throw Exception(Exception::cNotEnoughArguments, L"\\sqrt");
                *ptr = L"}";
                mTokens.push_back(L"\\rootReserved");
                mIsTokenReady = true;
                return L"\\rootReserved";
            }
            else
            {
                mTokens.push_back(L"\\sqrt");
                mIsTokenReady = true;
                return L"\\sqrt";
            }
        }
        else
        {
            wstring token = mTokens.back();
            wishful_hash_map<wstring, Macro>::const_iterator macroPtr = mMacros.find(token);
            if (macroPtr == mMacros.end())
            {
                // In this case it's not "\sqrt" and not a macro, so we're finished here.
                mIsTokenReady = true;
                return token;
            }
        
            const Macro& macro = macroPtr->second;
            mTokens.pop_back();

            // It's a macro. Determines the arguments to substitute in....
            vector<vector<wstring> > arguments(macro.mParameterCount);
            for (int argumentIndex = 0; argumentIndex < macro.mParameterCount; argumentIndex++)
                if (!ReadArgument(arguments[argumentIndex]))
                    throw Exception(Exception::cNotEnoughArguments, StripReservedSuffix(token));

            // ... and now write the replacement, substituting arguments as we go.
            const vector<wstring>& replacement = macro.mReplacement;
            vector<wstring> output;
            for (vector<wstring>::const_iterator source = replacement.begin(); source != replacement.end(); source++)
            {
                mCostIncurred++;
                if (*source == L"#")
                {
                    if (++source == replacement.end())
                        throw Exception(Exception::cMissingOrIllegalParameterIndex, token);
                    if (source->size() != 1)
                        throw Exception(Exception::cMissingOrIllegalParameterIndex, token);
                    int parameterIndex = static_cast<int>((*source)[0] - '1');
                    // FIX: perhaps this next error should be flagged when reading the definition of the
                    // macro rather than during macro expansion
                    if (parameterIndex < 0 || parameterIndex >= macro.mParameterCount)
                        throw Exception(Exception::cMissingOrIllegalParameterIndex, token);
                    copy(arguments[parameterIndex].begin(), arguments[parameterIndex].end(), back_inserter(output));
                    mCostIncurred += arguments[parameterIndex].size();
                }
                else
                    output.push_back(*source);
            }
            copy(output.rbegin(), output.rend(), back_inserter(mTokens));
            mCostIncurred += output.size();
        }
    }
    
    return L"";
}

}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
