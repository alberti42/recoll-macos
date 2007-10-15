/***************************************************************************
 *   Copyright (C)   Andreas Eckstein                                      *
 *    andreas.eckstein@gmx.net                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef KPIXMAPCOMBO_H
#define KPIXMAPCOMBO_H

#include <qwidget.h>
#include <qpixmap.h>
#include <qvaluelist.h>
#include <kcombobox.h>

class KPixmapCombo : public KHistoryCombo
{
    Q_OBJECT

    public:
        KPixmapCombo(QWidget *parent, const char *name);
        const QPixmap &icon() const;
        void setCurrentIcon(const QPixmap &icon);
        void addToHistory2(const QString &text);

    signals:
        void iconClicked(QMouseEvent* me);

    protected:
        virtual void mousePressEvent(QMouseEvent *e);

    private:
        QPixmap m_icon;

    public:
        QValueList<QPixmap> m_iconlist;
};

#endif
