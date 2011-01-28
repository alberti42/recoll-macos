#ifndef lint
static char rcsid[] = "@(#$Id: reslist.cpp,v 1.52 2008-12-17 15:12:08 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include "autoconfig.h"

#include <stdlib.h>
#include <time.h>

#include <algorithm>

#include <Qt>
#include <QShortcut>
#include <QAbstractTableModel>
#include <QSettings>
#include <QMenu>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>

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
// Restable "pager". We use it to display a single doc details in the 
// detail area
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

//////////////////////////
// Restable hiliter: to highlight search term in the table. This is actually
// the same as reslist's, could be shared.
class PlainToRichQtReslist : public PlainToRich {
public:
    virtual ~PlainToRichQtReslist() {}
    virtual string startMatch() {
	return string("<span style='color: ")
	    + string((const char *)prefs.qtermcolor.toAscii()) + string("'>");
    }
    virtual string endMatch() {return string("</span>");}
};
static PlainToRichQtReslist g_hiliter;
/////////////////////////////////////

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

// Routines used to extract named data from an Rcl::Doc. The basic one just uses the meta map. Others
// (ie: the date ones) need to do a little processing
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

// Static map to translate from internal column names to displayable ones
map<string, string> RecollModel::o_displayableFields = 
    create_map<string, string>
    ("abstract", QT_TR_NOOP("Abstract"))
    ("author", QT_TR_NOOP("Author"))
    ("dbytes", QT_TR_NOOP("Document size"))
    ("dmtime", QT_TR_NOOP("Document date"))
    ("fbytes", QT_TR_NOOP("File size"))
    ("filename", QT_TR_NOOP("File name"))
    ("fmtime", QT_TR_NOOP("File date"))
    ("ipath", QT_TR_NOOP(" Ipath"))
    ("keywords", QT_TR_NOOP("Keywords"))
    ("mtype", QT_TR_NOOP("Mime type"))
    ("origcharset", QT_TR_NOOP("Original character set"))
    ("relevancyrating", QT_TR_NOOP("Relevancy rating"))
    ("title", QT_TR_NOOP("Title"))
    ("url", QT_TR_NOOP("URL"))
    ("mtime", QT_TR_NOOP("Mtime"))
    ("date", QT_TR_NOOP("Date"))
    ("datetime", QT_TR_NOOP("Date and time"))
    ;

FieldGetter *RecollModel::chooseGetter(const string& field)
{
    if (!stringlowercmp("date", field))
	return dategetter;
    else if (!stringlowercmp("datetime", field))
	return datetimegetter;
    else
	return gengetter;
}

string RecollModel::baseField(const string& field)
{
    if (!stringlowercmp("date", field) || !stringlowercmp("datetime", field))
	return "mtime";
    else
	return field;
}

