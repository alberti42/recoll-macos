
LIBS = libmime.a
PROGS = trbinc

all: depend $(LIBS) 

SRCS = mime-getpart.cc mime-parsefull.cc mime-parseonlyheader.cc \
     mime-printbody.cc mime-printdoc.cc mime-printheader.cc mime.cc \
     convert.cc iodevice.cc iofactory.cc

OBJS = mime-getpart.o mime-parsefull.o mime-parseonlyheader.o \
     mime-printbody.o mime-printdoc.o mime-printheader.o mime.o  \
     convert.o iodevice.o iofactory.o

libmime.a : $(OBJS)
	 $(AR) ru libmime.a $(OBJS)

.cc .o : 
	$(CXX) $(CXXFLAGS) -c $<

TRBINCOBJS = trbinc.o
trbinc: trbinc.o
	$(CXX) -o trbinc trbinc.o libmime.a 

depend: alldeps.stamp
alldeps.stamp : $(SRCS)
	$(CXX) -M $(CXXFLAGS) $(SRCS) > alldeps
	touch alldeps.stamp

clean:
	cp /dev/null alldeps
	rm -f alldeps.stamp
	rm -f ${LIBS} ${PROGS} ${OBJS}

include ../mk/sysconf
include alldeps
