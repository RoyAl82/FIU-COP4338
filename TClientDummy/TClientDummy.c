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
#include "hashmap.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#define MAX_THREADS 5

/// prints error message and exits
/// with error status
void error(char const * message) {
    perror(message);
    exit(0);
}



struct hashmap_element {
    char* key;
    int in_use;
    any_t data;
};

struct hashmap_map {
    int table_size;
    int size;
    struct hashmap_element *data;
};

typedef struct
{
    char** list;
    int list_size;
}flight_data;

pthread_cond_t chat_room_cond_thread;
pthread_mutex_t chat_room_mutex_thread;
pthread_mutex_t port_mutex_thread;
pthread_mutex_t connect_mutex_thread;
pthread_mutex_t extra_mutex_thread;

char *opt[7];
char *opt_chat[5];

flight_data * flights;
char ** usernames;
int username_size;
int opt_size;
int opt_chat_size;
int port;
int port_init;
char * ip_address;
struct sockaddr_in ** server_address;
struct hostent ** server;
int ** clients_fd;

typedef struct
{
    char * username;
    char * chat_username;
    struct sockaddr_in * server_address;
    int client_connnection;
    char * buffer;
    pthread_t *client_chat_thread;
    int status;
    
}client_info;

void * chat_msg(void * arg);
void * socket_connection(void * arg);
void * server_connection(void* arg);
ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);

