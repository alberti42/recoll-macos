
CXXFLAGS = -Wall -g -I. -I../unac -I../utils  -I/usr/local/include
CFLAGS = -g -I. -I../unac -I/usr/local/include -DUNAC_VERSION=\"1.0.7\"


PROGS = unacpp
all: $(PROGS)

UNACPP_OBJS= trunacpp.o unacpp.o unac.o readfile.o
unacpp : $(UNACPP_OBJS)
	$(CXX) $(CXXFLAGS) -o unacpp $(UNACPP_OBJS) \
	       -L/usr/local/lib -liconv

unac.o : ../unac/unac.c ../unac/unac.h
	$(CC) $(CFLAGS) -c -o unac.o ../unac/unac.c

readfile.o : ../utils/readfile.cpp ../utils/readfile.h
	$(CXX) $(CXXFLAGS) -c -o readfile.o ../utils/readfile.cpp

trunacpp.o : unacpp.cpp unacpp.h
	$(CXX) $(CXXFLAGS) -DTEST_UNACPP -c -o trunacpp.o unacpp.cpp

clean: 
	rm -f *.o $(PROGS)
