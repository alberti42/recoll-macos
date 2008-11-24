# @(#$Id: Makefile,v 1.15 2008-11-24 15:47:40 dockes Exp $  (C) 2005 J.F.Dockes
depth = ..
include $(depth)/mk/sysconf

# Only test executables get build in here
PROGS = unacpp textsplit rclconfig

all: $(BIGLIB) $(PROGS) 

$(BIGLIB): force
	cd $(depth)/lib;$(MAKE)
force:

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

RCLCONFIG_OBJS= trrclconfig.o  $(BIGLIB) 
rclconfig : $(RCLCONFIG_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o rclconfig $(RCLCONFIG_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trrclconfig.o : rclconfig.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_RCLCONFIG -c -o trrclconfig.o \
	       rclconfig.cpp

clean:
	rm -f *.o $(PROGS)

