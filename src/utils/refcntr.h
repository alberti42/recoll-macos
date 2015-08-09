#ifndef _REFCNTR_H_
#define _REFCNTR_H_

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
    void release()
    {
        if (pcount && --(*pcount) == 0) {
            delete rep;
            delete pcount;
        }
        rep = 0;
        pcount = 0;
    }
    void reset() {
        release();
    }
    ~RefCntr() 
    {
        release();
    }
    X *operator->() {return rep;}
    X *getptr() const {return rep;}
    X *get() const {return rep;}
    const X *getconstptr() const {return rep;}
    int getcnt() const {return pcount ? *pcount : 0;}
    bool isNull() const {return rep == 0;}
    bool isNotNull() const {return rep != 0;}
    operator bool() const {return rep != 0;}
};


#endif /*_REFCNTR_H_ */
