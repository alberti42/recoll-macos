#ifndef lint
static char rcsid[] = "@(#$Id: rclmain_w.cpp,v 1.19 2007-01-08 07:02:25 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "autoconfig.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>

#include <utility>
#ifndef NO_NAMESPACES
using std::pair;
#endif /* NO_NAMESPACES */

#include <qapplication.h>
#include <qmessagebox.h>
#if (QT_VERSION < 0x040000)
#include <qcstring.h>
#endif
#include <qtabwidget.h>
#include <qtimer.h>
#include <qstatusbar.h>
#include <qwindowdefs.h>
#include <qcheckbox.h>
#include <qfontdialog.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qtextbrowser.h>
#include <qlineedit.h>
#include <qaction.h>
#include <qpushbutton.h>
#include <qimage.h>
#include <qiconset.h>

#include "recoll.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "smallut.h"
#include "advsearch_w.h"
#include "rclversion.h"
#include "sortseq.h"
#include "uiprefs_w.h"
#include "guiutils.h"
#include "reslist.h"
#include "transcode.h"
#include "refcntr.h"
#include "ssearch_w.h"
#include "execmd.h"
#include "internfile.h"

#include "rclmain_w.h"
#include "moc_rclmain_w.cpp"

extern "C" int XFlush(void *);

// Taken from qt designer. Don't know why it's needed.
#if (QT_VERSION < 0x040000)
static QIconSet createIconSet(const QString &name)
{
    QIconSet ic(QPixmap::fromMimeSource(name));
        QString iname = "d_" + name;
        ic.setPixmap(QPixmap::fromMimeSource(iname),
    		 QIconSet::Small, QIconSet::Disabled );
    return ic;
}
#endif

void RclMain::init()
{

    curPreview = 0;
    asearchform = 0;
    sortform = 0;
    uiprefs = 0;
    spellform = 0;
    m_searchId = 0;
    // Set the focus to the search terms entry:
    sSearch->queryText->setFocus();

    // Set result list font according to user preferences.
    if (prefs.reslistfontfamily.length()) {
	QFont nfont(prefs.reslistfontfamily, prefs.reslistfontsize);
	resList->setFont(nfont);
    }
    connect(sSearch, SIGNAL(startSearch(RefCntr<Rcl::SearchData>)), 
		this, SLOT(startSearch(RefCntr<Rcl::SearchData>)));

    // signals and slots connections
    connect(sSearch, SIGNAL(clearSearch()),
	    this, SLOT(resetSearch()));
    connect(prevPageAction, SIGNAL(activated()), 
	    resList, SLOT(resultPageBack()));
    connect(nextPageAction, SIGNAL(activated()),
	    resList, SLOT(resultPageNext()));

    connect(resList, SIGNAL(docExpand(int)), this, SLOT(docExpand(int)));
    connect(resList, SIGNAL(wordSelect(QString)),
	    this, SLOT(ssearchAddTerm(QString)));
    connect(resList, SIGNAL(nextPageAvailable(bool)), 
	    this, SLOT(enableNextPage(bool)));
    connect(resList, SIGNAL(prevPageAvailable(bool)), 
	    this, SLOT(enablePrevPage(bool)));
    connect(resList, SIGNAL(docEditClicked(int)), 
	    this, SLOT(startNativeViewer(int)));
    connect(resList, SIGNAL(docPreviewClicked(int, int)), 
	    this, SLOT(startPreview(int, int)));

    connect(resList, SIGNAL(previewRequested(Rcl::Doc)), 
	    this, SLOT(startPreview(Rcl::Doc)));

    connect(fileExitAction, SIGNAL(activated() ), this, SLOT(fileExit() ) );
    connect(fileStart_IndexingAction, SIGNAL(activated()), 
	    this, SLOT(startIndexing()));
    connect(helpAbout_RecollAction, SIGNAL(activated()), 
	    this, SLOT(showAboutDialog()));
    connect(userManualAction, SIGNAL(activated()), this, SLOT(startManual()));
    connect(toolsDoc_HistoryAction, SIGNAL(activated()), 
	    this, SLOT(showDocHistory()));
    connect(toolsAdvanced_SearchAction, SIGNAL(activated()), 
	    this, SLOT(showAdvSearchDialog()));
    connect(toolsSort_parametersAction, SIGNAL(activated()), 
	    this, SLOT(showSortDialog()));
#ifdef RCL_USE_ASPELL
    connect(toolsSpellAction, SIGNAL(activated()), 
	    this, SLOT(showSpellDialog()));
#else
    toolsSpellAction->setEnabled(FALSE);
#endif

    connect(preferencesQuery_PrefsAction, SIGNAL(activated()), 
	    this, SLOT(showUIPrefs()));


#if (QT_VERSION < 0x040000)
    nextPageAction->setIconSet(createIconSet("nextpage.png"));
    prevPageAction->setIconSet(createIconSet("prevpage.png"));
    toolsSpellAction->setIconSet(QPixmap("images/spell.png"));
    toolsDoc_HistoryAction->setIconSet(QPixmap("images/history.png"));
    toolsAdvanced_SearchAction->setIconSet(QPixmap("images/asearch.png"));
    toolsSort_parametersAction->setIconSet(QPixmap("images/sortparms.png"));
#else
    toolsSpellAction->setIcon(QIcon(":/images/spell.png"));
    nextPageAction->setIcon(QIcon(":/images/nextpage.png"));
    prevPageAction->setIcon(QIcon(":/images/prevpage.png"));
    toolsDoc_HistoryAction->setIcon(QIcon(":/images/history.png"));
    toolsAdvanced_SearchAction->setIcon(QIcon(":/images/asearch.png"));
    toolsSort_parametersAction->setIcon(QIcon(":/images/sortparms.png"));
#endif
}


