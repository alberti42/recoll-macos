.SUFFIXES: .txt .html

.txt.html:
	asciidoc $<

all: $(addsuffix .html,$(basename $(wildcard *.txt)))

clean:
	rm *.html
