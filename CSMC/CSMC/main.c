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
#include <semaphore.h>

#include "csmc.h"
#include "debug.h"

#define STUDENTS 7
#define TUTORS 3
#define CHAIRS 5
#define MAX_VISITS 5
#define TEST_MODE 1

struct student * queue [CHAIRS];
struct tutor * tutors [TUTORS];
pthread_t * t_threads;
pthread_t * s_threads;
pthread_t * c_thread;
pthread_mutex_t * s_locks[STUDENTS];
int empty_chairs = CHAIRS;

sem_t * stud;
sem_t * coor;

// TEST_MODE
struct waiting_hall * hall;

// Inserts a new student (new) in the hall, after a given student (prev).
void insertBefore(struct student * prev, struct student * new) {
    if (prev == NULL) { // Inserting at head
        new->next = hall->first;
        hall->first = new;
    }
    else { // Somewhere in the middle
        new->next = prev->next;
        prev->next = new;
    }
}

// Removes the first student in the hall (hall->first), if any.
void remove_student() {
    printf("Removing student\n");
    if (hall->first == NULL) { // Empty hall case
        printf("No one in the hall yet!\n");
        return;
    }
    hall->first = hall->first->next;
    if (hall->first == NULL) { // Only 1 student was there and removed
        hall->last = NULL;
    }
    hall->size--;
}

// Prints the students waiting in hall currently.
void print_hall() {
    struct student * node = hall->first;
    while (node != NULL) {
        printf("Student: %d Visits: %d\n", node->id, node->visits);
        node = node->next;
    }
}

// Adds the new student into the hall, if not full.
void add_student(struct student * new) {
    if (hall->size == CHAIRS) { // CASE 4
        printf("C4: Cannot add. Already full.\n");
        return;
    }
    hall->size++;
    if(hall->first == NULL) { // CASE 1
        printf("C1\n");
        hall->first = hall->last = new;
    }
    else if (hall->first->next == NULL) { // CASE 2
        printf("C2\n");
        if (new->visits < hall->first->visits) { // Insert at head
            new->next = hall->first;
            hall->first = new;
            hall->last = new->next;
        }
        else { // Insert at tail
            hall->first->next = new;
            hall->last = new;
        }
    }
    else if (hall->first->next != NULL) { // CASE 3
        printf("C3\n");
        struct student * node = hall->first, * prev = NULL;
        while (node != NULL) {
            if (new->visits < node->visits) {
                insertBefore(prev, new);
                return;
            }
            else {
                prev = node;
                node = node->next;
            }
        }
        // Not added anywhere in the middle of the list,
        // Hence add it to the tail.
        hall->last->next = new;
        hall->last = new;
    }
}

void test_queue() {
    hall = malloc(sizeof(struct waiting_hall));
    hall->first = NULL;
    hall->last = NULL;
    hall->size = 0;
    if (hall->first == NULL) {
        printf("No one in the hall yet!\n");
    }
    
    // Simulate Case 1 - Empty hall
    struct student * s1 = malloc(sizeof(struct student));
    s1->id=1; s1->status=0; s1->visits=0; s1->next = NULL;
    add_student(s1); print_hall();
    
    // Simulate Case 2 - Only 1 student
    struct student * s2 = malloc(sizeof(struct student));
    s2->id=2; s2->status=0; s2->visits=1; s2->next = NULL;
    add_student(s2); print_hall();
    
    // Simulate Case 3 - More than 1 students, but not full
    struct student * s3 = malloc(sizeof(struct student));
    s3->id=3; s3->status=0; s3->visits=3; s3->next = NULL;
    add_student(s3); print_hall();
    struct student * s4 = malloc(sizeof(struct student));
    s4->id=4; s4->status=0; s4->visits=2; s4->next = NULL;
    add_student(s4); print_hall();
    struct student * s5 = malloc(sizeof(struct student));
    s5->id=5; s5->status=0; s5->visits=1; s5->next = NULL;
    add_student(s5); print_hall();
    remove_student(); print_hall();
    struct student * s6 = malloc(sizeof(struct student));
    s6->id=6; s6->status=0; s6->visits=0; s6->next = NULL;
    add_student(s6); print_hall();
}

int main(int argc, const char * argv[]) {
    if (TEST_MODE) {
        test_queue();
        return 0;
    }
    
    int i;
    void *value;
    stud = sem_init(stud, 0, 1);
    stud = sem_init(coor, 0, 1);
    
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
    
    // Wait for all these threads to finish and join
    for (i=0; i<TUTORS; i++){
        int code = pthread_join(t_threads[i], &value);
        SPAM(("Completed T thread(%d) %d\n",i, code));
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
    
    // Wait for all these threads to finish and join
    for (i=0; i<STUDENTS; i++){
        int code = pthread_join(s_threads[i], &value);
        SPAM(("Completed S thread(%d) %d\n",i, code));
    }
    
    // Create thread for coordinator
    c_thread = malloc(sizeof(pthread_t));
    assert(pthread_create(c_thread, NULL, coordinate_tutoring, NULL) == 0);
    
    // Wait for all these threads to finish and join
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
//    SPAM(("S Thread: Student ID = %d\n", s->id));
    
    // Come back to get help as long as vists are remaining
    while (s->visits < MAX_VISITS) {
        // CRITICAL SECTION - START
        pthread_mutex_lock(&s_locks[s->id]);
        sem_wait(coor);
        if (empty_chairs <= 0) {
            // Go back to programming and return later...
            SPAM(("No Chairs\n"));
            do_programming();
            continue;
        }
        else {
            SPAM(("Student %d Got a Chair\n", s->id));
            // Notify the coordinator using semaphore
            sleep(1);
            sem_post(stud);
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
    while (1) {
        sem_wait(stud);
        SPAM(("Some student has come\n"));
        sleep(0.5);
        sem_post(coor);
    }
    return NULL;
}
