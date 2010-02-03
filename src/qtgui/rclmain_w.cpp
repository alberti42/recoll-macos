#ifndef lint
static char rcsid[] = "@(#$Id: rclmain_w.cpp,v 1.57 2008-10-13 07:57:12 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <qfiledialog.h>

#if (QT_VERSION < 0x040000)
#include <qcstring.h>
#include <qpopupmenu.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
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
#include <qapplication.h>
#include <qcursor.h>

#include "recoll.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "smallut.h"
#include "advsearch_w.h"
#include "sortseq.h"
#include "uiprefs_w.h"
#include "guiutils.h"
#include "reslist.h"
#include "transcode.h"
#include "refcntr.h"
#include "ssearch_w.h"
#include "execmd.h"
#include "internfile.h"
#include "docseqdb.h"
#include "docseqhist.h"
#include "confguiindex.h"
using namespace confgui;

#include "rclmain_w.h"
#include "rclhelp.h"
#include "moc_rclmain_w.cpp"

extern "C" int XFlush(void *);
QString g_stringAllStem, g_stringNoStem;

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
    // This is just to get the common catg strings into the message file
    static const char* catg_strings[] = {
            QT_TR_NOOP("All"), QT_TR_NOOP("media"),  QT_TR_NOOP("message"),
            QT_TR_NOOP("other"),  QT_TR_NOOP("presentation"),
            QT_TR_NOOP("spreadsheet"),  QT_TR_NOOP("text"), 
	    QT_TR_NOOP("sorted"), QT_TR_NOOP("filtered")
    };

    curPreview = 0;
    asearchform = 0;
    sortform = 0;
    uiprefs = 0;
    indexConfig = 0;
    spellform = 0;
    m_idxStatusAck = false;

    periodictimer = new QTimer(this);

    // At least some versions of qt4 don't display the status bar if
    // it's not created here.
    (void)statusBar();

    (void)new HelpClient(this);
    HelpClient::installMap(this->name(), "RCL.SEARCH.SIMPLE");

    // Set the focus to the search terms entry:
    sSearch->queryText->setFocus();

    // Set result list font according to user preferences.
    if (prefs.reslistfontfamily.length()) {
	QFont nfont(prefs.reslistfontfamily, prefs.reslistfontsize);
	resList->setFont(nfont);
    }

    // Stemming language menu
    g_stringNoStem = tr("(no stemming)");
    g_stringAllStem = tr("(all languages)");
    m_idNoStem = preferencesMenu->insertItem(g_stringNoStem);
    m_stemLangToId[g_stringNoStem] = m_idNoStem;
    m_idAllStem = preferencesMenu->insertItem(g_stringAllStem);
    m_stemLangToId[g_stringAllStem] = m_idAllStem;

    // Can't get the stemming languages from the db at this stage as
    // db not open yet (the case where it does not even exist makes
    // things complicated). So get the languages from the config
    // instead
    string slangs;
    list<string> langs;
    if (rclconfig->getConfParam("indexstemminglanguages", slangs)) {
	stringToStrings(slangs, langs);
    } else {
	QMessageBox::warning(0, "Recoll", 
			     tr("error retrieving stemming languages"));
    }
    int curid = prefs.queryStemLang == "ALL" ? m_idAllStem : m_idNoStem;
    int id; 
    for (list<string>::const_iterator it = langs.begin(); 
	 it != langs.end(); it++) {
	QString qlang = QString::fromAscii(it->c_str(), it->length());
	id = preferencesMenu->insertItem(qlang);
	m_stemLangToId[qlang] = id;
	if (prefs.queryStemLang == qlang) {
	    curid = id;
	}
    }
    preferencesMenu->setItemChecked(curid, true);

    // Toolbar+combobox version of the category selector
    QComboBox *catgCMB = 0;
    if (prefs.catgToolBar) {
        QToolBar *catgToolBar = new QToolBar(this);
	catgCMB = new QComboBox(FALSE, catgToolBar, "catCMB");
	catgCMB->insertItem(tr("All"));
#if (QT_VERSION >= 0x040000)
        catgToolBar->setObjectName(QString::fromUtf8("catgToolBar"));
	catgCMB->setToolTip(tr("Document category filter"));
        catgToolBar->addWidget(catgCMB);
        this->addToolBar(Qt::TopToolBarArea, catgToolBar);
#endif
    }

    // Document categories buttons
#if (QT_VERSION < 0x040000)
    catgBGRP->setColumnLayout(1, Qt::Vertical);
    connect(catgBGRP, SIGNAL(clicked(int)), this, SLOT(catgFilter(int)));
