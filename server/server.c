#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#define buflen 1000
#define MAX_USERS 10000
int server_socket;


typedef struct
{
    int port;
    int sockfd;
    int balance;
    char username[buflen];
    char public_key[buflen];
    char ip_address[buflen];
} User;


// User user_list[MAX_USERS];
User *user_list = NULL;
int num_users = 0; 	  // Track the number of registered users
int user_ports[MAX_USERS];	// Array to store user ports
int user_balances[MAX_USERS]; // Array to store user balances
pthread_mutex_t user_data_mutex = PTHREAD_MUTEX_INITIALIZER;

User *find_user_by_name(const char *username, User *user_list, int num_users);
bool is_username_duplicate(const char *username, User *user_list, int num_users);
// void add_user(User *new_user, User *user_list, int *num_users);
void add_user(User *new_user);
int find_user_index(const char *username, User *user_list, int num_users);
void update_user_data(const char *username, int port, int balance);
int get_user_port(const char *username);
int get_user_balance(const char *username);
void send_user_data(int sockfd, User *user);
void send_all_users_data(int sockfd, const char *username);
void send_microtransaction(int sockfd, const char *sender, const char *receiver, const char *payAmount);
int count_char_occurrences(const char *str, char target);

void handle_transaction(User *sender, User *payee, int amount)
{
    if (amount > 0 && sender->balance >= amount)
    {
        sender->balance -= amount;
        payee->balance += amount;
    }
}

int find_user_index(const char *username, User *user_list, int num_users)
{
	for (int i = 0; i < num_users; i++)
	{
    // printf("name: %s\n", username);   	 
    if (strcmp(user_list[i].username, username) == 0)
    	{
        	return i;
    	}
	}
	return -1;  // Return -1 if the user is not found
}


void update_user_data(const char *username, int port, int balance)
{
    pthread_mutex_lock(&user_data_mutex);
    int user_index = find_user_index(username, user_list, num_users);

    if (user_index != -1)
    {
   	 // Create a temporary user to hold the updated data
   	 User temp_user = user_list[user_index];
   	 temp_user.port = port;
   	 temp_user.balance = balance;

   	 // Copy the updated data back to the original user atomically
   	 user_list[user_index] = temp_user;
    }

    pthread_mutex_unlock(&user_data_mutex);

}

int get_user_port(const char *username)
{
    pthread_mutex_lock(&user_data_mutex);
    int user_index = find_user_index(username, user_list, num_users);
    int port = (user_index != -1) ? user_list[user_index].port : -1;
    pthread_mutex_unlock(&user_data_mutex);
    return port;
}

int get_user_balance(const char *username)
{
    pthread_mutex_lock(&user_data_mutex);
    int user_index = find_user_index(username, user_list, num_users);
    int balance = (user_index != -1) ? user_list[user_index].balance : -1;
    pthread_mutex_unlock(&user_data_mutex);
    return balance;
}

int count_char_occurrences(const char *str, char target)
{
	int count = 0;
	while (*str)
	{
    	if (*str == target)
        	count++;
    	str++;
	}
	return count;
}


// Function to add a new user
void add_user(User *new_user)
{
    pthread_mutex_lock(&user_data_mutex);
    // Allocate memory for the new user
    user_list = realloc(user_list, (num_users + 1) * sizeof(User));
    if (user_list == NULL)
    {
   		 perror("Error reallocating memory for user list");
   		 exit(EXIT_FAILURE);
    }

    // Add the new user to the array
    user_list[num_users] = *new_user;
    num_users++;
    pthread_mutex_unlock(&user_data_mutex);
}

// Function to free memory for user_list
void free_user_list()
{
    pthread_mutex_lock(&user_data_mutex);
    free(user_list);
    pthread_mutex_unlock(&user_data_mutex);
}

void send_user_data(int sockfd, User *user)
{
    pthread_mutex_lock(&user_data_mutex);
    char response[buflen] = {};
    printf(response, sizeof(response), "%d\n%s\n%d\n", user->balance, user->public_key, num_users);

    // Append user information to the response
    snprintf(response + strlen(response), sizeof(response) - strlen(response), "%s#%d#%d\n",
    		 user->username, user->balance, user->port);

    // Send the response to the client
    send(sockfd, response, strlen(response), 0);
    pthread_mutex_unlock(&user_data_mutex);
}

void send_all_users_data(int sockfd, const char *username)
{
	pthread_mutex_lock(&user_data_mutex);

	// Find the user in the user_list
	int user_index = find_user_index(username, user_list, num_users);

	if (user_index != -1)
	{
    	// Send account balance and server public key of the requesting client
    	char balance_and_key[buflen];
    	snprintf(balance_and_key, buflen, "%d\n%s\n", user_list[user_index].balance, user_list[user_index].public_key);
    	send(sockfd, balance_and_key, strlen(balance_and_key), 0);

    	// Send the number of online accounts
    	char num_accounts[buflen];
    	snprintf(num_accounts, buflen, "%d\n", num_users);
    	send(sockfd, num_accounts, strlen(num_accounts), 0);

    	// Send information about each online user
    	for (int i = 0; i < num_users; i++)
    	{
        	char user_info[buflen];
        	snprintf(user_info, buflen, "%s#%s#%d\n", user_list[i].username, user_list[i].ip_address, user_list[i].port);
        	send(sockfd, user_info, strlen(user_info), 0);
    	}
	}
	else
	{
	    	// Handle the case where the requesting user is not found
	    	send(sockfd, "240 USER_NOT_FOUND\n", strlen("240 USER_NOT_FOUND\n"), 0);
	}

	pthread_mutex_unlock(&user_data_mutex);
}


