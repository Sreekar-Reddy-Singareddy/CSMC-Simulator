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
int empty_chairs = CHAIRS;
struct student * active;
pthread_mutex_t * waiting_hall_lock;
pthread_mutex_t * tutors_list_lock;
pthread_mutex_t * empty_chairs_lock;

sem_t * stud, * coor, * done_tutoring, * tut_sems[TUTORS];

// TEST_MODE
struct waiting_hall * hall;
void test_queue(void);

int main(int argc, const char * argv[]) {
    if (TEST_MODE) {
        test_queue();
        return 0;
    }
    
    // All memory allocations
    stud = malloc(sizeof(sem_t));
    coor = malloc(sizeof(sem_t));
    done_tutoring = malloc(sizeof(sem_t));
    waiting_hall_lock = malloc(sizeof(pthread_mutex_t));
    tutors_list_lock = malloc(sizeof(pthread_mutex_t));
    empty_chairs_lock = malloc(sizeof(pthread_mutex_t));
    
    int i;
    void *value;
    sem_init(stud, 0, 1);
    sem_init(coor, 0, 0);
    sem_init(done_tutoring, 0, 0);
    pthread_mutex_init(&waiting_hall_lock, NULL);
    pthread_mutex_init(&tutors_list_lock, NULL);
    pthread_mutex_init(&empty_chairs_lock, NULL);
    
    
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
    t_threads = malloc(sizeof(pthread_t)*TUTORS);
    for (i=0; i<TUTORS; i++) {
        // Create a tutor for every thread
        tut_sems[i] = malloc(sizeof(sem_t));
        sem_init(tut_sems[i], 0, 0);
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
    s_threads = malloc(sizeof(pthread_t)*STUDENTS);
    for (i=0; i<STUDENTS; i++) {
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
    /*
     0. Open the cabin and start waiting. Before waiting, update the status: tutor->status = 0; This means tutor is idle. **LOCK NEEDED ON TUTOR LIST**
     1. Wait for coordinator to let you know about students: sem_wait(tut_sems[id]); This makes tut_sems[id] = -1 until coor signals.
     2. Keep waiting until coordinator signals tut_sems[id] to continue: spinning
     3. After signalled (makes tut_sems[id] = 0), update the status: tutot->status = 1; This means tutor is busy. **LOCK NEEDED ON TUTOR LIST**
     4. Get the 'first' student from the waiting hall: remove_student() -> student; This is the student you need to tutor. **LOCK NEEDED ON WAITING_HALL**
     5. Sleep for 2 sec (tutoring time) and then make 'student'->'tutor' as NULL: student->tutor = NULL; Because student is done getting help.
     6. Done for now. Continue waiting for coordinator signal: sem_wait(tut_sems[id]); Makes tut_sems[id] = -1 again.
     */
    return NULL;
}

// Starting point for the execution of student threads
void * get_tutor_help (void * student) {
    struct student * s = (struct student *) student;
    /*
     1. Student enters the CSMC as 's'.
     2. Gets a lock on empty chairs as they entered first: lock(empty_chairs)
     3. Checks for empty chairs > 0. If not, just release the lock and leave CSMC: unlock(empty_chairs); do_programming(); continue;
     4. If available, reduce the chairs by 1: empty_chairs--; Because a chair is now filled.
     5. Release the lock on empty_chairs now: unlock(empty_chairs)
     6. Wait on 'stud' semaphore until the 'new' student is added to the waiting hall: sem_wait(stud); this makes next students wait outside CSMC.
     7. Assign 's' to 'new' (global): new = s; this updates the currently interacting student with coor.
     8. Signal the 'coor' that 'new' has arrived: sem_post(coor); this make coor = 1.
     9. Wait until tutoring is done: sem_wait(done_tutoring); Makes the stud wait here until signal sent from tutor.
     10. Do programming. Come back to CSMC again later: do_programmming(); continue to while loop; This starts the process again from step 1.
     */
    while (s->visits < MAX_VISITS) {
        SPAM(("Hey, I (S%d) need some tutor help!", s->id));
        pthread_mutex_lock(&empty_chairs_lock);
        if (empty_chairs <= 0) {
            pthread_mutex_unlock(&empty_chairs_lock);
            do_programming();
            continue;
        }
        else {
            empty_chairs--;
            pthread_mutex_unlock(&empty_chairs_lock);
            sem_wait(stud);
            active = s;
            sem_post(coor);
//            sem_wait(done_tutoring);
            do_programming();
        }
    }
    
    return NULL;
}

// Starting point for the execution of coordintor thread
void * coordinate_tutoring() {
    /*
     1. Open CSMC and wait for students to come: sem_wait(coor); This makes coor = -1 until some student signals.
     2. Keep waiting until some student signals coor to continue: spinning
     3. Get the student who has signalled coor: use some global variable(new)
     4. Add this student to the waiting hall: add_student(new); This adds the student to the hall. **LOCK NEEDED ON WAITING_HALL**
     5. Signal that next student can come and wait now: sem_post(stud); this lets the next student come inside CSMC.
     6. Check the tutors list and see if any tutor is idle: get_idle_tutor() -> tutor (or NULL) **LOCK NEEDED ON TUTOR LIST**
     7. If 'tutor' is null (everyone busy), then continue and wait for next student: loop -> sem_wait(coor)
     8. Else, get the 'id' of the tutor and signal the corresponding semaphore: sem_post(tut_sems[id])
     9. This completes the job of coor for now. He must again repeat the cycle from step 1 through 7: continue while loop
     */
    int total = MAX_VISITS*STUDENTS;
    SPAM(("CSMC Opened.\n"));
    
    while (total > 0) {
        sem_wait(coor);
        active->visits++;
        pthread_mutex_lock(&waiting_hall_lock);
        add_student(active);
        pthread_mutex_unlock(&waiting_hall_lock);
        sem_post(stud);
        // Below is actually done by the tutor
        pthread_mutex_lock(&waiting_hall_lock);
        struct student * s = remove_student();
        pthread_mutex_unlock(&waiting_hall_lock);
        sleep(2); // Tutoring simulation
//        sem_post(done_tutoring);
        SPAM(("Student %d done. He has %d visits left.\n", s->id, MAX_VISITS-s->visits));
        total--;
    }
    
    return NULL;
}

// Do the programming for 2ms and come back
void do_programming (void) {
    //    SPAM(("Programming...\n"));
    usleep(2);
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
    printf("Removing student\n");
    if (hall->first == NULL) { // Empty hall case
        printf("No one in the hall yet!\n");
        return NULL;
    }
    struct student * s = hall->first;
    hall->first = hall->first->next;
    if (hall->first == NULL) { // Only 1 student was there and removed
        hall->last = NULL;
    }
    hall->size--;
    return s;
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
