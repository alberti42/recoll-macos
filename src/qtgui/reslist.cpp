#ifndef lint
static char rcsid[] = "@(#$Id: reslist.cpp,v 1.52 2008-12-17 15:12:08 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include "autoconfig.h"

#include <time.h>
#include <stdlib.h>

#include <qapplication.h>
#include <qvariant.h>
#include <qevent.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qimage.h>
#include <qclipboard.h>
#include <qscrollbar.h>
#include <QTextBlock>
#ifndef __APPLE__
#include <qx11info_x11.h>
#endif

#include "debuglog.h"
#include "smallut.h"
#include "recoll.h"
#include "guiutils.h"
#include "pathut.h"
#include "docseq.h"
#include "pathut.h"
#include "mimehandler.h"
#include "plaintorich.h"
#include "refcntr.h"
#include "internfile.h"

#include "reslist.h"
#include "moc_reslist.cpp"
#include "rclhelp.h"
#ifdef RCL_USE_ASPELL
#include "rclaspell.h"
#endif

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

class QtGuiResListPager : public ResListPager {
public:
    QtGuiResListPager(ResList *p, int ps) 
	: ResListPager(ps), m_parent(p) 
    {}
    virtual bool append(const string& data);
    virtual bool append(const string& data, int idx, const Rcl::Doc& doc);
    virtual string trans(const string& in);
    virtual string detailsLink();
    virtual const string &parFormat();
    virtual string nextUrl();
    virtual string prevUrl();
    virtual string pageTop();
    virtual string iconPath(const string& mt);
    virtual void suggest(const vector<string>uterms, vector<string>&sugg);
private:
    ResList *m_parent;
};

#if 0
FILE *fp;
void logdata(const char *data)
{
    if (fp == 0)
	fp = fopen("/tmp/recolltoto.html", "a");
    if (fp)
	fprintf(fp, "%s", data);
}
#else
#define logdata(X)
#endif

//////////////////////////////
// /// QtGuiResListPager methods:
bool QtGuiResListPager::append(const string& data)
{
    LOGDEB2(("QtGuiReslistPager::appendString   : %s\n", data.c_str()));
    logdata(data.c_str());
    m_parent->append(QString::fromUtf8(data.c_str()));
    return true;
}

bool QtGuiResListPager::append(const string& data, int docnum, 
			       const Rcl::Doc& doc)
{
    LOGDEB2(("QtGuiReslistPager::appendDoc: blockCount %d, %s\n",
	    m_parent->document()->blockCount(), data.c_str()));
    logdata(data.c_str());
    int blkcnt0 = m_parent->document()->blockCount();
    m_parent->m_curDocs.push_back(doc);
    m_parent->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    m_parent->textCursor().insertBlock();
    m_parent->insertHtml(QString::fromUtf8(data.c_str()));
    m_parent->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    m_parent->ensureCursorVisible();
    int blkcnt1 = m_parent->document()->blockCount();
    for (int block = blkcnt0; block < blkcnt1; block++) {
	m_parent->m_pageParaToReldocnums[block] = docnum;
    }
    return true;
}

string QtGuiResListPager::trans(const string& in)
{
    return string((const char*)ResList::tr(in.c_str()).toUtf8());
}

string QtGuiResListPager::detailsLink()
{
    string chunk = "<a href=\"H-1\">";
    chunk += trans("(show query)");
    chunk += "</a>";
    return chunk;
}

const string& QtGuiResListPager::parFormat()
{
    return prefs.creslistformat;
}

string QtGuiResListPager::nextUrl()
{
    return "n-1";
}

string QtGuiResListPager::prevUrl()
{
    return "p-1";
}

string QtGuiResListPager::pageTop() 
{
    return string();
}

string QtGuiResListPager::iconPath(const string& mtype)
{
    string iconpath;
    RclConfig::getMainConfig()->getMimeIconName(mtype, &iconpath);
    return iconpath;
}

