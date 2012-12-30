all:
	echo libdir: $(libdir)
	libdir=$(libdir) python setup.py build
install:
	sudo python setup.py install
clean:
	rm -rf build
	rm -f recoll/__init__.pyc
distclean: clean
	rm -f setup.py
