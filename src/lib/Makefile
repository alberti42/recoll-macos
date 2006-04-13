depth = ..
include $(depth)/mk/sysconf

UNACCFLAGS = -g -I. -I$(depth)/unac $(INCICONV) -DUNAC_VERSION=\"1.0.7\"

LIBS = librcl.a

all: depend $(LIBS)

OBJS = base64.o conftree.o csguess.o debuglog.o \
     execmd.o wipedir.o \
     fstreewalk.o mh_html.o mh_mail.o mh_exec.o mh_text.o history.o \
     htmlparse.o \
     idfile.o indexer.o internfile.o md5.o \
     mimehandler.o mimeparse.o mimetype.o myhtmlparse.o pathhash.o pathut.o \
     rclconfig.o rcldb.o rclinit.o readfile.o smallut.o \
     textsplit.o transcode.o \
     unacpp.o unac.o docseq.o sortseq.o copyfile.o stemdb.o

SRCS =  \
     $(depth)/utils/conftree.cpp $(depth)/index/csguess.cpp \
     $(depth)/utils/debuglog.cpp $(depth)/utils/execmd.cpp \
     $(depth)/utils/idfile.cpp $(depth)/utils/md5.cpp \
     $(depth)/utils/wipedir.cpp $(depth)/utils/fstreewalk.cpp \
     $(depth)/common/mh_html.cpp $(depth)/common/mh_mail.cpp \
     $(depth)/common/mh_exec.cpp $(depth)/common/mh_text.cpp \
     $(depth)/common/htmlparse.cpp $(depth)/index/indexer.cpp \
     $(depth)/common/internfile.cpp $(depth)/common/mimehandler.cpp \
     $(depth)/utils/mimeparse.cpp $(depth)/index/mimetype.cpp \
     $(depth)/common/myhtmlparse.cpp $(depth)/common/pathhash.cpp \
     $(depth)/utils/pathut.cpp $(depth)/common/rclconfig.cpp \
     $(depth)/common/rcldb.cpp $(depth)/common/rclinit.cpp \
     $(depth)/common/stemdb.cpp \
     $(depth)/utils/base64.cpp $(depth)/utils/readfile.cpp \
     $(depth)/utils/smallut.cpp $(depth)/common/textsplit.cpp \
     $(depth)/utils/transcode.cpp $(depth)/common/unacpp.cpp \
     $(depth)/unac/unac.c $(depth)/query/history.cpp \
     $(depth)/query/docseq.cpp $(depth)/query/sortseq.cpp \
     $(depth)/utils/copyfile.cpp

librcl.a : $(OBJS)
	ar ru librcl.a $(OBJS)
	$(RANLIB) librcl.a

unac.o : $(depth)/unac/unac.c $(depth)/unac/unac.h
	$(CC) $(UNACCFLAGS) -c $<

conftree.o : $(depth)/utils/conftree.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
csguess.o : $(depth)/index/csguess.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
debuglog.o : $(depth)/utils/debuglog.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
execmd.o : $(depth)/utils/execmd.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
wipedir.o : $(depth)/utils/wipedir.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
fstreewalk.o : $(depth)/utils/fstreewalk.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
history.o : $(depth)/query/history.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
sortseq.o : $(depth)/query/sortseq.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
docseq.o : $(depth)/query/docseq.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_html.o : $(depth)/common/mh_html.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_exec.o : $(depth)/common/mh_exec.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_text.o : $(depth)/common/mh_text.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
htmlparse.o : $(depth)/common/htmlparse.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
idfile.o : $(depth)/utils/idfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
indexer.o : $(depth)/index/indexer.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
internfile.o : $(depth)/common/internfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_mail.o : $(depth)/common/mh_mail.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mimehandler.o : $(depth)/common/mimehandler.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mimeparse.o : $(depth)/utils/mimeparse.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
mimetype.o : $(depth)/index/mimetype.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
myhtmlparse.o : $(depth)/common/myhtmlparse.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
pathut.o : $(depth)/utils/pathut.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
pathhash.o : $(depth)/common/pathhash.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
rclconfig.o : $(depth)/common/rclconfig.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
rclinit.o : $(depth)/common/rclinit.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
rcldb.o : $(depth)/common/rcldb.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
stemdb.o : $(depth)/common/stemdb.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
readfile.o : $(depth)/utils/readfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
base64.o : $(depth)/utils/base64.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
smallut.o : $(depth)/utils/smallut.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
textsplit.o : $(depth)/common/textsplit.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
transcode.o : $(depth)/utils/transcode.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
md5.o : $(depth)/utils/md5.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
unacpp.o : $(depth)/common/unacpp.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<
copyfile.o : $(depth)/utils/copyfile.cpp 
	$(CXX) $(ALL_CXXFLAGS) -c $<

depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(ALL_CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f $(OBJS) $(LIBS)

include alldeps