#else
    QHBoxLayout *bgrphbox = new QHBoxLayout(catgBGRP);
    QButtonGroup *bgrp  = new QButtonGroup(catgBGRP);
    bgrphbox->addWidget(allRDB);
    int bgrpid = 0;
    bgrp->addButton(allRDB, bgrpid++);
    connect(bgrp, SIGNAL(buttonClicked(int)), this, SLOT(catgFilter(int)));
#endif
    list<string> cats;
    rclconfig->getMimeCategories(cats);
    // Text for button 0 is not used. Next statement just avoids unused
    // variable compiler warning for catg_strings
    m_catgbutvec.push_back(catg_strings[0]);
    for (list<string>::const_iterator it = cats.begin();
	 it != cats.end(); it++) {
	QRadioButton *but = new QRadioButton(catgBGRP);
	QString catgnm = QString::fromUtf8(it->c_str(), it->length());
	m_catgbutvec.push_back(*it);
	but->setText(tr(catgnm));
	if (prefs.catgToolBar && catgCMB)
	    catgCMB->insertItem(tr(catgnm));
#if (QT_VERSION >= 0x040000)
        bgrphbox->addWidget(but);
        bgrp->addButton(but, bgrpid++);
#endif
    }
#if (QT_VERSION < 0x040000)
    catgBGRP->setButton(0);
#else
    catgBGRP->setLayout(bgrphbox);
#endif

    if (prefs.catgToolBar)
	catgBGRP->hide();
    // Connections
    connect(sSearch, SIGNAL(startSearch(RefCntr<Rcl::SearchData>)), 
		this, SLOT(startSearch(RefCntr<Rcl::SearchData>)));
#if QT_VERSION >= 0x040000
    sSearch->queryText->installEventFilter(this);
#else
    sSearch->queryText->lineEdit()->installEventFilter(this);
#endif

    connect(preferencesMenu, SIGNAL(activated(int)),
	    this, SLOT(setStemLang(int)));
    connect(preferencesMenu, SIGNAL(aboutToShow()),
	    this, SLOT(adjustPrefsMenu()));
    // signals and slots connections
    connect(sSearch, SIGNAL(clearSearch()), this, SLOT(resetSearch()));
    connect(firstPageAction, SIGNAL(activated()), 
	    resList, SLOT(resultPageFirst()));
    connect(prevPageAction, SIGNAL(activated()), 
	    resList, SLOT(resPageUpOrBack()));
    connect(nextPageAction, SIGNAL(activated()),
	    resList, SLOT(resPageDownOrNext()));

    connect(resList, SIGNAL(docExpand(int)), this, SLOT(docExpand(int)));
    connect(resList, SIGNAL(wordSelect(QString)),
	    this, SLOT(ssearchAddTerm(QString)));
    connect(resList, SIGNAL(nextPageAvailable(bool)), 
	    this, SLOT(enableNextPage(bool)));
    connect(resList, SIGNAL(prevPageAvailable(bool)), 
	    this, SLOT(enablePrevPage(bool)));
    connect(resList, SIGNAL(docEditClicked(int)), 
	    this, SLOT(startNativeViewer(int)));
    connect(resList, SIGNAL(docSaveToFileClicked(int)), 
	    this, SLOT(saveDocToFile(int)));
    connect(resList, SIGNAL(editRequested(Rcl::Doc)), 
	    this, SLOT(startNativeViewer(Rcl::Doc)));

    connect(resList, SIGNAL(docPreviewClicked(int, int)), 
	    this, SLOT(startPreview(int, int)));
    connect(resList, SIGNAL(previewRequested(Rcl::Doc)), 
	    this, SLOT(startPreview(Rcl::Doc)));

    connect(fileExitAction, SIGNAL(activated() ), this, SLOT(fileExit() ) );
    connect(fileToggleIndexingAction, SIGNAL(activated()), 
	    this, SLOT(toggleIndexing()));
    connect(fileEraseDocHistoryAction, SIGNAL(activated()), 
	    this, SLOT(eraseDocHistory()));
    connect(helpAbout_RecollAction, SIGNAL(activated()), 
	    this, SLOT(showAboutDialog()));
    connect(showMissingHelpers_Action, SIGNAL(activated()), 
	    this, SLOT(showMissingHelpers()));
    connect(userManualAction, SIGNAL(activated()), this, SLOT(startManual()));
    connect(toolsDoc_HistoryAction, SIGNAL(activated()), 
	    this, SLOT(showDocHistory()));
    connect(toolsAdvanced_SearchAction, SIGNAL(activated()), 
	    this, SLOT(showAdvSearchDialog()));
    connect(toolsSort_parametersAction, SIGNAL(activated()), 
	    this, SLOT(showSortDialog()));
    connect(toolsSpellAction, SIGNAL(activated()), 
	    this, SLOT(showSpellDialog()));

    connect(indexConfigAction, SIGNAL(activated()), this, SLOT(showIndexConfig()));
    connect(queryPrefsAction, SIGNAL(activated()), this, SLOT(showUIPrefs()));
    connect(extIdxAction, SIGNAL(activated()), this, SLOT(showExtIdxDialog()));
    if (prefs.catgToolBar && catgCMB)
	connect(catgCMB, SIGNAL(activated(int)), this, SLOT(catgFilter(int)));
    connect(toggleFullScreenAction, SIGNAL(activated()), 
            this, SLOT(toggleFullScreen()));
    connect(periodictimer, SIGNAL(timeout()), this, SLOT(periodic100()));
    // Start timer on a slow period (used for checking ^C). Will be
    // speeded up during indexing
    periodictimer->start(1000);

