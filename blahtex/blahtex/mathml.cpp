/*
File "mathml.cpp"

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

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Stuff to build the MathML tree from the layout tree. */

// Strings corresponding to the mathml_font_xxx constants.
wstring mathml_font_strings[] = {
	L"",
	L"normal",
	L"bold",
	L"italic",
	L"bold-italic",
	L"double-struck",
	L"bold-fraktur",
	L"script",
	L"bold-script",
	L"fraktur",
	L"sans-serif",
	L"bold-sans-serif",
	L"sans-serif-italic",
	L"sans-serif-bold-italic",
	L"monospace",
};

/* Converts certain characters to entities so they can be safely put inside an <mtext> block.
Also handles the "&nbsp; broken inside <mtext>" tweak. */
wstring protect_char(const wstring& input) {
	if      (input == L"<") return L"&lt;";
	else if (input == L">") return L"&gt;";
	else if (input == L" ") return tweak_mtext_nbsp ? L"<mstyle>&nbsp;</mstyle>" : L"&nbsp;";
	else return input;
}

/* This function converts a "layout_textblock" node to a MathML <mtext> block. It groups
characters into blocks based on their font and creates appropriate <mstyle> tags. */

/* FIX: currently this code produces <mstyle> nodes as children of an <mtext>. This doesn't
appear to be allowed by the MathML standard, but seems to work with every browser I've tried.
What to do about this? */
mathml_node* layout_textblock::build_mathml_tree() const {
	if (children.empty())
		return new mathml_tag_node(mathml_tag_node::mtext);
	
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mtext);
	list<layout_textchar*>::const_iterator ptr, previous;
	
	for (ptr = children.begin(); ptr != children.end(); ptr++) {
		if ((*ptr)->font == mathml_font_normal) {
			if (ptr == children.begin() || (*ptr)->font != (*previous)->font) {
				new_node->children.push_back(new mathml_string_node(protect_char((*ptr)->text)));
				previous = ptr;
			}
			else {
				dynamic_cast<mathml_string_node*>(new_node->children.back())->text
					+= protect_char((*ptr)->text);
			}
		}
		else {
			if (ptr == children.begin() || (*ptr)->font != (*previous)->font) {
				mathml_tag_node* new_child = new mathml_tag_node(mathml_tag_node::mstyle);
				new_child->children.push_back(new mathml_string_node(protect_char((*ptr)->text)));
				new_child->attributes[L"mathvariant"] = mathml_font_strings[(*ptr)->font];
				new_node->children.push_back(new_child);
				previous = ptr;							
			}
			else {
				dynamic_cast<mathml_string_node*>(dynamic_cast<mathml_tag_node*>(
					new_node->children.back())->children.front())->text +=
						protect_char((*ptr)->text);
			}
		}
	}
	
	if (new_node->children.size() == 1) {
		mathml_tag_node* is_tag = dynamic_cast<mathml_tag_node*>(new_node->children.front());
		if (is_tag && is_tag->type == mathml_tag_node::mstyle) {
			is_tag->type = mathml_tag_node::mtext;
			return is_tag;
		}
	}
	
	return new_node;
}

/* This function converts a "layout_join" node into an <mrow> sequence.
It also does a few other things:

- Merges adjacent numeral nodes into single numeral nodes, as long as they have the same font
  (e.g. <mn>2</mn><mn>4</mn> becomes <mn>24</mn>)
- Places <mo>&InvisibleTimes;</mo> operators between identifiers and identifiers, and between
  numerals and identifiers.
- Places <mo>&ApplyFunction;</mo> operators after "function-style" operators (like "\sin").
- Handles the "&Vert;" vertical stretching bug.
*/

