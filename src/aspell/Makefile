PROGS = rclaspell
all: $(PROGS)

RCLASPELL_OBJS= trrclaspell.o 
rclaspell : $(RCLASPELL_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o rclaspell $(RCLASPELL_OBJS) \
          $(LIBRECOLL)
trrclaspell.o : rclaspell.cpp
	$(CXX) $(ALL_CXXFLAGS) -DTEST_RCLASPELL -c -o trrclaspell.o \
	       rclaspell.cpp

include ../utils/utmkdefs.mk
