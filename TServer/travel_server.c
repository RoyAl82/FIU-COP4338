/// file travel_server.c
/// Travel agent server application
/// https://gitlab.com/openhid/travel-agency

#define _GNU_SOURCE

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include "hashmap.h"

// Synchronizes access to flight_map
pthread_mutex_t flight_map_mutex;
//pthread_mutex_t flight_data_mutex;
pthread_mutex_t client_map_mutex;

/// @brief contains socket information
/// includes file descriptor port pair.
typedef struct {
    int fd;
    int port;
    char * ip_address;
    int enabled;
    map_t * map;
} socket_info;

typedef struct
{
    char * username;
    struct sockaddr_in client_address;
    int client_connnection;
    pthread_t client_thread;
    socket_info * client_socket;
    pthread_t client_chat_thread;
    pthread_t client_list_thread;
    pthread_t client_reserve_thread;
    pthread_t client_return_thread;
    pthread_t client_query_thread;
    
}client_info;

typedef struct
{
    client_info * client;
    char * args;
    char * num;
    
}client_request_data;

typedef struct
{
    char ** list;
    size_t size;
    
}list_flights;

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

void *client_handler(void * arg);
void * get_flights(char * flight, void * seats);
char* inttochar(int num, char* str, int base);
void reverse(char str[], int length);
void * create_send_data(void * arg);
char * reserve_flight(map_t flight_map, char * flight_token, char * seats_token);
void * list_available(void * arg);
char * return_flight(map_t flight_map, char * flight_token, char * seats_token);
void * get_clients_list(char * username, void * client_data);
void * client_msg_chat(void * arg);
void send_msg_chat(client_info * myself, void ** clients_list, size_t list_size, char * msg);
void * client_query(void * arg);
void * client_return(void * arg);
void * client_reserve(void * arg);
void * client_list(void * arg);
void * client_list_n(void * arg);
void * client_list_available(void * arg);
void * client_list_available_n(void * arg);

void *free_flight_data(char * key, void * data);


map_t client_map;



/// @brief accepts socket connection handler
/// handler is run on separate thread
/// @param [in] socket_info  contains socket info
/// of type socket_info
void * server_handler(void * socket_info);

/// @brief Add flight to flight map
/// @param [in] flight_map hash map appended 
/// @param [in] flight flight string used as index
/// @param [in] seats on flight
void add_flight(map_t flight_map, char * flight, char * seats);

/// @brief frees memory associated with map items 
void free_flight_map_data(map_t flight_map);

/// @brief initializes inet socket with given address information
/// @param sockfd socket_info object to tbe initialized
/// @param ip_address ipv4 addresss
/// @param port adderess port
/// @returns 1 if error, 0 otherwise
int init_inet_socket(socket_info * sockfd, char * ip_address, int port); 

/// @brief binds socket info to sockddr object
/// @param sockfd socket info source
/// @param socket_address inet socket address
void bind_inet_socket(socket_info * sockfd, struct sockaddr_in * socket_address);

/// @brief launches server on @p ports on a thread,
/// listens on ports [start_port, start_port + no_ports].
/// @returns 1 if error, 0 otherwise
int launch_server(socket_info * server, pthread_t * thread);

/// @brief joins server threads, and destroys 
/// related synchronization objects.
void close_servers(socket_info * server, pthread_t * thread, int no_ports);

/// @brief Processes command string from client and provides response
/// @returns NULL if command is invalid, else string response
void * process_flight_request(char * command, client_info * flight_map);

/// @brief reads each line from @file_name expecting (flight, seats)
/// pair. flight pairs are added to @p flight_map
void read_flight_map_file(char * file_name, map_t flight_map);

/// @brief Wraps strcmp to compare in @p string & @p other_string equality
/// @returns 1 if true, false otherwise.
int string_equal(char const * string, char const * other_string);

/// @brief error handler
/// prints error and exits with error status
void error(char * const message) {
    perror(message);
    pthread_exit(NULL);
}

/// @brief iterates @p map and invokes @p callback with key/value
/// pair of each map item
/// @param map hashmap iterated
/// @param callback function pointer taking as arguments
void *hashmap_foreach(map_t map, void*(* callback)(char * key, void * data));


const char * argT[] = {"TServer.exe" ,"127.0.0.1", "43001", "4", "/Users/Roicxy/Projects/HW2/TServer/data.txt","output.txt"}; //For testing

