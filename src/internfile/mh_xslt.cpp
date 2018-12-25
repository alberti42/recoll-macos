/* Copyright (C) 2005 J.F.Dockes 
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "autoconfig.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>

#include "cstr.h"
#include "mh_xslt.h"
#include "log.h"
#include "smallut.h"
#include "md5ut.h"
#include "rclconfig.h"
#include "readfile.h"

using namespace std;


class FileScanXML : public FileScanDo {
public:
    FileScanXML(const string& fn) : m_fn(fn) {}
    virtual ~FileScanXML() {
        if (ctxt) {
            xmlFreeParserCtxt(ctxt);
        }
    }

    xmlDocPtr getDoc() {
        int ret;
        if ((ret = xmlParseChunk(ctxt, nullptr, 0, 1))) {
            xmlError *error = xmlGetLastError();
            LOGERR("FileScanXML: final xmlParseChunk failed with error " <<
                   ret << " error: " <<
                   (error ? error->message :
                    " null return from xmlGetLastError()") << "\n");
            return nullptr;
        }
        return ctxt->myDoc;
    }

    virtual bool init(int64_t size, string *) {
        LOGDEB1("FileScanXML: init: size " << size << endl);
        ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, m_fn.c_str());
        if (ctxt == nullptr) {
            LOGERR("FileScanXML: xmlCreatePushParserCtxt failed\n");
            return false;
        } else {
            return true;
        }
    }
    
    virtual bool data(const char *buf, int cnt, string*) {
        if (0) {
            string dt(buf, cnt);
            LOGDEB1("FileScanXML: data: cnt " << cnt << " data " << dt << endl);
        } else {
            LOGDEB1("FileScanXML: data: cnt " << cnt << endl);
        }            
        int ret;
        if ((ret = xmlParseChunk(ctxt, buf, cnt, 0))) {
            xmlError *error = xmlGetLastError();
            LOGERR("FileScanXML: xmlParseChunk failed with error " <<
                   ret << " for [" << buf << "] error " <<
                   (error ? error->message :
                    " null return from xmlGetLastError()") << "\n");
            return false;
        } else {
            LOGDEB1("xmlParseChunk ok (sent " << cnt << " bytes)\n");
            return true;
        }
    }

private:
    xmlParserCtxtPtr ctxt{nullptr};
    string m_fn;
};

class MimeHandlerXslt::Internal {
public:
    ~Internal() {
        if (metaOrAllSS) {
            xsltFreeStylesheet(metaOrAllSS);
        }
        if (dataSS) {
            xsltFreeStylesheet(dataSS);
        }
    }
    bool ok{false};
    xsltStylesheet *metaOrAllSS{nullptr};
    xsltStylesheet *dataSS{nullptr};
    string result;
};

MimeHandlerXslt::~MimeHandlerXslt()
{
    delete m;
}

MimeHandlerXslt::MimeHandlerXslt(RclConfig *cnf, const std::string& id,
                                 const std::vector<std::string>& params)
    : RecollFilter(cnf, id), m(new Internal)
{
    LOGDEB("MimeHandlerXslt: params: " << stringsToString(params) << endl);
    string filtersdir = path_cat(cnf->getDatadir(), "filters");

    xmlSubstituteEntitiesDefault(0);
    xmlLoadExtDtdDefaultValue = 0;

    // params can be "xslt stylesheetall" or
    // "xslt metamember stylesheetmeta datamember stylesheetdata"
    if (params.size() == 2) {
        string ssfn = path_cat(filtersdir, params[1]);
        FileScanXML XMLstyle(ssfn);
        string reason;
        if (!file_scan(ssfn, &XMLstyle, &reason)) {
            LOGERR("MimeHandlerXslt: file_scan failed for style sheet " <<
                   ssfn << " : " << reason << endl);
            return;
        }
        xmlDoc *stl = XMLstyle.getDoc();
        if (stl == nullptr) {
            LOGERR("MimeHandlerXslt: getDoc failed for style sheet " <<
                   ssfn << endl);
            return;
        }
        m->metaOrAllSS = xsltParseStylesheetDoc(stl);
        if (m->metaOrAllSS) {
            m->ok = true;
        }
    } else if (params.size() == 4) {
    } else {
        LOGERR("MimeHandlerXslt: constructor with wrong param vector: " <<
               stringsToString(params) << endl);
    }
}

bool MimeHandlerXslt::set_document_file_impl(const std::string& mt, 
                                             const std::string &file_path)
{
    LOGDEB0("MimeHandlerXslt::set_document_file_: fn: " << file_path << endl);
    if (!m || !m->ok) {
        return false;
    }
    if (nullptr == m->dataSS) {
        if (nullptr == m->metaOrAllSS) {
            LOGERR("MimeHandlerXslt::set_document_file_impl: both ss empty??\n");
            return false;
        }
        FileScanXML XMLdoc(file_path);
        string md5, reason;
        if (!file_scan(file_path, &XMLdoc, 0, -1, &reason,
                       m_forPreview ? nullptr : &md5)) {
            LOGERR("MimeHandlerXslt::set_document_file_impl: file_scan failed "
                   "for " << file_path << " : " << reason << endl);
            return false;
        }
        if (!m_forPreview) {
            m_metaData[cstr_dj_keymd5] = md5;
        }
        xmlDocPtr doc = XMLdoc.getDoc();
        if (nullptr == doc) {
            LOGERR("MimeHandlerXslt::set_doc_file_impl: no parsed doc\n");
            return false;
        }
        xmlDocPtr transformed = xsltApplyStylesheet(m->metaOrAllSS, doc, NULL);
        if (nullptr == transformed) {
            LOGERR("MimeHandlerXslt::set_doc_file_: xslt transform failed\n");
            xmlFreeDoc(doc);
            return false;
        }
        xmlChar *outstr;
        int outlen;
        xsltSaveResultToString(&outstr, &outlen, transformed, m->metaOrAllSS);
        m->result = string((const char*)outstr, outlen);
        xmlFree(outstr);
        xmlFreeDoc(transformed);
        xmlFreeDoc(doc);
    } else {
        LOGERR("Not ready for multipart yet\n");
        abort();
    }
            
    m_havedoc = true;
    return true;
}

bool MimeHandlerXslt::set_document_string_impl(const string& mt, 
                                               const string& msgtxt)
{
    if (!m || !m->ok) {
        return false;
    }
    return true;
}

bool MimeHandlerXslt::next_document()
{
    if (!m || !m->ok) {
        return false;
    }
    if (m_havedoc == false)
	return false;
    m_havedoc = false;
    m_metaData[cstr_dj_keymt] = cstr_texthtml;
    m_metaData[cstr_dj_keycontent].swap(m->result);
    LOGDEB1("MimeHandlerXslt::next_document: result: [" <<
            m_metaData[cstr_dj_keycontent] << "]\n");
    return true;
}

void MimeHandlerXslt::clear_impl()
{
    m_havedoc = false;
    m->result.clear();
}
