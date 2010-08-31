#ifndef lint
static char rcsid[] = "@(#$Id: confguiindex.cpp,v 1.13 2008-09-30 12:38:29 dockes Exp $ (C) 2007 J.F.Dockes";
#endif

#include <qglobal.h>
#if QT_VERSION < 0x040000
#define QFRAME_INCLUDE <qframe.h>
#define QFILEDIALOG_INCLUDE <qfiledialog.h>
#define QLISTBOX_INCLUDE <qlistbox.h>
#define QFILEDIALOG QFileDialog 
#define QFRAME QFrame
#define QHBOXLAYOUT QHBoxLayout
#define QLISTBOX QListBox
#define QLISTBOXITEM QListBoxItem
#define QLBEXACTMATCH Qt::ExactMatch
#define QVBOXLAYOUT QVBoxLayout
#define QGROUPBOX QGroupBox
#include <qgroupbox.h>
#else
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <Q3GroupBox>

#include <QFrame>
#define QFRAME_INCLUDE <q3frame.h>

#include <QFileDialog>
#define QFILEDIALOG_INCLUDE <q3filedialog.h>

#define QLISTBOX_INCLUDE <q3listbox.h>

#define QFILEDIALOG Q3FileDialog 
#define QFRAME Q3Frame
#define QHBOXLAYOUT Q3HBoxLayout
#define QLISTBOX Q3ListBox
#define QLISTBOXITEM Q3ListBoxItem
#define QLBEXACTMATCH Q3ListBox::ExactMatch
#define QVBOXLAYOUT Q3VBoxLayout
#define QGROUPBOX Q3GroupBox
#endif
#include <qlayout.h>
#include QFRAME_INCLUDE
#include <qwidget.h>
#include <qlabel.h>
#include QLISTBOX_INCLUDE
#include <qtimer.h>
#include <qmessagebox.h>
#include <qcheckbox.h>

#include <list>
using std::list;

#include "confgui.h"
#include "recoll.h"
#include "idxthread.h"
#include "confguiindex.h"
#include "smallut.h"
#include "debuglog.h"
#include "rcldb.h"
#include "conflinkrcl.h"
#include "execmd.h"
#include "rclconfig.h"

