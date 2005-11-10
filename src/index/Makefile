include ../mk/sysconf

BIGLIB = ../lib/librcl.a
MIMELIB = ../bincimapmime/libmime.a

PROGS = recollindex csguess mimetype
all: $(PROGS)

RECOLLINDEX_OBJS= recollindex.o $(BIGLIB) $(MIMELIB)
recollindex : $(RECOLLINDEX_OBJS)
	$(CXX) $(CXXFLAGS) -o recollindex $(RECOLLINDEX_OBJS) \
	       $(BSTATIC) $(LIBXAPIAN) $(LIBICONV) $(BDYNAMIC) $(LIBSYS)
recollindex.o : idxdriver.cpp
	$(CXX) $(CXXFLAGS) -c -o recollindex.o idxdriver.cpp

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
	       $(LIBICONV)
trmimetype.o : mimetype.cpp
	$(CXX) $(CXXFLAGS) -DTEST_MIMETYPE -c -o trmimetype.o \
	       mimetype.cpp

clean: 
	rm -f *.o $(PROGS)

alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M *.cpp > alldeps
include alldeps

