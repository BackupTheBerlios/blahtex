// File "main.cpp"
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

#include "blahtex.h"
#include "UnicodeConverter.h"
#include "md5Wrapper.h"
#include <cerrno>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <stdexcept>

using namespace std;
using namespace blahtex;

string gBlahtexVersion = "0.3.2";

// We only use a single instance of UnicodeConverter.
UnicodeConverter gUnicodeConverter;

// FIX: make blahtex print ShowUsage if no input is ready to read

// FIX: update this message:
void ShowUsage()
{
    cout <<
        "blahtex version " + gBlahtexVersion + "\n"
        "Copyright (C) 2005, David Harvey\n\n"
        "This is free software; see the source for copying conditions. There is NO\n"
        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
        "Usage: blahtex [ --indented ] [ --font-substitution ] [ --spacing N ]\n"
        "               [ --mathml-encoding type ] [ --other-encoding type ]\n"
        "               [ --mathml ] [ --html ] [ --png ] < inputfile\n\n"
        " --indented\n"
        "        produce nicely indented MathML tags\n\n"
        " --font-substitution\n"
        "        use character entities to avoid poorly-supported mathvariant font settings\n\n"
        " --spacing 0\n"
        "        emit spacing commands wherever possible\n"
        " --spacing 1 (default)\n"
        "        emit spacing commands where a MathML renderer is likely to get it wrong\n"
        " --spacing 2\n"
        "        only emit spacing commands where the user specifically requests it\n\n"
        " --mathml\n"
        "        generate MathML output\n\n"
        " --html\n"
        "        generate HTML output (not implemented yet, if ever :-))\n\n"
        " --png\n"
        "        generate PNG output\n\n"
        " --mathml-encoding raw\n"
        "        encode non-ASCII MathML characters as raw UTF-8\n"
        " --mathml-encoding numeric (default)\n"
        "        encode non-ASCII MathML characters as numeric codes (like \"&#x1234;\")\n"
        " --mathml-encoding short\n"
        "        encode non-ASCII MathML characters using short names (like \"&uarr;\")\n"
        " --mathml-encoding long\n"
        "        encode non-ASCII MathML characters using long names (like \"&UpArrow;\")\n\n"
        " --other-encoding raw\n"
        "        encode non-ASCII, non-MathML characters as raw UTF-8\n"
        " --other-encoding numeric\n"
        "        encode non-ASCII, non-MathML characters as numeric codes\n"
    ;
    exit(0);
}

bool FileExists(const string& filename)
{
    struct stat temp;
    if (stat(filename.c_str(), &temp) == -1)
    {
        if (errno == ENOENT)
            return false;
    }
    return true;
}

struct CommandLineException
{
    string mMessage;
    CommandLineException(const string& message) :
        mMessage(message) { }
};

