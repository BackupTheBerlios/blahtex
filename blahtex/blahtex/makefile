#
# makefile
#
# blahtex (version 0.4.2)
# a TeX to MathML converter designed with MediaWiki in mind
# Copyright (C) 2006, David Harvey
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


# If you are building with imagemagick support (to enable the
# "--compute-vertical-shift" option), you will need to set the
# following directories to point to where your ImageMagick header/
# library files are stored:

MAGICK_INCLUDE_DIR = /usr/include/
MAGICK_LIB_DIR = /usr/lib/

# e.g. if you are using fink (Mac OS X), then probably you want

# MAGICK_INCLUDE_DIR = /sw/include/
# MAGICK_LIB_DIR = /sw/lib/

# and if you are using DarwinPorts (Mac OS X), then probably you want

# MAGICK_INCLUDE_DIR = /opt/local/include/
# MAGICK_LIB_DIR = /opt/local/lib/

SOURCES = \
	source/main.cpp \
	source/mainPng.cpp \
	source/md5.c \
	source/md5Wrapper.cpp \
	source/Messages.cpp \
	source/UnicodeConverter.cpp \
	source/BlahtexCore/Interface.cpp \
	source/BlahtexCore/LayoutTree.cpp \
	source/BlahtexCore/MacroProcessor.cpp \
	source/BlahtexCore/Manager.cpp \
	source/BlahtexCore/Parser.cpp \
	source/BlahtexCore/ParseTree1.cpp \
	source/BlahtexCore/ParseTree2.cpp \
	source/BlahtexCore/ParseTree3.cpp \
	source/BlahtexCore/XmlNode.cpp
	
HEADERS = \
	source/md5.h \
	source/md5Wrapper.h \
	source/UnicodeConverter.h \
	source/BlahtexCore/Interface.h \
	source/BlahtexCore/LayoutTree.h \
	source/BlahtexCore/MacroProcessor.h \
	source/BlahtexCore/Manager.h \
	source/BlahtexCore/Misc.h \
	source/BlahtexCore/Parser.h \
	source/BlahtexCore/ParseTree.h \
	source/BlahtexCore/XmlNode.h

OBJECTS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

linux : CFLAGS = -O3 -DBLAHTEX_USING_MAGICK -I$(MAGICK_INCLUDE_DIR)
mac : CFLAGS = -O3 -DBLAHTEX_ICONV_CONST -DBLAHTEX_USING_MAGICK -I$(MAGICK_INCLUDE_DIR)
linux-without-imagemagick : CFLAGS = -O3
mac-without-imagemagick : CFLAGS = -O3 -DBLAHTEX_ICONV_CONST

CXXFLAGS = $(CFLAGS)

linux:  $(OBJECTS)  $(HEADERS)
	$(CXX) $(CFLAGS) -o blahtex -lMagick++ -L$(MAGICK_LIB_DIR) $(OBJECTS)

mac: $(OBJECTS)  $(HEADERS)
	$(CXX) $(CFLAGS) -o blahtex -liconv -lMagick++ -L$(MAGICK_LIB_DIR) $(OBJECTS)

linux-without-imagemagick:  $(OBJECTS)  $(HEADERS)
	$(CXX) $(CFLAGS) -o blahtex $(OBJECTS)

mac-without-imagemagick: $(OBJECTS)  $(HEADERS)
	$(CXX) $(CFLAGS) -o blahtex -liconv $(OBJECTS)

clean:
	rm -f blahtex $(OBJECTS)

########## end of file ##########
