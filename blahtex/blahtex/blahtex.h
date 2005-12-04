// File "blahtex.h"
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


// FIX: there's a bug when the input is just "\sqrt[]"; wrong error message is generated

// FIX: check md5 code doesn't care about endianness of wchar_t....?

// FIX: need to change GNU GPL headers on all files

// FIX: need to add "\odot"

#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <list>
#include <set>
#include <map>

// FIX: need to prevent non-ascii chars making it to latex if we don't get the unicode packages sorted out

// FIX: remember to mention Firefox version recommendation somewhere

// FIX: remember to go over all of Roger's comments on that PDF he sent me

// I use wishful_hash_set/map whenever I really want to use hash_set/map.
// Unfortunately hash_set/map is not quite standard enough yet, so for now it just gets mapped to set/map.
#define  wishful_hash_map  std::map
#define  wishful_hash_set  std::set

// Yes, I hate macros too. Sorry.
// It's used to set up a hash table from an array of raw data.
#define END_ARRAY(zzz_array) ((zzz_array) + sizeof(zzz_array)/sizeof((zzz_array)[0]))

// The blahtex namespace encompasses the blahtex core (as opposed to the blahtex command-line application):
// this includes tokenising, parsing, generating mathml, generating "reconstructed latex".
//
// It does NOT include: wide/narrow character conversion, shelling out to latex/dvips/imagemagick, I/O.

