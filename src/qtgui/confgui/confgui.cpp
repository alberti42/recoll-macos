#ifndef lint
static char rcsid[] = "@(#$Id: confgui.cpp,v 1.9 2008-05-21 07:21:37 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>
#include <stdlib.h>

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

#include <qobject.h>
#include <qlayout.h>
#include <qsize.h>
#include <qsizepolicy.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include QFILEDIALOG_INCLUDE
#include <qinputdialog.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include QLISTBOX_INCLUDE
#include <qcombobox.h>
#include QFRAME_INCLUDE

#include "confgui.h"
#include "smallut.h"
#include "debuglog.h"
#include "rcldb.h"

#include <list>
using std::list;

namespace confgui {

const static int spacing = 4;
const static int margin = 6;

void ConfParamW::setValue(const QString& value)
{
    if (m_fsencoding)
        m_cflink->set(string((const char *)value.local8Bit()));
    else
        m_cflink->set(string((const char *)value.utf8()));
}

void ConfParamW::setValue(int value)
{
    char buf[30];
    sprintf(buf, "%d", value);
    m_cflink->set(string(buf));
}
void ConfParamW::setValue(bool value)
{
    char buf[30];
    sprintf(buf, "%d", value);
    m_cflink->set(string(buf));
}

bool ConfParamW::createCommon(const QString& lbltxt, const QString& tltptxt)
{
    m_hl = new QHBOXLAYOUT(this);
    m_hl->setSpacing(spacing);

    QLabel *tl = new QLabel(this);
    tl->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  tl->sizePolicy().hasHeightForWidth() ) );
    tl->setText(lbltxt);
    QToolTip::add(tl, tltptxt);

    m_hl->addWidget(tl);

    return true;
}

ConfParamIntW::ConfParamIntW(QWidget *parent, ConfLink cflink, 
			     const QString& lbltxt,
			     const QString& tltptxt,
			     int minvalue, 
			     int maxvalue)
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;

    m_sb = new QSpinBox(this);
    m_sb->setMinValue(minvalue);
    m_sb->setMaxValue(maxvalue);
    m_sb->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
				    QSizePolicy::Fixed,
				    0,  // Horizontal stretch
				    0,  // Vertical stretch
				    m_sb->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(m_sb);

    QFRAME *fr = new QFRAME(this);
    fr->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
				  QSizePolicy::Fixed,
				  1,  // Horizontal stretch
				  0,  // Vertical stretch
				  fr->sizePolicy().hasHeightForWidth() ) );
    m_hl->addWidget(fr);

    loadValue();
    QObject::connect(m_sb, SIGNAL(valueChanged(int)), 
		     this, SLOT(setValue(int)));
}

void ConfParamIntW::loadValue()
{
    string s;
    m_cflink->get(s);
    m_sb->setValue(atoi(s.c_str()));
}

ConfParamStrW::ConfParamStrW(QWidget *parent, ConfLink cflink, 
			     const QString& lbltxt,
			     const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;
    m_le = new QLineEdit(this);
    m_le->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				    QSizePolicy::Fixed,
				    1,  // Horizontal stretch
				    0,  // Vertical stretch
				    m_le->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(m_le);

    loadValue();
    QObject::connect(m_le, SIGNAL(textChanged(const QString&)), 
		     this, SLOT(setValue(const QString&)));
}

void ConfParamStrW::loadValue()
{
    string s;
    m_cflink->get(s);
    if (m_fsencoding)
        m_le->setText(QString::fromLocal8Bit(s.c_str()));
    else
        m_le->setText(QString::fromUtf8(s.c_str()));
}

ConfParamCStrW::ConfParamCStrW(QWidget *parent, ConfLink cflink, 
			       const QString& lbltxt,
			       const QString& tltptxt,
			       const QStringList &sl
			       )
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;
    m_cmb = new QComboBox(false, this);
    m_cmb->insertStringList(sl);
    //    m_cmb->setEditable(false);
    m_cmb->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				     QSizePolicy::Fixed,
				     1,  // Horizontal stretch
				     0,  // Vertical stretch
				     m_cmb->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(m_cmb);

    loadValue();
    QObject::connect(m_cmb, SIGNAL(activated(const QString&)), 
		     this, SLOT(setValue(const QString&)));
}