//#define SHOWEVENTS
#if defined(SHOWEVENTS)
static const char *eventTypeToStr(int tp)
{
    switch (tp) {
    case 0: return "None";
    case 1: return "Timer";
    case 2: return "MouseButtonPress";
    case 3: return "MouseButtonRelease";
    case 4: return "MouseButtonDblClick";
    case 5: return "MouseMove";
    case 6: return "KeyPress";
    case 7: return "KeyRelease";
    case 8: return "FocusIn";
    case 9: return "FocusOut";
    case 10: return "Enter";
    case 11: return "Leave";
    case 12: return "Paint";
    case 13: return "Move";
    case 14: return "Resize";
    case 15: return "Create";
    case 16: return "Destroy";
    case 17: return "Show";
    case 18: return "Hide";
    case 19: return "Close";
    case 20: return "Quit";
    case 21: return "Reparent";
    case 22: return "ShowMinimized";
    case 23: return "ShowNormal";
    case 24: return "WindowActivate";
    case 25: return "WindowDeactivate";
    case 26: return "ShowToParent";
    case 27: return "HideToParent";
    case 28: return "ShowMaximized";
    case 29: return "ShowFullScreen";
    case 30: return "Accel";
    case 31: return "Wheel";
    case 32: return "AccelAvailable";
    case 33: return "CaptionChange";
    case 34: return "IconChange";
    case 35: return "ParentFontChange";
    case 36: return "ApplicationFontChange";
    case 37: return "ParentPaletteChange";
    case 38: return "ApplicationPaletteChange";
    case 39: return "PaletteChange";
    case 40: return "Clipboard";
    case 42: return "Speech";
    case 50: return "SockAct";
    case 51: return "AccelOverride";
    case 52: return "DeferredDelete";
    case 60: return "DragEnter";
    case 61: return "DragMove";
    case 62: return "DragLeave";
    case 63: return "Drop";
    case 64: return "DragResponse";
    case 70: return "ChildInserted";
    case 71: return "ChildRemoved";
    case 72: return "LayoutHint";
    case 73: return "ShowWindowRequest";
    case 74: return "WindowBlocked";
    case 75: return "WindowUnblocked";
    case 80: return "ActivateControl";
    case 81: return "DeactivateControl";
    case 82: return "ContextMenu";
    case 83: return "IMStart";
    case 84: return "IMCompose";
    case 85: return "IMEnd";
    case 86: return "Accessibility";
    case 87: return "TabletMove";
    case 88: return "LocaleChange";
    case 89: return "LanguageChange";
    case 90: return "LayoutDirectionChange";
    case 91: return "Style";
    case 92: return "TabletPress";
    case 93: return "TabletRelease";
    case 94: return "OkRequest";
    case 95: return "HelpRequest";
    case 96: return "WindowStateChange";
    case 97: return "IconDrag";
    case 1000: return "User";
    case 65535: return "MaxUser";
    default: return "Unknown";
    }
}
#endif

