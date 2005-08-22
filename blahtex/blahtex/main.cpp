/*
File "main.cpp"

blahtex (version 0.2.1): a LaTeX to MathML converter designed specifically for MediaWiki
Copyright (C) 2005, David Harvey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "blahtex.h"
#include "parser.tab.h"

string blahtex_version = "0.2.1";

/* Whether to render as "displayed" or "inline" equation.
The only effect this flag has is on the enclosing <mstyle> tags. */
bool mode_displayed = true;

// If this flag is set, blahtex will nicely indent the MathML output.
bool mode_indented = false;

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Here are some "tweak flags".

These indicate that various adjustments should be made to the MathML output to correct bugs or
limitations in browsers.

Currently the only browser with tweaks aimed at it is Mozilla 1.7.1 on Windows (and possibly other
related browsers).
*/

/* This flag indicates that "&Vert;" (double vertical bar symbol) doesn't stretch properly in the
vertical direction. This means things like "\Vmatrix{...}" and "\left\Vert ..." are broken.

Our workaround is to replace it by two consecutive vertical bar symbols whenever it needs to get
stretched. The main downside to this workaround is that the bars are spaced slightly too far apart.
*/
bool tweak_Vert_stretch = false;

/* This flag indicates that the "script", "fraktur" and "double-struck" fonts are broken.
(i.e. they just render as normal font.)

Our workaround is to explicitly substitute entities like "&Ropf;" (double-struck R). */
bool tweak_fancy_fonts = false;

/* This flag indicates that "&nbsp;" seems to have "negative width" inside <mtext> blocks. This
causes (visible) text in the <mtext> block to overlap symbols following the block.

Our workaround is to replace each "&nbsp;" with <mstyle>&nbsp;</mstyle>. For some reason this
makes Mozilla happy. */
bool tweak_mtext_nbsp = false;

/* This flag indicates that the prime character ("&prime;") does not display correctly when in
bold font. (It is placed far too high.)

Our workaround is to disallow bold primes; they are just drawn in normal font. */
bool tweak_bold_prime = false;

/* This flag indicates that the "&InvisibleTimes;" operator has too much space around it when
enclosed in <mo> tags.

Our workaround is to simply leave out the <mo> tags, i.e. to do something like this:
   <mi>x</mi>&InvisibleTimes;<mi>y</mi>.
Although such MathML is not technically valid, it seems to work, and corrects the spacing problem.

(Mysteriously, for Mozilla it is also necessary that there be no whitespace between "&it;" and
neighbouring tags. For example, the following markup still leaves way too much space:
   <mi>x</mi>   &InvisibleTimes;   <mi>y</mi>  ) */
bool tweak_weird_invisible_times = false;


/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
A few global utility functions */

