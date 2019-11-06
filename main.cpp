#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#define BUF_LEN 512
#define JOB_MAX 3
#define NUM_WORKERS 3
#define LOG_MAX 3

pthread_mutex_t job_mutex, log_mutex;
pthread_cond_t log_add,log_remove;
pthread_cond_t job_add,job_remove;
std::ifstream dictionaryFile;
std::ifstream testDictionaryFile;
std::string const default_dictionary = "words.txt";
std::string const default_listen_port = "2000";
std::string nameOfFile;
std::vector<std::string> words;
std::vector<std::string> testwords;

std::string log_buf[LOG_MAX];

struct sockaddr_in client;
unsigned int clientLen = sizeof(client);
int connected_socket, clientSocket, bytesReturned;
int listenPort;
int job_buf[JOB_MAX];
int job_buffer_count=0;
int log_buffer_count=0;
int job_buffer_front=0;//front is for input
int log_buffer_front=0;
int job_buffer_rear=0; //rear is for output
int log_buffer_rear=0;

int testSpellCheck(){
    std::string testline;
    std::string userInput;
    std::string exitChar="x";
    std::cout<<"Choose a word.\nType x to exit\n";
    std::cin>>userInput;
    std::cout<<userInput;
    if (std::strcmp(userInput.data(),exitChar.data())==0){
        return 0;
    }
    //word_lookup searches string vector table for input string
    //until escape character is pressed
            testDictionaryFile.open(default_dictionary);
            if(testDictionaryFile.fail()){
                std::cerr<<"Error opening file";}

            if(testDictionaryFile.is_open()) {
                //init vector

                std::cout << "\n\nDictionary has been opened.\n";
                std::cout << "\nSearching for element: " << userInput << "\n";
                while (std::getline(testDictionaryFile, testline)) {
                    testwords.push_back(testline);
                }
            }
            testDictionaryFile.close();
            if(std::find(testwords.begin(),testwords.end(),userInput)!=testwords.end()){
                std::cout << "\nYour word has been found\n\n";
            }
            else{
                std::cout<< "\nNo match\n\n";
            }
    return 0;
}

int testInsertWhenFull(int socketfd){

        //producer: places jobs into queue
        pthread_mutex_lock(&job_mutex); //get job queue lock

        //test insert job when full
        if(job_buffer_count==JOB_MAX){
            job_buf[job_buffer_front]=socketfd;
            job_buffer_count++;}

        while(job_buffer_count==JOB_MAX){//while job buffer is full, wait
            pthread_cond_wait(&job_add,&job_mutex);
        }
        job_buf[job_buffer_front]=socketfd;
        job_buffer_count++;
        job_buffer_front++;
        if(job_buffer_front==JOB_MAX){
            job_buffer_front=0;
        }
        //ready for consumption
        pthread_cond_signal(&job_remove);
        //unlock
        pthread_mutex_unlock(&job_mutex);
    }

int testRemoveWhenEmpty(){
    //consumer
    int returnSocket;
    pthread_mutex_lock(&job_mutex);
    if(job_buffer_count==0){
        returnSocket = job_buf[job_buffer_rear];
        job_buffer_rear++;
    }
    while(job_buffer_count==0){
        pthread_cond_wait(&job_remove,&job_mutex);
    }

    if(job_buffer_rear==JOB_MAX){
        job_buffer_rear=0;
    }
    job_buffer_count--;
    pthread_cond_signal(&job_add);
    pthread_mutex_unlock(&job_mutex);
    std::string threadMessage("This thread fd: " + std::to_string(returnSocket) + "\n");
    send(clientSocket,threadMessage.data(),threadMessage.size(),0);
    return returnSocket;
}

