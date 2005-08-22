/*
File "blahtex.h"

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

#include <string>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <iostream>
#include <sstream>
#include <iconv.h>

using namespace std;

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Some globals - these are all documented where they are defined */

extern wstring convert_utf8_to_wchar_t(const string& input);
extern string convert_wchar_t_to_utf8(const wstring& input);
extern bool is_plain_alpha(wchar_t c);

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
exception classes */

struct blahtex_error {
	wstring message;
	
	blahtex_error(const wstring& new_message) :
		message(L"blahtex error: " + new_message) { }
};

struct internal_error : blahtex_error {
	internal_error(const wstring& new_message) :
		blahtex_error(
			wstring(
				L"An internal error has occurred. This really shouldn't have happened. "
				L"Please accept our apologies, and let the blahtex developers know. "
				L"The error message was: \"" + new_message + L"\" Thanks. ")) { };
};

struct user_error : blahtex_error {
	user_error(const wstring& new_message) :
		blahtex_error(wstring(L"The supplied input was invalid, because: ") + new_message) { }
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Simple hash table with wstring keys. This is used for some hard-coded lookup tables.

- Keys and values are stored as POINTERS to already existing data, to speed up building the table.
  Therefore the keys/values inserted MUST remain at their current address during table lifetime.
- Table size fixed at initialisation.
- Uses simple open addressing (just tries successive slots).
- Supports lookup and insertion (insertion ASSUMES supplied key does not already exist in table,
  and that the table will not become full).

It would be nice if we could compute the hashes at compile-time, but C++ is not that enlightened.
*/

template <typename T> struct dictionary_item {
	wstring key;
	T value;
};

template <typename T> class hash_table {
	vector<pair<const wstring*, const T*> > data;

	/* This is based on a hash function I picked up on the interweb somewhere. I have no idea
	if it's any good, but it seems reasonable enough a priori, and looks to be quite fast. */
	unsigned hash_string(const wstring& key) const {
		unsigned hash = key.size();
		for (wstring::const_iterator ptr = key.begin(); ptr != key.end(); ptr++)
			hash = (hash << 6) + (hash << 16) - hash + static_cast<unsigned>(*ptr);
		return hash;
	}
	
public:
	// Constructs a hash table from an array of key/value pairs to be inserted.
	hash_table(const dictionary_item<T>* begin, const dictionary_item<T>* end) {
		data.resize((end - begin) * 4, pair<const wstring*, const T*>(NULL, NULL));
		while (begin != end) {
			insert(begin->key, &begin->value);
			begin++;
		}
	}

	// returns NULL if key is not found
	const T* lookup(const wstring& key) const {
		unsigned hash = hash_string(key) % data.size();
		typename vector<pair<const wstring*, const T*> >::const_iterator search =
			data.begin() + hash;
		while (search->first != NULL) {
			if (*search->first == key) return search->second;
			if (++search == data.end())
				search = data.begin();
		}
		return NULL;
	}
	
	void insert(const wstring& key, const T* value) {
		unsigned hash = hash_string(key) % data.size();
		typename vector<pair<const wstring*, const T*> >::iterator search = data.begin() + hash;
		while (search->first != NULL) {
			if (++search == data.end())
				search = data.begin();
		}
		search->first = &key;
		search->second = value;
	}
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Here is some stuff to do with fonts.

Dealing with fonts is a little more complicated than one might first guess. For example latex
handles the expression "z + \Sigma + \mathbf{x + \alpha + \Lambda}" as follows:

- "z" is in italics
- "\Sigma" is NOT in italics
- "x" is in bold (no italics)
- "\alpha" is in ITALICS, not bold
- "\Lambda" is bold, no italics.
- Even inside the \mathbf{...} command, operators like "+" are NOT in bold.

So it's not as simple as just using <mstyle mathvariant="bold">...</mstyle> for "\mathbf".

There are additional complications due to \boldsymbol, which works somewhat orthogonally to
font commands like "\mathbf" and "\mathit", and which DOES apply to things like "+" and "\alpha".

Also, MathML tries to make life "easier" by switching to italics for an identifier that is
only one character long, but using normal font for longer identifiers. So we need to compensate
for that.

Finally, Mozilla doesn't currently support the fonts "script", "fraktur", "double-struck" (and
bolded versions thereof), so we have a tweak (see "tweak_fancy_fonts") which explicitly
substitutes the correct character entities like "&Aopf;".

(Fonts in text mode are much simpler.)

Therefore, our approach is very conservative. We simulate the effects of latex font commands as
we build the layout tree (using a "latex_math_font" object), and for each symbol in the output we
approximate by the best available MathML font.
*/

/* These are all the fonts supported by MathML. (There *are* other font commands, but they are
deprecated in MathML 2.0, so we don't use them.) */
enum mathml_font {
	mathml_font_NA,
	mathml_font_normal,
	mathml_font_bold,
	mathml_font_italic,
	mathml_font_bold_italic,
	mathml_font_double_struck,
	mathml_font_bold_fraktur,
	mathml_font_script,
	mathml_font_bold_script,
	mathml_font_fraktur,
	mathml_font_sans_serif,
	mathml_font_bold_sans_serif,
	mathml_font_sans_serif_italic,
	mathml_font_sans_serif_bold_italic,
	mathml_font_monospace
};

// strings corresponding to each of the above fonts
extern wstring mathml_font_strings[];

// Represents the latex font state during math mode.
struct latex_math_font {
	enum types {
		none,  // whatever is default
		rm,    // roman
		bf,    // bold
		it,    // italics
		sf,    // sans serif
		tt,    // typewriter
		bb,    // blackboard bold
		cal,   // calligraphic
		frak   // fraktur
	} type;
	bool boldsymbol;
	
