#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <apra/inet.h>
#include "list/list.h"
#include <errno.h>
#include <netdb.h>  // Add this header for getaddrinfo
#include <sys/socket.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")


#define MAX_MESSAGE_SIZE 1024

// Structure to represent a message
typedef struct {
    char message[MAX_MESSAGE_SIZE];
} Message;

// Structure to represent a shared list of messages
typedef struct {
    List* receivedMessageList;
    List* sendingMessageList;
    pthread_mutex_t mutex;
    int stopped;
} SharedData;

// Global variables
SharedData sharedData;
// int udpSocket;  // Declare the UDP socket

// Function prototypes
void* keyboardInputThreadfunc(void* arg);
void* udpReceiverThreadfunc(void* arg);
void* screenOutputThreadfunc(void* arg);
void* udpOutputThreadfunc(void* arg);

// Function to initialize the UDP socket
int initializeUDPSocket(int port, int timeoutSec, int timeoutUsec) {
    // Create a UDP socket
    int socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        perror("Error creating UDP socket");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR to allow reuse of the local address
    int reuse = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = timeoutUsec;
    if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt(SO_RCVTIMEO) failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the local address
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        fprintf(stderr, "Error binding UDP socket port %d: %s\n", port, strerror(errno));
        close(socketFd);
        exit(EXIT_FAILURE);
    }

    return socketFd;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <local_port> <remote_machine> <remote_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("%s \n", argv[0]);
    printf("%s \n", argv[1]);
    printf("%s \n", argv[2]);
    printf("%s \n", argv[3]);

    // Initialize shared data
    sharedData.sendingMessageList = List_create();
    sharedData.receivedMessageList = List_create();
    pthread_mutex_init(&sharedData.mutex, NULL);

    // Initialize UDP socket
    int localPort = atoi(argv[1]);
    int remotePort = atoi(argv[3]);

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // You can change this to AF_UNSPEC to support both IPv4 and IPv6
    hints.ai_socktype = SOCK_DGRAM;

    int ret = getaddrinfo(argv[2], NULL, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

        // Iterate through the list of IP addresses
    struct addrinfo *ptr;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);
            printf("IPv4 Address: %s\n", ip);
            argv[2] = ip;
        } else if (ptr->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ptr->ai_addr;
            char ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip, INET6_ADDRSTRLEN);
            printf("IPv6 Address: %s\n", ip);
        }
    }

    // Create threads
    pthread_t keyboardThread, udpInputThread, screenOutputThread, udpOutputThread;

    pthread_create(&keyboardThread, NULL, keyboardInputThreadfunc, NULL);
    pthread_create(&udpInputThread, NULL, udpReceiverThreadfunc, argv); // Pass the result from getaddrinfo
    pthread_create(&screenOutputThread, NULL, screenOutputThreadfunc, NULL);
    pthread_create(&udpOutputThread, NULL, udpOutputThreadfunc, argv);

    // Wait for threads to finish
    pthread_join(keyboardThread, NULL);
    pthread_join(udpInputThread, NULL);
    pthread_join(screenOutputThread, NULL);
    pthread_join(udpOutputThread, NULL);

    // Cleanup
    List_free(sharedData.sendingMessageList, free);
    List_free(sharedData.receivedMessageList, free);
    pthread_mutex_destroy(&sharedData.mutex);
    freeaddrinfo(result);

    return 0;
}



void* keyboardInputThreadfunc(void* arg) {
    //printf("do inputkeyboardTHread");
    char input[MAX_MESSAGE_SIZE];
    //printf("do inputkeyboardTHread");
    while (sharedData.stopped==0) {
        fgets(input, MAX_MESSAGE_SIZE, stdin);

 // printf("input is %s",input);
        // Terminate the s-talk session if '!' is entered
        if (input[0] == '!') {
            printf("stopped");
            sleep(1);
            sharedData.stopped=1;
            
            break;
        }
//  if (input[0] == '!'){
//     printf("yees");
// sharedData.stopped=1;
// }

        // Add the message to the list
        pthread_mutex_lock(&sharedData.mutex);
        List_append(sharedData.sendingMessageList, strdup(input));
        pthread_mutex_unlock(&sharedData.mutex);
    }

    return NULL;
}




void* screenOutputThreadfunc(void* arg) {
    //printf("do screen output");

    while (sharedData.stopped==0) {
        pthread_mutex_lock(&sharedData.mutex);
        char* message = List_trim(sharedData.receivedMessageList);
        pthread_mutex_unlock(&sharedData.mutex);

        if (message != NULL) {
            printf("\t%s", message);
            free(message);
        }
    }

    return NULL;
}

void* udpOutputThreadfunc(void* arg) {
    //printf("do output");
    char** argv = (char**)arg;
    int localPort = atoi(argv[1]);
    int remotePort = atoi(argv[3]);
    //printf("do output");

    // Create a separate socket for sending to the remote port
    int udpSendSocket = initializeUDPSocket(remotePort,10,0);

    while (sharedData.stopped == 0) {
        pthread_mutex_lock(&sharedData.mutex);

        // Get the latest message from the list
        char* message = List_trim(sharedData.sendingMessageList);

        pthread_mutex_unlock(&sharedData.mutex);

        if (message != NULL) {
            // Send the message via UDP
            struct sockaddr_in remoteAddr;
            memset(&remoteAddr, 0, sizeof(remoteAddr));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = htons(remotePort);

            // Corrected the index to access the remote machine address
            inet_pton(AF_INET, argv[2], &remoteAddr.sin_addr);

            sendto(udpSendSocket, message, strlen(message), 0,
                   (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
            // printf("sent : %s",message);

            free(message);  // Free the memory allocated by List_trim
        }
    }

    // Close the separate socket for sending
    close(udpSendSocket);

    return NULL;
}

void* udpReceiverThreadfunc(void* arg) {
    // Cast the argument to the correct type
    char** argv = (char**)arg;

    // Check if argv is not NULL before accessing its elements
    if (argv == NULL || argv[1] == NULL || argv[3] == NULL) {
        fprintf(stderr, "Invalid argument for udpInputThreadfunc\n");
        return NULL;
    }

    //printf("do input");
    int localPort = atoi(argv[1]);
    //printf("do input");
    


    while (sharedData.stopped == 0) {
        int udpSocket = initializeUDPSocket(localPort,10,0);
        // printf("listening");

        char buffer[MAX_MESSAGE_SIZE];
        struct sockaddr_in remoteAddr;
        socklen_t addrLen = sizeof(remoteAddr);

        // Receive data from the UDP socket
        ssize_t bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                     (struct sockaddr*)&remoteAddr, &addrLen);



        if (bytesRead < 0) {
            // perror("Error receiving data from UDP socket");
        } else {
            buffer[bytesRead] = '\0';  // Null-terminate the received data
            // printf("received %s ",buffer);

            // Update the shared list with the received message
            pthread_mutex_lock(&sharedData.mutex);
            List_append(sharedData.receivedMessageList, strdup(buffer));
            pthread_mutex_unlock(&sharedData.mutex);
        }
        close(udpSocket);
    }

    

    return NULL;
}