mathml_node* layout_join::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mrow);

	list<layout_node*>::const_iterator ptr = children.begin();
	while (ptr != children.end()) {

		// Convert a stretchy "&Vert;" to two vertical bars if necessary.
		const layout_simple_operator* is_operator = dynamic_cast<layout_simple_operator*>(*ptr);
		if (tweak_Vert_stretch && is_operator && is_operator->text == L"&Vert;"
			&& is_operator->stretchy) {
			
			for (int i = 0; i < 2; i++) {
				mathml_tag_node* bar = new mathml_tag_node(mathml_tag_node::mo);
				bar->children.push_back(new mathml_string_node(L"|"));
				bar->attributes[L"stretchy"] = L"true";
				new_node->children.push_back(bar);
			}
			ptr++;
			continue;
		}
		
		// Insert &InvisibleTimes; and &ApplyFunction; as appropriate.
		if (ptr != children.begin()) {
			list<layout_node*>::const_iterator previous = ptr;
			previous--;

			const layout_node* previous_base = (*previous)->get_deepest_base();
			const layout_identifier* is_identifier =
				dynamic_cast<const layout_identifier*>(previous_base);
			if (is_identifier && is_identifier->function_style) {
				new_node->children.push_back(layout_simple_operator(L"&af;",
					mathml_font_normal).build_mathml_tree());
			}
			else {
				const layout_node* ptr_base = (*ptr)->get_deepest_base();
				
				const layout_identifier* ptr_is_identifier =
					dynamic_cast<const layout_identifier*>(ptr_base);

				if (
					(dynamic_cast<const layout_numeral*>(ptr_base) ||
					 (ptr_is_identifier && !ptr_is_identifier->function_style))
					&&
					(dynamic_cast<const layout_numeral*>(previous_base) ||
					 dynamic_cast<const layout_identifier*>(previous_base)))
				{
					if (tweak_weird_invisible_times)
						new_node->children.push_back(new mathml_string_node(L"&it;"));
					else
						new_node->children.push_back(layout_simple_operator(L"&it;",
							mathml_font_normal).build_mathml_tree());
				}
			}
		}
		
		// Finally: convert the current node to MathML.
		new_node->children.push_back((*ptr)->build_mathml_tree());
		ptr++;
	}
	
	if (new_node->children.size() == 1) return new_node->children.front();
	else return new_node;
}

/* Counts the numbers of characters in the input, where an entity like "&nbsp;" counts as a
single character. */
int count_characters_and_entities(const wstring& input) {
	int count = 0;
	for (wstring::const_iterator ptr = input.begin(); ptr != input.end(); ptr++, count++) {
		if (*ptr == L'&') {
			do ptr++; while (ptr != input.end() && *ptr != L';');
		}
	}
	return count;
}

/* This function converts a layout_identifier node to MathML.
It handles the "fancy fonts" bug (where double_stuck, script, and fraktur characters need to be
explicitly substituted.) */

mathml_node* layout_identifier::build_mathml_tree() const {
	if (tweak_fancy_fonts) {
		wstring suffix;
		bool bold;
		switch (font) {
			case mathml_font_double_struck:   suffix = L"opf;"; bold = false; break;
			case mathml_font_script:          suffix = L"scr;"; bold = false; break;
			case mathml_font_bold_script:     suffix = L"scr;"; bold = true;  break;
			case mathml_font_fraktur:         suffix = L"fr;";  bold = false; break;
			case mathml_font_bold_fraktur:    suffix = L"fr;";  bold = true;  break;
		}
		if (suffix != L"" && text.size() == 1 && is_plain_alpha(text[0])) {
			mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mi);
			new_node->children.push_back(new mathml_string_node(wstring(L"&") + text + suffix));
			new_node->attributes[L"mathvariant"] =
				mathml_font_strings[bold ? mathml_font_bold : mathml_font_normal];

			return new_node;
		}
	}
	
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mi);
	new_node->children.push_back(new mathml_string_node(text));

	// Need to override MathML default fonts depending on length of identifier string
	int count = count_characters_and_entities(text);
	if ((count > 1 && font != mathml_font_normal) || (count == 1 && font != mathml_font_italic))
		new_node->attributes[L"mathvariant"] = mathml_font_strings[font];
	return new_node;
}

// This function converts a layout_numeral node to MathML.
mathml_node* layout_numeral::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mn);
	new_node->children.push_back(new mathml_string_node(text));
	if (font != mathml_font_normal)
		new_node->attributes[L"mathvariant"] = mathml_font_strings[font];
	return new_node;
}

