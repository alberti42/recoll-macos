BIGLIB = ../lib/librcl.a
CXXFLAGS = -Wall -g -I. -I../unac -I../utils  -I/usr/local/include


PROGS = unacpp textsplit
all: $(PROGS)

UNACPP_OBJS= trunacpp.o $(BIGLIB)
unacpp : $(UNACPP_OBJS)
	$(CXX) $(CXXFLAGS) -o unacpp $(UNACPP_OBJS) \
	       -L/usr/local/lib -liconv
trunacpp.o : unacpp.cpp unacpp.h
	$(CXX) $(CXXFLAGS) -DTEST_UNACPP -c -o trunacpp.o unacpp.cpp

TEXTSPLIT_OBJS= trtextsplit.o  $(BIGLIB)
textsplit : $(TEXTSPLIT_OBJS)
	$(CXX) $(CXXFLAGS) -o textsplit $(TEXTSPLIT_OBJS)
trtextsplit.o : textsplit.cpp 
	$(CXX) $(CXXFLAGS) -DTEST_TEXTSPLIT -c -o trtextsplit.o \
	       textsplit.cpp
clean: 
	rm -f *.o $(PROGS)
