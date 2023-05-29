#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define PORT 8800
#define MAX_CLIENT 2
#define MAX_MSG 200

typedef struct {
    int client_sockfd;
    int guess_count;
    char current_word[10];
    char hint[10];
    bool guessed[26];
} ClientData;

char words[][10] = {
    "apple",
    "banana",
    "orange",
    "kiwi",
    "mango",
    "peach",
    "plum"
};
int word_count = 7;

void generate_hint(ClientData* client) {
    for (int i = 0; i < strlen(client->current_word); i++) {
        if (client->guessed[client->current_word[i] - 'a']) {
            client->hint[i] = client->current_word[i];
        } else {
            client->hint[i] = '_';
        }
    }
    client->hint[strlen(client->current_word)] = '\0';
}

void start_game(ClientData* client, const char* word) {
    strcpy(client->current_word, word);

    for (int i = 0; i < 26; i++) {
        client->guessed[i] = false;
    }

    generate_hint(client);
}

void* client_handler(void* arg) {
    ClientData* client = (ClientData*)arg;
    int client_sockfd = client->client_sockfd;
    int guess_count = client->guess_count;

    // send start game message
    char welcome_message[MAX_MSG];
    sprintf(welcome_message, "Welcome to the fruit word guessing game! You have 10 chances to guess.\nHint: %s\nLives: 10\n", client->hint);
    write(client_sockfd, welcome_message, strlen(welcome_message));

    while (guess_count < 10) {
        // receive letter that client guess
        char message[MAX_MSG];
        read(client_sockfd, message, MAX_MSG);
        char guess = message[0];
        guess_count++;

        // check letter that user send to server
        bool correct_guess = false;
        for (int i = 0; i < strlen(client->current_word); i++) {
            if (client->current_word[i] == guess) {
                correct_guess = true;
                client->guessed[guess - 'a'] = true;
            }
        }

        if (correct_guess) {
            // update the hint
            generate_hint(client);

            // check that user guess correct full word
            bool word_guessed = true;
            for (int i = 0; i < strlen(client->current_word); i++) {
                if (!client->guessed[client->current_word[i] - 'a']) {
                    word_guessed = false;
                    break;
                }
            }

            if (word_guessed) {
                // win
                char congrats_message[MAX_MSG];
                sprintf(congrats_message, "\n\nCongratulations! \nYou guessed the word '%s' correctly. \n", client->current_word);
                write(client_sockfd, congrats_message, strlen(congrats_message));
                close(client_sockfd);
                printf("Client disconnected \n");
                break;
            } else {
                // correct guess message
                char correct_message[MAX_MSG];
                sprintf(correct_message, "\nCorrect guessing!! Try to guess another.\nHint: %s\nLives: %d\n", client->hint, 10 - guess_count);
                write(client_sockfd, correct_message, strlen(correct_message));
            }
        } else {
            // incorrect guess message
            char incorrect_message[MAX_MSG];
            sprintf(incorrect_message, "\nInorrect guessing!! Try to guess another.\nHint: %s\nLives: %d\n", client->hint, 10 - guess_count);
            write(client_sockfd, incorrect_message, strlen(incorrect_message));
        }
    }

    if (guess_count >= 10) {
        // lose
        char game_over_message[MAX_MSG];
        sprintf(game_over_message, "\n\nGame over! The word that correct is '%s'.\n", client->current_word);
        write(client_sockfd, game_over_message, strlen(game_over_message));
        close(client_sockfd);
    }

    pthread_exit(NULL);
}

int main() {
    int server_sockfd, client_sockfd, bind_status, listen_status;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len;

    // create the server socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        perror("\n Error : Could not create socket \n");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bind the server socket to the specified IP and port
    bind_status = bind(server_sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bind_status < 0) {
        perror("\n cannot bind port \n");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    listen_status = listen(server_sockfd, MAX_CLIENT);
    if (listen_status < 0) {
        perror("\n failed to listen for client connections \n");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d, waiting for clients.\n", PORT);

    for (int i = 0; i < MAX_CLIENT; i++) {
        // Accept a client connection
        client_address_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_address, &client_address_len);
        printf("Client %d connected.\n", i + 1);

        ClientData* client_data = (ClientData*)malloc(sizeof(ClientData));
        client_data->client_sockfd = client_sockfd;
        client_data->guess_count = 0;

        // Generate a random word for the game
        srand(time(NULL));
        int random_index = rand() % word_count;
        char word[10];
        strcpy(word, words[random_index]);

        // start game and send msg to client
        start_game(client_data, word);

        // create thread for each client
        pthread_t thread;
        pthread_create(&thread, NULL, client_handler, (void*)client_data);
    }

    pthread_exit(NULL);

    close(server_sockfd);

    return 0;
}
