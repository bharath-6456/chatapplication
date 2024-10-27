// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define NICKNAME_SIZE 32

int client_socket;

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            exit(EXIT_FAILURE);
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    char nickname[NICKNAME_SIZE];
    char message[BUFFER_SIZE];

    printf("Enter your nickname: ");
    fgets(nickname, NICKNAME_SIZE, stdin);
    nickname[strcspn(nickname, "\n")] = '\0'; // Remove newline

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    send(client_socket, nickname, strlen(nickname), 0);

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    printf("You can now start chatting!\n");

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        send(client_socket, message, strlen(message), 0);
    }

    close(client_socket);
    return 0;
}
