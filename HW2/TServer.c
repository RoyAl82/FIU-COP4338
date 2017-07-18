//
//  TServer.c
//  HW2
//
//  Created by Roicxy Alonso Gonzalez on 7/17/17.
//  Copyright © 2017 AlonsoRoicxy. All rights reserved.
//


#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <sys/types.h>
#include <pthread.h>
#include "hashmap.h"



#define FALSE 0
#define TRUE 1
#define DEFAULT_TABLE_SIZE 26

#define NUM_OF_ARGUMENTS 6
#define IP_ADDRESS_INDEX 1
#define STARTING_PORT 2
#define NUMBER_OF_PORT 3
#define READING_FILE_INDEX 4
#define WRITING_FILE_INDEX 5
#define SIZE_OF_CHARACTERS 255



char * getReadingFile(const char * arg[], int argIndex);
char * getWritingFile(const char * arg[], int argIndex);
size_t getNumOfArguments(const char * arg[]);
char * readFile(FILE * readFile);
int getNumOfSeatsFromFile(char * elem);
int addFligtsToTable(FILE * read, map_t * myTable);
int createTableFromFile(FILE * read, map_t * myTable);
char ** agentsConnection(void ** socketTable);
void * agents(void *);



typedef struct
{
    char * flights;
    size_t seats;
    
}Flights;

typedef struct
{
    int socketID;
    struct sockaddr_in sockAddress;
    int socketBind;
    int socketList;
    socklen_t addrlen;
    
    
}Socket;




const char * argT[] = {"TServer.exe" ,"127.0.0.1", "9000", "4", "/Users/Roicxy/Projects/HW2/HW2/data.txt","output.txt"}; //For testing



int main(int argc, const char * argv[])
{
    if(getNumOfArguments(argT) == NUM_OF_ARGUMENTS)
    {
        char * rf = getReadingFile(argT, READING_FILE_INDEX);
        
        char * wf = getWritingFile(argT, WRITING_FILE_INDEX);
        
        FILE * fROpen;
        FILE * fWOpen;
        
        if((fROpen = fopen(rf, "r")) && (fWOpen = fopen(wf, "w")))
        {
            map_t myTable = hashmap_new();
            
            if(createTableFromFile(fROpen, myTable))
            {
                void ** socketTables = calloc(10, sizeof(void*) * 10);
                
                int ** socketTable = malloc(sizeof(int*));
                int port = 9000;
                
                Socket * socketItem = malloc(sizeof(Socket));
                
                
                //agentsConnection(socketTable);
                
                for(int i = 0; i < 8; i++)
                {
                    //socketTable[i] = malloc(sizeof(int));
                    
                    socketItem->socketID= socket(AF_INET, SOCK_STREAM, 0);
                    
                    
                    
                    //struct sockaddr_in sockAddress;
                    inet_aton(argT[1], &socketItem->sockAddress.sin_addr);
                    
                    //socketItem->sockAddress.sin_addr.s_addr = INADDR_ANY;
                    socketItem->sockAddress.sin_port = htonl(port++);
                    socketItem->sockAddress.sin_family = AF_INET;
                    socketItem->addrlen = sizeof((struct sockaddr_in)socketItem->sockAddress);
                    
                    int socketBind = bind(socketItem->socketID, (const struct sockaddr *)&socketItem->sockAddress, socketItem->addrlen);
//                    int socketList = listen(*socketTable[i], 5);
//                    socklen_t addrlen = sizeof(sockAddress);
//                    int socketAccept = accept(*socketTable[i], (struct sockaddr*)&sockAddress, &addrlen);
                    socketTables[i] = socketItem;
                    
                }
                
                agentsConnection(socketTables);
                
                
                
//                for(int i = 0; i < 4; i++)
//                {
//                    char * item = "MIAMI-ORL";
//                    Flights * getFlights;
//                    any_t flightItem = NULL;
//                    
//                    hashmap_get(myTable, item, &flightItem);
//                    
//                    getFlights = (Flights*) flightItem;
//                    if(getFlights)
//                        printf("new Data: %s %zu\n", getFlights->flights,getFlights->seats);
//                    
//                    getFlights->seats--;
//                    
//                }
               
                
//                char ** newItem = agentsConnection();
                
                
                
            }
            else
            {
                printf("An Error has occured. Data could NOT be read in its totality.\n");
                printf("Run the program again.\n");
                printf("Program Ended. Exiting.........\n");
                exit(-3);
            }
        }
        else
        {
            printf("Files does NOT exist.\n");
            printf("Please, enter the Files' path.\n");
            printf("Program Ended. Exiting.......\n");
            exit(-2);
        }
    }
    else
    {
        printf("The numbers of arguments does NOT match the format allow.\n");
        printf("The arguments format is: \n");
        printf("1: Name of program\n2: IP\n3: Beginning Port\n");
        printf("4: Number of Port\n5: Data File\n6: Output Data\n");
        printf("Program Ended. Exiting........\n");
        exit(-1);
    }
    
    printf("Thanks for using our services.\n");
    printf("Our Goal is to serve you.\n");
    printf("Program Ended. Exiting........\n");
    return 0;
}
/*
 *
 *
 */
