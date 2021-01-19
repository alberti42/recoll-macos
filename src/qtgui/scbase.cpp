/* Copyright (C) 2017-2019 J.F.Dockes
 *
 * License: GPL 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "scbase.h"

#include <map>
#include <string>

#include <QSettings>


#include "recoll.h"
#include "smallut.h"
#include "log.h"

struct SCDef {
    QString ctxt;
    QString desc;
    QKeySequence val;
    QKeySequence dflt;
};

class SCBase::Internal {
public:
    std::map<QString, SCDef> scdefs;
};

static QString mapkey(const QString& ctxt, const QString& desc)
{
    return ctxt + "/" + desc;
}
#if 0
static QString mapkey(const std::string& ctxt, const std::string& desc)
{
    return u8s2qs(ctxt) + "/" + u8s2qs(desc);
}
#endif

QString SCBase::scBaseSettingsKey()
{
    return "/Recoll/prefs/sckeys";
}
SCBase::SCBase()
{
    m = new Internal();

    QSettings settings;
    auto sl = settings.value(scBaseSettingsKey()).toStringList();

    for (int i = 0; i < sl.size(); ++i) {
        auto ssc = qs2utf8s(sl.at(i));
        std::vector<std::string> co_des_val;
        stringToStrings(ssc, co_des_val);
        if (co_des_val.size() != 3) {
            LOGERR("Bad shortcut def in prefs: [" << ssc << "]\n");
            continue;
        }
        QString ctxt = u8s2qs(co_des_val[0]);
        QString desc = u8s2qs(co_des_val[1]);
        QString val = u8s2qs(co_des_val[2]);
        QString key = mapkey(ctxt, desc);
        auto it = m->scdefs.find(key);
        if (it == m->scdefs.end()) {
            m->scdefs[key] = SCDef{ctxt, desc,QKeySequence(val), QKeySequence()};
        } else {
            it->second.val = QKeySequence(val);
        }
    }
}

SCBase::~SCBase()
{
    delete m;
}

QKeySequence SCBase::get(const QString& ctxt, const QString& desc,
                         const QString& defks)
{
    std::cerr << "SCBase::get\n";
    QString key = mapkey(ctxt, desc);
    auto it = m->scdefs.find(key);
    if (it == m->scdefs.end()) {
        if (defks.isEmpty()) {
            return QKeySequence();
        }
        QKeySequence qks(defks);
        m->scdefs[key] = SCDef{ctxt, desc, qks, qks};
        std::cerr << "get(" << qs2utf8s(ctxt) << ", " << qs2utf8s(desc) <<
            ", " << qs2utf8s(defks) << ") -> " <<
            qs2utf8s(qks.toString()) << "\n";
        return qks;
    }
    std::cerr << "get(" << qs2utf8s(ctxt) << ", " << qs2utf8s(desc) <<
        ", " << qs2utf8s(defks) << ") -> " <<
        qs2utf8s(it->second.val.toString()) << "\n";
    return it->second.val;
}

QStringList SCBase::getAll()
{
    QStringList result;
    for (const auto& entry : m->scdefs) {
        result.push_back(entry.second.ctxt);
        result.push_back(entry.second.desc);
        result.push_back(entry.second.val.toString());
        result.push_back(entry.second.dflt.toString());
    }
    return result;
}

static SCBase *theBase;

SCBase& SCBase::scBase()
{
    std::cerr << "SCBase::scBase()\n";
    if (nullptr == theBase) {
        std::cerr << "SCBase::scBase() creating class\n";
        theBase = new SCBase();
    }
    std::cerr << "SCBase::scBase() returning " << theBase << "\n";
    return *theBase;
}
