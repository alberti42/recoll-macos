depth = ..
include $(depth)/mk/sysconf

PROGS = rclaspell
SRCS = rclaspell.cpp

all: depend $(BIGLIB) $(PROGS)

RCLASPELL_OBJS= trrclaspell.o  $(BIGLIB) 
rclaspell : $(RCLASPELL_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o rclaspell $(RCLASPELL_OBJS) \
	       $(LIBXAPIAN) $(LIBICONV)
trrclaspell.o : rclaspell.cpp
	$(CXX) $(ALL_CXXFLAGS) -DTEST_RCLASPELL -c -o trrclaspell.o \
	       rclaspell.cpp

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
