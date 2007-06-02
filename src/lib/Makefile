# DONT EDIT BY HAND: created by script mkMake
depth = ..
include $(depth)/mk/sysconf

UNACCFLAGS = $(CFLAGS) -g -I. -I$(depth)/unac $(INCICONV) -DUNAC_VERSION=\"1.0.7\"

LIBS = librcl.a

all: $(LIBS)

OBJS =  rclaspell.o rclconfig.o rclinit.o textsplit.o unacpp.o csguess.o indexer.o mimetype.o htmlparse.o myhtmlparse.o mimehandler.o internfile.o mh_exec.o mh_html.o mh_mail.o mh_mbox.o mh_text.o docseq.o docseqdb.o docseqhist.o history.o sortseq.o wasastringtoquery.o wasatorcl.o pathhash.o rcldb.o searchdata.o stemdb.o stoplist.o base64.o conftree.o copyfile.o debuglog.o execmd.o fstreewalk.o idfile.o md5.o mimeparse.o pathut.o readfile.o smallut.o transcode.o wipedir.o x11mon.o
DEPS =  rclaspell.dep.stamp rclconfig.dep.stamp rclinit.dep.stamp textsplit.dep.stamp unacpp.dep.stamp csguess.dep.stamp indexer.dep.stamp mimetype.dep.stamp htmlparse.dep.stamp myhtmlparse.dep.stamp mimehandler.dep.stamp internfile.dep.stamp mh_exec.dep.stamp mh_html.dep.stamp mh_mail.dep.stamp mh_mbox.dep.stamp mh_text.dep.stamp docseq.dep.stamp docseqdb.dep.stamp docseqhist.dep.stamp history.dep.stamp sortseq.dep.stamp wasastringtoquery.dep.stamp wasatorcl.dep.stamp pathhash.dep.stamp rcldb.dep.stamp searchdata.dep.stamp stemdb.dep.stamp stoplist.dep.stamp base64.dep.stamp conftree.dep.stamp copyfile.dep.stamp debuglog.dep.stamp execmd.dep.stamp fstreewalk.dep.stamp idfile.dep.stamp md5.dep.stamp mimeparse.dep.stamp pathut.dep.stamp readfile.dep.stamp smallut.dep.stamp transcode.dep.stamp wipedir.dep.stamp x11mon.dep.stamp

librcl.a : $(DEPS) $(OBJS) unac.o
	ar ru librcl.a $(OBJS) unac.o
	$(RANLIB) librcl.a

unac.o : $(depth)/unac/unac.c $(depth)/unac/unac.h
	$(CC) $(UNACCFLAGS) -c $(depth)/unac/unac.c
rclaspell.o : ../aspell/rclaspell.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../aspell/rclaspell.cpp
rclconfig.o : ../common/rclconfig.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/rclconfig.cpp
rclinit.o : ../common/rclinit.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/rclinit.cpp
textsplit.o : ../common/textsplit.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/textsplit.cpp
unacpp.o : ../common/unacpp.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../common/unacpp.cpp
csguess.o : ../index/csguess.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../index/csguess.cpp
indexer.o : ../index/indexer.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../index/indexer.cpp
mimetype.o : ../index/mimetype.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../index/mimetype.cpp
htmlparse.o : ../internfile/htmlparse.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/htmlparse.cpp
myhtmlparse.o : ../internfile/myhtmlparse.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/myhtmlparse.cpp
mimehandler.o : ../internfile/mimehandler.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/mimehandler.cpp
internfile.o : ../internfile/internfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/internfile.cpp
mh_exec.o : ../internfile/mh_exec.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/mh_exec.cpp
mh_html.o : ../internfile/mh_html.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/mh_html.cpp
mh_mail.o : ../internfile/mh_mail.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/mh_mail.cpp
mh_mbox.o : ../internfile/mh_mbox.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/mh_mbox.cpp
mh_text.o : ../internfile/mh_text.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../internfile/mh_text.cpp
docseq.o : ../query/docseq.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/docseq.cpp
docseqdb.o : ../query/docseqdb.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/docseqdb.cpp
docseqhist.o : ../query/docseqhist.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/docseqhist.cpp
history.o : ../query/history.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/history.cpp
sortseq.o : ../query/sortseq.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/sortseq.cpp
wasastringtoquery.o : ../query/wasastringtoquery.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/wasastringtoquery.cpp
wasatorcl.o : ../query/wasatorcl.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../query/wasatorcl.cpp
pathhash.o : ../rcldb/pathhash.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../rcldb/pathhash.cpp
rcldb.o : ../rcldb/rcldb.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../rcldb/rcldb.cpp
searchdata.o : ../rcldb/searchdata.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../rcldb/searchdata.cpp
stemdb.o : ../rcldb/stemdb.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../rcldb/stemdb.cpp
stoplist.o : ../rcldb/stoplist.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../rcldb/stoplist.cpp
base64.o : ../utils/base64.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/base64.cpp
conftree.o : ../utils/conftree.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/conftree.cpp
copyfile.o : ../utils/copyfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/copyfile.cpp
debuglog.o : ../utils/debuglog.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/debuglog.cpp
execmd.o : ../utils/execmd.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/execmd.cpp
fstreewalk.o : ../utils/fstreewalk.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/fstreewalk.cpp
idfile.o : ../utils/idfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/idfile.cpp
md5.o : ../utils/md5.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/md5.cpp
mimeparse.o : ../utils/mimeparse.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/mimeparse.cpp
pathut.o : ../utils/pathut.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/pathut.cpp
readfile.o : ../utils/readfile.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/readfile.cpp
smallut.o : ../utils/smallut.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/smallut.cpp
transcode.o : ../utils/transcode.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/transcode.cpp
wipedir.o : ../utils/wipedir.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/wipedir.cpp
x11mon.o : ../utils/x11mon.cpp
	$(CXX) $(ALL_CXXFLAGS) -c ../utils/x11mon.cpp