namespace blahtex
{

// This function tests whether two sets are disjoint, assuming T has a total ordering.
// (You'd think the STL would have this, but I couldn't find it.)
template<typename T> bool disjoint(const std::set<T>& x, const std::set<T>& y)
{
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

enum MathmlFont
{
    cMathmlFontNormal,
    cMathmlFontBold,
    cMathmlFontItalic,
    cMathmlFontBoldItalic,
    cMathmlFontDoubleStruck,
    cMathmlFontBoldFraktur,
    cMathmlFontScript,
    cMathmlFontBoldScript,
    cMathmlFontFraktur,
    cMathmlFontSansSerif,
    cMathmlFontBoldSansSerif,
    cMathmlFontSansSerifItalic,
    cMathmlFontSansSerifBoldItalic,
    cMathmlFontMonospace
};

enum MathmlEncoding
{
    cMathmlEncodingRaw,         // directly as unicode chars (i.e. ends up as UTF-8)
    cMathmlEncodingNumeric,     // use e.g. &#x1234;
    cMathmlEncodingShort,       // use e.g. &lang;
    cMathmlEncodingLong         // use e.g. &LeftAngleBracket;
};

struct MathmlOptions
{
    int mSpacingExplicitness;
    bool mFancyFontSubstitution;
    MathmlEncoding mMathmlEncoding;     // How to encode MathML chars
    bool mOtherEncodingRaw;             // how to encode non-MathML chars
    
    MathmlOptions() :
        mSpacingExplicitness(1), mFancyFontSubstitution(false),
        mMathmlEncoding(cMathmlEncodingNumeric), mOtherEncodingRaw(false) { }
};

enum Flavour
{
    cFlavourOrd,
    cFlavourOp,
    cFlavourBin,
    cFlavourRel,
    cFlavourOpen,
    cFlavourClose,
    cFlavourPunct,
    cFlavourInner
};

enum Placement
{
    cPlacementSideset,
    cPlacementUnderOver,
    cPlacementAccent
};

enum Style
{
    cStyleDisplay,
    cStyleText,
    cStyleScript,
    cStyleScriptScript
};

enum Limits
{
    cLimitsDisplayLimits,
    cLimitsLimits,
    cLimitsNoLimits
};

enum Align
{
    cAlignLeft,
    cAlignCentre,
    cAlignRightLeft
};

struct XmlNode
{
    // If mIsTag is:
    // cTag:    then this node represents a tag pair, like "<mi>...</mi>". In this case mText is something
    //          like "mi", mAttributes is a list of pairs like ("mathvariant","sans-serif"), and
    //          mChildren gives the children nodes.
    // cString: then this node represents a simple text string, which is stored in mText, and the other
    //          members are unused.
    enum NodeType
    {
        cTag,
        cString,
    }
    mType;

    std::wstring mText;
    std::map<std::wstring, std::wstring> mAttributes;
    std::list<XmlNode*> mChildren;

    XmlNode(NodeType type, const std::wstring& text) :
        mType(type), mText(text) { }

    ~XmlNode();
    void Print(std::wostream& os, const MathmlOptions& options, bool indent, int depth = 0) const;
};

// Exception is the type of object thrown by all parts of the blahtex core. Generally they indicate
// some kind of syntax error in the input. More serious errors (e.g. STL memory errors) will presumably
// throw some kind of std::exception.
// 
// Each exception consists of an mCode plus zero or more arguments (mArgs).
//
// Implemented in Exception.cpp.
class Exception
{
public:
    // mCode may be assigned any of the following:
    enum Code
    {
        cIllegalCharacter,
        cIllegalBlahtexSuffix,
        cTooManyTokens,
        cIllegalFinalBackslash,
        cUnrecognisedCommand,
        cIllegalCommandInMathMode,
        cIllegalCommandInMathModeWithHint,
        cIllegalCommandInTextMode,
        cIllegalCommandInTextModeWithHint,
        cMissingOpenBraceBefore,
        cMissingOpenBraceAfter,
        cMissingOpenBraceAtEnd,
        cNotEnoughArguments,
        cMissingCommandAfterNewcommand,
        cIllegalRedefinition,
        cMissingOrIllegalParameterCount,
        cMissingOrIllegalParameterIndex,
        cUnmatchedOpenBracket,
        cUnmatchedOpenBrace,
        cUnmatchedCloseBrace,
        cUnmatchedLeft,
        cUnmatchedRight,
        cUnmatchedBegin,
        cUnmatchedEnd,
        cUnexpectedNextCell,
        cUnexpectedNextRow,
        cMismatchedBeginAndEnd,
        cCasesRowTooBig,
        cMissingDelimiter,
        cIllegalDelimiter,
        cMisplacedLimits,
        cDoubleSuperscript,
        cDoubleSubscript,
        cAmbiguousInfix,
        cUnavailableSymbolFontCombination,
        cInvalidNegation
    };

    Exception(Code code);
    Exception(Code code, const std::wstring& arg1);
    Exception(Code code, const std::wstring& arg1, const std::wstring& arg2);
    
    std::wstring GetXml(bool encodingRaw) const;
    
private:
    Code                        mCode;
    std::vector<std::wstring>   mArgs;

    // Returns short string corresponding to mCode, e.g. returns "TooManyTokens" if mCode == TooManyTokens
    std::wstring GetCodeAsText() const;
    
    static wishful_hash_map<Code, std::wstring> gCodesAsTextTable;
    static wishful_hash_map<Code, std::wstring> gEnglishMessagesTable;
};

namespace LayoutTree
{
    struct Node
    {
        virtual ~Node() { }
        
        // This field is only used during the layout tree building phase (to determine spacing).
        // It corresponds roughly to TeX's differently flavoured atoms.
        // It's ignored for Space nodes
        Flavour mFlavour;
        
        // This field is only used during the layout tree building phase (to determine script placement).
        // FIX: is this true anymore?
        // It indicates whether this (operator) node should have scripts placed in "script position".
        // It is only valid if mFlavour == cFlavourOp.
        Limits mLimits;

        // This field is ignored for Space nodes
        Style mStyle;
        
        Node(Style style, Flavour flavour, Limits limits) :
            mStyle(style), mFlavour(flavour), mLimits(limits) { }
                
        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const = 0;

        // This function recursively prints the layout tree under this node.
        // This is only used for debugging. Implemented in debug.cpp.
        virtual void Print(std::wostream& os, int depth = 0) const = 0;
    };
    
    struct Row : Node
    {
        std::list<Node*> mChildren;

        Row(Style style) :
            Node(style, cFlavourOrd, cLimitsDisplayLimits) { }
        ~Row();

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };

    struct Symbol : Node
    {
        std::wstring mText;
        MathmlFont mFont;

        Symbol(const std::wstring& text, MathmlFont font, Style style, Flavour flavour, Limits limits) :
            Node(style, flavour, limits), mText(text), mFont(font) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const = 0;
        virtual void Print(std::wostream& os, int depth = 0) const = 0;
    };
    
    struct SymbolPlain : Symbol
    {
        SymbolPlain(const std::wstring& text, MathmlFont font, Style style, Flavour flavour, Limits limits = cLimitsDisplayLimits) :
            Symbol(text, font, style, flavour, limits) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };

    struct SymbolText : Symbol
    {
        SymbolText(const std::wstring& text, MathmlFont font, Style style) :
            Symbol(text, font, style, cFlavourOrd, cLimitsDisplayLimits) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct SymbolOperator : Symbol
    {
        bool mIsStretchy;
        std::wstring mSize;

        SymbolOperator(bool isStretchy, const std::wstring& size, const std::wstring& text, MathmlFont font, Style style, Flavour flavour, Limits limits = cLimitsDisplayLimits) :
            Symbol(text, font, style, flavour, limits), mIsStretchy(isStretchy), mSize(size) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct Space : Node
    {
        int mWidth;  // measured in mu (so 1em = 18 mu in normal font size); may be negative
        bool mIsUserRequested;

        Space(int width, bool isUserRequested) :
            Node(cStyleDisplay, cFlavourOrd, cLimitsDisplayLimits), mWidth(width), mIsUserRequested(isUserRequested) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct Scripts : Node
    {
        // Any of the following three fields may be NULL.
        std::auto_ptr<Node> mBase, mUpper, mLower;
        Placement mPlacement;

        Scripts(Style style, Flavour flavour, Limits limits, Placement placement, std::auto_ptr<Node> base, std::auto_ptr<Node> upper, std::auto_ptr<Node> lower) :
            Node(style, flavour, limits), mPlacement(placement), mBase(base), mUpper(upper), mLower(lower) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct Fraction : Node
    {
        std::auto_ptr<Node> mNumerator, mDenominator;
        bool mIsLineVisible;

        Fraction(Style style, std::auto_ptr<Node> numerator, std::auto_ptr<Node> denominator, bool isLineVisible) :
            Node(style, cFlavourOrd, cLimitsDisplayLimits), mNumerator(numerator), mDenominator(denominator), mIsLineVisible(isLineVisible) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct Fenced : Node
    {
        std::wstring mLeftDelimiter, mRightDelimiter;
        std::auto_ptr<Node> mChild;

        Fenced(const std::wstring& leftDelimiter, const std::wstring& rightDelimiter, std::auto_ptr<Node> child) :
            Node(child->mStyle, cFlavourOrd, cLimitsDisplayLimits), mLeftDelimiter(leftDelimiter), mRightDelimiter(rightDelimiter), mChild(child) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct Sqrt : Node
    {
        std::auto_ptr<Node> mChild;
        
        Sqrt(std::auto_ptr<Node> child) :
            Node(child->mStyle, cFlavourOrd, cLimitsDisplayLimits), mChild(child) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
    struct Root : Node
    {
        std::auto_ptr<Node> mInside, mOutside;

        Root(std::auto_ptr<Node> inside, std::auto_ptr<Node> outside) :
            Node(inside->mStyle, cFlavourOrd, cLimitsDisplayLimits), mInside(inside), mOutside(outside) { }

        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };

    struct Table : Node
    {
        std::vector<std::vector<Node*> > mRows;
        Align mAlign;

        Table(Style style) :
            Node(style, cFlavourOrd, cLimitsDisplayLimits), mAlign(cAlignCentre) { }
        
        ~Table();
        virtual std::auto_ptr<XmlNode> BuildMathmlTree(const MathmlOptions& options, Style currentStyle) const;
        virtual void Print(std::wostream& os, int depth = 0) const;
    };
    
} // end LayoutTree namespace


extern wishful_hash_map<std::wstring, std::wstring> gDelimiterTable;

// The ParseTree namespace contains all classes representing nodes in the parse tree.
namespace ParseTree
{

    struct LatexMathFont
    {
        enum Family
        {
            cFamilyDefault,  // indicates default font (e.g. "x" gets italics, "1" gets roman)
            cFamilyRm,       // roman
            cFamilyBf,       // bold
            cFamilyIt,       // italics
            cFamilySf,       // sans serif
            cFamilyTt,       // typewriter (monospace)
            cFamilyBb,       // blackboard bold (double-struck)
            cFamilyCal,      // calligraphic
            cFamilyFrak      // fraktur
        }
        mFamily;
        
        bool mIsBoldsymbol;
        
        LatexMathFont(Family family = cFamilyDefault, bool isBoldsymbol = false) :
            mFamily(family), mIsBoldsymbol(isBoldsymbol) { }
        
        MathmlFont GetMathmlApproximation() const;
    };

    struct LatexTextFont
    {
        enum Family
        {
            cFamilyRm,       // roman
            cFamilySf,       // sans serif
            cFamilyTt        // typewriter (monospace)
        }
        mFamily;
        
        bool mIsBold;
        bool mIsItalic;
        
        LatexTextFont(Family family = cFamilyRm, bool isBold = false, bool isItalic = false) :
            mFamily(family), mIsBold(isBold), mIsItalic(isItalic) { }

        MathmlFont GetMathmlApproximation() const;
    };

    // Base class for nodes in the parse tree.
    struct Node
    {
        virtual ~Node() { };
        
        virtual void ReconstructLatex(std::wostream& os) const = 0;

        // 'Print' recursively prints the parse tree under this node.
        // This is only used for debugging. Implemented in 'debug.cpp'.
        virtual void Print(std::wostream& os, int depth = 0) const = 0;

    };

    // Any node that occurs during math mode.
    struct MathNode : Node
    {
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const = 0;
    };

    // Any node that occurs during text mode.
    struct TextNode : Node
    {
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const = 0;
    };

    // Represents any command like "a", "1", "\alpha", "\int" which blahtex treats as a single symbol.
    // (Also includes spacing commands like "\,".)
    struct MathSymbol : MathNode
    {
        std::wstring mCommand;

        MathSymbol(const std::wstring& command) :
            mCommand(command) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a blahtex primitive taking a single argument; that is, any command listed in
    // gMathTokenTable as having token code equal to cCommand1Arg.
    struct MathCommand1Arg : MathNode
    {
        std::wstring mCommand;
        std::auto_ptr<MathNode> mChild;
        
        MathCommand1Arg(const std::wstring& command, std::auto_ptr<MathNode> child) :
            mCommand(command), mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a style change primitive like "\rm", "\scriptstyle"
    // e.g. in the expression "abc \rm def", the mChild member of the "\rm" node points to "def".
    struct MathStyleChange : MathNode
    {
        std::wstring mCommand;
        std::auto_ptr<MathNode> mChild;
        
        MathStyleChange(const std::wstring& command, std::auto_ptr<MathNode> child) :
            mCommand(command), mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a blahtex primitive taking two arguments; that is, any command listed in
    // gMathTokenTable as having token code equal to cCommand2Args or cCommandInfix.
    struct MathCommand2Args : MathNode
    {
        std::wstring mCommand;
        std::auto_ptr<MathNode> mChild1, mChild2;
        bool mIsInfix;
        
        MathCommand2Args(const std::wstring& command,
            std::auto_ptr<MathNode> child1, std::auto_ptr<MathNode> child2, bool isInfix) :
            mCommand(command), mChild1(child1), mChild2(child2), mIsInfix(isInfix) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a "big" primitive like "\bigBlahtex" or "\BiggBlahtex".
    // (The ordinary TeX commands "\big" etc get translated via macros to one of these blahtex primitives.)
    struct MathBig : MathNode
    {
        std::wstring mCommand, mDelimiter;

        MathBig(const std::wstring& command, const std::wstring& delimiter) :
            mCommand(command), mDelimiter(delimiter) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents material surrounded by grouping braces.
    // e.g. "{abc}" gets stored as a MathGroup node whose child contains "abc".
    struct MathGroup : MathNode
    {
        std::auto_ptr<MathNode> mChild;

        MathGroup(std::auto_ptr<MathNode> child) :
            mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a sequence of nodes, concatenated together, left-to-right.
    // e.g. "abc" gets stored as a MathList containing three MathSymbol nodes.
    struct MathList : MathNode
    {
        std::vector<MathNode*> mChildren;

        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
        ~MathList();
    };

    // Represents a base with a superscript and/or subscript.
    // All three fields are optional (NULL indicates an empty field).
    struct MathScripts : MathNode
    {
        std::auto_ptr<MathNode> mBase, mUpper, mLower;

        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };
    
    // Represents a "limits" command, i.e. one of "\limits", "\nolimits", or "\displaylimits".
    // Its child should be the operator that it is applied to. e.g. in the input "x^2\limits_5",
    // the base of the MathScripts node should be a MathLimits node, whose child is the MathSymbol
    // node representing "x".
    struct MathLimits : MathNode
    {
        std::wstring mCommand;
        std::auto_ptr<MathNode> mChild;

        MathLimits(const std::wstring& command, std::auto_ptr<MathNode> child) :
            mCommand(command), mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents an expression surrounded by "\left( ... \right)".
    // The members mLeftDelimiter and mRightDelimiter must be delimiter commands like "(" or "\langle" or ".".
    // mChild is the stuff enclosed by the delimiters.
    struct MathDelimited : MathNode
    {
        // These two delimiters are allowed to be blank.
        std::wstring mLeftDelimiter, mRightDelimiter;
        std::auto_ptr<MathNode> mChild;

        MathDelimited(std::auto_ptr<MathNode> child,
            const std::wstring& leftDelimiter, const std::wstring& rightDelimiter) :
            mChild(child), mLeftDelimiter(leftDelimiter), mRightDelimiter(rightDelimiter) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };
    
    // Represents a row of a table, e.g. it might represent the expression "a & b & c".
    struct MathTableRow : MathNode
    {
        std::vector<MathNode*> mEntries;

        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
        ~MathTableRow();
    };

    // Represents a table, e.g. might represent the expression "a & b & c \\ d & e & f \\ g & h".
    struct MathTable : MathNode
    {
        std::vector<MathTableRow*> mRows;

        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
        ~MathTable();
    };

    // Represents an environment. Currently all supported environments are just various forms of table, so
    // for the moment we insist that it contains a table.
    // The mName' member is one of: "matrix", "pmatrix", "bmatrix", "Bmatrix", "vmatrix", "Vmatrix", "cases".
    struct MathEnvironment : MathNode
    {
        std::wstring mName;
        std::auto_ptr<MathTable> mTable;

        MathEnvironment(const std::wstring& name, std::auto_ptr<MathTable> table) :
            mName(name), mTable(table) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a command that switches from math mode into text mode (e.g. "\textrmBlahtex").
    // Note that certain commands (e.g. "\textrmBlahtex") will be translated into a TextCommand1Arg node
    // if encountered during text mode, but into a EnterTextMode if encountered during math mode.
    struct EnterTextMode : MathNode
    {
        std::wstring mCommand;
        std::auto_ptr<TextNode> mChild;
        
        EnterTextMode(const std::wstring& command, std::auto_ptr<TextNode> child) :
            mCommand(command), mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexMathFont& currentFont, Style currentStyle) const;
    };

    // Represents a sequence of nodes in text mode.
    struct TextList : TextNode
    {
        std::vector<TextNode*> mChildren;

        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const;
        ~TextList();
    };

    // Represents text mode material surrounded by grouping braces.
    struct TextGroup : TextNode
    {
        std::auto_ptr<TextNode> mChild;

        TextGroup(std::auto_ptr<TextNode> child) :
            mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const;
    };
    
    // Represents any text mode command like "a", "1", "\textbackslash" which blahtex treats as a
    // single symbol.
    struct TextSymbol : TextNode
    {
        std::wstring mCommand;

        TextSymbol(const std::wstring& command) :
            mCommand(command) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const;
    };

    // Represents a style change like "\rm" occurring in text mode.
    struct TextStyleChange : TextNode
    {
        std::wstring mCommand;
        std::auto_ptr<TextNode> mChild;

        TextStyleChange(const std::wstring& command, std::auto_ptr<TextNode> child) :
            mCommand(command), mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const;
    };
    
    // Represents a command in text mode taking a single argument (e.g. "\textrmBlahtex").
    struct TextCommand1Arg : TextNode
    {
        std::wstring mCommand;
        std::auto_ptr<TextNode> mChild;

        TextCommand1Arg(const std::wstring& command, std::auto_ptr<TextNode> child) :
            mCommand(command), mChild(child) { }
        virtual void Print(std::wostream& os, int depth) const;
        virtual void ReconstructLatex(std::wostream& os) const;
        virtual std::auto_ptr<LayoutTree::Node> BuildLayoutTree(const LatexTextFont& currentFont, Style currentStyle) const;
    };

} // end ParseTree namespace


// MacroProcessor maintains a stack of tokens, can be queried for the next available token, and expands
// macros automatically. It is the layer between tokenising (handled by the Instance class) and parsing
// proper.
//
// It does not process "\newcommand" commands automatically; instead it passes "\newcommand" straight back
// to the caller, and the caller is responsible for calling MacroProcessor::HandleNewcommand.
// (Rationale: this gives results much closer to real TeX parsing. For example, we wouldn't want
// "x^\newcommand{\stuff}{xyz}\stuff" to be construed as legal input.)
//
// Note that unlike TeX, macros are *not* local to their enclosing block.
// e.g. "{\newcommand{\abc}{xyz}}\abc" will not generate an "unrecognised command" exception.
//
// Implemented in MacroProcessor.cpp.

class MacroProcessor
{
public:
    MacroProcessor(const std::vector<std::wstring>& input);

    // Returns the next token on the stack (without removing it), after expanding macros.
    // (Returns empty string if there are no tokens left.)
    std::wstring Peek();

    // Same as Peek(), but also removes the token.
    std::wstring Get();
    
    // Pops the current token.
    void Advance();
    
    // Pops consecutive whitespace tokens.
    void SkipWhitespace();
    
    // Assuming that "\newcommand" has just been seen and popped off the stack, this function
    // processes a macro definition.
    void HandleNewcommand();

private:
    // Records information about a single macro.
    struct Macro
    {
        // The number of parameters the macro accepts. (Blahtex doesn't handle optional arguments.)
        int mParameterCount;
        
        // The sequence of tokens that get substituted when this macro is expanded.
        // Arguments are indicated by "#1", "#2", etc.
        std::vector<std::wstring> mReplacement;
        
        Macro() : mParameterCount(0) { }
    };

    // List of all currently recognised macros.
    wishful_hash_map<std::wstring, Macro> mMacros;

    // The token stack. (mTokens.back() is the top of the stack.)
    std::vector<std::wstring> mTokens;
    
    // This flag is set if we have already ascertained that the current token doesn't need to undergo macro
    // expansion. (This is just an optimisation so that successive calls to Peek/Get don't need to check the
    // macro table more than is necessary.)
    bool mIsTokenReady;

    // Reads a single macro argument; that is, either a single token, or if that token is "{", reads all the
    // way up to the matching "}". The argument (not including delimiting braces) is appended to 'output'.
    // Returns true on success, or false if the argument is missing.
    bool ReadArgument(std::vector<std::wstring>& output);

    // Total approximate cost of parsing activity so far (approximately). See cMaxParseCost for more info.
    unsigned mCostIncurred;
};

// The time spent by the parser should be O(cMaxParseCost). This corresponds roughly to the number of
// tokens that blahtex will allow in a single translation operation, but it also takes into account
// things like time taken during macro expansion, searching for matching braces, etc. The aim is to prevent
// a nasty user inducing exponential time via tricky macro definiitons.
const unsigned cMaxParseCost = 20000;

// The Parser class actually does the parsing work. It runs the supplied input tokens through a
// MacroProcessor, and builds a parse tree from the resulting token stream.

class Parser
{
public:
    // Main function that the caller should use to do a parsing job.
    // It is ok to call DoParse repeatedly for a single Parser object; each DoParse starts with a clean slate.
    std::auto_ptr<ParseTree::MathNode> DoParse(const std::vector<std::wstring>& input);

    // FIX: remember to mention somewhere that \newcommand is not local to blocks

    // The parser uses GetMathTokenCode/MathPrimitveTable (in math mode) or GetTextTokenCode/
    // TextTokenTable (in text mode) to translate each incoming token into one of the following values:
    enum TokenCode
    {
        cEndOfInput,
        cWhitespace,

        // The "\newcommand" command.
        cNewcommand,
        
        // Commands that are illegal in the current mode and don't start with a backslash (like "$", "%").
        cIllegal,
        
        // Opening and closing braces ("{" and "}").
        cBeginGroup,
        cEndGroup,

        // The commands "&" and "\\".
        cNextCell,
        cNextRow,
        
        // The commands "^" and "_".
        cSuperscript,
        cSubscript,
        
        // The prime symbol "'".
        cPrime,

        // Blahtex primitives accepting one argument or two arguments respectively.
        cCommand1Arg,
        cCommand2Args,
        
        // Infix commands like "\over".
        cCommandInfix,
        
        // The "\left" and "\right" commands.
        cLeft,
        cRight,
        
        // Any command like "\bigBlahtex" which must be followed by a delimiter.
        cBig,
        
        // Any of "\limits", "\nolimits", "\displaylimits".
        cLimits,
        
        // Commands like "\begin{matrix}", "\end{matrix}".
        cBeginEnvironment,
        cEndEnvironment,
        
        // Command that switch from math mode to text mode (e.g. "\textrmBlahtex").
        cEnterTextMode,

        // Style change commands, e.g. "\rm" and "\scriptstyle"
        cStyleChange,

        // Pretty much every other blahtex primitive: "a", "1", "\alpha", "+", "\rightarrow", "\,"...
        cSymbol,
        
        // Some commands that you might expect get translated as cSymbol actually become
        // cSymbolUnsafe' instead. What this means is that TeX/LaTeX/AMSLaTeX considers them a macro that
        // gets expanded, and they become subsequently unsafe for use as a single symbol. For example,
        // "x^\cong" is illegal in LaTeX, so we assign the code cSymbolUnsafe to "\cong".
        cSymbolUnsafe
    };

    // These tables contain all the primitives that blahtex recognises in math mode (respectively text mode).
    // They provide the token codes for the parser to do its job.
    static wishful_hash_map<std::wstring, TokenCode> gMathTokenTable;
    static wishful_hash_map<std::wstring, TokenCode> gTextTokenTable;
    
private:
    std::auto_ptr<MacroProcessor> mTokenSource;

    // ParseMathList starts parsing a math list, until it reaches a command indicating the end of
    // the list, like "}" or "\right" or "\end{...}".
    std::auto_ptr<ParseTree::MathNode> ParseMathList();
    
    // ParseMathField parses a TeX "math field", which is either a single symbol or an
    // expression grouped with braces.
    std::auto_ptr<ParseTree::MathNode> ParseMathField();
    
    // Handle a table enclosed in something like "\begin{matrix} ... \end{matrix}"; i.e. it breaks input up
    // into entries and rows based on "\\" and "&" commands.
    std::auto_ptr<ParseTree::MathTable> ParseMathTable();

    // PrepareScripts is used in several places in ParseMathList to set up a MathScripts node to fill.
    // Postcondition is that the last element in 'output' is a MathScripts node, whose base is equal to
    // whatever node was previously the last element in 'output' (unless that node was already a
    // MathScripts node).
    // The idea is that PrepareScripts should be called just after we encounter "^" or "_".
    // 
    // Note the argument and return values have no auto_ptr, so the caller does not get ownership of
    // the MathScripts node (PrepareScripts assigns this ownership to 'output' if necessary).
    ParseTree::MathScripts* PrepareScripts(ParseTree::MathList* output);

    // ParseTextList starts parsing a text list, until it reaches "}" or end of input.
    std::auto_ptr<ParseTree::TextNode> ParseTextList();

    // ParseTextField parses an argument to a command in text mode, which is either a single
    // symbol or an expression grouped with braces.
    std::auto_ptr<ParseTree::TextNode> ParseTextField();
    
    // These functions determine the appropriate token code for the supplied token.
    // Things like "1", "a", "+" are handled appropriately, as are backslash-prefixed commands listed in
    // gMathTokenTable or gTextTokenTable.
    // Exceptions are thrown for unrecognised tokens.
    TokenCode GetMathTokenCode(const std::wstring& token) const;
    TokenCode GetTextTokenCode(const std::wstring& token) const;
};


// Implemented in Instance.cpp.
class Instance
{
public:
    Instance();

    // 'process_input' generates a parse tree and a layout tree from the supplied input.
    void ProcessInput(const std::wstring& input);
    
    std::auto_ptr<XmlNode> GenerateMathml(const MathmlOptions& options);
    std::auto_ptr<XmlNode> GenerateHtml();
    std::wstring GenerateReconstructedLatex();
    
    const ParseTree::MathNode* GetParseTree() { return mParseTree.get(); }
    const LayoutTree::Node* GetLayoutTree() { return mLayoutTree.get(); }

private:
    std::auto_ptr<ParseTree::MathNode> mParseTree;
    std::auto_ptr<LayoutTree::Node> mLayoutTree;
    
    // The Tokenise function splits the given input into tokens, each represented by a string. The output is
    // appended to 'output'.
    // 
    // There are several types of tokens:
    // - single characters like "a", or "{", or single non-ascii unicode characters
    // - alphabetic commands like "\frac"
    // - commands like "\," which have a single nonalphabetic character after the backslash
    // - commands like "\   " which have their whitespace collapsed, stored as "\ "
    // - other consecutive whitespace characters which get collapsed to just " "
    // - the sequence "\begin   {  stuff  }" gets stored as the single token "\begin{  stuff  }";
    //   similarly for "\end"
    static void Tokenise(const std::wstring& input, std::vector<std::wstring>& output);

    // gStandardMacros is a string which, in effect, gets inserted at the beginning of any input string 
    // handled by ProcessInput. It contains a sequence of "\newcommand" commands.
    static std::wstring gStandardMacros;
    
    // Tokenised version of gStandardMacros (computed only once, when first used):
    static std::vector<std::wstring> gStandardMacrosTokenised;
};

// This functions strips off the "Blahtex" suffix from its input, if such a suffix appears.
// The idea is to convert internal commands like "\fracBlahtex" back into plain old "\frac".
extern std::wstring StripBlahtexSuffix(const std::wstring& input);

}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
