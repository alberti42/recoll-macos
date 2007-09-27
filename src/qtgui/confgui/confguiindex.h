#ifndef _confguiindex_h_included_
#define _confguiindex_h_included_
/* @(#$Id: confguiindex.h,v 1.1 2007-09-27 15:47:25 dockes Exp $  (C) 2007 J.F.Dockes */

#include <qwidget.h>

class RclConfig;

namespace confgui {
    // A panel with the top-level parameters (can't change in subdirs)
    class ConfTopPanelW : public QWidget {
    public:
	ConfTopPanelW(QWidget *parent, RclConfig *config);
    };

    // A panel for the parameters that can be changed in subdirectories
    class ConfSubPanelW : public QWidget {
    public:
	ConfSubPanelW(QWidget *parent, RclConfig *config);
    };
}

#endif /* _confguiindex_h_included_ */