void QtGuiResListPager::suggest(const vector<string>uterms, vector<string>&sugg)
{
    sugg.clear();
#ifdef RCL_USE_ASPELL
    bool noaspell = false;
    rclconfig->getConfParam("noaspell", &noaspell);
    if (noaspell)
        return;
    if (!aspell) {
        LOGERR(("QtGuiResListPager:: aspell not initialized\n"));
        return;
    }
    for (vector<string>::const_iterator uit = uterms.begin();
         uit != uterms.end(); uit++) {
        list<string> asuggs;
        string reason;
        if (aspell->check(*rcldb, *uit, reason))
            continue;
        else if (!reason.empty())
            return;
        if (!aspell->suggest(*rcldb, *uit, asuggs, reason)) {
            LOGERR(("QtGuiResListPager::suggest: aspell failed: %s\n", 
                    reason.c_str()));
            continue;
        }
        if (!asuggs.empty()) {
            sugg.push_back(*asuggs.begin());
        }
    }
#endif

}
/////// /////// End reslistpager methods

class PlainToRichQtReslist : public PlainToRich {
public:
    virtual ~PlainToRichQtReslist() {}
    virtual string startMatch() {
	return string("<span style='color: ")
	    + string((const char *)prefs.qtermcolor.toAscii()) + string("'>");
    }
    virtual string endMatch() {return string("</span>");}
};
static PlainToRichQtReslist g_hiliter;

/////////////////////////////////////

ResList::ResList(QWidget* parent, const char* name)
    : QTextBrowser(parent)
{
    if (!name)
	setObjectName("resList");
    else 
	setObjectName(name);
    setReadOnly(TRUE);
    setUndoRedoEnabled(FALSE);
    setOpenLinks(FALSE);
    languageChange();

    setTabChangesFocus(true);

    (void)new HelpClient(this);
    HelpClient::installMap((const char *)this->objectName().toAscii(), 
			   "RCL.SEARCH.RESLIST");

    // signals and slots connections
    connect(this, SIGNAL(anchorClicked(const QUrl &)), 
	    this, SLOT(linkWasClicked(const QUrl &)));
#if 0
    // See comments in "highlighted
    connect(this, SIGNAL(highlighted(const QString &)), 
	    this, SLOT(highlighted(const QString &)));
#endif
    connect(this, SIGNAL(headerClicked()), this, SLOT(showQueryDetails()));
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	    this, SLOT(createPopupMenu(const QPoint&)));

    m_curPvDoc = -1;
    m_lstClckMod = 0;
    m_listId = 0;
    m_pager = new QtGuiResListPager(this, prefs.respagesize);
    m_pager->setHighLighter(&g_hiliter);
}

ResList::~ResList()
{
    // These have to exist somewhere for translations to work
#ifdef __GNUC__
    __attribute__((unused))
#endif
    static const char* strings[] = {
	QT_TR_NOOP("<p><b>No results found</b><br>"),
	QT_TR_NOOP("Documents"),
	QT_TR_NOOP("out of at least"),
	QT_TR_NOOP("for"),
	QT_TR_NOOP("Previous"),
	QT_TR_NOOP("Next"),
	QT_TR_NOOP("Unavailable document"),
	QT_TR_NOOP("Preview"),
	QT_TR_NOOP("Open"),
	QT_TR_NOOP("(show query)"),
        QT_TR_NOOP("<p><i>Alternate spellings (accents suppressed): </i>"),
    };
}

int ResList::newListId()
{
    static int id;
    return ++id;
}

extern "C" int XFlush(void *);

void ResList::setDocSource(RefCntr<DocSequence> ndocsource)
{
    m_baseDocSource = ndocsource;
    setDocSource();
}

