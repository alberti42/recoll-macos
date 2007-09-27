#ifndef _CONFLINKRCL_H_INCLUDED_
#define _CONFLINKRCL_H_INCLUDED_
/* @(#$Id: conflinkrcl.h,v 1.1 2007-09-27 15:47:25 dockes Exp $  (C) 2004 J.F.Dockes */

#include "confgui.h"
#include "rclconfig.h"
#include "debuglog.h"

namespace confgui {

class ConfLinkRclRep : public ConfLinkRep {
public:
    ConfLinkRclRep(RclConfig *conf, const string& nm, 
		   const string& sk = "")
	: m_conf(conf), m_nm(nm), m_sk(sk)
    {
    }
    virtual ~ConfLinkRclRep() {}

    virtual bool set(const string& val) 
    {
	if (!m_conf)
	    return false;
	LOGDEB(("Setting [%s] value to [%s]\n", 
		m_nm.c_str(), val.c_str()));
	bool ret = m_conf->setConfParam(m_nm, val);
	if (!ret)
	    LOGERR(("Value set failed\n"));
	return ret;
    }
    virtual bool get(string& val) 
    {
	if (!m_conf)
	    return false;
	bool ret = m_conf->getConfParam(m_nm, val);
	LOGDEB(("Got [%s] for [%s]\n", 
		ret ? val.c_str() : "no value", m_nm.c_str()));
	return ret;
    }
private:
    RclConfig *m_conf;
    const string m_nm;
    const string m_sk;
};

} // Namespace confgui

#endif /* _CONFLINKRCL_H_INCLUDED_ */
