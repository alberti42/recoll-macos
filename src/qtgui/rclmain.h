#ifndef RCLMAIN_H
#define RCLMAIN_H

#include <qvariant.h>
#include <qmainwindow.h>
#include "sortseq.h"
#include "preview.h"
#include "recoll.h"
#include "advsearch.h"
#include "sort.h"
#include "uiprefs.h"
#include "rcldb.h"

#include "recollmain.h"

class RclMain : public RclMainBase
{
    Q_OBJECT

public:
    RclMain(QWidget* parent = 0, const char* name = 0, 
	    WFlags fl = WType_TopLevel) 
	: RclMainBase(parent,name,fl) {
	init();
    }
	~RclMain() {}

    virtual bool close( bool );

public slots:
    virtual void fileExit();
    virtual void periodic100();
    virtual void fileStart_IndexingAction_activated();
    virtual void reslistTE_doubleClicked( int par, int );
    virtual void reslistTE_clicked( int par, int car );
    virtual void reslistTE_delayedclick();
    virtual void startSimpleSearch();
    virtual void startAdvSearch( Rcl::AdvSearchData sdata );
    virtual void resPageUpOrBack();
    virtual void resPageDownOrNext();
    virtual void resultPageBack();
    virtual void showResultPage();
    virtual void previewClosed( QWidget * w );
    virtual void showAdvSearchDialog();
    virtual void showSortDialog();
    virtual void showAboutDialog();
    virtual void startManual();
    virtual void showDocHistory();
    virtual void searchTextChanged( const QString & text );
    virtual void sortDataChanged( int cnt, RclSortSpec spec );
    virtual void showUIPrefs();
    virtual void setUIPrefs();

protected:
    int reslist_winfirst;
    bool reslist_mouseDrag;
    bool reslist_mouseDown;
    int reslist_par;
    int reslist_car;
    bool reslist_waitingdbl;
    bool reslist_dblclck;
    Preview *curPreview;
    advsearch *asearchform;
    Rcl::AdvSearchData currentQueryData;
    SortForm *sortform;
    UIPrefsDialog *uiprefs;
    int sortwidth;
    RclSortSpec sortspecs;
    DocSequence *docsource;
    std::map<int,int> pageParaToReldocnums;

private:
    virtual void init();
    virtual bool eventFilter( QObject * target, QEvent * event );
    virtual int reldocnumfromparnum( int par );
    virtual void startPreview( int docnum );
    virtual void startNativeViewer( int docnum );
};

#endif // RCLMAIN_H
