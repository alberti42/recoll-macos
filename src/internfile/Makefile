# @(#$Id: Makefile,v 1.4 2008-11-24 15:47:40 dockes Exp $  (C) 2005 J.F.Dockes
depth = ..
include $(depth)/mk/sysconf

# Only test executables get build in here
PROGS = mh_mbox internfile

all: $(BIGLIB) $(PROGS) 

$(BIGLIB): force
	cd $(depth)/lib;$(MAKE)
force:

INTERNFILE_OBJS= trinternfile.o  $(BIGLIB) 
internfile : $(INTERNFILE_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o internfile $(INTERNFILE_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trinternfile.o : internfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_INTERNFILE -c -o trinternfile.o \
	       internfile.cpp

MH_MBOX_OBJS= trmh_mbox.o  $(BIGLIB) 
mh_mbox : $(MH_MBOX_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o mh_mbox $(MH_MBOX_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trmh_mbox.o : mh_mbox.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_MH_MBOX -c -o trmh_mbox.o \
	       mh_mbox.cpp


clean:
	rm -f *.o $(PROGS)

