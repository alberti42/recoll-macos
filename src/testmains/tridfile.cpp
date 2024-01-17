#include "idfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <iostream>

using namespace std;

#include "log.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        cerr << "Usage: idfile filename" << endl;
        exit(1);
    }
    Logger::getTheLog("")->setLogLevel(Logger::LLDEB1);
    for (int i = 1; i < argc; i++) {
        string mime = idFile(argv[i]);
        cout << argv[i] << " : " << mime << endl;
    }
    exit(0);
}