namespace confgui {
const static int spacing = 6;
const static int margin = 6;

ConfIndexW::ConfIndexW(QWidget *parent, RclConfig *config)
    : QTABDIALOG(parent), m_rclconf(config)
{
    setCaption(QString::fromLocal8Bit(config->getConfDir().c_str()));
    setOkButton();
    setCancelButton();

    reloadPanels();
    resize(QSize(600, 500).expandedTo(minimumSizeHint()));
    connect(this, SIGNAL(applyButtonPressed()), this, SLOT(acceptChanges()));
    connect(this, SIGNAL(cancelButtonPressed()), this, SLOT(rejectChanges()));
}

void ConfIndexW::acceptChanges()
{
    LOGDEB(("ConfIndexW::acceptChanges()\n"));
    if (!m_conf) {
	LOGERR(("ConfIndexW::acceptChanges: no config\n"));
	return;
    }
    // Let the changes to disk
    if (!m_conf->holdWrites(false)) {
	QMessageBox::critical(0, "Recoll",  
			      tr("Can't write configuration file"));
    }
    // Delete local copy
    delete m_conf;
    m_conf = 0;
    // Update in-memory config
    m_rclconf->updateMainConfig();

    QTimer::singleShot(0, this, SLOT(reloadPanels()));
    if (startIndexingAfterConfig) {
	startIndexingAfterConfig = 0;
	start_indexing(true);
    }
}

void ConfIndexW::rejectChanges()
{
    LOGDEB(("ConfIndexW::rejectChanges()\n"));
    // Discard local changes, and make new copy
    delete m_conf;
    m_conf = 0;
    QTimer::singleShot(0, this, SLOT(reloadPanels()));
}

void ConfIndexW::reloadPanels()
{
    if ((m_conf = m_rclconf->cloneMainConfig()) == 0) 
	return;
    m_conf->holdWrites(true);
    for (list<QWidget *>::iterator it = m_widgets.begin();
	 it != m_widgets.end(); it++) {
	removePage(*it);
	delete *it;
    }
    m_widgets.clear();

    QWidget *w = new ConfTopPanelW(this, m_conf);
    m_widgets.push_back(w);
    addTab(w, QObject::tr("Global parameters"));
	
    w = new ConfSubPanelW(this, m_conf);
    m_widgets.push_back(w);
    addTab(w, QObject::tr("Local parameters"));

    w = new ConfBeaglePanelW(this, m_conf);
    m_widgets.push_back(w);
    addTab(w, QObject::tr("Beagle web history"));

}

ConfBeaglePanelW::ConfBeaglePanelW(QWidget *parent, ConfNull *config)
    : QWidget(parent)
{
    QVBOXLAYOUT *vboxLayout = new QVBOXLAYOUT(this);
    vboxLayout->setSpacing(spacing);
    vboxLayout->setMargin(margin);

    ConfLink lnk1(new ConfLinkRclRep(config, "processbeaglequeue"));
    ConfParamBoolW* cp1 = 
        new ConfParamBoolW(this, lnk1, tr("Steal Beagle indexing queue"),
                           tr("Beagle MUST NOT be running. Enables processing "
                          "the beagle queue to index Firefox web history.<br>"
                          "(you should also install the Firefox Beagle plugin)"
			  ));
    vboxLayout->addWidget(cp1);

    ConfLink lnk2(new ConfLinkRclRep(config, "webcachedir"));
    ConfParamFNW* cp2 = 
        new ConfParamFNW(this, lnk2, tr("Web cache directory name"),
		     tr("The name for a directory where to store the cache "
			"for visited web pages.<br>"
                        "A non-absolute path is taken relative to the "
			"configuration directory."), true);
    cp2->setEnabled(cp1->m_cb->isOn());
    connect(cp1->m_cb, SIGNAL(toggled(bool)), cp2, SLOT(setEnabled(bool)));
    vboxLayout->addWidget(cp2);

    ConfLink lnk3(new ConfLinkRclRep(config, "webcachemaxmbs"));
    ConfParamIntW *cp3 =
        new ConfParamIntW(this, lnk3, tr("Max. size for the web cache (MB)"),
		      tr("Entries will be recycled once the size is reached"),
                          -1, 1000);
    cp3->setEnabled(cp1->m_cb->isOn());
    connect(cp1->m_cb, SIGNAL(toggled(bool)), cp3, SLOT(setEnabled(bool)));
    vboxLayout->addWidget(cp3);
    vboxLayout->insertStretch(-1);
}

ConfTopPanelW::ConfTopPanelW(QWidget *parent, ConfNull *config)
    : QWidget(parent)
{
    QGridLayout *gl1 = new QGridLayout(this);
    gl1->setSpacing(spacing);
    gl1->setMargin(margin);

    ConfLink lnktopdirs(new ConfLinkRclRep(config, "topdirs"));
    ConfParamDNLW *etopdirs = new 
	ConfParamDNLW(this, lnktopdirs, tr("Top directories"),
		      tr("The list of directories where recursive "
			 "indexing starts. Default: your home."));
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(etopdirs, 0, 0, 0, 1);
#else
    gl1->addWidget(etopdirs, 0, 0, 1, 2);
#endif

    ConfLink lnkskp(new ConfLinkRclRep(config, "skippedPaths"));
    ConfParamSLW *eskp = new 
	ConfParamSLW(this, lnkskp, tr("Skipped paths"),
		     tr("These are names of directories which indexing "
			"will not enter.<br> May contain wildcards. "
			"Must match "
			"the paths seen by the indexer (ie: if topdirs "
			"includes '/home/me' and '/home' is actually a link "
			"to '/usr/home', a correct skippedPath entry "
			"would be '/home/me/tmp*', not '/usr/home/me/tmp*')"));
    eskp->setFsEncoding(true);
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(eskp, 1, 1, 0, 1);
#else
    gl1->addWidget(eskp, 1, 0, 1, 2);
#endif

    list<string> cstemlangs = Rcl::Db::getStemmerNames();
    QStringList stemlangs;
    for (list<string>::const_iterator it = cstemlangs.begin(); 
	 it != cstemlangs.end(); it++) {
	stemlangs.push_back(QString::fromUtf8(it->c_str()));
    }
    ConfLink lnkidxsl(new ConfLinkRclRep(config, "indexstemminglanguages"));
    ConfParamCSLW *eidxsl = new 
	ConfParamCSLW(this, lnkidxsl, tr("Stemming languages"),
		      tr("The languages for which stemming expansion<br>"
			 "dictionaries will be built."), stemlangs);
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(eidxsl, 2, 2, 0, 1);
#else
    gl1->addWidget(eidxsl, 2, 0, 1, 2);
#endif

    ConfLink lnk4(new ConfLinkRclRep(config, "logfilename"));
    ConfParamFNW *e4 = new 
	ConfParamFNW(this, lnk4, tr("Log file name"),
		     tr("The file where the messages will be written.<br>"
			"Use 'stderr' for terminal output"), false);
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(e4, 3, 3, 0, 1);
#else
    gl1->addWidget(e4, 3, 0, 1, 2);
#endif

    QWidget *w = 0;

    ConfLink lnk1(new ConfLinkRclRep(config, "loglevel"));
    w = new ConfParamIntW(this, lnk1, tr("Log verbosity level"),
		      tr("This value adjusts the amount of "
			 "messages,<br>from only errors to a "
			 "lot of debugging data."), 0, 6);
    gl1->addWidget(w, 4, 0);

    ConfLink lnkidxflsh(new ConfLinkRclRep(config, "idxflushmb"));
    w = new ConfParamIntW(this, lnkidxflsh, 
                          tr("Index flush megabytes interval"),
                          tr("This value adjust the amount of "
			 "data which is indexed between flushes to disk.<br>"
			 "This helps control the indexer memory usage. "
			 "Default 10MB "), 0, 1000);
    gl1->addWidget(w, 4, 1);

    ConfLink lnkfsocc(new ConfLinkRclRep(config, "maxfsoccuppc"));
    w = new ConfParamIntW(this, lnkfsocc, tr("Max disk occupation (%)"),
		      tr("This is the percentage of disk occupation where "
			 "indexing will fail and stop (to avoid filling up "
			 "your disk).<br>"
			 "0 means no limit (this is the default)."), 0, 100);
    gl1->addWidget(w, 5, 0);

    ConfLink lnkusfc(new ConfLinkRclRep(config, "usesystemfilecommand"));
    w = new ConfParamBoolW(this, lnkusfc, tr("Use system's 'file' command"),
		       tr("Use the system's 'file' command if internal<br>"
			  "mime type identification fails."));
    gl1->addWidget(w, 5, 1);
    
    ConfLink lnknaspl(new ConfLinkRclRep(config, "noaspell"));
    ConfParamBoolW* cpasp =
    new ConfParamBoolW(this, lnknaspl, tr("No aspell usage"),
		       tr("Disables use of aspell to generate spelling "
			  "approximation in the term explorer tool.<br> "
			  "Useful if aspell is absent or does not work. "));
    gl1->addWidget(cpasp, 6, 0);

    ConfLink lnk2(new ConfLinkRclRep(config, "aspellLanguage"));
    ConfParamStrW* cpaspl =
        new ConfParamStrW(this, lnk2, tr("Aspell language"),
		      tr("The language for the aspell dictionary. "
			 "This should look like 'en' or 'fr' ...<br>"
			 "If this value is not set, the NLS environment "
			 "will be used to compute it, which usually works."
			 "To get an idea of what is installed on your system, "
			 "type 'aspell config' and look for .dat files inside "
			 "the 'data-dir' directory. "));
    cpaspl->setEnabled(!cpasp->m_cb->isOn());
    connect(cpasp->m_cb, SIGNAL(toggled(bool)), cpaspl,SLOT(setDisabled(bool)));
    gl1->addWidget(cpaspl, 6, 1);

    ConfLink lnkdbd(new ConfLinkRclRep(config, "dbdir"));
    ConfParamFNW *edbd = new 
	ConfParamFNW(this, lnkdbd, tr("Database directory name"),
		     tr("The name for a directory where to store the index<br>"
			"A non-absolute path is taken relative to the "
			" configuration directory. The default is 'xapiandb'."
			), true);
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(edbd, 7, 7, 0, 1);
#else
    gl1->addWidget(edbd, 7, 0, 1, 2);
#endif
}

ConfSubPanelW::ConfSubPanelW(QWidget *parent, ConfNull *config)
    : QWidget(parent), m_config(config)
{
    QVBOXLAYOUT *vboxLayout = new QVBOXLAYOUT(this);
    vboxLayout->setSpacing(spacing);
    vboxLayout->setMargin(margin);

    ConfLink lnksubkeydirs(new ConfLinkNullRep());
    m_subdirs = new 
	ConfParamDNLW(this, lnksubkeydirs, 
		      QObject::tr("<b>Customised subtrees"),
		      QObject::tr("The list of subdirectories in the indexed "
				  "hierarchy <br>where some parameters need "
				  "to be redefined. Default: empty."));
    m_subdirs->getListBox()->setSelectionMode(QLISTBOX::Single);
    connect(m_subdirs->getListBox(), SIGNAL(selectionChanged()),
	    this, SLOT(subDirChanged()));
    connect(m_subdirs, SIGNAL(entryDeleted(QString)),
	    this, SLOT(subDirDeleted(QString)));
    list<string> allkeydirs = config->getSubKeys(); 
    QStringList qls;
    for (list<string>::const_iterator it = allkeydirs.begin(); 
	 it != allkeydirs.end(); it++) {
	qls.push_back(QString::fromUtf8(it->c_str()));
    }
    m_subdirs->getListBox()->insertStringList(qls);
    vboxLayout->addWidget(m_subdirs);

    QFRAME *line2 = new QFRAME(this);
    line2->setFrameShape(QFRAME::HLine);
    line2->setFrameShadow(QFRAME::Sunken);
    vboxLayout->addWidget(line2);

    QLabel *explain = new QLabel(this);
    explain->setText(
		     QObject::
		     tr("<i>The parameters that follow are set either at the "
			"top level, if nothing<br>"
			"or an empty line is selected in the listbox above, "
			"or for the selected subdirectory.<br>"
			"You can add or remove directories by clicking "
			"the +/- buttons."));
    vboxLayout->addWidget(explain);


    m_groupbox = new QGROUPBOX(1, Qt::Horizontal, this);
    m_groupbox->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
					  QSizePolicy::Preferred,
					  1,  // Horizontal stretch
					  3,  // Vertical stretch
			    m_groupbox->sizePolicy().hasHeightForWidth()));

    QWidget *w = new QWidget(m_groupbox);
    QGridLayout *gl1 = new QGridLayout(w);
    gl1->setSpacing(spacing);
    gl1->setMargin(margin);

    ConfLink lnkskn(new ConfLinkRclRep(config, "skippedNames", &m_sk));
    ConfParamSLW *eskn = new 
	ConfParamSLW(w, lnkskn, 
		     QObject::tr("Skipped names"),
		     QObject::tr("These are patterns for file or directory "
				 " names which should not be indexed."));
    eskn->setFsEncoding(true);
    m_widgets.push_back(eskn);
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(eskn, 0, 0, 0, 1);
#else
    gl1->addWidget(eskn, 0, 0, 1, 2);
