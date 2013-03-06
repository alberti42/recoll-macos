= Building the Recoll user manual

The Recoll user manual usually used DocBook SGML and used the FreeBSD doc
toolchain to produce the output formats. This had the advantage of an easy
way to produce all formats including a PDF manual, but presented two
problems:

 - Dependancy on the FreeBSD platform.
 - No support for UTF-8 (last I looked), only latin1.

The manual is now compatible with XML. There is a small script that
converts the SGML (but XML-compatible) manual into XML (changes the header,
mostly). The SGML version is still the primary one.

Beyond fixing a few missing closing tags, the main change that had to be
made was to make the anchors explicitly upper-case because the SGML
toolchain converts them to upper-case and the XML one does not, so the only
way to have compatibility is to make them upper-case in the first place.

We still have a problem for producing the PDF manual, because few
straightforward approaches seem to exist:

 -  http://docbookpublishing.com qui a meme une version programmatique (cf:
     http://docbookpublishing.com/api/), mais necessite un peu de
     configuration. 
 - FOP but this is Java and complicated.

See also http://www.valdyas.org/linguistics/printing_unicode.html 
Does not look simple, but dates from 2002 and seems to imply that FOP is
making progress.

The current conclusion would seem to be that the SGML version should stay
operational to give an easy way to make the PDF one on FreeBSD.