/* This is a list of MathML operators which might stretch themselves by default if we don't
override them. */
dictionary_item<int> stretchy_data[] = {
	{L"&sum;"},
	{L"&int;"},
	{L"&prod;"},
	{L"/"},
	{L"&macr;"},
	{L"("},
	{L")"},
	{L"{"},
	{L"}"},
	{L"["},
	{L"]"},
	{L"&lang;"},
	{L"&rang;"},
	{L"&lfloor;"},
	{L"&rfloor;"},
	{L"&vert;"},
	{L"|"},
	{L"&uarr;"},
	{L"&darr;"},
	{L"&varr;"},
};

hash_table<int> stretchy_table(
	stretchy_data, stretchy_data + sizeof(stretchy_data)/sizeof(stretchy_data[0]));

/* This function converts a layout_simple_operator node to MathML.
It handles the "tweak_bold_prime" bug. */
mathml_node* layout_simple_operator::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mo);
	new_node->children.push_back(new mathml_string_node(text));

	if (font != mathml_font_normal) {
		
		if (tweak_bold_prime && text == L"&prime;" && font != mathml_font_normal) {
			// don't bold the prime!
		}
		else
			new_node->attributes[L"mathvariant"] = mathml_font_strings[font];
	}

	if (stretchy) {
		new_node->attributes[L"stretchy"] = L"true";
		if (size != L"") {
			new_node->attributes[L"minsize"] = size;
			new_node->attributes[L"maxsize"] = size;
		}
	}
	else {
		if (stretchy_table.lookup(text) != NULL)
			new_node->attributes[L"stretchy"] = L"false";
	}
	
	/* Here we do some tweaks to certain operators which should eventually be stored in a
	table somewhere, but for now we just do them manually: */
	if (text == L"&forall;" || text == L"&exist;" || text == L"&nexist;")
		new_node->attributes[L"rspace"] = L"0";
	else if (text == L"&vee;" || text == L"&wedge;")
		new_node->attributes[L"lspace"] = L"0.1em";
	else if (text == L",")
		new_node->attributes[L"rspace"] = L"0.1em";
	else if (text == L"&bigcup;" || text == L"&bigcap;") {
		new_node->attributes[L"stretchy"] = L"true";
		new_node->attributes[L"minsize"] = L"1.5em";
		new_node->attributes[L"maxsize"] = L"1.5em";
	}
	
	return new_node;
}

// This function converts a layout_compound_operator node to MathML.
mathml_node* layout_compound_operator::build_mathml_tree() const {
	return child->build_mathml_tree();
}

// This function converts a layout_space node to MathML.
mathml_node* layout_space::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mspace);
	new_node->attributes[L"width"] = width;
	return new_node;
}

// This function converts a layout_scripts node to MathML.
mathml_node* layout_scripts::build_mathml_tree() const {
	mathml_tag_node* new_node;

	if (upper && !lower) {
		new_node = new mathml_tag_node((accent || underover)
			? mathml_tag_node::mover : mathml_tag_node::msup);
		new_node->children.push_back(base->build_mathml_tree());
		new_node->children.push_back(upper->build_mathml_tree());
	}
	else if (!upper && lower) {
		new_node = new mathml_tag_node((accent || underover)
			? mathml_tag_node::munder : mathml_tag_node::msub);
		new_node->children.push_back(base->build_mathml_tree());
		new_node->children.push_back(lower->build_mathml_tree());
	}
	else if (upper && lower) {
		new_node = new mathml_tag_node((accent || underover)
			? mathml_tag_node::munderover : mathml_tag_node::msubsup);
		new_node->children.push_back(base->build_mathml_tree());
		new_node->children.push_back(lower->build_mathml_tree());
		new_node->children.push_back(upper->build_mathml_tree());
	}
	else throw internal_error(L"layout_scripts node without either script present.");
	
	if (accent) {
		if (upper && lower)
			throw internal_error(L"layout_scripts node with both under and over accents.");
		else if (upper) new_node->attributes[L"accent"]      = L"true";
		else if (lower) new_node->attributes[L"accentunder"] = L"true";
	}
	
	return new_node;
}