User *find_user_by_name(const char *username, User *user_list, int num_users)
{
    pthread_mutex_lock(&user_data_mutex);
    for (int i = 0; i < num_users; i++)
    {
   		 if (strcmp(user_list[i].username, username) == 0)
   	 {
   		 pthread_mutex_unlock(&user_data_mutex);
   		 return &user_list[i];
   		 }
    }
    pthread_mutex_unlock(&user_data_mutex);
    return NULL;
}

void *handle_client(void *arg)
{
    User *user = (User *)arg;
    // printf("Client: %s:%d\n", user->ip_address, ntohs(user->port));
    
    
    while (1)
    {
   		 char message[buflen] = {};
   		 ssize_t bytes_received = recv(user->sockfd, message, sizeof(message), 0);
   	 
   		 if (bytes_received <= 0)
   	 {
   		 if (bytes_received == 0)
   		 {
   				 printf("Client disconnected\n");
   		 }
   		 else
   		 {
   				 perror("recv");
   		 }

   		 close(user->sockfd);
   		 free(user);
   		 pthread_exit(NULL);
   		 }
   	 
   	 

   	 // Ensure the received message is null-terminated
   	 // message[bytes_received] = '\0';


   	 // Count the occurrences of '#'
   	 int numberOfHashes = count_char_occurrences(message, '#');

   	 printf("%s\n", message);
   	 // printf("Number of '#' characters: %d\n", numberOfHashes);
   	 // printf("Message length:       	%zd\n", bytes_received);
    
   	 // Register
   		 if (strncmp(message, "REGISTER#", 9) == 0)
   	 {
   		 char username[buflen - 9];
   		 sscanf(message, "REGISTER#%s", username);
   	 

   		 // Check if the username is already registered
   		 if (is_username_duplicate(username, user_list, num_users))
   		 {
   				 send(user->sockfd, "210 FAIL\n", strlen("210 FAIL\n"), 0);
   		 }
   		 else
   		 {
   				 // Register the new user
   				 User new_user;
   				 new_user.sockfd = user->sockfd;
   				 new_user.balance = 10000;
   				 strcpy(new_user.public_key, "dummy_public_key");
   				 strcpy(new_user.username, username);
   			 strncpy(new_user.ip_address, user->ip_address, sizeof(new_user.ip_address) - 1);
   				 // Add the new user to the user list
   				 add_user(&new_user);

   				 // printf("User %s registered\n", username);
   				 // printf("Number of users = %d\n", num_users);
   				 send(user->sockfd, "100 OK\n", strlen("100 OK\n"), 0);

   		 }

   		 }
   	 // Handle micropayment transaction
   	 else if (numberOfHashes == 2)
   	 {
   	 	// Handle micropayment transaction
   	 	char sender[buflen] = {}, receiver[buflen] = {}, payAmount[buflen] = {};
   	 	sscanf(message, "%[^#]#%[^#]#%s", sender, payAmount, receiver);
   	 	// printf("sender = %s\n", sender);

   	 	// Locate sender and receiver in the user array
   	 	User *senderUser = find_user_by_name(sender, user_list, num_users);
   	 	User *receiverUser = find_user_by_name(receiver, user_list, num_users);

   		 

   	 	if (senderUser != NULL && receiverUser != NULL)
   	 	{
   		 
   		 // printf("sender = %s\n", senderUser->username);
   		 // printf("receiver = %s\n", receiverUser->username);
   		 // Assuming sender has enough balance, deduct the amount
   		 if (senderUser->balance >= atoi(payAmount))
   		 {
   			 // Deduct the amount from the sender
   			 senderUser->balance -= atoi(payAmount);
   			 // printf("sender_balance = %d\n", senderUser->balance);

   			 // Add the amount to the receiver
   			 receiverUser->balance += atoi(payAmount);
   			 // printf("receiver_balance = %d\n", receiverUser->balance);

   			 // Send success response to both sender and receiver
   			 char successResponse[buflen] = {};
   			 snprintf(successResponse, sizeof(successResponse), "Micropayment success: %s transferred %s to %s\n", sender, payAmount, receiver);


   			 // Update user data for both sender and receiver
   			 update_user_data(senderUser->username, senderUser->port, senderUser->balance);
   			 update_user_data(receiverUser->username, receiverUser->port, receiverUser->balance);
   			 // printf("sender = %d\n", senderUser->balance);
   			 // printf("receiver = %d\n", receiverUser->balance);

   			 send(senderUser->sockfd, successResponse, strlen(successResponse), 0);
   			 send(receiverUser->sockfd, successResponse, strlen(successResponse), 0);

   			 //send_user_data(senderUser->sockfd, senderUser);
   			 //send_user_data(receiverUser->sockfd, receiverUser);

   			 


   		 }
   		 else
   		 {
   		 	// Send failure response to sender
   		 	char failResponse[buflen] = {};
   		 	snprintf(failResponse, sizeof(failResponse), "Micropayment failed: Insufficient balance\n");

   		 	send(senderUser->sockfd, failResponse, strlen(failResponse), 0);
   		 }
   	 	}
   	 	else
   	 	{
   		 // Send failure response if sender or receiver is not found
   		 char failResponse[buflen] = {};
   		 snprintf(failResponse, sizeof(failResponse), "Micropayment failed: Sender or receiver not found\n");

   		 send(user->sockfd, failResponse, strlen(failResponse), 0);
   	 	}
   	 }

   	 else if (strcmp(message, "Exit") == 0)
   	 {
   	 
   		 send(user->sockfd, "Bye\n", strlen("Bye\n"), 0);
   		 // Close the connection and free resources as needed
   		 close(user->sockfd);
   		 free(user);
   		 pthread_exit(NULL);

   		 }
   	 // Login
   	 else if (strstr(message, "#"))
   	 {
   	 	char username[buflen - 9];
   	 	int port;
   	 	sscanf(message, "%[^#]#%d", username, &port);

   	 	// Check if the user is registered
   	 	User *existing_user = find_user_by_name(username, user_list, num_users);

   	 	if (existing_user != NULL)
   	 	{
   		 // Update the user's port and username
   		 update_user_data(username, port, existing_user->balance);
   		 strcpy(user->username, username);

   		 // Send the response to the client
   		 // send_user_data(user->sockfd, existing_user);

   		 // Send information about all registered users to the client
   		 send_all_users_data(user->sockfd, username);
   	 	}
   	 	else
   	 	{
   		 // User is not registered
   		 send(user->sockfd, "220 AUTH_FAIL\n", strlen("220 AUTH_FAIL\n"), 0);
   	 	}
   	 }
 
   	 // List
   	 else if (strcmp(message, "List") == 0)
   	 {
   		 
   		 // printf("user in List: %s\n", user->username);
   		 // Send information about all registered users to the client
   		 send_all_users_data(user->sockfd, user->username);


   		 }
   	 
   	 else
   	 {
   		 send(user->sockfd, "230 Input format error\n", strlen("230 Input format error\n"), 0);
   		 }

   	 bzero(message, sizeof(message));
   	 
    }

    pthread_exit(NULL);
}