depend: $(DEPS)
clean:
	rm -f $(OBJS) $(LIBS) $(DEPS) unac.o
	for i in *.dep;do test -f $$i && cp /dev/null $$i;done
distclean: clean
	rm -f *.dep   
rclaspell.dep.stamp : ../aspell/rclaspell.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../aspell/rclaspell.cpp > rclaspell.dep
	touch rclaspell.dep.stamp
rclconfig.dep.stamp : ../common/rclconfig.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/rclconfig.cpp > rclconfig.dep
	touch rclconfig.dep.stamp
rclinit.dep.stamp : ../common/rclinit.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/rclinit.cpp > rclinit.dep
	touch rclinit.dep.stamp
textsplit.dep.stamp : ../common/textsplit.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/textsplit.cpp > textsplit.dep
	touch textsplit.dep.stamp
unacpp.dep.stamp : ../common/unacpp.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../common/unacpp.cpp > unacpp.dep
	touch unacpp.dep.stamp
csguess.dep.stamp : ../index/csguess.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../index/csguess.cpp > csguess.dep
	touch csguess.dep.stamp
indexer.dep.stamp : ../index/indexer.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../index/indexer.cpp > indexer.dep
	touch indexer.dep.stamp
mimetype.dep.stamp : ../index/mimetype.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../index/mimetype.cpp > mimetype.dep
	touch mimetype.dep.stamp
htmlparse.dep.stamp : ../internfile/htmlparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/htmlparse.cpp > htmlparse.dep
	touch htmlparse.dep.stamp
myhtmlparse.dep.stamp : ../internfile/myhtmlparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/myhtmlparse.cpp > myhtmlparse.dep
	touch myhtmlparse.dep.stamp
mimehandler.dep.stamp : ../internfile/mimehandler.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/mimehandler.cpp > mimehandler.dep
	touch mimehandler.dep.stamp
internfile.dep.stamp : ../internfile/internfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/internfile.cpp > internfile.dep
	touch internfile.dep.stamp
mh_exec.dep.stamp : ../internfile/mh_exec.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/mh_exec.cpp > mh_exec.dep
	touch mh_exec.dep.stamp
mh_html.dep.stamp : ../internfile/mh_html.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/mh_html.cpp > mh_html.dep
	touch mh_html.dep.stamp
mh_mail.dep.stamp : ../internfile/mh_mail.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/mh_mail.cpp > mh_mail.dep
	touch mh_mail.dep.stamp
mh_mbox.dep.stamp : ../internfile/mh_mbox.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/mh_mbox.cpp > mh_mbox.dep
	touch mh_mbox.dep.stamp
mh_text.dep.stamp : ../internfile/mh_text.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../internfile/mh_text.cpp > mh_text.dep
	touch mh_text.dep.stamp
docseq.dep.stamp : ../query/docseq.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/docseq.cpp > docseq.dep
	touch docseq.dep.stamp
docseqdb.dep.stamp : ../query/docseqdb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/docseqdb.cpp > docseqdb.dep
	touch docseqdb.dep.stamp
