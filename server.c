#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define NICKNAME_SIZE 32

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    char nickname[NICKNAME_SIZE];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int clients_count = 0;

void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = cl;
            clients_count++;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (clients[i]->sockfd == sockfd) {
                clients[i] = NULL;
                clients_count--;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message(char *message, int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (clients[i]->sockfd != sockfd) {
                if (write(clients[i]->sockfd, message, strlen(message)) < 0) {
                    perror("write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(char *message, char *nickname) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (strcmp(clients[i]->nickname, nickname) == 0) {
                if (write(clients[i]->sockfd, message, strlen(message)) < 0) {
                    perror("write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + NICKNAME_SIZE];
    int leave_flag = 0;
    client_t *cli = (client_t *)arg;

    // Nickname
    if (recv(cli->sockfd, cli->nickname, NICKNAME_SIZE, 0) <= 0 || strlen(cli->nickname) < 2 || strlen(cli->nickname) >= NICKNAME_SIZE - 1) {
        printf("Enter the nickname correctly\n");
        leave_flag = 1;
    } else {
        sprintf(buffer, "%s has joined\n", cli->nickname);
        printf("%s", buffer);
        send_message(buffer, cli->sockfd);
    }

    while (1) {
        if (leave_flag) {
            break;
        }

        int receive = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buffer) > 0) {
                send_message(buffer, cli->sockfd);
                printf("%s -> %s\n", cli->nickname, buffer);
            }
        } else if (receive == 0 || strcmp(buffer, "/quit") == 0) {
            sprintf(buffer, "%s has left\n", cli->nickname);
            printf("%s", buffer);
            send_message(buffer, cli->sockfd);
            leave_flag = 1;
        } else {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buffer, BUFFER_SIZE);
    }

    close(cli->sockfd);
    remove_client(cli->sockfd);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

void print_client_addr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

int main() {
    int sockfd, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t cli_len = sizeof(client_addr);
    pthread_t tid;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5000);

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(sockfd, 10);

    printf("Server started. Listening on port 5000...\n");

    while (1) {
        new_sock = accept(sockfd, (struct sockaddr*)&client_addr, &cli_len);

        if ((new_sock) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if ((clients_count + 1) == MAX_CLIENTS) {
            printf("Max clients reached. Rejected: ");
            print_client_addr(client_addr);
            printf(":%d\n", client_addr.sin_port);
            close(new_sock);
            continue;
        }

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = client_addr;
        cli->sockfd = new_sock;
        strcpy(cli->nickname, "Anonymous");

        add_client(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        sleep(1);
    }

    return 0;
}
