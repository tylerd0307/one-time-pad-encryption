#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 100000 // Maximum buffer size for data transmission

// Function to print error messages and exit the program
void error(const char *msg) {
    perror(msg); // Print system error message
    exit(EXIT_FAILURE); // Exit with failure status
}

// Function to read file content into a buffer and clean trailing newlines/spaces
void readFileContent(const char *filename, char *buffer, size_t bufferSize) {
    FILE *file = fopen(filename, "r"); // Open file in read mode
    if (file == NULL) {
        fprintf(stderr, "CLIENT: ERROR opening file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Read file content into buffer
    if (fread(buffer, sizeof(char), bufferSize - 1, file) <= 0) {
        fprintf(stderr, "CLIENT: ERROR reading file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fclose(file); // Close the file

    buffer[bufferSize - 1] = '\0'; // Null-terminate buffer

    // Remove trailing newlines or spaces from buffer
    while (strlen(buffer) > 0 && (buffer[strlen(buffer) - 1] == '\n' || buffer[strlen(buffer) - 1] == '\r' || buffer[strlen(buffer) - 1] == ' ')) {
        buffer[strlen(buffer) - 1] = '\0';
    }
}

// Function to validate plaintext for allowed characters (A-Z or space)
void validatePlaintext(const char *plaintext) {
    for (size_t i = 0; i < strlen(plaintext); ++i) {
        // Check for invalid characters
        if (!isalpha(plaintext[i]) && plaintext[i] != ' ') {
            fprintf(stderr, "CLIENT: ERROR - invalid character in plaintext: '%c'\n", plaintext[i]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    // Ensure correct usage
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s plaintext_file key_file port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int socketFD, portNumber; // File descriptor for the socket and port number
    struct sockaddr_in serverAddress; // Server address structure
    char buffer[BUFFER_SIZE], plaintext[BUFFER_SIZE], key[BUFFER_SIZE], response[BUFFER_SIZE];

    portNumber = atoi(argv[3]); // Convert port argument to integer

    // Read plaintext and key from files
    readFileContent(argv[1], plaintext, BUFFER_SIZE);
    readFileContent(argv[2], key, BUFFER_SIZE);

    // Check if key length is sufficient for plaintext
    if (strlen(key) < strlen(plaintext)) {
        fprintf(stderr, "CLIENT: ERROR - Key too short to match plaintext\n");
        exit(EXIT_FAILURE);
    }

    validatePlaintext(plaintext); // Validate plaintext content

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) error("Error opening socket");

    // Setup server address structure
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber); // Convert port number to network byte order
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Use localhost

    // Connect to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("Error connecting to server");
    }

    // Prepare plaintext and key for sending
    snprintf(buffer, BUFFER_SIZE, "%s@%s", plaintext, key);

    // Send data to the server
    if (send(socketFD, buffer, strlen(buffer), 0) < 0) {
        error("Error sending data");
    }

    // Receive response from server
    memset(response, '\0', BUFFER_SIZE);
    if (recv(socketFD, response, BUFFER_SIZE, 0) < 0) {
        error("Error receiving server response");
    }

    printf("%s\n", response); // Print the server response

    close(socketFD); // Close the socket
    return 0;
}