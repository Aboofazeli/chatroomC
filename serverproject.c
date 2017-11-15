/*
 ALI REZA ABOOFAZELI
MULTI USER SERVER



 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#define NUMBEROFCLIENTS	100

static unsigned int counter = 0;
static int clientnumber = 0;

/* Client information: each client has an address, socket descriptor, unique number and name */
typedef struct {
	struct sockaddr_in addr;	
	int clientsock;			
	int clientnumber;			
	char name[32];			
} clientsinfo;

clientsinfo *clients[NUMBEROFCLIENTS];  /*clients global structure for saving datas*/

 /*function prototypes*/
void activeclients(int clientsock);
void msgtoclient(char *msg, int clientnumber);
void msgtosender(const char *msg, int clientsock);
void sendtoall(char *msg);
void sendtoallexceptsender(char *msg, int clientnumber);
void deleteclient(int clientnumber);
void addclient(clientsinfo *cl);
void clientipaddress(struct sockaddr_in addr);
void *clienthandling(void *arg);
void removern(char *msg);
/*main function*/
int main(int argc, char *argv[]){
	int listentoclient = 0, clientsock = 0;
	struct sockaddr_in serveraddress;
	struct sockaddr_in clientaddress;
	pthread_t nthread;
	unsigned short serverport;

	serverport=atoi(argv[1]); /*recieve port id from input*/
	/* Socket creation */
	listentoclient = socket(AF_INET, SOCK_STREAM, 0);
	serveraddress.sin_family = AF_INET;
	serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddress.sin_port = htons(serverport); 

	/* Binding process */
	if(bind(listentoclient, (struct sockaddr*)&serveraddress, sizeof(serveraddress)) < 0){
		perror("bind failed");
		return 1;
	}

	/* Listening process to 10 clients at a time */
	if(listen(listentoclient, 10) < 0){
		perror("listen failed");
		return 1;
	}

	printf("SERVER HAS STARTED\n");

	/* Accept all clients*/
	while(1){
		socklen_t clilen = sizeof(clientaddress);
		clientsock = accept(listentoclient, (struct sockaddr*)&clientaddress, &clilen);

		/* max clients condition which is 100 now */
		if((counter+1) == NUMBEROFCLIENTS){
			printf("<<MAX CLIENTS LIMITS\n");
			printf("<<CONNECTION REJECTED ");
			clientipaddress(clientaddress);
			printf("\n");
			close(clientsock);
			continue;
		}

		/* Client settings */
		clientsinfo *clinfo = (clientsinfo *)malloc(sizeof(clientsinfo));
		clinfo->addr = clientaddress;
		clinfo->clientsock = clientsock;
		clinfo->clientnumber = clientnumber++;
		sprintf(clinfo->name, "%d", clinfo->clientnumber);

		/* Add client to the data and create a thread to handle it */
		addclient(clinfo);
		pthread_create(&nthread, NULL, &clienthandling, (void*)clinfo);

		/* for reducing CPU usage  */
		sleep(1);
	}
}

