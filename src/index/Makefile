PROGS = recollindex csguess mimetype
SRCS = recollindex.cpp

all: $(PROGS)

RECOLLINDEX_OBJS= recollindex.o $(BIGLIB) $(MIMELIB)
recollindex : $(RECOLLINDEX_OBJS)
	$(CXX) $(CXXFLAGS) -o recollindex $(RECOLLINDEX_OBJS) \
	       $(BSTATIC) $(LIBXAPIAN) $(LIBICONV) $(BDYNAMIC) $(LIBSYS)
recollindex.o : recollindex.cpp
	$(CXX) $(CXXFLAGS) -c -o recollindex.o $<

CSGUESS_OBJS= trcsguess.o  $(BIGLIB) 
csguess : $(CSGUESS_OBJS)
	$(CXX) $(CXXFLAGS) -o csguess $(CSGUESS_OBJS) \
	       $(LIBICONV)
trcsguess.o : csguess.cpp
	$(CXX) $(CXXFLAGS) -DTEST_CSGUESS -c -o trcsguess.o \
	       csguess.cpp

MIMETYPE_OBJS= trmimetype.o  $(BIGLIB) 
mimetype : $(MIMETYPE_OBJS)
	$(CXX) $(CXXFLAGS) -o mimetype $(MIMETYPE_OBJS) \
	       $(LIBICONV) $(LIBSYS)
trmimetype.o : mimetype.cpp
	$(CXX) $(CXXFLAGS) -DTEST_MIMETYPE -c -o trmimetype.o \
	       mimetype.cpp

$(BIGLIB): 
	cd ../lib;$(MAKE)
$(MIMELIB): 
	cd ../bincimapmime;$(MAKE)

depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f *.o $(PROGS)

include alldeps
include ../mk/sysconf
