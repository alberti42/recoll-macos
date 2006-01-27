#ifndef _CANCELCHECK_H_INCLUDED_
#define _CANCELCHECK_H_INCLUDED_
/* @(#$Id: cancelcheck.h,v 1.1 2006-01-27 13:43:31 dockes Exp $  (C) 2005 J.F.Dockes */


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
 private:
    bool cancelRequested;

    CancelCheck() : cancelRequested(false) {}
    CancelCheck& operator=(CancelCheck&);
    CancelCheck(const CancelCheck&);
};

#endif /* _CANCELCHECK_H_INCLUDED_ */
