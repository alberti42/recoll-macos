PROGS = synfamily stoplist

all: $(PROGS) 

STOPLIST_OBJS= trstoplist.o
stoplist : $(STOPLIST_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o stoplist $(STOPLIST_OBJS) $(LIBRECOLL)
trstoplist.o : stoplist.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_STOPLIST -c -o trstoplist.o \
	       stoplist.cpp

SYNFAMILY_OBJS= trsynfamily.o
synfamily : $(SYNFAMILY_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o synfamily $(SYNFAMILY_OBJS) \
        $(LIBRECOLL) -lxapian

trsynfamily.o : synfamily.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_SYNFAMILY -c -o trsynfamily.o \
	       synfamily.cpp

include ../utils/utmkdefs.mk
