#ifndef _CONFLINKRCL_H_INCLUDED_
#define _CONFLINKRCL_H_INCLUDED_
/* @(#$Id: conflinkrcl.h,v 1.2 2007-09-29 09:06:53 dockes Exp $  (C) 2004 J.F.Dockes */

/** 
 * A Gui-to-Data link class for RclConfig
 * Has a subkey pointer member which makes it easy to change the
 * current subkey for a number at a time.
 */
#include "confgui.h"
#include "rclconfig.h"
#include "debuglog.h"

namespace confgui {

class ConfLinkRclRep : public ConfLinkRep {
public:
    ConfLinkRclRep(RclConfig *conf, const string& nm, 
		   string *sk = 0)
	: m_conf(conf), m_nm(nm), m_sk(sk)
    {
    }
    virtual ~ConfLinkRclRep() {}

    virtual bool set(const string& val) 
    {
	if (!m_conf)
	    return false;
	LOGDEB1(("Setting [%s] value to [%s]\n", 
		m_nm.c_str(), val.c_str()));
	m_conf->setKeyDir(m_sk ? *m_sk : "");
	bool ret = m_conf->setConfParam(m_nm, val);
	if (!ret)
	    LOGERR(("Value set failed\n"));
	return ret;
    }
    virtual bool get(string& val) 
    {
	if (!m_conf)
	    return false;
	m_conf->setKeyDir(m_sk ? *m_sk : "");
	bool ret = m_conf->getConfParam(m_nm, val);
	LOGDEB1(("Got [%s] for [%s]\n", 
		ret ? val.c_str() : "no value", m_nm.c_str()));
	return ret;
    }
private:
    RclConfig    *m_conf;
    const string  m_nm;
    const string *m_sk;
};

} // Namespace confgui

#endif /* _CONFLINKRCL_H_INCLUDED_ */