void RclMain::closeEvent( QCloseEvent * )
{
    fileExit();
}

// We also want to get rid of the advanced search form and previews
// when we exit (not our children so that it's not systematically
// created over the main form). 
bool RclMain::close()
{
    LOGDEB(("RclMain::close\n"));
    fileExit();
    return false;
}

void RclMain::fileExit()
{
    LOGDEB(("RclMain: fileExit\n"));
    m_tempfiles.clear();
    prefs.mainwidth = width();
    prefs.mainheight = height();
    prefs.ssearchTyp = sSearch->searchTypCMB->currentItem();
    if (asearchform)
	delete asearchform;
    // Let the exit handler clean up things
    exit(0);
}

// This is called on a 100ms timer checks the status of the indexing
// thread and a possible need to exit
void RclMain::periodic100()
{
    static int toggle = 0;
    // Check if indexing thread done
    if (indexingstatus != IDXTS_NULL) {
	statusBar()->message("");
	if (indexingstatus != IDXTS_OK) {
	    indexingstatus = IDXTS_NULL;
	    QMessageBox::warning(0, "Recoll", 
				 QString::fromAscii(indexingReason.c_str()));
	}
	indexingstatus = IDXTS_NULL;
	fileStart_IndexingAction->setEnabled(TRUE);
	// Make sure we reopen the db to get the results.
	LOGINFO(("Indexing done: closing query database\n"));
	rcldb->close();
    } else if (indexingdone == 0) {
	if (toggle == 0) {
	    QString msg = tr("Indexing in progress: ");
	    DbIxStatus status = idxthread_idxStatus();
	    QString phs;
	    switch (status.phase) {
	    case DbIxStatus::DBIXS_FILES: phs=tr("Files");break;
	    case DbIxStatus::DBIXS_PURGE: phs=tr("Purge");break;
	    case DbIxStatus::DBIXS_STEMDB: phs=tr("Stemdb");break;
	    case DbIxStatus::DBIXS_CLOSING:phs=tr("Closing");break;
	    default: phs=tr("Unknown");break;
	    }
	    msg += phs + " ";
	    if (status.phase == DbIxStatus::DBIXS_FILES) {
		char cnts[100];
		if (status.dbtotdocs>0)
		    sprintf(cnts,"(%d/%d) ",status.docsdone, status.dbtotdocs);
		else
		    sprintf(cnts, "(%d) ", status.docsdone);
		msg += QString::fromAscii(cnts) + " ";
	    }
	    string mf;int ecnt = 0;
	    string fcharset = rclconfig->getDefCharset(true);
	    if (!transcode(status.fn, mf, fcharset, "UTF-8", &ecnt) || ecnt) {
		mf = url_encode(status.fn, 0);
	    }
	    msg += QString::fromUtf8(mf.c_str());
	    statusBar()->message(msg);
	} else if (toggle == 9) {
	    statusBar()->message("");
	}
	if (++toggle >= 10)
	    toggle = 0;
    }
    if (recollNeedsExit)
	fileExit();
}

void RclMain::startIndexing()
{
    if (indexingdone)
	startindexing = 1;
    fileStart_IndexingAction->setEnabled(FALSE);
}

// Note that all our 'urls' are like : file://...
static string urltolocalpath(string url)
{
    return url.substr(7, string::npos);
}

