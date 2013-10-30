import sys
from recoll import recoll

# Test the doc.getbinurl() method.
# Select file with a binary name (actually iso8859-1), open it and
# convert/print the contents (also iso8859-1)

if sys.version_info[0] >= 3:
    ISP3 = True
else:
    ISP3 = False

def utf8string(s):
    if ISP3:
        return s
    else:
        return s.encode('utf8')
if ISP3:
    def u(x):
        return x
else:
    import codecs
    def u(x):
        return codecs.unicode_escape_decode(x)[0]

db = recoll.connect()
query = db.query()

# This should select a file with an iso8859-1 file name
nres = query.execute("LATIN1NAME_UNIQUEXXX dir:iso8859name", stemming=0)
qs = "Xapian query: [%s]" % query.getxquery()
print(utf8string(qs))

print("Result count: %d %d" % (nres, query.rowcount))

for doc in query:
    print(utf8string(doc.filename))
    burl = doc.getbinurl()
    bytesname = burl[7:]
    f = open(bytesname, 'rb')
    s = f.read()
    f.close()
    if ISP3:
        content = str(s, "iso8859-1")
    else:
        content = unicode(s, "iso8859-1")
    print("Contents: [%s]"%utf8string(content))
