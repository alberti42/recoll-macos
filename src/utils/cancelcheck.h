/*
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
#ifndef _CANCELCHECK_H_INCLUDED_
#define _CANCELCHECK_H_INCLUDED_
/* @(#$Id: cancelcheck.h,v 1.3 2007-07-12 13:41:54 dockes Exp $  (C) 2005 J.F.Dockes */


class CancelExcept {};

class CancelCheck {
 public:
    static CancelCheck& instance() {
	static CancelCheck ck;
	return ck;
    }
    void setCancel(bool on = true) {
	cancelRequested = on;
    }
    void checkCancel() {
	if (cancelRequested) {
	    cancelRequested = false;
	    throw CancelExcept();
	}
    }
    bool cancelState() {return cancelRequested;}
 private:
    bool cancelRequested;

    CancelCheck() : cancelRequested(false) {}
    CancelCheck& operator=(CancelCheck&);
    CancelCheck(const CancelCheck&);
};

#endif /* _CANCELCHECK_H_INCLUDED_ */
