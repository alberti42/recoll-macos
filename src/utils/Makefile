
CXXFLAGS = -I.

BIGLIB = ../lib/librcl.a

PROGS = trfstreewalk trpathut execmd transcode
all: $(PROGS)

FSTREEWALK_OBJS= trfstreewalk.o fstreewalk.o pathut.o
trfstreewalk : $(FSTREEWALK_OBJS)
	$(CXX) -o trfstreewalk $(FSTREEWALK_OBJS)
trfstreewalk.o : fstreewalk.cpp fstreewalk.h
	$(CXX) -o trfstreewalk.o -c $(CXXFLAGS) \
	       -DTEST_FSTREEWALK fstreewalk.cpp

PATHUT_OBJS= trpathut.o pathut.o 
trpathut : $(PATHUT_OBJS)
	$(CXX) -o trpathut $(PATHUT_OBJS)
trpathut.o : pathut.cpp pathut.h
	$(CXX) -o trpathut.o -c $(CXXFLAGS) \
	       -DTEST_PATHUT pathut.cpp

execmd: pathut.o
	$(CXX) -o execmd $(CXXFLAGS) execmd.cpp pathut.o

TRANSCODE_OBJS= trtranscode.o  $(BIGLIB) 
transcode : $(TRANSCODE_OBJS)
	$(CXX) $(CXXFLAGS) -o transcode $(TRANSCODE_OBJS) \
	       -L/usr/local/lib -liconv
trtranscode.o : ../utils/transcode.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_TRANSCODE -c -o trtranscode.o \
	       transcode.cpp
clean: 
	rm -f *.o $(PROGS)
