/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <qfiledialog.h>

#include <list>
#include <string>

using std::list;
using std::string;

#include "recoll.h"
#include "rclconfig.h"
#include "debuglog.h"


extern RclConfig *rclconfig;

// Constructor/initialization
void advsearch::init()
{
    list<string> types = rclconfig->getAllMimeTypes();
    QStringList ql;
    for (list<string>::iterator it = types.begin(); it != types.end(); it++) {
	ql.append(it->c_str());
    }
    yesFiltypsLB->insertStringList(ql);
}

// Move selected file types from the searched to the ignored box
void advsearch::delFiltypPB_clicked()
{
    list<int> trl;
    QStringList moved;
    for (unsigned int i = 0; i < yesFiltypsLB->count();i++) {
	QListBoxItem *item = yesFiltypsLB->item(i);
	if (item && item->isSelected()) {
	    moved.push_back(item->text());
	    trl.push_front(i);
	}
    }
    if (!moved.empty()) {
	noFiltypsLB->insertStringList(moved);
	for (list<int>::iterator it = trl.begin();it != trl.end(); it++)
	    yesFiltypsLB->removeItem(*it);
    }
    yesFiltypsLB->sort();
    noFiltypsLB->sort();
}


// Move selected file types from the ignored to the searched box
void advsearch::addFiltypPB_clicked()
{
    list<int> trl;
    QStringList moved;
    for (unsigned int i = 0; i < noFiltypsLB->count(); i++) {
	QListBoxItem *item = noFiltypsLB->item(i);
	if (item && item->isSelected()) {
	    moved.push_back(item->text());
	    trl.push_front(i);
	}
    }
    if (!moved.empty()) {
	yesFiltypsLB->insertStringList(moved);
	for (list<int>::iterator it = trl.begin();it != trl.end(); it++)
	    noFiltypsLB->removeItem(*it);
    }
    yesFiltypsLB->sort();
    noFiltypsLB->sort();
}


// Activate file type selection
void advsearch::restrictFtCB_toggled(bool on)
{
    yesFiltypsLB->setEnabled(on);
    delFiltypPB->setEnabled(on);
    addFiltypPB->setEnabled(on);
    noFiltypsLB->setEnabled(on);
}

void advsearch::searchPB_clicked()
{
    Rcl::AdvSearchData mydata;
    mydata.allwords = string((const char*)(andWordsLE->text().utf8()));
    mydata.phrase  = string((const char*)(phraseLE->text().utf8()));
    mydata.orwords = string((const char*)(orWordsLE->text().utf8()));
    mydata.nowords = string((const char*)(noWordsLE->text().utf8()));
    if (restrictFtCB->isOn() && noFiltypsLB->count() > 0) {
	for (unsigned int i = 0; i < yesFiltypsLB->count(); i++) {
	    QCString ctext = yesFiltypsLB->item(i)->text().utf8();
	    mydata.filetypes.push_back(string((const char *)ctext));
	}
    }
    if (!subtreeLE->text().isEmpty()) {
	mydata.topdir = string((const char*)(subtreeLE->text().utf8()));
    }
    emit startSearch(mydata);
}


void advsearch::browsePB_clicked()
{
    QString dir = QFileDialog::getExistingDirectory();
    subtreeLE->setText(dir);
}
