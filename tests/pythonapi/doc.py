import sys
from recoll import recoll

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

nres = query.execute("testfield:testfieldvalue1", stemming=0)
qs = "Xapian query: [%s]" % query.getxquery()
print(utf8string(qs))

print("Result count: %d %d" % (nres, query.rowcount))

for doc in query:
    print("doc.title: [%s]"%utf8string(doc.title))
    print("doc.testfield: [%s]"%utf8string(doc.testfield))
    for fld in ('title', 'testfield', 'filename'):
        print("getattr(doc, %s) -> [%s]"%(fld,utf8string(getattr(doc, fld))))
        print("doc.get(%s) -> [%s]"%(fld,utf8string(doc.get(fld))))
    print("\nfor fld in sorted(doc.keys()):")
    for fld in sorted(doc.keys()):
        # Sig keeps changing and makes it impossible to compare test results
        if fld != 'sig':
            print(utf8string("[%s] -> [%s]" % (fld, getattr(doc, fld))))
    print("\nfor k,v in sorted(doc.items().items()):")
    for k,v in sorted(doc.items().items(), key=lambda itm: itm[0]):
        # Sig keeps changing and makes it impossible to compare test results
        if k != 'sig':
            print(utf8string("[%s] -> [%s]" % (k, v)))

print("\nAccented query:")
uqs = u('title:"\u00e9t\u00e9 \u00e0 no\u00ebl"')
print("User query [%s]"%utf8string(uqs))
nres = query.execute(uqs, stemming=0)
#nres = query.execute('title:"ete a noel"', stemming=0)
qs = "Xapian query: [%s]" % query.getxquery()
print(utf8string(qs))
print("nres %d" %(nres,))
doc = query.fetchone()
print("doc.title: [%s]"%utf8string(doc.title))
