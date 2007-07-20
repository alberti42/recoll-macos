#ifndef _PREVIEW_W_H_INCLUDED_
#define _PREVIEW_W_H_INCLUDED_
/* @(#$Id: preview_w.h,v 1.15 2007-07-20 14:43:21 dockes Exp $  (C) 2006 J.F.Dockes */
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
#if (QT_VERSION < 0x040000)
#include "preview.h"
#else
#include "ui_preview.h"
#define QTextEdit Q3TextEdit
#endif
#include "refcntr.h"
#include "plaintorich.h"

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

class QTextEdit;

//MOC_SKIP_BEGIN
#if QT_VERSION < 0x040000
class DummyPreviewBase : public PreviewBase
{
 public: DummyPreviewBase(QWidget* parent = 0) : PreviewBase(parent)	{}
};
#else
class DummyPreviewBase : public QWidget, public Ui::PreviewBase
{
public: DummyPreviewBase(QWidget* parent):QWidget(parent){setupUi(this);}
};
#endif
//MOC_SKIP_END

class Preview : public DummyPreviewBase
{
    Q_OBJECT

public:
    Preview(int sid, // Search Id
	    const HiliteData& hdata) // Search terms etc. for highlighting
	: DummyPreviewBase(0) 
    {
	init();
	m_searchId = sid;
	m_hData = hdata;
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
    bool          m_haveAnchors; // Search terms are marked in text

    void init();
    virtual void setCurTabProps(const string& fn, const Rcl::Doc& doc, 
				int docnum);
    virtual QTextEdit *getCurrentEditor();
    virtual QTextEdit *addEditorTab();
    virtual bool loadFileInCurrentTab(string fn, size_t sz, 
				      const Rcl::Doc& idoc, int dnm);
    TabData *tabDataForCurrent(); // Return auxiliary data pointer for cur tab
};

#endif /* _PREVIEW_W_H_INCLUDED_ */
