depth = ..
include $(depth)/mk/sysconf

PROGS = recollindex csguess mimetype
SRCS = recollindex.cpp

all: depend $(PROGS) 

RECOLLINDEX_OBJS= recollindex.o $(BIGLIB) $(MIMELIB)
recollindex : $(RECOLLINDEX_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o recollindex $(RECOLLINDEX_OBJS) \
	       $(BSTATIC) $(LIBXAPIAN) $(LIBICONV) $(BDYNAMIC) $(LIBSYS)
recollindex.o : recollindex.cpp
	$(CXX) $(ALL_CXXFLAGS) -c -o recollindex.o $<

CSGUESS_OBJS= trcsguess.o  $(BIGLIB) 
csguess : $(CSGUESS_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o csguess $(CSGUESS_OBJS) \
	       $(LIBICONV)
trcsguess.o : csguess.cpp
	$(CXX) $(ALL_CXXFLAGS) -DTEST_CSGUESS -c -o trcsguess.o \
	       csguess.cpp

MIMETYPE_OBJS= trmimetype.o  $(BIGLIB) 
mimetype : $(MIMETYPE_OBJS)
	$(CXX) $(ALL_CXXFLAGS) -o mimetype $(MIMETYPE_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trmimetype.o : mimetype.cpp
	$(CXX) $(ALL_CXXFLAGS) -DTEST_MIMETYPE -c -o trmimetype.o \
	       mimetype.cpp

$(BIGLIB): 
	cd $(depth)/lib;$(MAKE)
$(MIMELIB): 
	cd $(depth)/bincimapmime;$(MAKE)

depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(ALL_CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f *.o $(PROGS)

include alldeps