int main (int argc, char * argv[]) {
    // if not enough arguments
//    if (argc < 6) {
//        error("usage: talent_server ip_address start_port no_ports in_file out_file\n");
//    }

    // read command arguments
    char * ip_address = (char *)argT[1];
    int start_port = atoi((char *)argT[2]);
    int no_ports = atoi((char *)argT[3]);
    char * in_filename = (char *)argT[4];
    char * out_filename = (char *)argT[5];

    // initialize flight map
    // Holds flight/seat pairs of clients
    map_t flight_map = hashmap_new();
    
    client_map = hashmap_new();

    read_flight_map_file(in_filename, flight_map);

    /// @brief socket server handlers 
    /// used as thread args
    socket_info servers[no_ports];

    // initialize connection threads
    pthread_t * connection_threads = calloc(no_ports, sizeof(pthread_t));

    int index;
    for (index = 0; index < no_ports; index++) {
        int port = start_port + index;

        // set server map to flight map
        servers[index].map = flight_map;

        // initialize inet sockets
        if (init_inet_socket(&servers[index], ip_address, port) != 0) {
            printf("error: cannot open socket on %s:%d", ip_address, port);
            break;
        }

        // launch servers on sockets
        if (launch_server(&servers[index], &connection_threads[index]) != 0) {
            printf("error: cannot launch server on %s:%d", ip_address, port);
            break;
        }
    }

    // data input
    while (1) {
        // buffer incoming commands from command line
        size_t const BUFFER_SIZE = 256;
        char input[BUFFER_SIZE];
        memset(&input, 0, sizeof(input));

        printf("\nserver: enter command \n");
        fgets(input, sizeof(input), stdin);

        // if exit in command
        if (strstr(input, "EXIT")) {
            break;
        } 
    }

    printf("server: closing servers\n");

    // join server threads
    close_servers(servers, connection_threads, no_ports);
    
    free(connection_threads);

    // free memory of items in hashmap
    free_flight_map_data(flight_map);
    

    // free hashmap handle
    hashmap_free(flight_map);

    return 0; 
} // main

void add_flight(map_t flight_map, char * flight_token, char * seats_token) {
    // assign persistent memory for hash node
    char * flight = malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = malloc(sizeof(char) * strlen(seats_token) + 1);

    // TODO: free memory in request on server thread
    
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);

    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    assert(hashmap_put(flight_map, flight, seats) == MAP_OK);
    pthread_mutex_unlock(&flight_map_mutex);

    printf("server: reserving %s seats on flight %s\n", seats, flight);
} // add_flight

void * hashmap_foreach(map_t flight_map, void *(* callback)(char *, void *)) {
    // data stuctures used to access hashmap internals 
//    struct hashmap_element {
//        char* key;
//        int in_use;
//        any_t data;
//    };
//
//    struct hashmap_map {
//        int table_size;
//        int size;
//        struct hashmap_element *data;
//    };

    struct hashmap_map * map = (struct hashmap_map *) flight_map;
    void ** callback_flight = NULL;
    callback_flight = (void **)calloc(map->size + 2, sizeof(void*));
    
    // iterate each hashmap, invoke callback if index in use
    int index, i;
    for (index = 0, i = 0; index < map->table_size; index++) {
        if (map->data[index].in_use != 0) {
           callback_flight[i++] = callback(map->data[index].key, map->data[index].data);
        }
    }
    
    
    return callback_flight;
} // hashmap_foreach

void * free_flight_data(char * key, void * data) {
    free(key);
    free(data);
    return NULL;
}
char * reserve_flight(map_t flight_map, char * flight_token, char * seats_token)
{
    // assign persistent memory for hash node
    char * flight = NULL;
    flight = (char*)malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = NULL;
    seats = (char*)malloc(sizeof(char) * strlen(seats_token) + 1);
    memset(flight, 0, sizeof(char) * strlen(flight_token) + 1);
    memset(seats, 0, sizeof(char) * strlen(seats_token) + 1);
    
    // TODO: free memory in request on server thread
    
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);
    
     any_t *flight_to_reserve = malloc(sizeof(any_t));
    
    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    
    if(hashmap_get(flight_map, flight, flight_to_reserve) == MAP_OK)
    {
        char * seats_available = (char*) *flight_to_reserve;
        int client_seats = atoi(seats);
        int flight_seats = atoi(seats_available);
        int update_seats = flight_seats - client_seats;
        
        if(update_seats >= 0)
        {
            char * new_update_seats = NULL;
            new_update_seats = (char *)malloc(sizeof(char) * 5);
            inttochar(update_seats, new_update_seats, 10);
            strcpy((*flight_to_reserve), new_update_seats);
            pthread_mutex_unlock(&flight_map_mutex);
            
            free(seats);
            seats = NULL;
            free(flight);
            flight = NULL;
            
            return new_update_seats;
        }
    }
    
    pthread_mutex_unlock(&flight_map_mutex);
    
    free(seats);
    seats = NULL;
    free(flight);
    flight = NULL;
    
    return NULL;
}

char * return_flight(map_t flight_map, char * flight_token, char * seats_token)
{
    // assign persistent memory for hash node
    char * flight = NULL;
    flight = (char*)malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = NULL;
    seats = (char*)malloc(sizeof(char) * strlen(seats_token) + 1);
    memset(flight, 0, sizeof(char) * strlen(flight_token) + 1);
    memset(seats, 0, sizeof(char) * strlen(seats_token) + 1);
    
    // TODO: free memory in request on server thread
    
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);
    
    any_t *flight_to_reserve = malloc(sizeof(any_t));
    
    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    
    if(hashmap_get(flight_map, flight, flight_to_reserve) == MAP_OK)
    {
        char * seats_available = (char*) *flight_to_reserve;
        int client_seats = atoi(seats);
        int flight_seats = atoi(seats_available);
        int update_seats = flight_seats + client_seats;
        
        if(update_seats >= 0)
        {
            char * new_update_seats = NULL;
            new_update_seats = (char *)malloc(sizeof(char) * 5);
            inttochar(update_seats, new_update_seats, 10);
            strcpy((*flight_to_reserve), new_update_seats);
            
            pthread_mutex_unlock(&flight_map_mutex);
            
            free(seats);
            seats = NULL;
            free(flight);
            flight = NULL;
            
            return new_update_seats;
        }
    }
    
    pthread_mutex_unlock(&flight_map_mutex);
    
    free(seats);
    seats = NULL;
    free(flight);
    flight = NULL;
    
    return NULL;
}