int argcT = 3;
char * argvT[] = {"TClient.o", "127.0.0.1","43001"};
int main(int argc, char * argv[]) {
    // check number of arguments
    if (argcT < 3)
        error("usage: travel_client ip_address port\n");
    
    // assign command line arguments
    ip_address = argvT[1];
    port = atoi(argvT[2]);
    port_init = port;
//    // socket handler
//    int * client_fd;
    
    // client socket address info
 
    
    // stores host information
    opt_size = 0;
    opt[opt_size++] = "QUERY";
    opt[opt_size++] = "RESERVE";
    opt[opt_size++] = "RETURN";
    opt[opt_size++] = "LIST";
    opt[opt_size++] = "LIST_AVAILABLE";
    opt[opt_size++] = "ENTER CHAT";
    opt[opt_size++] = "LOGOFF";
    opt_chat_size = 0;
    opt_chat[opt_chat_size++] = "TEXT";
    opt_chat[opt_chat_size++] = "LIST";
    opt_chat[opt_chat_size++] = "LIST_ALL";
    opt_chat[opt_chat_size++] = "LIST_OFFLINE";
    opt_chat[opt_chat_size++] = "EXIT CHAT";
   
    usernames = (char**)malloc(sizeof(char*) * 10);
    username_size = 0;
    usernames[username_size++] = "ROY";
    usernames[username_size++] = "ROB";
    usernames[username_size++] = "CARLO";
    usernames[username_size++] = "PABLO";
    usernames[username_size++] = "JOHN";
    usernames[username_size++] = "COVFEFE";
    usernames[username_size++] = "CAROL";
    usernames[username_size++] = "RACHEL";
    usernames[username_size++] = "PEDRO";
    usernames[username_size++] = "ROBERT";
    
    flights = NULL;
    
    
    
    time_t t;
    
    srand((unsigned) time(&t));
    
    pthread_cond_init(&chat_room_cond_thread, NULL);
    pthread_mutex_init(&chat_room_mutex_thread, NULL);
    pthread_mutex_init(&port_mutex_thread, NULL);
    pthread_mutex_init(&connect_mutex_thread, NULL);
    pthread_mutex_init(&extra_mutex_thread, NULL);

    
    
    server_address = (struct sockaddr_in **)malloc(sizeof(struct sockaddr_in*) * MAX_THREADS + 10);
    server = (struct hostent **)malloc(sizeof(struct hostent*) * MAX_THREADS + 10);
    clients_fd = (int**)malloc(sizeof(int*) * MAX_THREADS + 10);
    
//    ip_address = (char **)malloc(sizeof(char*) * MAX_THREADS + 10);
//    pthread_mutex_init(&chat_room_mutex_thread, NULL);
    pthread_t ** clients_threads = (pthread_t **)malloc(sizeof(pthread_t*) * MAX_THREADS + 10);
//    int ** connection_fd = (int**)malloc(sizeof(int *) * MAX_THREADS);
    
//    if ((*connection_fd[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
//        error("error: opening client socket");
//    
//    int enable = 1;
//    if (setsockopt(*connection_fd[i], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
//        error("error: cannot set socket options");
//    
//    if ((server = gethostbyname(ip_address)) == NULL)
//        error("error: no host at ip addresss");
    
//    int i = 0;
    
    
    
        // socket handler
//        connection_fd[i] = (int *)malloc(sizeof(int));

//        if ((*connection_fd[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
//            error("error: opening client socket");
//        
//        int enable = 1;
//        if (setsockopt(*connection_fd[i], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
//            error("error: cannot set socket options");
//        
//        if ((server = gethostbyname(ip_address)) == NULL)
//            error("error: no host at ip addresss");
//        
//        // initialize server_address object
//        memset(&server_address, 0, sizeof(server_address));
//        server_address.sin_family = AF_INET; // set socket type
//        server_address.sin_port = htons(port++); // set port number
//        inet_pton(AF_INET, ip_address, &server_address.sin_addr); // set socket ip address
//        
//        // connect socket to server address
//        if (connect(*connection_fd[i], (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
//            error("error: connecting to server");
//        
//        clients_threads[i] = (pthread_t *)malloc(sizeof(pthread_t));
//        
//        pthread_create(clients_threads[i], NULL, server_connection, connection_fd[i]);
//        
//        if(port > port_init + 4)
//        {
//            port = port_init;
//        }
//        i++;
    
//    MAX_THREADS
    int i;
    for( i = 0; i < MAX_THREADS; i++)
    {
        int * i_addr = (int *)malloc(sizeof(int));
        *i_addr = i;
//        pthread_mutex_lock(&connect_mutex_thread);
        server_address[i] = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        server[i] = (struct hostent *)malloc(sizeof(struct hostent));
//        pthread_mutex_unlock(&connect_mutex_thread);
        clients_threads[i] = (pthread_t *)malloc(sizeof(pthread_t));
        
        pthread_create(clients_threads[i], NULL, socket_connection, (void*)i_addr);
    }

    
    
    for (int j = 0; j < i; j++)
    {
        void * out;
        
        pthread_join(*clients_threads[j], &out);
        int * fd = (int *)out;
        
        close(*fd);
        free(fd);
    }
    
    return 0;
}
void * socket_connection(void * arg)
{
//    pthread_mutex_lock(&connect_mutex_thread);
    pthread_mutex_lock(&port_mutex_thread);
    int index = *(int*)arg;
    
//    ip_address[index] = (char *)malloc(sizeof(char) * MAX_THREADS + 10);
    clients_fd[index] = (int*)malloc(sizeof(int));
//    *clients_fd[index] = 0;
//    int client_fd;
    
    if ((*clients_fd[index] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        error("error: opening client socket");
    
    int enable = 1;
    if (setsockopt(*clients_fd[index], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
        error("error: cannot set socket options");
//    pthread_mutex_unlock(&connect_mutex_thread);
    if ((server[index] = gethostbyname(ip_address)) == NULL)
        error("error: no host at ip addresss");
    
    // initialize server_address object
//    memset(server_address[index], 0, sizeof(*server_address[index]));
//    pthread_mutex_lock(&connect_mutex_thread);
    server_address[index]->sin_family = AF_INET; // set socket type
//    pthread_mutex_lock(&port_mutex_thread);
    server_address[index]->sin_port = htons(port); // set port number
    if(port++ >= (port_init + 3))
    {
        port = port_init;
    }
//    pthread_mutex_lock(&port_mutex_thread);
    struct sockaddr_in addr = *server_address[index];
    
    inet_pton(AF_INET, ip_address, &addr.sin_addr); // set socket ip address
//    pthread_mutex_lock(&connect_mutex_thread);
    // connect socket to server address
    if (connect(*clients_fd[index], (struct sockaddr *) &addr, sizeof(addr)) < 0)
        error("error: connecting to server");
    pthread_mutex_unlock(&port_mutex_thread);
    
    pthread_t client_thread;
//    clients_threads[index] = (pthread_t *)malloc(sizeof(pthread_t));
    
    pthread_create(&client_thread, NULL, server_connection, clients_fd[index]);
    
    

    pthread_join(client_thread, NULL);
    pthread_exit(NULL);
}

void * server_connection(void* arg)
{
    int * client_fd = (int*)arg;
    
    int const BUFFER_SIZE = 1025;
    char buffer[BUFFER_SIZE];
    char username[BUFFER_SIZE];
    char * buffer1 = (char *)malloc(sizeof(char*) * BUFFER_SIZE);

    memset(buffer, 0, BUFFER_SIZE);
    
    pthread_mutex_lock(&connect_mutex_thread);
    if (read(*client_fd, buffer, 51) < 0)
        error("error: reading from socket");
        
    fprintf(stdout,"%s", buffer);
        
    memset(buffer, 0, strlen(buffer));
        
    if (read(*client_fd, buffer, 51) < 0)
            error("error: reading from socket");
    pthread_mutex_unlock(&connect_mutex_thread);
    fprintf(stdout,"%s", buffer);
        
    memset(buffer, 0, BUFFER_SIZE);
        
    
    int index = rand() % username_size;
    fprintf(stdout, "%s\n", usernames[index]);
    
    pthread_mutex_lock(&port_mutex_thread);
    if (write(*client_fd, usernames[index], strlen(usernames[index])) < 0)
            error("error: writing to socket");
    pthread_mutex_unlock(&port_mutex_thread);
    
    pthread_mutex_lock(&connect_mutex_thread);
    if(read(*client_fd, username, BUFFER_SIZE) < 0)
        error("error: reading from socket");
    pthread_mutex_unlock(&connect_mutex_thread);
    client_info * my_connection = (client_info *)malloc(sizeof(client_info));
    
    my_connection->client_connnection = *client_fd;
    my_connection->username = username;
    char * chat_username = (char*)malloc(sizeof(char) * 25);
    sprintf(chat_username, "chat room: %s > ",username);
    my_connection->chat_username = chat_username;

//    my_connection->client_chat_thread = (pthread_t *)malloc(sizeof(pthread_t));
    my_connection->status = 1;
//    fprintf(stdout, "%s", my_connection->username);
    
    pthread_mutex_lock(&port_mutex_thread);
    if (write(*client_fd, opt[3],strlen(opt[3])) < 0)
        error("error: writing to socket");
    pthread_mutex_unlock(&port_mutex_thread);
    memset(buffer1, 0, BUFFER_SIZE);
    pthread_mutex_unlock(&connect_mutex_thread);
    if(read(*client_fd, buffer1, BUFFER_SIZE) < 0)
    {
        my_connection->status = 0;
        error("error: reading from socket");
    }
    pthread_mutex_unlock(&connect_mutex_thread);
    
    pthread_mutex_lock(&chat_room_mutex_thread);
    if(!flights)
    {
        int count = 0;
        int size = 10000;
        flights = (flight_data*)malloc(sizeof(flight_data));
        flights->list = (char**)malloc(sizeof(char*) * size);
        for(int i = 0; buffer1 != NULL; i++)
        {
            if(count < size)
            {
                char * fl = NULL;
                fl = strtok_r(buffer1, "\n",&buffer1);
                strtok(fl, " ");
                flights->list[count++] = fl;
            }
            else
            {
                size *= 2;
                char** temp = (char**)realloc(flights->list, sizeof(char*) * size);
                
                if(temp)
                    flights->list = temp;
            }
        }
        flights->list_size = count;
        
    }
    
    pthread_mutex_unlock(&chat_room_mutex_thread);
    
    memset(buffer, 0, BUFFER_SIZE);
    
    while(my_connection->status)
    {
//        if(strcmp(username, buffer) != 0)
//        {
//            fprintf(stdout, "%s", username);
//        }
        
        if(strcmp(username, my_connection->username) == 0)
        {
            fprintf(stdout, "%s", my_connection->username);
//            my_connection->username_status = 1;
//            my_connection->chat_username_status = 0;
        }
        else if(strcmp(my_connection->chat_username, username) == 0)
        {
            fprintf(stdout, "%s", my_connection->chat_username);
//            my_connection->username_status = 0;
//            my_connection->chat_username_status = 1;
        }
        else
        {
            fprintf(stdout, "%s", my_connection->username);
        }

        
        
        if(strcmp(buffer, "LOGOFF") == 0)
        {
            my_connection->status = 0;
            break;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
        
        if(strcmp(username, my_connection->username) == 0)
        {
            index = rand() % opt_size;
            switch (index) {
                case 0:
                {   //QUERY
                    int sec_arg = rand() % flights->list_size;
                    sprintf(buffer, "%s %s",opt[index], flights->list[sec_arg]);
                    break;
                }
                case 1:
                {   //RESERVE
                    int sec_arg = rand() % flights->list_size;
                    int amount = rand() % 2000;
                    sprintf(buffer, "%s %s %d",opt[index], flights->list[sec_arg], amount);
                    break;
                }
                case 2:
                {   //RETURN
                    int sec_arg = rand() % flights->list_size;
                    int amount = rand() % 2000;
                    sprintf(buffer, "%s %s %d",opt[index], flights->list[sec_arg], amount);
                    break;
                }
                case 3:
                {   //LIST
                    
                    sprintf(buffer, "%s",opt[index]);
                    break;
                }
                case 4:
                {   //LIST_AVAILABLE
                    sprintf(buffer, "%s",opt[index]);
                    break;
                }
                case 5:
                {   //ENTER CHAT
                    sprintf(buffer, "%s",opt[index]);
                    break;
                }
                case 6:
                {
                    //LOGOFF
                    sprintf(buffer, "%s",opt[index]);
                    break;
                }
                default:
                    break;
            }

        }
        else
        {
            index = rand() % opt_chat_size;
            switch (index) {
                case 0:
                {   //TEXT
                    sprintf(buffer, "%s %s",opt_chat[index], "HEllO COP4338 CLASS!");
                    break;
                }
                case 1:
                {   //LIST
                    sprintf(buffer, "%s",opt_chat[index]);
                    break;
                }
                case 2:
                {   //LIST_ALL
                    sprintf(buffer, "%s",opt_chat[index]);
                    break;
                }
                case 3:
                {   //LIST_OFFLINE
                    sprintf(buffer, "%s",opt_chat[index]);
                    break;
                }
                case 4:
                {   //EXIT CHAT
                    sprintf(buffer, "%s",opt_chat[index]);
                    break;
                }
                default:
                    break;
            }

        }
        
        fprintf(stdout, "%s\n", buffer);
        
        pthread_mutex_lock(&port_mutex_thread);
        // send input to server
        if (write(*client_fd, buffer,strlen(buffer)) < 0)
            error("error: writing to socket");
        pthread_mutex_unlock(&port_mutex_thread);
        memset(buffer, 0, BUFFER_SIZE);
        
        pthread_mutex_lock(&connect_mutex_thread);
        if(read(*client_fd, buffer, BUFFER_SIZE) < 0)
        {
            my_connection->status = 0;
            error("error: reading from socket");
        }
        pthread_mutex_unlock(&connect_mutex_thread);
        
        fprintf(stdout, "%s", buffer);
        
        strtok(buffer, "\n"); // remove newline from input
        
        if(strcmp(buffer, username) != 0)
        {
            memset(username, 0, sizeof(username));
            pthread_mutex_lock(&connect_mutex_thread);
            if(read(*client_fd, username, sizeof(username)) < 0)
            {
                my_connection->status = 0;
                error("error: reading from socket");
            }
            pthread_mutex_unlock(&connect_mutex_thread);
        }
        
    }
    
    //free(my_connection);
    
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