	latex_math_font() : type(none), boldsymbol(false) { }
	
	// Determine which MathML font best represents the given latex font.
	mathml_font get_mathml_approximation() const;
};

// Represents the latex font state during text mode.
struct latex_text_font {
	enum families {
		rm,    // roman family
		sf,    // sans serif family
		tt     // typewriter family
	} family;
	bool bold;
	bool italic;
	
	latex_text_font(families new_family = rm, bool new_bold = false, bool new_italic = false) :
		family(new_family), bold(new_bold), italic(new_italic) { }

	// Works out which MathML font best represents the given latex font.
	mathml_font get_mathml_approximation() const;
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Structs used in various hard-coded lookup tables */

// For each latex command, gives the corresponding tokens produced during math mode and text mode
struct latex_command_info {
	int math_token;
	int text_token;
};

/* Specifies which delimiters should go around each type of environment
(e.g. pmatrix gets "(" and ")") */
struct environment_info {
	wstring left_delimiter;
	wstring right_delimiter;
};

// Information about a latex identifier command.
struct identifier_info {
	// is the identifier italic by default (false for capital greek letters, "\infty", etc.)
	bool italic;
	// stuff to put between the <mi>...</mi> tags (e.g. "&alpha;")
	wstring text;		
};

// Information about a latex operator command.
struct operator_info {
	/* does this operator have scripts under/over by default instead of sub/sup?
	True for things like "lim" and "\sum". */
	bool underover;
	// stuff to put between the <mo>...</mo> tags.
	wstring text;
};

// Information about a latex accent command.
struct accent_info {
	// stuff to put between the <mo>...</mo> tags.
	wstring text;
	// e.g. true for "\widehat", false for "\hat"
	bool stretchy;
	// true means this is an over-accent, false means an under-accent
	bool over;
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Here are some classes representing the MathML tree.

The MathML tree corresponds precisely to the MathML output; the "write_output" function converts
the tree to linear text.

The layout tree to MathML tree conversion is handled in "mathml.cpp".
*/

struct mathml_node {
	// Recursively write the MathML tree to a stream, taking into account indentation if required.
	virtual void write_output(wostream& os, int depth = 0) const = 0;
};

// Represents a literal string in a MathML expression.
struct mathml_string_node : mathml_node {
	wstring text;
	
	mathml_string_node(const wstring& new_text) : text(new_text) { }
	virtual void write_output(wostream& os, int depth = 0) const;
};

// Represents a pair of tags <mxyz>...</mxyz>.
struct mathml_tag_node : mathml_node {
	enum types {
		mrow,
		mi,
		mo,
		mn,
		mfrac,
		msup,
		msub,
		msubsup,
		munder,
		mover,
		munderover,
		msqrt,
		mroot,
		mtext,
		mtable,
		mtr,
		mtd,
		mstyle,
		mspace
	} type;

	list<mathml_node*> children;
	
