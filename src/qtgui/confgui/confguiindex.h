#ifndef _confguiindex_h_included_
#define _confguiindex_h_included_
/* @(#$Id: confguiindex.h,v 1.2 2007-09-29 09:06:53 dockes Exp $  (C) 2007 J.F.Dockes */

/**
 * Classes to handle the gui for the indexing configuration. These group 
 * confgui elements, linked to configuration parameters, into panels.
 */

#include <qwidget.h>
#include <qstring.h>

#include <string>
#include <list>
using std::string;
using std::list;


class RclConfig;
class ConfParamW;
class ConfParamDNLW;

namespace confgui {

/** 
 * A panel with the top-level parameters which can't be redefined in 
 * subdirectoriess:
 */
class ConfTopPanelW : public QWidget {
public:
    ConfTopPanelW(QWidget *parent, RclConfig *config);
};


/**
 * A panel for the parameters that can be changed in subdirectories:
 */
class ConfSubPanelW : public QWidget {
    Q_OBJECT
public:
    ConfSubPanelW(QWidget *parent, RclConfig *config);

    void setkeydir(const string& sk) 
    {
	m_sk = sk;
	reloadAll();
    }
    void reloadAll();
public slots:
    void subDirChanged();
    void subDirDeleted(QString);
    void restoreEmpty();
private:
    string            m_sk;
    ConfParamDNLW    *m_subdirs;
    list<ConfParamW*> m_widgets;
    RclConfig        *m_config;
};



} // Namespace confgui

#endif /* _confguiindex_h_included_ */
