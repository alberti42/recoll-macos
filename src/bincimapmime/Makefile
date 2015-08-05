PROGS = trbinc
all: $(PROGS)

TRBINCOBJS = trbinc.o
trbinc: trbinc.o
	$(CXX) -o trbinc trbinc.o $(LIBRECOLL)

trbinc.o: trbinc.cc
	$(CXX) $(ALL_CXXFLAGS) -c -o trbinc.o trbinc.cc

include ../utils/utmkdefs.mk
