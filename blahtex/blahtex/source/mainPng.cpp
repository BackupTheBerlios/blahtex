// File "mainPng.cpp"
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

#ifdef BLAHTEX_USING_MAGICK
#include <Magick++.h>
#endif

#include "BlahtexCore/Misc.h"
#include "UnicodeConverter.h"
#include "md5Wrapper.h"
#include <cerrno>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


using namespace std;
using namespace blahtex;


// From main.cpp:
extern UnicodeConverter gUnicodeConverter;


// TemporaryFile manages a temporary file; it deletes the named file when
// the object goes out of scope.
class TemporaryFile
{
    string mFilename;
    
    // This flag might get set to false if we are in some kind of
    // debugging mode and want to keep temp files.
    bool mShouldDelete;

public:
    TemporaryFile(
        const string& filename,
        bool shouldDelete = true
    ) :
        mFilename(filename),
        mShouldDelete(shouldDelete)
    { }

    ~TemporaryFile()
    {
        if (mShouldDelete)
            unlink(mFilename.c_str());
    }
};


// Tests whether a file exists
bool FileExists(const string& filename)
{
    struct stat temp;
    return (stat(filename.c_str(), &temp) == 0);
}


// Attempts to run given command from the given directory.
// Returns true if the system() call was successful, otherwise false.
// Can throw a "CannotChangeDirectory" exception if problems occur.
bool Execute(
    const string& command,
    const string& directory = "./"
)
{
    char buffer[5000];

    bool NeedToChange = (directory != "" && directory != "./");

    if (NeedToChange)
    {
        if (getcwd(buffer, 5000) == NULL)
            throw blahtex::Exception(L"CannotChangeDirectory");

        if (chdir(directory.c_str()) != 0)
            throw blahtex::Exception(L"CannotChangeDirectory");
    }

    bool result = (system(command.c_str()) == 0);

    if (NeedToChange)
    {
        if (chdir(buffer) != 0)
            throw blahtex::Exception(L"CannotChangeDirectory");
    }

    return result;
}


