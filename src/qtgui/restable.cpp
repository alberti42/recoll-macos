#ifndef lint
static char rcsid[] = "@(#$Id: reslist.cpp,v 1.52 2008-12-17 15:12:08 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include "autoconfig.h"

#include <stdlib.h>
#include <time.h>

#include <Qt>
#include <QShortcut>
#include <QAbstractTableModel>
#include <QSettings>

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
static string gengetter(const string& fld, const Rcl::Doc& doc)
{
    map<string, string>::const_iterator it = doc.meta.find(fld);
    if (it == doc.meta.end()) {
	return string();
    }
    return it->second;
}

static string dategetter(const string&, const Rcl::Doc& doc)
{
    char datebuf[100];
    datebuf[0] = 0;
    if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
	time_t mtime = doc.dmtime.empty() ?
	    atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	struct tm *tm = localtime(&mtime);
	strftime(datebuf, 99, "%x", tm);
    }
    return datebuf;
}

static string datetimegetter(const string&, const Rcl::Doc& doc)
{
    char datebuf[100];
    datebuf[0] = 0;
    if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
	time_t mtime = doc.dmtime.empty() ?
	    atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	struct tm *tm = localtime(&mtime);
#ifndef sun
	strftime(datebuf, 99, "%Y-%m-%d %H:%M:%S %z", tm);
#else
	strftime(datebuf, 99, "%c", tm);
#endif
    }
    return datebuf;
}

RecollModel::RecollModel(const QStringList fields, QObject *parent)
    : QAbstractTableModel(parent)
{
    for (QStringList::const_iterator it = fields.begin(); 
	 it != fields.end(); it++) {
	m_fields.push_back((const char *)(it->toUtf8()));
	if (!stringlowercmp("date", m_fields[m_fields.size()-1]))
	    m_getters.push_back(dategetter);
	else if (!stringlowercmp("datetime", m_fields[m_fields.size()-1]))
	    m_getters.push_back(datetimegetter);
	else
	    m_getters.push_back(gengetter);
    }
}

int RecollModel::rowCount(const QModelIndex&) const
{
    LOGDEB2(("RecollModel::rowCount\n"));
    if (m_source.isNull())
	return 0;
    return m_source->getResCnt();
}

int RecollModel::columnCount(const QModelIndex&) const
{
    LOGDEB2(("RecollModel::columnCount\n"));
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

QVariant RecollModel::headerData(int idx, Qt::Orientation orientation, 
				 int role) const
{
    LOGDEB2(("RecollModel::headerData: idx %d\n", idx));
    if (orientation == Qt::Vertical && role == Qt::DisplayRole) {
	return idx;
    }
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole &&
	idx < int(m_fields.size())) {
	return QString::fromUtf8(m_fields[idx].c_str());
    }
    return QVariant();
}

QVariant RecollModel::data(const QModelIndex& index, int role) const
{
    LOGDEB2(("RecollModel::data: row %d col %d\n", index.row(),
	    index.column()));
    if (m_source.isNull() || role != Qt::DisplayRole || !index.isValid() ||
	index.column() >= int(m_fields.size())) {
	return QVariant();
    }

    Rcl::Doc doc;
    if (!m_source->getDoc(index.row(), doc)) {
	return QVariant();
    }

    // Have to handle the special cases here. Some fields are
    // synthetic and their name is hard-coded. Only date and datetime
    // for now.
    string colname = m_fields[index.column()];
    return QString::fromUtf8(m_getters[index.column()](colname, doc).c_str());
}

// This is called when the column headers are clicked
void RecollModel::sort(int column, Qt::SortOrder order)
{
    LOGDEB(("RecollModel::sort(%d, %d)\n", column, int(order)));
    
    if (column >= 0 && column < int(m_fields.size())) {
	DocSeqSortSpec spec;
	spec.field = m_fields[column];
	if (!stringlowercmp("date", spec.field) || 
	    !stringlowercmp("datetime", spec.field))
	    spec.field = "mtime";
	spec.desc = order == Qt::AscendingOrder ? false : true;
	m_source->setSortSpec(spec);
	setDocSource(m_source);
	emit sortDataChanged(spec);
    }
}


/////////////////////////// 
// ResTable panel methods
void ResTable::init()
{
    if (!(m_model = new RecollModel(prefs.restableFields)))
	return;
    tableView->setModel(m_model);
    tableView->setMouseTracking(true);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);

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
    }

    header = tableView->verticalHeader();
    if (header) {
	header->setDefaultSectionSize(22);
    }

    QKeySequence seq("Esc");
    QShortcut *sc = new QShortcut(seq, this);
    connect(sc, SIGNAL (activated()), 
	    tableView->selectionModel(), SLOT (clear()));
    connect(tableView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex &)),
	    this, SLOT(onTableView_currentChanged(const QModelIndex&)));

    m_pager = new ResTablePager(this);
    QSettings settings;
    splitter->restoreState(settings.value("resTableSplitterSizes").toByteArray());
}

void ResTable::saveSizeState()
{
    QSettings settings;
    settings.setValue("resTableSplitterSizes", splitter->saveState());
}

void ResTable::onTableView_currentChanged(const QModelIndex& index)
{
    LOGDEB(("ResTable::onTableView_currentChanged(%d, %d)\n", 
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

void ResTable::on_tableView_entered(const QModelIndex& index)
{
    LOGDEB(("ResTable::on_tableView_entered(%d, %d)\n", 
	    index.row(), index.column()));
    if (!tableView->selectionModel()->hasSelection())
	onTableView_currentChanged(index);
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

// This is called when the sort order is changed from another widget
void ResTable::onSortDataChanged(DocSeqSortSpec)
{
    QHeaderView *header = tableView->horizontalHeader();
    if (!header)
	return;
    header->setSortIndicator(-1, Qt::AscendingOrder);
}

void ResTable::readDocSource()
{
    m_model->setDocSource(m_model->m_source);
}