// Start a db query and set the reslist docsource
void RclMain::startSearch(RefCntr<Rcl::SearchData> sdata)
{
    LOGDEB(("RclMain::startSearch\n"));
    // The db may have been closed at the end of indexing
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	exit(1);
    }

    resList->resetSearch();

    int qopts = 0;
    if (!prefs.queryStemLang.length() == 0)
	qopts |= Rcl::Db::QO_STEM;

    if (!rcldb->setQuery(sdata, qopts, prefs.queryStemLang.ascii())) {
	QMessageBox::warning(0, "Recoll", tr("Cant start query: ") +
			     QString::fromAscii(rcldb->getReason().c_str()));
	return;
    }
    curPreview = 0;
    m_searchData = sdata;
    m_docSource = RefCntr<DocSequence>(new DocSequenceDb(rcldb, string(tr("Query results").utf8())));
    setDocSequence();
}

void RclMain::resetSearch()
{
    resList->resetSearch();
    m_searchData = RefCntr<Rcl::SearchData>();
}

void RclMain::setDocSequence()
{
    if (m_searchData.getcnt() == 0)
	return;
    RefCntr<DocSequence> docsource;
    if (m_sortspecs.sortwidth > 0) {
	docsource = RefCntr<DocSequence>(new DocSeqSorted(m_docSource, 
							  m_sortspecs,
			  string(tr("Query results (sorted)").utf8())));
    } else {
	docsource = m_docSource;
    }
    m_searchId++;
    resList->setDocSource(docsource, m_searchData);
}

// Open advanced search dialog.
void RclMain::showAdvSearchDialog()
{
    if (asearchform == 0) {
	asearchform = new AdvSearch(0);
	connect(asearchform, SIGNAL(startSearch(RefCntr<Rcl::SearchData>)), 
		this, SLOT(startSearch(RefCntr<Rcl::SearchData>)));
	asearchform->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	asearchform->close();
	asearchform->show();
    }
}

void RclMain::showSortDialog()
{
    if (sortform == 0) {
	sortform = new SortForm(0);
	connect(sortform, SIGNAL(sortDataChanged(DocSeqSortSpec)), 
		this, SLOT(sortDataChanged(DocSeqSortSpec)));
	connect(sortform, SIGNAL(applySortData()), 
		this, SLOT(setDocSequence()));
	sortform->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	sortform->close();
        sortform->show();
    }

}

void RclMain::showSpellDialog()
{
    if (spellform == 0) {
	spellform = new SpellW(0);
	connect(spellform, SIGNAL(wordSelect(QString)),
		this, SLOT(ssearchAddTerm(QString)));
	spellform->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	spellform->close();
        spellform->show();
    }

}

void RclMain::showUIPrefs()
{
    if (uiprefs == 0) {
	uiprefs = new UIPrefsDialog(0);
	connect(uiprefs, SIGNAL(uiprefsDone()), this, SLOT(setUIPrefs()));
	uiprefs->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	uiprefs->close();
        uiprefs->show();
    }
}

// If a preview (toplevel) window gets closed by the user, we need to
// clean up because there is no way to reopen it. And check the case
// where the current one is closed
void RclMain::previewClosed(QWidget *w)
{
    if (w == (QWidget *)curPreview) {
	LOGDEB(("Active preview closed\n"));
	curPreview = 0;
    } else {
	LOGDEB(("Old preview closed\n"));
    }
    delete w;
}

/** 
 * Open a preview window for a given document, or load it into new tab of 
 * existing window.
 *
 * @param docnum db query index
 * @param mod keyboards modifiers like ControlButton, ShiftButton
 */
