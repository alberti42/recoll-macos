include ../mk/FreeBSD

BIGLIB = ../lib/librcl.a

PROGS = smallut trfstreewalk trpathut execmd transcode trmimeparse
all: $(PROGS)

FSTREEWALK_OBJS= trfstreewalk.o fstreewalk.o pathut.o
trfstreewalk : $(FSTREEWALK_OBJS)
	$(CXX) -o trfstreewalk $(FSTREEWALK_OBJS)
trfstreewalk.o : fstreewalk.cpp fstreewalk.h
	$(CXX) -o trfstreewalk.o -c $(CXXFLAGS) \
	       -DTEST_FSTREEWALK fstreewalk.cpp

PATHUT_OBJS= trpathut.o pathut.o 
trpathut : $(PATHUT_OBJS)
	$(CXX) $(CXXFLAGS) -o trpathut $(PATHUT_OBJS)
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

MIMEPARSE_OBJS= trmimeparse.o  $(BIGLIB) 
mimeparse : $(MIMEPARSE_OBJS)
	$(CXX) $(CXXFLAGS) -o mimeparse $(MIMEPARSE_OBJS) \
	       -L/usr/local/lib -liconv
trmimeparse.o : ../utils/mimeparse.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_MIMEPARSE -c -o trmimeparse.o \
	       mimeparse.cpp
SMALLUT_OBJS= trsmallut.o  $(BIGLIB) 
smallut : $(SMALLUT_OBJS)
	$(CXX) $(CXXFLAGS) -o smallut $(SMALLUT_OBJS) \
	       -L/usr/local/lib -liconv
trsmallut.o : ../utils/smallut.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_SMALLUT -c -o trsmallut.o \
	       smallut.cpp
clean: 
	rm -f *.o $(PROGS)
