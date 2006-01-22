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
    virtual void startAdvSearch( Rcl::AdvSearchData sdata );
    virtual void previewClosed( QWidget * w );
    virtual void showAdvSearchDialog();
    virtual void showSortDialog();
    virtual void showAboutDialog();
    virtual void startManual();
    virtual void showDocHistory();
    virtual void sortDataChanged( int cnt, RclSortSpec spec );
    virtual void showUIPrefs();
    virtual void setUIPrefs();
    virtual void enableNextPage(bool);
    virtual void enablePrevPage(bool);
    virtual void showQueryDetails();
protected:
    Preview *curPreview;
    advsearch *asearchform;
    Rcl::AdvSearchData currentQueryData;
    SortForm *sortform;
    UIPrefsDialog *uiprefs;
    int sortwidth;
    RclSortSpec sortspecs;
    DocSequence *docsource;
    RclDHistory *m_history;
private:
    virtual void init();
    virtual bool eventFilter( QObject * target, QEvent * event );
    virtual void startPreview( int docnum );
    virtual void startNativeViewer( int docnum );
};

#endif // RCLMAIN_H
