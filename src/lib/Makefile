# DONT EDIT BY HAND: created by script mkMake
depth = ..
include $(depth)/mk/sysconf

UNACCFLAGS = -g -I. -I$(depth)/unac $(INCICONV) -DUNAC_VERSION=\"1.0.7\"

LIBS = librcl.a

all: $(LIBS)

OBJS =  conftree.o csguess.o debuglog.o execmd.o idfile.o md5.o wipedir.o fstreewalk.o mh_html.o mh_mail.o mh_exec.o mh_text.o htmlparse.o indexer.o internfile.o mimehandler.o mimeparse.o mimetype.o myhtmlparse.o pathhash.o pathut.o rclconfig.o rcldb.o rclinit.o stemdb.o base64.o readfile.o smallut.o textsplit.o transcode.o unacpp.o history.o docseq.o sortseq.o copyfile.o rclaspell.o
DEPS =  conftree.dep.stamp csguess.dep.stamp debuglog.dep.stamp execmd.dep.stamp idfile.dep.stamp md5.dep.stamp wipedir.dep.stamp fstreewalk.dep.stamp mh_html.dep.stamp mh_mail.dep.stamp mh_exec.dep.stamp mh_text.dep.stamp htmlparse.dep.stamp indexer.dep.stamp internfile.dep.stamp mimehandler.dep.stamp mimeparse.dep.stamp mimetype.dep.stamp myhtmlparse.dep.stamp pathhash.dep.stamp pathut.dep.stamp rclconfig.dep.stamp rcldb.dep.stamp rclinit.dep.stamp stemdb.dep.stamp base64.dep.stamp readfile.dep.stamp smallut.dep.stamp textsplit.dep.stamp transcode.dep.stamp unacpp.dep.stamp history.dep.stamp docseq.dep.stamp sortseq.dep.stamp copyfile.dep.stamp rclaspell.dep.stamp

librcl.a : $(DEPS) $(OBJS) unac.o
	ar ru librcl.a $(OBJS) unac.o
	$(RANLIB) librcl.a

unac.o : $(depth)/unac/unac.c $(depth)/unac/unac.h
	$(CC) $(UNACCFLAGS) -c $(depth)/unac/unac.c
conftree.o : ../utils/conftree.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/conftree.cpp
csguess.o : ../index/csguess.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../index/csguess.cpp
debuglog.o : ../utils/debuglog.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/debuglog.cpp
execmd.o : ../utils/execmd.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/execmd.cpp
idfile.o : ../utils/idfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/idfile.cpp
md5.o : ../utils/md5.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/md5.cpp
wipedir.o : ../utils/wipedir.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/wipedir.cpp
fstreewalk.o : ../utils/fstreewalk.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/fstreewalk.cpp
mh_html.o : ../common/mh_html.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/mh_html.cpp
mh_mail.o : ../common/mh_mail.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/mh_mail.cpp
mh_exec.o : ../common/mh_exec.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/mh_exec.cpp
mh_text.o : ../common/mh_text.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/mh_text.cpp
htmlparse.o : ../common/htmlparse.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/htmlparse.cpp
indexer.o : ../index/indexer.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../index/indexer.cpp
internfile.o : ../common/internfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/internfile.cpp
mimehandler.o : ../common/mimehandler.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/mimehandler.cpp
mimeparse.o : ../utils/mimeparse.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/mimeparse.cpp
mimetype.o : ../index/mimetype.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../index/mimetype.cpp
myhtmlparse.o : ../common/myhtmlparse.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/myhtmlparse.cpp
pathhash.o : ../common/pathhash.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/pathhash.cpp
pathut.o : ../utils/pathut.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/pathut.cpp
rclconfig.o : ../common/rclconfig.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/rclconfig.cpp
rcldb.o : ../common/rcldb.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/rcldb.cpp
rclinit.o : ../common/rclinit.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/rclinit.cpp
stemdb.o : ../common/stemdb.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/stemdb.cpp
base64.o : ../utils/base64.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/base64.cpp
readfile.o : ../utils/readfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/readfile.cpp
smallut.o : ../utils/smallut.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/smallut.cpp
textsplit.o : ../common/textsplit.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/textsplit.cpp
transcode.o : ../utils/transcode.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/transcode.cpp
unacpp.o : ../common/unacpp.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/unacpp.cpp
history.o : ../query/history.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/history.cpp
docseq.o : ../query/docseq.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/docseq.cpp
sortseq.o : ../query/sortseq.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/sortseq.cpp
copyfile.o : ../utils/copyfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/copyfile.cpp
rclaspell.o : ../aspell/rclaspell.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../aspell/rclaspell.cpp
depend: $(DEPS)
clean:
	rm -f $(OBJS) $(LIBS) $(DEPS) unac.o
	for i in *.dep;do test -f $$i && cp /dev/null $$i;done
distclean: clean
	rm -f *.dep   
conftree.dep.stamp : ../utils/conftree.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/conftree.cpp > conftree.dep
	touch conftree.dep.stamp
csguess.dep.stamp : ../index/csguess.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../index/csguess.cpp > csguess.dep
	touch csguess.dep.stamp
debuglog.dep.stamp : ../utils/debuglog.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/debuglog.cpp > debuglog.dep
	touch debuglog.dep.stamp
