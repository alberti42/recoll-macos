#ifndef _RCLRESLIST_H_INCLUDED_
#define _RCLRESLIST_H_INCLUDED_
/* @(#$Id: rclreslist.h,v 1.1 2006-03-21 09:15:56 dockes Exp $  (C) 2005 J.F.Dockes */

#include <qtextbrowser.h>

#include "rcldb.h"
#include "docseq.h"

class RclResList : public QTextBrowser
{
    Q_OBJECT;

 public:
    RclResList(QWidget* parent = 0, const char* name = 0);
    virtual ~RclResList();

    int m_winfirst;
    bool m_mouseDrag;
    bool m_mouseDown;
    int m_par;
    int m_car;
    bool m_waitingdbl;
    bool m_dblclck;

    virtual bool getDoc( int, Rcl::Doc & );
    virtual void setDocSource(DocSequence *);
    bool eventFilter( QObject *o, QEvent *e );

 public slots:
    virtual void doubleClicked( int, int );
    virtual void clicked( int, int );
    virtual void delayedClick();
    virtual void resPageUpOrBack();
    virtual void resPageDownOrNext();
    virtual void resultPageBack();
    virtual void showResultPage();

 signals:
    void nextPageAvailable(bool);
    void prevPageAvailable(bool);
    void docDoubleClicked(int);
    void docClicked(int);
    void headerClicked();

 protected:
    void keyPressEvent( QKeyEvent * e);

 protected slots:
    virtual void languageChange();
    virtual void linkWasClicked( const QString & );

 private:
    std::map<int,int> m_pageParaToReldocnums;
    DocSequence *m_docsource;
    std::vector<Rcl::Doc> m_curDocs;

    virtual int reldocnumfromparnum( int );
    void emitLinkClicked(const QString &s) {
	emit linkClicked(s);
    };
};


#endif /* _RCLRESLIST_H_INCLUDED_ */
