//
//  waiting_hall.h
//  CSMC
//
//  Created by Sreekar on 30/10/19.
//  Copyright © 2019 Sreekar. All rights reserved.
//
#include "csmc.h"

#ifndef waiting_hall_h
#define waiting_hall_h

// First and last are based on the priorities of the students
struct waiting_hall {
    struct student * first; // First student in the hall
    struct student * last; // Last student in the hall
    int size; // Total students waiting
};

// Utility functions for managing the hall waiting list.
void add_student(struct student *);
void insertBefore(struct student *, struct student *);
void remove_student(void);
void print_hall(void);

#endif /* waiting_hall_h */
