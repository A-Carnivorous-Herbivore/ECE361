#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};


int main(int arg, char** argc){
	//initial error checking
	if(arg != 3){
		printf("Parameter numbers do not match.\n" );
		return 0;
	}
	struct in_addr server_add;
	char temp1[] = "128.100.13.";
	char* temp2 = strcat(temp1,argc[1]);
	int err = inet_pton(AF_INET, temp2, &server_add);
	if(err == 0){
		printf("Source does not contain a character string representing a valid network address in the specified address family.\n");
		return 0;
	}
	if(err == -1){
		printf("AF doesn not contain a valid address family.\n");
		return 0;
	}
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd == -1){
		printf("Error generating socket.\n");
		return 0;
	}

	//structure setup 
	struct sockaddr_in server;
	server.sin_port = htons(atoi(argc[2]));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = server_add.s_addr;

	//user prompt
	printf("Enter command: ftp <file name>\n");
	char cmd[1000];
	char file_name[1000];
	scanf("%s%s", &cmd, &file_name);
	if(strcmp("ftp", cmd) != 0){
		printf("Command is %s, not ftp.\n", cmd);
		return 0;
	}

	//prepare to send message
	if(access(file_name, F_OK) == 0)
		printf("File %s exists, sending message ftp to server.\n", file_name);
	else{
		printf("File %s doesn't exist.\n", file_name);	 
		return 0;
	}
	sendto(sd, "ftp", 3, 0, (struct sockaddr*)&server, sizeof(server));

	
	//take receiving message
	struct sockaddr_storage server_info;
	char message[1000];
	socklen_t server_size = 0;
	int rd = recvfrom(sd, message, 1000, 0, (struct sockaddr*)&server_info, &server_size);
	message[rd] = '\0';	
	if(rd && (strcmp("Yes", message) == 0))
		printf("A file transfer can start.\n");
	else
		printf("Did not receive (a yes).\n");
	
	
	//allocate the number of packets neccesary, packets are always size 1000
	struct stat st;
	stat(file_name, &st);
	int size = st.st_size;
	int noPackets = size/1000 + 1;
	struct packet** packets = (struct packet**)malloc(noPackets*sizeof(struct packet*));
	
	//generate the packet contents based on the file
	int f = open(file_name, O_RDONLY);
	for(int i = 1; i <= noPackets; i++){
		struct packet* pac = malloc(sizeof(*pac));
		pac -> total_frag = noPackets;
		pac -> frag_no = i;
		pac -> filename = file_name;
		pac -> size = 1000;
		read(f, pac -> filedata, 1000);
		
		//last packet is smaller than 1000
		if(i == noPackets)
			pac -> size = size % 1000;
		packets[i-1] = pac;
	}
	
	//send each packet in a stop and wait manner
	for(int i = 0; i < noPackets; i++){
		struct packet* pac = packets[i];
		//use 250 extra chars to hold the front stuff
		char* string = malloc(1250*sizeof(char));
		sprintf(string, "%d:%d:%d:%s:", pac -> total_frag, pac -> frag_no, pac -> size, pac -> filename);
		//use memcpy for the real binary data
		memcpy(strlen(string) + string, pac -> filedata, pac -> size);
		
		//server receives data in string form which it must "deserialize"
		sendto(sd, pac, 1250, 0, (struct sockaddr*)&server, sizeof(server));
		printf("Packet %d/%d sent ... ", i+1, noPackets);
		
		char ack[50];
		int rd = recvfrom(sd, ack, 50, 0, (struct sockaddr*)&server_info, &server_size);
		//must recieve an "ack" from server before proceeding to next packet	
		if(!(rd > 0 && strcmp(ack, "ack") == 0)){
			i--;
			printf("Retransmitting.\n ");
		}else
			printf("Received.\n ") ;

	}
	return 0;
}