#if (QT_VERSION < 0x040000)
    nextPageAction->setIconSet(createIconSet("nextpage.png"));
    prevPageAction->setIconSet(createIconSet("prevpage.png"));
    firstPageAction->setIconSet(createIconSet("firstpage.png"));
    toolsSpellAction->setIconSet(QPixmap::fromMimeSource("spell.png"));
    toolsDoc_HistoryAction->setIconSet(QPixmap::fromMimeSource("history.png"));
    toolsAdvanced_SearchAction->setIconSet(QPixmap::fromMimeSource("asearch.png"));
    toolsSort_parametersAction->setIconSet(QPixmap::fromMimeSource("sortparms.png"));
#else
    toolsSpellAction->setIcon(QIcon(":/images/spell.png"));
    nextPageAction->setIcon(QIcon(":/images/nextpage.png"));
    prevPageAction->setIcon(QIcon(":/images/prevpage.png"));
    firstPageAction->setIcon(QIcon(":/images/firstpage.png"));
    toolsDoc_HistoryAction->setIcon(QIcon(":/images/history.png"));
    toolsAdvanced_SearchAction->setIcon(QIcon(":/images/asearch.png"));
    toolsSort_parametersAction->setIcon(QIcon(":/images/sortparms.png"));
#endif


    // If requested by prefs, restore sort state. The easiest way is to let
    // a SortForm do it for us.
    if (prefs.keepSort && prefs.sortActive) {
	SortForm sf(0);
	connect(&sf, SIGNAL(sortDataChanged(DocSeqSortSpec)), 
		resList, SLOT(setSortParams(DocSeqSortSpec)));
	// Have to call setdata again after sig connected...
	sf.setData();
    }
}

// This is called by a timer right after we come up. Try to open
// the database and talk to the user if we can't
void RclMain::initDbOpen()
{
    bool needindexconfig = false;
    bool nodb = false;
    string reason;
    if (!maybeOpenDb(reason)) {
        nodb = true;
	switch (QMessageBox::
#if (QT_VERSION >= 0x030200)
		question
#else
		information
#endif
		(this, "Recoll",
		 qApp->translate("Main", "Could not open database in ") +
		 QString::fromLocal8Bit(rclconfig->getDbDir().c_str()) +
		 qApp->translate("Main", 
			       ".\n"
			       "Click Cancel if you want to edit the configuration file before indexation starts, or Ok to let it proceed."),
		 "Ok", "Cancel", 0,   0)) {

	case 0: // Ok: indexing is going to start.
	    start_indexing(true);
	    break;

	case 1: // Cancel
	    needindexconfig = true;
	    break;
	}
    }

    if (needindexconfig) {
	startIndexingAfterConfig = 1;
	showIndexConfig();
    } else {
	if (prefs.startWithAdvSearchOpen)
	    showAdvSearchDialog();
	if (prefs.startWithSortToolOpen)
	    showSortDialog();
        // If we have something in the search entry, it comes from a
        // command line argument
        if (!nodb && sSearch->hasSearchString())
            QTimer::singleShot(0, sSearch, SLOT(startSimpleSearch()));
    }
}

void RclMain::setStemLang(int id)
{
    LOGDEB(("RclMain::setStemLang(%d)\n", id));
    // Check that the menu entry is for a stemming language change
    // (might also be "show prefs" etc.
    bool isLangId = false;
    for (map<QString, int>::const_iterator it = m_stemLangToId.begin();
	 it != m_stemLangToId.end(); it++) {
	if (id == it->second)
	    isLangId = true;
    }
    if (!isLangId)
	return;

    // Set the "checked" item state for lang entries
    for (map<QString, int>::const_iterator it = m_stemLangToId.begin();
	 it != m_stemLangToId.end(); it++) {
	preferencesMenu->setItemChecked(it->second, false);
    }
    preferencesMenu->setItemChecked(id, true);

    // Retrieve language value (also handle special cases), set prefs,
    // notify that we changed
    QString lang;
    if (id == m_idNoStem) {
	lang = "";
    } else if (id == m_idAllStem) {
	lang = "ALL";
    } else {
	lang = preferencesMenu->text(id);
    }
    prefs.queryStemLang = lang;
    LOGDEB(("RclMain::setStemLang(%d): lang [%s]\n", 
	    id, (const char *)prefs.queryStemLang.ascii()));
    rwSettings(true);
    emit stemLangChanged(lang);
}

