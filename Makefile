CXX 			:= clang++
CXXFLAGS  		?= -Wall -g -O3 -std=c++20
LFLAGS 			?=

SOURCEDIR := src
SOURCES := $(wildcard $(SOURCEDIR)/*.cpp)
BUILDDIR := build
OBJECTS := $(subst /src/,/,$(addprefix $(BUILDDIR)/,$(SOURCES:%.cpp=%.o)))
BINARY := $(BUILDDIR)/sudoku.exe

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) -o $(BINARY)

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.cpp
	mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I$(dir $<) -c $< -o $@

.PHONY: clean
clean:
	rm -rf ${BUILDDIR}
