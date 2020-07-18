/* Copyright (C) 2005-2020 J.F.Dockes
 *
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

#include <qapplication.h>
#include <qmenu.h>
#include <qclipboard.h>

#include "log.h"
#include "smallut.h"
#include "recoll.h"
#include "docseq.h"
#include "respopup.h"
#include "appformime.h"

namespace ResultPopup {

QMenu *create(QWidget *me, int opts, std::shared_ptr<DocSequence> source,
              Rcl::Doc& doc)
{
    QMenu *popup = new QMenu(me);

    LOGDEB("ResultPopup::create: opts " << opts << " haspages " <<
           doc.haspages << " " <<(source ? "Source not null" : "Source is Null")
           << " "  << (source ? (source->snippetsCapable() ? 
                                 "snippetsCapable":"not snippetsCapable") : "")
           << "\n");

    string apptag;
    doc.getmeta(Rcl::Doc::keyapptg, &apptag);

    // Is this a top level file system file (accessible by regular utilities)?
    bool isFsTop = doc.ipath.empty() && doc.isFsFile();

    popup->addAction(QWidget::tr("&Preview"), me, SLOT(menuPreview()));

    if (!theconfig->getMimeViewerDef(doc.mimetype, apptag, 0).empty()) {
        popup->addAction(QWidget::tr("&Open"), me, SLOT(menuEdit()));
    }

    if (isFsTop) {
        // Openable by regular program. Add "open with" entry.
        vector<DesktopDb::AppDef> apps;
        DesktopDb *ddb = DesktopDb::getDb();
        if (ddb && ddb->appForMime(doc.mimetype, &apps) && !apps.empty()) {
            QMenu *sub = popup->addMenu(QWidget::tr("Open With"));
            if (sub) {
                for (const auto& app : apps) {
                    QAction *act = new QAction(u8s2qs(app.name), me);
                    QVariant v(u8s2qs(app.command));
                    act->setData(v);
                    sub->addAction(act);
                }
                sub->connect(sub, SIGNAL(triggered(QAction *)), me, 
                             SLOT(menuOpenWith(QAction *)));
            }
        }

        // See if there are any desktop files in $RECOLL_CONFDIR/scripts
        // and possibly create a "run script" menu.
        apps.clear();
        ddb = new DesktopDb(path_cat(theconfig->getConfDir(), "scripts"));
        if (ddb && ddb->allApps(&apps) && !apps.empty()) {
            QMenu *sub = popup->addMenu(QWidget::tr("Run Script"));
            if (sub) {
                for (const auto& app : apps) {
                    QAction *act = new QAction(u8s2qs(app.name), me);
                    QVariant v(u8s2qs(app.command));
                    act->setData(v);
                    sub->addAction(act);
                }
                sub->connect(sub, SIGNAL(triggered(QAction *)), me, 
                             SLOT(menuOpenWith(QAction *)));
            }
        }
        delete ddb;
    }

    if (doc.isFsFile()) {
        popup->addAction(QWidget::tr("Copy &File Name"), me, SLOT(menuCopyFN()));
    }
    popup->addAction(QWidget::tr("Copy &URL"), me, SLOT(menuCopyURL()));

    if ((opts&showSaveOne) && !(isFsTop))
        popup->addAction(QWidget::tr("&Write to File"), me,
                         SLOT(menuSaveToFile()));

    if ((opts&showSaveSel))
        popup->addAction(QWidget::tr("Save selection to files"), 
                         me, SLOT(menuSaveSelection()));


    // We now separate preview/open parent, which only makes sense for
    // an embedded doc, and open folder (which was previously done if
    // the doc was a top level file and was not accessible else).
    Rcl::Doc pdoc;
    bool isEnclosed = source && source->getEnclosing(doc, pdoc);
    if (isEnclosed) {
        popup->addAction(QWidget::tr("Preview P&arent document/folder"), 
                         me, SLOT(menuPreviewParent()));
        popup->addAction(QWidget::tr("&Open Parent document"), 
                         me, SLOT(menuOpenParent()));
    }
    if (doc.isFsFile())
        popup->addAction(QWidget::tr("&Open Parent Folder"),
                         me, SLOT(menuOpenFolder()));

    if (opts & showExpand)
        popup->addAction(QWidget::tr("Find &similar documents"), 
                         me, SLOT(menuExpand()));

    if (doc.haspages && source && source->snippetsCapable()) 
        popup->addAction(QWidget::tr("Open &Snippets window"), 
                         me, SLOT(menuShowSnippets()));

    if ((opts & showSubs) && rcldb && rcldb->hasSubDocs(doc)) 
        popup->addAction(QWidget::tr("Show subdocuments / attachments"), 
                         me, SLOT(menuShowSubDocs()));

    return popup;
}

Rcl::Doc getParent(std::shared_ptr<DocSequence> source, Rcl::Doc& doc)
{
    Rcl::Doc pdoc;
    if (source) {
        source->getEnclosing(doc, pdoc);
    }
    return pdoc;
}

Rcl::Doc getFolder(Rcl::Doc& doc)
{
    Rcl::Doc pdoc;
    pdoc.url = url_parentfolder(doc.url);
    pdoc.meta[Rcl::Doc::keychildurl] = doc.url;
    pdoc.meta[Rcl::Doc::keyapptg] = "parentopen";
    pdoc.mimetype = "inode/directory";
    return pdoc;
}

void copyFN(const Rcl::Doc &doc)
{
    // Our urls currently always begin with "file://" 
    //
    // Problem: setText expects a QString. Passing a (const char*)
    // as we used to do causes an implicit conversion from
    // latin1. File are binary and the right approach would be no
    // conversion, but it's probably better (less worse...) to
    // make a "best effort" tentative and try to convert from the
    // locale's charset than accept the default conversion.
    QString qfn = path2qs(doc.url.c_str()+7);
    QApplication::clipboard()->setText(qfn, QClipboard::Selection);
    QApplication::clipboard()->setText(qfn, QClipboard::Clipboard);
}

void copyURL(const Rcl::Doc &doc)
{
    QString url =  path2qs(doc.url);
    QApplication::clipboard()->setText(url, QClipboard::Selection);
    QApplication::clipboard()->setText(url, QClipboard::Clipboard);
}

}