// This function converts a layout_fraction node to MathML.
mathml_node* layout_fraction::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mfrac);
	new_node->children.push_back(numerator->build_mathml_tree());
	new_node->children.push_back(denominator->build_mathml_tree());
	if (!visible_line) new_node->attributes[L"linethickness"] = L"0";
	return new_node;
}

// This function converts a layout_sqrt node to MathML.
mathml_node* layout_sqrt::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::msqrt);
	new_node->children.push_back(inside->build_mathml_tree());
	return new_node;
}

// This function converts a layout_root node to MathML.
mathml_node* layout_root::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mroot);
	new_node->children.push_back(inside->build_mathml_tree());
	new_node->children.push_back(outside->build_mathml_tree());
	return new_node;
}

// This function converts a layout_table_row node to MathML.
mathml_node* layout_table_row::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mtr);
	for (list<layout_node*>::const_iterator ptr = entries.begin(); ptr != entries.end(); ptr++) {
		mathml_tag_node* mtd_node = new mathml_tag_node(mathml_tag_node::mtd);
		mtd_node->children.push_back((*ptr)->build_mathml_tree());
		new_node->children.push_back(mtd_node);
	}
	return new_node;
}

// This function converts a layout_table node to MathML.
mathml_node* layout_table::build_mathml_tree() const {
	mathml_tag_node* new_node = new mathml_tag_node(mathml_tag_node::mtable);
	if (align != L"") new_node->attributes[L"columnalign"] = align;
	
	for (list<layout_table_row*>::const_iterator ptr = rows.begin(); ptr != rows.end(); ptr++)
		new_node->children.push_back((*ptr)->build_mathml_tree());
	return new_node;
}

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Stuff to write the MathML tree to a stream. */

void write_indent(wostream& os, int depth) {
	if (mode_indented) {
		os << L'\n';
		for (int i = 0; i < depth; i++) os << L"  ";
	}
}

/* This function writes a MathML string literal to an output stream.
It handles the bug where &nbsp; has negative width. */
void mathml_string_node::write_output(wostream& os, int depth) const {
	if (!tweak_mtext_nbsp) {
		os << text;
		return;
	}
	
	// replace all "&nbsp;" by "<mstyle>&nbsp;</mstyle>"
	wstring tweak_text;
	int source = 0;
	while (true) {
		int found = text.find(L"&nbsp;", source);
		if (found == wstring::npos) {
			tweak_text.append(text, source, text.size() - source);
			break;
		}
		else {
			tweak_text.append(text, source, found - source);
			tweak_text += L"<mstyle>&nbsp;</mstyle>";
			source = found + 6;
		}
	}
	
	os << tweak_text;
}

wstring mathml_tag_strings[] = {
	L"mrow",
	L"mi",
	L"mo",
	L"mn",
	L"mfrac",
	L"msup",
	L"msub",
	L"msubsup",
	L"munder",
	L"mover",
	L"munderover",
	L"msqrt",
	L"mroot",
	L"mtext",
	L"mtable",
	L"mtr",
	L"mtd",
	L"mstyle",
	L"mspace"
};

/* This function writes a MathML node (other than a string literal) to an output stream,
and handles indenting if requested. */
void mathml_tag_node::write_output(wostream& os, int depth) const {
	wstring tag = mathml_tag_strings[type];
	
	os << L"<" << tag;
	for (map<wstring, wstring>::const_iterator ptr = attributes.begin();
		ptr != attributes.end(); ptr++)
			os << L" " << ptr->first << L"=\"" << ptr->second << L"\"";
	
	if (children.empty()) {
		os << L"/>";
		return;
	}
	
	os << L">";
	
	bool just_wrote_string = false;
	for (list<mathml_node*>::const_iterator ptr = children.begin(); ptr != children.end(); ptr++) {
		bool is_string = (dynamic_cast<mathml_string_node*>(*ptr) != NULL);
		if (!is_string && !just_wrote_string) write_indent(os, depth+1);
		(*ptr)->write_output(os, depth+1);
		just_wrote_string = is_string;
	}
	
	if (!just_wrote_string)
		write_indent(os, depth);
	
	os << L"</" << tag << L">";
}

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
end of file */
