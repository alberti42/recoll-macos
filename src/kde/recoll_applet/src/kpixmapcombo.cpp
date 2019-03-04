/***************************************************************************
 *   Copyright (C)   Andreas Eckstein                                      *
 *                                                                         *
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 ***************************************************************************/
#include "kpixmapcombo.h"

#include <qstyle.h>

KPixmapCombo::KPixmapCombo(QWidget *parent, const char *name) : KHistoryCombo(parent, name)
{
}

void KPixmapCombo::setCurrentIcon(const QPixmap &icon)
{
    m_icon = icon;
    if(count()>0)
        removeItem(0);
    clearEdit();
    insertItem(m_icon, 0);
    setCurrentItem(0);
}

void KPixmapCombo::addToHistory2(const QString &text)
{
    removeItem(0);
    addToHistory(text);
    changeItem(m_icon, text, 0);
    insertItem(m_icon, 0);
    clearEdit();
    setCurrentItem(0);
}

void KPixmapCombo::mousePressEvent(QMouseEvent *e)
{
    int x0 = QStyle::visualRect( style().querySubControlMetrics( QStyle::CC_ComboBox, this, QStyle::SC_ComboBoxEditField ), this ).x();

    if(e->x() > x0 + 2 && e->x() < lineEdit()->x())
    {
        emit iconClicked(e);
        e->accept();
    }
    else
        KHistoryCombo::mousePressEvent(e);
}
#include "kpixmapcombo.moc"