// Generates the PNG file, and computes vertical shift if requested.
// Returns:
//  * md5 of the UTF-8 version of the input.
//  * vertical shift (a non-positive integer);
//    or +1 if either the vertical shift was not requested, or if it
//    could not be computed.
pair<string, int> MakePngFile
(
    const wstring& purifiedTex,
    bool computeVerticalShift,
    const string& tempDirectory,
    const string& pngDirectory,
    const string& shellLatex,
    const string& shellDvips,
    const string& shellConvert,
    const string& imageMagickOptions,
    bool deleteTempFiles
)
{
    string purifiedTexUtf8 = gUnicodeConverter.ConvertOut(purifiedTex);

    string md5 = ComputeMd5(purifiedTexUtf8);
    int shift = 1;

    {
        ofstream texFile(
            (tempDirectory + md5 + ".tex").c_str(),
            ios::out | ios::binary
        );
        if (!texFile)
            throw blahtex::Exception(
                L"CannotCreateTexFile"
            );
        texFile << purifiedTexUtf8;
        if (!texFile)
            throw blahtex::Exception(
                L"CannotWriteTexFile"
            );
    }

    // These are temporary files we want deleted
    // when we're done.
    TemporaryFile texTemp(tempDirectory + md5 + ".tex", deleteTempFiles);
    TemporaryFile auxTemp(tempDirectory + md5 + ".aux", deleteTempFiles);
    TemporaryFile logTemp(tempDirectory + md5 + ".log", deleteTempFiles);
    TemporaryFile dviTemp(tempDirectory + md5 + ".dvi", deleteTempFiles);
    TemporaryFile  psTemp(tempDirectory + md5 + ".ps", deleteTempFiles);
    TemporaryFile psOriginalTemp(
        tempDirectory + md5 + "-original.ps",
        deleteTempFiles
    );

    // FIX: is the md5 hash enough here for the
    // temporary file name? Should we also throw in a
    // process ID, timestamp, or something?

    if (!Execute(
            shellLatex + " " + md5
                + ".tex >/dev/null 2>/dev/null",
            tempDirectory
        )
        ||
        !FileExists(tempDirectory + md5 + ".dvi")
    )
        throw blahtex::Exception(L"CannotRunLatex");

    if (!Execute(
        shellDvips + " -R -E " + tempDirectory + md5
            + ".dvi -f -o " + tempDirectory + md5
            + "-original.ps 2>/dev/null"
        )
        ||
        !FileExists(tempDirectory + md5 + "-original.ps")
    )
        throw blahtex::Exception(L"CannotRunDvips");

    // Extend the PS file's bounding box to the right a few pixels. This
    // addresses the unfortunate situation that can occur when dvips messes
    // up the bounding box; for example, a single integral sign
    // ("\displaystyle \int") gets chopped off.
    {
        ifstream psInput(
            (tempDirectory + md5 + "-original.ps").c_str(),
            ios::in | ios::binary
        );

        ofstream psOutput(
            (tempDirectory + md5 + ".ps").c_str(),
            ios::out | ios::binary
        );
        
        string line;
        bool foundBoundingBox = false;
        while (getline(psInput, line))
        {
            if (!foundBoundingBox && line.substr(0, 14) == "%%BoundingBox:")
            {
                // Add 10 to the 3rd coordinate of the bounding box line
                foundBoundingBox = true;
                string half = line.substr(14, string::npos);
                istringstream decoder(half);
                int coord1, coord2, coord3, coord4;
                decoder >> coord1 >> coord2 >> coord3 >> coord4;
                ostringstream encoder;
                encoder << "%%BoundingBox: "
                    << coord1 << " "
                    << coord2 << " "
                    << (coord3 + 10) << " "
                    << coord4;
                line = encoder.str();
            }
            psOutput << line << endl;
        }
    }

    if (!Execute(
        shellConvert + " " + imageMagickOptions
            + " -trim " + tempDirectory + md5 + ".ps "
            + pngDirectory + md5 + ".png "
            + " >/dev/null 2>/dev/null"
        )
        ||
        !FileExists(pngDirectory + md5 + ".png")
    )
        throw blahtex::Exception(L"CannotRunConvert");

#ifdef BLAHTEX_USING_MAGICK
    if (computeVerticalShift)
    {
        try     // for Magick exceptions
        {
            Magick::Image image(
                pngDirectory + md5 + ".png"
            );
            
            unsigned width = image.baseColumns();
            unsigned height = image.baseRows();

            // In Manager::GeneratePurifiedTex() we
            // arranged for the PNG to have an extra
            // pixel located on the baseline of the
            // equation. Now we search for that pixel.
            int row;
            Magick::Pixels view(image);
            Magick::PixelPacket* pixels = view.get(0, 0, 1, height);
            for (row = 0; row < height; ++row, ++pixels)
                if (pixels->red)
                    break;

            if (row < height)
            {
                // Found it.
                shift = row - height + 1;

                // Remove the first column of the image.
                
                // FIX: I wanted to use the ImageMagick crop function, but
                // it seemed really buggy; it didn't seem to respect the
                // desired crop region accurately enough. So instead we just
                // create a new image from scratch and copy the pixels
                // across, one at a time.
                
                if (width >= 2)
                {
                    Magick::Image newImage(
                        Magick::Geometry(width - 2, height),
                        Magick::Color(0, 0, 0, 0)
                    );
                    
                    newImage.modifyImage();
                    
                    Magick::Pixels newView(newImage);
                    
                    Magick::PixelPacket* pixels =
                        view.get(2, 0, width - 2, height);

                    Magick::PixelPacket* newPixels =
                        newView.get(0, 0, width - 2, height);
                    
                    for (unsigned k = (width - 2) * height; k > 0; --k)
                        *newPixels++ = *pixels++;
                    
                    newView.sync();
                    
                    if (!deleteTempFiles)
                        image.write(tempDirectory + md5 + "-original.png");

                    newImage.write(pngDirectory + md5 + ".png");
                }
            }
        }
        catch (Magick::Exception& e)
        {
            // If imagemagick produces any exceptions,
            // we just ignore it and give up on the
            // vertical alignment stuff.
        }
    }
#endif

    return make_pair(md5, shift);
}
