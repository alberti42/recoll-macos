.SUFFIXES: .txt .html

.txt.html:
	asciidoc $<

all: recoll-windows.html recoll-windows-faq.html \
     recoll-webui-install-wsgi.html

clean:
	rm -f *.html
