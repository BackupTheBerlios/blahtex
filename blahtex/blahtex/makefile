#
# makefile
#
# blahtex (version 0.2.1): a LaTeX to MathML converter designed specifically for MediaWiki
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

SOURCES = parser.tab.c  layout.cpp  mathml.cpp  main.cpp

linux: parser.tab.h $(SOURCES)
	g++ -O3 -o blahtex $(SOURCES)
	
mac: parser.tab.h $(SOURCES)
#	g++ -O3 -o blahtex -liconv -DBLAHTEX_ICONV_CONST $(SOURCES)
	g++ -g -o blahtex -liconv -DBLAHTEX_ICONV_CONST $(SOURCES)

parser.tab.c parser.tab.h:   parser.ypp blahtex.h
	yacc -d -b parser parser.ypp

clean:
	rm -f parser.tab.c
	rm -f parser.tab.h
	rm -f parser.output
	rm -f blahtex
