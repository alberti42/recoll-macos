include ../mk/sysconf

BIGLIB = ../lib/librcl.a

PROGS = wipedir smallut trfstreewalk trpathut transcode trmimeparse \
      trexecmd utf8iter

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
	$(CXX) $(CXXFLAGS) -o transcode $(TRANSCODE_OBJS) $(LIBICONV)
trtranscode.o : ../utils/transcode.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_TRANSCODE -c -o trtranscode.o \
	       transcode.cpp

IDFILE_OBJS= tridfile.o  $(BIGLIB) 
idfile : $(IDFILE_OBJS)
	$(CXX) $(CXXFLAGS) -o idfile $(IDFILE_OBJS) $(LIBICONV)
tridfile.o : ../utils/idfile.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_IDFILE -c -o tridfile.o \
	       idfile.cpp

MIMEPARSE_OBJS= trmimeparse.o  $(BIGLIB) 
trmimeparse : $(MIMEPARSE_OBJS)
	$(CXX) $(CXXFLAGS) -o trmimeparse $(MIMEPARSE_OBJS) $(LIBICONV)
trmimeparse.o : mimeparse.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_MIMEPARSE -c -o trmimeparse.o \
	       mimeparse.cpp

SMALLUT_OBJS= trsmallut.o  $(BIGLIB) 
smallut : $(SMALLUT_OBJS)
	$(CXX) $(CXXFLAGS) -o smallut $(SMALLUT_OBJS) $(LIBICONV)
trsmallut.o : ../utils/smallut.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_SMALLUT -c -o trsmallut.o \
	       smallut.cpp

WIPEDIR_OBJS= trwipedir.o  $(BIGLIB) 
wipedir : $(WIPEDIR_OBJS)
	$(CXX) $(CXXFLAGS) -o wipedir $(WIPEDIR_OBJS) $(LIBICONV)
trwipedir.o : ../utils/wipedir.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_WIPEDIR -c -o trwipedir.o \
	       wipedir.cpp

UTF8ITER_OBJS= trutf8iter.o  $(BIGLIB)
utf8iter : $(UTF8ITER_OBJS)
	$(CXX) $(CXXFLAGS) -o utf8iter $(UTF8ITER_OBJS) $(LIBICONV)
trutf8iter.o : ../utils/utf8iter.cpp utf8iter.h
	$(CXX) $(CXXFLAGS) -DTEST_UTF8ITER -c -o trutf8iter.o \
	       utf8iter.cpp
clean: 
	rm -f *.o $(PROGS)
