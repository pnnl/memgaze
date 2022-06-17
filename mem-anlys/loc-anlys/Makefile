CC=g++
CFLAGS=-Wall -g 

SRC=./src
#LDIR =lib

#LIBS=-lm

_DEPS=memoryanalysis.h memorymodeling.h structure.h TraceLine.hpp BlockInfo.hpp SpatialRUD.hpp
DEPS=$(patsubst %,$(SRC)/%,$(_DEPS))

_OBJ=main.o BlockInfo.o SpatialRUD.o
OBJ=$(patsubst %,$(SRC)/%,$(_OBJ))

APP=memCAM

%.o: %.cc $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(APP): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 
	rm $(SRC)/*.o

.PHONY: clean distclean 

clean:
	rm -f $(SRC)/*.o

distclean: 
	rm -f $(APP)
