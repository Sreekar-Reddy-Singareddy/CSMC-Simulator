//
//  main.c
//  CSMC
//
//  Created by Sreekar on 24/10/19.
//  Copyright © 2019 Sreekar. All rights reserved.
//

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include "csmc.h"

#define STUDENTS 7
#define TUTORS 3
#define CHAIRS 4

struct student * queue [CHAIRS];
struct tutor * tutors [TUTORS];
pthread_t * t_threads [TUTORS];
pthread_t * s_threads [STUDENTS];
pthread_t * c_thread;

int main(int argc, const char * argv[]) {
    int i;
    
    // Initialising the queue for students
    for (i=0; i<CHAIRS; i++) {
        queue[i] = malloc(sizeof(struct student));
    }
    
    // Initialising the list for tutors
    for (i=0; i<TUTORS; i++) {
        tutors[i] = malloc(sizeof(struct tutor));
    }
    
    // Create threads for tutors
    // Also initialise each thread such that the
    // execution starts from the start_tutoring function
    for (i=0; i<TUTORS; i++) {
        t_threads[i] = malloc(sizeof(pthread_t));
        // Create a tutor for every thread
        struct tutor * t = malloc(sizeof(struct tutor));
        t->id = i;
        t->status = 0;
        t->student = NULL;
        assert(pthread_create(t_threads[i], NULL, start_tutoring, t) == 0);
    }
    
    // Create threads for students
    // Also initialise each thread such that the
    // execution starts from the get_tutor_help function
    for (i=0; i<STUDENTS; i++) {
        s_threads[i] = malloc(sizeof(pthread_t));
        // Create a student for this thread
        struct student * s = malloc(sizeof(struct student));
        s->id = i;
        s->status = 0;
        s->visits = 0;
        assert(pthread_create(s_threads[i], NULL, get_tutor_help, s) == 0);
    }
    
    // Create thread for coordinator
    c_thread = malloc(sizeof(pthread_t));
    assert(pthread_create(c_thread, NULL, coordinate_tutoring, NULL) == 0);
    
    return 0;
}

// Starting point for the exeution of tutor threads
void * start_tutoring (void * arg) {
    struct tutor * t = (struct tutor *) arg;
    printf("T Thread: ID = %d\n",t->id);
    return NULL;
}

// Starting point for the execution of student threads
void * get_tutor_help (void * student) {
    struct student * s = (struct student *) student;
    printf("S Thread: Student ID = %d\n", s->id);
    return NULL;
}

// Starting point for the execution of coordintor thread
void * coordinate_tutoring() { // Why not error without any args?
    printf("C Thread\n");
    return NULL;
}
