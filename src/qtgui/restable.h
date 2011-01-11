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

signals:
    void sortDataChanged(DocSeqSortSpec);

private:
    mutable RefCntr<DocSequence> m_source;
    vector<string> m_fields;
    vector<FieldGetter*> m_getters;
    static map<string, string> o_displayableFields;
    FieldGetter* chooseGetter(const string&);
};

class ResTablePager;
class QUrl;

class ResTable : public QWidget, public Ui::ResTable 
{
    Q_OBJECT

public:
    ResTable(QWidget* parent = 0) 
	: QWidget(parent),
	  m_model(0), m_pager(0), m_detaildocnum(-1)
    {
	setupUi(this);
	init();
    }
	
    virtual ~ResTable() {}
    virtual RecollModel *getModel() {return m_model;}
    virtual void saveColState();

public slots:
    virtual void onTableView_currentChanged(const QModelIndex&);
    virtual void on_tableView_entered(const QModelIndex& index);
    virtual void saveColWidths();
    virtual void setDocSource(RefCntr<DocSequence> nsource);
    virtual void resetSource();
    virtual void readDocSource(bool resetPos = true);
    virtual void onSortDataChanged(DocSeqSortSpec);
    virtual void linkWasClicked(const QUrl&);
    virtual void createHeaderPopupMenu(const QPoint&);
    virtual void deleteColumn();
    virtual void addColumn();

signals:
    void docPreviewClicked(int, Rcl::Doc, int);
    void docEditClicked(Rcl::Doc);

    friend class ResTablePager;
private:
    void init();
    RecollModel *m_model;
    ResTablePager  *m_pager;
    int            m_detaildocnum;
    int          m_popcolumn;
};


#endif /* _RESTABLE_H_INCLUDED_ */
