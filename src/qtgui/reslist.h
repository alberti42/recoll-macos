/* Copyright (C) 2005 J.F.Dockes 
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

#ifndef _RESLIST_H_INCLUDED_
#define _RESLIST_H_INCLUDED_

#include <list>
#include <utility>

#ifndef NO_NAMESPACES
using std::list;
using std::pair;
#endif

#include <qtextbrowser.h>
#include <QTextCursor>

#include "docseq.h"
#include "sortseq.h"
#include "filtseq.h"
#include "refcntr.h"
#include "rcldoc.h"
#include "reslistpager.h"

class ResList;
class QtGuiResListPager;
class QMenu;

/**
 * Display a list of document records. The data can be out of the history 
 * manager or from an index query, both abstracted as a DocSequence. 
 * Sorting and filtering are applied by stacking Sort/Filter DocSequences.
 * This is nice because history and index result are handled the same, but 
 * not nice because we can't use the sort/filter capabilities in the index 
 * engine, and do it instead on the index output, which duplicates code and 
 * may be sometimes slower.
 */
class ResList : public QTextBrowser
{
    Q_OBJECT;

    friend class QtGuiResListPager;
 public:
    ResList(QWidget* parent = 0, const char* name = 0);
    virtual ~ResList();
    
    // Return document for given docnum. We mostly act as an
    // intermediary to the docseq here, but this has also the
    // side-effect of making the entry current (visible and
    // highlighted), and only works if the num is inside the current
    // page or its immediate neighbours.
    bool getDoc(int docnum, Rcl::Doc &);
    bool displayingHistory();
    int listId() const {return m_listId;}
    int pageFirstDocNum();

 public slots:
    virtual void setDocSource(RefCntr<DocSequence> nsource);
    virtual void resetList();     // Erase current list
    virtual void resPageUpOrBack(); // Page up pressed
    virtual void resPageDownOrNext(); // Page down pressed
    virtual void resultPageBack(); // Previous page of results
    virtual void resultPageFirst(); // First page of results
    virtual void resultPageNext(); // Next (or first) page of results
    virtual void resultPageFor(int docnum); // Page containing docnum
    virtual void displayPage(); // Display current page
    virtual void menuPreview();
    virtual void menuSaveToFile();
    virtual void menuEdit();
    virtual void menuCopyFN();
    virtual void menuCopyURL();
    virtual void menuExpand();
    virtual void menuPreviewParent();
    virtual void menuOpenParent();
    virtual void previewExposed(int);
    virtual void append(const QString &text);
    virtual void readDocSource();
    virtual void setSortParams(DocSeqSortSpec spec);
    virtual void setFilterParams(const DocSeqFiltSpec &spec);
    virtual void highlighted(const QString& link);
    virtual void createPopupMenu(const QPoint& pos);
	
 signals:
    void nextPageAvailable(bool);
    void prevPageAvailable(bool);
    void docEditClicked(Rcl::Doc);
    void docPreviewClicked(int, Rcl::Doc, int);
    void docSaveToFileClicked(Rcl::Doc);
    void previewRequested(Rcl::Doc);
    void editRequested(Rcl::Doc);
    void headerClicked();
    void docExpand(Rcl::Doc);
    void wordSelect(QString);
    void wordReplace(const QString&, const QString&);
    void linkClicked(const QString&, int); // See emitLinkClicked()
    void hasResults(int);

 protected:
    void keyPressEvent(QKeyEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent*);

 protected slots:
    virtual void languageChange();
    virtual void linkWasClicked(const QUrl &);

 private:
    QtGuiResListPager  *m_pager;
    RefCntr<DocSequence> m_source;

    // Translate from textedit paragraph number to relative
    // docnum. Built while we insert text into the qtextedit
    std::map<int,int>  m_pageParaToReldocnums;

    int                m_popDoc; // Docnum for the popup menu.
    int                m_curPvDoc;// Docnum for current preview
    int                m_lstClckMod; // Last click modifier. 
    list<int>          m_selDocs;
    int                m_listId; // query Id for matching with preview windows
    
    virtual int docnumfromparnum(int);
    virtual pair<int,int> parnumfromdocnum(int);

    // Don't know why this is necessary but it is
    void emitLinkClicked(const QString &s) {
	emit linkClicked(s, m_lstClckMod);
    };
    static int newListId();
    void resetView();
};


#endif /* _RESLIST_H_INCLUDED_ */
