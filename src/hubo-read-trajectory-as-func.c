/*
Copyright (c) 2012, Daniel M. Lofaro
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its contributors may 
      be used to endorse or promote products derived from this software 
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>
#include <stdio.h>

// for timer
#include <time.h>
#include <sched.h>
#include <sys/io.h>
#include <unistd.h>

// for RT
#include <stdlib.h>
#include <sys/mman.h>

// for hubo
#include "hubo.h"

// for ach
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <inttypes.h>
#include "ach.h"

// for keyboard

#include <termio.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>




//#include "../include/hubo_ref_filter.h"
#include "hubo-ref-filter.h"


/* At time of writing, these constants are not defined in the headers */
#ifndef PF_CAN
#define PF_CAN 29
#endif

#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif

/* ... */

/* Somewhere in your app */

// Priority
#define MY_PRIORITY (49)/* we use 49 as the PRREMPT_RT use 50
                            as the priority of kernel tasklets
                            and interrupt handler by default */

#define MAX_SAFE_STACK (1024*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */


// Timing info
#define NSEC_PER_SEC    1000000000

char* fileName = "";
//int interval = 1000000000; // 1hz (1.0 sec)
//int interval = 500000000; // 2hz (0.5 sec)
int interval =   40000000; // 25 hz (0.04 sec)
//int interval = 20000000; // 50 hz (0.02 sec)
//int interval = 10000000; // 100 hz (0.01 sec)
//int interval = 5000000; // 200 hz (0.005 sec)
//int interval = 2000000; // 500 hz (0.002 sec)

struct timeb {
        time_t   time;
        unsigned short millitm;
        short    timezone;
        short    dstflag;
};



/* functions */
// for keyboard
static void tty_atexit(void);
static int tty_reset(int);
static void tweak_init();

static struct termios save_termios;
static int ttysavefd = -1;


void stack_prefault(void);
static inline void tsnorm(struct timespec *ts);
void getMotorPosFrame(int motor, struct can_frame *frame);
int huboLoop(int mode, bool compliance_mode, bool pause_feature);
int ftime(struct timeb *tp);
int getArg(char* s,struct hubo_ref *r);
int runTraj(char* s, int mode,  struct hubo_ref *r, struct timespec *t, struct hubo_state* H_state, bool compliance_mode, bool pause_feature);
int runTrajFunction(char* s, int mode,  bool compliance_mode, bool pause_feature);
// ach message type
//typedef struct hubo h[1];

// ach channels
ach_channel_t chan_hubo_ref;      // hubo-ach
ach_channel_t chan_hubo_ref_filter;      // hubo-ach-filter
ach_channel_t chan_hubo_init_cmd; // hubo-ach-console
ach_channel_t chan_hubo_state;    // hubo-ach-state
ach_channel_t chan_hubo_param;    // hubo-ach-param
ach_channel_t chan_hubo_from_sim; // ach channel for sim

int goto_init_flag = 0;

int debug = 0;
int hubo_debug = 1;
int i = 0;
int huboLoop(int mode, bool compliance_mode, bool pause_feature) {
	double newRef[2] = {1.0, 0.0};
        // get initial values for hubo
        struct hubo_ref H_ref;
	memset( &H_ref,   0, sizeof(H_ref));
        hubo_virtual_t H_virtual;
        memset( &H_virtual, 0, sizeof(H_virtual));

        size_t fs;
	int r = ach_get( &chan_hubo_ref, &H_ref, sizeof(H_ref), &fs, NULL, ACH_O_COPY );
	if(ACH_OK != r) {
		if(hubo_debug) {
                       	printf("Ref ini r = %s\n",ach_result_to_string(r));}
		}
	else{   assert( sizeof(H_ref) == fs ); }

	struct hubo_state H_state;
	memset( &H_state, 0, sizeof(H_state) );
	
	r = ach_get( &chan_hubo_state, &H_state, sizeof(H_state), &fs, NULL, ACH_O_COPY );
	if (ACH_OK != r) {
	  if (hubo_debug) { 
	    printf("State ini r = %s\n", ach_result_to_string(r)); 
	  }
	} else {
	  assert( sizeof(H_state) == fs );
	}


        // time info
        struct timespec t;


	/* Sampling Period */
	double T = (double)interval/(double)NSEC_PER_SEC; // (sec)
	clock_gettime( 0,&t);
// ------------------------------------------------------------------------------
// ---------------[ DO NOT EDIT AVBOE THIS LINE]---------------------------------
// ------------------------------------------------------------------------------

//	char* fileName = "valve0.traj";

	runTraj(fileName,mode,  &H_ref, &t, &H_state, compliance_mode, pause_feature);

// ------------------------------------------------------------------------------
// ---------------[ DO NOT EDIT BELOW THIS LINE]---------------------------------
// ------------------------------------------------------------------------------


	printf("Trajectory Finished\n");
	return 0;
}


