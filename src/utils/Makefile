depth = ..
include $(depth)/mk/sysconf

PROGS = trconftree wipedir smallut trfstreewalk trpathut \
      transcode \
      trmimeparse trexecmd utf8iter idfile

all: $(PROGS) $(BIGLIB)

$(BIGLIB): force
	cd $(depth)/lib;$(MAKE)
force:

FSTREEWALK_OBJS= trfstreewalk.o fstreewalk.o $(BIGLIB)
trfstreewalk : $(FSTREEWALK_OBJS)
	$(CXX) -o trfstreewalk $(FSTREEWALK_OBJS)
trfstreewalk.o : fstreewalk.cpp fstreewalk.h
	$(CXX) -o trfstreewalk.o -c $(ALL_CXXFLAGS) \
	       -DTEST_FSTREEWALK fstreewalk.cpp

PATHUT_OBJS= trpathut.o  $(BIGLIB)
trpathut : $(PATHUT_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o trpathut $(PATHUT_OBJS)
trpathut.o : pathut.cpp pathut.h
	$(CXX) -o trpathut.o -c $(ALL_CXXFLAGS) -DTEST_PATHUT pathut.cpp

EXECMD_OBJS= trexecmd.o $(BIGLIB)
trexecmd : $(EXECMD_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o trexecmd $(EXECMD_OBJS)
trexecmd.o : execmd.cpp execmd.h
	$(CXX) -o trexecmd.o -c $(ALL_CXXFLAGS) -DTEST_EXECMD execmd.cpp

TRANSCODE_OBJS= trtranscode.o  $(BIGLIB) 
transcode : $(TRANSCODE_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o transcode $(TRANSCODE_OBJS) $(LIBICONV)
trtranscode.o : transcode.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_TRANSCODE -c -o trtranscode.o \
	       transcode.cpp

IDFILE_OBJS= tridfile.o  $(BIGLIB) 
idfile : $(IDFILE_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o idfile $(IDFILE_OBJS) $(LIBICONV)
tridfile.o : idfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_IDFILE -c -o tridfile.o \
	       idfile.cpp

MIMEPARSE_OBJS= trmimeparse.o  $(BIGLIB) 
trmimeparse : $(MIMEPARSE_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o trmimeparse $(MIMEPARSE_OBJS) $(LIBICONV)
trmimeparse.o : mimeparse.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_MIMEPARSE -c -o trmimeparse.o \
	       mimeparse.cpp

SMALLUT_OBJS= trsmallut.o  $(BIGLIB) 
smallut : $(SMALLUT_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o smallut $(SMALLUT_OBJS) $(LIBICONV)
trsmallut.o : smallut.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_SMALLUT -c -o trsmallut.o \
	       smallut.cpp

WIPEDIR_OBJS= trwipedir.o  $(BIGLIB) 
wipedir : $(WIPEDIR_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o wipedir $(WIPEDIR_OBJS) $(LIBICONV)
trwipedir.o : wipedir.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_WIPEDIR -c -o trwipedir.o \
	       wipedir.cpp

UTF8ITER_OBJS= trutf8iter.o 
utf8iter : $(UTF8ITER_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o utf8iter $(UTF8ITER_OBJS) $(BIGLIB) $(LIBICONV) 
trutf8iter.o : utf8iter.cpp utf8iter.h
	$(CXX) $(ALL_CXXFLAGS) -DTEST_UTF8ITER -c -o trutf8iter.o \
	       utf8iter.cpp


CONFTREE_OBJS= trconftree.o  ../lib/pathut.o ../lib/smallut.o ../lib/conftree.o
trconftree : $(CONFTREE_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o trconftree $(CONFTREE_OBJS)
trconftree.o : conftree.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_CONFTREE -c -o trconftree.o \
	       conftree.cpp

clean:
	rm -f *.o $(PROGS)