char * getReadingFile(const char * arg[], int argIndex)
{
      if(arg[argIndex])
          return (char*)arg[argIndex];
    
    return NULL;
}
/*
 *
 *
 */
char * getWritingFile(const char * arg[], int argIndex)
{
    if(arg[argIndex])
        return (char*)arg[argIndex];
    
    return NULL;
}
/*
 *
 *
 */
size_t getNumOfArguments(const char * arg[])
{
    int numOfArguments = 0;
    
    while(arg[numOfArguments] != NULL)
        numOfArguments++;
    
    return numOfArguments;
}
/*
 *
 *
 */
char * readFile(FILE * readFile)
{
    char newItem[SIZE_OF_CHARACTERS];
    
    for(int i = 0; i < SIZE_OF_CHARACTERS; i++)
        newItem[i] = '\0';
    
    if(!fgets(newItem, SIZE_OF_CHARACTERS,readFile))
        return NULL;
    
    int len;
    for(len = 0; newItem[len] != '\0' && newItem[len] != '\n'; len++);
    
    char * newFlight = calloc(len,sizeof(char) * len);
    if(!newFlight)
        return NULL;
    
    int i;
    for(i = 0; i < len; i++)
        newFlight[i] = newItem[i];
    newFlight[i] = '\0';
    
    return newFlight;
}
/*
 *
 *
 */
int getNumOfSeatsFromFile(char * elem)
{
    int i;
    int j;
    
    for (i = 0; elem[i] != ' '; i++);
    
    for(j = 0; elem[j] != '\0'; j++);
    
    int num = atoi(&elem[i]);
    elem[i] = '\0';
    for(int k = i; k < j; k++)
        elem[k] = '\0';
    
    char * tmp = realloc(elem, sizeof(char) * i);
    
    if(tmp)
      elem = tmp;
    
    return num;
}
/*
 *
 *
 */
int createTableFromFile(FILE * read, map_t * myTable)
{
    char * newElem;
   
    while((newElem = readFile(read)) && newElem)
    {
        Flights * newFlights = malloc(sizeof(Flights));
        
        newFlights->seats = getNumOfSeatsFromFile(newElem);
        newFlights->flights = newElem;
        
        if(hashmap_put(myTable, newElem, newFlights))
            return FALSE;
    }
    
    return TRUE;
}
/*
 *
 *
 */
int processSales(map_t * myTable, char * trip, int seats)
{
     if(myTable && trip && seats)
     {
         Flights * flights = NULL;
         any_t * item = NULL;
         
         hashmap_get(myTable, trip, &item);
         
         flights = (Flights *) item;
         
         if(seats > flights->seats || flights->seats == 0)
             return FALSE;
         
         flights->seats--;
         
         return TRUE;
     }
    return FALSE;
}
/*
 *
 *
 */
