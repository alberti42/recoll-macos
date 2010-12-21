#ifndef lint
static char rcsid[] = "@(#$Id: reslist.cpp,v 1.52 2008-12-17 15:12:08 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include "autoconfig.h"

#include <stdlib.h>

#include <Qt>
#include <QAbstractTableModel>

#include "refcntr.h"
#include "docseq.h"
#include "debuglog.h"
#include "restable.h"
#include "guiutils.h"
#include "reslistpager.h"
#include "reslist.h"
#include "rclconfig.h"
#include "plaintorich.h"

//////////////////////////////////
// Restable "pager". We use it to display doc details in the detail area
///
class ResTablePager : public ResListPager {
public:
    ResTablePager(ResTable *p)
	: ResListPager(1), m_parent(p) 
    {}
    virtual bool append(const string& data, int idx, const Rcl::Doc& doc);
    virtual string trans(const string& in);
    virtual const string &parFormat();
    virtual string iconPath(const string& mt);
private:
    ResTable *m_parent;
};

bool ResTablePager::append(const string& data, int docnum, const Rcl::Doc&)
{
    m_parent->textBrowser->moveCursor(QTextCursor::End, 
				      QTextCursor::MoveAnchor);
    m_parent->textBrowser->textCursor().insertBlock();
    m_parent->textBrowser->insertHtml(QString::fromUtf8(data.c_str()));
    m_parent->m_detaildocnum = docnum;
    return true;
}

string ResTablePager::trans(const string& in)
{
    return string((const char*)ResList::tr(in.c_str()).toUtf8());
}

const string& ResTablePager::parFormat()
{
    return prefs.creslistformat;
}

string ResTablePager::iconPath(const string& mtype)
{
    string iconpath;
    RclConfig::getMainConfig()->getMimeIconName(mtype, &iconpath);
    return iconpath;
}

//////////////////////////////////////////////
//// Data model methods
////
RecollModel::RecollModel(const QStringList fields, QObject *parent)
    : QAbstractTableModel(parent)
{
    for (QStringList::const_iterator it = fields.begin(); 
	 it != fields.end(); it++)
	m_fields.push_back((const char *)(it->toUtf8()));
}

int RecollModel::rowCount(const QModelIndex&) const
{
    LOGDEB(("RecollModel::rowCount\n"));
    if (m_source.isNull())
	return 0;
    return m_source->getResCnt();
}

int RecollModel::columnCount(const QModelIndex&) const
{
    LOGDEB(("RecollModel::columnCount\n"));
    return m_fields.size();
}

void RecollModel::setDocSource(RefCntr<DocSequence> nsource)
{
    LOGDEB(("RecollModel::setDocSource\n"));
    if (nsource.isNull())
	m_source = RefCntr<DocSequence>();
    else
	m_source = RefCntr<DocSequence>(new DocSource(nsource));
    beginResetModel();
    endResetModel();
}

bool RecollModel::getdoc(int index, Rcl::Doc &doc)
{
    LOGDEB(("RecollModel::getDoc\n"));
    if (m_source.isNull())
	return false;
    return m_source->getDoc(index, doc);
}

QVariant RecollModel::headerData(int col, Qt::Orientation orientation, 
				  int role) const
{
    LOGDEB(("RecollModel::headerData: col %d\n", col));
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole ||
	col >= int(m_fields.size())) {
	return QVariant();
    }
    return QString::fromUtf8(m_fields[col].c_str());
}

QVariant RecollModel::data(const QModelIndex& index, int role) const
{
    LOGDEB(("RecollModel::data: row %d col %d\n", index.row(),
	    index.column()));
    if (m_source.isNull() || role != Qt::DisplayRole || !index.isValid() ||
	index.column() >= int(m_fields.size())) {
	return QVariant();
    }
    Rcl::Doc doc;
    if (!m_source->getDoc(index.row(), doc)) {
	return QVariant();
    }
    map<string, string>::const_iterator it = 
	doc.meta.find(m_fields[index.column()]);
    if (it == doc.meta.end()) {
	return QVariant();
    }
    return QString::fromUtf8(it->second.c_str());
}

void RecollModel::sort(int column, Qt::SortOrder order)
{
    LOGDEB(("RecollModel::sort(%d, %d)\n", column, int(order)));
    
}


/////////////////////////// 
// ResTable panel methods
void ResTable::init()
{
    if (!(m_model = new RecollModel(prefs.restableFields)))
	return;
    tableView->setModel(m_model);
    m_pager = new ResTablePager(this);

    QHeaderView *header = tableView->horizontalHeader();
    if (header) {
	if (int(prefs.restableColWidths.size()) == header->count()) {
	    for (int i = 0; i < header->count(); i++) {
		header->resizeSection(i, prefs.restableColWidths[i]);
	    }
	}
	header->setSortIndicatorShown(true);
	header->setSortIndicator(-1, Qt::AscendingOrder);
	connect(header, SIGNAL(sectionResized(int,int,int)),
		this, SLOT(saveColWidths()));
	connect(header, SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
		this, SLOT(sortByColumn(int, Qt::SortOrder)));
    }
}

void ResTable::on_tableView_clicked(const QModelIndex& index)
{
    LOGDEB(("ResTable::on_tableView_clicked(%d, %d)\n", 
	    index.row(), index.column()));

    if (!m_model || m_model->m_source.isNull())
	return;
    HiliteData hdata;
    m_model->m_source->getTerms(hdata.terms, hdata.groups, hdata.gslks);
    Rcl::Doc doc;
    if (m_model->getdoc(index.row(), doc)) {
	textBrowser->clear();
	m_detaildocnum = index.row();
	m_pager->displayDoc(index.row(), doc, hdata);
    }
}

void ResTable::setDocSource(RefCntr<DocSequence> nsource)
{
    LOGDEB(("ResTable::setDocSource\n"));
    if (m_model)
	m_model->setDocSource(nsource);
    if (m_pager)
	m_pager->setDocSource(nsource);
    if (textBrowser)
	textBrowser->clear();
}

void ResTable::resetSource()
{
    LOGDEB(("ResTable::resetSource\n"));
    setDocSource(RefCntr<DocSequence>());
}

void ResTable::saveColWidths()
{
    LOGDEB(("ResTable::saveColWidths()\n"));
    QHeaderView *header = tableView->horizontalHeader();
    if (!header)
	return;
    prefs.restableColWidths.clear();
    for (int i = 0; i < header->count(); i++) {
	prefs.restableColWidths.push_back(header->sectionSize(i));
    }
}

void ResTable::sortByColumn(int column, Qt::SortOrder order)
{
    LOGDEB(("ResTable::sortByColumn(%d,%d)\n", column, int(order)));

    if (column >= 0 && m_model && column < int(m_model->m_fields.size())) {
	DocSeqSortSpec spec;
	spec.field = m_model->m_fields[column];
	spec.desc = order == Qt::AscendingOrder ? false : true;
	m_model->m_source->setSortSpec(spec);
	readDocSource();
	emit sortDataChanged(spec);
    }
}

void ResTable::readDocSource()
{
    m_model->setDocSource(m_model->m_source);
}
