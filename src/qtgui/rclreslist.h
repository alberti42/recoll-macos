#ifndef _RCLRESLIST_H_INCLUDED_
#define _RCLRESLIST_H_INCLUDED_
/* @(#$Id: rclreslist.h,v 1.6 2006-04-20 09:20:10 dockes Exp $  (C) 2005 J.F.Dockes */

#include <qtextbrowser.h>
#include <qpopupmenu.h>

#include "rcldb.h"
#include "docseq.h"
#include "searchdata.h"

class RclResList : public QTextBrowser
{
    Q_OBJECT;

 public:
    RclResList(QWidget* parent = 0, const char* name = 0);
    virtual ~RclResList();

    virtual bool getDoc( int, Rcl::Doc & );
    virtual void setDocSource(DocSequence *, Rcl::AdvSearchData& qdata);
    virtual QPopupMenu *createPopupMenu(const QPoint& pos);
    virtual QString getDescription();

 public slots:
    virtual void resetSearch() {m_winfirst = -1;clear();}
    virtual void clicked(int, int);
    virtual void resPageUpOrBack();
    virtual void resPageDownOrNext();
    virtual void resultPageBack();
    virtual void showResultPage();
    virtual void menuPreview();
    virtual void menuEdit();
    virtual void menuCopyFN();
    virtual void menuCopyURL();

 signals:
    void nextPageAvailable(bool);
    void prevPageAvailable(bool);
    void docEditClicked(int);
    void docPreviewClicked(int);
    void headerClicked();

 protected:
    void keyPressEvent(QKeyEvent *e);

 protected slots:
    virtual void languageChange();
    virtual void linkWasClicked(const QString &);
    virtual void showQueryDetails();

 private:
    std::map<int,int>  m_pageParaToReldocnums;
    Rcl::AdvSearchData m_queryData;
    DocSequence       *m_docsource;
    std::vector<Rcl::Doc> m_curDocs;
    int                m_winfirst;
    int                m_docnum; // Docnum matching the 

    virtual int docnumfromparnum(int);
    void emitLinkClicked(const QString &s) {
	emit linkClicked(s);
    };
};


#endif /* _RCLRESLIST_H_INCLUDED_ */
