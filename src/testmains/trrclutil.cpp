
#include "rclutil.h"


void path_to_thumb(const string& _input)
{
    string input(_input);
    // Make absolute path if needed
    if (input[0] != '/') {
        input = path_absolute(input);
    }

    input = string("file://") + path_canon(input);

    string path;
    //path = url_encode(input, 7);
    thumbPathForUrl(input, 7, path);
    cout << path << endl;
}

const char *thisprog;

int main(int argc, const char **argv)
{
    thisprog = *argv++;
    argc--;

    string s;
    vector<string>::const_iterator it;

#if 0
    if (argc > 1) {
        cerr <<  "Usage: thumbpath <filepath>" << endl;
        exit(1);
    }
    string input;
    if (argc == 1) {
        input = *argv++;
        if (input.empty())  {
            cerr << "Usage: thumbpath <filepath>" << endl;
            exit(1);
        }
        path_to_thumb(input);
    } else {
        while (getline(cin, input)) {
            path_to_thumb(input);
        }
    }
    exit(0);
#endif
}