RecollModel::RecollModel(const QStringList fields, QObject *parent)
    : QAbstractTableModel(parent), m_ignoreSort(false)
{
    // Add dynamic "stored" fields to the full column list. This
    // could be protected to be done only once, but it's no real
    // problem
    RclConfig *config = RclConfig::getMainConfig();
    if (config) {
	const set<string>& stored = config->getStoredFields();
	for (set<string>::const_iterator it = stored.begin(); 
	     it != stored.end(); it++) {
	    if (o_displayableFields.find(*it) == o_displayableFields.end()) {
		o_displayableFields[*it] = *it;
	    }
	}
    }

    // Construct the actual list of column names
    for (QStringList::const_iterator it = fields.begin(); 
	 it != fields.end(); it++) {
	m_fields.push_back((const char *)(it->toUtf8()));
	m_getters.push_back(chooseGetter(m_fields[m_fields.size()-1]));
    }

    g_hiliter.set_inputhtml(false);
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

void RecollModel::readDocSource()
{
    LOGDEB(("RecollModel::readDocSource()\n"));
    beginResetModel();
    endResetModel();
}

void RecollModel::setDocSource(RefCntr<DocSequence> nsource)
{
    LOGDEB(("RecollModel::setDocSource\n"));
    if (nsource.isNull()) {
	m_source = RefCntr<DocSequence>();
    } else {
	m_source = RefCntr<DocSequence>(new DocSource(nsource));
	m_hdata.reset();
	m_source->getTerms(m_hdata.terms, m_hdata.groups, m_hdata.gslks);
    }
    readDocSource();
}

void RecollModel::deleteColumn(int col)
{
    if (col > 0 && col < int(m_fields.size())) {
	vector<string>::iterator it = m_fields.begin();
	it += col;
	m_fields.erase(it);
	vector<FieldGetter*>::iterator it1 = m_getters.begin();
	it1 += col;
	m_getters.erase(it1);
	readDocSource();
    }
}

void RecollModel::addColumn(int col, const string& field)
{
    LOGDEB(("AddColumn: col %d fld [%s]\n", col, field.c_str()));
    if (col >= 0 && col < int(m_fields.size())) {
	col++;
	vector<string>::iterator it = m_fields.begin();
	vector<FieldGetter*>::iterator it1 = m_getters.begin();
	if (col) {
	    it += col;
	    it1 += col;
	}
	m_fields.insert(it, field);
	m_getters.insert(it1, chooseGetter(field));
	readDocSource();
    }
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
	map<string, string>::const_iterator it = 
	    o_displayableFields.find(m_fields[idx]);
	if (it == o_displayableFields.end())
	    return QString::fromUtf8(m_fields[idx].c_str());
	else 
	    return QString::fromUtf8(it->second.c_str());
    }
    return QVariant();
}

QVariant RecollModel::data(const QModelIndex& index, int role) const
{
    LOGDEB1(("RecollModel::data: row %d col %d\n", index.row(),
	     index.column()));
    if (m_source.isNull() || role != Qt::DisplayRole || !index.isValid() ||
	index.column() >= int(m_fields.size())) {
	return QVariant();
    }

    Rcl::Doc doc;
    if (!m_source->getDoc(index.row(), doc)) {
	return QVariant();
    }

    string colname = m_fields[index.column()];

    list<string> lr;
    g_hiliter.plaintorich(m_getters[index.column()](colname, doc), lr, m_hdata);
    return QString::fromUtf8(lr.front().c_str());
}

// This gets called when the column headers are clicked
void RecollModel::sort(int column, Qt::SortOrder order)
{
    if (m_ignoreSort)
	return;
    LOGDEB(("RecollModel::sort(%d, %d)\n", column, int(order)));
    
    DocSeqSortSpec spec;
    if (column >= 0 && column < int(m_fields.size())) {
	spec.field = m_fields[column];
	if (!stringlowercmp("date", spec.field) || 
	    !stringlowercmp("datetime", spec.field))
	    spec.field = "mtime";
	spec.desc = order == Qt::AscendingOrder ? false : true;
    } 
    emit sortColumnChanged(spec);
}

/////////////////////////// 
// ResTable panel methods

// We use a custom delegate to display the cells because the base
// tableview's can't handle rich text to highlight the match terms
class ResTableDelegate: public QStyledItemDelegate {
public:
    ResTableDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, 
	       const QModelIndex &index) const
    {
	QVariant value = index.data(Qt::DisplayRole);
	if (value.isValid() && !value.isNull()) {
	    // We might possibly want to optimize by passing the data
	    // to the base method if the text does not contain any
	    // term matches. Would need a modif to plaintorich to
	    // return the match count (easy), and a way to pass an
	    // indicator from data(), a bit more difficult. Anyway,
	    // the display seems fast enough as is.
	    QTextDocument document;
	    document.setHtml(value.toString());
	    painter->save();
	    painter->setClipRect(option.rect);
	    painter->translate(option.rect.topLeft());
	    document.drawContents(painter);
	    painter->restore();
	} 
    }
};

