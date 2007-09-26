#ifndef lint
static char rcsid[] = "@(#$Id: confgui.cpp,v 1.1 2007-09-26 12:16:48 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>

#include <qobject.h>
#include <qlayout.h>
#include <qsize.h>
#include <qsizepolicy.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qfiledialog.h>
#include <qinputdialog.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qlistbox.h>
#include <qcombobox.h>

#include "confgui.h"
#include "smallut.h"
#include "debuglog.h"

#include <list>
using std::list;

namespace confgui {


void ConfParamW::setValue(const QString& value)
{
    m_cflink.set(string((const char *)value.utf8()));
}

void ConfParamW::setValue(int value)
{
    char buf[30];
    sprintf(buf, "%d", value);
    m_cflink.set(string(buf));
}
void ConfParamW::setValue(bool value)
{
    char buf[30];
    sprintf(buf, "%d", value);
    m_cflink.set(string(buf));
}

bool ConfParamW::createCommon(const QString& lbltxt, const QString& tltptxt)
{
    m_hl = new QHBoxLayout(this);
    m_hl->setSpacing(6);

    QLabel *tl = new QLabel(this);
    tl->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  tl->sizePolicy().hasHeightForWidth() ) );
    tl->setText(lbltxt);
    QToolTip::add(tl, tltptxt);
    /* qt4    tl->setProperty("toolTip", tltptxt);*/

    m_hl->addWidget(tl);

    return true;
}

ConfParamIntW::ConfParamIntW(QWidget *parent, ConfLink &cflink, 
			     const QString& lbltxt,
			     const QString& tltptxt,
			     int minvalue, 
			     int maxvalue)
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;


    QSpinBox *sb = new QSpinBox(this);
    //    sb->setMinimum(minvalue);
    sb->setMinValue(minvalue);
    //    sb->setMaximum(maxvalue);
    sb->setMaxValue(maxvalue);
    sb->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  sb->sizePolicy().hasHeightForWidth() ) );


    string s;
    m_cflink.get(s);
    sb->setValue(atoi(s.c_str()));

    m_hl->addWidget(sb);

    QObject::connect(sb, SIGNAL(valueChanged(int)), 
		     this, SLOT(setValue(int)));
}

ConfParamStrW::ConfParamStrW(QWidget *parent, ConfLink& cflink, 
			     const QString& lbltxt,
			     const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;
    QLineEdit *le = new QLineEdit(this);
    //    le->setMinimumSize( QSize( 200, 0 ) );

    string s;
    m_cflink.get(s);
    le->setText(QString::fromUtf8(s.c_str()));
    le->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  1,  // Horizontal stretch
				  0,  // Vertical stretch
				  le->sizePolicy().hasHeightForWidth() ) );

    m_hl->addWidget(le);

    QObject::connect(le, SIGNAL(textChanged(const QString&)), 
		     this, SLOT(setValue(const QString&)));
}

ConfParamCStrW::ConfParamCStrW(QWidget *parent, ConfLink& cflink, 
			       const QString& lbltxt,
			       const QString& tltptxt,
			       const QStringList &sl
			       )
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;
    QComboBox *cmb = new QComboBox(this);
    cmb->insertStringList(sl);
    cmb->setEditable(false);
    string s;
    m_cflink.get(s);
    QString cs = QString::fromUtf8(s.c_str());
    for (int i = 0; i < cmb->count(); i++) {
	if (!cs.compare(cmb->text(i))) {
	    cmb->setCurrentItem(i);
	    break;
	}
    }

    cmb->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  1,  // Horizontal stretch
				  0,  // Vertical stretch
				   cmb->sizePolicy().hasHeightForWidth() ) );

    m_hl->addWidget(cmb);

    QObject::connect(cmb, SIGNAL(activated(const QString&)), 
		     this, SLOT(setValue(const QString&)));
}

ConfParamBoolW::ConfParamBoolW(QWidget *parent, ConfLink& cflink, 
			     const QString& lbltxt,
			     const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;
    QCheckBox *cb = new QCheckBox(this);

    string s;
    m_cflink.get(s);
    cb->setChecked(stringToBool(s));
    cb->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  cb->sizePolicy().hasHeightForWidth() ) );

    m_hl->addWidget(cb);

    QObject::connect(cb, SIGNAL(toggled(bool)), 
		     this, SLOT(setValue(bool)));
}

ConfParamFNW::ConfParamFNW(QWidget *parent, ConfLink& cflink, 
			   const QString& lbltxt,
			   const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    if (!createCommon(lbltxt, tltptxt))
	return;


    m_le = new QLineEdit(this);
    m_le->setMinimumSize(QSize(150, 0 ));
    m_le->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  1,  // Horizontal stretch
				  0,  // Vertical stretch
				  m_le->sizePolicy().hasHeightForWidth() ) );
    m_hl->addWidget(m_le);

    QPushButton *pb = new QPushButton(this);
    pb->setText(tr("Browse"));
    pb->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  pb->sizePolicy().hasHeightForWidth() ) );
    m_hl->addWidget(pb);


    string s;
    m_cflink.get(s);
    m_le->setText(QString::fromUtf8(s.c_str()));

    QObject::connect(pb, SIGNAL(clicked()), this, SLOT(showBrowserDialog()));
    QObject::connect(m_le, SIGNAL(textChanged(const QString&)), 
		     this, SLOT(setValue(const QString&)));
}

