#include "utf8fn.h"
#include "rclconfig.h"
#include "transcode.h"
#include "log.h"

using namespace std;

string compute_utf8fn(const RclConfig *config, const string& ifn, bool simple)
{
    string charset = config->getDefCharset(true);
    string utf8fn; 
    int ercnt;
    string lfn(simple ? path_getsimple(ifn) : ifn);
    if (!transcode(lfn, utf8fn, charset, "UTF-8", &ercnt)) {
	LOGERR("compute_utf8fn: fn transcode failure from ["  << charset <<
               "] to UTF-8 for: [" << lfn << "]\n");
    } else if (ercnt) {
	LOGDEB("compute_utf8fn: "  << ercnt << " transcode errors from [" <<
               charset << "] to UTF-8 for: ["  << lfn << "]\n");
    }
    LOGDEB1("compute_utf8fn: transcoded from ["  << lfn << "] to ["  <<
            utf8fn << "] ("  << charset << "->"  << "UTF-8)\n");
    return utf8fn;
}
