include ../mk/sysconf

BIGLIB = ../lib/librcl.a
MIMELIB = ../bincimapmime/libmime.a

PROGS = recollindex csguess
all: $(PROGS)

RECOLLINDEX_OBJS= recollindex.o $(BIGLIB) $(MIMELIB)
recollindex : $(RECOLLINDEX_OBJS)
	$(CXX) $(CXXFLAGS) -o recollindex $(RECOLLINDEX_OBJS) \
		$(LIBXAPIAN) $(LIBICONV) $(LIBSYS)
recollindex.o : idxdriver.cpp
	$(CXX) $(CXXFLAGS) -c -o recollindex.o idxdriver.cpp

CSGUESS_OBJS= trcsguess.o  $(BIGLIB) 
csguess : $(CSGUESS_OBJS)
	$(CXX) $(CXXFLAGS) -o csguess $(CSGUESS_OBJS) \
	       $(LIBICONV)
trcsguess.o : csguess.cpp
	$(CXX) $(CXXFLAGS) -DTEST_CSGUESS -c -o trcsguess.o \
	       csguess.cpp

clean: 
	rm -f *.o $(PROGS)

alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M *.cpp > alldeps
include alldeps

