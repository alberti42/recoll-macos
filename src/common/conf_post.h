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

