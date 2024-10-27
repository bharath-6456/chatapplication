// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define NICKNAME_SIZE 32

typedef struct {
    int socket;
    char nickname[NICKNAME_SIZE];
} Client;

Client clients[MAX_CLIENTS];
fd_set active_fd_set, read_fd_set;

void send_to_all(char *message, int sender_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
}

void handle_new_connection(int server_socket) {
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("Error accepting new connection");
        return;
    }

    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            client_index = i;
            break;
        }
    }

    if (client_index == -1) {
        printf("Server full: rejecting new connection.\n");
        close(client_socket);
        return;
    }

    recv(client_socket, clients[client_index].nickname, NICKNAME_SIZE, 0);
    printf("%s has joined the chat.\n", clients[client_index].nickname);

    char welcome_message[BUFFER_SIZE];
    sprintf(welcome_message, "%s has joined the chat.\n", clients[client_index].nickname);
    send_to_all(welcome_message, client_socket);

    FD_SET(client_socket, &active_fd_set);
}

void handle_client_message(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);

    if (bytes_read <= 0) {
        printf("Client disconnected.\n");
        close(client_socket);
        FD_CLR(client_socket, &active_fd_set);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == client_socket) {
                clients[i].socket = 0;
                char leave_message[BUFFER_SIZE];
                sprintf(leave_message, "%s has left the chat.\n", clients[i].nickname);
                send_to_all(leave_message, client_socket);
                break;
            }
        }
    } else {
        buffer[bytes_read] = '\0';
        char message[BUFFER_SIZE + NICKNAME_SIZE];
        
        char sender_nickname[NICKNAME_SIZE];
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == client_socket) {
                strcpy(sender_nickname, clients[i].nickname);
                break;
            }
        }
        
        sprintf(message, "%s: %s", sender_nickname, buffer);
        send_to_all(message, client_socket);
    }
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, MAX_CLIENTS);

    printf("Server listening on port %d...\n", PORT);

    FD_ZERO(&active_fd_set);
    FD_SET(server_socket, &active_fd_set);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
    }

    while (1) {
        read_fd_set = active_fd_set;

        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("Error with select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == server_socket) {
                    handle_new_connection(server_socket);
                } else {
                    handle_client_message(i);
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