int runTraj(char* s, int mode,  struct hubo_ref *r, struct timespec *t, struct hubo_state* H_state, bool compliance_mode, bool pause_feature) {
	int i = 0;
// int interval = 10000000; // 100 hz (0.01 sec)

        hubo_virtual_t H_virtual;
        memset( &H_virtual, 0, sizeof(H_virtual));
        size_t fs;
        int rr = 0;
 	char str[1000];
	FILE *fp;		// file pointer
	fp = fopen(s,"r");
	if(!fp) {
		printf("No Trajectory File!!!\n");
		return 1;  // exit if not file
	}
	char c;
	bool paused=false;
       	tweak_init();

	int line_counter=0;

        double T = (double)interval/(double)NSEC_PER_SEC;

//	printf("Reading %s\n",s);
        double id = 0.0;
        while(fgets(str,sizeof(str),fp) != NULL) {
	//	printf("i = %d\n",i);
	//	i = i+1;
                // wait until next shot
                clock_nanosleep(0,TIMER_ABSTIME,t, NULL);
		
                if( HUBO_VIRTUAL_MODE_OPENHUBO == mode ){
                    for( id = 0 ; id < T;  id = id + HUBO_LOOP_PERIOD ){
                        rr = ach_get( &chan_hubo_from_sim, &H_virtual, sizeof(H_virtual), &fs, NULL, ACH_O_WAIT );
                    }
                }
// ------------------------------------------------------------------------------
// ---------------[ DO NOT EDIT AVBOE THIS LINE]---------------------------------
// ------------------------------------------------------------------------------
		line_counter++;
		printf("line is %d \n", line_counter);

		if ( read(STDIN_FILENO, &c, 1) == 1) {
        	         if (c=='p' && pause_feature==true) {
				paused=!paused;
				printf("paused is now %s \n", paused ? "true" : "false"); 
                	 }
             	}

		while (paused==true){
			usleep(1000000);//1 second
			t->tv_sec+=1; // for the 1 sec delay in line above		
			if ( read(STDIN_FILENO, &c, 1) == 1) {
		                if (c=='p'&& pause_feature==true) {
					paused=!paused;
 					printf("paused is now %s \n", paused ? "true" : "false");    
	       		 	}
               		 }
 		}


		int len = strlen(str)-1;
		if(str[len] == '\n') {
			str[len] = 0;
		}

		getArg(str, r); 	
		int joint;
		if (compliance_mode==true){
			for (joint=4; joint<18; joint++){//hard coded for arms only
				r->comply[joint]=1;
			}
		}
		else{
			for (joint=4; joint<18; joint++){//hard coded for arms only
				r->comply[joint]=0;
			}
		}

		if (goto_init_flag) {

		  struct hubo_ref tmpref;
		  int i;
		  
		  for (i=0; i<200; ++i) {

		    double u = (double)(i+1)/200;

		    for (joint=0; joint<HUBO_JOINT_COUNT; ++joint) {

		      tmpref.ref[joint] = u * r->ref[joint] + (1-u) * H_state->joint[joint].ref;

		    }

		    ach_put( &chan_hubo_ref, &tmpref, sizeof(tmpref));

		    usleep(0.01 * 1e6);		

		  }

		  goto_init_flag = 0;
		  clock_gettime(0, t);
		  // reset clock

		}
// ------------------------------------------------------------------------------
// ---------------[ DO NOT EDIT BELOW THIS LINE]---------------------------------
// ------------------------------------------------------------------------------

		// Cheeting No more RAP or LAP
/*
		r->ref[RHP] = 0.0;
		r->ref[LHP] = 0.0;
		r->ref[RAP] = 0.0;
		r->ref[LAP] = 0.0;
		r->ref[RKN] = 0.0;
		r->ref[LKN] = 0.0;
		r->ref[RAR] = 0.0;
		r->ref[LAR] = 0.0;
		r->ref[RHR] = 0.0;
		r->ref[LHR] = 0.0;
*/

/*
		for( i = 0 ; i < HUBO_JOINT_COUNT; i++){
			r->mode[i] = HUBO_REF_MODE_REF;
		}
*/

        	ach_put( &chan_hubo_ref, r, sizeof(*r));
		//printf("Ref r = %s\n",ach_result_to_string(r));
                t->tv_nsec+=interval;
                tsnorm(t);
        }

}


