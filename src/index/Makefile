PROGS = subtreelist mimetype
all: $(PROGS)

SUBTREELIST_OBJS= subtreelist.o
subtreelist : $(SUBTREELIST_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o subtreelist $(SUBTREELIST_OBJS) \
	       $(LIBRECOLL)
subtreelist.o : subtreelist.cpp 
	$(CXX) $(ALL_CXXFLAGS) -DTEST_SUBTREELIST -c subtreelist.cpp

MIMETYPE_OBJS= trmimetype.o
mimetype : $(MIMETYPE_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o mimetype $(MIMETYPE_OBJS) $(LIBRECOLL)
trmimetype.o : mimetype.cpp
	$(CXX) $(ALL_CXXFLAGS) -DTEST_MIMETYPE -c -o trmimetype.o \
	       mimetype.cpp

include ../utils/utmkdefs.mk