// Reapply parameters. Sort params probably changed
void ResList::setDocSource()
{
    LOGDEB(("ResList::setDocSource\n"));
    if (m_baseDocSource.isNull())
	return;
    resetList();
    m_listId = newListId();
    m_docSource = m_baseDocSource;

    // Filtering must be done before sorting, (which usually
    // truncates the original list)
    if (m_baseDocSource->canFilter()) {
	m_baseDocSource->setFiltSpec(m_filtspecs);
    } else {
	if (m_filtspecs.isNotNull()) {
	    string title = m_baseDocSource->title() + " (" + 
		string((const char*)tr("filtered").toUtf8()) + ")";
	    m_docSource = 
		RefCntr<DocSequence>(new DocSeqFiltered(m_docSource,m_filtspecs,
							title));
	} 
    }
    
    if (m_docSource->canSort()) {
	m_docSource->setSortSpec(m_sortspecs);
    } else {
	if (m_sortspecs.isNotNull()) {
	    LOGDEB(("Reslist: sortspecs not Null\n"));
	    string title = m_baseDocSource->title() + " (" + 
		string((const char *)tr("sorted").toUtf8()) + ")";
	    m_docSource = RefCntr<DocSequence>(new DocSeqSorted(m_docSource, 
								m_sortspecs,
								title));
	}
    }
    // Reset the page size in case the preference was changed
    m_pager->setPageSize(prefs.respagesize);
    m_pager->setDocSource(m_docSource);
    resultPageNext();
    emit hasResults(getResCnt());
}

void ResList::setSortParams(DocSeqSortSpec spec)
{
    LOGDEB(("ResList::setSortParams: %s\n", spec.isNotNull() ? "notnull":"null"));
    m_sortspecs = spec;
}

void ResList::setFilterParams(const DocSeqFiltSpec& spec)
{
    LOGDEB(("ResList::setFilterParams\n"));
    m_filtspecs = spec;
}

void ResList::resetList() 
{
    m_curPvDoc = -1;
    // There should be a progress bar for long searches but there isn't 
    // We really want the old result list to go away, otherwise, for a
    // slow search, the user will wonder if anything happened. The
    // following helps making sure that the textedit is really
    // blank. Else, there are often icons or text left around
    clear();
    QTextBrowser::append(".");
    clear();
#ifndef __APPLE__
    XFlush(QX11Info::display());
#endif
}

bool ResList::displayingHistory()
{
    // We want to reset the displayed history if it is currently
    // shown. Using the title value is an ugly hack
    string htstring = string((const char *)tr("Document history").toUtf8());
    if (m_docSource.isNull() || m_docSource->title().empty())
	return false;
    return m_docSource->title().find(htstring) == 0;
}

void ResList::languageChange()
{
    setWindowTitle(tr("Result list"));
}

bool ResList::getTerms(vector<string>& terms, 
		       vector<vector<string> >& groups, vector<int>& gslks)
{
    return m_baseDocSource->getTerms(terms, groups, gslks);
}

list<string> ResList::expand(Rcl::Doc& doc)
{
    if (m_baseDocSource.isNull())
	return list<string>();
    return m_baseDocSource->expand(doc);
}

// Get document number from paragraph number
int ResList::docnumfromparnum(int par)
{
    if (m_pager->pageNumber() < 0)
	return -1;
    std::map<int,int>::iterator it;

    // Try to find the first number < input and actually in the map
    // (result blocks can be made of several text blocks)
    while (par > 0) {
	it = m_pageParaToReldocnums.find(par);
	if (it != m_pageParaToReldocnums.end())
	    break;
	par--;
    }
    int dn;
    if (it != m_pageParaToReldocnums.end()) {
        dn = m_pager->pageNumber() * prefs.respagesize + it->second;
    } else {
        dn = -1;
    }
    return dn;
}

