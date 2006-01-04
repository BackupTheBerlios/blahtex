// File "main.cpp"
// 
// blahtex (version 0.3.5): a TeX to MathML converter designed with MediaWiki in mind
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
#include "UnicodeConverter.h"
#include "md5Wrapper.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <stdexcept>
#include <cerrno>

using namespace std;
using namespace blahtex;

string gBlahtexVersion = "0.3.5";

// A single global instance of UnicodeConverter.
UnicodeConverter gUnicodeConverter;

extern wstring GetErrorMessage(const blahtex::Exception& e);

wstring FormatError(const blahtex::Exception& e, const EncodingOptions& options)
{
    wstring output = L"<error><id>" + e.mCode + L"</id>";
    for (vector<wstring>::const_iterator arg = e.mArgs.begin(); arg != e.mArgs.end(); arg++)
        output += L"<arg>" + XmlEncode(*arg, options) + L"</arg>";

    output += L"<message>" + XmlEncode(GetErrorMessage(e), options) + L"</message>";
    
    output += L"</error>";
    return output;
}

// FIX: Interesting bug. In markup like "\frac 1+", which gives something like
// <mfrac><mn>1</mn><mo>+</mo></mfrac>
// Firefox puts space around the + sign. I don't think it should. Interesting.
// Not sure if this is my bug or theirs.

void ShowUsage()
{
    cout <<
        "Blahtex version " << gBlahtexVersion << "\n"
        "Copyright (C) 2005, David Harvey\n"
        "\n"
        "This is free software; see the source for copying conditions. There is NO\n"
        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
        "\n"
        "Usage: blahtex [ --texvc-compatible-commands ]\n"
        "               [ --mathml ] [ --indented ]  [ --spacing  strict | moderate | relaxed ]\n"
        "               [ --mathml-version-1-fonts ] [ --disallow-plane-1 ]\n"
        "               [ --mathml-encoding  raw | numeric | short | long ]\n"
        "               [ --other-encoding  raw | numeric ]\n"
        "               [ --png ] [ --use-ucs-package ]\n"
        "               [ --shell-latex  path ] [ --shell-dvips  path ] [ --shell-convert  path ]\n"
        "               [ --convert-options  options-string ]\n"
        "               [ --temp-directory  path ] [ --png-directory  path ]\n"
        "               [ --debug  parse | layout | purified ] [ --debug ... ]\n"
        "               < inputfile\n"
        "\n"
        " --texvc-compatible-commands\n"
        "     recognise nonstandard texvc-specific commands\n"
        "\n"
        " --mathml\n"
        "     generate MathML output\n"
        " --indented\n"
        "     produce nicely indented MathML tags\n"
        "\n"
        " --spacing strict\n"
        "     emit MathML spacing markup wherever possible\n"
        " --spacing moderate (default)\n"
        "     emit spacing markup where a MathML renderer is likely to have trouble\n"
        " --spacing relaxed\n"
        "     emit spacing markup only where specifically requested in the TeX input\n"
        "\n"
        " --mathml-version-1-fonts\n"
        "     use character entities and MathML version 1 font attributes instead of \"mathvariant\"\n"
        "\n"
        " --disallow-plane-1\n"
        "     force plane-1 MathML characters to be encoded as e.g. \"&Afr;\" instead of e.g. \"&#x1d504;\"\n"
        "\n"
        " --mathml-encoding raw\n"
        "     encode non-ASCII MathML characters as raw UTF-8\n"
        " --mathml-encoding numeric (default)\n"
        "     encode non-ASCII MathML characters as numeric codes (like \"&#x1234;\")\n"
        " --mathml-encoding short\n"
        "     encode non-ASCII MathML characters using short names (like \"&uarr;\")\n"
        " --mathml-encoding long\n"
        "     encode non-ASCII MathML characters using long names (like \"&UpArrow;\")\n"
        "\n"
        " --other-encoding raw\n"
        "     encode non-ASCII, non-MathML characters as raw UTF-8\n"
        " --other-encoding numeric (default)\n"
        "     encode non-ASCII, non-MathML characters as numeric codes\n"
        "\n"
        " --png\n"
        "     generate PNG output\n"
        "\n"
        " --use-ucs-package\n"
        "     use the LaTeX ucs package to handle some non-ASCII characters\n"
        "\n"
        " --shell-latex  path\n"
        " --shell-dvips  path\n"
        " --shell-convert  path\n"
        "     indicates path for latex, dvips, convert (ImageMagick) utilities\n"
        "     (default is just \"latex\", \"dvips\", \"convert\")\n"
        "\n"
        " --convert-options  options-string\n"
        "     indicates options to be passed to ImageMagick convert utility\n"
        "     (default is \"-quality 100 -density 240 -resize 50% -gamma 0.75\")\n"
        "\n"
        " --temp-directory  path\n"
        "     which directory to use for temp files (.tex, .aux, .log, .dvi, .ps, files)\n"
        "     (default is current directory)\n"
        " --png-directory  path\n"
        "     where to put the output PNG file\n"
        "     (default is current directory)\n"
        "\n"
        " --debug parse\n"
        "     display the parse tree (output is NOT XML)\n"
        " --debug layout\n"
        "     display the layout tree (output is NOT XML)\n"
        " --debug purified\n"
        "     display purified TeX (output is NOT XML)\n"
        "\n";
    exit(0);
}