#endif

    list<string> args;
    args.push_back("-l");
    ExecCmd ex;
    string icout;
    string cmd = "iconv";
    int status = ex.doexec(cmd, args, 0, &icout);
    if (status) {
	LOGERR(("Can't get list of charsets from 'iconv -l'"));
    }
    icout = neutchars(icout, ",");
    list<string> ccsets;
    stringToStrings(icout, ccsets);
    QStringList charsets;
    charsets.push_back("");
    for (list<string>::const_iterator it = ccsets.begin(); 
	 it != ccsets.end(); it++) {
	charsets.push_back(QString::fromUtf8(it->c_str()));
    }

    ConfLink lnk21(new ConfLinkRclRep(config, "defaultcharset", &m_sk));
    ConfParamCStrW *e21 = new 
	ConfParamCStrW(w, lnk21, 
		       QObject::tr("Default character set"),
		       QObject::tr("This is the character set used for reading files "
			  "which do not identify the character set "
			  "internally, for example pure text files.<br>"
			  "The default value is empty, "
			  "and the value from the NLS environnement is used."
			  ), charsets);
    m_widgets.push_back(e21);
#if QT_VERSION < 0x040000
    gl1->addMultiCellWidget(e21, 1, 1, 0, 1);
#else
    gl1->addWidget(e21, 1, 0, 1, 2);
