include ../mk/FreeBSD

UNACCFLAGS = -g -I. -I../unac -I/usr/local/include -DUNAC_VERSION=\"1.0.7\"


LIBS = librcl.a
all: $(LIBS)

OBJS = conftree.o csguess.o debuglog.o \
     execmd.o \
     fstreewalk.o html.o htmlparse.o indexer.o \
     mimehandler.o mimeparse.o mimetype.o myhtmlparse.o pathut.o \
     rclconfig.o rcldb.o readfile.o smallut.o \
     textsplit.o transcode.o \
     unacpp.o unac.o
SRCS = ../utils/conftree.cpp ../index/csguess.cpp ../utils/debuglog.cpp \
     ../utils/execmd.cpp \
     ../utils/fstreewalk.cpp ../common/html.cpp ../common/htmlparse.cpp \
     ../index/indexer.cpp \
     ../common/mimehandler.cpp ../utils/mimeparse.cpp ../index/mimetype.cpp \
     ../common/myhtmlparse.cpp ../utils/pathut.cpp \
     ../common/rclconfig.cpp ../common/rcldb.cpp ../utils/readfile.cpp \
     ../utils/smallut.cpp \
     ../common/textsplit.cpp ../utils/transcode.cpp \
     ../common/unacpp.cpp ../unac/unac.c

librcl.a : $(OBJS)
	 ar ru librcl.a $(OBJS)

unac.o : ../unac/unac.c ../unac/unac.h
	$(CC) $(UNACCFLAGS) -c $<

#	$(CXX) $(CXXFLAGS) -c $<
conftree.o : ../utils/conftree.cpp 
	$(CXX) $(CXXFLAGS) -c $<
csguess.o : ../index/csguess.cpp 
	$(CXX) $(CXXFLAGS) -c $<
debuglog.o : ../utils/debuglog.cpp 
	$(CXX) $(CXXFLAGS) -c $<
execmd.o : ../utils/execmd.cpp 
	$(CXX) $(CXXFLAGS) -c $<
fstreewalk.o : ../utils/fstreewalk.cpp 
	$(CXX) $(CXXFLAGS) -c $<
html.o : ../common/html.cpp 
	$(CXX) $(CXXFLAGS) -c $<
htmlparse.o : ../common/htmlparse.cpp 
	$(CXX) $(CXXFLAGS) -c $<
indexer.o : ../index/indexer.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mimehandler.o : ../common/mimehandler.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mimeparse.o : ../utils/mimeparse.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mimetype.o : ../index/mimetype.cpp 
	$(CXX) $(CXXFLAGS) -c $<
myhtmlparse.o : ../common/myhtmlparse.cpp 
	$(CXX) $(CXXFLAGS) -c $<
pathut.o : ../utils/pathut.cpp 
	$(CXX) $(CXXFLAGS) -c $<
rclconfig.o : ../common/rclconfig.cpp 
	$(CXX) $(CXXFLAGS) -c $<
rcldb.o : ../common/rcldb.cpp 
	$(CXX) $(CXXFLAGS) -c $<
readfile.o : ../utils/readfile.cpp 
	$(CXX) $(CXXFLAGS) -c $<
smallut.o : ../utils/smallut.cpp 
	$(CXX) $(CXXFLAGS) -c $<
textsplit.o : ../common/textsplit.cpp 
	$(CXX) $(CXXFLAGS) -c $<
transcode.o : ../utils/transcode.cpp 
	$(CXX) $(CXXFLAGS) -c $<
unacpp.o : ../common/unacpp.cpp 
	$(CXX) $(CXXFLAGS) -c $<


clean: 
	rm -f *.o $(LIBS)

alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M $(SRCS) > alldeps
include alldeps

