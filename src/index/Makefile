include ../mk/sysconf

BIGLIB = ../lib/librcl.a

PROGS = idxdriver csguess
all: $(PROGS)

IDXDRIVER_OBJS= idxdriver.o $(BIGLIB)
idxdriver : $(IDXDRIVER_OBJS)
	$(CXX) $(CXXFLAGS) -o idxdriver $(IDXDRIVER_OBJS) \
		$(LIBXAPIAN) $(LIBICONV) $(LIBSYS)

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

