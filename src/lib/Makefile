# DONT EDIT BY HAND: created by script mkMake
depth = ..
include $(depth)/mk/sysconf

UNACCFLAGS = -g -I. -I$(depth)/unac $(INCICONV) -DUNAC_VERSION=\"1.0.7\"

LIBS = librcl.a

all: $(LIBS)

OBJS =  conftree.o csguess.o debuglog.o execmd.o idfile.o md5.o wipedir.o fstreewalk.o mh_html.o mh_mail.o mh_exec.o mh_text.o htmlparse.o indexer.o internfile.o mimehandler.o mimeparse.o mimetype.o myhtmlparse.o pathhash.o pathut.o rclconfig.o rcldb.o rclinit.o stemdb.o base64.o readfile.o smallut.o textsplit.o transcode.o unacpp.o history.o docseq.o sortseq.o copyfile.o
DEPS =  conftree.dep csguess.dep debuglog.dep execmd.dep idfile.dep md5.dep wipedir.dep fstreewalk.dep mh_html.dep mh_mail.dep mh_exec.dep mh_text.dep htmlparse.dep indexer.dep internfile.dep mimehandler.dep mimeparse.dep mimetype.dep myhtmlparse.dep pathhash.dep pathut.dep rclconfig.dep rcldb.dep rclinit.dep stemdb.dep base64.dep readfile.dep smallut.dep textsplit.dep transcode.dep unacpp.dep history.dep docseq.dep sortseq.dep copyfile.dep

librcl.a : $(OBJS) $(DEPS) unac.o
	ar ru librcl.a $(OBJS) unac.o
	$(RANLIB) librcl.a

unac.o : $(depth)/unac/unac.c $(depth)/unac/unac.h
	$(CC) $(UNACCFLAGS) -c $<
conftree.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
csguess.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
debuglog.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
execmd.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
idfile.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
md5.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
wipedir.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
fstreewalk.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_html.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_mail.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_exec.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mh_text.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
htmlparse.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
indexer.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
internfile.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mimehandler.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mimeparse.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
mimetype.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
myhtmlparse.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
pathhash.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
pathut.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
rclconfig.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
rcldb.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
rclinit.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
stemdb.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
base64.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
readfile.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
smallut.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
textsplit.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
transcode.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
unacpp.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
history.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
docseq.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
sortseq.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
copyfile.o :
	$(CXX) $(ALL_CXXFLAGS) -c $<
depend: $(DEPS)
clean:
	rm -f $(OBJS) $(LIBS) $(DEPS) unac.o
conftree.dep : ../utils/conftree.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > conftree.dep
csguess.dep : ../index/csguess.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > csguess.dep
debuglog.dep : ../utils/debuglog.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > debuglog.dep
execmd.dep : ../utils/execmd.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > execmd.dep
idfile.dep : ../utils/idfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > idfile.dep
md5.dep : ../utils/md5.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > md5.dep
wipedir.dep : ../utils/wipedir.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > wipedir.dep
fstreewalk.dep : ../utils/fstreewalk.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > fstreewalk.dep
mh_html.dep : ../common/mh_html.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mh_html.dep
mh_mail.dep : ../common/mh_mail.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mh_mail.dep
mh_exec.dep : ../common/mh_exec.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mh_exec.dep
mh_text.dep : ../common/mh_text.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mh_text.dep
htmlparse.dep : ../common/htmlparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > htmlparse.dep
indexer.dep : ../index/indexer.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > indexer.dep
internfile.dep : ../common/internfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > internfile.dep
mimehandler.dep : ../common/mimehandler.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mimehandler.dep
mimeparse.dep : ../utils/mimeparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mimeparse.dep
mimetype.dep : ../index/mimetype.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > mimetype.dep
myhtmlparse.dep : ../common/myhtmlparse.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > myhtmlparse.dep
pathhash.dep : ../common/pathhash.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > pathhash.dep
pathut.dep : ../utils/pathut.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > pathut.dep
rclconfig.dep : ../common/rclconfig.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > rclconfig.dep
rcldb.dep : ../common/rcldb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > rcldb.dep
rclinit.dep : ../common/rclinit.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > rclinit.dep
stemdb.dep : ../common/stemdb.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > stemdb.dep
base64.dep : ../utils/base64.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > base64.dep
readfile.dep : ../utils/readfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > readfile.dep
smallut.dep : ../utils/smallut.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > smallut.dep
textsplit.dep : ../common/textsplit.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > textsplit.dep
transcode.dep : ../utils/transcode.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > transcode.dep
unacpp.dep : ../common/unacpp.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > unacpp.dep
history.dep : ../query/history.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > history.dep
docseq.dep : ../query/docseq.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > docseq.dep
sortseq.dep : ../query/sortseq.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > sortseq.dep
copyfile.dep : ../utils/copyfile.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< > copyfile.dep
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
