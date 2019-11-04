//
// Created by Andrey on 11/3/19.
//

std::ifstream dictionaryFile;
std::string const default_dictionary = "words.txt";
std::string const default_listen_port = "2000";
std::string listenPortString;
std::string nameOfFile;
std::__1::vector<std::string> words;
int listenPort;

int onStart(int argc, char *argv[]){




/* Running from command line:
 * ProgramName [<File> <Port>]
 * If dictionary file or port are passed in through
 * command line, use them. Otherwise, use defaults.
 * Defaults: words.txt
 * argv1 file
 * argv2 port
 */

std::cout << ("\nOpening dictionary.");

    //user file, default port
    if(argc==2){
        dictionaryFile.open(argv[1]);
        if(dictionaryFile.fail()){
            std::cerr<<"Error opening file";
            exit(2);
        }
        listenPort = stoi(default_listen_port);
        nameOfFile=argv[1];}

    //user file user port
    else if(argc==3){
        nameOfFile=argv[1];
        dictionaryFile.open(argv[1]);
        if(dictionaryFile.fail()){
            std::cerr<<"Error opening file";
            exit(3);}

        listenPort = atoi(argv[2]);
        //We can't use ports below 1024 and ports above 65535 don't exist.
        if(listenPort < 1024 || listenPort > 65535){
            printf("Port number is either too low(below 1024), or too high(above 65535).\n");
            return -1;
        }

    }

    //default file, default port
    else{
        nameOfFile=default_dictionary;
        dictionaryFile.open(default_dictionary);
        if(dictionaryFile.fail()){
            std::cerr<<"Error opening file";
            exit(4);}
        listenPort = stoi(default_listen_port);

    }


    //A vector of type string will be used to
    // store dictionary in memory
    std::string line="";


    if(dictionaryFile.is_open()){

    std::cout << "\nFile open.";
    std::cout << "\nName of file: ";
    std::cout << nameOfFile;
        while(std::__1::getline(dictionaryFile, line)){
        words.push_back(line);
    }

    std::cout << "\nFirst 10 words:\n";

        for (int i = 0; i < 10; ++i) {
            std::cout << words[i];
            std::cout << "\n";
        }
    }



return 0;
}

#ifndef NETWORKSPELLCHECK_ONSTART_H
#define NETWORKSPELLCHECK_ONSTART_H

#endif //NETWORKSPELLCHECK_ONSTART_H