bool FileExists(const string& filename)
{
    struct stat temp;
    return (stat(filename.c_str(), &temp) == 0);
}

// Attempts to run given command from the given directory
// (current directory if directory == "")
bool Execute(const string& command, const string& directory = "")
{
    char buffer[2000];

    if (!directory.empty())
    {
        if (getcwd(buffer, 2000) == NULL)
            throw runtime_error("Current directory name is too long");

        if (chdir(directory.c_str()) != 0)
            throw blahtex::Exception(L"CannotChangeDirectory");
    }

    bool result = (system(command.c_str()) == 0);

    if (!directory.empty())
    {
        if (chdir(buffer) != 0)
            throw blahtex::Exception(L"CannotChangeDirectory");
    }

    return result;
}

struct CommandLineException
{
    string mMessage;
    CommandLineException(const string& message) :
        mMessage(message) { }
};

// This class deletes the referenced file when it goes out of scope.
struct TemporaryFile
{
    TemporaryFile(const string& filename) :
        mFilename(filename) { }

    ~TemporaryFile()
    {
        unlink(mFilename.c_str());
    }

private:
    string mFilename;
};

void AddTrailingSlash(string& s)
{
    if (!s.empty() && s[s.size() - 1] != '/')
        s += '/';
}

int main (int argc, char* const argv[]) {
    // This outermost try block catches std::runtime_error and CommandLineException.
    try
    {
        gUnicodeConverter.Open();

        if (isatty(0))
            ShowUsage();

        MathmlOptions mathmlOptions;
        EncodingOptions encodingOptions;
        PurifiedTexOptions purifiedTexOptions;

        bool indented = false;
        bool doPng    = false;
        bool doMathml = false;
        bool texvcCompatibility = false;
        
        bool debugLayoutTree  = false;
        bool debugParseTree   = false;
        bool debugPurifiedTex = false;

        string shellLatex   = "latex";
        string shellDvips   = "dvips";
        string shellConvert = "convert";
        string imageMagickOptions = "-quality 100 -density 240 -resize 50% -gamma 0.75";
        string tempDirectory = "./";
        string  pngDirectory = "./";
        
        // Process command line arguments
        for (int i = 1; i < argc; i++)
        {
            string arg(argv[i]);

            if (arg == "--help")
                ShowUsage();
            
            // This option is provided for testing error logging code
            else if (arg == "--throw-logic-error")
                throw logic_error("Aaarrrgggghhhh!");

            else if (arg == "--shell-latex")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--shell-latex\"");
                shellLatex = string(argv[i]);
            }
            
            else if (arg == "--shell-dvips")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--shell-dvips\"");
                shellDvips = string(argv[i]);
            }
            
            else if (arg == "--shell-convert")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--shell-convert\"");
                shellConvert = string(argv[i]);
            }
            
            else if (arg == "--convert-options")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--convert-options\"");
                imageMagickOptions = string(argv[i]);
            }
            
            else if (arg == "--temp-directory")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--temp-directory\"");
                tempDirectory = string(argv[i]);
                AddTrailingSlash(tempDirectory);
            }
            
            else if (arg == "--png-directory")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--png-directory\"");
                pngDirectory = string(argv[i]);
                AddTrailingSlash(pngDirectory);
            }
            
            else if (arg == "--indented")
                indented = true;
            
            else if (arg == "--spacing")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--spacing\"");
                arg = string(argv[i]);
                if (arg == "strict")
                    mathmlOptions.mSpacingControl = cSpacingControlStrict;
                else if (arg == "moderate")
                    mathmlOptions.mSpacingControl = cSpacingControlModerate;
                else if (arg == "relaxed")
                    mathmlOptions.mSpacingControl = cSpacingControlRelaxed;
                else
                    throw CommandLineException("Illegal string after \"--spacing\"");
            }
            
            else if (arg == "--use-ucs-package")
                purifiedTexOptions.mUseUcsPackage = true;
            
            else if (arg == "--mathml-version-1-fonts")
                mathmlOptions.mMathmlVersion1Fonts = true;
            
            else if (arg == "--texvc-compatible-commands")
                texvcCompatibility = true;
            
            else if (arg == "--png")
                doPng = true;
            
            else if (arg == "--mathml")
                doMathml = true;
                
            else if (arg == "--mathml-encoding")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--mathml-encoding\"");
                arg = string(argv[i]);
                if (arg == "raw")
                    encodingOptions.mMathmlEncoding = cMathmlEncodingRaw;
                else if (arg == "numeric")
                    encodingOptions.mMathmlEncoding = cMathmlEncodingNumeric;
                else if (arg == "short")
                    encodingOptions.mMathmlEncoding = cMathmlEncodingShort;
                else if (arg == "long")
                    encodingOptions.mMathmlEncoding = cMathmlEncodingLong;
                else
                    throw CommandLineException("Illegal string after \"--mathml-encoding\"");
            }

            else if (arg == "--disallow-plane-1")
            {
                mathmlOptions  .mAllowPlane1 = false;
                encodingOptions.mAllowPlane1 = false;
            }
            
            else if (arg == "--other-encoding")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--other-encoding\"");
                arg = string(argv[i]);
                if (arg == "raw")
                    encodingOptions.mOtherEncodingRaw = true;
                else if (arg == "numeric")
                    encodingOptions.mOtherEncodingRaw = false;
                else
                    throw CommandLineException("Illegal string after \"--other-encoding\"");
            }

            else if (arg == "--debug")
            {
                if (++i == argc)
                    throw CommandLineException("Missing string after \"--debug\"");
                arg = string(argv[i]);
                if (arg == "layout")
                    debugLayoutTree = true;
                else if (arg == "parse")
                    debugParseTree = true;
                else if (arg == "purified")
                    debugPurifiedTex = true;
                else
                    throw CommandLineException("Illegal string after \"--debug\"");
            }
            
            else
                throw CommandLineException("Unrecognised command line option \"" + arg + "\"");
        }

        // Finished processing command line, now process the input

        wostringstream mainOutput;
        
        try
        {
            blahtex::Instance instance;
            wstring input;

            // Read input file
            string inputUtf8;
            {
                char c;
                while (cin.get(c))
                    inputUtf8 += c;
            }

            // This try block converts UnicodeConverter::Exception into an input syntax error.
            // (Later we treat such exceptions as debug assertions.)
            try
            {
                input = gUnicodeConverter.ConvertIn(inputUtf8);
            }
            catch (UnicodeConverter::Exception& e)
            {
                throw blahtex::Exception(L"InvalidUtf8Input");
            }
            
            instance.ProcessInput(input, texvcCompatibility);
            
            if (debugParseTree)
            {
                mainOutput << L"\n=== BEGIN PARSE TREE ===\n\n";
                instance.GetParseTree()->Print(mainOutput);
                mainOutput << L"\n=== END PARSE TREE ===\n\n";
            }
            
            if (debugLayoutTree)
            {
                mainOutput << L"\n=== BEGIN LAYOUT TREE ===\n\n";
                wostringstream temp;
                instance.GetLayoutTree()->Print(temp);
                mainOutput << XmlEncode(temp.str(), EncodingOptions());
                mainOutput << L"\n=== END LAYOUT TREE ===\n\n";
            }

            if (doPng || debugPurifiedTex)
            {
                wostringstream pngOutput;
                
                try
                {
                    wstring purifiedTex = instance.GeneratePurifiedTex(purifiedTexOptions);
                    
                    if (debugPurifiedTex)
                    {
                        pngOutput << L"\n=== BEGIN PURIFIED TEX ===\n\n";
                        pngOutput << purifiedTex;
                        pngOutput << L"\n=== END PURIFIED TEX ===\n\n";
                    }

                    if (doPng)
                    {
                        string purifiedTexUtf8 = gUnicodeConverter.ConvertOut(purifiedTex);
                        string md5 = ComputeMd5(purifiedTexUtf8);
                        
                        {
                            ofstream texFile((tempDirectory + md5 + ".tex").c_str(), ios::out | ios::binary);
                            if (!texFile)
                                throw blahtex::Exception(L"CannotCreateTexFile");
                            texFile << purifiedTexUtf8;
                            if (!texFile)
                                throw blahtex::Exception(L"CannotWriteTexFile");
                        }
                        
                        TemporaryFile texTemp(tempDirectory + md5 + ".tex");
                        TemporaryFile auxTemp(tempDirectory + md5 + ".aux");
                        TemporaryFile logTemp(tempDirectory + md5 + ".log");
                        TemporaryFile dviTemp(tempDirectory + md5 + ".dvi");
                        TemporaryFile  psTemp(tempDirectory + md5 + ".ps");
                        
                        // FIX: is the md5 hash enough here for the temporary file name?
                        // Should we also throw in a process ID, microtime, or something?
                        
                        if (!Execute(shellLatex + " " + md5 + ".tex >/dev/null 2>/dev/null", tempDirectory))
                            throw blahtex::Exception(L"CannotRunLatex");
                        if (!FileExists(tempDirectory + md5 + ".dvi"))
                            throw blahtex::Exception(L"CannotRunLatex");
                            
                        if (!Execute(shellDvips + " -R -E " + tempDirectory + md5 + ".dvi -f -o "
                                                + tempDirectory + md5 + ".ps 2>/dev/null"))
                            throw blahtex::Exception(L"CannotRunDvips");
                        if (!FileExists(tempDirectory + md5 + ".ps"))
                            throw blahtex::Exception(L"CannotRunDvips");

                        if (!Execute(shellConvert + " " + imageMagickOptions + " -trim "
                                                + tempDirectory + md5 + ".ps " + pngDirectory + md5 + ".png "
                                                + " >/dev/null 2>/dev/null"))
                            throw blahtex::Exception(L"CannotRunConvert");
                        if (!FileExists(pngDirectory + md5 + ".png"))
                            throw blahtex::Exception(L"CannotRunConvert");

                        pngOutput << L"<md5>" << gUnicodeConverter.ConvertIn(md5) << L"</md5>\n";
                        
                        // The temporary files are deleted automatically by TemporaryFile's destructor.
                    }
                }
                
                catch (blahtex::Exception& e)
                {
                    pngOutput.str(L"");
                    pngOutput << FormatError(e, encodingOptions) << endl;
                }
                
                mainOutput << L"<png>\n" << pngOutput.str() << L"</png>\n";
            }
            
            if (doMathml)
            {
                wostringstream mathmlOutput;
                
                try
                {
                    auto_ptr<XmlNode> mathmlRoot = instance.GenerateMathml(mathmlOptions);
                    mathmlOutput << L"<markup>\n";
                    mathmlRoot->Print(mathmlOutput, encodingOptions, indented);
                    mathmlOutput << L"\n</markup>\n";
                }

                catch (blahtex::Exception& e)
                {
                    mathmlOutput.str(L"");
                    mathmlOutput << FormatError(e, encodingOptions) << endl;
                }
                
                mainOutput << L"<mathml>\n" << mathmlOutput.str() << L"</mathml>\n";
            }
        }

        // This catches input syntax errors.
        catch (blahtex::Exception& e)
        {
            mainOutput.str(L"");
            mainOutput << FormatError(e, encodingOptions) << endl;
        }

        cout << "<blahtex>\n" << gUnicodeConverter.ConvertOut(mainOutput.str()) << "</blahtex>\n";
    }

    // These errors might occur if there's a bug in blahtex that some assertion condition picked up.
    // We still want to report these nicely to the user so that they can notify the developers.
    catch (std::logic_error& e)
    {
        // WARNING: this doesn't XML-encode the message
        // (We don't expect to the message to contain the characters &<>)
        cout << "<blahtex>\n<logicError>" << e.what() << "</logicError>\n</blahtex>\n";
    }

    // These indicate incorrect command line syntax
    catch (CommandLineException& e)
    {
        cout << "Blahtex: " << e.mMessage << " (try \"blahtex --help\")\n";
    }

    // These kind of errors should only occur if the program has been installed incorrectly.
    catch (std::runtime_error& e)
    {
        cout << "Blahtex runtime error: " << e.what() << endl;
    }
    
    return 0;
}
            
// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
