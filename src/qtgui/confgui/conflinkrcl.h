#ifndef _CONFLINKRCL_H_INCLUDED_
#define _CONFLINKRCL_H_INCLUDED_
/* @(#$Id: conflinkrcl.h,v 1.3 2007-10-09 11:08:17 dockes Exp $  (C) 2004 J.F.Dockes */

/** 
 * A Gui-to-Data link class for ConfTree
 * Has a subkey pointer member which makes it easy to change the
 * current subkey for a number at a time.
 */
#include "confgui.h"
#include "conftree.h"
#include "debuglog.h"

namespace confgui {

class ConfLinkRclRep : public ConfLinkRep {
public:
    ConfLinkRclRep(ConfNull *conf, const string& nm, 
		   string *sk = 0)
	: m_conf(conf), m_nm(nm), m_sk(sk)
    {
    }
    virtual ~ConfLinkRclRep() {}

    virtual bool set(const string& val) 
    {
	if (!m_conf)
	    return false;
	LOGDEB1(("Setting [%s] value to [%s]\n",  m_nm.c_str(), val.c_str()));
	bool ret = m_conf->set(m_nm, val, m_sk?*m_sk:"");
	if (!ret)
	    LOGERR(("Value set failed\n"));
	return ret;
    }
    virtual bool get(string& val) 
    {
	if (!m_conf)
	    return false;
	bool ret = m_conf->get(m_nm, val, m_sk?*m_sk:"");
	LOGDEB1(("ConfLinkRcl::get: [%s] sk [%s] -> [%s]\n", 
		 m_nm.c_str(), m_sk?m_sk->c_str():"",
		 ret ? val.c_str() : "no value"));
	return ret;
    }
private:
    ConfNull     *m_conf;
    const string  m_nm;
    const string *m_sk;
};

} // Namespace confgui

#endif /* _CONFLINKRCL_H_INCLUDED_ */
