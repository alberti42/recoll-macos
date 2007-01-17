depth = ..
include $(depth)/mk/sysconf

PROGS = xadump rclqlang #trhist qtry qxtry 

all: $(PROGS)

SRCS = xadump.cpp rclqlang.cpp
.cpp.o : 
	$(CXX) -c $(ALL_CXXFLAGS) -o $@ $<

XADUMP_OBJS= xadump.o $(BIGLIB)
xadump : $(XADUMP_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o xadump $(XADUMP_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN) $(LIBSYS)

RCLQLANG_OBJS= rclqlang.o $(BIGLIB)
rclqlang : $(RCLQLANG_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o rclqlang $(RCLQLANG_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN) $(LIBSYS)

HISTORY_OBJS= trhist.o  $(BIGLIB) $(MIMELIB)
trhist : $(HISTORY_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o trhist $(HISTORY_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN)
trhist.o : history.cpp history.h
	$(CXX) $(ALL_CXXFLAGS) -DTEST_HISTORY -c -o trhist.o history.cpp

WASASTRINGTOQUERY_OBJS= trwasastrtoq.o  $(BIGLIB) $(MIMELIB)
trwasastrtoq : $(WASASTRINGTOQUERY_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o trwasastrtoq $(WASASTRINGTOQUERY_OBJS) \
	       $(LIBICONV) $(LIBXAPIAN)
trwasastrtoq.o : wasastringtoquery.cpp wasastringtoquery.h
	$(CXX) $(ALL_CXXFLAGS) -DTEST_WASASTRINGTOQUERY -c \
	       -o trwasastrtoq.o wasastringtoquery.cpp

$(BIGLIB): force
	cd $(depth)/lib;$(MAKE)
force:

  
depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(ALL_CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f *.o $(PROGS)

include alldeps
