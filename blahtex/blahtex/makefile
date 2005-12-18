#
# makefile
#
# blahtex (version 0.3.4): a LaTeX to MathML converter designed with MediaWiki in mind
# Copyright (C) 2005, David Harvey
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

SOURCES = \
	Exception.cpp \
	Parser.cpp \
	MacroProcessor.cpp \
	ParseTree.cpp \
	LayoutTree.cpp \
	XmlNode.cpp \
	Instance.cpp \
	md5.c \
	md5Wrapper.cpp \
	UnicodeConverter.cpp \
	main.cpp

OBJECTS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

CFLAGS = -O3
CXXFLAGS = $(CFLAGS)

linux:  $(OBJECTS)  blahtex.h  md5Wrapper.h  md5.h  UnicodeConverter.h
	$(CXX) $(CFLAGS) -o blahtex $(OBJECTS)

mac: $(SOURCES)
	g++ -O3 -o blahtex -liconv -DBLAHTEX_ICONV_CONST $(SOURCES)

clean:
	rm -f blahtex $(OBJECTS)