void ResTable::init()
{
    if (!(m_model = new RecollModel(prefs.restableFields)))
	return;
    tableView->setModel(m_model);
    tableView->setMouseTracking(true);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setItemDelegate(new ResTableDelegate(this));

    QHeaderView *header = tableView->horizontalHeader();
    if (header) {
	if (int(prefs.restableColWidths.size()) == header->count()) {
	    for (int i = 0; i < header->count(); i++) {
		header->resizeSection(i, prefs.restableColWidths[i]);
	    }
	}
	header->setSortIndicatorShown(true);
	header->setSortIndicator(-1, Qt::AscendingOrder);
	header->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(header, SIGNAL(sectionResized(int,int,int)),
		this, SLOT(saveColWidths()));
	connect(header, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(createHeaderPopupMenu(const QPoint&)));
    }
    header->setMovable(true);

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
    QVariant saved = settings.value("resTableSplitterSizes");
    if (saved != QVariant()) {
	splitter->restoreState(saved.toByteArray());
    } else {
	QList<int> sizes;
	sizes << 355 << 125;
	splitter->setSizes(sizes);
    }

    textBrowser->setReadOnly(TRUE);
    textBrowser->setUndoRedoEnabled(FALSE);
    textBrowser->setOpenLinks(FALSE);
    // signals and slots connections
    connect(textBrowser, SIGNAL(anchorClicked(const QUrl &)), 
	    this, SLOT(linkWasClicked(const QUrl &)));

}

