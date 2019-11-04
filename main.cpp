#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "simpleserver.h"

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

    std::ifstream dictionaryFile;
    std::string const default_dictionary = "words.txt";
    std::string const default_listen_port = "2000";
    std::string listenPortString;
    std::string nameOfFile;
    std::__1::vector<std::string> words;
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


//This section of code straight from
//Bryant O Hallaron book ch.11
//"We find it helpful to combine the socket, bind, and listen functions into a
//helper function called open_listenfd that a server can use to create a listening
//descriptor."

//open_listenfd opens and returns a listening descriptor that is
//ready to receive connection requests on the well-known port
int open_listenfd(int port)
{
    int listenq=20;
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    /* Eliminates "Address already in use" error from bind */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0){
        return -1;
    }

    /* Listenfd will be an end point for all requests to port
      all requests to port on any IP address for this host */


    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
        return -1;
    }

    //Prepare the socket to allow accept() calls. The value 20 is
    //the backlog, this is the maximum number of connections that will be placed
    //on queue until accept() is called again.

    if (listen(listenfd, listenq) < 0){
        return -1;
    }

    return listenfd;
}


int main(int argc, char *argv[]) {

    onStart(argc,argv); //Settings init: int listenPort, vector<string> words
    std::cout << "\nListening on port: ";
    std::cout << listenPort;
    std::cout << "\n";

    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connectionSocket, clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';

    //Create a listening socket on the specified port
    int listen_socket;
    if ((listen_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }





    return 0;
}

//#define LOG(x) std::cout << x << std::endl
