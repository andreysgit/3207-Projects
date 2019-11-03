#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include "onStart.h"
//#define LOG(x) std::cout << x << std::endl


int main(int argc, char *argv[]) {

    onStart(argc,argv);

    return 0;
}