void ConfParamCStrW::loadValue()
{
    string s;
    m_cflink->get(s);
    QString cs;
    if (m_fsencoding)
        cs = QString::fromLocal8Bit(s.c_str());
    else
        cs = QString::fromUtf8(s.c_str());
        
    for (int i = 0; i < m_cmb->count(); i++) {
	if (!cs.compare(m_cmb->text(i))) {
	    m_cmb->setCurrentItem(i);
	    break;
	}
    }
}

ConfParamBoolW::ConfParamBoolW(QWidget *parent, ConfLink cflink, 
			       const QString& lbltxt,
			       const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    // No createCommon because the checkbox has a label
    m_hl = new QHBOXLAYOUT(this);
    m_hl->setSpacing(spacing);

    m_cb = new QCheckBox(lbltxt, this);
    m_cb->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  m_cb->sizePolicy().hasHeightForWidth()));
    QToolTip::add(m_cb, tltptxt);
    m_hl->addWidget(m_cb);

    QFRAME *fr = new QFRAME(this);
    fr->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
				  QSizePolicy::Fixed,
				  1,  // Horizontal stretch
				  0,  // Vertical stretch
				  fr->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(fr);

    loadValue();
    QObject::connect(m_cb, SIGNAL(toggled(bool)), 
		     this, SLOT(setValue(bool)));
}

void ConfParamBoolW::loadValue()
{
    string s;
    m_cflink->get(s);
    m_cb->setChecked(stringToBool(s));
}

ConfParamFNW::ConfParamFNW(QWidget *parent, ConfLink cflink, 
			   const QString& lbltxt,
			   const QString& tltptxt,
			   bool isdir
			   )
    : ConfParamW(parent, cflink), m_isdir(isdir)
{
    if (!createCommon(lbltxt, tltptxt))
	return;

    m_fsencoding = true;
    m_le = new QLineEdit(this);
    m_le->setMinimumSize(QSize(150, 0 ));
    m_le->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				    QSizePolicy::Fixed,
				    1,  // Horizontal stretch
				    0,  // Vertical stretch
				    m_le->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(m_le);

    m_pb = new QPushButton(this);
    m_pb->setText(tr("Browse"));
    m_pb->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
                                    QSizePolicy::Fixed,
                                    0,  // Horizontal stretch
                                    0,  // Vertical stretch
                                    m_pb->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(m_pb);

    loadValue();
    QObject::connect(m_pb, SIGNAL(clicked()), this, SLOT(showBrowserDialog()));
    QObject::connect(m_le, SIGNAL(textChanged(const QString&)), 
		     this, SLOT(setValue(const QString&)));
}

void ConfParamFNW::loadValue()
{
    string s;
    m_cflink->get(s);
    m_le->setText(QString::fromLocal8Bit(s.c_str()));
}

void ConfParamFNW::showBrowserDialog()
{
    QString s = m_isdir ?  
	QFILEDIALOG::getExistingDirectory() : QFILEDIALOG::getSaveFileName();
    if (!s.isEmpty()) 
	m_le->setText(s);
}