void RclMain::startPreview(int docnum, int mod)
{
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				     " from database"));
	return;
    }
	
    // Check file exists in file system
    string fn = urltolocalpath(doc.url);
    struct stat st;
    if (stat(fn.c_str(), &st) < 0) {
	QMessageBox::warning(0, "Recoll", tr("Cannot access document file: ") +
			     fn.c_str());
	return;
    }
    if (mod & Qt::ShiftButton) {
	// User wants new preview window
	curPreview = 0;
    }
    if (curPreview == 0) {
	curPreview = new Preview(0);
	if (curPreview == 0) {
	    QMessageBox::warning(0, tr("Warning"), 
				 tr("Can't create preview window"),
				 QMessageBox::Ok, 
				 QMessageBox::NoButton);
	    return;
	}
	curPreview->setSId(m_searchId, resList->getSearchData());
	curPreview->setCaption(resList->getDescription());
	connect(curPreview, SIGNAL(previewClosed(QWidget *)), 
		this, SLOT(previewClosed(QWidget *)));
	connect(curPreview, SIGNAL(wordSelect(QString)),
		this, SLOT(ssearchAddTerm(QString)));
	connect(curPreview, SIGNAL(showNext(int, int)),
		this, SLOT(previewNextInTab(int, int)));
	connect(curPreview, SIGNAL(showPrev(int, int)),
		this, SLOT(previewPrevInTab(int, int)));
	connect(curPreview, SIGNAL(previewExposed(int, int)),
		this, SLOT(previewExposed(int, int)));
	curPreview->show();
    } else {
	if (curPreview->makeDocCurrent(fn, doc)) {
	    // Already there
	    return;
	}
	(void)curPreview->addEditorTab();
    }
    // Enter document in document history
    g_dynconf->enterDoc(fn, doc.ipath);
    if (!curPreview->loadFileInCurrentTab(fn, st.st_size, doc, docnum))
	curPreview->closeCurrentTab();
}

/** 
 * Open a preview window for a given document, no linking to result list
 *
 * This is used to show ie parent documents, which have no corresponding
 * entry in the result list.
 * 
 */
void RclMain::startPreview(Rcl::Doc doc)
{
    // Check file exists in file system
    string fn = urltolocalpath(doc.url);
    struct stat st;
    if (stat(fn.c_str(), &st) < 0) {
	QMessageBox::warning(0, "Recoll", tr("Cannot access document file: ") +
			     fn.c_str());
	return;
    }
    Preview *preview = new Preview(0);
    if (preview == 0) {
	QMessageBox::warning(0, tr("Warning"), 
			     tr("Can't create preview window"),
			     QMessageBox::Ok, 
			     QMessageBox::NoButton);
	return;
    }
    RefCntr<Rcl::SearchData> searchdata(new Rcl::SearchData(Rcl::SCLT_AND));
    preview->setSId(0, searchdata);
    connect(preview, SIGNAL(wordSelect(QString)),
	    this, SLOT(ssearchAddTerm(QString)));
    g_dynconf->enterDoc(fn, doc.ipath);
    preview->show();
    if (!preview->loadFileInCurrentTab(fn, st.st_size, doc, 0))
	preview->closeCurrentTab();
}

// Show next document from result list in current preview tab
void RclMain::previewNextInTab(int sid, int docnum)
{
    LOGDEB(("RclMain::previewNextInTab  sid %d docnum %d, m_sid %d\n", 
	    sid, docnum, m_searchId));

    if (sid != m_searchId) {
	QMessageBox::warning(0, "Recoll", 
			     tr("This search is not active any more"));
	return;
    }

    docnum++;
    if (docnum >= resList->getResCnt()) {
	QApplication::beep();
	return;
    }

    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				     " from database"));
	return;
    }
	
    // Check that file exists in file system
    string fn = urltolocalpath(doc.url);
    struct stat st;
    if (stat(fn.c_str(), &st) < 0) {
	QMessageBox::warning(0, "Recoll", tr("Cannot access document file: ") +
			     fn.c_str());
	return;
    }

    if (!curPreview->loadFileInCurrentTab(fn, st.st_size, doc, docnum))
	curPreview->closeCurrentTab();
}