int reverseSales(map_t * myTable, char * trip, int seats)
{
    if(myTable && trip && seats)
    {
        Flights * flights = NULL;
        
        any_t * item = NULL;
        
        hashmap_get(myTable, trip, flights);
        
        flights = (Flights *) item;
        
        flights->seats += seats;
        
        return TRUE;
    }
    return FALSE;
}
/*
 *
 *
 */
int tripHasOpenSeats(map_t * myTable, char * trip)
{
    if(myTable && trip)
    {
        Flights * flights = NULL;
        
        any_t * item = NULL;
        
        hashmap_get(myTable, trip, flights);
        
        flights = (Flights *) item;
        
        return flights->seats;
    }
    
    return FALSE;
}
/*
 *
 *
 */
char ** agentsConnection(void** socketTable)
{
    
    pthread_t agent1;
    pthread_t agent2;
    pthread_t agent3;
    
    char * msg1 = malloc(sizeof(char) * SIZE_OF_CHARACTERS);
    char * msg2 = malloc(sizeof(char) * SIZE_OF_CHARACTERS);
    char * msg3 = malloc(sizeof(char) * SIZE_OF_CHARACTERS);
    
    int index = 0;
    
    pthread_create(&agent1, NULL, agents((void*) &socketTable), NULL);
    
    Socket * sockets = socketTable[index++];
    
    accept(sockets->socketID, &sockets->sockAddress, sockets->addrlen);
    
    
    
    pthread_create(&agent2, NULL, agents((void*) &socketTable), NULL);
    
    sockets = socketTable[index++];
    
    accept(sockets->socketID, &sockets->sockAddress, sockets->addrlen);
    
    
    
    //pthread_create(&agent3, NULL, agents((void*) &socketTable), NULL);
    
    pthread_cond_t agent1Cond;
    
    pthread_mutex_t agMutex;
    
    int mutexT = pthread_mutex_init(&agMutex, NULL);
    
    int condT = pthread_cond_init(&agent1Cond, NULL);
    
    int condSignal = pthread_cond_signal_thread_np(&agent1Cond, agent1);
    int condSigna2 = pthread_cond_signal_thread_np(&agent1Cond, agent2);
    
    int mutexLock = pthread_mutex_lock(&agMutex);
    
    int condWait = pthread_cond_wait(&agent1Cond, &agMutex);
    
    int mutexUnlock = pthread_mutex_unlock(&agMutex);
    
    
    //int condSigna3 = pthread_cond_signal_thread_np(&agent1Cond, agent3);

    
    pthread_join(agent1, &msg1);
    pthread_join(agent2, &msg2);
    //pthread_join(agent3, &msg3);
    
    char ** table = calloc(3, sizeof(char*) * 3);
    table[0] = msg1;
    table[1] = msg2;
    table[2] = msg3;
    
    return table;
}
void * agents(void * in)
{
    char * buff = calloc(SIZE_OF_CHARACTERS, sizeof(char) * SIZE_OF_CHARACTERS);
    static int i = 0;
    void** table = in;
    Socket * sockets = (Socket*)table[i++];
    
    
//    void ** socketTable = (int**) in;
//    int port = 9000;
//    socketTable[i] = malloc(sizeof(int));
//    
//    *socketTable[i] = socket(AF_INET, SOCK_STREAM, 0);
//    struct sockaddr_in sockAddress;
//    
//    sockAddress.sin_addr.s_addr = INADDR_ANY;
//    sockAddress.sin_port = htonl(port++);
//    sockAddress.sin_family = AF_INET;
//    
//    int socketBind = bind(*socketTable[i], (const struct sockaddr *)&sockAddress, sizeof(sockAddress));
    
    
    int socketListen = listen(sockets->socketID, 5);
    
    
    
//    socklen_t addrlen = sizeof(sockAddress);
//    int socketAccept = accept(*socketTable[i++], (struct sockaddr*)&sockAddress, &addrlen);
    
    
//    int * socketfd = malloc(sizeof(int));
//    *socketfd = socket(AF_INET, SOCK_STREAM, 0);
//    struct sockaddr_in sockAddress;
//
//    sockAddress.sin_addr.s_addr = INADDR_ANY;
//    sockAddress.sin_port = htonl(*(int*)in);
//    sockAddress.sin_family = AF_INET;
//    
//    int socketBind = bind(*socketfd, &sockAddress, sizeof(sockAddress));
//    int socketList = listen(*socketfd, 5);
//    socklen_t addrlen = sizeof(sockAddress);
//    int socketAccept = accept(*socketfd, &sockAddress, &addrlen);
    
//    read(socketAccept, buff, SIZE_OF_CHARACTERS);
    
    return buff;

    
}




