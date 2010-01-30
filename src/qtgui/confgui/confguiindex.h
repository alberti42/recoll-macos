#ifndef _confguiindex_h_included_
#define _confguiindex_h_included_
/* @(#$Id: confguiindex.h,v 1.5 2007-11-24 16:43:51 dockes Exp $  (C) 2007 J.F.Dockes */

/**
 * Classes to handle the gui for the indexing configuration. These group 
 * confgui elements, linked to configuration parameters, into panels.
 */

#include <qwidget.h>
#include <qstring.h>
#if QT_VERSION < 0x040000
#include <qgroupbox.h>
#include <qtabdialog.h>
#define QTABDIALOG QTabDialog
#else // Qt4 -> 
#include <Q3GroupBox>
#include <Q3TabDialog>
#define QTABDIALOG Q3TabDialog
#endif // QT 3/4

#include <string>
#include <list>
using std::string;
using std::list;

class ConfNull;
class RclConfig;
class ConfParamW;
class ConfParamDNLW;

namespace confgui {

class ConfIndexW : public QTABDIALOG {
    Q_OBJECT
public:
    ConfIndexW(QWidget *parent, RclConfig *config);
public slots:
    void acceptChanges();
    void rejectChanges();
    void reloadPanels();
private:
    RclConfig *m_rclconf;
    ConfNull  *m_conf;
    list<QWidget *> m_widgets;
};

/** 
 * A panel with the top-level parameters which can't be redefined in 
 * subdirectoriess:
 */
class ConfTopPanelW : public QWidget {
    Q_OBJECT
public:
    ConfTopPanelW(QWidget *parent, ConfNull *config);
};

/**
 * A panel for the parameters that can be changed in subdirectories:
 */
class ConfSubPanelW : public QWidget {
    Q_OBJECT
public:
    ConfSubPanelW(QWidget *parent, ConfNull *config);

private slots:
    void subDirChanged();
    void subDirDeleted(QString);
    void restoreEmpty();
private:
    string            m_sk;
    ConfParamDNLW    *m_subdirs;
    list<ConfParamW*> m_widgets;
    ConfNull         *m_config;
#if QT_VERSION < 0x040000
    QGroupBox        *m_groupbox;
#else
    Q3GroupBox        *m_groupbox;
#endif
    void reloadAll();
};

/** 
 * Extra or little used parameters
 */
class ConfBeaglePanelW : public QWidget {
    Q_OBJECT
public:
    ConfBeaglePanelW(QWidget *parent, ConfNull *config);
};

} // Namespace confgui

#endif /* _confguiindex_h_included_ */