	// Attributes appearing in the opening tag, e.g. attributes[L"mathvariant"] = L"bold"
	map<wstring, wstring> attributes;

	mathml_tag_node(types new_type) : type(new_type) { }
	virtual void write_output(wostream& os, int depth = 0) const;
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Here are some classes representing the layout tree.

The layout tree is an intermediate stage between the parse tree and the MathML tree.

It does not contain any latex commands; all symbols like "\alpha" have been converted to MathML
entities, and all fonts have been converted to their best MathML approximations.

It has essentially the same structure as the MathML tree, but it is encoded in a way that is
programatically easier to work with.

The layout tree to MathML tree conversion is handled in "mathml.cpp".
The parse tree to layout tree conversion is handled in "layout.cpp".

The class hierarchy for nodes representing the layout tree is:

layout_node
	layout_textchar
	layout_textblock
	layout_join
	layout_identifier
	layout_numeral
	layout_operator
		layout_simple_operator
		layout_compound_operator
	layout_space
	layout_scripts
	layout_fraction
	layout_sqrt
	layout_root
	layout_table_row
	layout_table
*/

struct layout_node {
	/* Finds the base of an expression containing nested super/subscripts. This is required (for
	example) to work out that in the expression "a^{b^c} p_q +_x", an &InvisibleTimes; operator
	should be inserted between "a^{b^c}" and "p_q" (because the bases "a" and "p" are both
	identifiers) but not between "p_q" and "+_x" (because "+" is not an identifier). */
	virtual const layout_node* get_deepest_base() const { return this; }

	// Recursively builds the MathML tree corresponding to this layout node.
	virtual mathml_node* build_mathml_tree() const
	{ throw internal_error(L"Unexpected call to build_mathml_tree."); }
	
	/* Recursively merges adjacent numeral nodes, so we get <mn>42</mn> instead
	of <mn>4</mn><mn>2</mn>. The return value is a replacement for the node, should the node
	wish to change itself (e.g. <mrow><mn>4</mn><mn>2</mn></mrow> should be replaced with
	<mn>42</mn>, not <mrow><mn>42</mn></mrow>). */
	virtual layout_node* merge_numerals() { return this; }
};

// Represents a single character in text mode. (Or a few characters on rare occasions.)
struct layout_textchar : layout_node {
	// Something like "a" or "&amp;" or even "&nbsp;&nbsp;"
	wstring text;
	// The font that this character should be rendered in.
	mathml_font font;
	
	layout_textchar(const wstring& new_text, mathml_font new_font) :
		text(new_text), font(new_font) { }
};

/* Represents a sequence of characters in text mode.
This will become an <mtext> element in the MathML tree. */
struct layout_textblock : layout_node {
	list<layout_textchar*> children;
	
	virtual mathml_node* build_mathml_tree() const;
};

/* Represents a sequence of nodes in math mode. These kind of nodes are spliced together so that
no layout_join node has another layout_join node as a child (except in cases involving
"prohibit_merge"). */
struct layout_join : layout_node {
	/* "prohibit_merge" is set if this node should not merge with a parent layout_join.
	This is necessary so that we know to put <mrow>...</mrow> tags around something like
		<mo stretchy="true">(></mo> ... <mo stretchy="false">)</mo>
	so that the parentheses only stretch for the things that they enclose. */
	bool prohibit_merge;
	
	list<layout_node*> children;

	layout_join() : prohibit_merge(false) { }
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

// Represents an identifier, i.e. anything that eventually will be inside <mi>...</mi> tags.
struct layout_identifier : layout_node {
	// The string that goes between <mi> tags (e.g. "a" or "&alpha;" or "sin")
	wstring text;
	// The font that this identifier should be rendered in.
	mathml_font font;
	/* "function_style" is true for something like "sin". This flag is used to work out where
	the "&ApplyFunction;" operator needs to go. */
	bool function_style;

	layout_identifier(
		const wstring& new_text, mathml_font new_font, bool new_function_style = false) :
		text(new_text), font(new_font), function_style(new_function_style) { }
	virtual mathml_node* build_mathml_tree() const;
};

// Represents a numeral, i.e. anything that eventually will be inside <mn>...</mn> tags.
struct layout_numeral : layout_node {
	// The string that goes between <mn> tags (e.g. "42"). Only contains digits.
	wstring text;
	// The font that this numeral should be rendered in.
	mathml_font font;

