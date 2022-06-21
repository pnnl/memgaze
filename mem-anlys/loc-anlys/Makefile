#****************************************************************************
# Package defs
#****************************************************************************

include ../../Makefile-defs.mk

#----------------------------------------------------------------------------
# Build
#----------------------------------------------------------------------------


CC=g++
CFLAGS=-Wall -g 

SRC=./src
#LDIR =lib

#LIBS=-lm

_DEPS=memoryanalysis.h memorymodeling.h structure.h TraceLine.hpp BlockInfo.hpp SpatialRUD.hpp
DEPS=$(patsubst %,$(SRC)/%,$(_DEPS))

_OBJ=main.o BlockInfo.o SpatialRUD.o
OBJ=$(patsubst %,$(SRC)/%,$(_OBJ))

#APP=memCAM
APP=memgaze-analyze-loc
MK_PROGRAMS_CXX = $(APP)

%.o: %.cc $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(APP): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 
	rm $(SRC)/*.o

#.PHONY: clean distclean 
#
#clean:
#	rm -f $(SRC)/*.o
#
#distclean: 
#	rm -f $(APP)

#****************************************************************************
# Template Rules
#****************************************************************************

include ../../Makefile-template.mk


#****************************************************************************
# Local Rules
#****************************************************************************

info.local :

install.local :
	$(INSTALL) -d $(PREFIX_LIBEXEC)
	$(INSTALL) memgaze-analyze-loc $(PREFIX_LIBEXEC)      

check.local :
