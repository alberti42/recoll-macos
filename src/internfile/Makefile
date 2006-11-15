# @(#$Id: Makefile,v 1.1 2006-11-15 07:27:42 dockes Exp $  (C) 2005 J.F.Dockes
depth = ..
include $(depth)/mk/sysconf

# Only test executables get build in here
PROGS = internfile unacpp textsplit rclconfig

all: $(BIGLIB) $(PROGS) 

$(BIGLIB): force
	cd $(depth)/lib;$(MAKE)
force:

INTERNFILE_OBJS= trinternfile.o  $(BIGLIB) $(MIMELIB)
internfile : $(INTERNFILE_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o internfile $(INTERNFILE_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trinternfile.o : internfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_INTERNFILE -c -o trinternfile.o \
	       internfile.cpp
clean:
	rm -f *.o $(PROGS)

