
UNACCFLAGS = -g -I. -I../unac $(INCICONV) -DUNAC_VERSION=\"1.0.7\"

LIBS = librcl.a

all: depend $(LIBS)

OBJS = base64.o conftree.o csguess.o debuglog.o \
     execmd.o wipedir.o \
     fstreewalk.o mh_html.o mh_mail.o mh_exec.o mh_text.o htmlparse.o \
     idfile.o indexer.o internfile.o md5.o \
     mimehandler.o mimeparse.o mimetype.o myhtmlparse.o pathhash.o pathut.o \
     rclconfig.o rcldb.o rclinit.o readfile.o smallut.o \
     textsplit.o transcode.o \
     unacpp.o unac.o
SRCS = ../utils/conftree.cpp ../index/csguess.cpp ../utils/debuglog.cpp \
     ../utils/execmd.cpp ../utils/idfile.cpp ../utils/md5.cpp \
     ../utils/wipedir.cpp ../utils/fstreewalk.cpp \
     ../common/mh_html.cpp ../common/mh_mail.cpp ../common/mh_exec.cpp \
     ../common/mh_text.cpp ../common/htmlparse.cpp \
     ../index/indexer.cpp ../common/internfile.cpp \
     ../common/mimehandler.cpp ../utils/mimeparse.cpp ../index/mimetype.cpp \
     ../common/myhtmlparse.cpp ../common/pathhash.cpp ../utils/pathut.cpp \
     ../common/rclconfig.cpp ../common/rcldb.cpp ../common/rclinit.cpp \
     ../utils/base64.cpp ../utils/readfile.cpp ../utils/smallut.cpp \
     ../common/textsplit.cpp ../utils/transcode.cpp \
     ../common/unacpp.cpp ../unac/unac.c

librcl.a : $(OBJS)
	ar ru librcl.a $(OBJS)
	$(RANLIB) librcl.a

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
wipedir.o : ../utils/wipedir.cpp 
	$(CXX) $(CXXFLAGS) -c $<
fstreewalk.o : ../utils/fstreewalk.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mh_html.o : ../common/mh_html.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mh_exec.o : ../common/mh_exec.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mh_text.o : ../common/mh_text.cpp 
	$(CXX) $(CXXFLAGS) -c $<
htmlparse.o : ../common/htmlparse.cpp 
	$(CXX) $(CXXFLAGS) -c $<
idfile.o : ../utils/idfile.cpp 
	$(CXX) $(CXXFLAGS) -c $<
indexer.o : ../index/indexer.cpp 
	$(CXX) $(CXXFLAGS) -c $<
internfile.o : ../common/internfile.cpp 
	$(CXX) $(CXXFLAGS) -c $<
mh_mail.o : ../common/mh_mail.cpp 
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
pathhash.o : ../common/pathhash.cpp 
	$(CXX) $(CXXFLAGS) -c $<
rclconfig.o : ../common/rclconfig.cpp 
	$(CXX) $(CXXFLAGS) -c $<
rclinit.o : ../common/rclinit.cpp 
	$(CXX) $(CXXFLAGS) -c $<
rcldb.o : ../common/rcldb.cpp 
	$(CXX) $(CXXFLAGS) -c $<
readfile.o : ../utils/readfile.cpp 
	$(CXX) $(CXXFLAGS) -c $<
base64.o : ../utils/base64.cpp 
	$(CXX) $(CXXFLAGS) -c $<
smallut.o : ../utils/smallut.cpp 
	$(CXX) $(CXXFLAGS) -c $<
textsplit.o : ../common/textsplit.cpp 
	$(CXX) $(CXXFLAGS) -c $<
transcode.o : ../utils/transcode.cpp 
	$(CXX) $(CXXFLAGS) -c $<
md5.o : ../utils/md5.cpp 
	$(CXX) $(CXXFLAGS) -c $<
unacpp.o : ../common/unacpp.cpp 
	$(CXX) $(CXXFLAGS) -c $<

depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f $(OBJS) $(LIBS)

include alldeps
include ../mk/sysconf