ConfParamSLW::ConfParamSLW(QWidget *parent, ConfLink cflink, 
			   const QString& lbltxt,
			   const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    // Can't use createCommon here cause we want the buttons below the label
    m_hl = new QHBOXLAYOUT(this);
    m_hl->setSpacing(spacing);

    QVBOXLAYOUT *vl1 = new QVBOXLAYOUT();
    QHBOXLAYOUT *hl1 = new QHBOXLAYOUT();

    QLabel *tl = new QLabel(this);
    tl->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  tl->sizePolicy().hasHeightForWidth()));
    tl->setText(lbltxt);
    QToolTip::add(tl, tltptxt);
    vl1->addWidget(tl);

    QPushButton *pbA = new QPushButton(this);
    pbA->setText(tr("+"));
    pbA->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				   QSizePolicy::Fixed,
				   0,  // Horizontal stretch
				   0,  // Vertical stretch
				   pbA->sizePolicy().hasHeightForWidth()));
    hl1->addWidget(pbA);
    QPushButton *pbD = new QPushButton(this);
    pbD->setText(tr("-"));
    pbD->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				   QSizePolicy::Fixed,
				   0,  // Horizontal stretch
				   0,  // Vertical stretch
				   pbD->sizePolicy().hasHeightForWidth()));
    hl1->addWidget(pbD);

    vl1->addLayout(hl1);
    m_hl->addLayout(vl1);

    m_lb = new QLISTBOX(this);
    m_lb->setSelectionMode(QLISTBOX::Extended);
    m_lb->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				    QSizePolicy::Preferred,
				    1,  // Horizontal stretch
				    1,  // Vertical stretch
				    m_lb->sizePolicy().hasHeightForWidth()));
    m_hl->addWidget(m_lb);

    this->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				    QSizePolicy::Preferred,
				    1,  // Horizontal stretch
				    1,  // Vertical stretch
				    this->sizePolicy().hasHeightForWidth()));

    loadValue();
    QObject::connect(pbA, SIGNAL(clicked()), this, SLOT(showInputDialog()));
    QObject::connect(pbD, SIGNAL(clicked()), this, SLOT(deleteSelected()));
}

void ConfParamSLW::loadValue()
{
    string s;
    m_cflink->get(s);
    list<string> ls; 
    stringToStrings(s, ls);
    QStringList qls;
    for (list<string>::const_iterator it = ls.begin(); it != ls.end(); it++) {
        if (m_fsencoding)
            qls.push_back(QString::fromLocal8Bit(it->c_str()));
        else
            qls.push_back(QString::fromUtf8(it->c_str()));
    }
    m_lb->clear();
    m_lb->insertStringList(qls);
}

void ConfParamSLW::showInputDialog()
{
    bool ok;
    QString s = QInputDialog::getText("", // Caption
				      "", // Label
				      QLineEdit::Normal, // Mode
				      QString::null, // text
				      &ok,
				      this);
    if (ok && !s.isEmpty()) {
	if (m_lb->findItem(s, QLBEXACTMATCH) == 0) {
	    m_lb->insertItem(s);
	    m_lb->sort();
	    listToConf();
	}
    }
}

void ConfParamSLW::listToConf()
{
    list<string> ls;
    for (unsigned int i = 0; i < m_lb->count(); i++) {
        // General parameters are encoded as utf-8. File names as
        // local8bit There is no hope for 8bit file names anyway
        // except for luck: the original encoding is unknown.
        if (m_fsencoding)
            ls.push_back((const char *)(m_lb->text(i).local8Bit()));
        else
            ls.push_back((const char *)(m_lb->text(i).utf8()));
    }
    string s;
    stringsToString(ls, s);
    m_cflink->set(s);
}

void ConfParamSLW::deleteSelected()
{
    bool didone;
    do {
	didone = false;
	for (unsigned int i = 0; i < m_lb->count(); i++) {
	    if (m_lb->isSelected(i)) {
		emit entryDeleted(m_lb->text(i));
		m_lb->removeItem(i);
		didone = true;
		break;
	    }
	}
    } while (didone);
    listToConf();
}

// "Add entry" dialog for a file name list
void ConfParamDNLW::showInputDialog()
{
    QString s = QFILEDIALOG::getExistingDirectory();
    if (!s.isEmpty()) {
	if (m_lb->findItem(s, QLBEXACTMATCH) == 0) {
	    m_lb->insertItem(s);
	    m_lb->sort();
	    QLISTBOXITEM *item = m_lb->findItem(s, QLBEXACTMATCH);
	    if (m_lb->selectionMode() == QLISTBOX::Single && item)
		m_lb->setSelected(item, true);
	    listToConf();
	}
    }
}

// "Add entry" dialog for a constrained string list
void ConfParamCSLW::showInputDialog()
{
    bool ok;
    QString s = QInputDialog::getItem("", // Caption
				      "", // Label
				      m_sl, // List
				      0, // current
				      false, // editable,
				      &ok);
    if (ok && !s.isEmpty()) {
	if (m_lb->findItem(s, QLBEXACTMATCH) == 0) {
	    m_lb->insertItem(s);
	    m_lb->sort();
	    listToConf();
	}
    }
}

} // Namespace confgui
