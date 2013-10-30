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

db = recoll.connect()
query = db.query()

nres = query.execute("huniique", stemlang="english")
qs = "Xapian query: [%s]" % query.getxquery()
print(utf8string(qs))

print("Result count: %d %d" % (nres, query.rowcount))

print("for i in range(nres):")
for i in range(nres):
    doc = query.fetchone()
    print(utf8string(doc.filename))

query.scroll(0, 'absolute')
print("\nfor doc in query:")
for doc in query:
    print(utf8string(doc.filename))

try:
    query.scroll(0, 'badmode')
except:
    print("\nCatched bad mode. (ok)")
    