int runTrajFunction(char* s, int mode,  bool compliance_mode, bool pause_feature) {
	printf("into the func \n");

        double newRef[2] = {1.0, 0.0};
        // get initial values for hubo
        struct hubo_ref H_ref;
        memset( &H_ref,   0, sizeof(H_ref));
        hubo_virtual_t H_virtual;
        memset( &H_virtual, 0, sizeof(H_virtual));

	printf("after creating \n");
	fflush(stdout);
        size_t fs;
        int r = ach_get( &chan_hubo_ref, &H_ref, sizeof(H_ref), &fs, NULL, ACH_O_COPY );
	printf("after r \n");
	fflush(stdout);

        if(ACH_OK != r) {
                if(hubo_debug) {
                        printf("Ref ini r = %s\n",ach_result_to_string(r));}
                }
        else{   assert( sizeof(H_ref) == fs ); }

	printf("after achget \n");
	fflush(stdout);
        struct hubo_state H_state;
        memset( &H_state, 0, sizeof(H_state) );

        r = ach_get( &chan_hubo_state, &H_state, sizeof(H_state), &fs, NULL, ACH_O_COPY );
        if (ACH_OK != r) {
          if (hubo_debug) {
            printf("State ini r = %s\n", ach_result_to_string(r));
          }
        } else {
          assert( sizeof(H_state) == fs );
        }
	printf("after ach \n");

        // time info
        struct timespec t;


        /* Sampling Period */
        double T = (double)interval/(double)NSEC_PER_SEC; // (sec)
        clock_gettime( 0,&t);
// ------------------------------------------------------------------------------
// ---------------[ DO NOT EDIT AVBOE THIS LINE]---------------------------------
// ------------------------------------------------------------------------------

//      char* fileName = "valve0.traj";
	printf("getting into trajectory \n");
        runTraj(fileName,mode,  &H_ref, &t, &H_state, compliance_mode, pause_feature);

// ------------------------------------------------------------------------------
// ---------------[ DO NOT EDIT BELOW THIS LINE]---------------------------------
// ------------------------------------------------------------------------------


        printf("Trajectory Finished\n");
        return 0;


}


int  getArg(char* s,struct hubo_ref *r) {

sscanf(s, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", 
	&r->ref[RHY],
	&r->ref[RHR],
	&r->ref[RHP],
	&r->ref[RKN],
	&r->ref[RAP],
	&r->ref[RAR],
	&r->ref[LHY],
	&r->ref[LHR],
	&r->ref[LHP],
	&r->ref[LKN],
	&r->ref[LAP],
	&r->ref[LAR],
	&r->ref[RSP],
	&r->ref[RSR],
	&r->ref[RSY],
	&r->ref[REB],
	&r->ref[RWY],
	&r->ref[RWR],
	&r->ref[RWP],
	&r->ref[LSP],
	&r->ref[LSR],
	&r->ref[LSY],
	&r->ref[LEB],
	&r->ref[LWY],
	&r->ref[LWR],
	&r->ref[LWP],
	&r->ref[NKY],
	&r->ref[NK1],
	&r->ref[NK2],
	&r->ref[WST],
	&r->ref[RF1],
	&r->ref[RF2],
	&r->ref[RF3],
	&r->ref[RF4],
	&r->ref[RF5],
	&r->ref[LF1],
	&r->ref[LF2],
	&r->ref[LF3],
	&r->ref[LF4],
	&r->ref[LF5]);

        return 0;
}




void stack_prefault(void) {
        unsigned char dummy[MAX_SAFE_STACK];
        memset( dummy, 0, MAX_SAFE_STACK );
}


		
static inline void tsnorm(struct timespec *ts){

//	clock_nanosleep( NSEC_PER_SEC, TIMER_ABSTIME, ts, NULL);
        // calculates the next shot
        while (ts->tv_nsec >= NSEC_PER_SEC) {
                //usleep(100);	// sleep for 100us (1us = 1/1,000,000 sec)
                ts->tv_nsec -= NSEC_PER_SEC;
                ts->tv_sec++;
        }
}


// KEyboard Input

static int
tty_unbuffered(int fd) /* put terminal into a raw mode */
{
    struct termios buf;

    if (tcgetattr(fd, &buf) < 0)
        return(-1);

    save_termios = buf; /* structure copy */

    /* echo off, canonical mode off */
    buf.c_lflag &= ~(ECHO | ICANON);

    /* 1 byte at a time, no timer */
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
        return(-1);

    ttysavefd = fd;
    return(0);
}

static int
tty_reset(int fd) /* restore terminal's mode */
{
    if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
        return(-1);
    return(0);
}

static void
tty_atexit(void) /* can be set up by atexit(tty_atexit) */
{
    if (ttysavefd >= 0)
        tty_reset(ttysavefd);
}

static void
tweak_init()
{
   /* make stdin unbuffered */
    if (tty_unbuffered(STDIN_FILENO) < 0) {
        printf("Set tty unbuffered error");// << std::endl;
        //std::cout << "Set tty unbuffered error" << std::endl;
        exit(1);
    }

    atexit(tty_atexit);

    /* nonblock I/O */
    int flags;
    if ( (flags = fcntl(STDIN_FILENO, F_GETFL, 0)) == 1) {
        perror("fcntl get flag error");
        exit(1);
    }
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set flag error");
        exit(1);
    }
}
