/* Copyright (C) 2005 J.F.Dockes
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

/** Saving and restoring named queries */

#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>

#include "rclmain_w.h"
#include "log.h"
#include "readfile.h"
#include "xmltosd.h"
#include "searchdata.h"
#include "copyfile.h"
#include "pathut.h"

using namespace std;
using namespace Rcl;

static QString prevDir()
{
    QSettings settings;
    QString prevdir = settings.value("/Recoll/prefs/lastQuerySaveDir").toString();
    string defpath = path_cat(theconfig->getConfDir(), "saved_queries");
    if (prevdir.isEmpty()) {
        if (!path_exists(defpath)) {
            path_makepath(defpath, 0700);
        }
        return path2qs(defpath);
    } else {
        return prevdir;
    }
}

void RclMain::saveLastQuery()
{
    string xml;
    if (lastSearchSimple()) {
        xml = sSearch->asXML();
    } else {
        if (g_advshistory) {
            std::shared_ptr<Rcl::SearchData> sd;
            sd = g_advshistory->getnewest();
            if (sd) {
                xml = sd->asXML();
            }
        }
    }
    if (xml.empty()) {
        QMessageBox::information(this, tr("No search"), tr("No preserved previous search"));
        return;
    }
    xml = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
        "<recollquery version='1.0'>\n" + xml + "\n</recollquery>\n";

    QFileDialog fileDialog(this, tr("Choose file to save"));
    fileDialog.setNameFilter(tr("Saved Queries (*.rclq)"));
    fileDialog.setDefaultSuffix("rclq");
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setDirectory(prevDir());

    if (!fileDialog.exec())
        return;

    QString s = fileDialog.selectedFiles().first();
    if (s.isEmpty()) {
        return;
    }
    
    string tofile(qs2path(s));

    // Work around qt 5.9-11 bug (linux at least): defaultSuffix is
    // not added to saved file name
    string suff = path_suffix(tofile);
    if (suff.compare("rclq")) {
        tofile += ".rclq";
    }

    LOGDEB("RclMain::saveLastQuery: XML: [" << xml << "]\n");
    string reason;
    if (!stringtofile(xml, tofile.c_str(), reason)) {
        QMessageBox::warning(this, tr("Write failed"), tr("Could not write to file"));
    }
    return;
}


void RclMain::loadSavedQuery()
{
    QString s = QFileDialog::getOpenFileName(this, "Open saved query", prevDir(), 
                                             tr("Saved Queries (*.rclq)"));
    if (s.isEmpty())
        return;

    string fromfile(qs2path(s));
    string xml, reason;
    if (!file_to_string(fromfile, xml, &reason)) {
        QMessageBox::warning(this, tr("Read failed"), tr("Could not open file: ") + 
                             QString::fromUtf8(reason.c_str()));
        return;
    }

    // Try to parse as advanced search SearchData. This fails because of the "type" attribute if
    // this is a simple search entry.
    std::shared_ptr<SearchData> sd = SearchData::fromXML(xml, false);
    if (sd) {
        showAdvSearchDialog();
        asearchform->fromSearch(sd);
        return;
    }
    LOGDEB1("loadSavedQuery: Not advanced search. Parsing as simple search\n");
    // Try to parse as Simple Search
    SSearchDef sdef;
    if (xmlToSSearch(xml, sdef)) {
        if (sSearch->fromXML(sdef))
            return;
    }
    QMessageBox::warning(this, tr("Load error"), tr("Could not load saved query"));
}