	layout_numeral(const wstring& new_text, mathml_font new_font) :
		text(new_text), font(new_font) { }
	virtual mathml_node* build_mathml_tree() const;
};

/* Represents an operator. This includes things that will be inside <mo>...</mo> tags
(layout_simple_operator), but also things enclosed by the "\mathop" command
(layout_compound_operator). */
struct layout_operator : layout_node {
	// True for operators like "lim" or "\sum" which have scripts under/over rather than sub/sup.
	bool underover;
	
	layout_operator(bool new_underover) : underover(new_underover) { }
};

// Represents an operator constructed directly from a LaTeX command or operator character.
struct layout_simple_operator : layout_operator {
	// The string that goes between <mo> tags (e.g. "(" or "+" or "&int;" or "lim").
	wstring text;
	// Is this a stretchy operator?
	bool stretchy;
	/* If "stretchy" is set and "size" is non-empty, then "size" indicates the vertical size that
	the operator should be. (e.g. "2.5em".) This is used to implement the "\big" commands, by
	setting the "minsize" and "maxsize" attributes. */
	wstring size;
	// The font that this operator should be rendered in.
	mathml_font font;
	
	layout_simple_operator(const wstring& new_text, mathml_font new_font) :
		text(new_text), font(new_font), stretchy(false), layout_operator(false) { }
	virtual mathml_node* build_mathml_tree() const;
};

/* Represents an operator formed by the "\mathop" command. (We use a separate enclosing node so
that it is easier to implement the \limits and \nolimits commands.) */
struct layout_compound_operator : layout_operator {
	// The stuff being treated as a single operator.
	layout_node* child;
	
	layout_compound_operator(layout_node* new_child) : child(new_child), layout_operator(true) { }
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

// Represents a space element (i.e. an <mspace>).
struct layout_space : layout_node {
	// The width of the space, e.g. "2.5em"
	wstring width;
	
	layout_space(const wstring& new_width) : width(new_width) { }
	virtual mathml_node* build_mathml_tree() const;
};

/* Represents sub/superscripts (or both together) or under/overscripts (or both together) or
under/over accents (or both together).

The "underover" flag determines whether these are under/over or sub/super scripts. It is determined
while building the layout tree, by examining its base, in particular the underover flag of any
layout_operator appearing as the base.

If "accent" is true, then "underover" is ignored. */
struct layout_scripts : layout_node {
	layout_node* base;
	// NULL if no lower script
	layout_node* lower;
	// NULL if no upper script
	layout_node* upper;
	bool underover;
	bool accent;

	layout_scripts(
		layout_node* new_base, layout_node* new_lower, layout_node* new_upper, bool new_underover) :
		base(new_base), lower(new_lower), upper(new_upper), accent(false),
		underover(new_underover) { }
	
	virtual const layout_node* get_deepest_base() const { return base->get_deepest_base(); }
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

/* Represents a fraction (i.e. an <mfrac> element), including things like binomial coefficients
which don't have a horizontal line. */
struct layout_fraction : layout_node {
	layout_node* numerator;
	layout_node* denominator;
	// false for things like binomial coefficients
	bool visible_line;

	layout_fraction(
		layout_node* new_numerator, layout_node* new_denominator, bool new_visible_line = true) :
		numerator(new_numerator), denominator(new_denominator), visible_line(new_visible_line) { }
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

// Represents the square root of whatever is "inside" (i.e. an <msqrt> element).
struct layout_sqrt : layout_node {
	layout_node* inside;
	
	layout_sqrt(layout_node* new_inside) : inside(new_inside) { }
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

// Represents a general radical (i.e. an <mroot> element).
struct layout_root : layout_node {
	layout_node* inside;
	layout_node* outside;
	
	layout_root(layout_node* new_inside, layout_node* new_outside) :
		inside(new_inside), outside(new_outside) { }
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

/* Represents a row of a table (i.e. an <mtr> element, whose entries are enclosed
in <mtd> elements). */
struct layout_table_row : layout_node {
	list<layout_node*> entries;
	
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

// Represents a table (i.e. an <mtable> element).
struct layout_table : layout_node {
	list<layout_table_row*> rows;
	/* "align" is used as the "colalign" attribute of the <mtable> element.
	Currently it is only used to implement the "cases" block, to left-align things. */
	wstring align;
	