execmd.dep.stamp : ../utils/execmd.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/execmd.cpp > execmd.dep
	touch execmd.dep.stamp
idfile.dep.stamp : ../utils/idfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/idfile.cpp > idfile.dep
	touch idfile.dep.stamp
md5.dep.stamp : ../utils/md5.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/md5.cpp > md5.dep
	touch md5.dep.stamp
wipedir.dep.stamp : ../utils/wipedir.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/wipedir.cpp > wipedir.dep
	touch wipedir.dep.stamp
fstreewalk.dep.stamp : ../utils/fstreewalk.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/fstreewalk.cpp > fstreewalk.dep
	touch fstreewalk.dep.stamp
mh_html.dep.stamp : ../common/mh_html.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/mh_html.cpp > mh_html.dep
	touch mh_html.dep.stamp
mh_mail.dep.stamp : ../common/mh_mail.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/mh_mail.cpp > mh_mail.dep
	touch mh_mail.dep.stamp
mh_exec.dep.stamp : ../common/mh_exec.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/mh_exec.cpp > mh_exec.dep
	touch mh_exec.dep.stamp
mh_text.dep.stamp : ../common/mh_text.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/mh_text.cpp > mh_text.dep
	touch mh_text.dep.stamp
htmlparse.dep.stamp : ../common/htmlparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/htmlparse.cpp > htmlparse.dep
	touch htmlparse.dep.stamp
indexer.dep.stamp : ../index/indexer.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../index/indexer.cpp > indexer.dep
	touch indexer.dep.stamp
internfile.dep.stamp : ../common/internfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/internfile.cpp > internfile.dep
	touch internfile.dep.stamp
mimehandler.dep.stamp : ../common/mimehandler.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/mimehandler.cpp > mimehandler.dep
	touch mimehandler.dep.stamp
mimeparse.dep.stamp : ../utils/mimeparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/mimeparse.cpp > mimeparse.dep
	touch mimeparse.dep.stamp
mimetype.dep.stamp : ../index/mimetype.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../index/mimetype.cpp > mimetype.dep
	touch mimetype.dep.stamp
myhtmlparse.dep.stamp : ../common/myhtmlparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/myhtmlparse.cpp > myhtmlparse.dep
	touch myhtmlparse.dep.stamp
pathhash.dep.stamp : ../common/pathhash.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/pathhash.cpp > pathhash.dep
	touch pathhash.dep.stamp
pathut.dep.stamp : ../utils/pathut.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/pathut.cpp > pathut.dep
	touch pathut.dep.stamp
rclconfig.dep.stamp : ../common/rclconfig.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/rclconfig.cpp > rclconfig.dep
	touch rclconfig.dep.stamp
rcldb.dep.stamp : ../common/rcldb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/rcldb.cpp > rcldb.dep
	touch rcldb.dep.stamp
rclinit.dep.stamp : ../common/rclinit.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/rclinit.cpp > rclinit.dep
	touch rclinit.dep.stamp
stemdb.dep.stamp : ../common/stemdb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/stemdb.cpp > stemdb.dep
	touch stemdb.dep.stamp
base64.dep.stamp : ../utils/base64.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/base64.cpp > base64.dep
	touch base64.dep.stamp
readfile.dep.stamp : ../utils/readfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/readfile.cpp > readfile.dep
	touch readfile.dep.stamp
smallut.dep.stamp : ../utils/smallut.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/smallut.cpp > smallut.dep
	touch smallut.dep.stamp
textsplit.dep.stamp : ../common/textsplit.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/textsplit.cpp > textsplit.dep
	touch textsplit.dep.stamp
transcode.dep.stamp : ../utils/transcode.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/transcode.cpp > transcode.dep
	touch transcode.dep.stamp
unacpp.dep.stamp : ../common/unacpp.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/unacpp.cpp > unacpp.dep
	touch unacpp.dep.stamp
history.dep.stamp : ../query/history.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/history.cpp > history.dep
	touch history.dep.stamp
docseq.dep.stamp : ../query/docseq.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/docseq.cpp > docseq.dep
	touch docseq.dep.stamp
sortseq.dep.stamp : ../query/sortseq.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/sortseq.cpp > sortseq.dep
	touch sortseq.dep.stamp
copyfile.dep.stamp : ../utils/copyfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/copyfile.cpp > copyfile.dep
	touch copyfile.dep.stamp
rclaspell.dep.stamp : ../aspell/rclaspell.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../aspell/rclaspell.cpp > rclaspell.dep
	touch rclaspell.dep.stamp
include conftree.dep
include csguess.dep
include debuglog.dep
include execmd.dep
include idfile.dep
include md5.dep
include wipedir.dep
include fstreewalk.dep
include mh_html.dep
include mh_mail.dep
include mh_exec.dep
include mh_text.dep
include htmlparse.dep
include indexer.dep
include internfile.dep
include mimehandler.dep
include mimeparse.dep
include mimetype.dep
include myhtmlparse.dep
include pathhash.dep
include pathut.dep
include rclconfig.dep
include rcldb.dep
include rclinit.dep
include stemdb.dep
include base64.dep
include readfile.dep
include smallut.dep
include textsplit.dep
include transcode.dep
include unacpp.dep
include history.dep
include docseq.dep
include sortseq.dep
include copyfile.dep
include rclaspell.dep
