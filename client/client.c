#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#define buflen 1000

void *transcation(void *p)
{
	int *input = (int *) p;
	int sd = input[0];
	int sockfd = input[1];
    
	while(1)
	{
	    	struct sockaddr_in client;
	    	int clientfd = 0;
	    	int clientlen = sizeof(client);
	    	clientfd = accept(sockfd, (struct sockaddr *) &client, &clientlen); 
	    	char received[buflen] = {};
	    	recv(clientfd, received, sizeof(received),0);
	    	send(sd, received, sizeof(received),0);
	    	bzero(received, sizeof(received));
	}

}

int main(int argc , char *argv[])
{
	const char* d = "#";
	int myport = 0;
	int sd = 0;
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd < 0)
	{
	    	printf("Fail to create a socket");
	    	return -1;
	}

	struct sockaddr_in server_address;
	bzero(&server_address,sizeof(server_address));
	server_address.sin_family = PF_INET;
	server_address.sin_addr.s_addr = inet_addr(argv[1]);
	server_address.sin_port = htons(atoi(argv[2]));
    
	int retcode = connect(sd, (struct sockaddr *)&server_address, sizeof(server_address));
	if(retcode < 0)
	{
	    	printf("Connection error!\n");
	    	return -1;
	}
	else
	{
    		printf("Connect to server!\n>");
	}


	while(1)
	{
	    	char message[buflen] = {};
	    	char message_receive[buflen] = {};
	    	scanf("%s", message);

	    	send(sd, message, strlen(message), 0);
	    	recv(sd, message_receive, sizeof(message_receive) ,0);
	    	printf("%s--------------------\n>", message_receive);
	    	if(strcmp(message, "Exit") == 0)
	    	{
			printf("Connection closed");
			close(sd);
			return 0;
	    	}
	    	else if(strcmp(message_receive, "220 AUTH_FAIL\n") != 0 && strcmp(message_receive, "210 FAIL\n") != 0 &&
		    	strcmp(message_receive, "230 Input format error\n") != 0 && strcmp(message, "List") != 0 && strcmp(message_receive, "100 OK\n") != 0)
	    	{
			char *instructor;
			char a[buflen] = {};
			strcpy(a, message);
			instructor = strtok(a, d);
			instructor = strtok(NULL, d);
			myport = atoi(instructor);
			break;
	    	}
	}

	int sockfd = 0;
	sockfd = socket(PF_INET , SOCK_STREAM , 0);
	if (sockfd < 0)
	{
    		printf("Fail to create a socket\n");
	}

	struct sockaddr_in client_s;
	bzero(&client_s, sizeof(client_s));
	client_s.sin_family = PF_INET;
	client_s.sin_addr.s_addr = inet_addr(argv[1]);
	client_s.sin_port = htons(myport);
	bind(sockfd, (struct sockaddr *)&client_s, sizeof(client_s));
	listen(sockfd,10);

	int two_socket[2] = {sd, sockfd}; 
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tid, &attr, transcation, &two_socket);
	if(pthread_create(&tid, &attr, transcation, &two_socket) != 0)
	{
    		printf("Fail to create a thread\n");
	}

	while (1)
	{
	    	char message[buflen] = {};
	    	char message_receive[buflen] = {};
	    	scanf("%s", message);
	   	 
	    	int count = 0;
	    	for (int i = 0; message[i] != '\0'; i++)
	    	{
			if (message[i] == '#')
			{
		    	count++;
			}
	    	}
	   	 
	    	//P2P
	    	if(count == 2)
	    	{
			char *payee_name;
			int payee_port = 0;
			char payee_IP[buflen] = {};
			char *money;
			char a[buflen] = {};
			strcpy(a, message);
			char *p;
			p = strtok(a, d);
			for(int i = 0 ; i < 3 ; i++)
			{
			    	if(i == 2)
			    	{
					payee_name = p;
			    	}

			    	p = strtok(NULL, d);
			}
			char client_req[buflen] = {};
			char server_reply[buflen] = {};
	       	 
	       	 
	  		 	
			while(1)
			{
				printf("auto renew list from tracker before transfer...\n");
            	
		    	
				send(sd, "List", sizeof("List"), 0);
			    	recv(sd, message_receive, sizeof(message_receive) ,0);
			    	//puts("List:\n");
                		printf("%s", message_receive);
				printf("--------------------\n");

		   	 	char* find_payee = strstr(message_receive, payee_name);
			    	if(find_payee == NULL)
			    	{
					printf("Please make sure that the payee is online");
					continue;
			
			    	}
			    	else
			    	{
		   		 	 
			   		 find_payee = strtok(find_payee, d);
			   		 find_payee = strtok(NULL, d);
			   		 strcat(payee_IP, find_payee);
			   		 find_payee = strtok(NULL, d);
			   		 payee_port = atoi(find_payee);               	 
			   		 break;
			    	}
			}

			
			int transfd = 0;
			transfd = socket(PF_INET, SOCK_STREAM, 0);
			if(transfd < 0)
			{
		    		printf("Fail to create a socket");
		    		continue;
			}
	       	 
			struct sockaddr_in trans;
			bzero(&trans, sizeof(trans));
			trans.sin_family = PF_INET;
			trans.sin_addr.s_addr = inet_addr(payee_IP);
			trans.sin_port = htons(payee_port);

			if(connect(transfd, (struct sockaddr *)&trans, sizeof(trans)) < 0)
			{
		    		printf("Fail to connect other client!\n>");
		    		continue;
			}
			else
			{
		    		printf("Connect to other client!\n");
			}

			send(transfd, message, strlen(message), 0);
			recv(sd, server_reply, sizeof(server_reply), 0);
			printf("%s",server_reply);
			printf("--------------------\n");

			printf("auto renew list from tracker after transfer...\n");
			send(sd, "List", strlen("List"), 0);
			recv(sd, server_reply, sizeof(server_reply), 0);
			printf("%s", server_reply);
			
	   	 
			printf("--------------------\n>");

	    	}
	    	else
	    	{
		   	 send(sd, message, strlen(message), 0);
		   	 recv(sd, message_receive, sizeof(message_receive) ,0);
		   	 //printf("%s--------------------\n>", message_receive);
		   	 // printf("--------------------\n>");

		   	 if(strcmp(message, "Exit") == 0)
		   	 {
		   	 	break;
		   	 }
			 printf("%s--------------------\n>", message_receive);
				
	    	}
   	 
	}
	printf("Bye\nConnection closed\n");
 	close(sockfd);
 	close(sd);
    
    
	return 0;
}

