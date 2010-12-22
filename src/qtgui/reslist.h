#ifndef _RESLIST_H_INCLUDED_
#define _RESLIST_H_INCLUDED_
/* @(#$Id: reslist.h,v 1.17 2008-12-16 14:20:10 dockes Exp $  (C) 2005 J.F.Dockes */

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
    
    // Return document for given docnum. We act as an intermediary to
    // the docseq here. This has also the side-effect of making the
    // entry current (visible and highlighted), and only works if the
    // num is inside the current page or its immediate neighbours.
    bool getDoc(int docnum, Rcl::Doc &);
    bool displayingHistory();
    bool getTerms(vector<string>& terms, 
		  vector<vector<string> >& groups, vector<int>& gslks);
    list<string> expand(Rcl::Doc& doc);
    int listId() const {return m_listId;}

 public slots:
    virtual void setDocSource(RefCntr<DocSequence> nsource);
    virtual void resetList();     // Erase current list
    virtual void resPageUpOrBack(); // Page up pressed
    virtual void resPageDownOrNext(); // Page down pressed
    virtual void resultPageBack(); // Previous page of results
    virtual void resultPageFirst(); // First page of results
    virtual void resultPageNext(); // Next (or first) page of results
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
};


#endif /* _RESLIST_H_INCLUDED_ */
