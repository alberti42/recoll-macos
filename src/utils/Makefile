include ../mk/FreeBSD

BIGLIB = ../lib/librcl.a

PROGS = smallut trfstreewalk trpathut transcode trmimeparse trexecmd
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
	$(CXX) -o trpathut.o -c $(CXXFLAGS) -DTEST_PATHUT pathut.cpp

EXECMD_OBJS= trexecmd.o $(BIGLIB)
trexecmd : $(EXECMD_OBJS)
	$(CXX) $(CXXFLAGS) -o trexecmd $(EXECMD_OBJS)
trexecmd.o : execmd.cpp execmd.h
	$(CXX) -o trexecmd.o -c $(CXXFLAGS) -DTEST_EXECMD execmd.cpp

TRANSCODE_OBJS= trtranscode.o  $(BIGLIB) 
transcode : $(TRANSCODE_OBJS)
	$(CXX) $(CXXFLAGS) -o transcode $(TRANSCODE_OBJS) \
	       -L/usr/local/lib -liconv
trtranscode.o : ../utils/transcode.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_TRANSCODE -c -o trtranscode.o \
	       transcode.cpp

MIMEPARSE_OBJS= trmimeparse.o  $(BIGLIB) 
trmimeparse : $(MIMEPARSE_OBJS)
	$(CXX) $(CXXFLAGS) -o mimeparse $(MIMEPARSE_OBJS) \
	       -L/usr/local/lib -liconv
trmimeparse.o : mimeparse.cpp 
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
