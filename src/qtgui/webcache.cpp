/* Copyright (C) 2016 J.F.Dockes
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
#include "autoconfig.h"

#include <sstream>
#include <iostream>
#include MEMORY_INCLUDE
#include UNORDERED_MAP_INCLUDE

#ifndef USING_STD_REGEX
#include <sys/types.h>
#include <regex.h>
#else
#include <regex>
#endif

#include <QDebug>

#include "recoll.h"
#include "webcache.h"
#include "beaglequeuecache.h"
#include "circache.h"
#include "conftree.h"

using namespace std;

class CEnt {
public:
    CEnt(const string& ud, const string& ur, const string& mt)
        : udi(ud), url(ur), mimetype(mt) {
    }
    string udi;
    string url;
    string mimetype;
};

class WebcacheModelInternal {
public:
    STD_SHARED_PTR<BeagleQueueCache> cache;
    vector<CEnt> all;
    vector<CEnt> disp;
};

WebcacheModel::WebcacheModel(QObject *parent)
    : QAbstractTableModel(parent), m(new WebcacheModelInternal())
{
    qDebug() << "WebcacheModel::WebcacheModel()";
    m->cache =
        STD_SHARED_PTR<BeagleQueueCache>(new BeagleQueueCache(theconfig));

    if (m->cache) {
        bool eof;
        m->cache->cc()->rewind(eof);
        while (!eof) {
            string udi, sdic;
            m->cache->cc()->getCurrent(udi, sdic);
            ConfSimple dic(sdic);
            string mime, url;
            dic.get("mimetype", mime);
            dic.get("url", url);
            if (!udi.empty()) {
                m->all.push_back(CEnt(udi, url, mime));
                m->disp.push_back(CEnt(udi, url, mime));
            }
            if (!m->cache->cc()->next(eof))
                break;
        }
    }
}

int WebcacheModel::rowCount(const QModelIndex&) const
{
    //qDebug() << "WebcacheModel::rowCount(): " << m->disp.size();
    return int(m->disp.size());
}

int WebcacheModel::columnCount(const QModelIndex&) const
{
    //qDebug() << "WebcacheModel::columnCount()";
    return 2;
}

QVariant WebcacheModel::headerData (int col, Qt::Orientation orientation, 
                                    int role) const
{
    // qDebug() << "WebcacheModel::headerData()";
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    switch (col) {
    case 0: return QVariant(tr("MIME"));
    case 1: return QVariant(tr("Url"));
    default: return QVariant();
    }
}

QVariant WebcacheModel::data(const QModelIndex& index, int role) const
{
    //qDebug() << "WebcacheModel::data()";
    Q_UNUSED(index);
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    int row = index.row();
    if (row < 0 || row >= int(m->disp.size())) {
        return QVariant();
    }

    /* We now read the data on init */
#if 0
    string sdic;
    if (!m->cache->cc()->get(m->disp[row].udi, sdic)) {
        return QVariant();
    }
    ConfSimple dic(sdic);
    //ostringstream os; dic.write(os);  cerr << "DIC: " << os.str() << endl;
    string mime, url;
    dic.get("mimetype", mime);
    dic.get("url", url);
#else
    const string& mime = m->disp[row].mimetype;
    const string& url = m->disp[row].url;
#endif
    
    switch (index.column()) {
    case 0: return QVariant(QString::fromUtf8(mime.c_str()));
    case 1: return QVariant(QString::fromUtf8(url.c_str()));
    default: return QVariant();
    }
}

#ifndef USING_STD_REGEX
#define M_regexec(A,B,C,D,E) regexec(&(A),B,C,D,E)
#else
#define M_regexec(A,B,C,D,E) (!regex_match(B,A))
#endif

void WebcacheModel::setSearchFilter(const QString& _txt)
{
    string txt = qs2utf8s(_txt);
    
#ifndef USING_STD_REGEX
    regex_t exp;
    if (regcomp(&exp, txt.c_str(), REG_NOSUB|REG_EXTENDED)) {
        //qDebug() << "regcomp failed for " << _txt;
        return;
    }
#else
    basic_regex exp;
    try {
        exp = basic_regexp(txt, std::regex_constants::nosubs |
                           std::regex_constants::extended);
    } catch(...) {
        return;
    }
#endif
    
    m->disp.clear();
    for (unsigned int i = 0; i < m->all.size(); i++) {
        if (!M_regexec(exp, m->all[i].url.c_str(), 0, 0, 0)) {
            m->disp.push_back(m->all[i]);
        } else {
            //qDebug() << "match failed. exp" << _txt << "data" <<
            // m->all[i].url.c_str();
        }
    }
    emit dataChanged(createIndex(0,0,0), createIndex(1, m->all.size(),0));
}


WebcacheEdit::WebcacheEdit(QWidget *parent)
    : QDialog(parent)
{
    qDebug() << "WebcacheEdit::WebcacheEdit()";
    setupUi(this);
    m_model = new WebcacheModel(this);
    webcacheTV->setModel(m_model);
    connect(searchLE, SIGNAL(textEdited(const QString&)),
            m_model, SLOT(setSearchFilter(const QString&)));
}