// Set the checked stemming language item before showing the prefs menu
void RclMain::setStemLang(const QString& lang)
{
    LOGDEB(("RclMain::setStemLang(%s)\n", (const char *)lang.ascii()));
    int id;
    if (lang == "") {
	id = m_idNoStem;
    } else if (lang == "ALL") {
	id = m_idAllStem;
    } else {
	map<QString, int>::iterator it = m_stemLangToId.find(lang);
	if (it == m_stemLangToId.end()) 
	    return;
	id = it->second;
    }
    for (map<QString, int>::const_iterator it = m_stemLangToId.begin();
	 it != m_stemLangToId.end(); it++) {
	preferencesMenu->setItemChecked(it->second, false);
    }
    preferencesMenu->setItemChecked(id, true);
}

// Prefs menu about to show
void RclMain::adjustPrefsMenu()
{
    setStemLang(prefs.queryStemLang);
}

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
    // Don't save geometry if we're currently fullscreened
    if (!isFullScreen()) {
        prefs.mainwidth = width();
        prefs.mainheight = height();
    }
    prefs.ssearchTyp = sSearch->searchTypCMB->currentItem();
    if (asearchform)
	delete asearchform;
    // We'd prefer to do this in the exit handler, but it's apparently to late
    // and some of the qt stuff is already dead at this point?
    LOGDEB2(("RclMain::fileExit() : stopping idx thread\n"));
    stop_idxthread();
    // Let the exit handler clean up the rest (internal recoll stuff).
    exit(0);
}

// This is called by a periodic timer to check the status of the
// indexing thread and a possible need to exit
void RclMain::periodic100()
{
    static int toggle = 0;
    // Check if indexing thread done
    if (idxthread_getStatus() != IDXTS_NULL) {
	// Indexing is stopped
	fileToggleIndexingAction->setText(tr("Update &Index"));
	fileToggleIndexingAction->setEnabled(TRUE);
	if (m_idxStatusAck == false) {
	    m_idxStatusAck = true;
	    if (idxthread_getStatus() != IDXTS_OK) {
		if (idxthread_idxInterrupted()) {
		    QMessageBox::warning(0, "Recoll", 
					 tr("Indexing interrupted"));
		} else {
		    QMessageBox::warning(0, "Recoll", 
		       QString::fromAscii(idxthread_getReason().c_str()));
		}
	    }
	    // Make sure we reopen the db to get the results. If there
	    // is current search data, we should reset it else things
	    // are inconsistent (ie: applying sort will fail. But we
	    // don't like erasing results while the user may be
	    // looking at them either). Fixing this would be
	    // relatively complicated (keep an open/close gen number
	    // and check this / restart query in DocSeqDb() ?)
	    string reason;
	    maybeOpenDb(reason, 1);
            periodictimer->changeInterval(1000);
	}
    } else {
	// Indexing is running
	m_idxStatusAck = false;
	fileToggleIndexingAction->setText(tr("Stop &Indexing"));
	fileToggleIndexingAction->setEnabled(TRUE);
        periodictimer->changeInterval(100);
	// The toggle thing is for the status to flash
	if (toggle < 9) {
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
	    statusBar()->message(msg, 4000);
	} else if (toggle == 9) {
	    statusBar()->message("");
	}
	if (++toggle >= 10)
	    toggle = 0;
    }
    if (recollNeedsExit)
	fileExit();
}

// This gets called when the indexing action is activated. It starts
// the requested action, and disables the menu entry. This will be
// re-enabled by the indexing status check
void RclMain::toggleIndexing()
{
    if (idxthread_getStatus() == IDXTS_NULL) {
	// Indexing was in progress, stop it
	stop_indexing();
        periodictimer->changeInterval(1000);
	fileToggleIndexingAction->setText(tr("Update &Index"));
    } else {
	start_indexing(false);
        periodictimer->changeInterval(100);
	fileToggleIndexingAction->setText(tr("Stop &Indexing"));
    }
    fileToggleIndexingAction->setEnabled(FALSE);
}