void  free_flight_map_data(map_t flight_map) {
    hashmap_foreach(flight_map, &free_flight_data);
//    hashmap_foreach(client_map, <#void *(*callback)(char *, void *)#>);
    
    
} // free_flight_map_data

void * server_handler(void * handler_args) {
    // retrieve socket_info from args
    socket_info * inet_socket = (socket_info *) handler_args;
    
    int clientfd;
    int clientCounter = 0;
    
    // maximum number of input connections queued at a time
    int const MAX_CLIENTS = 100;

    // listen for connections on sockets 
    if (listen(inet_socket->fd, MAX_CLIENTS) < 0) {
        error("error: listening to connections");
    }
    printf("\n%d: listening on %s:%d\n", inet_socket->port, inet_socket->ip_address, inet_socket->port);

    // TODO: replace 1 with semaphore
    while (inet_socket->enabled) {
        // socket address information
        client_info * client = malloc(sizeof(client_info));
        
        // accept incoming connection from client
        socklen_t client_length = sizeof(client->client_address);
        if ((clientfd = accept(inet_socket->fd, (struct sockaddr *) &client->client_address, &client_length)) < 0) {
            error("error: accepting connection");
        }

        client->client_connnection = clientfd;
        client->client_socket = inet_socket;
        
        if(clientCounter++ == 3)
        {
            sleep(10);
            clientCounter = 0;
        }
        
        pthread_create(&client->client_thread, NULL, client_handler, client);
        
    }
    
    // close server socket
    close(inet_socket->fd);
    printf("%d: exiting\n", inet_socket->port);
    
    pthread_exit(handler_args);
} // server_handler

void *client_handler(void * arg)
{
    // data stuctures used to access hashmap internals
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
    
    client_info * client = (client_info *) arg;
    
    char * login = "Enter your Username: ";
    
    const size_t USERNAME_SIZE = 200;
    
    char * userName = (char*)malloc(sizeof(char) * USERNAME_SIZE);
    
    write(client->client_connnection, login, (size_t)strlen(login));
    
    read(client->client_connnection, userName, USERNAME_SIZE);
    
    client->username = (char*)malloc(sizeof(char) * USERNAME_SIZE);
    
    int posfix = 1;
    any_t * dat = malloc(sizeof(any_t));
    pthread_mutex_lock(&client_map_mutex);
    
    while(hashmap_get(client_map, userName, dat) == MAP_OK)
    {
        char * num = malloc(sizeof(char) * posfix);
        inttochar(posfix -1, num, 10);
        userName = strtok_r(NULL, num, &userName);
        sprintf(userName, "%s%d",userName, posfix++);
    }
    sprintf(client->username, "%s",userName);
    hashmap_put(client_map, client->username, (void*)client);
    pthread_mutex_unlock(&client_map_mutex);
    
    strcat(userName, " >: ");
    
   
    
    // holds incoming data from socket
    const size_t BUFFER_SIZE = 2000;
    char * input = (char *)malloc(BUFFER_SIZE);
    list_flights * current_data = NULL;
    
    while(1)
    {
        // get incoming connection information
        struct sockaddr_in * incoming_address = (struct sockaddr_in *) &client->client_address;
        int incoming_port = ntohs(incoming_address->sin_port); // get port
        char incoming_ip_address[INET_ADDRSTRLEN]; // holds ip address string
        // get ip address string from address object
        inet_ntop(AF_INET, &incoming_address->sin_addr, incoming_ip_address, sizeof(incoming_ip_address));
        
        printf("%d: incoming connection from %s:%d\n", client->client_socket->port, incoming_ip_address, incoming_port);
        
        //Send Username to client
        if (write(client->client_connnection, userName, USERNAME_SIZE + 1) < 0) {
            error("error: writing to connection");
        }
        
        
        memset(input, 0, BUFFER_SIZE);
        
        // read data sent from socket into input
        if (read(client->client_connnection, input, BUFFER_SIZE) < 0) {
            error("error: reading from connection");
        }
        
        printf("%d: read %s from client\n", client->client_socket->port, input);
        
        // exit command breaks loop
        if (string_equal(input, "EXIT") || string_equal(input, "LOGOFF")) {
            write(client->client_connnection, "LOGOFF\n", sizeof("LOGOFF\n"));
            break;
        }
        
        // process commands
        current_data = (list_flights *) process_flight_request(input, client);
        
        pthread_t threadid;
        
        pthread_create(&threadid, NULL, create_send_data, current_data);
        
        void * data = NULL;
        
        pthread_join(threadid, &data);
        
        char * sendData = (char*) data;
        
        if (write(client->client_connnection, sendData, strlen(sendData) + 1) < 0) {
            error("error: writing to connection");
        }
    }
    
    
    // close socket
    close(client->client_connnection);
    free(client);
    free(input);
    return NULL;
}//client_handler