// Show previous document from result list in current preview tab
void RclMain::previewPrevInTab(int sid, int docnum)
{
    LOGDEB(("RclMain::previewPrevInTab  sid %d docnum %d, m_sid %d\n", 
	    sid, docnum, m_searchId));

    if (sid != m_searchId) {
	QMessageBox::warning(0, "Recoll", 
			     tr("This search is not active any more"));
	return;
    }
    if (docnum <= 0) {
	QApplication::beep();
	return;
    }
    docnum--;
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				     " from database"));
	return;
    }
	
    // Check that the file exists in the file system
    string fn = urltolocalpath(doc.url);
    struct stat st;
    if (stat(fn.c_str(), &st) < 0) {
	QMessageBox::warning(0, "Recoll", tr("Cannot access document file: ") +
			     fn.c_str());
	return;
    }
    if (!curPreview->loadFileInCurrentTab(fn, st.st_size, doc, docnum))
	curPreview->closeCurrentTab();
}

// Preview tab exposed: possibly tell reslist (to color the paragraph)
void RclMain::previewExposed(int sid, int docnum)
{
    LOGDEB2(("RclMain::previewExposed: sid %d docnum %d, m_sid %d\n", 
	     sid, docnum, m_searchId));

    if (sid != m_searchId) {
	return;
    }
    resList->previewExposed(docnum);
}

// Add term to simple search. Term comes out of double-click in
// reslist or preview. The cleanup code is really horrible. Selection
// out of the reslist will typically look like 
// <!-- Fragment -->sometext
// It would probably be better to cleanup in preview.ui.h and
// reslist.cpp and do the proper html stuff in the latter case
// (which is different because it's format is explicit richtext
// instead of auto as for preview, needed because it's built by
// fragments?).
static const char* punct = " \t()<>\"'[]{}!^*,\n\r";
void RclMain::ssearchAddTerm(QString term)
{
    string t = (const char *)term.utf8();
    string::size_type pos = t.find_last_not_of(punct);
    if (pos == string::npos)
	return;
    t = t.substr(0, pos+1);
    pos = t.find_last_of(punct);
    if (pos != string::npos)
	t = t.substr(pos+1);
    if (t.empty())
	return;
    term = QString::fromUtf8(t.c_str());

    QString text = sSearch->queryText->currentText();
    text += QString::fromLatin1(" ") + term;
    sSearch->queryText->setEditText(text);
}

void RclMain::startNativeViewer(int docnum)
{
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				" from database"));
	return;
    }
    
    // Look for appropriate viewer
    string cmd = rclconfig->getMimeViewerDef(doc.mimetype);
    if (cmd.length() == 0) {
	QMessageBox::warning(0, "Recoll", 
			     tr("No external viewer configured for mime type ")
			     + doc.mimetype.c_str());
	return;
    }
    list<string> lcmd;
    if (!stringToStrings(cmd, lcmd)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("Bad viewer command line for %1: [%2]\n"
				"Please check the mimeconf file")
			     .arg(QString::fromAscii(doc.mimetype.c_str()))
			     .arg(QString::fromLocal8Bit(cmd.c_str())));
	return;
    }
    string cmdpath;
    if (!ExecCmd::which(lcmd.front(), cmdpath)) {
	QString message = tr("The viewer specified in mimeconf for %1: %2"
			     " is not found.\nDo you want to start the "
			     " preferences dialog ?")
	    .arg(QString::fromAscii(doc.mimetype.c_str()))
	    .arg(QString::fromLocal8Bit(lcmd.front().c_str()));

	switch(QMessageBox::warning(0, "Recoll", message, 
				    "Yes", "No", 0, 0, 1)) {
	case 0: showUIPrefs();break;
	case 1:
	    
	    return;
	}
    }

    // For files with an ipath, we do things differently depending if the 
    // configured command seems to be able to grok it or not.
    bool wantsipath = cmd.find("%i") != string::npos;
    bool istempfile = false;
    string fn, url;
    if (doc.ipath.empty() || wantsipath) {
	fn = urltolocalpath(doc.url);
	url = url_encode(doc.url, 7);
    } else {
	// There is an ipath and the command does not know about
	// them. We need a temp file.
	TempFile temp;
	if (!FileInterner::idocTempFile(temp, rclconfig, 
					urltolocalpath(doc.url), 
					doc.ipath, doc.mimetype)) {
	    QMessageBox::warning(0, "Recoll",
				 tr("Cannot extract document or create "
				    "temporary file"));
	    return;
	}
	istempfile = true;
	m_tempfiles.push_back(temp);
	fn = temp->filename();
	url = string("file://") + fn;
    }

    // Substitute %xx inside prototype command
    string ncmd;
    map<char, string> subs;
    subs['u'] = escapeShell(url);
    subs['f'] = escapeShell(fn);
    subs['i'] = escapeShell(doc.ipath);
    pcSubst(cmd, ncmd, subs);

    ncmd += " &";
    QStatusBar *stb = statusBar();
    if (stb) {
	string fcharset = rclconfig->getDefCharset(true);
	string prcmd;
	transcode(ncmd, prcmd, fcharset, "UTF-8");
	QString msg = tr("Executing: [") + 
	    QString::fromUtf8(prcmd.c_str()) + "]";
	stb->message(msg, 5000);
    }
    if (!istempfile)
	g_dynconf->enterDoc(fn, doc.ipath);
    // We should actually monitor these processes so that we can
    // delete the temp files when they exit
    system(ncmd.c_str());
}


