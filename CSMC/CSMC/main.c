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

#include "waiting_hall.h"
#include "csmc.h"
#include "debug.h"

#define TEST_MODE 0

struct tutor * tutors [2];
pthread_t * t_threads;
pthread_t * s_threads;
pthread_t * c_thread;
int empty_chairs;
struct student * active;
pthread_mutex_t * active_stud_lock;
pthread_mutex_t * waiting_hall_lock;
pthread_mutex_t * tutors_list_lock;
pthread_mutex_t * empty_chairs_lock;
int is_csmc_open = 1;

int STUDENTS, TUTORS, CHAIRS, MAX_VISITS;


sem_t * stud, * coor, * done_tutoring, * tut_sems[2], * stu_sems[8];
struct timespec timer;

// TEST_MODE
struct waiting_hall * hall;
void test_queue(void);

int main(int argc, const char * argv[]) {
    if (TEST_MODE) {
        int i;
        
        return 0;
    }
    
    //    freopen("output.txt", "w", stdout);
    
    // All memory allocations
    STUDENTS = atoi(argv[1]);
    TUTORS = atoi(argv[2]);
    CHAIRS = atoi(argv[3]);
    MAX_VISITS = atoi(argv[4]);
    empty_chairs = CHAIRS;
    time_t t = 1;
    timer.tv_sec = t;
    timer.tv_nsec = 0;
    stud = malloc(sizeof(sem_t));
    coor = malloc(sizeof(sem_t));
    active = malloc(sizeof(struct student));
    done_tutoring = malloc(sizeof(sem_t));
    active_stud_lock = malloc(sizeof(pthread_mutex_t));
    waiting_hall_lock = malloc(sizeof(pthread_mutex_t));
    tutors_list_lock = malloc(sizeof(pthread_mutex_t));
    empty_chairs_lock = malloc(sizeof(pthread_mutex_t));
    hall = malloc(sizeof(struct waiting_hall));
    
    int i;
    sem_init(stud, 0, 1);
    sem_init(coor, 0, 0);
    sem_init(done_tutoring, 0, 0);
    pthread_mutex_init(active_stud_lock, NULL);
    pthread_mutex_init(waiting_hall_lock, NULL);
    pthread_mutex_init(tutors_list_lock, NULL);
    pthread_mutex_init(empty_chairs_lock, NULL);
    hall->first = NULL;
    hall->last = NULL;
    hall->size = 0;
    
    // Initialising the list for tutors
    for (i=0; i<TUTORS; i++) {
        tutors[i] = malloc(sizeof(struct tutor));
        tutors[i]->id = i;
        tutors[i]->status = 0;
        tutors[i]->number_tutored = 0;
        tutors[i]->student = NULL;
    }
    
    // Create thread for coordinator
    c_thread = malloc(sizeof(pthread_t));
    assert(pthread_create(c_thread, NULL, coordinate_tutoring, NULL) == 0);
    
    // Create threads for tutors
    // Also initialise each thread such that the
    // execution starts from the start_tutoring function
    t_threads = malloc(sizeof(pthread_t)*TUTORS);
    for (i=0; i<TUTORS; i++) {
        // Semaphore for tutor to wait for coordinator to signal about a waiting student.
        tut_sems[i] = malloc(sizeof(sem_t));
        sem_init(tut_sems[i], 0, 0);
        // Pass the tutor for every thread
        assert(pthread_create(&t_threads[i], NULL, start_tutoring, tutors[i]) == 0);
    }
    
    // Create threads for students
    // Also initialise each thread such that the
    // execution starts from the get_tutor_help function
    s_threads = malloc(sizeof(pthread_t)*STUDENTS);
    for (i=0; i<STUDENTS; i++) {
        // Semaphore for student to wait until tutoring is done. Tutor tells when it is done.
        stu_sems[i] = malloc(sizeof(sem_t));
        sem_init(stu_sems[i], 0, 0);
        // Create a student for this thread
        struct student * s = malloc(sizeof(struct student));
        s->id = i;
        s->status = 0;
        s->visits = 0;
        s->tutor_id = -1;
        assert(pthread_create(&s_threads[i], NULL, get_tutor_help, s) == 0);
    }
    
    // Wait for all student threads to finish and join
    for (i=0; i<STUDENTS; i++){
        int code = pthread_join(s_threads[i], NULL);
        SPAM(("Completed S thread(%d) %d\n",i, code));
    }
    
    // Wait for all tutor threads to finish and join
    for (i=0; i<TUTORS; i++){
        int code = pthread_join(t_threads[i], NULL);
        SPAM(("Completed T thread(%d) %d\n",i, code));
    }
    
    // Wait for coordinator thread to finish
    pthread_join(*c_thread, NULL);
    
    SPAM(("Completed C thread.\n"));
    
    return 0;
}