#endif

    ConfLink lnk3(new ConfLinkRclRep(config, "followLinks", &m_sk));
    ConfParamBoolW *e3 = new 
	ConfParamBoolW(w, lnk3, 
		        QObject::tr("Follow symbolic links"),
		        QObject::tr("Follow symbolic links while "
			  "indexing. The default is no, "
			  "to avoid duplicate indexing"));
    m_widgets.push_back(e3);
    gl1->addWidget(e3, 2, 0);

    ConfLink lnkafln(new ConfLinkRclRep(config, "indexallfilenames", &m_sk));
    ConfParamBoolW *eafln = new 
	ConfParamBoolW(w, lnkafln, 
		       QObject::tr("Index all file names"),
		       QObject::tr("Index the names of files for which the contents "
			  "cannot be identified or processed (no or "
			  "unsupported mime type). Default true"));
    m_widgets.push_back(eafln);
    gl1->addWidget(eafln, 2, 1);

    ConfLink lnkzfmaxkbs(new ConfLinkRclRep(config, "compressedfilemaxkbs"));
    ConfParamIntW *ezfmaxkbs = new 
	ConfParamIntW(w, lnkzfmaxkbs, 
		      tr("Max. compressed file size (KB)"),
		      tr("This value sets a threshold beyond which compressed"
			 "files will not be processed. Set to -1 for no "
			 "limit, to 0 for no decompression ever."),
		      -1, 1000000, -1);
    m_widgets.push_back(ezfmaxkbs);
    gl1->addWidget(ezfmaxkbs, 3, 0);

    ConfLink lnktxtmaxmbs(new ConfLinkRclRep(config, "textfilemaxmbs"));
    ConfParamIntW *etxtmaxmbs = new 
	ConfParamIntW(w, lnktxtmaxmbs, 
		      tr("Max. text file size (MB)"),
		      tr("This value sets a threshold beyond which text "
			 "files will not be processed. Set to -1 for no "
			 "limit. \nThis is for excluding monster "
                         "log files from the index."),
		      -1, 1000000);
    m_widgets.push_back(etxtmaxmbs);
    gl1->addWidget(etxtmaxmbs, 3, 1);

    ConfLink lnktxtpagekbs(new ConfLinkRclRep(config, "textfilepagekbs"));
    ConfParamIntW *etxtpagekbs = new 
	ConfParamIntW(w, lnktxtpagekbs, 
		      tr("Text file page size (KB)"),
		      tr("If this value is set (not equal to -1), text "
                         "files will be split in chunks of this size for "
                         "indexing.\nThis will help searching very big text "
                         " files (ie: log files)."),
		      -1, 1000000);
    m_widgets.push_back(etxtpagekbs);
    gl1->addWidget(etxtpagekbs, 4, 0);

    ConfLink lnkfiltmaxsecs(new ConfLinkRclRep(config, "filtermaxseconds"));
    ConfParamIntW *efiltmaxsecs = new 
	ConfParamIntW(w, lnkfiltmaxsecs, 
		      tr("Max. filter exec. time (S)"),
		      tr("External filters working longer than this will be "
                         "aborted. This is for the rare case (ie: postscript) "
                         "where a document could cause a filter to loop"
			 "Set to -1 for no limit.\n"),
		      -1, 10000);
    m_widgets.push_back(efiltmaxsecs);
    gl1->addWidget(efiltmaxsecs, 4, 1);

    vboxLayout->addWidget(m_groupbox);
    subDirChanged();
}

