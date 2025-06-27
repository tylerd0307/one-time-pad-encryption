#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

#define BUFFER_SIZE 100000 // Maximum buffer size for data transmission
#define MAX_CONCURRENT_CONNECTIONS 5 // Maximum number of clients that can connect simultaneously

// Function to print error messages and exit
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Setup server address structure
void setupAddressStruct(struct sockaddr_in *address, int portNumber) {
    memset((char *)address, '\0', sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_port = htons(portNumber); // Convert port to network byte order
    address->sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
}

// Validate input text for allowed characters (A-Z or space)
void validateInput(const char *text) {
    for (int i = 0; i < strlen(text); i++) {
        if (!isalpha(text[i]) && text[i] != ' ') {
            fprintf(stderr, "SERVER: ERROR - invalid character detected: '%c'\n", text[i]);
            exit(EXIT_FAILURE);
        }
    }
}

// Encrypt plaintext using key and store the result in ciphertext
void encryptText(const char *plaintext, const char *key, char *ciphertext) {
    int len = strlen(plaintext);

    // Validate input for invalid characters
    validateInput(plaintext);
    validateInput(key);

    for (int i = 0; i < len; i++) {
        int plainVal = (plaintext[i] == ' ') ? 26 : plaintext[i] - 'A'; // Convert char to 0-26
        int keyVal = (key[i] == ' ') ? 26 : key[i] - 'A'; // Convert char to 0-26
        int cipherVal = (plainVal + keyVal) % 27; // Encrypt using modular addition
        ciphertext[i] = (cipherVal == 26) ? ' ' : 'A' + cipherVal; // Convert back to char
    }
    ciphertext[len] = '\0'; // Null-terminate ciphertext
}

// Handle incoming client connection
void handleClient(int connectionSocket) {
    char buffer[BUFFER_SIZE], plaintext[BUFFER_SIZE], key[BUFFER_SIZE], ciphertext[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);

    // Receive data from client
    int charsRead = recv(connectionSocket, buffer, BUFFER_SIZE - 1, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }

    // Split plaintext and key using '@' as delimiter
    char *token = strtok(buffer, "@");
    if (token == NULL) {
        send(connectionSocket, "ERROR: Invalid input", 20, 0);
        close(connectionSocket);
        return;
    }
    strncpy(plaintext, token, BUFFER_SIZE - 1);

    token = strtok(NULL, "@");
    if (token == NULL) {
        send(connectionSocket, "ERROR: Invalid key", 20, 0);
        close(connectionSocket);
        return;
    }
    strncpy(key, token, BUFFER_SIZE - 1);

    // Ensure key is long enough for plaintext
    if (strlen(key) < strlen(plaintext)) {
        send(connectionSocket, "ERROR: Key too short", 20, 0);
        close(connectionSocket);
        return;
    }

    encryptText(plaintext, key, ciphertext); // Perform encryption

    // Send ciphertext to client
    if (send(connectionSocket, ciphertext, strlen(ciphertext), 0) < 0) {
        error("ERROR writing to socket");
    }

    close(connectionSocket); // Close connection
}

// Reap zombie processes created by child processes
void reapZombies(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    signal(SIGCHLD, reapZombies); // Handle zombie processes

    int listenSocket, connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);

    listenSocket = socket(AF_INET, SOCK_STREAM, 0); // Create socket
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    setupAddressStruct(&serverAddress, atoi(argv[1])); // Configure server address

    // Bind the socket to the specified port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    listen(listenSocket, MAX_CONCURRENT_CONNECTIONS); // Start listening for connections

    while (1) {
        // Accept new client connection
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &clientAddressSize);
        if (connectionSocket < 0) {
            if (errno == EINTR) continue; // Retry if interrupted
            error("ERROR on accept");
        }

        pid_t pid = fork(); // Fork to handle client connection
        if (pid < 0) {
            error("ERROR on fork");
        } else if (pid == 0) { // Child process
            close(listenSocket); // Child doesn't need listening socket
            handleClient(connectionSocket); // Process client request
            exit(0); // Exit child process
        } else { // Parent process
            close(connectionSocket); // Parent doesn't need connected socket
        }
    }

    close(listenSocket); // Close listening socket
    return 0;
}