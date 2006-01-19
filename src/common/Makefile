# @(#$Id: Makefile,v 1.9 2006-01-19 12:01:42 dockes Exp $  (C) 2005 J.F.Dockes
depth = ..
include $(depth)/mk/sysconf

# Only test executables get build in here
PROGS = internfile unacpp textsplit

all: $(BIGLIB) $(PROGS) 

$(BIGLIB):
	cd $(depth)/lib;$(MAKE)

UNACPP_OBJS= trunacpp.o $(BIGLIB)
unacpp : $(UNACPP_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o unacpp $(UNACPP_OBJS) \
	       $(LIBICONV)
trunacpp.o : unacpp.cpp unacpp.h
	$(CXX) $(ALL_CXXFLAGS) -DTEST_UNACPP -c -o trunacpp.o unacpp.cpp

TEXTSPLIT_OBJS= trtextsplit.o  $(BIGLIB)
textsplit : $(TEXTSPLIT_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o textsplit $(TEXTSPLIT_OBJS)
trtextsplit.o : textsplit.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_TEXTSPLIT -c -o trtextsplit.o \
	       textsplit.cpp

INTERNFILE_OBJS= trinternfile.o  $(BIGLIB) $(MIMELIB)
internfile : $(INTERNFILE_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o internfile $(INTERNFILE_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trinternfile.o : internfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_INTERNFILE -c -o trinternfile.o \
	       internfile.cpp

clean:
	rm -f *.o $(PROGS)

