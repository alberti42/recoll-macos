include ../mk/sysconf

LIBS = libmime.a
all: $(LIBS)

SRCS = mime-getpart.cc mime-parsefull.cc mime-parseonlyheader.cc \
     mime-printbody.cc mime-printdoc.cc mime-printheader.cc mime.cc \
     convert.cc iodevice.cc iofactory.cc

OBJS = mime-getpart.o mime-parsefull.o mime-parseonlyheader.o \
     mime-printbody.o mime-printdoc.o mime-printheader.o mime.o  \
     convert.o iodevice.o iofactory.o

libmime.a : $(OBJS)
	 ar ru libmime.a $(OBJS)

#	$(CXX) $(CXXFLAGS) -c $<
.cc .o : 
	$(CXX) $(CXXFLAGS) -c $<


clean: 
	rm -f *.o $(LIBS)

alldeps:depend
depend:
	$(CXX) $(CXXFLAGS) -M $(SRCS) > alldeps
include alldeps