void ConfParamFNW::showBrowserDialog()
{
    QString s = QFileDialog::getOpenFileName("",
					     "",
					     this,
					     "open file dialog",
					     "Choose a file");
    if (!s.isEmpty()) 
	m_le->setText(s);
}

ConfParamSLW::ConfParamSLW(QWidget *parent, ConfLink& cflink, 
			     const QString& lbltxt,
			     const QString& tltptxt)
    : ConfParamW(parent, cflink)
{
    // Can't use createCommon here cause we want the buttons below the label
    m_hl = new QHBoxLayout(this);
    m_hl->setSpacing(6);

    QVBoxLayout *vl1 = new QVBoxLayout();
    QHBoxLayout *hl1 = new QHBoxLayout();

    QLabel *tl = new QLabel(this);
    tl->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Fixed,
				  0,  // Horizontal stretch
				  0,  // Vertical stretch
				  tl->sizePolicy().hasHeightForWidth() ) );
    tl->setText(lbltxt);
    QToolTip::add(tl, tltptxt);
    /* qt4    tl->setProperty("toolTip", tltptxt);*/
    vl1->addWidget(tl);

    QPushButton *pbA = new QPushButton(this);
    pbA->setText(tr("Add"));
    pbA->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				   QSizePolicy::Fixed,
				   0,  // Horizontal stretch
				   0,  // Vertical stretch
				   pbA->sizePolicy().hasHeightForWidth() ) );
    hl1->addWidget(pbA);
    QPushButton *pbD = new QPushButton(this);
    pbD->setText(tr("Delete"));
    pbD->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, 
				   QSizePolicy::Fixed,
				   0,  // Horizontal stretch
				   0,  // Vertical stretch
				   pbD->sizePolicy().hasHeightForWidth() ) );

    hl1->addWidget(pbD);
    vl1->addLayout(hl1);
    m_hl->addLayout(vl1);


    m_lb = new QListBox(this);

    string s;
    m_cflink.get(s);
    list<string> ls; 
    stringToStrings(s, ls);
    QStringList qls;
    for (list<string>::const_iterator it = ls.begin(); it != ls.end(); it++) {
	qls.push_back(QString::fromUtf8(it->c_str()));
    }
    m_lb->insertStringList(qls);

    m_lb->setSelectionMode(QListBox::Extended);

    m_lb->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Preferred,
				  1,  // Horizontal stretch
				  1,  // Vertical stretch
				  m_lb->sizePolicy().hasHeightForWidth() ) );
    m_hl->addWidget(m_lb);

    this->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, 
				  QSizePolicy::Preferred,
				  1,  // Horizontal stretch
				  1,  // Vertical stretch
				  this->sizePolicy().hasHeightForWidth() ) );

    
    QObject::connect(pbA, SIGNAL(clicked()), this, SLOT(showInputDialog()));
    QObject::connect(pbD, SIGNAL(clicked()), this, SLOT(deleteSelected()));
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
	if (m_lb->findItem(s, Qt::ExactMatch) == 0) {
	    m_lb->insertItem(s);
	    m_lb->sort();
	    listToConf();
	}
    }
}

void ConfParamSLW::listToConf()
{
    LOGDEB(("ConfParamSLW::listToConf()\n"));
    list<string> ls;
    for (unsigned int i = 0; i < m_lb->count(); i++) {
	ls.push_back((const char *)m_lb->text(i));
    }
    string s;
    stringsToString(ls, s);
    m_cflink.set(s);
}

void ConfParamSLW::deleteSelected()
{
    bool didone;
    do {
	didone = false;
	for (unsigned int i = 0; i < m_lb->count(); i++) {
	    if (m_lb->isSelected(i)) {
		LOGDEB(("%d is selected\n", i));
		m_lb->removeItem(i);
		didone = true;
		break;
	    }
	}
    } while (didone);
    listToConf();
}

// "Add entry" dialog for a file name list
void ConfParamFNLW::showInputDialog()
{
    QString s = QFileDialog::getOpenFileName("",
					     "",
					     this,
					     "open file dialog",
					     "Choose a file");
    if (!s.isEmpty()) {
	if (m_lb->findItem(s, Qt::ExactMatch) == 0) {
	    m_lb->insertItem(s);
	    m_lb->sort();
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
				      &ok,
				      this);
    if (ok && !s.isEmpty()) {
	if (m_lb->findItem(s, Qt::ExactMatch) == 0) {
	    m_lb->insertItem(s);
	    m_lb->sort();
	    listToConf();
	}
    }
}

}