void * create_send_data(void * arg)
{   
    list_flights * current_data = NULL;
    current_data = (list_flights *) arg;
    
    char * sendData = NULL;
    size_t size = sizeof(char) * (strlen(current_data->list[0]) + current_data->size + 1);
    sendData = (char *)malloc(size);
    memset(sendData, 0, size);
    
    for(int i = 0; i < current_data->size ; i++)
    {
        if(current_data->list[i] != NULL)
        {
            strcat(sendData, current_data->list[i]);
            free(current_data->list[i]);
            current_data->list[i] = NULL;
        }
    }
    free(current_data->list);
    current_data->list = NULL;
    free(current_data);
    current_data = NULL;
    
    return (void *) sendData;
}

void *get_flights(char * flight, void * seats)
{
    char * newFlight = NULL;
    size_t newFlightSize = sizeof(char) * strlen(flight) + strlen((char *)seats) + strlen("    ") + strlen("\n") + 1;
    
    newFlight = (char *)malloc(newFlightSize);
    
    memset((void *)newFlight, 0, newFlightSize);
    sprintf(newFlight, "%7s %5s\n", flight, (char *)seats);
    
    return (void *)newFlight;
}

void close_servers(socket_info * servers, pthread_t * threads, int no_ports) {
    int index;
    for (index = 0; index < no_ports; index++) {
        /// signal threads sockets are disabled
        servers[index].enabled = 0;
        pthread_join(threads[index], NULL);
    }

    pthread_mutex_destroy(&flight_map_mutex);
    pthread_mutex_destroy(&client_map_mutex);
    
} // close_servers

