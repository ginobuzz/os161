/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>
#include <wchan.h>
#include <current.h>

struct lock * get_lock_ref(int quad);

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

//Global Wait Channels.
struct semaphore *sem_Male;
struct semaphore *sem_Female;
struct semaphore *sem_Match_M;
struct semaphore *sem_Match_F;

//Used only for debugging purposes. Not needed.
int num;

void whalemating_init() {
	//Create semaphores.
	sem_Male    = sem_create("Male Semaphore",0);
	sem_Female  = sem_create("Female Semaphore",0);
	sem_Match_M = sem_create("Match-Male Semaphore",0); 
	sem_Match_F = sem_create("Match-Female Semaphore",0); 
	//Check semaphores did not fail to create.
	if (sem_Male == NULL || sem_Female == NULL || sem_Match_M == NULL || sem_Match_F == NULL){
		panic("semaphore failed to create.\n");
	}else{
		kprintf("[Semaphores Created Successfully]\n");
	}  	
	return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.
void whalemating_cleanup() {
	sem_destroy(sem_Male);
	sem_destroy(sem_Female);
	sem_destroy(sem_Match_M);
	sem_destroy(sem_Match_F);
}

void
male(void *p, unsigned long which)
{
	struct semaphore *whalematingMenuSemaphore = (struct semaphore *)p;
  	(void)which;
  
 	male_start();
	V(sem_Match_M);
	P(sem_Male);
  	male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
	struct semaphore *whalematingMenuSemaphore = (struct semaphore *)p;
  	(void)which;
  
  	female_start();
	V(sem_Match_F);
	P(sem_Female);
  	female_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
	struct semaphore *whalematingMenuSemaphore = (struct semaphore *)p;
  	(void)which;

  	matchmaker_start();
	P(sem_Match_M);
	P(sem_Match_F);
	V(sem_Male);
	V(sem_Female);
  	matchmaker_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}






/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

struct lock * lock_0;
struct lock * lock_1;
struct lock * lock_2;
struct lock * lock_3;

void stoplight_init() {
	lock_0 = lock_create("Lock for quad 0");
	lock_1 = lock_create("Lock for quad 1");
	lock_2 = lock_create("Lock for quad 2");
	lock_3 = lock_create("Lock for quad 3");
  return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {
	lock_release(lock_0);
	lock_release(lock_1);
	lock_release(lock_2);
	lock_release(lock_3);
  return;
}

// Always grab the highest number quadrant first!

void
gostraight(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
	struct lock *my_lock0, *my_lock1;
	int A,B;

	A = direction;
	B = (direction+3)%4;

	if (A>B) {
		my_lock0 = get_lock_ref(A);
		my_lock1 = get_lock_ref(B);		
	}
	else {
		my_lock0 = get_lock_ref(B);
		my_lock1 = get_lock_ref(A);
	}

	lock_acquire(my_lock0);
	lock_acquire(my_lock1);
	inQuadrant(A);
	inQuadrant(B);
	lock_release(get_lock_ref(A));
	leaveIntersection();
	lock_release(get_lock_ref(B));

  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
	struct lock * my_lock0, *my_lock1, *my_lock2;
	int A,B,C;

	A = direction;
	B = (direction+3)%4;
	C = (direction+2)%4;

	if (A>B && B>C) {
		my_lock0 = get_lock_ref(A);
		my_lock1 = get_lock_ref(B);
		my_lock2 = get_lock_ref(C);				
	} else if (B>C && C>A) {
		my_lock0 = get_lock_ref(B);
		my_lock1 = get_lock_ref(C);
		my_lock2 = get_lock_ref(A);
	} else {
		my_lock0 = get_lock_ref(C);
		my_lock1 = get_lock_ref(A);
		my_lock2 = get_lock_ref(B);
	}

	lock_acquire(my_lock0);
	lock_acquire(my_lock1);
	lock_acquire(my_lock2);
	inQuadrant(A);
	inQuadrant(B);
	lock_release(get_lock_ref(A));
	inQuadrant(C);
	lock_release(get_lock_ref(B));
	leaveIntersection();
	lock_release(get_lock_ref(C));
	
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
	struct lock* my_lock;
	
	my_lock = get_lock_ref(direction);

	lock_acquire(my_lock);

	inQuadrant(direction);
	leaveIntersection();
	lock_release(my_lock);
	

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}


struct lock *
get_lock_ref(int quad)
{
	switch (quad) {	
		case 0: return lock_0;
		case 1: return lock_1;
		case 2: return lock_2;
		case 3: return lock_3;
	}
	return NULL;
}