	virtual mathml_node* build_mathml_tree() const;
	virtual layout_node* merge_numerals();
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Here are some classes to represent the parse tree.

The parse tree is a tree representation of the input latex expression.

The latex to parse tree conversion is handled in "parse.ypp".
The parse tree to layout tree conversion is handled in "layout.cpp".

The class hierarchy for nodes representing the parse tree is:

parse_node
	parse_text
		parse_text_empty
		parse_text_join
		parse_text_command_1arg
		parse_text_atom
	parse_math
		parse_math_empty
		parse_math_join
		parse_math_atom
		parse_math_script
			parse_math_prime
			parse_math_superscript
			parse_math_subscript
		parse_math_scripts
		parse_math_delimited
		parse_math_command_1arg
		parse_math_command_2args
		parse_math_table
		parse_math_table_row
		parse_math_environment
		parse_enter_text_mode
*/

struct parse_node {
};

// Any node occurring in text mode
struct parse_text : parse_node {
	virtual layout_node* build_layout_tree(const latex_text_font& current_font) const
	{ throw internal_error(L"Unexpected call to build_layout_tree"); }
};

// Any node occurring in math mode
struct parse_math : parse_node {
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const
	{ throw internal_error(L"Unexpected call to build_layout_tree"); }
	
	/* The function "make_operatorname" attempts to make an operator name out of the parse tree
	under this node. If the tree consists only of alphanumeric atoms and spaces, it will succeed
	and place the resulting string in "output". If it fails, it throws an "operatorname_too_hard"
	exception. See the "\operatorname" section of "parse_math_command_1arg::build_layout_tree"
	for more details. */
	struct operatorname_too_hard { };
	virtual void make_operatorname(wstring& output) const
	{ throw operatorname_too_hard(); }
};

// Represents absence of a node in text mode.
struct parse_text_empty : parse_text {
	virtual layout_node* build_layout_tree(const latex_text_font& current_font) const;
};

// Represents absence of a node in math mode.
struct parse_math_empty : parse_math {
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;

	virtual void make_operatorname(wstring& output) const { }
};

// Represents a pair of concatenated nodes in text mode.
struct parse_text_join : parse_text {
	parse_text* child1;
	parse_text* child2;

	parse_text_join(parse_text* new_child1, parse_text* new_child2) :
		child1(new_child1), child2(new_child2) { }
	virtual layout_node* build_layout_tree(const latex_text_font& current_font) const;
};

// Represents a text mode command with one argument (like "\textbf" or "\bf").
struct parse_text_command_1arg : parse_text {
	wstring command;
	parse_text* argument;
	
	parse_text_command_1arg(const wstring& new_command, parse_text* new_argument) :
		command(new_command), argument(new_argument) { }
	virtual layout_node* build_layout_tree(const latex_text_font& current_font) const;
};

// Represents a text mode atom like "a" or "\," or "\backslash".
struct parse_text_atom : parse_text {
	wstring text;
	
	parse_text_atom(const wstring& new_text) : text(new_text) { }
	virtual layout_node* build_layout_tree(const latex_text_font& current_font) const;
};

// Represents a math mode atom like "a" or "2" or "\," or "+" or "\alpha" or "\lim".
struct parse_math_atom : parse_math {
	wstring text;

	parse_math_atom(const wstring& new_text) : text(new_text) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;

	virtual void make_operatorname(wstring& output) const {
		if (text == L"\\," || text == L"\\ ") {
			output += L" ";
			return;
		}
		else if (text.size() == 1) {
			wchar_t c = text[0];
			if (is_plain_alpha(c) || (c >= '0' && c <= '9')) {
				output += c;
				return;
			}
		}
		else throw operatorname_too_hard();
	}
};

// Represents a pair of concatenated nodes in math mode.
struct parse_math_join : parse_math {
	parse_math* child1;
	parse_math* child2;
	
	parse_math_join(parse_math* new_child1, parse_math* new_child2) :
		child1(new_child1), child2(new_child2) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;

	virtual void make_operatorname(wstring& output) const {
		child1->make_operatorname(output);
		child2->make_operatorname(output);
	}
};

// Represents a script attached to a base; either a subscript, a superscript, or a prime.
struct parse_math_script : parse_math {
};

// Represents a subscript attached to a base.
struct parse_math_subscript : parse_math_script {
	parse_math* subscript;
	
