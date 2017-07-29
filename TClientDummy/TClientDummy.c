//
//  TClientDummy.c
//  TClientDummy
//
//  Created by Roicxy Alonso Gonzalez on 7/17/17.
//  Copyright © 2017 AlonsoRoicxy. All rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "hasmap.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#define MAX_THREADS 300

/// prints error message and exits
/// with error status
void error(char const * message) {
    perror(message);
    exit(0);
}
void * server_connection(void* arg);
ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);


pthread_cond_t chat_room_cond_thread;

pthread_mutex_t chat_room_mutex_thread;

map_t flights_map;


typedef struct
{
    char * username;
    struct sockaddr_in * server_address;
    int client_connnection;
    char * buffer;
    pthread_t *client_chat_thread;
    int status;
    
}client_info;

void * chat_msg(void * arg);

int argcT = 3;
char * argvT[] = {"TClient.o", "127.0.0.1","43001"};
int main(int argc, char * argv[]) {
    // check number of arguments
    if (argcT < 3)
        error("usage: travel_client ip_address port\n");
    
    // assign command line arguments
    char * ip_address = argvT[1];
    int port = atoi(argvT[2]);
    int port_init = port;
//    // socket handler
//    int * client_fd;
    
    // client socket address info
    struct sockaddr_in server_address;
    
    // stores host information
    struct hostent * server;
    flight_map = hashmap_new();
    
    pthread_cond_init(&chat_room_cond_thread, NULL);
    
    pthread_mutex_init(&chat_room_mutex_thread, NULL);
    pthread_t ** clients_threads = (pthread_t **)malloc(sizeof(pthread_t*));
    int i = 0;
    
    while (i < MAX_THREADS)
    {
        // socket handler
        int * client_fd = (int *)malloc(sizeof(int));

        if ((*client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            error("error: opening client socket");
        
        int enable = 1;
        if (setsockopt(*client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
            error("error: cannot set socket options");
        
        if ((server = gethostbyname(ip_address)) == NULL)
            error("error: no host at ip addresss");
        
        // initialize server_address object
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET; // set socket type
        server_address.sin_port = htons(port++); // set port number
        inet_pton(AF_INET, ip_address, &server_address.sin_addr); // set socket ip address
        
        // connect socket to server address
        if (connect(*client_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
            error("error: connecting to server");
        
        clients_threads[i] = (pthread_t *)malloc(sizeof(pthread_t));
        
        pthread_create(clients_threads[i++], NULL, server_connection, &client_fd);
        
        if(port > port_init + 4)
        {
            port = port_init;
        }
    }
 
    
    for (int j = 0; j < i; j++)
    {
        void * out;
        
        pthread_join(*clients_threads[i], &out);
        int * fd = (int *)out;
        
        close(*fd);
    }
    
    return 0;
}
void * server_connection(void* arg)
{
    int * client_fd = (int*)arg;
    
    int const BUFFER_SIZE = 1025;
    char buffer[BUFFER_SIZE];
    char username[BUFFER_SIZE];
    
    while(strcmp(buffer, "") == 0 || strcmp(buffer, "\n") == 0)
    {
        memset(buffer, 0, BUFFER_SIZE);
        
        if (read(*client_fd, buffer, 51) < 0)
            error("error: reading from socket");
        
        fprintf(stdout,"%s", buffer);
        
        memset(buffer, 0, strlen(buffer));
        
        if (read(*client_fd, buffer, 51) < 0)
            error("error: reading from socket");
        
        fprintf(stdout,"%s", buffer);
        
        memset(buffer, 0, BUFFER_SIZE);
        
        fgets(buffer, sizeof(buffer), stdin);
        strtok(buffer, "\n");
        
        if (write(*client_fd, buffer, (size_t)strlen(buffer)) < 0)
            error("error: writing to socket");
    }
    
    if(read(*client_fd, username, BUFFER_SIZE) < 0)
        error("error: reading from socket");
    
    client_info * my_connection = (client_info *)malloc(sizeof(client_info));
    
    my_connection->client_connnection = *client_fd;
    my_connection->username = username;
    my_connection->client_chat_thread = (pthread_t *)malloc(sizeof(pthread_t));
    my_connection->status = 1;
    
    
    while(my_connection->status)
    {
        if(strcmp(username, buffer) != 0)
        {
            fprintf(stdout, "%s", username);
        }
        
        if(strcmp(buffer, "LOGOFF") == 0)
        {
            my_connection->status = 0;
            break;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
        
        pthread_create(my_connection->client_chat_thread, NULL, chat_msg, my_connection);
        
        fgets(buffer, BUFFER_SIZE, stdin);
        
        my_connection->buffer = buffer;
        
        // read input from terminal to be sent
        while(my_connection->status && strcmp("\n", (const char*)buffer) == 0)
        {
            fprintf(stdout, "%s", username);
            memset(buffer, 0, BUFFER_SIZE);
            
            fgets(buffer, BUFFER_SIZE, stdin);
            my_connection->buffer = buffer;
        }
        
        
        pthread_cancel(*my_connection->client_chat_thread);
        
        
        strtok(buffer, "\n"); // remove newline from input
        
        
        // send input to server
        if (write(*client_fd, buffer, (size_t)strlen(buffer)) < 0)
            error("error: writing to socket");
        
        memset(buffer, 0, BUFFER_SIZE);
        
        if(read(*client_fd, buffer, BUFFER_SIZE) < 0)
        {
            my_connection->status = 0;
            error("error: reading from socket");
        }
        
        
        fprintf(stdout, "%s", buffer);
        
        strtok(buffer, "\n"); // remove newline from input
        
        if(strcmp(buffer, username) != 0)
        {
            memset(username, 0, sizeof(username));
            if(read(*client_fd, username, sizeof(username)) < 0)
            {
                my_connection->status = 0;
                error("error: reading from socket");
            }
        }
        
    }
    
    free(my_connection);
    
    pthread_exit(&client_fd);
    
}

void * chat_msg(void * arg)
{
    client_info * my_connection = (client_info*)arg;
    size_t msg_len = sizeof(char) * 2000;
    char * msg = (char*)malloc(msg_len);
    
    memset(msg, 0, msg_len);
    
    while (my_connection->status)
    {
        //        size_t msg_len = sizeof(char) * 2000;
        //        char * msg = (char*)malloc(msg_len);
        //        struct iovec * data = (struct iovec *)malloc(sizeof(struct iovec));
        //        data.iov_base = msg;
        //        data.iov_len = msg_len;
        //
        //        memset(msg, 0, msg_len);
        //
        //        messag[i] = data;
        
        
        if(read(my_connection->client_connnection, msg, msg_len) < 0)
            //        if(readv(my_connection->client_connnection, messag, 1))
        {
            my_connection->status = 0;
            error("error: reading from socket");
        }
        
        if(!msg)
            break;
        
        //        pthread_cond_signal(&chat_room_cond_thread);
        
        fprintf(stdout, "\n%s",msg);
        
        strtok(msg, "\n"); // remove newline from input
        
        
        //        ungetc(10, stdin);
        //        fflush(stdin);
        
        //        if(strcmp(msg, my_connection->username) != 0)
        //        {
        //            memset(msg, 0, msg_len);
        //            if(read(my_connection->client_connnection, msg, msg_len) < 0)
        //            {
        //                my_connection->status = 0;
        //                error("error: reading from socket");
        //            }
        //
        //            fprintf(stdout, "%s",msg);
        ////            fprintf(stdout, "\n%s",my_connection->username);
        //        }
        
        //        pthread_mutex_unlock(&chat_room_mutex_thread);
        
    }
    if(msg)
        free(msg);
    pthread_exit(NULL);
    
}
ssize_t readn(int fd, void *vptr, size_t n)
{
    
    size_t nleft; ssize_t nread; char *ptr;
    /* Read "n" bytes from a descriptor. */
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0; /* and call read() again */ else
                    return (-1);
        } else if (nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft); /* EOF *//* return >= 0 */
}
ssize_t writen(int fd, const void *vptr, size_t n)
{
    
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    /* Write "n" bytes to a descriptor. */
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
                return (-1); /* error */
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);  /* error */
}