void RclMain::showAboutDialog()
{
    string vstring = string("Recoll ") + rclversion + 
	"<br>" + "http://www.recoll.org";
    QMessageBox::information(this, tr("About Recoll"), vstring.c_str());
}

void RclMain::startManual()
{
    QString msg = tr("Starting help browser ");
    if (prefs.htmlBrowser != QString(""))
	msg += prefs.htmlBrowser;
    statusBar()->message(msg, 3000);
    startHelpBrowser();
}

// Search for document 'like' the selected one.
void RclMain::docExpand(int docnum)
{
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc))
	return;
    list<string> terms;
    terms = rcldb->expand(doc);
    // Do we keep the original query. I think we'd better not.
    // rcldb->expand is set to keep the original query terms instead.
    QString text;// = sSearch->queryText->currentText();
    for (list<string>::iterator it = terms.begin(); it != terms.end(); it++) {
	text += QString::fromLatin1(" \"") +
	    QString::fromUtf8((*it).c_str()) + QString::fromLatin1("\"");
    }
    // We need to insert item here, its not auto-done like when the user types
    // CR
    sSearch->queryText->setEditText(text);
    sSearch->setAnyTermMode();
    sSearch->startSimpleSearch();
}

void RclMain::showDocHistory()
{
    LOGDEB(("RclMain::showDocHistory\n"));
    resList->resetSearch();
    curPreview = 0;

    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	exit(1);
    }
    // Construct a bogus SearchData structure
    m_searchData = 
	RefCntr<Rcl::SearchData>(new Rcl::SearchData(Rcl::SCLT_AND));
    m_searchData->setDescription((const char *)tr("History data").utf8());

    m_searchId++;

    m_docSource = RefCntr<DocSequence>(new DocSequenceHistory(rcldb, 
							      g_dynconf, 
			      string(tr("Document history").utf8())));
    setDocSequence();
}


void RclMain::sortDataChanged(DocSeqSortSpec spec)
{
    LOGDEB(("RclMain::sortDataChanged\n"));
    m_sortspecs = spec;
}

// Called when the uiprefs dialog is ok'd
void RclMain::setUIPrefs()
{
    if (!uiprefs)
	return;
    LOGDEB(("Recollmain::setUIPrefs\n"));
    if (prefs.reslistfontfamily.length()) {
	QFont nfont(prefs.reslistfontfamily, prefs.reslistfontsize);
	resList->setFont(nfont);
    } else {
	resList->setFont(this->font());
    }
}

void RclMain::enableNextPage(bool yesno)
{
    nextPageAction->setEnabled(yesno);
}

void RclMain::enablePrevPage(bool yesno)
{
    prevPageAction->setEnabled(yesno);
}

