// File "XmlNode.h"
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

#ifndef BLAHTEX_XMLNODE_H
#define BLAHTEX_XMLNODE_H

#include <string>
#include <iostream>
#include <map>
#include <list>
#include "Misc.h"

namespace blahtex
{

// Encodes the given string as XML using the supplied options.
// (See Misc.h for explanation of EncodingOptions.)
extern std::wstring XmlEncode(
    const std::wstring& input,
    const EncodingOptions& options
);

// XmlNode represents a node in an XML tree.
//
// (I'm no expert on XML, so I don't know if this struct really captures
// XML trees that well. But it seems to be fine for encoding a MathML tree,
// which is all that blahtex cares about right now.)
struct XmlNode
{
    enum NodeType
    {
        // cTag indicates that this node represents a tag pair, like
        // "<mi>...</mi>". In this case mText is something like "mi",
        // mAttributes is a list of pairs like ("mathvariant", "sans-serif")
        // which describe the attributes in the opening tag, and mChildren
        // is a list of children nodes.
        cTag,

        // cString means this is like a CDATA string, stored in mText
        // (mAttributes and mChildren are unused). Note that characters are
        // stored directly in unicode (not using entities like "&amp;").
        cString
    }
    mType;

    std::wstring mText;
    std::map<std::wstring, std::wstring> mAttributes;
    std::list<XmlNode*> mChildren;

    XmlNode(
        NodeType type,
        const std::wstring& text
    ) :
        mType(type),
        mText(text)
    { }

    ~XmlNode();

    // Print() recursively prints the XML tree rooted at this node to the
    // given output stream.
    //
    // Also handles XML entity encoding (see EncodingOptions).
    //
    // If "indent" is true, it will print each tag pair on a new line, and
    // add appropriate indenting.
    void Print(
        std::wostream& os,
        const EncodingOptions& options,
        bool indent,
        int depth = 0
    ) const;
};

}

#endif

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
