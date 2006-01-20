#ifndef _DEBUGLOG_H_
#define _DEBUGLOG_H_
/* Macros for log and debug messages */
#include <stack>

#ifndef NO_NAMESPACES
namespace DebugLog {
    using std::stack;
#endif // NO_NAMESPACES

#ifndef DEBUGLOG_USE_THREADS
#define DEBUGLOG_USE_THREADS 1
#endif

#define DEBFATAL 1
#define DEBERR   2
#define DEBINFO  3
#define DEBDEB   4
#define DEBDEB0  5
#define DEBDEB1  6
#define DEBDEB2  7
#define DEBDEB3  8

#ifndef STATICVERBOSITY
#define STATICVERBOSITY DEBDEB0
#endif

class DebugLogWriter;

class DebugLog {
    std::stack<int> levels;
    int debuglevel;
    int dodate;
    DebugLogWriter *writer;
  public:
    DebugLog() : debuglevel(10), dodate(0), writer(0) {}
    DebugLog(DebugLogWriter *w) : debuglevel(-1), dodate(0), writer(w) {}
    virtual ~DebugLog() {}
    virtual void setwriter(DebugLogWriter *w) {writer = w;}
    virtual DebugLogWriter *getwriter() {return writer;}
    virtual void prolog(int lev, const char *srcfname, int line);
    virtual void log(const char *s ...);
    virtual void setloglevel(int lev);
    inline int getlevel() {return debuglevel;}
    virtual void pushlevel(int lev);
    virtual void poplevel();
    virtual void logdate(int onoff) {dodate = onoff;}
};

extern DebugLog *getdbl();
extern const char *getfilename();
extern int setfilename(const char *fname, int trnc = 1);
#if STATICVERBOSITY >= DEBFATAL
#define LOGFATAL(X) {if (DebugLog::getdbl()->getlevel()>=DEBFATAL){DebugLog::getdbl()->prolog(DEBFATAL,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGFATAL(X)
#endif
#if STATICVERBOSITY >= DEBERR
#define LOGERR(X) {if (DebugLog::getdbl()->getlevel()>=DEBERR){DebugLog::getdbl()->prolog(DEBERR,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGERR(X)
#endif
#if STATICVERBOSITY >= DEBINFO
#define LOGINFO(X) {if (DebugLog::getdbl()->getlevel()>=DEBINFO){DebugLog::getdbl()->prolog(DEBINFO,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGINFO(X)
#endif
#if STATICVERBOSITY >= DEBDEB
#define LOGDEB(X) {if (DebugLog::getdbl()->getlevel()>=DEBDEB){DebugLog::getdbl()->prolog(DEBDEB,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGDEB(X)
#endif
#if STATICVERBOSITY >= DEBDEB0
#define LOGDEB0(X) {if (DebugLog::getdbl()->getlevel()>=DEBDEB0){DebugLog::getdbl()->prolog(DEBDEB0,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGDEB0(X)
#endif
#if STATICVERBOSITY >= DEBDEB1
#define LOGDEB1(X) {if (DebugLog::getdbl()->getlevel()>=DEBDEB1){DebugLog::getdbl()->prolog(DEBDEB1,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGDEB1(X)
#endif
#if STATICVERBOSITY >= DEBDEB2
#define LOGDEB2(X) {if (DebugLog::getdbl()->getlevel()>=DEBDEB2){DebugLog::getdbl()->prolog(DEBDEB2,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGDEB2(X)
#endif
#if STATICVERBOSITY >= DEBDEB3
#define LOGDEB3(X) {if (DebugLog::getdbl()->getlevel()>=DEBDEB3){DebugLog::getdbl()->prolog(DEBDEB3,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGDEB3(X)
#endif
#if STATICVERBOSITY >= DEBDEB4
#define LOGDEB4(X) {if (DebugLog::getdbl()->getlevel()>=DEBDEB4){DebugLog::getdbl()->prolog(DEBDEB4,__FILE__,__LINE__) ;DebugLog::getdbl()->log X;}}
#else
#define LOGDEB4(X)
#endif
#ifndef NO_NAMESPACES
}
#endif // NO_NAMESPACES
#endif /* _DEBUGLOG_H_ */