// Get paragraph number from document number
pair<int,int> ResList::parnumfromdocnum(int docnum)
{
    LOGDEB(("parnumfromdocnum: docnum %d\n", docnum));
    if (m_pager->pageNumber() < 0) {
	LOGDEB(("parnumfromdocnum: no page return -1,-1\n"));
	return pair<int,int>(-1,-1);
    }
    int winfirst = m_pager->pageNumber() * prefs.respagesize;
    if (docnum - winfirst < 0) {
	LOGDEB(("parnumfromdocnum: not in win return -1,-1\n"));
	return pair<int,int>(-1,-1);
    }
    docnum -= winfirst;
    for (std::map<int,int>::iterator it = m_pageParaToReldocnums.begin();
	 it != m_pageParaToReldocnums.end(); it++) {
	if (docnum == it->second) {
	    int first = it->first;
	    int last = first+1;
	    std::map<int,int>::iterator it1;
	    while ((it1 = m_pageParaToReldocnums.find(last)) != 
		   m_pageParaToReldocnums.end() && it1->second == docnum) {
		last++;
	    }
	    LOGDEB(("parnumfromdocnum: return %d,%d\n", first, last));
	    return pair<int,int>(first, last);
	}
    }
    LOGDEB(("parnumfromdocnum: not found return -1,-1\n"));
    return pair<int,int>(-1,-1);
}

// Return doc from current or adjacent result pages. We can get called
// for a document not in the current page if the user browses through
// results inside a result window (with shift-arrow). This can only
// result in a one-page change.
bool ResList::getDoc(int docnum, Rcl::Doc &doc)
{
    LOGDEB(("ResList::getDoc: docnum %d winfirst %d\n", docnum, 
	    m_pager->pageNumber() * prefs.respagesize));
    if (docnum < 0)
	return false;
    if (m_pager->pageNumber() < 0)
	return false;
    int winfirst = m_pager->pageNumber() * prefs.respagesize;
    // Is docnum in current page ? Then all Ok
    if (docnum >= winfirst && docnum < winfirst + int(m_curDocs.size())) {
	doc = m_curDocs[docnum - winfirst];
	return true;
    }

    // Else we accept to page down or up but not further
    if (docnum < winfirst && docnum >= winfirst - prefs.respagesize) {
	resultPageBack();
    } else if (docnum < winfirst + int(m_curDocs.size()) + prefs.respagesize) {
	resultPageNext();
    }
    winfirst = m_pager->pageNumber() * prefs.respagesize;
    if (docnum >= winfirst && docnum < winfirst + int(m_curDocs.size())) {
	doc = m_curDocs[docnum - winfirst];
	return true;
    }
    return false;
}

void ResList::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Q && (e->modifiers() & Qt::ControlModifier)) {
	recollNeedsExit = 1;
	return;
    } else if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_Backspace) {
	resPageUpOrBack();
	return;
    } else if (e->key() == Qt::Key_PageDown || e->key() == Qt::Key_Space) {
	resPageDownOrNext();
	return;
    }
    QTextBrowser::keyPressEvent(e);
}

void ResList::mouseReleaseEvent(QMouseEvent *e)
{
    m_lstClckMod = 0;
    if (e->modifiers() & Qt::ControlModifier) {
	m_lstClckMod |= Qt::ControlModifier;
    } 
    if (e->modifiers() & Qt::ShiftModifier) {
	m_lstClckMod |= Qt::ShiftModifier;
    }
    QTextBrowser::mouseReleaseEvent(e);
}

// Return total result list count
int ResList::getResCnt()
{
    if (m_docSource.isNull())
	return -1;
    return m_docSource->getResCnt();
}

void ResList::highlighted(const QString& )
{
    // This is supposedly called when a link is preactivated (hover or tab
    // traversal, but is not actually called for tabs. We would have liked to
    // give some kind of visual feedback for tab traversal
}

// Page Up/Down: we don't try to check if current paragraph is last or
// first. We just page up/down and check if viewport moved. If it did,
// fair enough, else we go to next/previous result page.
void ResList::resPageUpOrBack()
{
    int vpos = verticalScrollBar()->value();
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    if (vpos == verticalScrollBar()->value())
	resultPageBack();
}

void ResList::resPageDownOrNext()
{
    int vpos = verticalScrollBar()->value();
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    LOGDEB(("ResList::resPageDownOrNext: vpos before %d, after %d\n",
	    vpos, verticalScrollBar()->value()));
    if (vpos == verticalScrollBar()->value()) 
	resultPageNext();
}

// Show previous page of results. We just set the current number back
// 2 pages and show next page.
void ResList::resultPageBack()
{
    m_pager->resultPageBack();
    displayPage();
}

