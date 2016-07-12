ALL_CXXFLAGS = -std=c++11 \
    -I../aspell \
    -I../bincimapmime \
    -I../common \
    -I../index \
    -I../internfile \
    -I../rcldb \
    -I../unac \
    -I../utils 
LIBRECOLL = -L../.libs -lrecoll -Wl,-rpath=$(shell pwd)/../.libs

clean:
	rm -f $(PROGS) *.o
