
CXXFLAGS = -Wall -g -I. -I../utils -I../common -I/usr/local/include


PROGS = transcode textsplit idxdriver csguess
all: $(PROGS)

IDXDRIVER_OBJS= idxdriver.o pathut.o conftree.o rclconfig.o fstreewalk.o \
		mimetype.o rcldb.o readfile.o transcode.o csguess.o \
		textsplit.o unac.o unacpp.o

idxdriver : $(IDXDRIVER_OBJS)
	$(CXX) $(CXXFLAGS) -o idxdriver $(IDXDRIVER_OBJS) \
	       -L/usr/local/lib -lxapian -liconv

TEXTSPLIT_OBJS= trtextsplit.o textsplit.o readfile.o
textsplit : $(TEXTSPLIT_OBJS)
	$(CXX) $(CXXFLAGS) -o textsplit $(TEXTSPLIT_OBJS)
trtextsplit.o : ../common/textsplit.cpp ../common/textsplit.h
	$(CXX) $(CXXFLAGS) -DTEST_TEXTSPLIT -c -o trtextsplit.o \
	       ../common/textsplit.cpp

pathut.o : ../utils/pathut.cpp ../utils/pathut.h
	$(CXX) $(CXXFLAGS) -c -o pathut.o ../utils/pathut.cpp
conftree.o : ../utils/conftree.cpp ../utils/conftree.h
	$(CXX) $(CXXFLAGS) -c -o conftree.o ../utils/conftree.cpp
rclconfig.o : ../common/rclconfig.cpp ../common/rclconfig.h
	$(CXX) $(CXXFLAGS) -c -o rclconfig.o ../common/rclconfig.cpp
fstreewalk.o : ../utils/fstreewalk.cpp ../utils/fstreewalk.h
	$(CXX) $(CXXFLAGS) -c -o fstreewalk.o ../utils/fstreewalk.cpp
readfile.o : ../utils/readfile.cpp ../utils/readfile.h
	$(CXX) $(CXXFLAGS) -c -o readfile.o ../utils/readfile.cpp


unacpp.o : ../common/unacpp.cpp ../common/unacpp.h
	$(CXX) $(CXXFLAGS) -I../unac -c -o unacpp.o ../common/unacpp.cpp


rcldb.o : ../common/rcldb.cpp ../common/rcldb.h
	$(CXX) $(CXXFLAGS) -c -o rcldb.o ../common/rcldb.cpp
textsplit.o : ../common/textsplit.cpp ../common/textsplit.h
	$(CXX) $(CXXFLAGS) -c -o textsplit.o ../common/textsplit.cpp

CSGUESS_OBJS= trcsguess.o csguess.o readfile.o 
csguess : $(CSGUESS_OBJS)
	$(CXX) $(CXXFLAGS) -o csguess $(CSGUESS_OBJS) \
	       -L/usr/local/lib -liconv
trcsguess.o : csguess.cpp csguess.h
	$(CXX) $(CXXFLAGS) -DTEST_CSGUESS -c -o trcsguess.o \
	       csguess.cpp
TRANSCODE_OBJS= trtranscode.o transcode.o readfile.o 
transcode : $(TRANSCODE_OBJS)
	$(CXX) $(CXXFLAGS) -o transcode $(TRANSCODE_OBJS) \
	       -L/usr/local/lib -liconv
trtranscode.o : transcode.cpp transcode.h
	$(CXX) $(CXXFLAGS) -DTEST_TRANSCODE -c -o trtranscode.o \
	       transcode.cpp


CFLAGS = -g -I. -I../unac -I/usr/local/include -DUNAC_VERSION=\"1.0.7\"
unac.o : ../unac/unac.c ../unac/unac.h
	$(CC) $(CFLAGS) -c -o unac.o ../unac/unac.c



clean: 
	rm -f *.o $(PROGS)

alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M *.cpp > alldeps
include alldeps