int main (int argc, char * const argv[]) {
    // This outermost try block catches all std::exceptions.
    try
    {
        gUnicodeConverter.Open();

        MathmlOptions options;
        bool indented = false;
        bool doPng    = false;
        bool doMathml = false;
        bool doHtml   = false;
        
        try {
            // Process command line arguments
            for (int i = 1; i < argc; i++)
            {
                string arg(argv[i]);

                if (arg == "--help")
                    ShowUsage();
                
                else if (arg == "--indented")
                    indented = true;
                
                else if (arg == "--spacing")
                {
                    if (++i == argc)
                        throw CommandLineException("Missing number after \"--spacing\" (try blahtex --help)");
                    arg = string(argv[i]);
                    if (arg.empty() || arg[0] < '0' || arg[0] > '2')
                        throw CommandLineException("Illegal number after \"--spacing\" (try blahtex --help)");
                    options.mSpacingExplicitness = atoi(argv[i]);
                }
                
                else if (arg == "--font-substitution")
                    options.mFancyFontSubstitution = true;
                
                else if (arg == "--png")
                    doPng = true;
                
                else if (arg == "--mathml")
                    doMathml = true;
                    
                else if (arg == "--html")
                    doHtml = true;
                
                else if (arg == "--mathml-encoding")
                {
                    if (++i == argc)
                        throw CommandLineException("Missing string after \"--mathml-encoding\" (try blahtex --help)");
                    arg = string(argv[i]);
                    if (arg == "raw")
                        options.mMathmlEncoding = cMathmlEncodingRaw;
                    else if (arg == "numeric")
                        options.mMathmlEncoding = cMathmlEncodingNumeric;
                    else if (arg == "short")
                        options.mMathmlEncoding = cMathmlEncodingShort;
                    else if (arg == "long")
                        options.mMathmlEncoding = cMathmlEncodingLong;
                    else
                        throw CommandLineException("Illegal string after \"--mathml-encoding\" (try blahtex --help)");
                }
                
                else if (arg == "--other-encoding")
                {
                    if (++i == argc)
                        throw CommandLineException("Missing string after \"--other-encoding\" (try blahtex --help)");
                    arg = string(argv[i]);
                    if (arg == "raw")
                        options.mOtherEncodingRaw = true;
                    else if (arg == "numeric")
                        options.mOtherEncodingRaw = false;
                    else
                        throw CommandLineException("Illegal string after \"--other-encoding\" (try blahtex --help)");
                }
                
                else
                    throw CommandLineException("Unrecognised command line option \"" + arg + "\" (try blahtex --help)");
            }
        }
        catch (CommandLineException& e)
        {
            cout << "Blahtex: " << e.mMessage << endl;
            return 0;
        }

        wstring input;
        string inputUtf8;
        char c;
        
        while (cin.get(c))
            inputUtf8 += c;
            
        try
        {
            input = gUnicodeConverter.ConvertIn(inputUtf8);
        }
        catch (UnicodeConverter::Exception& e)
        {
            cout << "<unicodeError>Input contained invalid UTF-8</unicodeError>" << endl;
            return 0;
        }
        
        try
        {
            wostringstream output;

            Instance I;
            I.ProcessInput(input);
            
            if (doMathml)
            {
                auto_ptr<XmlNode> mathml = I.GenerateMathml(options);
                output << L"<mathml>";
                if (indented)
                    output << endl;
                mathml->Print(output, options, indented);
                if (indented)
                    output << endl;
                output << L"</mathml>" << endl;
            }
            
            if (doHtml)
            {
                // ..... ;-)
            }
            
            if (doPng)
            {
                string reconstructed = gUnicodeConverter.ConvertOut(I.GenerateReconstructedLatex());
                string md5 = ComputeMd5(reconstructed);
                
                string shellLatex = "/sw/bin/latex";
                string shellDvips = "/sw/bin/dvips";
                string shellConvert = "/sw/bin/convert";
                
                try {
                    string texFilename = md5 + ".tex";
                    {
                        ofstream texFile(texFilename.c_str(), ios::out | ios::binary);
                        if (!texFile)
                            throw wstring(L"Could not create tex file");
                        texFile << reconstructed;
                    }
                    
                    string command;
                    
                    command = shellLatex + " " + texFilename + " >/dev/null 2>/dev/null";
                    system(command.c_str());
                    
                    if (!FileExists(md5 + ".dvi"))
                        throw wstring(L"Could not run latex");

                    command = shellDvips + " -R -E " + md5 + ".dvi -f -o " + md5 + ".ps 2>/dev/null";
                    system(command.c_str());
                    
                    if (!FileExists(md5 + ".ps"))
                        throw wstring(L"Could not run dvips");
                    
                    command = "/sw/bin/convert -quality 100 -density 200 -gamma 0.5 -trim " + md5 + ".ps " + md5 + ".png >/dev/null 2>/dev/null";
                    system(command.c_str());

                    if (!FileExists(md5 + ".png"))
                        throw wstring(L"Could not run convert");
                    
                    output << L"<png>" << gUnicodeConverter.ConvertIn(md5) << L"</png>" << endl;
                }
                catch (wstring& s)
                {
                    output << L"<pngError>" << s << L"</pngError>" << endl;
                }

                unlink((md5 + ".dvi").c_str());
                unlink((md5 + ".aux").c_str());
                unlink((md5 + ".log").c_str());
                unlink((md5 + ".ps" ).c_str());
                unlink((md5 + ".tex").c_str());
            }
            
            cout << gUnicodeConverter.ConvertOut(output.str());
        }
        
        catch (UnicodeConverter::Exception& e)
        {
            throw logic_error("Unicode conversion problem");
        }

        catch (Exception& e)
        {
            cout << gUnicodeConverter.ConvertOut(e.GetXml(options.mOtherEncodingRaw)) << endl;
            return 0;
        }

    }

    // These errors might occur if there's a bug in blahtex that some assertion condition picked up.
    // We still want to report these nicely to the user so that they can notify the developers.
    catch (std::logic_error& e)
    {
        // WARNING: this doesn't XML encode anything, because we don't expect to the message
        // to contain the characters &<>
        cout << "<logicError>" << e.what() << "</logicError>" << endl;
        return 0;
    }

    // These kind of errors should only occur if the program has been installed incorrectly, etc.
    catch (std::runtime_error& e)
    {
        cout << "blahtex: " << e.what() << endl;
        return 0;
    }
    
    return 0;
}

// end of file @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
