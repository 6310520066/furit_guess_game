#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8800
#define MAX_MSG 200

int main() {
    int client_sockfd, connect_status;
    struct sockaddr_in server_address;
    char message[MAX_MSG];

    // create the client socket
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("\n Error : Could not create socket \n");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_address.sin_port = htons(PORT);

    // connect to server
    connect_status = connect(client_sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (connect_status < 0) {
        perror("\n Error : Can't connect to server. \n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server on port %d.\n\n", PORT);

    // receive welcome msg from server
    read(client_sockfd, message, MAX_MSG);
    printf("%s", message);

    // input 1 letter then send it to server
    while (1) {
        // user input
        printf("Enter your guess letter (or 'quit' to exit): ");
        fgets(message, MAX_MSG, stdin);
        message[strcspn(message, "\n")] = '\0';

        // send input to server
        write(client_sockfd, message, strlen(message));

 if (strcmp(message, "quit") == 0) {
            break;
        }

        // get server response
        memset(message, 0, MAX_MSG);
        read(client_sockfd, message, MAX_MSG);

        // // Check if the connection is closed
        if (strcmp(message, "Connection closed.\n") == 0) {
            printf("%s", message);
            break;
        }

 if (strstr(message, "Congratulations!") || strstr(message, "Game over")) {
        printf("%s", message);
           break;
 }

        // Print the server response
        printf("%s", message);
    }

    // Close the client socket
    close(client_sockfd);
    printf("Disconnected from server \n");

    return 0;
}