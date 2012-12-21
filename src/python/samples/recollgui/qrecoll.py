#!/usr/bin/env python

import sys
import datetime
try:
    from recoll import recoll
    from recoll import rclextract
    hasextract = True
except:
    import recoll
    hasextract = False

import queryarea
from getopt import getopt

from PyQt4 import QtCore, QtGui

class RecollQuery(QtCore.QAbstractTableModel):
    def __init__(self):
        QtCore.QAbstractTableModel.__init__(self)
        self.totres = -1
        self.query = None
        self.docs = []
        self.pagelen = 10
        self.attrs = ("filename", "title", "mtime", "url", "ipath")
    def rowCount(self, parent):
        ret = len(self.docs)
        #print "RecollQuery.rowCount(): ", ret
        return ret
    def columnCount(self, parent):
        #print "RecollQuery.columnCount()"
        if parent.isValid():
            return 0
        else:
            return len(self.attrs)
    def setquery(self, db, q):
        """Parse and execute query on open db"""
        print "RecollQuery.setquery():"
        # Get query object
        self.query = db.query()
        # Parse/run input query string
        self.totres = self.query.execute(q)
        self.docs = []
        self.fetchMore(None)
    def getdoc(self, index):
        if index.row() < len(self.docs):
            return self.docs[index.row()]
        else:
            return None
    def headerData(self, idx, orient, role):
        if orient == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return self.attrs[idx]
        return None
    def data(self, index, role):
        #print "RecollQuery.data: row %d, role: " % (index.row(),), role
        if not index.isValid():
            return QtCore.QVariant()

        if index.row() >= len(self.docs):
            return QtCore.QVariant()

        if role == QtCore.Qt.DisplayRole:
            #print "RecollQuery.data: row %d, col %d role: " % \
             #     (index.row(), index.column()), role
            attr = self.attrs[index.column()]
            value = getattr(self.docs[index.row()], attr)
            if attr == "mtime":
                dte = datetime.datetime.fromtimestamp(int(value))
                value = str(dte)
            return value
        else:
            return QtCore.QVariant()
    def canFetchMore(self, parent):
        #print "RecollQuery.canFetchMore:"
        if len(self.docs) < self.totres:
            return True
        else:
            return False
    def fetchMore(self, parent):
        #print "RecollQuery.fetchMore:"
        self.beginInsertRows(QtCore.QModelIndex(), len(self.docs), \
                             len(self.docs) + self.pagelen)
        count = 0
        while self.query.next >= 0 and self.query.next < self.totres \
                  and count < self.pagelen:
            #print "Got: ", title.encode("utf-8")
            self.docs.append(self.query.fetchone())
            count += 1
        self.endInsertRows()

class HlMeths:
    def __init__(self, groups):
        self.groups = groups
    def startMatch(self, idx):
        ugroup = " ".join(self.groups[idx][1])
        return '<font color="blue">'
    def endMatch(self):
        return '</font>'

def extract(doc):
    extractor = rclextract.Extractor(doc)
    newdoc = extractor.textextract(doc.ipath)
    return newdoc

def extractofile(doc, outfilename=""):
    extractor = rclextract.Extractor(doc)
    outfilename = extractor.idoctofile(doc.ipath, doc.mimetype, \
                                       ofilename=outfilename)
    return outfilename

class RclGui_Main(QtGui.QWidget):
    def __init__(self, db, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.ui = queryarea.Ui_queryArea()
        self.ui.setupUi(self)
        self.db = db
        self.qmodel = RecollQuery()
        self.connect(self.ui.exitPB, QtCore.SIGNAL("clicked()"),
                     self.onexit)
    def on_searchEntry_returnPressed(self):
        self.startQuery()
    def on_resTable_clicked(self, index):
        doc = self.qmodel.getdoc(index)
        query = self.qmodel.query;
        groups = self.qmodel.query.getgroups()
        meths = HlMeths(groups)
        if doc is not None:
            ipath = doc.get('ipath')
            print "ipath[", ipath, "]"
            if index.column() == 1:
                newdoc = extract(doc)
                print "newdoc.mimetype:", newdoc.mimetype
                if newdoc.mimetype == 'text/html':
                    ishtml = True
                else:
                    ishtml = False
                text = query.highlight(newdoc.text,
                                       methods=meths,
                                       ishtml=ishtml,
                                       eolbr=True)
                print text
                
                text = '<qt><head></head><body>' + text + '</body></qt>'
                self.ui.resDetail.setText(text)
            elif index.column() == 3 and ipath:
                fn = QtGui.QFileDialog.getSaveFileName(self)
                if fn:
                    docitems = doc.items()
                    fn = extractofile(doc, str(fn.toLocal8Bit()))
                    print "Saved as", fn
                else:
                    print >> sys.stderr, "Canceled"
            else:
                abs = query.makedocabstract(doc, methods=meths)
                self.ui.resDetail.setText(abs)
                
    def startQuery(self):
        self.qmodel.setquery(self.db, self.ui.searchEntry.text())
        self.ui.resTable.setModel(self.qmodel)
    def onexit(self):
        sys.exit(0)
        
def Usage():
    print >> sys.stderr, '''Usage: qt.py [<qword1> [<qword2> ...]]'''
    sys.exit(1)

def main(args):

    app = QtGui.QApplication(args)

    confdir=""
    extra_dbs = []
    # Snippet params
    maxchars = 300
    contextwords = 6

    # Process options: [-c confdir] [-i extra_db [-i extra_db] ...]
    options, args = getopt(args[1:], "c:i:")
    for opt,val in options:
        if opt == "-c":
            confdir = val
        elif opt == "-i":
            extra_dbs.append(val)
        else:
            print >> sys.stderr, "Bad opt: ", opt
            Usage()

    # The query should be in the remaining arg(s)
    q = None
    if len(args) > 0:
        q = ""
        for word in args:
            q += word + " "

    db = recoll.connect(confdir=confdir, extra_dbs=extra_dbs)
    db.setAbstractParams(maxchars=maxchars, contextwords=contextwords)

    topwindow = RclGui_Main(db)
    topwindow.show()
    if q is not None:
        topwindow.ui.searchEntry.setText(q)
        topwindow.startQuery()
    
    sys.exit(app.exec_())


if __name__=="__main__":
    main(sys.argv)
