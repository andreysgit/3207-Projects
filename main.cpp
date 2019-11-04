#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>
#include <stdio.h>
#define BUF_LEN 512
#define JOB_MAX 20
#define NUM_WORKERS 10
#define LOG_MAX 20

pthread_mutex_t job_mutex, log_mutex;
pthread_cond_t log_wait,log_signal;
pthread_cond_t job_wait, job_signal;
std::ifstream dictionaryFile;
std::string const default_dictionary = "words.txt";
std::string const default_listen_port = "2000";
std::string listenPortString;
std::string nameOfFile;
std::__1::vector<std::string> words;
std::string log_buf[LOG_MAX];

int listenPort;
int num_workers;
int job_buf[JOB_MAX];
int job_buffer_count=0;
int log_buffer_count=0;
int job_buffer_front=0;//front is for input
int log_buffer_front=0;
int job_buffer_rear=0; //rear is for output
int log_buffer_rear=0;

void addToJobQueue(int socketfd){
    //producer
    pthread_mutex_lock(&job_mutex); //get lock
    while(job_buffer_count==JOB_MAX){//while job buffer is full, wait
        pthread_cond_wait(&job_wait,&job_mutex);
    }
    job_buf[job_buffer_front]=socketfd;
    job_buffer_count++;
    if(job_buffer_front==JOB_MAX){
        job_buffer_front=0;
    }

    pthread_cond_signal(&job_signal);
    pthread_mutex_unlock(&job_mutex);
}

int processJobQueue(){
    //consumer

    int returnSocket = NULL;
    pthread_mutex_lock(&job_mutex);
    while(job_buffer_count==0){
        pthread_cond_wait(&job_wait,&job_mutex);
    }
    returnSocket = job_buf[job_buffer_rear];
    job_buffer_rear++;
    if(job_buffer_rear==JOB_MAX){
        job_buffer_rear=0;
    }
    job_buffer_count--;

}

void addToLogQueue(std::string word){
    pthread_mutex_lock(&log_mutex); //get lock
    while(job_buffer_count==JOB_MAX){//while log buffer is full, wait
        pthread_cond_wait(&log_wait,&log_mutex);
    }
    log_buf[log_buffer_front]=word;
    log_buffer_count++;
    if(log_buffer_front==LOG_MAX){
        log_buffer_front=0;
    }

    pthread_cond_signal(&log_signal);
    pthread_mutex_unlock(&log_mutex);
}

void init_pthreads(){
pthread_mutex_init(&job_mutex,NULL);
pthread_mutex_init(&log_mutex,NULL);
pthread_cond_init(&log_wait,NULL);
pthread_cond_init(&log_signal,NULL);
pthread_cond_init(&job_signal,NULL);
pthread_cond_init(&job_wait,NULL);

}

bool word_lookup(std::string word){
    //Searches vector table for input string
    std::cout<<"\nSearching for element: " << word;
    if(std::find(words.begin(),words.end(),word)!=words.end()){
        std::cout << "\nElement found";
        return true;
    }
    else{
        std::cout<< "\nno match";
        return false;
    }
}

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

        std::cout << "\nDictionary has been opened.";
        std::cout << "\nName of dictionary file: ";
        std::cout << nameOfFile;
        while(std::__1::getline(dictionaryFile, line)){
            words.push_back(line);
        }

//        used for early testing
//        std::cout << "\nFirst 10 words:\n";
//
//        for (int i = 0; i < 10; ++i) {
//            std::cout << words[i];
//            std::cout << "\n";
//        }
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

    onStart(argc,argv);
    //Settings init: int listenPort, vector<string> words, dictionary file open
    init_pthreads();

    word_lookup("bananas");


    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connected_socket, clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';

    if ((connected_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }
        std::cout << "\nListening on port: ";
        std::cout << listenPort;
        std::cout << "\n";

    while (true) {

        //accept() waits until a user connects to the server, writing information about that server
        //into the sockaddr_in client.
        //If the connection is successful, we obtain A SECOND socket descriptor.
        //There are two socket descriptors being used now:
        //One by the server to listen for incoming connections.
        //The second that was just created that will be used to communicate with
        //the connected user.

        if((clientSocket = accept(connected_socket, (struct sockaddr*)&client, reinterpret_cast<socklen_t *>(&clientLen))) == -1){

            printf("Error connecting to client.\n");
            return -1;
        }

        const char* clientMessage = "Hello! I hope you can see this.\n";
        const char* msgRequest = "Send me some text and I'll respond with something interesting!\nSend the escape key to close the connection.\n";
        const char* msgResponse = "I actually don't have anything interesting to say...but I know you sent ";
        const char* msgPrompt = ">>>";
        const char* msgError = "I didn't get your message. ):\n";
        const char* msgClose = "Goodbye!\n";

        //send()...sends a message.
        //We specify the socket we want to send, the message and it's length, the
        //last parameter are flags.
        send(clientSocket, clientMessage, strlen(clientMessage), 0);
        send(clientSocket, msgRequest, strlen(msgRequest), 0);


//        add connected_socket to the work queue;
//        signal any sleeping workers that there's a new socket in the queue;
    }



        //Create a listening socket on the specified port


        //Begin sending and receiving messages.
//        while(1){
//            send(clientSocket, msgPrompt, strlen(msgPrompt), 0);
//            //recv() will store the message from the user in the buffer, returning
//            //how many bytes we received.
//            bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);
//
//            //Check if we got a message, send a message back or quit if the
//            //user specified it.
//            if(bytesReturned == -1){
//                send(clientSocket, msgError, strlen(msgError), 0);
//            }
//                //'27' is the escape key.
//            else if(recvBuffer[0] == 27){
//                send(clientSocket, msgClose, strlen(msgClose), 0);
//                close(clientSocket);
//                break;
//            }
//            else{
//                send(clientSocket, msgResponse, strlen(msgResponse), 0);
//                send(clientSocket, recvBuffer, bytesReturned, 0);
//            }
//        }


        return 0;
    }

//#define LOG(x) std::cout << x << std::endl
