// File "UnicodeConverter.cpp"
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

#include "UnicodeConverter.h"
#include <iostream>
#include <stdexcept>

using namespace std;

UnicodeConverter::UnicodeConverter() {
    mIsOpen = false;
}

UnicodeConverter::~UnicodeConverter() {
    if (mIsOpen) {
        iconv_close(mInHandle);
        iconv_close(mOutHandle);
        delete[] mNarrowBuf;
        delete[] mWideBuf;
    }
}

// Precondition: mIsOpen == true
void UnicodeConverter::EnsureBufSize(unsigned newSize) {
    if (newSize > mBufSize) {
        delete[] mNarrowBuf;
        delete[] mWideBuf;
        mBufSize = newSize;
        mNarrowBuf = new char[mBufSize];
        mWideBuf = new wchar_t[mBufSize];
    }
}

void UnicodeConverter::Open() {
    if (mIsOpen)
        throw logic_error("UnicodeConverter::Open() called on already open object");

    if (sizeof(wchar_t) != 4)
        throw runtime_error(
            "Blahtex runtime error: "
            "the wchar_t data type on this system is not four bytes wide.");
    
    // Determine endian-ness of wchar_t.
    // (Really we should be able to just use "WCHAR_T", which apparently works on linux, but not on darwin.)
    wchar_t testChar = L'A';
    const char* UcsString = (*(reinterpret_cast<char*>(&testChar)) == 'A') ? "UCS-4LE" : "UCS-4BE";

    mInHandle = iconv_open(UcsString, "UTF-8");
    mOutHandle = iconv_open("UTF-8", UcsString);
    if (mInHandle == (iconv_t)(-1) || mOutHandle == (iconv_t)(-1))
        throw runtime_error("Blahtex runtime error: iconv_open call failed.");
    
    mBufSize = 1024;
    mNarrowBuf = new char[mBufSize];
    mWideBuf = new wchar_t[mBufSize];
    mIsOpen = true;
}

wstring UnicodeConverter::ConvertIn(const string& input) {
    if (!mIsOpen)
        throw logic_error("UnicodeConverter::ConvertIn() called before Open()");

    // The following garbage is needed to handle the unfortunate inconsistency between linux and BSD
    // definitions for the second parameter of 'iconv'. BSD (including Mac OS X) requires 'const char*',
    // whereas Linux requires 'char*', and neither option seems to produce error-free, warning-free
    // compilation on both systems simultaneously.
#ifdef BLAHTEX_ICONV_CONST
    const
#endif
    char* source;
    char* dest;

    EnsureBufSize(input.size());

    source = mNarrowBuf;
    dest = reinterpret_cast<char*>(mWideBuf);
    memcpy(mNarrowBuf, input.c_str(), input.size());
    
    size_t  inBytesLeft = input.size();
    size_t outBytesLeft = 4 * input.size();

    if (iconv(mInHandle, &source, &inBytesLeft, &dest, &outBytesLeft) == -1)
        throw UnicodeConverter::Exception();
    
    return wstring(mWideBuf, input.size() - outBytesLeft / 4);
}

string UnicodeConverter::ConvertOut(const wstring& input) {
    if (!mIsOpen)
        throw logic_error("UnicodeConverter::ConvertOut() called before Open()");

#ifdef BLAHTEX_ICONV_CONST
    const
#endif
    char* source;
    char* dest;

    EnsureBufSize(input.size());

    source = reinterpret_cast<char*>(mWideBuf);
    dest = mNarrowBuf;
    wmemcpy(mWideBuf, input.c_str(), input.size());

    size_t  inBytesLeft = 4 * input.size();
    size_t outBytesLeft = 4 * input.size();
    
    if (iconv(mOutHandle, &source, &inBytesLeft, &dest, &outBytesLeft) == -1)
        throw UnicodeConverter::Exception();

    return string(mNarrowBuf, 4 * input.size() - outBytesLeft);
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
