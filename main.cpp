#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "onStart.h"
#include "simpleserver.h"


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

    //Create a listening socket on the specified port
    int listen_socket;
    if ((listen_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }





    return 0;
}

//#define LOG(x) std::cout << x << std::endl