/* This determines whether a character is a plain vanilla alphabetic character.
I don't trust iswalpha; it might treat funny characters with accents as alphabetic. */
bool is_plain_alpha(wchar_t c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* Parameter to pass to iconv_open to describe our wchar_t format: one of "UCS-4BE" or "UCS-4LE".
Really we should be able to just use "WCHAR_T", which apparently works on linux, but not on darwin.
So we work it out at runtime in main(). */
const char* iconv_setting;

// Converts a UTF-8 string to wchar_t. (Wrapper for iconv.)
wstring convert_utf8_to_wchar_t(const string& input) {
	iconv_t converter;
	converter = iconv_open(iconv_setting, "UTF-8");
	if (converter == (iconv_t)(-1))
		throw internal_error(L"Could not create UTF-8 to wchar_t conversion table.");
	
	wchar_t* outputbuf = new wchar_t[input.size()];
	char* dest = reinterpret_cast<char*>(outputbuf);
	char* inputbuf = new char[input.size()];
	memcpy(inputbuf, input.c_str(), input.size());
	/* The following #define is needed to handle the unfortunate inconsistency between
	linux and BSD definitions for the second parameter of iconv() */
#ifdef BLAHTEX_ICONV_CONST
	// For BSD (including Mac OS X)
	const char* source = inputbuf;
#else
	// For Linux
	char* source = inputbuf;
#endif
	size_t inbytesleft = input.size();
	size_t outbytesleft = 4 * input.size();
	if (iconv(converter, &source, &inbytesleft, &dest, &outbytesleft) == -1)
		throw user_error(L"Could not convert UTF-8 string to wchar_t.");
	iconv_close(converter);
	
	wstring output(outputbuf, input.size() - outbytesleft/4);
	delete outputbuf;
	delete inputbuf;
	return output;
}

// Converts a wchar_t string to UTF-8. (Wrapper for iconv.)
string convert_wchar_t_to_utf8(const wstring& input) {
	iconv_t converter;
	converter = iconv_open("UTF-8", iconv_setting);
	if (converter == (iconv_t)(-1))
		throw internal_error(L"Could not create wchar_t to UTF-8 conversion table.");
	
	char* outputbuf = new char[4 * input.size()];
	char* dest = outputbuf;
	wchar_t* inputbuf = new wchar_t[input.size()];
	wmemcpy(inputbuf, input.c_str(), input.size());
	/* The following #define is needed to handle the unfortunate inconsistency between
	linux and BSD definitions for the second parameter of iconv() */
#ifdef BLAHTEX_ICONV_CONST
	const char* source = const_cast<const char*>(reinterpret_cast<char*>(inputbuf));
#else
	char* source = reinterpret_cast<char*>(inputbuf);
#endif
	size_t inbytesleft = 4 * input.size();
	size_t outbytesleft = 4 * input.size();
	if (iconv(converter, &source, &inbytesleft, &dest, &outbytesleft) == -1)
		throw user_error(L"Could not convert wchar_t string to UTF-8.");
	iconv_close(converter);
	
	string output(outputbuf, 4 * input.size() - outbytesleft);
	delete outputbuf;
	delete inputbuf;
	return output;
}

// Prints command line usage.
void show_usage() {
	cout <<
		"blahtex version " + blahtex_version + "\n"
		"Copyright (C) 2005 David Harvey\n\n"
		"This is free software; see the source for copying conditions. There is NO\n"
		"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
		"Usage: blahtex [ -inline ] [ -indented ] [ -tweak-mozilla171-win ] < inputfile\n\n"
		" -inline      output \"inline\" equation instead of \"displayed\" equation\n"
		" -indented    produce nicely indented MathML tags, instead of tags just strung together\n"
		" -tweak-xxx   tweak the output to work best on the given browser/OS combination\n";
	exit(0);
}

// Reads standard input, converting UTF-8 to wchar_t.
wstring get_input() {
	string input;
	char c;
	while (cin.get(c)) input += c;
	return convert_utf8_to_wchar_t(input);
}

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Main function */

int main (int argc, char * const argv[]) {
	{
		/* determine whether wchar_t is big-endian or little-endian,
		for the UTF-8 <=> wchar_t conversion routines */
		if (sizeof(wchar_t) != 4) {
			cout << "Sorry, blahtex only runs on systems whose wchar_t is four bytes wide.\n";
			return 0;
		}
		wchar_t test_char = L'A';
		iconv_setting = (*(reinterpret_cast<char*>(&test_char)) == 'A') ? "UCS-4LE" : "UCS-4BE";
	}
	
	// Process command line arguments
	for (int i = 1; i < argc; i++) {
		string arg(argv[i]);

		if (arg == "-help")
			show_usage();
		
		else if (arg == "-inline")
			mode_displayed = false;
		
		else if (arg == "-indented")
			mode_indented = true;
		
		else if (arg == "-tweak-mozilla171-win") {
			tweak_Vert_stretch = true;
			tweak_fancy_fonts = true;
			tweak_mtext_nbsp = true;
			tweak_bold_prime = true;
			tweak_weird_invisible_times = true;
		}
	}
	
	try {
		wstring input = get_input();
		split_into_tokens(input, input_tokens);

		// Generate parse tree
		yyparse();

		// Generate layout tree
		layout_node* layout_tree_root = parse_root->build_layout_tree(latex_math_font());
		layout_tree_root->merge_numerals();

		// Generate MathML tree
		mathml_node* mathml_root = layout_tree_root->build_mathml_tree();

		wstring output;
		wostringstream s;

		if (mode_displayed) {
			s << L"<mstyle displaystyle=\"true\">";
			if (mode_indented) s << endl;
		}

		// Write the MathML output
		mathml_root->write_output(s);
		
		if (mode_displayed) {
			if (mode_indented) s << endl;
			s << L"</mstyle>";
		}

		cout << convert_wchar_t_to_utf8(s.str());
		return 0;
	}
	catch (blahtex_error& e) {
		cout << convert_wchar_t_to_utf8(e.message);
		return 0;
	}
	catch (...) {
		cout << "An unexpected exception occurred.";
		return 0;
	}
}

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
end of file */
