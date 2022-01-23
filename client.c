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

int main(int arg, char** argc){
	//initial error checking
	if(arg != 3){
		printf("Parameter numbers do not match.\n" );
		return 0;
	}
	struct in_addr server_add;
	int err = inet_pton(AF_INET, argc[1], &server_add);
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
	int fd = sendto(sd, "ftp", 3, 0, (struct sockaddr*)&server, sizeof(server));

	
	//take receiving message
	struct sockaddr_storage server_info;
	char message[1000];
	socklen_t sender_size = 0;
	int rd = recvfrom(sd, message, 1000, 0, (struct sockaddr*)&server_info, &sender_size);
	
	if(rd && strcmp("Yes", message) == 0)
		printf("A file transfer can start.");
	else
		printf("Did not receive (a yes).");	
	return 0;
}