// Go to the first page
void ResList::resultPageFirst()
{
    // In case the preference was changed
    m_pager->setPageSize(prefs.respagesize);
    m_pager->resultPageFirst();
    displayPage();
}

void ResList::append(const QString &text)
{
    LOGDEB2(("QtGuiReslistPager::appendQString  : %s\n", 
	    (const char*)text.toUtf8()));
    QTextBrowser::append(text);
}

// Fill up result list window with next screen of hits
void ResList::resultPageNext()
{
    m_pager->resultPageNext();
    displayPage();
}

void ResList::displayPage()
{
    m_curDocs.clear();
    m_pageParaToReldocnums.clear();
    clear();
    m_pager->displayPage();
    LOGDEB0(("ResList::resultPageNext: hasNext %d hasPrev %d\n",
	    m_pager->hasPrev(), m_pager->hasNext()));
    emit prevPageAvailable(m_pager->hasPrev());
    emit nextPageAvailable(m_pager->hasNext());
    // Possibly color paragraph of current preview if any
    previewExposed(m_curPvDoc);
    ensureCursorVisible();
}

// Color paragraph (if any) of currently visible preview
void ResList::previewExposed(int docnum)
{
    LOGDEB(("ResList::previewExposed: doc %d\n", docnum));

    // Possibly erase old one to white
    pair<int,int> blockrange;
    if (m_curPvDoc != -1) {
	blockrange = parnumfromdocnum(m_curPvDoc);
	if (blockrange.first != -1) {
	    for (int blockn = blockrange.first;
		 blockn < blockrange.second; blockn++) {
		QTextBlock block = document()->findBlockByNumber(blockn);
		QTextCursor cursor(block);
		QTextBlockFormat format = cursor.blockFormat();
		format.clearBackground();
		cursor.setBlockFormat(format);
	    }
	}
	m_curPvDoc = -1;
    }

    // Set background for active preview's doc entry
    m_curPvDoc = docnum;
    blockrange = parnumfromdocnum(docnum);

    // Maybe docnum is -1 or not in this window, 
    if (blockrange.first < 0)
	return;

    // Color the new active paragraph
    QColor color("LightBlue");
    for (int blockn = blockrange.first+1;
	 blockn < blockrange.second; blockn++) {
	QTextBlock block = document()->findBlockByNumber(blockn);
	QTextCursor cursor(block);
	QTextBlockFormat format;
	format.setBackground(QBrush(color));
	cursor.mergeBlockFormat(format);
	setTextCursor(cursor);
	ensureCursorVisible();
    }
}


// Double click in res list: add selection to simple search
void ResList::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTextBrowser::mouseDoubleClickEvent(event);
    if (textCursor().hasSelection())
	emit(wordSelect(textCursor().selectedText()));
}

void ResList::linkWasClicked(const QUrl &url)
{
    QString s = url.toString();
    const char *ascurl = s.toAscii();
    LOGDEB(("ResList::linkWasClicked: [%s]\n", ascurl));
    int i = atoi(ascurl+1) - 1;
    int what = ascurl[0];
    switch (what) {
    case 'H': 
	emit headerClicked(); 
	break;
    case 'P': 
	emit docPreviewClicked(i, m_lstClckMod);
	break;
    case 'E': 
	emit docEditClicked(i);
	break;
    case 'n':
	resultPageNext();
	break;
    case 'p':
	resultPageBack();
	break;
    default: break;// ?? 
    }
}