int init_inet_socket(socket_info * inet_socket, char * ip_address, int port) {
    // enable socket
    inet_socket->enabled = 1;
    inet_socket->port = port;

    // copy ip_address to socket_info object
    inet_socket->ip_address = malloc(sizeof(char) * strlen(ip_address) + 1);
    strcpy(inet_socket->ip_address, ip_address);
    
    struct sockaddr_in server_address;
    
    // open server socket 
    if ((inet_socket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error: opening server socket");
        return 1;
    }
    
    if (setsockopt(inet_socket->fd, 
                   SOL_SOCKET, 
                   SO_REUSEADDR, 
                   &inet_socket->enabled, 
                   sizeof(int)) < 0) {
        perror("error: cannot set socket options");
        return 1;
    }

    // initialize server_address (sin_zero) to 0
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET; // set socket type, INET sockets are bound to IP 
    server_address.sin_port = htons(port); // assign port number
    inet_aton(ip_address, &server_address.sin_addr); // assign socket ip address

    // bind socket information to socket handler
    if (bind(inet_socket->fd, (struct sockaddr *) & server_address, sizeof(server_address)) < 0) {
        perror("error: binding socket to address");
        return 1;
    }

    return 0;
} // init_inet_socket

int launch_server(socket_info * server, pthread_t * thread) {
    /// initialize multithreading objects
    pthread_mutex_init(&flight_map_mutex, NULL);
    pthread_mutex_init(&client_map_mutex, NULL);

    if (pthread_create(thread, NULL, server_handler, server) == 0) {
        printf("server: started server on port %d\n", server->port);
        return 0;
    }

    return 1;
} // launch_server

void read_flight_map_file(char * file_name, map_t flight_map) {
    // read input data
    FILE * file;
    if (!(file = fopen(file_name, "r"))) {
        perror("error: cannot open input file"); 
        exit(1);
    }

    // read all lines from input
    size_t length = 0;
    ssize_t read;
    char * input = malloc(sizeof(char));
    while ((read = getline(&input, &length, file)) != -1) {
        // get tokens from input
       	char * input_tokens = NULL;
	    char * flight = strtok_r(input, " ", &input_tokens);
        char * seats = strtok_r(NULL, "\n", &input_tokens);

        add_flight(flight_map, flight, seats);
    }
} // read_flight_map_file

int string_equal(char const * string, char const * other_string) {
    return strcmp(string, other_string) == 0; 
} // string_equal

void print_flight(char * flight, void * seats) {
    printf("%s %s \n", flight, (char *)seats);
}
void * client_query(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    char * flight = NULL;
    if (!(flight = strtok_r(NULL, " ", &client_data->args))) {
        return "error: cannot proccess flight to query";
    }
    
    char * seats;
    int result;
    printf("server: querying flight %s\n", flight);
    pthread_mutex_lock(&flight_map_mutex);
    // retrieve seats from map
    result = hashmap_get(client_data->client->client_socket->map, flight, (void**) &seats);
    pthread_mutex_unlock(&flight_map_mutex);
    
    if (result != MAP_OK) {
        return "error: query failed";
    }
    list_flights * seats_flights = NULL;
    seats_flights = (list_flights *) malloc(sizeof(list_flights));
    seats_flights->list =(char **) malloc(sizeof(char *));
    char * newSeats = NULL;
    newSeats = (char*) malloc(sizeof(char) * strlen(seats) + 1);
    memset(newSeats, 0, sizeof(char) * strlen(seats) + 1);
    sprintf(newSeats, "%s\n", seats);
    seats_flights->list[0] = newSeats;
    seats_flights->size = 1;
    
    return (void *)seats_flights;
}
void * client_return(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    char * flight = NULL;
    char * seats = NULL;
    
    if (!(flight = strtok_r(NULL, " ", &client_data->args))) {
        return "error: cannot process flight to reserve";
    }
    
    if (!(seats = strtok_r(NULL, " ", &client_data->args))) {
        return "error: cannot process seats to reserve";
    }
    
    printf("server: returning %s seats on flight %s\n", seats, flight);
    char * status = return_flight(client_data->client->client_socket->map, flight, seats);
    
    list_flights * reserve_flights = NULL;
    reserve_flights = (list_flights *)malloc(sizeof(list_flights));
    reserve_flights->list = (char **)malloc(sizeof(char*));
    char * status_add_flight = NULL;
    char * message = "Your return flight -------- with -------- seats was (UN)SUCCESSFUL.\n";
    
    status_add_flight = (char *)malloc(sizeof(char) * strlen(message) + 1);
    memset(status_add_flight, 0, sizeof(char) * strlen(message) + 1);
    
    if(status)
        sprintf(status_add_flight, "Your return flight %s with %s seats was SUSCCESSFUL.\n", flight, seats);
    else
        sprintf(status_add_flight, "Your return flight %s with %s seats was UNSUCCESSFUl.\n", flight, seats);
    
    reserve_flights->list[0] = status_add_flight;
    reserve_flights->size = 1;
    
    return (void *)reserve_flights;

}
void * client_reserve(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    char * flight = NULL;
    char * seats = NULL;
    
    if (!(flight = strtok_r(NULL, " ", &client_data->args))) {
        return "error: cannot process flight to reserve";
    }
    
    if (!(seats = strtok_r(NULL, " ", &client_data->args))) {
        return "error: cannot process seats to reserve";
    }
    
    printf("server: reserving %s seats on flight %s\n", seats, flight);
    char * status = reserve_flight(client_data->client->client_socket->map, flight, seats);
    
    list_flights * reserve_flights = NULL;
    reserve_flights = (list_flights *)malloc(sizeof(list_flights));
    reserve_flights->list = (char **)malloc(sizeof(char*));
    char * status_add_flight = NULL;
    char * message = "Your reservation flight -------- with -------- seats was (UN)SUCCESSFUL.\n";
    
    status_add_flight = (char *)malloc(sizeof(char) * strlen(message) + 1);
    memset(status_add_flight, 0, sizeof(char) * strlen(message) + 1);
    
    if(status)
        sprintf(status_add_flight, "Your reservation flight %s with %s seats was SUSCCESSFUL.\n", flight, seats);
    else
        sprintf(status_add_flight, "Your reservation flight %s with %s seats was UNSUCCESSFUl.\n", flight, seats);
    
    reserve_flights->list[0] = status_add_flight;
    reserve_flights->size = 1;
    
    return (void *)reserve_flights;
}
void * client_list(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    printf("server: listing flights\n");
    list_flights * list = (list_flights *)malloc(sizeof(list_flights));
    
    pthread_mutex_lock(&flight_map_mutex);
    
    list->list = (char**)hashmap_foreach(client_data->client->client_socket->map, &get_flights);
    
    pthread_mutex_unlock(&flight_map_mutex);
    struct hashmap_map * m = (struct hashmap_map *)client_data->client->client_socket->map;
    list->size = m->size;
    
    return list;
}
void * client_list_n(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    printf("server: listing flights\n");
    list_flights * list = (list_flights*) malloc(sizeof(list_flights));
    
    pthread_mutex_lock(&flight_map_mutex);
    
    list->list = (char**)hashmap_foreach(client_data->client->client_socket->map, &get_flights);
    
    pthread_mutex_unlock(&flight_map_mutex);
    
    struct hashmap_map * m = (struct hashmap_map *)client_data->client->client_socket->map;
    list->size = m->size;
    
    int n = atoi(client_data->num);
    
    if(n < list->size)
    {
        for(int i = n; i < list->size;i++)
            free(list->list[i]);
        list->size = n;
    }
    
    return (void *) list;
}
void * client_list_available(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    list_flights * list = (list_flights *)malloc(sizeof(list_flights));
    pthread_mutex_lock(&flight_map_mutex);
    
    list->list = (char**)hashmap_foreach(client_data->client->client_socket->map, &get_flights);
    
    pthread_mutex_unlock(&flight_map_mutex);
    struct hashmap_map * m = (struct hashmap_map *)client_data->client->client_socket->map;
    list->size = m->size;
    
    pthread_t thread_to_list;
    
    pthread_create(&thread_to_list, NULL, list_available, list);
    
    void * new_list = NULL;
    pthread_join(thread_to_list, &new_list);
    
    
    return new_list;
}
void * client_list_available_n(void * arg)
{
    client_request_data * client_data = (client_request_data *) arg;
    
    list_flights * list = (list_flights *)malloc(sizeof(list_flights));
    pthread_mutex_lock(&flight_map_mutex);
    
    list->list = (char**)hashmap_foreach(client_data->client->client_socket->map, &get_flights);
    
    pthread_mutex_unlock(&flight_map_mutex);
    struct hashmap_map * m = (struct hashmap_map *)client_data->client->client_socket->map;
    
    list->size = m->size;
    
    pthread_t thread_to_list;
    
    pthread_create(&thread_to_list, NULL, list_available, list);
    
    void * new_list = NULL;
    pthread_join(thread_to_list, &new_list);
    
    list = (list_flights *) new_list;
    
    int n = atoi(client_data->num);
    
    if(n < list->size)
    {
        for(int i = n; i < list->size;i++)
            free(list->list[i]);
        list->size = n;
    }
    
    return (void *) list;
}

void * process_flight_request(char * input, client_info * client_flight_map) {
    // parse input for commands
    // for now we're just taking the direct command
    // split command string to get command and arguments
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
    
//    map_t * flight_map = client_flight_map->client_socket->map;
    
    char * input_tokens = NULL; // used to save tokens when splitting string  
    char * command = NULL; 
    char * number = NULL;
    
    if (!(command= strtok_r(input, " ", &input_tokens)))
    {
        list_flights * error_message = (list_flights *)malloc(sizeof(list_flights));
        error_message->list = (char**)malloc(sizeof(char*));
        char * message = NULL;
        char * messageLen = "Error: cannot proccess server command\n";
        
        message = (char *)malloc(sizeof(char) * strlen(messageLen) + 1);
        memset(message, 0, sizeof(char) * strlen(messageLen) + 1);
        
        sprintf(message, "Error: cannot proccess server command\n");
        error_message->list[0] = message;
        error_message->size = 1;
        
        return (void *) error_message;
	}

    if (string_equal(command, "QUERY"))
    {
//		// get flight to query 
//		char * flight = NULL; 
//		if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
//			return "error: cannot proccess flight to query";
//		}
//
//		char * seats;
//		int result;
//		printf("server: querying flight %s\n", flight);
//		pthread_mutex_lock(&flight_map_mutex);
//		// retrieve seats from map
//		result = hashmap_get(flight_map, flight, (void**) &seats);
//		pthread_mutex_unlock(&flight_map_mutex);
//
//		if (result != MAP_OK) {
//			return "error: query failed"; 
//		}
//        list_flights * seats_flights = NULL;
//        seats_flights = (list_flights *) malloc(sizeof(list_flights));
//        seats_flights->list =(char **) malloc(sizeof(char *));
//        char * newSeats = NULL;
//        newSeats = (char*) malloc(sizeof(char) * strlen(seats) + 1);
//        memset(newSeats, 0, sizeof(char) * strlen(seats) + 1);
//        sprintf(newSeats, "%s\n", seats);
//        seats_flights->list[0] = newSeats;
//        seats_flights->size = 1;
//        
//		return (void *)seats_flights;
        
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        void * query = NULL;
        pthread_create(&client_flight_map->client_query_thread, NULL, client_query, myData);
        pthread_join(client_flight_map->client_query_thread, &query);
        free(myData);
        
        return query;
    }
    else if (string_equal(command, "LIST") && !(number = strtok_r(NULL, " ", &input_tokens)))
    {
//        printf("server: listing flights\n");
//        list_flights * list = (list_flights *)malloc(sizeof(list_flights));
//        
//        pthread_mutex_lock(&flight_map_mutex);
//        
//        list->list = (char**)hashmap_foreach(flight_map, &get_flights);
//        
//        pthread_mutex_unlock(&flight_map_mutex);
//        struct hashmap_map * m = (struct hashmap_map *)client_flight_map->client_socket->map;
//        list->size = m->size;
//        
//        return list;
        
        
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        
        void * list_flight = NULL;
        pthread_create(&client_flight_map->client_list_thread, NULL, client_list, myData);
        pthread_join(client_flight_map->client_list_thread, &list_flight);
        free(myData);
        
        return list_flight;
        
    }
    else if(string_equal(command, "LIST") && number )
    {
//        printf("server: listing flights\n");
//        list_flights * list = (list_flights*) malloc(sizeof(list_flights));
//        
//        pthread_mutex_lock(&flight_map_mutex);
//        
//        list->list = (char**)hashmap_foreach(flight_map, &get_flights);
//        
//        pthread_mutex_unlock(&flight_map_mutex);
//        
//        struct hashmap_map * m = (struct hashmap_map *)client_flight_map->client_socket->map;
//        list->size = m->size;
//        
//        int n = atoi(number);
//        
//        if(n < list->size)
//        {
//            for(int i = n; i < list->size;i++)
//                free(list->list[i]);
//            list->size = n;
//        }
//        
//        return (void *) list;
        
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        myData->num = number;
        void * list_n_flight = NULL;
        pthread_create(&client_flight_map->client_list_thread, NULL, client_list_n, myData);
        pthread_join(client_flight_map->client_list_thread, &list_n_flight);
        free(myData);
        
        return list_n_flight;

        
    }
    else if(string_equal(command, "RETURN"))
    {
//        char * flight = NULL;
//        char * seats = NULL;
//        
//        if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
//            return "error: cannot process flight to reserve";
//        }
//        
//        if (!(seats = strtok_r(NULL, " ", &input_tokens))) {
//            return "error: cannot process seats to reserve";
//        }
//        
//        printf("server: reserving %s seats on flight %s\n", seats, flight);
//        char * status = return_flight(flight_map, flight, seats);
//        
//        list_flights * reserve_flights = NULL;
//        reserve_flights = (list_flights *)malloc(sizeof(list_flights));
//        reserve_flights->list = (char **)malloc(sizeof(char*));
//        char * status_add_flight = NULL;
//        char * message = "Your return flight -------- with -------- seats was (UN)SUCCESSFUL.\n";
//        
//        status_add_flight = (char *)malloc(sizeof(char) * strlen(message) + 1);
//        memset(status_add_flight, 0, sizeof(char) * strlen(message) + 1);
//        
//        if(status)
//            sprintf(status_add_flight, "Your return flight %s with %s seats was SUSCCESSFUL.\n", flight, seats);
//        else
//            sprintf(status_add_flight, "Your return flight %s with %s seats was UNSUCCESSFUl.\n", flight, seats);
//        
//        reserve_flights->list[0] = status_add_flight;
//        reserve_flights->size = 1;
//        
//        return (void *)reserve_flights;
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        void * return_flight = NULL;
        pthread_create(&client_flight_map->client_return_thread, NULL, client_return, myData);
        pthread_join(client_flight_map->client_return_thread, &return_flight);
        free(myData);
        
        return return_flight;
    }
    else if (string_equal(command, "RESERVE"))
    {
//        char * flight = NULL;
//        char * seats = NULL;
//
//        if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
//            return "error: cannot process flight to reserve";
//        }
//
//        if (!(seats = strtok_r(NULL, " ", &input_tokens))) {
//            return "error: cannot process seats to reserve";
//        }
//
//        printf("server: reserving %s seats on flight %s\n", seats, flight);
//        char * status = reserve_flight(flight_map, flight, seats);
//        
//        list_flights * reserve_flights = NULL;
//        reserve_flights = (list_flights *)malloc(sizeof(list_flights));
//        reserve_flights->list = (char **)malloc(sizeof(char*));
//        char * status_add_flight = NULL;
//        char * message = "Your reservation flight -------- with -------- seats was (UN)SUCCESSFUL.\n";
//        
//        status_add_flight = (char *)malloc(sizeof(char) * strlen(message) + 1);
//        memset(status_add_flight, 0, sizeof(char) * strlen(message) + 1);
//        
//        if(status)
//            sprintf(status_add_flight, "Your reservation flight %s with %s seats was SUSCCESSFUL.\n", flight, seats);
//        else
//            sprintf(status_add_flight, "Your reservation flight %s with %s seats was UNSUCCESSFUl.\n", flight, seats);
//        
//        reserve_flights->list[0] = status_add_flight;
//        reserve_flights->size = 1;
//        
//        return (void *)reserve_flights;
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        void * reserve_flight = NULL;
        pthread_create(&client_flight_map->client_reserve_thread, NULL, client_reserve, myData);
        pthread_join(client_flight_map->client_reserve_thread, &reserve_flight);
        free(myData);
        
        return reserve_flight;
        
    }
    else if(string_equal(command, "LIST_AVAILABLE") && !(number = strtok_r(NULL, " ", &input_tokens)))
    {
//        list_flights * list = (list_flights *)malloc(sizeof(list_flights));
//        pthread_mutex_lock(&flight_map_mutex);
//        
//        list->list = (char**)hashmap_foreach(flight_map, &get_flights);
//        
//        pthread_mutex_unlock(&flight_map_mutex);
//        struct hashmap_map * m = (struct hashmap_map *)client_flight_map->client_socket->map;
//        list->size = m->size;
//        
//        pthread_t thread_to_list;
//        
//        pthread_create(&thread_to_list, NULL, list_available, list);
//        
//        void * new_list = NULL;
//        pthread_join(thread_to_list, &new_list);
//        
//        
//        return new_list;
        
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        void * list_available_flight = NULL;
        pthread_create(&client_flight_map->client_list_thread, NULL, client_list, myData);
        pthread_join(client_flight_map->client_list_thread, &list_available_flight);
        free(myData);
        
        return list_available_flight;
        
    }
    else if(string_equal(command, "LIST_AVAILABLE") && number)
    {
//        list_flights * list = (list_flights *)malloc(sizeof(list_flights));
//        pthread_mutex_lock(&flight_map_mutex);
//        
//        list->list = (char**)hashmap_foreach(flight_map, &get_flights);
//        
//        pthread_mutex_unlock(&flight_map_mutex);
//        struct hashmap_map * m = (struct hashmap_map *)client_flight_map->client_socket->map;
//        
//        list->size = m->size;
//        
//        pthread_t thread_to_list;
//        
//        pthread_create(&thread_to_list, NULL, list_available, list);
//        
//        void * new_list = NULL;
//        pthread_join(thread_to_list, &new_list);
//        
//        list = (list_flights *) new_list;
//        
//        int n = atoi(number);
//        
//        if(n < list->size)
//        {
//            for(int i = n; i < list->size;i++)
//                free(list->list[i]);
//            list->size = n;
//        }
//        
//        return (void *) list;
        
        client_request_data * myData = (client_request_data*)malloc(sizeof(client_request_data));
        
        myData->args = input_tokens;
        myData->client = client_flight_map;
        myData->num = number;
        void * list_available_n_flight = NULL;
        pthread_create(&client_flight_map->client_list_thread, NULL, client_list_n, myData);
        pthread_join(client_flight_map->client_list_thread, &list_available_n_flight);
        free(myData);
        
        return list_available_n_flight;
    }
    else if(string_equal(command, "CHAT"))
    {
        pthread_create(&client_flight_map->client_chat_thread, NULL, client_msg_chat, client_flight_map);
        
        pthread_join(client_flight_map->client_chat_thread, NULL);
        
        list_flights * chat_exit_message = (list_flights *)malloc(sizeof(list_flights));
        chat_exit_message->list = (char**)malloc(sizeof(char*));
        char * message = NULL;
        char * messageLen = "CHAT LOGOFF SUCCESSFUL.\n";
        
        message = (char *)malloc(sizeof(char) * strlen(messageLen) + 1);
        memset(message, 0, sizeof(char) * strlen(messageLen) + 1);
        
        sprintf(message, "CHAT LOGOFF SUCCESSFUL.\n");

        chat_exit_message->list[0] = message;
        chat_exit_message->size = 1;
        
        return (void *) chat_exit_message;
    }
    
    list_flights * error_message = (list_flights *)malloc(sizeof(list_flights));
    error_message->list = (char**)malloc(sizeof(char*));
    char * message = NULL;
    char * messageLen = "WARNNING!: Cannot recognize command ------------------\nPlease, try again.\n";
        
    message = (char *)malloc(sizeof(char) * strlen(messageLen) + 1);
    memset(message, 0, sizeof(char) * strlen(messageLen) + 1);
        
    sprintf(message, "WARNNING!: Cannot recognize command %s\nPlease, try again.\n", command);
    error_message->list[0] = message;
    error_message->size = 1;
    
    return (void *) error_message;
    
	
} // process_flight_request

void * client_msg_chat(void * arg)
{
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

    client_info * client = (client_info*) arg;
    
    size_t buff_len = sizeof(char) * 2000;
    char * buff = (char *) malloc(buff_len);
    
    char * chat_user = (char *) malloc(sizeof(char) * 250);
    sprintf(chat_user, "chat room: %s > ", client->username);
    
    struct hashmap_map * map = (struct hashmap_map *)client_map;
    
    write(client->client_connnection, "ENTER CHAT\n", strlen("ENTER CHAT\n"));
    
    write(client->client_connnection, chat_user, strlen(chat_user));
    
//    void ** clients_list = hashmap_foreach(client_map, &get_clients_list);
    
    while (1)
    {
        read(client->client_connnection, buff, buff_len);
        
        if(string_equal(buff, "EXIT"))
            break;
        
        pthread_mutex_lock(&client_map_mutex);
        void ** clients_list = hashmap_foreach(client_map, &get_clients_list);
        pthread_mutex_unlock(&client_map_mutex);
        
        send_msg_chat(client, clients_list, map->size, buff);
        
    
        write(client->client_connnection, chat_user, strlen(chat_user));
        memset(buff, 0, buff_len);
    }
   
    pthread_exit(NULL);
}

void * get_clients_list(char * username, void * client_data)
{
    return client_data;
}


void send_msg_chat(client_info * myself, void ** clients_list, size_t list_size, char * msg)
{
    client_info * clientfd = NULL;
    char * send_prefix_msg = "Chat message from %s :\n%s\n";
    size_t send_msg_len = sizeof(char) * strlen(msg) + strlen(myself->username) + strlen(send_prefix_msg) + 10;
    char * send_msg = (char *)malloc(send_msg_len);
    sprintf(send_msg, "Chat message from %s :\n%s\n", myself->username, msg);
    
    for (int i = 0 ; i < list_size; i++)
    {
        clientfd = clients_list[i];
        
        if(string_equal(clientfd->username, myself->username) == 0)
        {
            write(clientfd->client_connnection, send_msg, strlen(send_msg));
        }
    }
    
    free(send_msg);
    
}

void * list_available(void * arg)
{
    list_flights * list = (list_flights*) arg;
    
    list_flights * new_list = NULL;
    new_list = (list_flights *)malloc(sizeof(list_flights));
    char ** new_flight_list = NULL;
    new_flight_list = (char **)malloc(sizeof(char*) * list->size + 1);
    memset(new_flight_list, 0, sizeof(char*) * list->size + 1);
    
    char * flights = NULL;
    char * seats = NULL;
    int numSeats = 0;
    size_t len = sizeof(char) * strlen(list->list[0]) + 10;
    
    flights = (char*) malloc(len);
    
    for (int i = 0, j = 0; i < list->size; i++)
    {
        memset(flights, 0, len);
        strcpy(flights, list->list[i]);
        strtok_r(NULL, " ", &flights);
        
        if((seats = strtok_r(NULL, " ", &flights)) && (numSeats = atoi(seats)) && numSeats > 0)
        {
            new_flight_list[j++] = list->list[i];
            new_list->size = j;
        }
        
    }
    free(flights);
    free(list->list);
    list->list = NULL;
    free(list);
    list = NULL;
    
    new_list->list = new_flight_list;
    
    return (void *) new_list;
}


char* inttochar(int num, char* str, int base)
{
    int i = 0;
    int isNegative = 0;
    
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10)
    {
        isNegative = 1;
        num = -num;
    }
    
    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
    
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
    
    str[i] = '\0'; // Append string terminator
    
    // Reverse the string
    reverse(str, i);
    
    return str;
}

/* A utility function to reverse a string  */
void reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end)
    {
        str[start] = str[start] + str[end];
        str[end] = str[start] - str[end];
        str[start] = str[start] - str[end];
        
//        swap(*(str+start), *(str+end));
        start++;
        end--;
    }
}