	parse_math_subscript(parse_math* new_subscript) : subscript(new_subscript) { }
};

// Represents a superscript attached to a base.
struct parse_math_superscript : parse_math_script {
	parse_math* superscript;
	
	parse_math_superscript(parse_math* new_superscript) : superscript(new_superscript) { }
};

// Represents a prime attached to a base.
struct parse_math_prime : parse_math_script {
};

/* Represents a node with several scripts attached, and possibly some "limit controls".
The scripts are attached in any order, and we haven't yet checked if there are double
superscripts, etc. */
struct parse_math_scripts : parse_math {
	// something like "\limits" or "\nolimits"; ignored if empty
	wstring limits;
	parse_math* base;
	list<parse_math_script*> scripts;
	
	parse_math_scripts(parse_math* new_base) : base(new_base) { }
	parse_math_scripts(parse_math* new_base, const wstring& new_limits) :
		base(new_base), limits(new_limits) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

/* Represents a delimited expression (something like "\left( ... \right]"). At this stage we
allow the delimiters to be any math atom. */
struct parse_math_delimited : parse_math {
	// The stuff inside the delimiters.
	parse_math* enclosed;
	parse_math_atom* left;
	parse_math_atom* right;
	
	parse_math_delimited(
		parse_math* new_enclosed, parse_math_atom* new_left, parse_math_atom* new_right) :
		enclosed(new_enclosed), left(new_left), right(new_right) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

// Represents a latex command with one argument (e.g. "\sqrt", "\mathbf", "\mathop")
struct parse_math_command_1arg : parse_math {
	// The command (e.g. "\sqrt")
	wstring command;
	// The argument to the command
	parse_math* argument;
	
	parse_math_command_1arg(const wstring& new_command, parse_math* new_argument) :
		command(new_command), argument(new_argument) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

/* Represents a latex command with two arguments (e.g. "\frac", "\choose").
This includes commands like "\sqrt" which have an optional argument present. */
struct parse_math_command_2args : parse_math {
	// The command (e.g. "\sqrt")
	wstring command;
	// The two arguments.
	parse_math* argument1;
	parse_math* argument2;

	parse_math_command_2args(
		const wstring& new_command, parse_math* new_argument1, parse_math* new_argument2) :
		command(new_command), argument1(new_argument1), argument2(new_argument2) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

// Represents a row of a table.
struct parse_math_table_row : parse_math {
	list<parse_math*> entries;

	parse_math_table_row(parse_math* new_entry) { entries.push_back(new_entry); }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

// Represents a table.
struct parse_math_table : parse_math {
	list<parse_math_table_row*> rows;
	
	parse_math_table(parse_math_table_row* new_row) { rows.push_back(new_row); }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

/* Represents an environment, such as "\begin{pmatrix} ... \end{pmatrix}", or the simpler notation
"\pmatrix{...}" (which is deprecated in standard latex).

Note: in this implementation, we assume that every environment contains a table. This is of course
not true in latex; but currently the only ones we want to implement have this property. */
struct parse_math_environment : parse_math {
	// The name of the environment (e.g. "pmatrix" or "cases")
	wstring name;
	// The table contained in the environment.
	parse_math_table* child;
	
	parse_math_environment(const wstring& new_name, parse_math_table* new_child) :
		name(new_name), child(new_child) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

// Represents a command that indicates entry to text mode (e.g. "\textbf" or "\mbox").
struct parse_enter_text_mode : parse_math {
	// The relevant command.
	wstring command;
	// The stuff in text mode.
	parse_text* argument;
	
	parse_enter_text_mode(const wstring& new_command, parse_text* new_argument) :
		command(new_command), argument(new_argument) { }
	virtual layout_node* build_layout_tree(const latex_math_font& current_font) const;
};

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
More globals - these are all documented where they are defined */

extern bool mode_displayed;
extern bool mode_indented;

extern bool tweak_Vert_stretch;
extern bool tweak_fancy_fonts;
extern bool tweak_mtext_nbsp;
extern bool tweak_bold_prime;
extern bool tweak_weird_invisible_times;

extern void split_into_tokens(const wstring& input, vector<wstring>& output);
extern vector<wstring> input_tokens;
extern parse_math* parse_root;
extern int yyparse();

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
end of file */
