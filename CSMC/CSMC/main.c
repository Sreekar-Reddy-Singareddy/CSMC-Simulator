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
#include <unistd.h>
#include <errno.h>

#include "csmc.h"
#include "debug.h"

#define STUDENTS 7
#define TUTORS 3
#define CHAIRS 5
#define MAX_VISITS 5

struct student * queue [CHAIRS];
struct tutor * tutors [TUTORS];
pthread_t * t_threads;
pthread_t * s_threads;
pthread_t * c_thread;
pthread_mutex_t * s_locks[STUDENTS];
int empty_chairs = CHAIRS;

int main(int argc, const char * argv[]) {
    int i;
    void *value;
    
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
        t_threads = malloc(sizeof(pthread_t)*TUTORS);
        // Create a tutor for every thread
        struct tutor * t = malloc(sizeof(struct tutor));
        t->id = i;
        t->status = 0;
        t->student = NULL;
        assert(pthread_create(&t_threads[i], NULL, start_tutoring, t) == 0);
    }
    
    // Create threads for students
    // Also initialise each thread such that the
    // execution starts from the get_tutor_help function
    for (i=0; i<STUDENTS; i++) {
        pthread_mutex_init(&s_locks[i], NULL);
        s_threads = malloc(sizeof(pthread_t)*STUDENTS);
        // Create a student for this thread
        struct student * s = malloc(sizeof(struct student));
        s->id = i;
        s->status = 0;
        s->visits = 0;
        assert(pthread_create(&s_threads[i], NULL, get_tutor_help, s) == 0);
    }
    
    // Create thread for coordinator
    c_thread = malloc(sizeof(pthread_t));
    assert(pthread_create(c_thread, NULL, coordinate_tutoring, NULL) == 0);
    
    // Wait for all these threads to finish and join
    for (i=0; i<TUTORS; i++){
        int code = pthread_join(t_threads[i], &value);
        SPAM(("Completed T thread(%d) %d\n",i, code));
    }
    for (i=0; i<STUDENTS; i++){
        int code = pthread_join(s_threads[i], &value);
        SPAM(("Completed S thread(%d) %d\n",i, code));
    }
    pthread_join(*c_thread, &value);
    
    return 0;
}

// Starting point for the exeution of tutor threads
void * start_tutoring (void * arg) {
    struct tutor * t = (struct tutor *) arg;
    SPAM(("T Thread: ID = %d\n",t->id));
    return NULL;
}

// Starting point for the execution of student threads
void * get_tutor_help (void * student) {
    struct student * s = (struct student *) student;
    SPAM(("S Thread: Student ID = %d\n", s->id));
    
    // Come back to get help as long as vists are remaining
    while (s->visits < MAX_VISITS) {
        // CRITICAL SECTION - START
        pthread_mutex_lock(&s_locks[s->id]);
        if (empty_chairs <= 0) {
            // Go back to programming and return later...
            SPAM(("No Chairs\n"));
            do_programming();
            continue;
        }
        else {
            SPAM(("Student %d Got a Chair\n", s->id));
            // Get a chair and sit
            empty_chairs -= 1; // Sit in chair
            usleep(20); // Get tutoring for 200ms
            empty_chairs += 1; // Get up from chair
            s->visits++; // Increase visits
            do_programming(); // Get back to work
        }
        pthread_mutex_unlock(&s_locks[s->id]);
        // CRITICAL SECTION - END
    }
    
    // Got maximum help. Terminate the thread now!
    return NULL;
}

// Do the programming for 2ms and come back
void do_programming (void) {
//    SPAM(("Programming...\n"));
    usleep(2);
}

// Starting point for the execution of coordintor thread
void * coordinate_tutoring() { // Why not error without any args?
    return NULL;
}
