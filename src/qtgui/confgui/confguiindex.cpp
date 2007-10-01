#ifndef lint
static char rcsid[] = "@(#$Id: confguiindex.cpp,v 1.3 2007-10-01 06:35:31 dockes Exp $ (C) 2007 J.F.Dockes";
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
#else
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>

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
#endif
#include <qlayout.h>
#include QFRAME_INCLUDE
#include <qwidget.h>
#include <qlabel.h>
#include QLISTBOX_INCLUDE
#include <qtimer.h>

#include <list>
using std::list;

#include "confgui.h"
#include "confguiindex.h"
#include "smallut.h"
#include "debuglog.h"
#include "rcldb.h"
#include "conflinkrcl.h"
#include "execmd.h"

namespace confgui {
const static int spacing = 3;
const static int margin = 6;

ConfTopPanelW::ConfTopPanelW(QWidget *parent, RclConfig *config)
    : QWidget(parent)
{
    QVBOXLAYOUT *vboxLayout = new QVBOXLAYOUT(this);
    vboxLayout->setSpacing(spacing);
    vboxLayout->setMargin(margin);

    ConfLink lnktopdirs(new ConfLinkRclRep(config, "topdirs"));
    ConfParamDNLW *etopdirs = new 
	ConfParamDNLW(this, lnktopdirs, tr("Top directories"),
		      tr("The list of directories where recursive "
			 "indexing starts. Default: your home."));
    vboxLayout->addWidget(etopdirs);

    ConfLink lnkskp(new ConfLinkRclRep(config, "skippedPaths"));
    ConfParamSLW *eskp = new 
	ConfParamSLW(this, lnkskp, tr("List of skipped paths"),
		     tr("These are names of directories which indexing "
			"will not enter.<br> May contain wildcards. "
			"Must match "
			"the paths seen by the indexer (ie: if topdirs "
			"includes '/home/me' and '/home' is actually a link "
			"to '/usr/home', a correct skippedPath entry "
			"would be '/home/tmp*', not '/usr/home/tmp*')"));
    vboxLayout->addWidget(eskp);

    list<string> cstemlangs = Rcl::Db::getStemmerNames();
    QStringList stemlangs;
    for (list<string>::const_iterator it = cstemlangs.begin(); 
	 it != cstemlangs.end(); it++) {
	stemlangs.push_back(QString::fromUtf8(it->c_str()));
    }
    ConfLink lnkidxsl(new ConfLinkRclRep(config, "indexstemminglanguages"));
    ConfParamCSLW *eidxsl = new 
	ConfParamCSLW(this, lnkidxsl, tr("Index stemming languages"),
		      tr("The languages for which stemming expansion "
			 "dictionaries will be built."), stemlangs);
    vboxLayout->addWidget(eidxsl);

    ConfLink lnk4(new ConfLinkRclRep(config, "logfilename"));
    ConfParamFNW *e4 = new 
	ConfParamFNW(this, lnk4, tr("Log file name"),
		     tr("The file where the messages will be written. "
			"Use 'stderr' for terminal output"), false);
    vboxLayout->addWidget(e4);

    ConfLink lnk1(new ConfLinkRclRep(config, "loglevel"));
    ConfParamIntW *e1 = new 
	ConfParamIntW(this, lnk1, tr("Log verbosity level"),
		      tr("This value adjusts the amount of "
			 "messages, from only errors to a "
			 "lot of debugging data."), 0, 6);
    vboxLayout->addWidget(e1);

    ConfLink lnkidxflsh(new ConfLinkRclRep(config, "idxflushmb"));
    ConfParamIntW *eidxflsh = new 
	ConfParamIntW(this, lnkidxflsh, tr("Index flush megabytes interval"),
		      tr("This value adjust the amount of "
			 "data which is indexed betweeen flushes to disk.<br>"
			 "This helps control the indexer memory usage. "
			 "Default 10MB "), 0, 1000);
    vboxLayout->addWidget(eidxflsh);

    ConfLink lnkfsocc(new ConfLinkRclRep(config, "maxfsoccuppc"));
    ConfParamIntW *efsocc = new 
	ConfParamIntW(this, lnkfsocc, tr("Max disk occupation (%)"),
		      tr("This is the percentage of disk occupation where "
			 "indexing will fail and stop (to avoid filling up "
			 "your disk). <br>"
			 "0 means no limit (this is the default)."), 0, 100);
    vboxLayout->addWidget(efsocc);


    ConfLink lnknaspl(new ConfLinkRclRep(config, "noaspell"));
    ConfParamBoolW *enaspl = new 
	ConfParamBoolW(this, lnknaspl, tr("No aspell usage"),
		       tr("Disables use of aspell to generate spelling "
			  "approximation in the term explorer tool.<br> "
			  "Useful is aspell is absent or does not work. "));
    vboxLayout->addWidget(enaspl);

    ConfLink lnk2(new ConfLinkRclRep(config, "aspellLanguage"));
    ConfParamStrW *e2 = new 
	ConfParamStrW(this, lnk2, tr("Aspell language"),
		      tr("The language for the aspell dictionary. "
			 "This should look like 'en' or 'fr' ...<br>"
			 "If this value is not set, the NLS environment "
			 "will be used to compute it, which usually works."
			 "To get an idea of what is installed on your system, "
			 "type 'aspell config' and look for .dat files inside "
			 "the 'data-dir' directory. "));
    vboxLayout->addWidget(e2);

    ConfLink lnkdbd(new ConfLinkRclRep(config, "dbdir"));
    ConfParamFNW *edbd = new 
	ConfParamFNW(this, lnkdbd, tr("Database directory name"),
		     tr("The name for a directory where to store the index<br>"
			"A non-absolute path is taken relative to the "
			" configuration directory. The default is 'xapiandb'."
			), true);
    vboxLayout->addWidget(edbd);

    ConfLink lnkusfc(new ConfLinkRclRep(config, "usesystemfilecommand"));
    ConfParamBoolW *eusfc = new 
	ConfParamBoolW(this, lnkusfc, tr("Use system's 'file' command"),
		       tr("Use the system's 'file' command if internal "
			  "mime type identification fails."));
    vboxLayout->addWidget(eusfc);
}

ConfSubPanelW::ConfSubPanelW(QWidget *parent, RclConfig *config)
    : QWidget(parent), m_config(config)
{
    QVBOXLAYOUT *vboxLayout = new QVBOXLAYOUT(this);
    vboxLayout->setSpacing(spacing);
    vboxLayout->setMargin(margin);

    ConfLink lnksubkeydirs(new ConfLinkNullRep());
    m_subdirs = new 
	ConfParamDNLW(this, lnksubkeydirs, 
		      QObject::tr("Subdirectories with specific parameters"),
		      QObject::tr("The list of subdirectories in the indexed "
				  "hierarchy where some parameters need to be "
				  "customised. "
				  "Default: empty."));
    m_subdirs->getListBox()->setSelectionMode(QLISTBOX::Single);
    connect(m_subdirs->getListBox(), SIGNAL(selectionChanged()),
	    this, SLOT(subDirChanged()));
    connect(m_subdirs, SIGNAL(entryDeleted(QString)),
	    this, SLOT(subDirDeleted(QString)));
    list<string> allkeydirs = config->getKeyDirs(); 
    QStringList qls;
    for (list<string>::const_iterator it = allkeydirs.begin(); 
	 it != allkeydirs.end(); it++) {
	qls.push_back(QString::fromUtf8(it->c_str()));
    }
    m_subdirs->getListBox()->insertStringList(qls);

    vboxLayout->addWidget(m_subdirs);

    QLabel *explain = new QLabel(this);
    explain->setText(
		     QObject::
		     tr("<i>The following parameters are set either at the "
			"top level (no selection or empty line selected) "
			"or for the selected subdirectory. "
			"You can add / remove directories by clicking "
			"the +/- buttons."));
    vboxLayout->addWidget(explain);

    QFRAME *line2 = new QFRAME(this);
    line2->setFrameShape(QFRAME::HLine);
    line2->setFrameShadow(QFRAME::Sunken);
    vboxLayout->addWidget(line2);

    ConfLink lnkskn(new ConfLinkRclRep(config, "skippedNames", &m_sk));
    ConfParamSLW *eskn = new 
	ConfParamSLW(this, lnkskn, 
		     QObject::tr("List of skipped names"),
		     QObject::tr("These are patterns for file or directory "
				 " names which should not be indexed."));
    vboxLayout->addWidget(eskn);
    m_widgets.push_back(eskn);

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
	ConfParamCStrW(this, lnk21, 
		       QObject::tr("Default character set"),
		       QObject::tr("This is the character set used for reading files "
			  "which do not identify the character set "
			  "internally, for example pure text files.<br>"
			  "The default value is empty, "
			  "and the value from the NLS environnement is used."
			  ), charsets);
    vboxLayout->addWidget(e21);
    m_widgets.push_back(e21);

    ConfLink lnk3(new ConfLinkRclRep(config, "followLinks", &m_sk));
    ConfParamBoolW *e3 = new 
	ConfParamBoolW(this, lnk3, 
		        QObject::tr("Follow symbolic links"),
		        QObject::tr("Follow symbolic links while "
			  "indexing. The default is no, "
			  "to avoid duplicate indexing"));
    vboxLayout->addWidget(e3);
    m_widgets.push_back(e3);

    ConfLink lnkafln(new ConfLinkRclRep(config, "indexallfilenames", &m_sk));
    ConfParamBoolW *eafln = new 
	ConfParamBoolW(this, lnkafln, 
		       QObject::tr("Index all file names"),
		       QObject::tr("Index the names of files for which the contents "
			  "cannot be identified or processed (no or "
			  "unsupported mime type). Default true"));
    vboxLayout->addWidget(eafln);
    m_widgets.push_back(eafln);
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
    if (item == 0) {
	m_sk = "";
    } else {
	m_sk = (const char *)item->text().utf8();
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
    m_config->setKeyDir((const char *)sbd.utf8());
    m_config->eraseKeyDir();
}

void ConfSubPanelW::restoreEmpty()
{
    LOGDEB(("ConfSubPanelW::restoreEmpty()\n"));
    m_subdirs->getListBox()->insertItem("", 0);
}

} // Namespace confgui
