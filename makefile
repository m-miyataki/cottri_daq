TARGET = cottriFEdecoder 

SRCS = cottriFEdecoder.cpp
OBJS = $(SRCS:.cpp=.o)  

ROOTCFLAGS = $(shell root-config --cflags)
ROOTLIBS   = $(shell root-config --libs)
#ROOTGLIBS = $(shell root-config --glibs)

CXXFLAGS   = $(ROOTCFLAGS) -Wall -fPIC
CXXLIBS    = $(ROOTLIBS)
CC = g++ 

$(TARGET): $(OBJS)
	        $(CC) $(CXXLIBS) $(OBJS) -o $@

# suffix rule
# .cc.o:
#         $(CC) $(CXXFLAGS) -c $<
#
#         # clean
clean:
	        rm -f $(TARGET) $(OBJS)
