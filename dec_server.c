#include <stdio.h>       // Standard input/output library
#include <stdlib.h>      // Standard library for memory management, process control
#include <string.h>      // String handling functions
#include <unistd.h>      // POSIX API for system calls
#include <sys/types.h>   // Definitions for data types like `pid_t`
#include <sys/socket.h>  // Socket programming library
#include <netinet/in.h>  // Structures for internet addresses
#include <signal.h>      // Signal handling
#include <sys/wait.h>    // For `waitpid` to reap child processes
#include <errno.h>       // Error number definitions

#define BUFFER_SIZE 100000 // Define maximum buffer size for messages
#define MAX_CONCURRENT_CONNECTIONS 5 // Define max simultaneous client connections

// Function to print an error message and exit
void error(const char *msg) {
    perror(msg); // Print the system error message
    exit(1);     // Exit with error code
}

// Initialize the server address structure
void setupAddressStruct(struct sockaddr_in* address, int portNumber) {
    memset((char*) address, '\0', sizeof(*address)); // Clear the structure
    address->sin_family = AF_INET;                  // Set address family to Internet
    address->sin_port = htons(portNumber);          // Convert port number to network byte order
    address->sin_addr.s_addr = INADDR_ANY;          // Bind to any available address
}

// Decrypt the ciphertext using the key
void decryptText(char* ciphertext, char* key, char* plaintext) {
    for (int i = 0; i < strlen(ciphertext); i++) { // Iterate through each character
        int cipherVal = (ciphertext[i] == ' ') ? 26 : ciphertext[i] - 'A'; // Map space to 26, A-Z to 0-25
        int keyVal = (key[i] == ' ') ? 26 : key[i] - 'A';                  // Map key space similarly
        int plainVal = (cipherVal - keyVal + 27) % 27; // Perform decryption with wrap-around
        plaintext[i] = (plainVal == 26) ? ' ' : 'A' + plainVal; // Map back to char
    }
    plaintext[strlen(ciphertext)] = '\0'; // Null-terminate the decrypted string
}

// Handle client connection and decryption request
void handleClient(int connectionSocket) {
    char buffer[BUFFER_SIZE], ciphertext[BUFFER_SIZE], key[BUFFER_SIZE], plaintext[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE); // Clear buffer

    // Receive message from client
    int charsRead = recv(connectionSocket, buffer, BUFFER_SIZE - 1, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket"); // Handle read error
    }

    // Parse input in format "ciphertext@key"
    char* token = strtok(buffer, "@");
    if (token == NULL) {
        send(connectionSocket, "ERROR: Invalid input", 20, 0); // Error for malformed input
        close(connectionSocket);
        return;
    }
    strncpy(ciphertext, token, BUFFER_SIZE - 1);

    token = strtok(NULL, "@");
    if (token == NULL) {
        send(connectionSocket, "ERROR: Invalid key", 20, 0); // Error for missing key
        close(connectionSocket);
        return;
    }
    strncpy(key, token, BUFFER_SIZE - 1);

    // Validate key length
    if (strlen(key) < strlen(ciphertext)) {
        send(connectionSocket, "ERROR: Key too short", 20, 0);
        close(connectionSocket);
        return;
    }

    // Validate ciphertext and key characters
    for (int i = 0; i < strlen(ciphertext); i++) {
        if ((ciphertext[i] < 'A' || ciphertext[i] > 'Z') && ciphertext[i] != ' ') {
            send(connectionSocket, "ERROR: Invalid ciphertext character", 35, 0);
            close(connectionSocket);
            return;
        }
    }

    for (int i = 0; i < strlen(key); i++) {
        if ((key[i] < 'A' || key[i] > 'Z') && key[i] != ' ') {
            send(connectionSocket, "ERROR: Invalid key character", 32, 0);
            close(connectionSocket);
            return;
        }
    }

    // Decrypt and send plaintext
    decryptText(ciphertext, key, plaintext);
    if (send(connectionSocket, plaintext, strlen(plaintext), 0) < 0) {
        error("ERROR writing to socket");
    }

    close(connectionSocket); // Close client connection
}

// Reap zombie child processes
void reapZombies(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0); // Non-blocking reap
}

int main(int argc, char *argv[]) {
    int listenSocket, connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo;

    // Validate number of arguments
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Register signal handler for child process cleanup
    signal(SIGCHLD, reapZombies);

    // Create the socket
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Bind the socket to a port
    setupAddressStruct(&serverAddress, atoi(argv[1]));
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for client connections
    listen(listenSocket, MAX_CONCURRENT_CONNECTIONS);

    // Main loop to accept and handle connections
    while (1) {
        sizeOfClientInfo = sizeof(clientAddress);

        // Accept a connection
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            error("ERROR on accept");
        }

        pid_t pid = fork(); // Create child process
        if (pid < 0) {
            error("ERROR on fork");
        } else if (pid == 0) { // Child process
            close(listenSocket);       // Child doesn't need the listening socket
            handleClient(connectionSocket); // Process the client request
            exit(0);                   // Terminate child process
        } else { // Parent process
            close(connectionSocket);   // Parent doesn't need client socket
        }
    }

    close(listenSocket); // Close listening socket
    return 0;
}