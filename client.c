#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define NICKNAME_SIZE 32

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char nickname[NICKNAME_SIZE];

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void str_trim_lf(char* arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void send_message_handler() {
    char message[BUFFER_SIZE] = {};
    char buffer[BUFFER_SIZE + NICKNAME_SIZE + 3] = {}; // Ensure enough space for nickname, message, and ": \n"

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        str_trim_lf(message, BUFFER_SIZE);

        if (strcmp(message, "/quit") == 0) {
            break;
        } else {
            snprintf(buffer, sizeof(buffer), "%s: %s\n", nickname, message);
            send(sockfd, buffer, strlen(buffer), 0);
        }

        bzero(message, BUFFER_SIZE);
        bzero(buffer, BUFFER_SIZE + NICKNAME_SIZE + 3);
    }
    catch_ctrl_c_and_exit(2);
}

void recv_message_handler() {
    char message[BUFFER_SIZE] = {};
    while (1) {
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            printf("%s", message);
        } else if (receive == 0) {
            break;
        }
        bzero(message, BUFFER_SIZE);
    }
}

int main() {
    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Enter server IP address: ");
    char ip[16];
    scanf("%s", ip);

    printf("Enter server port number: ");
    int port;
    scanf("%d", &port);

    printf("Enter your nickname: ");
    scanf("%s", nickname);

    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    send(sockfd, nickname, NICKNAME_SIZE, 0);

    printf("=== WELCOME TO THE CHATROOM ===\n");

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *) send_message_handler, NULL) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *) recv_message_handler, NULL) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            printf("\nBye\n");
            break;
        }
    }

    close(sockfd);

    return 0;
}
