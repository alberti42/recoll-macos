#ifndef _REFCNTR_H_
#define _REFCNTR_H_
/* Copyright (C) 2014 J.F.Dockes
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

// See Stroustrup C++ 3rd ed, p. 783
// This is only used if std::shared_ptr is not available
template <class X> class RefCntr {
    X   *rep;
    int *pcount;
public:
    RefCntr() 
        : rep(0), pcount(0) 
    {}
    explicit RefCntr(X *pp) 
        : rep(pp), pcount(new int(1)) 
    {}
    RefCntr(const RefCntr &r) 
        : rep(r.rep), pcount(r.pcount) 
    { 
        if (pcount)
            (*pcount)++;
    }
    RefCntr& operator=(const RefCntr& r) 
    {
        if (rep == r.rep) 
            return *this;
        if (pcount && --(*pcount) == 0) {
            delete rep;
            delete pcount;
        }
        rep = r.rep;
        pcount = r.pcount;
        if (pcount)
            (*pcount)++;
        return  *this;
    }
    void reset() {
        if (pcount && --(*pcount) == 0) {
            delete rep;
            delete pcount;
        }
        rep = 0;
        pcount = 0;
    }
    ~RefCntr() 
    {
        reset();
    }
    X *operator->() {return rep;}
    X *get() const {return rep;}
    int use_count() const {return pcount ? *pcount : 0;}
    operator bool() const {return rep != 0;}
};

#endif /*_REFCNTR_H_ */
