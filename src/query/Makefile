
CXXFLAGS = -Wall -g -I. -I../index -I../utils -I../common -I/usr/local/include


PROGS = xadump
all: $(PROGS)

XADUMP_OBJS= xadump.o transcode.o
xadump : $(XADUMP_OBJS)
	$(CXX) $(CXXFLAGS) -o xadump $(XADUMP_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

transcode.o : ../index/transcode.cpp ../index/transcode.h
	$(CXX) $(CXXFLAGS) -c -o transcode.o ../index/transcode.cpp

clean: 
	rm -f *.o $(PROGS)