docseqhist.dep.stamp : ../query/docseqhist.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/docseqhist.cpp > docseqhist.dep
	touch docseqhist.dep.stamp
history.dep.stamp : ../query/history.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/history.cpp > history.dep
	touch history.dep.stamp
sortseq.dep.stamp : ../query/sortseq.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/sortseq.cpp > sortseq.dep
	touch sortseq.dep.stamp
wasastringtoquery.dep.stamp : ../query/wasastringtoquery.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/wasastringtoquery.cpp > wasastringtoquery.dep
	touch wasastringtoquery.dep.stamp
wasatorcl.dep.stamp : ../query/wasatorcl.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../query/wasatorcl.cpp > wasatorcl.dep
	touch wasatorcl.dep.stamp
pathhash.dep.stamp : ../rcldb/pathhash.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../rcldb/pathhash.cpp > pathhash.dep
	touch pathhash.dep.stamp
rcldb.dep.stamp : ../rcldb/rcldb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../rcldb/rcldb.cpp > rcldb.dep
	touch rcldb.dep.stamp
searchdata.dep.stamp : ../rcldb/searchdata.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../rcldb/searchdata.cpp > searchdata.dep
	touch searchdata.dep.stamp
stemdb.dep.stamp : ../rcldb/stemdb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../rcldb/stemdb.cpp > stemdb.dep
	touch stemdb.dep.stamp
stoplist.dep.stamp : ../rcldb/stoplist.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../rcldb/stoplist.cpp > stoplist.dep
	touch stoplist.dep.stamp
base64.dep.stamp : ../utils/base64.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/base64.cpp > base64.dep
	touch base64.dep.stamp
conftree.dep.stamp : ../utils/conftree.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/conftree.cpp > conftree.dep
	touch conftree.dep.stamp
copyfile.dep.stamp : ../utils/copyfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/copyfile.cpp > copyfile.dep
	touch copyfile.dep.stamp
debuglog.dep.stamp : ../utils/debuglog.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/debuglog.cpp > debuglog.dep
	touch debuglog.dep.stamp
execmd.dep.stamp : ../utils/execmd.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/execmd.cpp > execmd.dep
	touch execmd.dep.stamp
fstreewalk.dep.stamp : ../utils/fstreewalk.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/fstreewalk.cpp > fstreewalk.dep
	touch fstreewalk.dep.stamp
idfile.dep.stamp : ../utils/idfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/idfile.cpp > idfile.dep
	touch idfile.dep.stamp
md5.dep.stamp : ../utils/md5.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/md5.cpp > md5.dep
	touch md5.dep.stamp
mimeparse.dep.stamp : ../utils/mimeparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/mimeparse.cpp > mimeparse.dep
	touch mimeparse.dep.stamp
pathut.dep.stamp : ../utils/pathut.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/pathut.cpp > pathut.dep
	touch pathut.dep.stamp
readfile.dep.stamp : ../utils/readfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/readfile.cpp > readfile.dep
	touch readfile.dep.stamp
smallut.dep.stamp : ../utils/smallut.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/smallut.cpp > smallut.dep
	touch smallut.dep.stamp
transcode.dep.stamp : ../utils/transcode.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/transcode.cpp > transcode.dep
	touch transcode.dep.stamp
wipedir.dep.stamp : ../utils/wipedir.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/wipedir.cpp > wipedir.dep
	touch wipedir.dep.stamp
x11mon.dep.stamp : ../utils/x11mon.cpp
	$(CXX) -M $(ALL_CXXFLAGS) ../utils/x11mon.cpp > x11mon.dep
	touch x11mon.dep.stamp
include rclaspell.dep
include rclconfig.dep
include rclinit.dep
include textsplit.dep
include unacpp.dep
include csguess.dep
include indexer.dep
include mimetype.dep
include htmlparse.dep
include myhtmlparse.dep
include mimehandler.dep
include internfile.dep
include mh_exec.dep
include mh_html.dep
include mh_mail.dep
include mh_mbox.dep
include mh_text.dep
include docseq.dep
include docseqdb.dep
include docseqhist.dep
include history.dep
include sortseq.dep
include wasastringtoquery.dep
include wasatorcl.dep
include pathhash.dep
include rcldb.dep
include searchdata.dep
include stemdb.dep
include stoplist.dep
include base64.dep
include conftree.dep
include copyfile.dep
include debuglog.dep
include execmd.dep
include fstreewalk.dep
include idfile.dep
include md5.dep
include mimeparse.dep
include pathut.dep
include readfile.dep
include smallut.dep
include transcode.dep
include wipedir.dep
include x11mon.dep