//
// #include<stdio.h>
// #include<stdlib.h>
// #include<string.h>    //strlen
// #include <sys/socket.h>
// #include<arpa/inet.h> //inet_addr
// #include<unistd.h>    //write
// #include<sys/types.h>
// #include<pthread.h>
// //#include "TServer.h" // EDIT: removed header reference
// #include "hashmap.h"
// 
// //populate map w/flight records
// void readInput(char * fileName, map_t flight_map);
// void reserve_Seats(map_t flight_map, char * flight, int toBuy );
// void return_Seats(map_t flight_map, char * flight, int noMore);
// int open_Seats(map_t flight_map, char * flight);
// void populate_Map(map_t flight_map, char * flight, int * seats );
// void readInput(char * fileName, map_t flight_map); //populate map w/flight records
// void error(char * message) {
// perror(message);
// exit(0);
// }
// 
// pthread_mutex_t mutex1;
// 
// 
// int main(int argc, char *argv[])
// {
// // if not enough arguments
// // if (argc < 6) {error("usage: Server ip_address start_port no_ports in_file out_file\n");}
// 
// 
// 
// int c, * ptr;
// struct sockaddr_in server,client;
// int family = AF_INET;
// int protocol = IPPROTO_TCP;
// int socktype = SOCK_STREAM;
// //int listen1,clientS, read_size;
// char client_message[2000];
// int optval;
// //gather info passed in the arguments
// // char * ip_address = argv[1];
// //int start_port = atoi(argv[2]);
// //int no_ports = atoi(argv[3]);
// // char * in_filename = argv[4];
// //char * out_filename = argv[5];
// char * in_filename = "data.txt";
// //create the hashmap to hold our data
// map_t flights = hashmap_new();
// //now need to fill the hash map .... with the input file
// readInput(in_filename, flights);
// 
// 
// //reserve_Seats(flights,"MIA-ORL",5);
// int t = open_Seats(flights,"MIA-ORL");
// printf("\n# of available seats on MIA-ORL is %i\n",t);
// return 0;
// 
// }
// 
// void reserve_Seats(map_t flight_map, char * flight, int toBuy )
// {
// int * seats = 0;
// 
// //int * temp = 0; // EDIT: made temporary value an int
// int temp = 0;
// 
// int check = 0;
// //pthread_mutex_lock(&mutex1);
// // retrieve seats from map
// check= hashmap_get(flight_map, flight, (void**) &seats); //seats is gonna hold result
// 
// if (check != MAP_OK)
// return; // "error: expecting flight"; // EDIT: cannot return string from void function
// 
// // temp = &(*seats); EDIT:
// temp = *seats; // EDIT: dereference pointer to get value
// 
// if(temp == 0 | temp < toBuy)
// {
// printf("Error: no more room on this flight");
// return; //cant reserver if not enough or none left
// }
// 
// int hold = temp;
// hold -= toBuy;
// temp = hold;
// *seats = hold;
// 
// printf("Successfully reserved %i seats for flight \"%s\".\n",toBuy,flight);
// 
// //  pthread_mutex_unlock(&mutex1);
// 
// 
// }
// 
// void return_Seats(map_t flight_map, char * flight, int noMore)
// {
// //int ** seats; //gonna hold value of available seats
// int * seats; // EDIT: made seaters a pointer to int
// 
// int check = 0;
// //pthread_mutex_lock(&mutex1);
// // retrieve seats from map
// check= hashmap_get(flight_map, flight, (void**) &seats); //seats is gonna hold result
// if (check != MAP_OK)
// {
// return; // "error: expecting flight"; // EDIT: cannot return string from void function
// }
// 
// //seats+= noMore; //add the return tickets to that value
// *seats += noMore; // EDIT: must dereference pointer before modifying value
// 
// //printf("Successfully returned %s seats for flight %s\n\n", *seats,flight);
// printf("Successfully returned %d seats for flight %s\n\n", *seats,flight); // EDIT: dereferenced seats
// 
// //pthread_mutex_unlock(&mutex1);
// 
// 
// }
// 
// int open_Seats(map_t flight_map, char * flight)
// {
// int *seats; //points to the pointer(that has what I need)
// 
// //int * temp;
// int check = 0;
// //pthread_mutex_lock(&mutex1);
// // retrieve seats from map
// check= hashmap_get(flight_map, flight, (void**) &seats); //seats is gonna hold result
// //  pthread_mutex_unlock(&mutex1);
// //  int *t; //
// // t= *seats;
// 
// if (check != MAP_OK)
// return 0; // "error: expecting flight"; // EDIT: cannot return string from void function
// 
// // temp = &(*seats);
// // int hold = temp + 1;
// //temp+=1;
// // temp = 3;
// 
// printf("checking: %d\n",*seats);
// 
// int x = *seats;
// 
// return x;
// }
// 
// 
// void populate_Map(map_t flight_map, char * flight, int * seats )
// {
// char * flight1 = malloc(sizeof(char) * strlen(flight)+ 1); //to add flexibilty
// // char * seat1 = malloc(sizeof(char) * strlen(seats)+ 1);
// 
// int * seats1 = malloc(sizeof(int)); // EDIT: assigned persistent memory for seats
// *seats1 = *seats; // EDIT: copy value into memory
// 
// strcpy(flight1,flight);
// // strcpy(seat1,seats);
// 
// //pthread_mutex_lock(&mutex1);
// hashmap_put(flight_map,flight1,seats1); //need to check if it worked, later.
// printf("Added flight \"%s\" with %i seats.\n",flight1, *seats1);
// // pthread_mutex_unlock(&mutex1);
// 
// 
// 
// }
// 
// 
// void readInput(char * fileName, map_t flight_map) //populate map w/flight records
// {
// FILE * file;
// if(!(file = fopen(fileName,"r+"))) //check if couldnt open file
// {
// //error("ERROR: Could not open \"%s\"",fileName);
// printf("ERROR: Could not open %s\n", fileName); // EDIT: added error break
// return; //we couldnt proceed
// }
// 
// size_t len = 0;
// size_t read; //ssize_t originally....
// //char *in;
// char *in = NULL; // EDIT: Initialized in to null
// 
// while((read = getline(&in, &len, file)) != -1)
// {
// //'in' has the line that was Read
// //char* tokens; //so we dont alter original string - will hold
// char flight[8]; //will always be XXX-XXX
// char seats[3];
// int * seats1 = NULL;
// int seats2 = 0;
// //int * seats1 = seats2;
// // int ** dseat = &seats1;
// sscanf(in, "%[^' '] %[^'\n']", flight, seats);
// 
// seats2 = atoi(seats);
// seats1 = &seats2; //points to add
// //*seats1 = seats2; //value is content of s2
// printf("Read %s %i\n", flight,*seats1);
// 
// populate_Map(flight_map,flight, seats1); //add flight/seats to map
// }
// 
// }
// 
// 
// 



