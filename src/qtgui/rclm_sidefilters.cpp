/* Copyright (C) 2022 J.F.Dockes
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "autoconfig.h"

#include <vector>
#include <string>

#include "log.h"
#include "rclmain_w.h"
#include "idxmodel.h"
#include "guiutils.h"

void RclMain::populateFilters()
{
    m_idxtreemodel = new IdxTreeModel(theconfig, prefs.idxFilterTreeDepth, idxTreeView);
    m_idxtreemodel->populate();
    m_idxtreemodel->setHeaderData(0, Qt::Horizontal, QVariant(tr("Filter directories")));
    idxTreeView->setModel(m_idxtreemodel);
    idxTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    idxTreeView->setSelectionMode(QAbstractItemView::MultiSelection);
}

void RclMain::clearDirFilter()
{
    idxTreeView->clearSelection();
}

std::vector<std::string> RclMain::idxTreeGetDirs()
{
    std::vector<std::string> out;
    auto selmodel = idxTreeView->selectionModel();
    if (nullptr != selmodel) {
        auto selected = selmodel->selectedIndexes();
        std::string clause;
        for (int i = 0; i < selected.size(); i++) {
            out.push_back(qs2path(selected[i].data(Qt::ToolTipRole).toString()));
        }
    }
    return out;
}
