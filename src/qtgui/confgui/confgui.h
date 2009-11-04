#ifndef _confgui_h_included_
#define _confgui_h_included_
/* @(#$Id: confgui.h,v 1.7 2008-05-21 07:21:37 dockes Exp $  (C) 2007 J.F.Dockes */
/**
 * This file defines a number of simple classes (virtual base: ConfParamW) 
 * which let the user input configuration parameters. 
 *
 * Subclasses are defined for entering different kind of data, ie a string, 
 * a file name, an integer, etc.
 *
 * Each configuration gui object is linked to the configuration data through
 * a "link" object which knows the details of interacting with the actual
 * configuration data, like the parameter name, the actual config object, 
 * the method to call etc.
 * 
 * The link object is set when the input widget is created and cannot be 
 * changed.
 *
 * The widgets are typically linked to a temporary configuration object, which
 * is then copied to the actual configuration if the data is accepted, or
 * destroyed and recreated as a copy if Cancel is pressed (you have to 
 * delete/recreate the widgets in this case as the links are no longer valid).
 */
#include <string>
#include <limits.h>

#include <qglobal.h>
#include <qstring.h>
#include <qwidget.h>
#if QT_VERSION < 0x040000
#define QHBOXLAYOUT QHBoxLayout
#define QLISTBOX QListBox
#else
#define QHBOXLAYOUT Q3HBoxLayout
#define QLISTBOX Q3ListBox
#endif

#include "refcntr.h"

using std::string;

class QHBOXLAYOUT;
class QLineEdit;
class QLISTBOX;
class QSpinBox;
class QComboBox;
class QCheckBox;

namespace confgui {

    // A class to isolate the gui widget from the config storage mechanism
    class ConfLinkRep {
    public:
	virtual ~ConfLinkRep() {}
	virtual bool set(const string& val) = 0;
	virtual bool get(string& val) = 0;
    };
    typedef RefCntr<ConfLinkRep> ConfLink;

    // Useful to store/manage data which has no direct representation in
    // the config, ie list of subkey directories
    class ConfLinkNullRep : public ConfLinkRep {
    public:
	virtual ~ConfLinkNullRep() {}
	virtual bool set(const string&)
	{
	    return true;
	}
	virtual bool get(string& val) {val = ""; return true;}
    };

    // A widget to let the user change one configuration
    // parameter. Subclassed for specific parameter types. Basically
    // has a label and some kind of entry widget
    class ConfParamW : public QWidget {
	Q_OBJECT
    public:
	ConfParamW(QWidget *parent, ConfLink cflink)
	    : QWidget(parent), m_cflink(cflink), m_fsencoding(false)
	{
	}
	virtual void loadValue() = 0;
        virtual void setFsEncoding(bool onoff) {m_fsencoding = onoff;}
    protected:
	ConfLink     m_cflink;
	QHBOXLAYOUT *m_hl;
        // File names are encoded as local8bit in the config files. Other
        // are encoded as utf-8
        bool         m_fsencoding;
	virtual bool createCommon(const QString& lbltxt,
				  const QString& tltptxt);

    protected slots:
        void setValue(const QString& newvalue);
        void setValue(int newvalue);
        void setValue(bool newvalue);
    };


    // Widgets for setting the different types of configuration parameters:

    // Int
    class ConfParamIntW : public ConfParamW {
    public:
	ConfParamIntW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt,
		      int minvalue = INT_MIN, 
		      int maxvalue = INT_MAX);
	virtual void loadValue();
    protected:
	QSpinBox *m_sb;
    };

    // Arbitrary string
    class ConfParamStrW : public ConfParamW {
    public:
	ConfParamStrW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt);
	virtual void loadValue();
    protected:
	QLineEdit *m_le;
    };

    // Constrained string: choose from list
    class ConfParamCStrW : public ConfParamW {
    public:
	ConfParamCStrW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt, const QStringList& sl);
	virtual void loadValue();
    protected:
	QComboBox *m_cmb;
    };

    // Boolean
    class ConfParamBoolW : public ConfParamW {
    public:
	ConfParamBoolW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt);
	virtual void loadValue();
    protected:
	QCheckBox *m_cb;
    };

    // File name
    class ConfParamFNW : public ConfParamW {
	Q_OBJECT
    public:
	ConfParamFNW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		     const QString& tltptxt, bool isdir = false);
	virtual void loadValue();
    protected slots:
	void showBrowserDialog();
    protected:
	QLineEdit *m_le;
	bool       m_isdir;
    };

    // String list
    class ConfParamSLW : public ConfParamW {
	Q_OBJECT
    public:
	ConfParamSLW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt);
	virtual void loadValue();
	QLISTBOX *getListBox() {return m_lb;}
	
    protected slots:
	virtual void showInputDialog();
	void deleteSelected();
    signals:
        void entryDeleted(QString);
    protected:
	QLISTBOX *m_lb;
	void listToConf();
    };

    // Dir name list
    class ConfParamDNLW : public ConfParamSLW {
	Q_OBJECT
    public:
	ConfParamDNLW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt)
	    : ConfParamSLW(parent, cflink, lbltxt, tltptxt)
	    {
                m_fsencoding = true;
	    }
    protected slots:
	virtual void showInputDialog();
    };

    // Constrained string list (chose from predefined)
    class ConfParamCSLW : public ConfParamSLW {
	Q_OBJECT
    public:
	ConfParamCSLW(QWidget *parent, ConfLink cflink, 
		      const QString& lbltxt,
		      const QString& tltptxt,
		      const QStringList& sl)
	    : ConfParamSLW(parent, cflink, lbltxt, tltptxt), m_sl(sl)
	    {
	    }
    protected slots:
	virtual void showInputDialog();
    protected:
	const QStringList m_sl;
    };
}

#endif /* _confgui_h_included_ */
