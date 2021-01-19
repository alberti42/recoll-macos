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

#ifndef _SCBASE_H_INCLUDED_
#define _SCBASE_H_INCLUDED_

#include <QString>
#include <QKeySequence>
#include <QStringList>

class SCBase {
public:
    ~SCBase();

    static SCBase& scBase();

    QKeySequence get(const QString& context, const QString& description,
                     const QString& defkeyseq);
    QStringList getAll();
    static QString scBaseSettingsKey();
    
    class Internal;
private:
    Internal *m{nullptr};
    SCBase();
};

#endif /* _SCBASE_H_INCLUDED_ */
