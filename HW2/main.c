//
//  main.c
//  HW2
//
//  Created by Roicxy Alonso Gonzalez on 7/14/17.
//  Copyright © 2017 AlonsoRoicxy. All rights reserved.
//

#include <stdio.h>

void * f(void * num);

int main(int argc, const char * argv[]) {
    
    pthread_t nId;
    pthread_t tid1;
    pthread_t tid2;
    
    int num = 10;
    
    void * newNum = &num;
    
    pthread_create(&nId, NULL, f, newNum);
    
    pthread_create(&tid1, NULL, f, newNum);
    pthread_create(&tid2, NULL, f, newNum);
    
    
    pthread_join(nId, &newNum);
    
    pthread_join(tid1, &newNum);
    pthread_join(tid2, &newNum);
    
    num = *(int*) newNum;
    
    printf("%d\n",num);
    
    
    
    
    
    
    
    
    
    
    
    return 0;
}
void * f(void * num)
{
    int * out = malloc(sizeof(int));
    void * s;
    
    static int t = 0;
    
    t += *(int *) num;
    
    *out = 0;
    for(int i = 0; i < t; i++)
        *out+= (2 * 23) ;
    s = &out;
    
    
    return (void*) out;
}
