
PROGS = unacpp textsplit rclconfig

all: $(PROGS) 

UNACPP_OBJS= trunacpp.o
unacpp : $(UNACPP_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o unacpp $(UNACPP_OBJS) $(LIBRECOLL)
trunacpp.o : unacpp.cpp unacpp.h
	$(CXX) $(ALL_CXXFLAGS) -DTEST_UNACPP -c -o trunacpp.o unacpp.cpp

TEXTSPLIT_OBJS= trtextsplit.o  
textsplit : $(TEXTSPLIT_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o textsplit $(TEXTSPLIT_OBJS) $(LIBRECOLL)
trtextsplit.o : textsplit.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_TEXTSPLIT -c -o trtextsplit.o \
	       textsplit.cpp

RCLCONFIG_OBJS= trrclconfig.o 
rclconfig : $(RCLCONFIG_OBJS) 
	$(CXX) $(ALL_CXXFLAGS) -o rclconfig $(RCLCONFIG_OBJS) $(LIBRECOLL) 
trrclconfig.o : rclconfig.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_RCLCONFIG -c -o trrclconfig.o \
	       rclconfig.cpp

include ../utils/utmkdefs.mk
