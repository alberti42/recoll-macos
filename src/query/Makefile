include ../mk/FreeBSD

BIGLIB=../lib/librcl.a

PROGS = qtry qxtry xadump
all: $(PROGS)

XADUMP_OBJS= xadump.o $(BIGLIB)
xadump : $(XADUMP_OBJS)
	$(CXX) $(CXXFLAGS) -o xadump $(XADUMP_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

QXTRY_OBJS= qxtry.o  $(BIGLIB)
qxtry : $(QXTRY_OBJS)
	$(CXX) $(CXXFLAGS) -o qxtry $(QXTRY_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

QTRY_OBJS= qtry.o  $(BIGLIB)
qtry : $(QTRY_OBJS)
	$(CXX) $(CXXFLAGS) -o qtry $(QTRY_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

$(BIGLIB):
	cd ../lib;make
   
clean: 
	rm -f *.o $(PROGS)
alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M *.cpp > alldeps
include alldeps