void addToJobQueue(int socketfd){
    //producer: places jobs into queue
    pthread_mutex_lock(&job_mutex); //get job queue lock

    //test insert job when full
    if(job_buffer_count==JOB_MAX){
        testInsertWhenFull(socketfd);
    }

    while(job_buffer_count==JOB_MAX){//while job buffer is full, wait
        pthread_cond_wait(&job_add,&job_mutex);
    }
    job_buf[job_buffer_front]=socketfd;
    job_buffer_count++;
    job_buffer_front++;
    if(job_buffer_front==JOB_MAX){
        job_buffer_front=0;
    }
    //ready for consumption
    pthread_cond_signal(&job_remove);
    //unlock
    pthread_mutex_unlock(&job_mutex);
}
int processJobQueue(){
    //consumer
    int returnSocket;
    pthread_mutex_lock(&job_mutex);
    while(job_buffer_count==0){
        pthread_cond_wait(&job_remove,&job_mutex);
    }
    returnSocket = job_buf[job_buffer_rear];
    job_buffer_rear++;
    if(job_buffer_rear==JOB_MAX){
        job_buffer_rear=0;
    }
    job_buffer_count--;
    pthread_cond_signal(&job_add);
    pthread_mutex_unlock(&job_mutex);
    std::string threadMessage("This thread fd: " + std::to_string(returnSocket) + "\n");
    send(clientSocket,threadMessage.data(),threadMessage.size(),0);
    return returnSocket;
}
std::string processLogQueue(){
    //consumer: processes log events from queue

    std::string nextString;
    //lock mutex log queue
    pthread_mutex_lock(&log_mutex);
    //check if empty
    while(log_buffer_count==0){
        pthread_cond_wait(&log_remove,&log_mutex);
    }
    //fetch from queue
    nextString = log_buf[log_buffer_rear];
    log_buffer_rear++;
    //rollover
    if(log_buffer_rear==JOB_MAX){
        log_buffer_rear=0;
    }
    log_buffer_count--;
    //signal ready for production
    pthread_cond_signal(&log_add);
    //unlock mutex
    pthread_mutex_unlock(&log_mutex);
    return nextString;

}
void addToLogQueue(std::string word){
    //producer: puts log events into queue

    pthread_mutex_lock(&log_mutex); //get lock
    while(job_buffer_count==LOG_MAX){//while log buffer is full, wait
        pthread_cond_wait(&log_add,&log_mutex);
    }
    //do work
    log_buf[log_buffer_front]=word;
    log_buffer_count++;
    log_buffer_front++;
    //rolls over
    if(log_buffer_front==LOG_MAX){
        log_buffer_front=0;
    }
    //ready for consumption
    pthread_cond_signal(&log_remove);
    //unlocks log queue mutex
    pthread_mutex_unlock(&log_mutex);
}
bool word_lookup(std::string word){
    //Searches global vector table 'word' for input string
    std::cout << "\nSearching for element: " << word;
    if(std::find(words.begin(),words.end(),word)!=words.end()){
        std::cout << "\nElement found";
        return true;
    }
    else{
        return false;
    }
}
void* logger(void* redheadedsteparg){
    //performs logging duties
    std::string logEntry;
    std::ofstream logFile;
    logFile.open("log.txt",std::ios::out|std::ios::trunc);
    logFile.close();
    while(1){
        logFile.open("log.txt",std::ios::out|std::ios::app);
        logEntry=processLogQueue();
        logFile<<logEntry << "\n";
        logFile.close();
    }
}
void* worker(void* something) {
    //communicates with client and calls word lookup method

    //testing
    // std::cout << "\nWorker thread created\n";
    // std::cout << "\n thread: " << (char*)something << " is running";

    //preset messages to send to user
    const char* clientMessage = "Send a word for me to look up!\nPress escape to exit.\n";
    const char* msgError = "I didn't get your message. ):\n";
    const char* msgClose = "Goodbye!\n";

    std::string result;
    std::string response;
    int clientSocket;
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';

    //talks to client
    while(1){
        bzero(&recvBuffer, BUF_LEN);
        result.clear();
        clientSocket = processJobQueue();
        std::cout<<"\nServer is processing client id: " << clientSocket << "\n";
        send(clientSocket,clientMessage,strlen(clientMessage),0);
        //recv() will store the message from the user in the buffer, returning
        //how many bytes we received.
        bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);

        //Check if we got a message, send a message back or quit if the
        //user specified it.
        if(bytesReturned == -1){
            send(clientSocket, msgError, strlen(msgError), 0);
        }
        //'27' is the escape key.
        else if(recvBuffer[0] == 27){
            send(clientSocket, msgClose, strlen(msgClose), 0);
            std::cout<<"\nEscape character pressed, closing connection id: " << clientSocket << "\n";
            close(clientSocket);
            break;
        }
        //gets client word
        else{
            for(int i = 0; i < (strlen(recvBuffer)-2); i++){
                //testing
//                std::cout<<"\nresult = " << result;
//                std::cout<<"\nrecvBuffer[i] = " << recvBuffer[i] << "\n";
//                std::cout<<result;
//                std::cout<<"\nnew result = " << result;
                result+=recvBuffer[i];

            }

            //searches table for word
            bool correct = word_lookup(result);

            //testing
            //std::cout << "bool correct: " << correct;

            //dictionary contains user's word
            if(correct){
                result+= " eh?.. I found this word in my dictionary!\n";
                send(clientSocket,result.data(),result.size(),0);
                std::cout << "\nLogged to log.txt\n";
                addToLogQueue(result);
                std::cout << "\nClosing connection id " << clientSocket << "\n";
                close(clientSocket);
            }

            //dictionary does not contain user's word
            if(!correct){
                result+= " eh?.. I think this word is misspelled\n";
                send(clientSocket,result.data(),result.size(),0);
                std::cout << "\nLogged to log.txt\n";
                addToLogQueue(result);
                std::cout << "\nClosing connection id " << clientSocket << "\n";
                close(clientSocket);
            }
        }
    }

}
void init_pthreads(){ //necessary initialization step for threading
    pthread_mutex_init(&job_mutex,NULL);
    pthread_mutex_init(&log_mutex,NULL);
    pthread_cond_init(&log_add,NULL);
    pthread_cond_init(&log_remove,NULL);
    pthread_cond_init(&job_remove,NULL);
    pthread_cond_init(&job_add,NULL);
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

    init_pthreads();//necessary thread initialization

    std::cout << ("\nOpening dictionary...");

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

    //Loop through items in text file, putting them into <string>vector
    if(dictionaryFile.is_open()){
        std::cout << "\nDictionary has been opened!";
        std::cout << "\nName of dictionary file: " << nameOfFile << "\n";
        while(std::getline(dictionaryFile, line)){
            words.push_back(line);
        }
    }

    return 0;
}
int open_listenfd(int port)

