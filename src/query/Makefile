
CXXFLAGS = -Wall -g -I. -I../index -I../utils -I../common -I/usr/local/include


PROGS = qtry qxtry xadump
all: $(PROGS)

XADUMP_OBJS= xadump.o transcode.o
xadump : $(XADUMP_OBJS)
	$(CXX) $(CXXFLAGS) -o xadump $(XADUMP_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

QXTRY_OBJS= qxtry.o transcode.o
qxtry : $(QXTRY_OBJS)
	$(CXX) $(CXXFLAGS) -o qxtry $(QXTRY_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

QTRY_OBJS= qtry.o conftree.o rclconfig.o \
		rcldb.o transcode.o  \
		textsplit.o unac.o unacpp.o pathut.o

qtry : $(QTRY_OBJS)
	$(CXX) $(CXXFLAGS) -o qtry $(QTRY_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

transcode.o : ../index/transcode.cpp ../index/transcode.h
	$(CXX) $(CXXFLAGS) -c -o transcode.o ../index/transcode.cpp

conftree.o : ../utils/conftree.cpp ../utils/conftree.h
	$(CXX) $(CXXFLAGS) -c -o conftree.o ../utils/conftree.cpp
rclconfig.o : ../common/rclconfig.cpp ../common/rclconfig.h
	$(CXX) $(CXXFLAGS) -c -o rclconfig.o ../common/rclconfig.cpp
unacpp.o : ../common/unacpp.cpp ../common/unacpp.h
	$(CXX) $(CXXFLAGS) -I../unac -c -o unacpp.o ../common/unacpp.cpp
pathut.o : ../utils/pathut.cpp ../utils/pathut.h
	$(CXX) $(CXXFLAGS) -c -o pathut.o ../utils/pathut.cpp
rcldb.o : ../common/rcldb.cpp ../common/rcldb.h
	$(CXX) $(CXXFLAGS) -c -o rcldb.o ../common/rcldb.cpp
textsplit.o : ../common/textsplit.cpp ../common/textsplit.h
	$(CXX) $(CXXFLAGS) -c -o textsplit.o ../common/textsplit.cpp


CFLAGS = -g -I. -I../unac -I/usr/local/include -DUNAC_VERSION=\"1.0.7\"
unac.o : ../unac/unac.c ../unac/unac.h
	$(CC) $(CFLAGS) -c -o unac.o ../unac/unac.c

clean: 
	rm -f *.o $(PROGS)
alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M *.cpp > alldeps
include alldeps

