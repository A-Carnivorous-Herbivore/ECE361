//7.3 Beej's Guide

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"   // port we're listening on

//integer codes for different control packets 
#define LOGIN 0
#define LO_ACK  1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NACK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10
#define QUERY 11
#define QU_ACK 12

#define MAX_NAME 100
#define MAX_DATA 100

struct message { 
 unsigned int type; 
 unsigned int size; 
 unsigned char source[MAX_NAME]; 
 unsigned char data[MAX_DATA]; 
}; 

//hard coded usernames and passwords
char* users[] = {"Weihang", "Rudy", "Eric"};
char* passwords[] = {"1005910383", "1005734975", "1005865789"};
int connected[] = {0, 0, 0};
int inSession[] = {0, 0, 0};
int id_to_fd_mapping[] = {0, 0, 0};

//linked list of sessions, default int values are 0
struct sessNode{
    unsigned char sessName[MAX_NAME]; 
    int members[3]; //represents whether or not a user is in a session
    struct sessNode* next;
    int invalid; //represents whether a not is still valid or not, lazy delete
};

struct sessNode* head;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv)
{

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {


                        //START EVERYTHING I WROTE
                        struct message m;
                        //deserialize_message(&message);
                        //if user not recognized continue with error
                        int k;
                        int id = -1;
                        for(k = 0; k < 3; k++){
                            if(strcmp(users[k], m.source) == 0){
                                id = k;
                                break;
                            }
                        }
                        //set id to map to fd
                        id_to_fd_mapping[id] = i;

                        if(id == -1){
                            printf("user %s not found\n", m.source);
                            continue;
                        }

                        //if user is not logged and the command isn't login, continue with error
                        if(m.type != LOGIN && !connected[id]){
                            printf("user %s is not logged in\n", users[id]);
                            continue;
                        }

                        //response is always sent by server
                        struct message response;
                        strcpy(response.source, "server");

                        //formulate appropriate control packets
                        if(m.type == LOGIN){
                            if(connected[id]){
                                response.type = LO_NAK;
                                char* msg = "user is already logged in\n";
                                printf(msg);
                                strcpy(response.data, msg);
                            }
                            else if(strcmp(passwords[id], m.data) != 0){
                                response.type = LO_NAK;
                                char* msg = "incorrect password\n";
                                printf(msg);
                                strcpy(response.data, msg);
                            }else{//successful login
                                response.type = LO_ACK;
                                printf("successful login for user\n");
                                connected[id] = 1;
                            }
                            
                        }else if(m.type == EXIT){
                            //reset connections for that user
                            connected[id] = 0;
                            inSession[id] = 0;
                            struct sessNode* ptr = head;
                            while(ptr){
                                if(!ptr->invalid && ptr->members[id] == 1){                                  
                                    ptr->members[id] = 0;
                                    //make node invalid if all members left
                                    if(ptr->members[0] == 0 && ptr->members[1] == 0 && ptr->members[2] == 0)
                                        ptr->invalid = 1;
                                    break;
                                }
                                ptr = ptr->next;
                            }
                            printf("successful logout for user\n");
                        }

                        else if(m.type == JOIN){
                            //if already in session invalid request
                            if(inSession[id] == 1){
                                response.type = JN_NACK;
                                char* msg = "user already in session\n";
                                printf(msg);
                                strcpy(response.data, msg);
                            }else{
                                //find the appropriate session to join
                                struct sessNode* ptr = head;
                                while(ptr){
                                    if(!ptr->invalid && strcmp(m.data, ptr->sessName) == 0){
                                        ptr->members[id] = 1;
                                        inSession[id] = 1;
                                        break;
                                    }
                                    ptr = ptr->next;
                                }
                                //if successfully found session to join
                                if(inSession[id] == 1){
                                    response.type = JN_ACK;
                                    printf("succcesfully joined session\n");
                                }else{
                                    response.type = JN_NACK;
                                    char* msg = "didn't find the session name\n";
                                    printf(msg);
                                    strcpy(response.data, msg);
                                }
                            }
                        }

                        else if(m.type == LEAVE_SESS){
                            //user not in session, nack not needed
                            if(inSession[id] == 0){
                                printf("user not in session\n");
                                continue;
                            }
                            //reset connections for that user
                            inSession[id] = 0;
                            struct sessNode* ptr = head;
                            while(ptr){
                                if(!ptr->invalid && ptr->members[id] == 1){                            
                                    ptr->members[id] = 0;
                                    //make node invalid if all members left
                                    if(ptr->members[0] == 0 && ptr->members[1] == 0 && ptr->members[2] == 0)
                                        ptr->invalid = 1;
                                    break;                                   
                                }
                                ptr = ptr->next;
                            }
                            printf("successfully left session");
                        }


                        else if(m.type == NEW_SESS){
                            //already in session, nack not needed
                            if(inSession[id] == 1){
                                printf("user already in session\n");
                                continue;
                            }
                            //create a new session with given name, duplicate names not handled
                            struct sessNode* session = (struct sessNode*)malloc(sizeof(struct sessNode*));
                            strcpy(session->sessName, m.data);
                            //creating user is automatically part of the session
                            session->members[id] = 1;
                            inSession[id] = 1;
                            //attach session to end of list
                            struct sessNode* ptr = head;
                            ptr = session;

                            response.type = NS_ACK;
                            printf("successfully created and joined session\n");
                        }


                        else if(m.type == MESSAGE){
                            //user not in session, nack not needed
                            if(inSession[id] == 0){
                                printf("user not in session\n");
                                continue;
                            }

                            //find session with user in it
                            struct sessNode* ptr = head;
                            while(ptr){
                                if(!ptr->invalid && ptr->members[id] == 1){                                
                                    //broadcast to other users
                                    int j;
                                    for(j = 0; j < 3 && j != id; j++){
                                        //if another member is also in this session
                                        if(ptr->members[j] == 1){
                                            if (send(id_to_fd_mapping[j], buf, nbytes, 0) == -1)
                                                perror("send");
                                        }
                                    } 
                                    break;                                  
                                }
                                ptr = ptr->next;
                            }
                            printf("message sent\n");


                        }else if(m.type == QUERY){
                            response.type = QU_ACK;
                            char* msg = "Users: Weihang Rudy Eric\n Sessions:";
            
                            //gather all session names in addition to user names
                            struct sessNode* ptr = head;
                            while(ptr){
                                if(!ptr->invalid)
                                    sprintf("%s %s", msg, ptr->sessName);
                                ptr = ptr->next;
                            }
                            //gather appropriate stuff to send back
                            printf(msg);
                            strcpy(response.data, msg);
                        }


                        else{
                            //command not found, nack not needed
                            printf("command number %d not found\n", m.type);
                            continue;
                        }

                        //sent out appropriate acks or responses
                        //i is the socket that we are sending back (except for broadcast)
                        if(m.type == LOGIN || m.type == JOIN || m.type == NEW_SESS || m.type == QUERY){
                            response.size = sizeof(response.data);
                            //buf = serialize_message(response);
                            if (send(i, buf, nbytes, 0) == -1) 
                                        perror("send");
                        }
                        
                        //END EVERYTHING I WROTE
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}