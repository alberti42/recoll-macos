#ifndef _RESTABLE_H_INCLUDED_
#define _RESTABLE_H_INCLUDED_
/* @(#$Id: spell_w.h,v 1.7 2006-12-22 11:01:28 dockes Exp $  (C) 2006 J.F.Dockes */
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

#include <Qt>

#include "ui_restable.h"
#include "refcntr.h"
#include "docseq.h"
#include "plaintorich.h"

class ResTable;

typedef string (FieldGetter)(const string& fldname, const Rcl::Doc& doc);

class RecollModel : public QAbstractTableModel {

    Q_OBJECT

public:
    RecollModel(const QStringList fields, QObject *parent = 0);

    // Reimplemented methods
    virtual int rowCount (const QModelIndex& = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& = QModelIndex()) const;
    virtual QVariant headerData (int col, Qt::Orientation orientation, 
				 int role = Qt::DisplayRole ) const;
    virtual QVariant data(const QModelIndex& index, 
			   int role = Qt::DisplayRole ) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    // Specific methods
    virtual void readDocSource();
    virtual void setDocSource(RefCntr<DocSequence> nsource);
    virtual RefCntr<DocSequence> getDocSource() {return m_source;}
    virtual void deleteColumn(int);
    virtual const vector<string>& getFields() {return m_fields;}
    virtual const map<string, string>& getAllFields() 
    { 
	return o_displayableFields;
    }
    virtual void addColumn(int, const string&);
    // Some column name are aliases/translator for base document field 
    // (ie: date, datetime->mtime). Help deal with this:
    virtual string baseField(const string&);

    // Ignore sort() call because 
    virtual void setIgnoreSort(bool onoff) {m_ignoreSort = onoff;}

    friend class ResTable;

signals:
    void sortColumnChanged(DocSeqSortSpec);

private:
    mutable RefCntr<DocSequence> m_source;
    vector<string> m_fields;
    vector<FieldGetter*> m_getters;
    static map<string, string> o_displayableFields;
    bool m_ignoreSort;
    FieldGetter* chooseGetter(const string&);
    HiliteData m_hdata;
};

class ResTable;

// Modified textBrowser for the detail area
class ResTableDetailArea : public QTextBrowser {
    Q_OBJECT;

 public:
    ResTableDetailArea(ResTable* parent = 0);
    
 public slots:
    virtual void createPopupMenu(const QPoint& pos);

private:
    ResTable *m_table;
};


class ResTablePager;
class QUrl;

class ResTable : public QWidget, public Ui::ResTable 
{
    Q_OBJECT

public:
    ResTable(QWidget* parent = 0) 
	: QWidget(parent),
	  m_model(0), m_pager(0), m_detail(0), m_detaildocnum(-1)
    {
	setupUi(this);
	init();
    }
	
    virtual ~ResTable() {}
    virtual RecollModel *getModel() {return m_model;}
    virtual ResTableDetailArea* getDetailArea() {return m_detail;}
public slots:
    virtual void onTableView_currentChanged(const QModelIndex&);
    virtual void on_tableView_entered(const QModelIndex& index);
    virtual void setDocSource(RefCntr<DocSequence> nsource);
    virtual void saveColState();
    virtual void resetSource();
    virtual void readDocSource(bool resetPos = true);
    virtual void onSortDataChanged(DocSeqSortSpec);
    virtual void createPopupMenu(const QPoint& pos);
    virtual void menuPreview();
    virtual void menuSaveToFile();
    virtual void menuEdit();
    virtual void menuCopyFN();
    virtual void menuCopyURL();
    virtual void menuExpand();
    virtual void menuPreviewParent();
    virtual void menuOpenParent();
    virtual void createHeaderPopupMenu(const QPoint&);
    virtual void deleteColumn();
    virtual void addColumn();
    virtual void resetSort(); // Revert to natural (relevance) order
    virtual void linkWasClicked(const QUrl&);

signals:
    void docPreviewClicked(int, Rcl::Doc, int);
    void docEditClicked(Rcl::Doc);
    void docSaveToFileClicked(Rcl::Doc);
    void previewRequested(Rcl::Doc);
    void editRequested(Rcl::Doc);
    void headerClicked();
    void docExpand(Rcl::Doc);

    friend class ResTablePager;
    friend class ResTableDetailArea;
private:
    void init();
    RecollModel   *m_model;
    ResTablePager *m_pager;
    ResTableDetailArea *m_detail;
    int            m_detaildocnum;
    Rcl::Doc       m_detaildoc;
    int            m_popcolumn;
};


#endif /* _RESTABLE_H_INCLUDED_ */
