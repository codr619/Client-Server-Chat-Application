#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
typedef struct {
 struct sockaddr_in addr;
 int connfd;
 char nickname[50];
} client_t;
client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int clients_count = 0;
void str_trim_lf(char* arr, int length) {
 for (int i = 0; i < length; i++) {
 if (arr[i] == '\n') {
 arr[i] = '\0';
 break;
 }
 }
}
void add_client(client_t *cl) {
 pthread_mutex_lock(&clients_mutex);
 for (int i = 0; i < MAX_CLIENTS; ++i) {
 if (!clients[i]) {
 clients[i] = cl;
 clients_count++;
 break;
 }
 }
 pthread_mutex_unlock(&clients_mutex);
}
void remove_client(int connfd) {
 pthread_mutex_lock(&clients_mutex);
 for (int i = 0; i < MAX_CLIENTS; ++i) {
 if (clients[i]) {
 if (clients[i]->connfd == connfd) {
 clients[i] = NULL;
 clients_count--;
 break;
 }
 }
 }
 pthread_mutex_unlock(&clients_mutex);
}
void send_message(char *message, int connfd) {
 pthread_mutex_lock(&clients_mutex);
 for (int i = 0; i < MAX_CLIENTS; ++i) {
 if (clients[i]) {
 if (clients[i]->connfd != connfd) {
 send(clients[i]->connfd, message, strlen(message), 0);
 }
 }
 }
 pthread_mutex_unlock(&clients_mutex);
}
void send_private_message(char *message, char *nickname, int connfd) {
 pthread_mutex_lock(&clients_mutex);
 for (int i = 0; i < MAX_CLIENTS; ++i) {
 if (clients[i]) {
 if (strcmp(clients[i]->nickname, nickname) == 0) {
 send(clients[i]->connfd, message, strlen(message), 0);
 break;
 }
 }
 }
 pthread_mutex_unlock(&clients_mutex);
}
void *handle_client(void *arg) {
 char buffer[BUFFER_SIZE];
 char message[BUFFER_SIZE + 50];
 int leave_flag = 0;
 client_t *cli = (client_t *)arg;
 // Receive nickname
 if (recv(cli->connfd, cli->nickname, 50, 0) <= 0 || strlen(cli->nickname) < 2 || strlen(cli-
>nickname) >= 50-1) {
 printf("Nickname not entered.\n");
 leave_flag = 1;
 } else {
 sprintf(message, "%s has joined\n", cli->nickname);
 printf("%s", message);
 send_message(message, cli->connfd);
 }
 bzero(buffer, BUFFER_SIZE);
 while (1) {
 if (leave_flag) {
 break;
 }
 int receive = recv(cli->connfd, buffer, BUFFER_SIZE, 0);
 if (receive > 0) {
 if (strlen(buffer) > 0) {
 if (buffer[0] == '/') {
 // Command handling
 if (strncmp(buffer, "/help", 5) == 0) {
 sprintf(message, "List of commands:\n/help - Display this help
message\n/private [nickname] [message] - Send a private message\n/broadcast [message] -
Send a broadcast message\n/list - List all connected clients\n/quit - Disconnect from the
server\n");
 send(cli->connfd, message, strlen(message), 0);
 } else if (strncmp(buffer, "/private", 8) == 0) {
 char *nickname = strtok(buffer + 9, " ");
 char *msg = strtok(NULL, "");
 if (nickname != NULL && msg != NULL) {
 sprintf(message, "[Private from %s]: %s\n", cli->nickname, msg);
 send_private_message(message, nickname, cli->connfd);
 } else {
 sprintf(message, "Usage: /private [nickname] [message]\n");
 send(cli->connfd, message, strlen(message), 0);
 }
 } else if (strncmp(buffer, "/broadcast", 10) == 0) {
 sprintf(message, "[Broadcast from %s]: %s\n", cli->nickname, buffer + 11);
 send_message(message, cli->connfd);
 printf("%s", message);
 } else if (strncmp(buffer, "/list", 5) == 0) {
 sprintf(message, "Connected clients:\n");
 send(cli->connfd, message, strlen(message), 0);
 for (int i = 0; i < MAX_CLIENTS; ++i) {
 if (clients[i]) {
 sprintf(message, "- %s (%s:%d)\n", clients[i]->nickname,
inet_ntoa(clients[i]->addr.sin_addr), ntohs(clients[i]->addr.sin_port));
 send(cli->connfd, message, strlen(message), 0);
 }
 }
 } else if (strncmp(buffer, "/quit", 5) == 0) {
 sprintf(message, "%s has left\n", cli->nickname);
 printf("%s", message);
 send_message(message, cli->connfd);
 leave_flag = 1;
 } else {
 sprintf(message, "Unknown command: %s\n", buffer);
 send(cli->connfd, message, strlen(message), 0);
 }
 } else {
 // Normal message
 send_message(buffer, cli->connfd);
 str_trim_lf(buffer, strlen(buffer));
 printf("%s: %s\n", cli->nickname, buffer);
 }
 }
 } else if (receive == 0 || strcmp(buffer, "/quit") == 0) {
 sprintf(message, "%s has left\n", cli->nickname);
 printf("%s", message);
 send_message(message, cli->connfd);
 leave_flag = 1;
 } else {
 printf("Error occurred.\n");
 leave_flag = 1;
 }
 bzero(buffer, BUFFER_SIZE);
 }
 close(cli->connfd);
 remove_client(cli->connfd);
 free(cli);
 pthread_detach(pthread_self());
 return NULL;
}
int main() {
 int listenfd = 0, connfd = 0;
 struct sockaddr_in server_addr;
 struct sockaddr_in client_addr;
 pthread_t tid;
 listenfd = socket(AF_INET, SOCK_STREAM, 0);
 if (listenfd < 0) {
 printf("Socket creation failed.\n");
 return 1;
 }
 server_addr.sin_family = AF_INET;
 server_addr.sin_addr.s_addr = INADDR_ANY;
 server_addr.sin_port = htons(5000);
 if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
 printf("Bind failed.\n");
 return 1;
 }
 if (listen(listenfd, 10) < 0) {
 printf("Listen failed.\n");
 return 1;
 }
 printf("Server started. Listening on port 5000...\n");
 while (1) {
 socklen_t clilen = sizeof(client_addr);
 connfd = accept(listenfd, (struct sockaddr*)&client_addr, &clilen);
 if ((clients_count + 1) == MAX_CLIENTS) {
 printf("Max clients reached. Rejected: ");
 printf("%d\n", client_addr.sin_addr.s_addr);
 close(connfd);
 continue;
 }
 client_t *cli = (client_t *)malloc(sizeof(client_t));
 cli->addr = client_addr;
 cli->connfd = connfd;
 add_client(cli);
 pthread_create(&tid, NULL, &handle_client, (void*)cli);
 }
 return 0;
}