// Start a db query and set the reslist docsource
void RclMain::startSearch(RefCntr<Rcl::SearchData> sdata)
{
    LOGDEB(("RclMain::startSearch. Indexing %s\n", 
	    idxthread_getStatus() == IDXTS_NULL?"on":"off"));
    // The db may have been closed at the end of indexing
    string reason;
    // If indexing is being performed, we reopen the db at each query.
    if (!maybeOpenDb(reason, idxthread_getStatus() == IDXTS_NULL)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	return;
    }

    resList->resetList();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    string stemLang = (const char *)prefs.queryStemLang.ascii();
    if (stemLang == "ALL") {
	rclconfig->getConfParam("indexstemminglanguages", stemLang);
    }
    sdata->setStemlang(stemLang);

    Rcl::Query *query = new Rcl::Query(rcldb);
    query->setCollapseDuplicates(prefs.collapseDuplicates);

    if (!query || !query->setQuery(sdata)) {
	QMessageBox::warning(0, "Recoll", tr("Can't start query: ") +
			     QString::fromAscii(query->getReason().c_str()));
	QApplication::restoreOverrideCursor();
	return;
    }
    curPreview = 0;
    DocSequenceDb *src = 
	new DocSequenceDb(RefCntr<Rcl::Query>(query), 
			  string(tr("Query results").utf8()), sdata);
    src->setAbstractParams(prefs.queryBuildAbstract, 
                           prefs.queryReplaceAbstract);

    resList->setDocSource(RefCntr<DocSequence>(src));
    QApplication::restoreOverrideCursor();
}

void RclMain::resetSearch()
{
    resList->resetList();
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
		resList, SLOT(setSortParams(DocSeqSortSpec)));
	connect(sortform, SIGNAL(applySortData()), 
		resList, SLOT(setDocSource()));
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

void RclMain::showIndexConfig()
{
    LOGDEB(("showIndexConfig()\n"));
    if (indexConfig == 0) {
	indexConfig = new ConfIndexW(0, rclconfig);
	LOGDEB(("showIndexConfig(): confindexW created\n"));
    } else {
	// Close and reopen, in hope that makes us visible...
	indexConfig->close();
    }
    indexConfig->show();
}

void RclMain::showUIPrefs()
{
    if (uiprefs == 0) {
	uiprefs = new UIPrefsDialog(0);
	connect(uiprefs, SIGNAL(uiprefsDone()), this, SLOT(setUIPrefs()));
	connect(this, SIGNAL(stemLangChanged(const QString&)), 
		uiprefs, SLOT(setStemLang(const QString&)));
    } else {
	// Close and reopen, in hope that makes us visible...
	uiprefs->close();
    }
    uiprefs->show();
}

void RclMain::showExtIdxDialog()
{
    if (uiprefs == 0) {
	uiprefs = new UIPrefsDialog(0);
	connect(uiprefs, SIGNAL(uiprefsDone()), this, SLOT(setUIPrefs()));
    } else {
	// Close and reopen, in hope that makes us visible...
	uiprefs->close();
    }
    uiprefs->tabWidget->setCurrentPage(2);
    uiprefs->show();
}

void RclMain::showAboutDialog()
{
    string vstring = Rcl::version_string() +
        string("<br> http://www.recoll.org") +
	string("<br> http://www.xapian.org");
    QMessageBox::information(this, tr("About Recoll"), vstring.c_str());
}
void RclMain::showMissingHelpers()
{
    string miss = rclconfig->getMissingHelperDesc();
    QString msg = tr("External applications/commands needed and not found "
		     "for indexing your file types:\n\n");
    if (!miss.empty()) {
	msg += QString::fromUtf8(miss.c_str());
    } else {
	msg += tr("No helpers found missing");
    }
    QMessageBox::information(this, tr("Missing helper programs"), msg);
}

