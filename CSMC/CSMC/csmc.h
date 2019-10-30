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
    struct student * next; // Pointer to the next student in the hall
};

struct tutor {
    int id;
    int status;
    struct student * student; // Student being tutored by this tutor
};

// First and last are based on the priorities of the students
struct waiting_hall {
    struct student * first; // First student in the hall
    struct student * last; // Last student in the hall
    int size; // Total students waiting
};

// Entry points for different threads.
void * start_tutoring (void *); // Start point for the tutor threads
void * get_tutor_help (void *); // Start point for the student threads
void * coordinate_tutoring (void *); // Start point for the coordinator thread
void do_programming (void);

// Utility functions for managing the hall waiting list.
void add_student(struct student *);
void insertBefore(struct student *, struct student *);
void remove_student(void);
void print_hall(void);

#endif /* csmc_h */
