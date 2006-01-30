// File "Manager.h"
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

#ifndef BLAHTEX_MANAGER_H
#define BLAHTEX_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <set>
#include "Misc.h"
#include "XmlNode.h"
#include "LayoutTree.h"
#include "ParseTree.h"

namespace blahtex
{

// The Manager class coordinates all the bits and pieces required to convert
// the given TeX input into MathML and purified TeX output, including
// tokenising, texvc-compatiblity macros, building the parse and layout
// trees, deciding which LaTeX packages to include, converting mathvariant
// to MathML version 1 fonts.
//
// The Manager class could be used as an interface between the blahtex core
// and an external program; alternatively, the Interface class (see
// Interface.h) provides a simpler interface.

class Manager
{
public:
    Manager();

    // ProcessInput generates a parse tree and a layout tree from the
    // supplied input.
    //
    // If texvcCompatibility is set, then ProcessInput will append a series
    // of macros to emulate various non-standard commands that texvc
    // recognises (see gTexvcCompatibilityMacros). This corresponds to the
    // command line option "--texvc-compatible-commands".
    void ProcessInput(
        const std::wstring& input,
        bool texvcCompatibility = false
    );

    // GenerateMathml generates a XML tree containing MathML markup.
    // Returns the root node.
    std::auto_ptr<XmlNode> GenerateMathml(
        const MathmlOptions& options
    ) const;

    // GeneratePurifiedTex returns a string containing a complete TeX file
    // (including any required \usepackage commands) that could be fed to
    // LaTeX to produce a graphical version of the input.
    std::wstring GeneratePurifiedTex(
        const PurifiedTexOptions& options
    ) const;

    // A few accessor functions.
    const ParseTree::MathNode* GetParseTree() const
    {
        return mParseTree.get();
    }

    const LayoutTree::Node* GetLayoutTree() const
    {
        return mLayoutTree.get();
    }

private:
    // These store the parse tree and layout tree generated by ProcessInput.
    std::auto_ptr<ParseTree::MathNode> mParseTree;
    std::auto_ptr<LayoutTree::Node> mLayoutTree;

    // This flag is set if the user has requested "strict spacing" rules
    // (see SpacingControl) via the magic "\strictspacing" command.
    bool mStrictSpacingRequested;

    // gStandardMacros is a string which, in effect, gets inserted at the
    // beginning of any input string handled by ProcessInput. It contains
    // a sequence of macro definitions ("\newcommand"s) which set up some
    // standard TeX synonyms.
    static std::wstring gStandardMacros;

    // gTexvcCompatibilityMacros is similar; it contains definitions for
    // commands recognised by texvc but that are not standard TeX/LaTeX/
    // AMS-LaTeX. (See also the texvcCompatibility flag.)
    static std::wstring gTexvcCompatibilityMacros;

    // Tokenised version of gStandardMacros and gTexvcCompatibilityMacros
    // (computed only once, when first used):
    static std::vector<std::wstring> gStandardMacrosTokenised;
    static std::vector<std::wstring> gTexvcCompatibilityMacrosTokenised;
};

}

#endif

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
