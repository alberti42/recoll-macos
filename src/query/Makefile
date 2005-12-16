
PROGS = xadump #trhist qtry qxtry 
all: $(PROGS)

XADUMP_OBJS= xadump.o $(BIGLIB) 
xadump : $(XADUMP_OBJS)
	$(CXX) $(CXXFLAGS) -o xadump $(XADUMP_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN) $(LIBSYS)

QXTRY_OBJS= qxtry.o  $(BIGLIB)
qxtry : $(QXTRY_OBJS)
	$(CXX) $(CXXFLAGS) -o qxtry $(QXTRY_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN)

QTRY_OBJS= qtry.o  $(BIGLIB) $(MIMELIB)
qtry : $(QTRY_OBJS)
	$(CXX) $(CXXFLAGS) -o qtry $(QTRY_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN)

HISTORY_OBJS= trhist.o  $(BIGLIB) $(MIMELIB)
trhist : $(HISTORY_OBJS)
	$(CXX) $(CXXFLAGS) -o trhist $(HISTORY_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN)
trhist.o : history.cpp history.h
	$(CXX) $(CXXFLAGS) -DTEST_HISTORY -c -o trhist.o history.cpp

$(BIGLIB):
	cd ../lib;make
   
depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f *.o $(PROGS)

include ../mk/sysconf
include alldeps
