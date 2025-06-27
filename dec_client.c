#include <stdio.h>       // Standard input/output library
#include <stdlib.h>      // Standard library for memory management and process control
#include <string.h>      // String handling functions
#include <unistd.h>      // POSIX API for system calls
#include <sys/types.h>   // Definitions for data types like `pid_t`
#include <sys/socket.h>  // Socket programming library
#include <netdb.h>       // Networking definitions and functions
#include <fcntl.h>       // File control options for low-level I/O

#define BUFFER_SIZE 100000 // Define the maximum buffer size for messages

// Function to print an error message and exit
void error(const char *msg) {
    perror(msg); // Print the system error message
    exit(EXIT_FAILURE); // Exit the program with failure status
}

// Initialize the server address structure
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname) {
    memset((char*) address, '\0', sizeof(*address)); // Clear the structure
    address->sin_family = AF_INET;                  // Set address family to Internet
    address->sin_port = htons(portNumber);          // Convert port number to network byte order

    // Get host information for the given hostname
    struct hostent* hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL) { // Handle error if the host does not exist
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // Copy the host address to the server address structure
    memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// Read the content of a file into a buffer
void readFileContent(const char* filename, char* buffer, size_t bufferSize) {
    memset(buffer, '\0', bufferSize); // Clear the buffer
    FILE *file = fopen(filename, "r"); // Open the file for reading
    if (file == NULL) { // Handle error if the file cannot be opened
        fprintf(stderr, "CLIENT: ERROR opening file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Read the file content into the buffer
    size_t bytesRead = fread(buffer, sizeof(char), bufferSize - 1, file);
    if (bytesRead <= 0) { // Handle error if the file is empty or unreadable
        fprintf(stderr, "CLIENT: ERROR reading file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fclose(file); // Close the file
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove the newline character at the end of the buffer
}

int main(int argc, char *argv[]) {
    // Validate the number of arguments
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s ciphertext_file key_file port [hostname]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int socketFD, portNumber;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE], ciphertext[BUFFER_SIZE], key[BUFFER_SIZE], response[BUFFER_SIZE];
    char hostname[100] = "localhost"; // Default hostname is "localhost"

    portNumber = atoi(argv[3]); // Convert the port number argument to an integer

    // If a hostname is provided, copy it to the `hostname` variable
    if (argc >= 5) {
        strncpy(hostname, argv[4], sizeof(hostname) - 1);
    }

    // Read the ciphertext file into the `ciphertext` buffer
    readFileContent(argv[1], ciphertext, BUFFER_SIZE);

    // Read the key file into the `key` buffer
    readFileContent(argv[2], key, BUFFER_SIZE);

    // Validate that the key is long enough to decrypt the ciphertext
    if (strlen(key) < strlen(ciphertext)) {
        fprintf(stderr, "CLIENT: ERROR - Key too short to match ciphertext\n");
        exit(EXIT_FAILURE);
    }

    // Create the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }

    // Set up the server address structure
    setupAddressStruct(&serverAddress, portNumber, hostname);

    // Connect to the server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
    }

    // Prepare the message to send in the format "ciphertext@key"
    snprintf(buffer, BUFFER_SIZE - 1, "%s@%s", ciphertext, key);

    // Send the message to the server
    if (send(socketFD, buffer, strlen(buffer), 0) < 0) {
        error("CLIENT: ERROR writing to socket");
    }

    // Clear the response buffer
    memset(response, '\0', BUFFER_SIZE);

    // Receive the response from the server
    if (recv(socketFD, response, BUFFER_SIZE - 1, 0) < 0) {
        error("CLIENT: ERROR reading server response");
    }

    // Print the decrypted plaintext received from the server
    printf("%s\n", response);

    // Close the socket
    close(socketFD);

    return 0;
}