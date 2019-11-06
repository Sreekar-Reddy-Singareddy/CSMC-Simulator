//
//  csmc.h
//  CSMC
//
//  Created by Sreekar on 24/10/19.
//  Copyright © 2019 Sreekar. All rights reserved.
//

#ifndef csmc_h
#define csmc_h

struct student {
    int id;
    int status;
    int visits;
    int tutor_id;
    struct student * next; // Pointer to the next student in the hall
};

struct tutor {
    int id;
    int status;
  int number_tutored;
    struct student * student; // Student being tutored by this tutor
};

// Entry points for different threads.
void * start_tutoring (void *); // Start point for the tutor threads
void * get_tutor_help (void *); // Start point for the student threads
void * coordinate_tutoring (void *); // Start point for the coordinator thread
void do_programming (void);
struct tutor * get_idle_tutor(void); // Searches and get an idle tutor or returns NULL
int get_current_tutoring_students (void);
void notify_all(); // Notify all the waiting threads to continue

#endif /* csmc_h */
