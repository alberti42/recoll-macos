/***************************************************************************
 *   Copyright (C) 2007 by Andreas Eckstein   *
 *   andreas.eckstein@gmx.net   *
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

#ifndef FIND_APPLET_H
#define FIND_APPLET_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kpanelapplet.h>
#include <qstring.h>
#include <kconfig.h>
//#include <kcombobox.h>
#include <kpixmapcombo.h>
#include <qpopupmenu.h>
#include <qptrvector.h>
#include <kiconloader.h>

class ServiceType
{
public:
    QPixmap icon;
    QString cmd;
    QString label;

    ServiceType(QString cmd, QString label, QString icon)
    {
        this->icon = SmallIcon(icon);
        this->cmd = cmd;
        this->label = label;
    }
};

class ServiceList : public QPtrList<ServiceType>
{
public:
    ServiceList() : QPtrList<ServiceType>()
    {
    }

    uint getServiceType(const QPixmap *p)
    {
        for(uint i=0; i<count(); i++)
        if(at(i)->icon.serialNumber()==p->serialNumber())
            return i;
        return 0;
    }
};


class recoll_applet : public KPanelApplet
{
    Q_OBJECT
    
public:
    recoll_applet(const QString& configFile, Type t = Normal, int actions = 0,
    QWidget *parent = 0, const char *name = 0);
    ~recoll_applet();

    virtual int widthForHeight(int height) const;
    virtual int heightForWidth(int width) const;
    virtual void about();
    virtual void help();
    virtual void preferences();

protected:
    void resizeEvent(QResizeEvent *);

protected slots:
    void evaluate(const QString&);
    void selectFromHistory(int i);
    void showPopup(QMouseEvent *e);
    void changeServiceType(int i);
    void initCombo();
    
private:
    void performAction(QString text, int serviceType);
    ServiceList m_serviceTypes;
    KConfig *ksConfig;
    QWidget *mainView;
    KPixmapCombo *m_combo;
    QPopupMenu *m_popup;
    QFrame *m_frame;
    int currentServiceType;

    void addServiceType(QString str, QString label, QString icon);
    void addServiceType(const char *str, const char *label, const char *icon);
};

#endif
