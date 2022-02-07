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
struct packet{
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};
char* serverIP = "128.100.13.241";
void decodePacket(char* buffer, struct packet* result){
	int objectNumber = 0;
	int startFrom = 0;
	int sectionSize = 0;
	//while(1){
	for(int i = 0;;i++){
		char temp[1000];		//Max number at 1000
		if(buffer[i] == ':'){		//Found division, [startFrom, i]
			strncpy(temp, buffer+startFrom, sectionSize);
			temp[sectionSize] = '\0';		//In case of corruption
			sectionSize = 0;			//Reset section size
			startFrom = startFrom + sectionSize + 1;	//Skip the :
			objectNumber++;				//Update number of found items
			if(objectNumber == 1){
				result->total_frag = atoi(temp);
			}else if(objectNumber == 2){
				result->frag_no = atoi(temp);
			}else if(objectNumber == 3){
				result->size = atoi(temp);
			}else if(objectNumber == 4){
				result->filename= (char*)malloc(sizeof(char)*(strlen(temp)+1));
				strncpy(result->filedata, buffer + startFrom, sizeof(char) * result->size);
				(result->filedata)[result->size] = '\0';
				return ;
			}
		}else{
			sectionSize++;
			
		}
	}
}
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
	char recvBuffer[2000];
	int packetReceived = 0;
	struct packet* result = (struct packet*)malloc(sizeof(struct packet));
	while( packetReceived == 0 || result->frag_no != result->total_frag){
		char msg[10];
		if(recvfrom(sockfd, recvBuffer, 1999, 0, (struct sockaddr*)&their_addr, &addr_len) == -1){
			strcpy(msg,"BAD");
			msg[3] = '\0';
			numbytes = sendto(sockfd, msg,strlen(msg),0, (const struct sockaddr*) &their_addr, addr_len);
			perror("BADBAD");
			exit(1);
		}
		decodePacket(recvBuffer,result);
		strcpy(msg, "BAD");
		msg[3] = '\0';
		numbytes = sendto(sockfd, msg, strlen(msg), 0, (const struct sockaddr*)&their_addr, addr_len);		//Send ACK to msg, needs to create and write to file. Every packet is stored in result.
			
	}
	return 0;

	
	/*FILE *f;

	int status = 1;
	char *response = "ACK"; 

	while(status){
		//recieving packets
	}

	//writing to file
	if(frag_no==1){
		f = fopen(filename, "w");
	}
	fwrite(filedata, 1, size, f);

	if(frag_no!=total_frag){
        	if ((numbytes = sendto(socketfd, response, strlen(response), 0, (const struct sockaddr *) their_addr, addr_len)) == -1){
			perror("server: sendto");
                	exit(1);
		}
	
	}
	if(frag_no!=total_frag){    
		flag = 0;  
	}

	fclose(f);
	freeaddrinfo(serverinfo); 
	close(socketfd);
	return 0;
	}
	*/
}
