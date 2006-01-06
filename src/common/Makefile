# @(#$Id: Makefile,v 1.8 2006-01-06 13:18:17 dockes Exp $  (C) 2005 J.F.Dockes

include ../mk/sysconf

# Only test executables get build in here
PROGS = internfile unacpp textsplit

all: $(BIGLIB) $(PROGS) 

$(BIGLIB):
	cd ../lib;$(MAKE)

UNACPP_OBJS= trunacpp.o $(BIGLIB)
unacpp : $(UNACPP_OBJS)
	$(CXX) $(CXXFLAGS) -o unacpp $(UNACPP_OBJS) \
	       $(LIBICONV)
trunacpp.o : unacpp.cpp unacpp.h
	$(CXX) $(CXXFLAGS) -DTEST_UNACPP -c -o trunacpp.o unacpp.cpp

TEXTSPLIT_OBJS= trtextsplit.o  $(BIGLIB)
textsplit : $(TEXTSPLIT_OBJS)
	$(CXX) $(CXXFLAGS) -o textsplit $(TEXTSPLIT_OBJS)
trtextsplit.o : textsplit.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_TEXTSPLIT -c -o trtextsplit.o \
	       textsplit.cpp

INTERNFILE_OBJS= trinternfile.o  $(BIGLIB) $(MIMELIB)
internfile : $(INTERNFILE_OBJS) 
	$(CXX) $(CXXFLAGS) -o internfile $(INTERNFILE_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trinternfile.o : internfile.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_INTERNFILE -c -o trinternfile.o \
	       internfile.cpp

clean:
	rm -f *.o $(PROGS)

