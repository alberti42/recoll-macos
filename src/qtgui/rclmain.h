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
#include "searchdata.h"

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
    virtual void startAdvSearch(Rcl::AdvSearchData sdata);
    virtual void previewClosed(QWidget * w);
    virtual void showAdvSearchDialog();
    virtual void showSortDialog();
    virtual void showAboutDialog();
    virtual void startManual();
    virtual void showDocHistory();
    virtual void sortDataChanged(RclSortSpec spec);
    virtual void showUIPrefs();
    virtual void setUIPrefs();
    virtual void enableNextPage(bool);
    virtual void enablePrevPage(bool);
    virtual void docExpand(int);
    virtual void ssearchAddTerm(QString);
protected:
    Preview *curPreview;
    advsearch *asearchform;
    SortForm *sortform;
    UIPrefsDialog *uiprefs;
    RclSortSpec sortspecs;
    RclDHistory *m_history;
private:
    virtual void init();
    virtual void startPreview( int docnum );
    virtual void startNativeViewer( int docnum );
};

#endif // RCLMAIN_H
