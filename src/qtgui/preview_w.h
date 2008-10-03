#ifndef _PREVIEW_W_H_INCLUDED_
#define _PREVIEW_W_H_INCLUDED_
/* @(#$Id: preview_w.h,v 1.19 2008-10-03 08:09:35 dockes Exp $  (C) 2006 J.F.Dockes */
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

#include <qvariant.h>
#include <qwidget.h>

#include "rcldb.h"
#include "refcntr.h"
#include "plaintorich.h"

class QTabWidget;
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTextEditFixed;

#if (QT_VERSION < 0x040000)
#include <qtextedit.h>
#include <private/qrichtext_p.h>
#define QTEXTEDIT QTextEdit
#else
#include <q3textedit.h>
#include <q3richtext_p.h>
#define QTEXTEDIT Q3TextEdit
#endif

class QTextEditFixed : public QTEXTEDIT {
    Q_OBJECT
public:
    QTextEditFixed( QWidget* parent=0, const char* name=0 ) 
	: QTEXTEDIT(parent, name)
    {}
    void moveToAnchor(const QString& name);
};


// We keep a list of data associated to each tab
class TabData {
 public:
    string fn; // filename for this tab
    string ipath; // Internal doc path inside file
    QWidget *w; // widget for setCurrent
    int docnum;  // Index of doc in db search results.

    TabData(QWidget *wi) 
	: w(wi), docnum(-1) 
    {}
};

// Subclass plainToRich to add <termtag>s and anchors to the preview text
class PlainToRichQtPreview : public PlainToRich {
public:
    int lastanchor;
    PlainToRichQtPreview(bool inputhtml = false) : PlainToRich(inputhtml) {
	lastanchor = 0;
    }    
    virtual ~PlainToRichQtPreview() {}
    virtual string header() {
	if (m_inputhtml) {
	    return snull;
	} else {
	    return string("<qt><head><title></title></head><body><p>");
	}
    }
    virtual string startMatch() {return string("<termtag>");}
    virtual string endMatch() {return string("</termtag>");}
    virtual string termAnchorName(int i) {
	static const char *termAnchorNameBase = "TRM";
	char acname[sizeof(termAnchorNameBase) + 20];
	sprintf(acname, "%s%d", termAnchorNameBase, i);
	if (i > lastanchor)
	    lastanchor = i;
	return string(acname);
    }

    virtual string startAnchor(int i) {
	return string("<a name=\"") + termAnchorName(i) + "\">";
    }
};

class Preview : public QWidget {

    Q_OBJECT

public:

    Preview(int sid, // Search Id
	    const HiliteData& hdata) // Search terms etc. for highlighting
	: QWidget(0), m_searchId(sid), m_hData(hdata)
    {
	init();
    }
    ~Preview(){}

    virtual void closeEvent(QCloseEvent *e );
    virtual bool eventFilter(QObject *target, QEvent *event );
    virtual bool makeDocCurrent(const string &fn, size_t sz, 
				const Rcl::Doc& idoc, int docnum, 
				bool sametab = false);

public slots:
    virtual void searchTextLine_textChanged(const QString& text);
    virtual void doSearch(const QString& str, bool next, bool reverse,
			  bool wo = false);
    virtual void nextPressed();
    virtual void prevPressed();
    virtual void currentChanged(QWidget *tw);
    virtual void closeCurrentTab();
    virtual void textDoubleClicked(int, int);
    virtual void selecChanged();

signals:
    void previewClosed(Preview *);
    void wordSelect(QString);
    void showNext(Preview *w, int sid, int docnum);
    void showPrev(Preview *w, int sid, int docnum);
    void previewExposed(Preview *w, int sid, int docnum);

private:
    // Identifier of search in main window. This is used to check that
    // we make sense when requesting the next document when browsing
    // successive search results in a tab.
    int           m_searchId; 

    bool          m_dynSearchActive;
    bool          m_canBeep;
    bool          m_loading;
    list<TabData> m_tabData;
    QWidget      *m_currentW;
    HiliteData    m_hData;
    bool          m_justCreated; // First tab create is different
    PlainToRichQtPreview m_plaintorich;
    bool          m_haveAnchors; // Search terms are marked in text
    int           m_lastAnchor; // Number of last anchor. Then rewind to 1
    int           m_curAnchor;

    QTabWidget* pvTab;
    QLabel* searchLabel;
    QLineEdit* searchTextLine;
    QPushButton* nextButton;
    QPushButton* prevButton;
    QPushButton* clearPB;
    QCheckBox* matchCheck;

    void init();
    virtual void setCurTabProps(const string& fn, const Rcl::Doc& doc, 
				int docnum);
    virtual QTextEditFixed *getCurrentEditor();
    virtual QTextEditFixed *addEditorTab();
    virtual bool loadFileInCurrentTab(string fn, size_t sz, 
				      const Rcl::Doc& idoc, int dnm);
    TabData *tabDataForCurrent(); // Return auxiliary data pointer for cur tab
};

#endif /* _PREVIEW_W_H_INCLUDED_ */
