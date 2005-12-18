// File "UnicodeConverter.h"
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

#include <string>
#include <iconv.h>

// 'UnicodeConverter' handles all UTF8 <=> wchar_t conversions. It's basically a wrapper for the iconv
// library in terms of:
// 
// - 'string' (assumed to be in UTF-8) and
// - 'wstring' (in internal wchar_t format, which may be big-endian or little-endian depending on platform).
// 
// Implemented in 'UnicodeConverter.cpp'.
class UnicodeConverter {
    public:
        UnicodeConverter();
        ~UnicodeConverter();

        // Open() must be called before using this object. A std::runtime_error object will be thrown if
        //    (1) we are running on a platform with less than 4 bytes per wchar_t, or
        //    (2) an appropriate 'iconv_t' converter object can't be created
        void Open();

        std::wstring ConvertIn(const std::string& input);
        std::string ConvertOut(const std::wstring& input);
        
        // The above 'ConvertIn' and 'ConvertOut' functions will throw this exception object if
        // something goes wrong during conversion (e.g. invalid UTF-8 input).
        class Exception {
        };
    
    private:
        bool mIsOpen;
    
        // mOutHandle is the iconv object handling wchar_t => UTF-8, mInHandle does the other way.
        iconv_t mOutHandle;
        iconv_t mInHandle;

        // We keep per-instance data buffers to avoid repetitious allocate/free calls.
        unsigned mBufSize;
        char*    mNarrowBuf;
        wchar_t* mWideBuf;

        // Reallocates mNarrowBuf and mWideBuf if necessary to accomodate newSize characters.
        void EnsureBufSize(unsigned newSize);
};

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
