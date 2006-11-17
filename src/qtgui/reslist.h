#ifndef _RESLIST_H_INCLUDED_
#define _RESLIST_H_INCLUDED_
/* @(#$Id: reslist.h,v 1.4 2006-11-17 12:55:59 dockes Exp $  (C) 2005 J.F.Dockes */

#include <list>

#ifndef NO_NAMESPACES
using std::list;
#endif

#include <qtextbrowser.h>
#include <qpopupmenu.h>

#include "rcldb.h"
#include "docseq.h"
#include "searchdata.h"
#include "refcntr.h"

class ResList : public QTextBrowser
{
    Q_OBJECT;

 public:
    ResList(QWidget* parent = 0, const char* name = 0);
    virtual ~ResList();
    
    // Return document for given docnum. We act as an intermediary to
    // the docseq here. This has also the side-effect of making the
    // entry current (visible and highlighted), and only work if the
    // num is inside the current page or its immediate neighbours.
    virtual bool getDoc(int docnum, Rcl::Doc &);

    virtual void setDocSource(DocSequence *, 
			      RefCntr<Rcl::SearchData> qdata);
    virtual QPopupMenu *createPopupMenu(const QPoint& pos);
    virtual QString getDescription(); // Printable actual query performed on db
    virtual int getResCnt(); // Return total result list size
    virtual RefCntr<Rcl::SearchData> getSearchData() {return m_searchData;}

 public slots:
    virtual void resetSearch() {m_winfirst = -1;clear();}
    virtual void clicked(int, int);
    virtual void doubleClicked(int, int);
    virtual void resPageUpOrBack(); // Page up pressed
    virtual void resPageDownOrNext(); // Page down pressed
    virtual void resultPageBack(); // Display previous page of results
    virtual void resultPageNext(); // Display next (or first) page of results
    virtual void menuPreview();
    virtual void menuEdit();
    virtual void menuCopyFN();
    virtual void menuCopyURL();
    virtual void menuExpand();
    virtual void previewExposed(int);

 signals:
    void nextPageAvailable(bool);
    void prevPageAvailable(bool);
    void docEditClicked(int);
    void docPreviewClicked(int, int);
    void headerClicked();
    void docExpand(int);
    void wordSelect(QString);
    void linkClicked(const QString&, int);

 protected:
    void keyPressEvent(QKeyEvent *e);
    void contentsMouseReleaseEvent(QMouseEvent *e);

 protected slots:
    virtual void languageChange();
    virtual void linkWasClicked(const QString &, int);
    virtual void showQueryDetails();

 private:
    std::map<int,int>  m_pageParaToReldocnums;
    RefCntr<Rcl::SearchData> m_searchData;
    DocSequence       *m_docsource;
    std::vector<Rcl::Doc> m_curDocs;
    int                m_winfirst;
    int                m_popDoc; // Docnum for the popup menu.
    int                m_curPvDoc;// Docnum for current preview
    int                m_lstClckMod; // Last click modifier. 
    list<int>          m_selDocs;

    virtual int docnumfromparnum(int);
    virtual int parnumfromdocnum(int);

    // Don't know why this is necessary but it is
    void emitLinkClicked(const QString &s) {
	emit linkClicked(s, m_lstClckMod);
    };
};


#endif /* _RESLIST_H_INCLUDED_ */
