#include <iostream>
#include <fstream>
#include <string>

#define LOG(x) std::cout << x << std::endl
using namespace std;

fstream dictionaryFile;
string const default_dictionary = "words.txt";
string const default_port = "127.0.0.1";
string port;


int main(int argc, char *argv[]) {
    LOG(argc);
    LOG(*argv[0]);

    /* Running from command line:
     * ProgramName [<File> <Port>]
     * If dictionary file or port are passed in through
     * command line, use them. Otherwise, use defaults.
     * Defaults: words.txt
     */
    if(argc==2){
        LOG("\nopening: ");
        LOG(argv[1]);
        dictionaryFile.open(argv[1]);
        port = default_port;
    }
    if(argc==3){
        dictionaryFile.open(argv[1]);
        port = argv[2];
    }
    else{
        dictionaryFile.open(default_dictionary);
        port = default_port;
    }


    return 0;
}