#ifdef  HAVE_CXX0X_UNORDERED
#  define UNORDERED_MAP_INCLUDE <unordered_map>
#  define UNORDERED_SET_INCLUDE <unordered_set>
#  define STD_UNORDERED_MAP std::unordered_map
#  define STD_UNORDERED_SET std::unordered_set
#elif defined(HAVE_TR1_UNORDERED)
#  define UNORDERED_MAP_INCLUDE <tr1/unordered_map>
#  define UNORDERED_SET_INCLUDE <tr1/unordered_set>
#  define STD_UNORDERED_MAP std::tr1::unordered_map
#  define STD_UNORDERED_SET std::tr1::unordered_set
#else
#  define UNORDERED_MAP_INCLUDE <map>
#  define UNORDERED_SET_INCLUDE <set>
#  define STD_UNORDERED_MAP std::map
#  define STD_UNORDERED_SET std::set
#endif

#ifdef HAVE_SHARED_PTR_STD
#  define MEMORY_INCLUDE <memory>
#  define STD_SHARED_PTR    std::shared_ptr
#elif defined(HAVE_SHARED_PTR_TR1)
#  define MEMORY_INCLUDE <tr1/memory>
#  define STD_SHARED_PTR    std::tr1::shared_ptr
#else
#  define MEMORY_INCLUDE "refcntr.h"
#  define STD_SHARED_PTR    RefCntr
#endif

#ifdef _WIN32
#include "safewindows.h"

#undef RCL_ICONV_INBUF_CONST

#ifdef _MSC_VER
// gmtime is supposedly thread-safe on windows
#define gmtime_r(A, B) gmtime(A)
#define localtime_r(A,B) localtime(A)
typedef int mode_t;
#define fseeko _fseeki64
#define ftello (off_t)_ftelli64
#define ftruncate _chsize_s
#define PATH_MAX MAX_PATH
#define RCL_ICONV_INBUF_CONST 1
#else
// Gminw
#undef RCL_ICONV_INBUF_CONST
#endif


typedef int pid_t;
inline int readlink(const char *cp, void *buf, int cnt) {
	return -1;
}
#define HAVE_STRUCT_TIMESPEC
#define strdup _strdup
#define timegm _mkgmtime


#define MAXPATHLEN PATH_MAX
typedef DWORD32 u_int32_t;
typedef DWORD64 u_int64_t;
typedef unsigned __int8 u_int8_t;
typedef int ssize_t;
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define chdir _chdir

#define R_OK 4
#define W_OK 2
#define X_OK 4
#define RECOLL_DATADIR "C:\\recoll\\"
#define S_ISLNK(X) false
#define lstat stat
#define timegm _mkgmtime
#endif


