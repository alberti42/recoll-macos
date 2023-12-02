/* Copyright (C) 2005-2023 J.F.Dockes 
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

#ifndef _RCLWEBPAGE_H_INCLUDED_
#define _RCLWEBPAGE_H_INCLUDED_

#if defined(USING_WEBENGINE)

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QUrl>

// Subclass the page to hijack the link clicks
class RclWebPage : public QWebEnginePage {
    Q_OBJECT

public:
    RclWebPage(QWidget *parent) 
        : QWebEnginePage((QWidget *)parent) {}

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType tp, bool isMainFrame) override {
        Q_UNUSED(isMainFrame);
        if (tp == QWebEnginePage::NavigationTypeLinkClicked) {
            emit linkClicked(url);
            return false;
        } else {
            return true;
        }
    }
signals:
    void linkClicked(const QUrl&);
};

// For enums
#define WEBPAGE QWebEnginePage

#elif defined(USING_WEBKIT)

#include <QWebView>
#include <QWebFrame>
#define RclWebPage QWebPage

// For enums
#define WEBPAGE QWebPage

#endif // Webkit

#endif // _RCLWEBPAGE_H_INCLUDED_