// Starting point for the exeution of tutor threads
void * start_tutoring (void * arg) {
    struct tutor * t = (struct tutor *) arg;
    while (is_csmc_open) {
        // Going to idle state here
        pthread_mutex_lock(tutors_list_lock);
        t->status = 0;
        pthread_mutex_unlock(tutors_list_lock);
        Debug(("(T%d) Waiting for coordinator. Address: %p \n", t->id, t));
        sem_wait(tut_sems[t->id]);
        
        // Going to busy state here
        pthread_mutex_lock(tutors_list_lock);
        t->status = 1;
        Debug(("I (T%d) Got work. Address: %p\n", t->id, t));
        pthread_mutex_unlock(tutors_list_lock);
        
        // Getting the student with the highest priority.
        print_hall();
        Debug(("I (T%d) am going to remove the first student in hall.\n", t->id));
        pthread_mutex_lock(waiting_hall_lock);
        struct student * s = remove_student();
        Debug(("Removed Student Address: %p\n", s));
        pthread_mutex_unlock(waiting_hall_lock);
        if (s == NULL) continue;
        print_hall();
        Debug(("Removed Student by T%d is %p (S%d).\n", t->id, s, s->id));
        t->student = s;
        t->number_tutored++; // Increase the number of students tutored.
        Debug(("I (T%d) is tutoring student (S%d). Visits (%d)\n", t->id, s->id, s->visits));
        
        // The student has now moved from the hall to tutor cabin.
        pthread_mutex_lock(empty_chairs_lock); // IS THIS LOCK NEEDED?
        empty_chairs++;
        pthread_mutex_unlock(empty_chairs_lock);
        
        // Simulation of actual tutoring
        usleep(2000);
        printf("Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = ??.\n",
               t->student->id, t->id, t->number_tutored);
        SPAM(("I (T%d) am done tutoring student (S%d).\n", t->id, s->id));
        
        // Done tutoring. Let the student know the same.
        t->student->tutor_id = t->id;
        int id = t->student->id;
        t->student = NULL;
        sem_post(stu_sems[id]);
    }
    return NULL;
}

// Starting point for the execution of student threads
void * get_tutor_help (void * student) {
    struct student * s = (struct student *) student;
    while (s->visits < MAX_VISITS || is_csmc_open) {
        Debug(("Hey, I (S%d) need some tutor help!\n", s->id));
        
        // Entered the CSMC for help.
        pthread_mutex_lock(empty_chairs_lock);
        if (empty_chairs <= 0) {
            // No chairs. Come back later.
            pthread_mutex_unlock(empty_chairs_lock);
	    sem_wait(stud);
	    sem_post(coor);
            SPAM(("Oops! No chairs. I (S%d) am going back...\n", s->id));
            printf("Student %d found no empty chair. Will try again later.\n", s->id);
            do_programming();
            continue;
        }
        else {
            // Found a chair and sat. Now notify the coordinator. Other students wait.
            empty_chairs--;
            pthread_mutex_unlock(empty_chairs_lock);
            s->visits++;
            sem_wait(stud);
            active = s;
            sem_post(coor);
            printf("Student %d takes a seat. Empty Chairs = %d.\n", s->id, empty_chairs);
            
            // Now you told coor. Wait until you are called and completed getting the help.
            sem_wait(stu_sems[s->id]);
            printf("Student %d received help from Tutor %d.\n", s->id, s->tutor_id);
            
            // You are done with getting help. You can leave now.
            do_programming();
        }
    }
    Debug(("I (S%d) came here MAX number of times.\n",s->id));
    
    return NULL;
}

