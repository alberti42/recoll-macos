.SUFFIXES: .txt .html

.txt.html:
	asciidoc $<

all: recoll-mingw.html recoll-windows.html \
     recoll-webui-install-wsgi.html

clean:
	rm -f *.html
