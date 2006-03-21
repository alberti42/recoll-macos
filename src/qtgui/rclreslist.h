#ifndef _RCLRESLIST_H_INCLUDED_
#define _RCLRESLIST_H_INCLUDED_
/* @(#$Id: rclreslist.h,v 1.3 2006-03-21 15:11:30 dockes Exp $  (C) 2005 J.F.Dockes */

#include <qtextbrowser.h>
#include <qpopupmenu.h>

#include "rcldb.h"
#include "docseq.h"

class RclResList : public QTextBrowser
{
    Q_OBJECT;

 public:
    RclResList(QWidget* parent = 0, const char* name = 0);
    virtual ~RclResList();

    virtual void resetSearch() {m_winfirst = -1;}
    virtual bool getDoc( int, Rcl::Doc & );
    virtual void setDocSource(DocSequence *);
    virtual QPopupMenu *createPopupMenu(const QPoint& pos);

 public slots:
    virtual void clicked(int, int);
    virtual void resPageUpOrBack();
    virtual void resPageDownOrNext();
    virtual void resultPageBack();
    virtual void showResultPage();
    virtual void menuPreview();
    virtual void menuEdit();
    virtual void menuCopyFN();

 signals:
    void nextPageAvailable(bool);
    void prevPageAvailable(bool);
    void docDoubleClicked(int);
    void docClicked(int);
    void headerClicked();

 protected:
    void keyPressEvent(QKeyEvent *e);

 protected slots:
    virtual void languageChange();
    virtual void linkWasClicked(const QString &);

 private:
    std::map<int,int> m_pageParaToReldocnums;
    virtual int docnumfromparnum(int);

    DocSequence *m_docsource;
    std::vector<Rcl::Doc> m_curDocs;
    int m_winfirst;
    int m_docnum; // Docnum matching the 
    void emitLinkClicked(const QString &s) {
	emit linkClicked(s);
    };
};


#endif /* _RCLRESLIST_H_INCLUDED_ */
