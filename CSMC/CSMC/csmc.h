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
};

struct tutor {
    int id;
    int status;
    struct student * student;
};

// Start point for the tutor threads
void * start_tutoring (void *);
// Start point for the student threads
void * get_tutor_help (void *);
// Start point for the coordinator thread
void * coordinate_tutoring (void *);

#endif /* csmc_h */
