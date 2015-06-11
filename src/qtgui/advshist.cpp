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

#include "advshist.h"
#include "guiutils.h"
#include "debuglog.h"
#include "xmltosd.h"

using namespace std;
using namespace Rcl;

AdvSearchHist::AdvSearchHist()
{
    read();
    m_current = -1;
}

AdvSearchHist::~AdvSearchHist()
{
    for (vector<RefCntr<SearchData> >::iterator it = m_entries.begin();
	 it != m_entries.end(); it++) {
	it->release();
    }
}

RefCntr<Rcl::SearchData> AdvSearchHist::getnewest() 
{
    if (m_entries.empty())
        return RefCntr<Rcl::SearchData>();

    return m_entries[0];
}

RefCntr<Rcl::SearchData> AdvSearchHist::getolder()
{
    m_current++;
    if (m_current >= int(m_entries.size())) {
	m_current--;
	return RefCntr<Rcl::SearchData>();
    }
    return m_entries[m_current];
}

RefCntr<Rcl::SearchData> AdvSearchHist::getnewer()
{
    if (m_current == -1 || m_current == 0 || m_entries.empty())
	return RefCntr<Rcl::SearchData>();
    return m_entries[--m_current];
}

bool AdvSearchHist::push(RefCntr<SearchData> sd)
{
    m_entries.insert(m_entries.begin(), sd);
    if (m_current != -1)
	m_current++;

    string xml = sd->asXML();
    g_dynconf->enterString(advSearchHistSk, xml, 100);
    return true;
}

bool AdvSearchHist::read()
{
    if (!g_dynconf)
	return false;
    list<string> lxml = g_dynconf->getStringList(advSearchHistSk);
    
    for (list<string>::const_iterator it = lxml.begin(); it != lxml.end();
	 it++) {
        RefCntr<SearchData> sd = xmlToSearchData(*it);
        if (sd)
            m_entries.push_back(sd);
    }
    return true;
}

void AdvSearchHist::clear()
{
    g_dynconf->eraseAll(advSearchHistSk);
}
