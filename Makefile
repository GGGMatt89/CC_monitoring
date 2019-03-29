#---------------------------------------
# Resources section - Symbol definitions
#---------------------------------------
ROOTFLAGS = $(shell root-config --cflags)
ROOTLIBS  = $(shell root-config --libs)
#THREAD = -lpthread

CXX := g++
#CXX := clang++
CXXFLAGS := -std=c++11 #-stdlib=libstdc++
#CXXFLAGS := -std=c++11 -stdlib=libc++


C++ = clang -O
LIB = monitor.a
AR  = ar -r

C++ = clang -O
LIB = monitor_both.a
AR  = ar -r

C++ = clang -O
LIB = create_data.a
AR  = ar -r

#----------------------
# Makefile head section
#----------------------

all:
	make create_data monitor monitor_both

create_data: create_data.o $(LIB)
	$(CXX) $(CXXFLAGS) -o create_data create_data.o $(ROOTFLAGS) $(ROOTLIBS)

monitor: monitor.o $(LIB)
	$(CXX) $(CXXFLAGS) -o monitor monitor.o $(ROOTFLAGS) $(ROOTLIBS)
monitor_both: monitor_both.o $(LIB)
	$(CXX) $(CXXFLAGS) -o monitor_both monitor_both.o $(ROOTFLAGS) $(ROOTLIBS)


clean:
	rm -f *.o *.a

cleanall:
	rm -f *.o *.a create_data monitor monitor_both


#----------------------------------
# Dependencies for the source files
#----------------------------------

CREATE_DATA_CPP_DEP = \
	functions.h \

MONITOR_CPP_DEP = \
	functions.h \
	
MONITOR_BOTH_CPP_DEP = \
	functions.h \

#--------------------------------
# Compilation of the source files
#--------------------------------

create_data.o : create_data.cxx $(CREATE_DATA_CPP_DEP)
	$(CXX) $(CXXFLAGS) -c create_data.cxx -o create_data.o $(ROOTFLAGS)

monitor.o : monitor.cxx $(MONITOR_CPP_DEP)
	$(CXX) $(CXXFLAGS) -c monitor.cxx -o monitor.o $(ROOTFLAGS)

monitor_both.o : monitor_both.cxx $(MONITOR_BOTH_CPP_DEP)
	$(CXX) $(CXXFLAGS) -c monitor_both.cxx -o monitor_both.o $(ROOTFLAGS)

#----------------------------------
# Library dependencies and creation
#----------------------------------

LIB_DEP = \
	create_data.o \
	monitor.o\
	monitor_both.o\

$(LIB) : $(LIB_DEP)
	$(AR) $(LIB) $(LIB_DEP) $(POST_AR)