// Starting point for the execution of coordintor thread
void * coordinate_tutoring(void * arg) {
    int total = MAX_VISITS*STUDENTS;
    SPAM(("CSMC Opened.\n"));
    
    while (total > 0) {
        // Wait for a student to come in
        sem_wait(coor);
        
        // Someone has come. Add them to the waiting hall queue
        int hall_size, student_added = 0;
        pthread_mutex_lock(waiting_hall_lock);
        if (active != NULL && active->visits <= MAX_VISITS) {
            add_student(active);
            student_added = 1;
            hall_size = hall->size;
            printf("Student %d with priority %d in the queue. Waiting students now = %d. Total requests = %d.\n",
                   active->id, MAX_VISITS-active->visits, hall_size, MAX_VISITS*STUDENTS-total+1);
	    active = NULL;
        }
	else {
	  student_added = 0;
	}
        pthread_mutex_unlock(waiting_hall_lock);
        
        
        // Check for an idle tutor.
        // Notify the tutor only if there is someone actually waiting.
        // Else there is no point in notifying.
        struct tutor * idle_tutor = get_idle_tutor();
        if (idle_tutor != NULL && hall_size > 0) {
            Debug(("Idle tutor found - T%d.\n", idle_tutor->id));
            // Someone is there! Then do the tutoring...
            // Since this tutor will take away one student,
            // I will assume he has been served.
            sem_post(tut_sems[idle_tutor->id]);
            total--;
        }
        else {
            Debug(("All tutors are busy right now. Please have a seat.\n"));
        }
        sem_post(stud);
    }
    
    // Close the CSMC once all students are served.
    is_csmc_open = 0;
    SPAM(("CSMC Closing time. Everyone leave...\n"));
    
    // Notify all the waiting tutors, if any, to STOP!
    notify_all();
    
    return NULL;
}

// Do the programming for 2ms and come back
void do_programming (void) {
    SPAM(("Programming...\n"));
    usleep(200);
}

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
struct student * remove_student() {
    //  SPAM(("\nRemoving student\n"));
    if (hall->first == NULL) { // Empty hall case
        SPAM(("No one in the hall yet!\n"));
        return NULL;
    }
    struct student * s = hall->first; // 's' is gonna be removed
    hall->first = hall->first->next;
    if (hall->first == NULL) { // Only 1 student was there and removed
        hall->last = NULL;
    }
    s->next = NULL;
    SPAM(("Removing Student's Next: %p\n", s->next));
    hall->size--;
    return s;
}

// Prints the students waiting in hall currently.
void print_hall() {
    //    SPAM(("Printing... %s\n", hall->first));
    struct student * node = hall->first;
    while (node != NULL) {
        Debug(("S: %d (V: %d) ---> ", node->id, node->visits));
        node = node->next;
    }
    Debug(("\n"));
}

// Adds the new student into the hall, if not full.

void add_student(struct student * new) {
    new->next = NULL;
    if (hall->size == CHAIRS) { // CASE 4
        SPAM(("C4: Cannot add. Already full.\n"));
        return;
    }
    hall->size++;
    if(hall->first == NULL) { // CASE 1
        SPAM(("C1\n"));
        hall->first = new;
        hall->last = new;
    }
    else if (hall->first->next == NULL) { // CASE 2
        SPAM(("C2\n"));
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
        SPAM(("C3\n"));
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

// Checks the tutor list and returns the first idle tutor.
// If no one is idle, returns NULL.
struct tutor * get_idle_tutor () {
    int i;
    pthread_mutex_lock(tutors_list_lock);
    for (i=0; i<TUTORS; i++){
        SPAM(("****** Checking tutor T%d, Status: %d Address: %p ******\n", tutors[i]->id, tutors[i]->status, tutors[i]));
    }
    for (i=0; i<TUTORS; i++) {
        if (tutors[i]->status == 0) {
            // This is the first idle tutor in the list.
            struct tutor * idle_tutor = tutors[i];
            pthread_mutex_unlock(tutors_list_lock);
            return idle_tutor;
        }
    }
    pthread_mutex_unlock(tutors_list_lock);
    return NULL;
}

// Notifies all the endlessly waiting tutors to STOP
void notify_all () {
    int i;
    for (i=0; i<TUTORS; i++){
        sem_post(tut_sems[i]);
    }
    for (i=0; i<STUDENTS; i++){
      sem_post(stu_sems[i]);
    }
}

void test_queue() {
    hall = malloc(sizeof(struct waiting_hall));
    hall->first = NULL;
    hall->last = NULL;
    hall->size = 0;
    if (hall->first == NULL) {
        SPAM(("No one in the hall yet!\n"));
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