void ResList::createPopupMenu(const QPoint& pos)
{
    LOGDEB(("ResList::createPopupMenu(%d, %d)\n", pos.x(), pos.y()));
    QTextCursor cursor = cursorForPosition(pos);
    int blocknum = cursor.blockNumber();
    LOGDEB(("ResList::createPopupMenu(): block %d\n", blocknum));
    m_popDoc = docnumfromparnum(blocknum);

    if (m_popDoc < 0) 
	return;
    QMenu *popup = new QMenu(this);
    popup->addAction(tr("&Preview"), this, SLOT(menuPreview()));
    popup->addAction(tr("&Open"), this, SLOT(menuEdit()));
    popup->addAction(tr("Copy &File Name"), this, SLOT(menuCopyFN()));
    popup->addAction(tr("Copy &URL"), this, SLOT(menuCopyURL()));
    Rcl::Doc doc;
    if (getDoc(m_popDoc, doc) && !doc.ipath.empty()) {
	popup->addAction(tr("&Write to File"), this, SLOT(menuSaveToFile()));
    }

    popup->addAction(tr("Find &similar documents"), this, SLOT(menuExpand()));
    popup->addAction(tr("Preview P&arent document/folder"), 
		      this, SLOT(menuPreviewParent()));
    popup->addAction(tr("&Open Parent document/folder"), 
		      this, SLOT(menuOpenParent()));
    popup->popup(mapToGlobal(pos));
}

void ResList::menuPreview()
{
    emit docPreviewClicked(m_popDoc, 0);
}
void ResList::menuSaveToFile()
{
    emit docSaveToFileClicked(m_popDoc);
}

void ResList::menuPreviewParent()
{
    Rcl::Doc doc;
    if (!getDoc(m_popDoc, doc) || m_baseDocSource.isNull()) 
	return;
    Rcl::Doc pdoc;
    if (m_baseDocSource->getEnclosing(doc, pdoc)) {
	emit previewRequested(pdoc);
    } else {
	// No parent doc: show enclosing folder with app configured for
	// directories
	pdoc.url = path_getfather(doc.url);
	pdoc.mimetype = "application/x-fsdirectory";
	emit editRequested(pdoc);
    }
}

void ResList::menuOpenParent()
{
    Rcl::Doc doc;
    if (!getDoc(m_popDoc, doc) || m_baseDocSource.isNull()) 
	return;
    Rcl::Doc pdoc;
    if (m_baseDocSource->getEnclosing(doc, pdoc)) {
	emit editRequested(pdoc);
    } else {
	// No parent doc: show enclosing folder with app configured for
	// directories
	pdoc.url = path_getfather(doc.url);
	pdoc.mimetype = "application/x-fsdirectory";
	emit editRequested(pdoc);
    }
}

void ResList::menuEdit()
{
    emit docEditClicked(m_popDoc);
}
void ResList::menuCopyFN()
{
    LOGDEB(("menuCopyFN\n"));
    Rcl::Doc doc;
    if (getDoc(m_popDoc, doc)) {
	LOGDEB(("menuCopyFN: Got doc, fn: [%s]\n", doc.url.c_str()));
	// Our urls currently always begin with "file://" 
        //
        // Problem: setText expects a QString. Passing a (const char*)
        // as we used to do causes an implicit conversion from
        // latin1. File are binary and the right approach would be no
        // conversion, but it's probably better (less worse...) to
        // make a "best effort" tentative and try to convert from the
        // locale's charset than accept the default conversion.
        QString qfn = QString::fromLocal8Bit(doc.url.c_str()+7);
	QApplication::clipboard()->setText(qfn, QClipboard::Selection);
	QApplication::clipboard()->setText(qfn, QClipboard::Clipboard);
    }
}
void ResList::menuCopyURL()
{
    Rcl::Doc doc;
    if (getDoc(m_popDoc, doc)) {
	string url =  url_encode(doc.url, 7);
	QApplication::clipboard()->setText(url.c_str(), 
					   QClipboard::Selection);
	QApplication::clipboard()->setText(url.c_str(), 
					   QClipboard::Clipboard);
    }
}

void ResList::menuExpand()
{
    emit docExpand(m_popDoc);
}

QString ResList::getDescription()
{
    return QString::fromUtf8(m_docSource->getDescription().c_str());
}

/** Show detailed expansion of a query */
void ResList::showQueryDetails()
{
    string oq = breakIntoLines(m_docSource->getDescription(), 100, 50);
    QString desc = tr("Query details") + ": " + QString::fromUtf8(oq.c_str());
    QMessageBox::information(this, tr("Query details"), desc);
}