// If a preview (toplevel) window gets closed by the user, we need to
// clean up because there is no way to reopen it. And check the case
// where the current one is closed
void RclMain::previewClosed(Preview *w)
{
    LOGDEB(("RclMain::previewClosed(%p)\n", w));
    if (w == curPreview) {
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
	QMessageBox::warning(0, "Recoll", tr("Cannot retrieve document info" 
					     " from database"));
	return;
    }
	
    if (mod & Qt::ShiftButton) {
	// User wants new preview window
	curPreview = 0;
    }
    if (curPreview == 0) {
	HiliteData hdata;
	resList->getTerms(hdata.terms, hdata.groups, hdata.gslks);
	curPreview = new Preview(resList->listId(), hdata);
	if (curPreview == 0) {
	    QMessageBox::warning(0, tr("Warning"), 
				 tr("Can't create preview window"),
				 QMessageBox::Ok, 
				 QMessageBox::NoButton);
	    return;
	}
	connect(curPreview, SIGNAL(previewClosed(Preview *)), 
		this, SLOT(previewClosed(Preview *)));
	connect(curPreview, SIGNAL(wordSelect(QString)),
		this, SLOT(ssearchAddTerm(QString)));
	connect(curPreview, SIGNAL(showNext(Preview *, int, int)),
		this, SLOT(previewNextInTab(Preview *, int, int)));
	connect(curPreview, SIGNAL(showPrev(Preview *, int, int)),
		this, SLOT(previewPrevInTab(Preview *, int, int)));
	connect(curPreview, SIGNAL(previewExposed(Preview *, int, int)),
		this, SLOT(previewExposed(Preview *, int, int)));
	curPreview->setCaption(resList->getDescription());
	curPreview->show();
    } 
    curPreview->makeDocCurrent(doc, docnum);
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
    Preview *preview = new Preview(0, HiliteData());
    if (preview == 0) {
	QMessageBox::warning(0, tr("Warning"), 
			     tr("Can't create preview window"),
			     QMessageBox::Ok, 
			     QMessageBox::NoButton);
	return;
    }
    connect(preview, SIGNAL(wordSelect(QString)),
	    this, SLOT(ssearchAddTerm(QString)));
    preview->show();
    preview->makeDocCurrent(doc, 0);
}

// Show next document from result list in current preview tab
void RclMain::previewNextInTab(Preview * w, int sid, int docnum)
{
    previewPrevOrNextInTab(w, sid, docnum, true);
}

// Show previous document from result list in current preview tab
void RclMain::previewPrevInTab(Preview * w, int sid, int docnum)
{
    previewPrevOrNextInTab(w, sid, docnum, false);
}

// Combined next/prev from result list in current preview tab
void RclMain::previewPrevOrNextInTab(Preview * w, int sid, int docnum, bool nxt)
{
    LOGDEB(("RclMain::previewNextInTab  sid %d docnum %d, listId %d\n", 
	    sid, docnum, resList->listId()));

    if (w == 0) // ??
	return;

    if (sid != resList->listId()) {
	QMessageBox::warning(0, "Recoll", 
			     tr("This search is not active any more"));
	return;
    }

    if (nxt)
	docnum++;
    else 
	docnum--;
    if (docnum < 0 || docnum >= resList->getResCnt()) {
	QApplication::beep();
	return;
    }

    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("Cannot retrieve document info from database"));
	return;
    }
	
    w->makeDocCurrent(doc, docnum, true);
}

// Preview tab exposed: if the preview comes from the currently
// displayed result list, tell reslist (to color the paragraph)
void RclMain::previewExposed(Preview *, int sid, int docnum)
{
    LOGDEB2(("RclMain::previewExposed: sid %d docnum %d, m_sid %d\n", 
	     sid, docnum, resList->listId()));
    if (sid != resList->listId()) {
	return;
    }
    resList->previewExposed(docnum);
}

// Add term to simple search. Term comes out of double-click in
// reslist or preview. 
// It would probably be better to cleanup in preview.ui.h and
// reslist.cpp and do the proper html stuff in the latter case
// (which is different because it format is explicit richtext
// instead of auto as for preview, needed because it's built by
// fragments?).
static const char* punct = " \t()<>\"'[]{}!^*.,:;\n\r";
void RclMain::ssearchAddTerm(QString term)
{
    LOGDEB(("RclMain::ssearchAddTerm: [%s]\n", (const char *)term.utf8()));
    string t = (const char *)term.utf8();
    string::size_type pos = t.find_last_not_of(punct);
    if (pos == string::npos)
	return;
    t = t.substr(0, pos+1);
    pos = t.find_first_not_of(punct);
    if (pos != string::npos)
	t = t.substr(pos);
    if (t.empty())
	return;
    term = QString::fromUtf8(t.c_str());

    QString text = sSearch->queryText->currentText();
    text += QString::fromLatin1(" ") + term;
    sSearch->queryText->setEditText(text);
}

void RclMain::saveDocToFile(int docnum)
{
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				" from database"));
	return;
    }
    QString s = 
	QFileDialog::getSaveFileName(path_home().c_str(),
				     "",  this,
				     tr("Save file dialog"),
				     tr("Choose a file name to save under"));
    string tofile((const char *)s.local8Bit());
    TempFile temp; // not used
    if (!FileInterner::idocToFile(temp, tofile, rclconfig, doc)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot extract document or create "
				"temporary file"));
	return;
    }
}

/* Look for html browser. We make a special effort for html because it's
 * used for reading help */
