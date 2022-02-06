#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

char* serverIP = "128.100.13.241";

//Some of the following functions have taken ideas from Beej's Guide
int main(int arg, char** argc){
	if(arg != 2){
		printf("Parameter numbers do not match.");
		exit(1);
	}
	if(strcmp(argc[0], "server")){
		printf("Incorrect format of invoking");
		exit(1);
	}
	int sockfd; //new_fd;	//Listen socket, and process socket.
	struct addrinfo hints;
	struct addrinfo* serverinfo;
	struct addrinfo* p;
	unsigned long serverIPAddr;
	inet_pton(AF_INET,serverIP, &serverIPAddr);
	memset(&hints,0,sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	socklen_t addr_len;
	struct sockaddr_storage their_addr;
	int rv;
	if((rv = getaddrinfo(NULL, argc[1], &hints, &serverinfo) != 0)){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	for(p = serverinfo; p != NULL; p = p->ai_next){
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("listener: socket");
			continue;
		}
		//fprintf(stderr,"HEY");	
		if((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1){
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}
	
	if(p == NULL){
		fprintf(stderr, "listener: failed to bind socket \n");
		return 2;
	}

	freeaddrinfo(serverinfo);
	printf("listener: waiting to recvfrom...\n");
	
	char buffer[100];
	addr_len = sizeof their_addr;
	int numbytes;
	if((numbytes = recvfrom(sockfd,buffer, 99, 0, (struct sockaddr*)&their_addr, &addr_len)) == -1){
		perror("recvfrom");
		exit(1);
	}
	buffer[numbytes] = '\0';
	char temp[] = "ftp";
	char reply[10];
	if(!strcmp(buffer,temp)){
		printf("Received.\n");
		strcpy(reply,"Yes");
	}else{
		printf("Wrong.\n");
		strcpy(reply,"No");
	}
	if((numbytes = sendto(sockfd, reply,strlen(reply),0, (const struct sockaddr*) &their_addr, addr_len)) == -1){
		perror("send error");
		exit(1);
	}
	//printf("listener:got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s));
	return 0;
}
