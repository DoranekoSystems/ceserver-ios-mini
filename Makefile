LIBS      =
INCLUDE   = -I../
TARGET    = ./ceserver
SRCDIR    = ./
SOURCES   = ceserver.cpp api.cpp

all: $(SOURCES)
	$(CXX) $(CXXFLAGS) -std=c++11 -lz -o $(TARGET) $(SOURCES)