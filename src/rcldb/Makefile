# @(#$Id: Makefile,v 1.1 2007-06-02 08:30:42 dockes Exp $  (C) 2005 J.F.Dockes
depth = ..
include $(depth)/mk/sysconf

# Only test executables get build in here
PROGS = stoplist

all: $(BIGLIB) $(PROGS) 

$(BIGLIB): force
	cd $(depth)/lib;$(MAKE)
force:

STOPLIST_OBJS= trstoplist.o  $(BIGLIB)
stoplist : $(STOPLIST_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o stoplist $(STOPLIST_OBJS) \
	   $(LIBICONV) $(LIBSYS)
trstoplist.o : stoplist.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_STOPLIST -c -o trstoplist.o \
	       stoplist.cpp

clean:
	rm -f *.o $(PROGS)