{
    //This section of code straight from
    //Bryant O Hallaron book ch.11
    //"We find it helpful to combine the socket, bind, and listen functions into a
    //helper function called open_listenfd that a server can use to create a listening
    //descriptor."
    //open_listenfd opens/returns a listening descriptor that is
    //ready to receive connection requests on the well-known port

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

int testInsertWhenFull(int argc, char *argv[]){

    onStart(argc,argv);
    //Settings init: int listenPort, vector<string> words, dictionary file open


    //create number of worker & logger threads based on arbitrary NUM_WORKERS
    pthread_t workers[NUM_WORKERS];
    for(int i=0;i<NUM_WORKERS;i++){
        if(pthread_create(&workers[i],NULL,worker,NULL)!=0){
            std::cout << "\n Worker thread creation error: " << i << "\n";
        }

        //testing worker threads created
        // else{
        // std::cout <<"\nWorker thread " << i << " created successfully\n";
        // }

        if(pthread_create(&workers[i],NULL,logger,NULL)!=0){
            std::cout << "\n Logger thread creation error: " << i << "\n";
        }
        //testing logger threads created
        //else{
        // std::cout <<"\nLogger thread " << i << " created successfully\n";
        // }
    }

    //starts listening on port
    if ((connected_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }
    std::cout << "\nListening on port: " << listenPort << "\n";


    //accept() waits in a loop until a user connects to the server
    //writing information about that serv into the sockaddr_in client.
    while (true) {

        if((clientSocket = accept(connected_socket, (struct sockaddr*)&client, &clientLen)) == -1){
            printf("Error connecting to client.\n");
            return -1;
        }
        std::cout << "\nAccepted client " << clientSocket << "\n";
        testInsertWhenFull(clientSocket);
    }
}

int testRemoveWhenEmpty(int argc, char *argv[]){

    onStart(argc,argv);
    //Settings init: int listenPort, vector<string> words, dictionary file open


    //create number of worker & logger threads based on arbitrary NUM_WORKERS
    pthread_t workers[NUM_WORKERS];
    for(int i=0;i<NUM_WORKERS;i++){
        if(pthread_create(&workers[i],NULL,worker,NULL)!=0){
            std::cout << "\n Worker thread creation error: " << i << "\n";
        }

        //testing worker threads created
        // else{
        // std::cout <<"\nWorker thread " << i << " created successfully\n";
        // }

        if(pthread_create(&workers[i],NULL,logger,NULL)!=0){
            std::cout << "\n Logger thread creation error: " << i << "\n";
        }
        //testing logger threads created
        //else{
        // std::cout <<"\nLogger thread " << i << " created successfully\n";
        // }
    }

    //starts listening on port
    if ((connected_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }
    std::cout << "\nListening on port: " << listenPort << "\n";


    //accept() waits in a loop until a user connects to the server
    //writing information about that serv into the sockaddr_in client.
    while (true) {

        if((clientSocket = accept(connected_socket, (struct sockaddr*)&client, &clientLen)) == -1){
            printf("Error connecting to client.\n");
            return -1;
        }
        std::cout << "\nAccepted client " << clientSocket << "\n";
        testRemoveWhenEmpty();
    }
}
int testEchoServer(int argc, char *argv[]) {
    //a simple server that echos user message in return
    const char *clientMessage = "Send a word!\n";
    const char *msgError = "I didn't get your message\n";
    const char* msgClose = "Goodbye!\n";
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';
    std::string userString;
    int clientSocket;

    //can pass in a user port as second arg
    if (argc == 2) {
        listenPort = atoi(argv[1]);
        if (listenPort < 2000) {
            listenPort += 2000;
        }
        std::cout << argv[1];
        //We can't use ports below 1024 and ports above 65535 don't exist.
        if (listenPort < 1024 || listenPort > 65535) {
            printf("Port number is either too low(below 1024), or too high(above 65535).\n");
            return -1;
        }
    } else {
        listenPort = stoi(default_listen_port);
    }

    if ((connected_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }

    while (true) {

        //clear the buffer
        bzero(&recvBuffer, BUF_LEN);

        if((clientSocket = accept(connected_socket, (struct sockaddr*)&client, &clientLen)) == -1){
            printf("Error connecting to client.\n");
            return -1;
        }
        std::cout << "\nEcho client running\n";

        send(clientSocket,clientMessage,strlen(clientMessage),0);

        //Check if we got a message, send a message back or quit if the
        //user specified it.

        bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);

        if(bytesReturned == -1){
            send(clientSocket, msgError, strlen(msgError), 0);
        }

        //'27' is the escape key.
        else if(recvBuffer[0] == 27){
            send(clientSocket, msgClose, strlen(msgClose), 0);
            std::cout<<"\nEscape character pressed, closing connection id: " << clientSocket << "\n";
            close(clientSocket);
        }
        else{
            userString.clear();
            for(int i = 0; i < (strlen(recvBuffer)-2); i++){
                userString+=recvBuffer[i];
            }
            userString+="\n";

        send(clientSocket,userString.data(),userString.size(),0);
        userString.clear();
    }}
}

int normalFunctions(int argc, char *argv[]){

    onStart(argc,argv);
    //Settings init: int listenPort, vector<string> words, dictionary file open


    //create number of worker & logger threads based on arbitrary NUM_WORKERS
    pthread_t workers[NUM_WORKERS];
    for(int i=0;i<NUM_WORKERS;i++){
        if(pthread_create(&workers[i],NULL,worker,NULL)!=0){
            std::cout << "\n Worker thread creation error: " << i << "\n";
        }

        //testing worker threads created
        // else{
        // std::cout <<"\nWorker thread " << i << " created successfully\n";
        // }

        if(pthread_create(&workers[i],NULL,logger,NULL)!=0){
            std::cout << "\n Logger thread creation error: " << i << "\n";
        }
        //testing logger threads created
        //else{
        // std::cout <<"\nLogger thread " << i << " created successfully\n";
        // }
    }

    //starts listening on port
    if ((connected_socket = open_listenfd(listenPort)) < 0) {
        perror("Couldn't open listening socket");
        exit(EXIT_FAILURE);
    }
    std::cout << "\nListening on port: " << listenPort << "\n";


    //accept() waits in a loop until a user connects to the server
    //writing information about that serv into the sockaddr_in client.
    while (true) {

        if((clientSocket = accept(connected_socket, (struct sockaddr*)&client, &clientLen)) == -1){
            printf("Error connecting to client.\n");
            return -1;
        }
        std::cout << "\nAccepted client " << clientSocket << "\n";
        addToJobQueue(clientSocket);
    }
}

int main(int argc, char *argv[]) {

    while(1){
        std::cout<<"Choose a functionality:\n";
        std::cout<<"1:Normal function\n";
        std::cout<<"2:Echo Server\n";
        std::cout<<"3:Server tries to remove job while empty\n";
        std::cout<<"4:Server tries to add job while while\n";
        std::cout<<"5:Simple spell check\n";

        int userchoice;
        std::cin >> userchoice;
        if(userchoice==1){
            normalFunctions(argc, argv);
        }
        else if(userchoice==2){
            testEchoServer(argc,argv);
        }
        else if(userchoice==3){
            testRemoveWhenEmpty();

        }
        else if(userchoice==4){
            testInsertWhenFull(argc,argv);
        }
        else if(userchoice==5){
            testSpellCheck();
        }
        else{
            std::cout<<"user input error";
        }
    }


return 0;
}