static bool lookForHtmlBrowser(string &exefile)
{
    static const char *htmlbrowserlist = 
	"opera konqueror firefox mozilla netscape epiphany";
    list<string> blist;
    stringToTokens(htmlbrowserlist, blist, " ");

    const char *path = getenv("PATH");
    if (path == 0)
	path = "/bin:/usr/bin:/usr/bin/X11:/usr/X11R6/bin:/usr/local/bin";

    // Look for each browser 
    for (list<string>::const_iterator bit = blist.begin(); 
	 bit != blist.end(); bit++) {
	if (ExecCmd::which(*bit, exefile, path)) 
	    return true;
    }
    exefile.clear();
    return false;
}

void RclMain::startNativeViewer(int docnum)
{
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc)) {
	QMessageBox::warning(0, "Recoll", tr("Cannot retrieve document info" 
					     " from database"));
	return;
    }
    startNativeViewer(doc);
}

// Convert to file path if url is like file://
static string fileurltolocalpath(string url)
{
    if (url.find("file://") == 0)
        return url.substr(7, string::npos);
    return string();
}

void RclMain::startNativeViewer(Rcl::Doc doc)
{
    // Look for appropriate viewer
    string cmdplusattr;
    if (prefs.useDesktopOpen) {
	cmdplusattr = rclconfig->getMimeViewerDef("application/x-all", "");
    } else {
        string apptag;
        map<string,string>::const_iterator it;
        if ((it = doc.meta.find(Rcl::Doc::keyapptg)) != doc.meta.end())
            apptag = it->second;
	cmdplusattr = rclconfig->getMimeViewerDef(doc.mimetype, apptag);
    }

    if (cmdplusattr.length() == 0) {
	QMessageBox::warning(0, "Recoll", 
			     tr("No external viewer configured for mime type [")
			     + doc.mimetype.c_str() + "]");
	return;
    }

    // Extract possible viewer attributes
    ConfSimple attrs;
    string cmd;
    rclconfig->valueSplitAttributes(cmdplusattr, cmd, attrs);
    bool ignoreipath = false;
    if (attrs.get("ignoreipath", cmdplusattr))
        ignoreipath = stringToBool(cmdplusattr);

    // Split the command line
    list<string> lcmd;
    if (!stringToStrings(cmd, lcmd)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("Bad viewer command line for %1: [%2]\n"
				"Please check the mimeconf file")
			     .arg(QString::fromAscii(doc.mimetype.c_str()))
			     .arg(QString::fromLocal8Bit(cmd.c_str())));
	return;
    }

    // Look for the command to execute in the exec path and a few
    // other places
    string cmdpath;
    if (prefs.useDesktopOpen) {
	// Findfilter searches the recoll filters directory in
	// addition to the path. We store a copy of xdg-open there, to be 
	// used as last resort
	cmdpath = rclconfig->findFilter(lcmd.front());
	// Substitute path for cmd
	if (!cmdpath.empty()) {
	    lcmd.front() = cmdpath;
	    cmd.erase();
	    stringsToString(lcmd, cmd);
	}
    } else {
	ExecCmd::which(lcmd.front(), cmdpath);
    }

    // Specialcase text/html because of the help browser need
    if (cmdpath.empty() && !doc.mimetype.compare("text/html")) {
	if (lookForHtmlBrowser(cmdpath)) {
	    cmd = cmdpath + " %u";
	}
    }

    // Command not found: start the user dialog to help find another one:
    if (cmdpath.empty()) {
	QString mt = QString::fromAscii(doc.mimetype.c_str());
	QString message = tr("The viewer specified in mimeview for %1: %2"
			     " is not found.\nDo you want to start the "
			     " preferences dialog ?")
	    .arg(mt).arg(QString::fromLocal8Bit(lcmd.front().c_str()));

	switch(QMessageBox::warning(0, "Recoll", message, 
				    "Yes", "No", 0, 0, 1)) {
	case 0: 
	    showUIPrefs();
	    if (uiprefs)
		uiprefs->showViewAction(mt);
	    break;
	case 1:
	    break;
	}
        // The user will have to click on the link again to try the
        // new command.
	return;
    }

    // We may need a temp file, or not depending on the command arguments
    // and the fact that this is a subdoc or not.
    bool wantsipath = (cmd.find("%i") != string::npos) || ignoreipath;
    bool wantsfile = cmd.find("%f") != string::npos;
    bool istempfile = false;
    string fn = fileurltolocalpath(doc.url);
    string url = doc.url;

    // If the command wants a file but this is not a file url, or
    // there is an ipath that it won't understand, we need a temp file:
    rclconfig->setKeyDir(path_getfather(fn));
    if ((wantsfile && fn.empty()) || (!wantsipath && !doc.ipath.empty())) {
	TempFile temp;
	if (!FileInterner::idocToFile(temp, string(), rclconfig, doc)) {
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
    string efftime;
    if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
        efftime = doc.dmtime.empty() ? doc.fmtime : doc.dmtime;
    } else {
        efftime = "0";
    }
    // Try to keep the letters used more or less consistent with the reslist
    // paragraph format.
    string ncmd;
    map<string, string> subs;
    subs["D"] = escapeShell(efftime);
    subs["f"] = escapeShell(fn);
    subs["i"] = escapeShell(doc.ipath);
    subs["M"] = escapeShell(doc.mimetype);
    subs["U"] = escapeShell(url);
    subs["u"] = escapeShell(url);
    // Let %(xx) access all metadata.
    for (map<string,string>::const_iterator it = doc.meta.begin();
         it != doc.meta.end(); it++) {
        subs[it->first] = escapeShell(it->second);
    }

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
	historyEnterDoc(g_dynconf, doc.meta[Rcl::Doc::keyudi]);
    // We should actually monitor these processes so that we can
    // delete the temp files when they exit
    LOGDEB(("Executing: [%s]\n", ncmd.c_str()));
    system(ncmd.c_str());
}