// This is called by rclmain_w prior to exiting
void ResTable::saveColState()
{
    QSettings settings;
    settings.setValue("resTableSplitterSizes", splitter->saveState());

    QHeaderView *header = tableView->horizontalHeader();
    if (header && header->sectionsMoved()) {
	// Remember the current column order. Walk in visual order and
	// create new list
	QStringList newfields;
	vector<int> newwidths;
	for (int vi = 0; vi < header->count(); vi++) {
	    int li = header->logicalIndex(vi);
	    newfields.push_back(prefs.restableFields.at(li));
	    newwidths.push_back(header->sectionSize(li));
	}
	prefs.restableFields = newfields;
	prefs.restableColWidths = newwidths;
    } else {
	const vector<string>& vf = m_model->getFields();
	prefs.restableFields.clear();
	for (int i = 0; i < int(vf.size()); i++) {
	    prefs.restableFields.push_back(QString::fromUtf8(vf[i].c_str()));
	}
	saveColWidths();
    }
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

void ResTable::onTableView_currentChanged(const QModelIndex& index)
{
    LOGDEB0(("ResTable::onTableView_currentChanged(%d, %d)\n", 
	    index.row(), index.column()));

    if (!m_model || m_model->getDocSource().isNull())
	return;
    Rcl::Doc doc;
    if (m_model->getDocSource()->getDoc(index.row(), doc)) {
	textBrowser->clear();
	m_detaildocnum = index.row();
	m_pager->displayDoc(index.row(), doc, m_model->m_hdata);
    }
}

void ResTable::on_tableView_entered(const QModelIndex& index)
{
    LOGDEB0(("ResTable::on_tableView_entered(%d, %d)\n", 
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
	m_pager->setDocSource(nsource, 0);
    if (textBrowser)
	textBrowser->clear();
}

void ResTable::resetSource()
{
    LOGDEB(("ResTable::resetSource\n"));
    setDocSource(RefCntr<DocSequence>());
}

// This is called when the sort order is changed from another widget
void ResTable::onSortDataChanged(DocSeqSortSpec spec)
{
    LOGDEB(("ResTable::onSortDataChanged: [%s] desc %d\n", 
	    spec.field.c_str(), int(spec.desc)));
    QHeaderView *header = tableView->horizontalHeader();
    if (!header || !m_model)
	return;

    // Check if the specified field actually matches one of columns
    // and set indicator
    m_model->setIgnoreSort(true);
    bool matched = false;
    const vector<string> fields = m_model->getFields();
    for (unsigned int i = 0; i < fields.size(); i++) {
	if (!spec.field.compare(m_model->baseField(fields[i]))) {
	    header->setSortIndicator(i, spec.desc ? 
				     Qt::DescendingOrder : Qt::AscendingOrder);
	    matched = true;
	}
    }
    if (!matched)
	header->setSortIndicator(-1, Qt::AscendingOrder);
    m_model->setIgnoreSort(false);
}

void ResTable::resetSort()
{
    LOGDEB(("ResTable::resetSort()\n"));
    QHeaderView *header = tableView->horizontalHeader();
    if (header)
	header->setSortIndicator(-1, Qt::AscendingOrder); 
    // the model's sort slot is not called by qt in this case (qt 4.7)
    if (m_model)
	m_model->sort(-1, Qt::AscendingOrder);
}

void ResTable::readDocSource(bool resetPos)
{
    LOGDEB(("ResTable::readDocSource(%d)\n", int(resetPos)));
    if (resetPos)
	tableView->verticalScrollBar()->setSliderPosition(0);

    m_model->readDocSource();
    textBrowser->clear();
}

void ResTable::linkWasClicked(const QUrl &url)
{
    if (!m_model || m_model->getDocSource().isNull())
	return;
    QString s = url.toString();
    const char *ascurl = s.toAscii();
    LOGDEB(("ResTable::linkWasClicked: [%s]\n", ascurl));

    int i = atoi(ascurl+1) -1;
    int what = ascurl[0];
    switch (what) {
    case 'P': 
    case 'E': 
    {
	Rcl::Doc doc;
	if (!m_model->getDocSource()->getDoc(i, doc)) {
	    LOGERR(("ResTable::linkWasClicked: can't get doc for %d\n", i));
	    return;
	}
	if (what == 'P')
	    emit docPreviewClicked(i, doc, 0);
	else
	    emit docEditClicked(doc);
    }
    break;
    default: 
	LOGERR(("ResList::linkWasClicked: bad link [%s]\n", ascurl));
	break;// ?? 
    }
}

void ResTable::createHeaderPopupMenu(const QPoint& pos)
{
    LOGDEB(("ResTable::createHeaderPopupMenu(%d, %d)\n", pos.x(), pos.y()));
    QHeaderView *header = tableView->horizontalHeader();
    if (!header || !m_model)
	return;

    m_popcolumn = header->logicalIndexAt(pos);
    if (m_popcolumn < 0)
	return;

    const map<string, string>& allfields = m_model->getAllFields();
    const vector<string>& fields = m_model->getFields();
    QMenu *popup = new QMenu(this);

    popup->addAction(tr("&Reset sort"), this, SLOT(resetSort()));
    popup->addSeparator();

    popup->addAction(tr("&Delete column"), this, SLOT(deleteColumn()));
    popup->addSeparator();

    QAction *act;
    for (map<string, string>::const_iterator it = allfields.begin();
	 it != allfields.end(); it++) {
	if (std::find(fields.begin(), fields.end(), it->first) != fields.end())
	    continue;
	act = new QAction(tr("Add \"")+tr(it->second.c_str())+tr("\" column"),
			  popup);
	act->setData(QString::fromUtf8(it->first.c_str()));
	connect(act, SIGNAL(triggered(bool)), this , SLOT(addColumn()));
	popup->addAction(act);
    }
    popup->popup(mapToGlobal(pos));
}

void ResTable::deleteColumn()
{
    if (m_model)
	m_model->deleteColumn(m_popcolumn);
}

void ResTable::addColumn()
{
    if (!m_model)
	return;
    QAction *action = (QAction *)sender();
    LOGDEB(("addColumn: text %s, data %s\n", 
	    (const char *)action->text().toUtf8(),
	    (const char *)action->data().toString().toUtf8()
	       ));
    string field((const char *)action->data().toString().toUtf8());
    m_model->addColumn(m_popcolumn, field);
}