void ConfSubPanelW::reloadAll()
{
    for (list<ConfParamW*>::iterator it = m_widgets.begin();
	 it != m_widgets.end(); it++) {
	(*it)->loadValue();
    }
}

void ConfSubPanelW::subDirChanged()
{
    LOGDEB(("ConfSubPanelW::subDirChanged\n"));
    QLISTBOXITEM *item = m_subdirs->getListBox()->selectedItem();
    if (item == 0 || item->text() == "") {
	m_sk = "";
	m_groupbox->setTitle(tr("Global"));
    } else {
	m_sk = (const char *)item->text().utf8();
	m_groupbox->setTitle(item->text());
    }
    LOGDEB(("ConfSubPanelW::subDirChanged: now [%s]\n", m_sk.c_str()));
    reloadAll();
}

void ConfSubPanelW::subDirDeleted(QString sbd)
{
    LOGDEB(("ConfSubPanelW::subDirDeleted(%s)\n", (const char *)sbd.utf8()));
    if (sbd == "") {
	// Can't do this, have to reinsert it
	QTimer::singleShot(0, this, SLOT(restoreEmpty()));
	return;
    }
    // Have to delete all entries for submap
    m_config->eraseKey((const char *)sbd.utf8());
}

void ConfSubPanelW::restoreEmpty()
{
    LOGDEB(("ConfSubPanelW::restoreEmpty()\n"));
    m_subdirs->getListBox()->insertItem("", 0);
}

} // Namespace confgui