/* Handling the clients */
void *clienthandling(void *arg){
	char bufferall[1024];
	char bufferin[1024];
	int rcv;

	counter++;
	clientsinfo *clinfo = (clientsinfo *)arg;

	printf("<<ACCEPT ");
	clientipaddress(clinfo->addr);
	printf(" CLIENT ID %d\n", clinfo->clientnumber);

	sprintf(bufferall, "<<JOINED, HELLO CLIENT ID %s\r\n", clinfo->name);
	sendtoall(bufferall);

	/* input from client */
	while((rcv = read(clinfo->clientsock, bufferin, sizeof(bufferin)-1)) > 0){
	        bufferin[rcv] = '\0';
	        bufferall[0] = '\0';
		removern(bufferin);

		/* Ignore empty buffer */
		if(!strlen(bufferin)){
			continue;
		}
	
		/* MENU */
		if(bufferin[0] == '\\'){
			char *choice, *arg;
			choice = strtok(bufferin," ");
			if(!strcmp(choice, "\\QUIT")){
				break;
			}else if(!strcmp(choice, "\\PING")){
				msgtosender("<<RECIEVED\r\n", clinfo->clientsock);
			}else if(!strcmp(choice, "\\NAME")){
				arg = strtok(NULL, " ");
				if(arg){
					char *prename = strdup(clinfo->name);
					strcpy(clinfo->name, arg);
					sprintf(bufferall, "<<RENAME, %s TO %s\r\n", prename, clinfo->name);
					free(prename);
					sendtoall(bufferall);
				}else{
					msgtosender("<<NAME CANNOT BE NULL\r\n", clinfo->clientsock);
				}
			}else if(!strcmp(choice, "\\PRIVATE")){
				arg = strtok(NULL, " ");
				if(arg){
					int clientnumber = atoi(arg);
					arg = strtok(NULL, " ");
					if(arg){
						sprintf(bufferall, "[Private msg][%s]", clinfo->name);
						while(arg != NULL){
							strcat(bufferall, " ");
							strcat(bufferall, arg);
							arg = strtok(NULL, " ");
						}
						strcat(bufferall, "\r\n");
						msgtoclient(bufferall, clientnumber);
					}else{
						msgtosender("<<MESSAGE CANNOT BE NULL\r\n", clinfo->clientsock);
					}
				}else{
					msgtosender("<<CLIENT ID CANNOT BE NULL\r\n", clinfo->clientsock);
				}
			}else if(!strcmp(choice, "\\ACTIVE")){
				sprintf(bufferall, "<<CLIENTS %d\r\n", counter);
				msgtosender(bufferall, clinfo->clientsock);
				activeclients(clinfo->clientsock);
			}else if(!strcmp(choice, "\\HELP")){
				strcat(bufferall, "\\QUIT     Quit chatroom\r\n");
				strcat(bufferall, "\\PING     Server test\r\n");
				strcat(bufferall, "\\NAME     <name> create nickname\r\n");
				strcat(bufferall, "\\PRIVATE  <client id> <message> Send private message\r\n");
				strcat(bufferall, "\\ACTIVE   Show active clients\r\n");
				strcat(bufferall, "\\HELP     Show help\r\n");
				msgtosender(bufferall, clinfo->clientsock);
			}else{
				msgtosender("<<UNRECOGNIZABLE TRY AGAIN\r\n", clinfo->clientsock);
			}
		}else{
			/* Sending a message */
			sprintf(bufferall, "[%s] %s\r\n", clinfo->name, bufferin);
			sendtoallexceptsender(bufferall, clinfo->clientnumber);
		}
	}

	/* Close connection */
	close(clinfo->clientsock);
	sprintf(bufferall, "<<LEFT, GOODBYE CLIENT ID %s\r\n", clinfo->name);
	sendtoall(bufferall);

	/* Delete client from datas and release thread */
	deleteclient(clinfo->clientnumber);
	printf("<<LEFT ");
	clientipaddress(clinfo->addr);
	printf(" CLIENT ID %d\n", clinfo->clientnumber);
	free(clinfo);
	counter--;
	pthread_detach(pthread_self());
	
	return NULL;
}


/* ip address */
void clientipaddress(struct sockaddr_in addr){
	
printf("%s",inet_ntoa(addr.sin_addr));
}

/* Add client to the structure */
void addclient(clientsinfo *cl){
	int i;
	for(i=0;i<NUMBEROFCLIENTS;i++){
		if(!clients[i]){
			clients[i] = cl;
			return;
		}
	}
}

/* Delete client from the strcture */
void deleteclient(int clientnumber){
	int i;
	for(i=0;i<NUMBEROFCLIENTS;i++){
		if(clients[i]){
			if(clients[i]->clientnumber == clientnumber){
				clients[i] = NULL;
				return;
			}
		}
	}
}

/* Send message to all clients except the sender */
void sendtoallexceptsender(char *msg, int clientnumber){
	int i;
	for(i=0;i<NUMBEROFCLIENTS;i++){
		if(clients[i]){
			if(clients[i]->clientnumber != clientnumber){
				write(clients[i]->clientsock, msg, strlen(msg));
			}
		}
	}
}

/* Send message to all clients */
void sendtoall(char *msg){
	int i;
	for(i=0;i<NUMBEROFCLIENTS;i++){
		if(clients[i]){
			write(clients[i]->clientsock, msg, strlen(msg));
		}
	}
}

/* Send message to sender */
void msgtosender(const char *msg, int clientsock){
	write(clientsock, msg, strlen(msg));
}

/* Send message to client */
void msgtoclient(char *msg, int clientnumber){
	int i;
	for(i=0;i<NUMBEROFCLIENTS;i++){
		if(clients[i]){
			if(clients[i]->clientnumber == clientnumber){
				write(clients[i]->clientsock, msg, strlen(msg));
			}
		}
	}
}

/* list of active clients */
void activeclients(int clientsock){
	int i;
	char msg[64];
	for(i=0;i<NUMBEROFCLIENTS;i++){
		if(clients[i]){
			sprintf(msg, "<<CLIENT %d | %s\r\n", clients[i]->clientnumber, clients[i]->name);
			msgtosender(msg, clientsock);
		}
	}
}

/* remove /r and /n from strings */
void removern(char *msg){
	while(*msg != '\0'){
		if(*msg == '\r' || *msg == '\n'){
			*msg = '\0';
		}
		msg++;
	}
}