bool is_username_duplicate(const char *username, User *user_list, int num_users)
{
    for (int i = 0; i < num_users; i++)
    {
        if (strcmp(user_list[i].username, username) == 0)
        {
     	   return true; // 找到相同的 username，表示有重複
        }
    }
    return false; // 沒有找到相同的 username，表示沒有重複
}



int main(int argc, char *argv[])
{
    // Allocate memory for user_list
    user_list = malloc(MAX_USERS * sizeof(User));
    if (user_list == NULL)
    {
   		 perror("Error allocating memory for user list");
   		 exit(EXIT_FAILURE);
    }

    // Register free_user_list to be called at exit
    atexit(free_user_list);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
   	 perror("Failed to create a socket");
   	 return -1;
    }


    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
   		 perror("Bind failed");
   		 return -1;
    }

    listen(server_socket, 10);
    printf("Server listening on port %s\n", argv[1]);

    while (1)
    {
   		 struct sockaddr_in client;
   	 socklen_t clientlen = sizeof(client);
   	 int client_socket = accept(server_socket, (struct sockaddr *)&client, &clientlen);
   	 if (client_socket < 0)
   	 {
   		 perror("Accept failed");
   		 return -1;
   	 }

   	 // Print client's IP address and port
   	 char client_ip[INET_ADDRSTRLEN];
   	 inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
   	 printf("Client IP: %s\n", client_ip);
   	 printf("Client port: %d\n",  ntohs(client.sin_port));


   		 User *user = (User *)malloc(sizeof(User));
   	 // strcpy(user->username, "test");
   		 inet_ntop(AF_INET, &client.sin_addr, user->ip_address, sizeof(user->ip_address));


   	 pthread_t tid;
   	 pthread_attr_t attr;
   	 pthread_attr_init(&attr);


   	 user->sockfd = client_socket;
   	 user->balance = 10000;
   	 strcpy(user->public_key, "dummy_public_key");

   	 // Create a new thread for the client

   		 if (pthread_create(&tid, NULL, handle_client, (void *)user) != 0)
   	 {
   		 perror("Failed to create a thread");
   		 return -1;
   		 }

   		 // Detach the thread to avoid memory leaks
   		 pthread_detach(tid);
    }

    close(server_socket);

    return 0;
}