void RclMain::startManual()
{
    startManual(string());
}

void RclMain::startManual(const string& index)
{
    Rcl::Doc doc;
    doc.url = "file://";
    doc.url = path_cat(doc.url, rclconfig->getDatadir());
    doc.url = path_cat(doc.url, "doc");
    doc.url = path_cat(doc.url, "usermanual.html");
    LOGDEB(("RclMain::startManual: help index is %s\n", 
	    index.empty()?"(null)":index.c_str()));
    if (!index.empty()) {
	doc.url += "#";
	doc.url += index;
    }
    doc.mimetype = "text/html";
    startNativeViewer(doc);
}

// Search for document 'like' the selected one. We ask rcldb/xapian to find
// significant terms, and add them to the simple search entry.
void RclMain::docExpand(int docnum)
{
    if (!rcldb)
	return;
    Rcl::Doc doc;
    if (!resList->getDoc(docnum, doc))
	return;
    list<string> terms;
    terms = resList->expand(doc);
    if (terms.empty())
	return;
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
    resList->resetList();
    curPreview = 0;

    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	return;
    }
    // Construct a bogus SearchData structure
    RefCntr<Rcl::SearchData>searchdata = 
	RefCntr<Rcl::SearchData>(new Rcl::SearchData(Rcl::SCLT_AND));
    searchdata->setDescription((const char *)tr("History data").utf8());


    // If you change the title, also change it in eraseDocHistory()
    DocSequenceHistory *src = 
	new DocSequenceHistory(rcldb, g_dynconf, 
			       string(tr("Document history").utf8()));
    src->setDescription((const char *)tr("History data").utf8());
    resList->setDocSource(RefCntr<DocSequence>(src));
}

// Erase all memory of documents viewed
void RclMain::eraseDocHistory()
{
    // Clear file storage
    if (g_dynconf)
	g_dynconf->eraseAll(docHistSubKey);
    // Clear possibly displayed history
    if (resList->displayingHistory()) {
	showDocHistory();
    }
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
    firstPageAction->setEnabled(yesno);
}

// User pressed a category button: set filter params in reslist
void RclMain::catgFilter(int id)
{
    LOGDEB(("RclMain::catgFilter: id %d\n"));
    if (id < 0 || id >= int(m_catgbutvec.size()))
	return; 

    DocSeqFiltSpec spec;

    if (id != 0)  {
	string catg = m_catgbutvec[id];
	list<string> tps;
	rclconfig->getMimeCatTypes(catg, tps);
	for (list<string>::const_iterator it = tps.begin();
	     it != tps.end(); it++) 
	    spec.orCrit(DocSeqFiltSpec::DSFS_MIMETYPE, *it);
    }

    resList->setFilterParams(spec);
    resList->setDocSource();
}

void RclMain::toggleFullScreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

bool RclMain::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)  {
        LOGDEB2(("RclMain::eventFilter: keypress\n"));
	QKeyEvent *ke = (QKeyEvent *)event;
        // Shift-Home is the shortcut for the 1st result page action, but it is
        // filtered by the search entry to mean "select all line". We prefer to
        // keep it for the action as it's easy to find another combination to
        // select all (ie: home, then shift-end)
        if (ke->key() == Qt::Key_Home && (ke->state() & Qt::ShiftButton)) {
            // Shift-Home -> first page of results
            resList->resultPageFirst();
            return true;
        } 
    }
    return false;
}
