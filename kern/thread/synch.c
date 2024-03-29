/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, int initial_count)
{
        struct semaphore *sem;

        KASSERT(initial_count >= 0);

        sem = kmalloc(sizeof(struct semaphore));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void 
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 * Bridge to the wchan lock, so if someone else comes
		 * along in V right this instant the wakeup can't go
		 * through on the wchan until we've finished going to
		 * sleep. Note that wchan_sleep unlocks the wchan.
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_lock(sem->sem_wchan);
		spinlock_release(&sem->sem_lock);
                wchan_sleep(sem->sem_wchan);

		spinlock_acquire(&sem->sem_lock);
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
    struct lock *lock;

    lock = kmalloc(sizeof(struct lock));
    if (lock == NULL) {
            return NULL;
    }

    lock->lk_name = kstrdup(name);
    if (lock->lk_name == NULL) {
            kfree(lock);
            return NULL;
    }
        
	lock->lk_free = true;

	lock->lk_holder = NULL;
        
	spinlock_init(&lock->lk_spin);

	lock->lk_wchan = wchan_create("Lock wait channel");

    return lock;
}

void
lock_destroy(struct lock *lock)
{
        KASSERT(lock != NULL);

        // add stuff here as needed

        if(lock->lk_wchan != NULL){
		wchan_destroy(lock->lk_wchan);
		spinlock_cleanup(&lock->lk_spin);
        	kfree(lock->lk_name);
        	kfree(lock);
        }
}	

void
lock_acquire(struct lock *lock)
{
        // Write this
	spinlock_acquire(&lock->lk_spin);

	if (lock_do_i_hold(lock))
		panic("\nThread attempted to acquire lock that it already held\n");

	while (!lock->lk_free) {			//if the lock is already taken
		wchan_lock(lock->lk_wchan);
		spinlock_release(&lock->lk_spin);
               	wchan_sleep(lock->lk_wchan);
		spinlock_acquire(&lock->lk_spin);
  	}
		//the lock is finally free, so the current thread can now acquire it
	lock->lk_free = false;
	lock->lk_holder = curthread;

	spinlock_release(&lock->lk_spin);
}

void
lock_release(struct lock *lock)
{
        // Write this
	spinlock_acquire(&lock->lk_spin);

	if(!lock_do_i_hold(lock))
		panic("Thread attempted to release a lock that it did not hold\n");

	if (!lock->lk_free) {
        	lock->lk_free = true;
		lock->lk_holder = NULL;
		wchan_wakeone(lock->lk_wchan);
	}

	spinlock_release(&lock->lk_spin);
}

bool
lock_do_i_hold(struct lock *lock)
{
	bool result;
	
	//spinlock_acquire(&lock->lk_spin);

	if (lock->lk_holder == NULL) {
		result = false;
	} else {
       		result = lock->lk_holder == curthread;
	}

	//spinlock_release(&lock->lk_spin);

	return result;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(struct cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }
        
        // add stuff here as needed

	cv->cv_wchan = wchan_create(cv->cv_name);
        
        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed
	wchan_destroy(cv->cv_wchan);    
        kfree(cv->cv_name);
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        // Write this
	wchan_lock(cv->cv_wchan);
	lock_release(lock);
	wchan_sleep(cv->cv_wchan);
	lock_acquire(lock);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
        // Write this
	KASSERT(lock_do_i_hold(lock));

	wchan_wakeone(cv->cv_wchan);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(lock_do_i_hold(lock));

	wchan_wakeall(cv->cv_wchan);
}

////////////////////////////////////////////////////////////////////
///////      Reader-Witer Lock

struct rwlock * rwlock_create(const char *name)
{
	struct rwlock * rw;

	rw = kmalloc(sizeof(struct rwlock));
	
	rw->rw_name = kstrdup(name);

	rw->rw_lock = lock_create("RW lock");
	rw->rw_rlock = lock_create("Reader lock");

	rw->rw_numreads = 0;

	return rw;
}

void rwlock_destroy(struct rwlock *rw)
{
	kfree(rw->rw_name);
	lock_destroy(rw->rw_lock);
	lock_destroy(rw->rw_rlock);
	kfree(rw);
}

void rwlock_acquire_read(struct rwlock *rw)
{
	lock_acquire(rw->rw_lock);		//a reader acquires the main lock
	rw->rw_numreads++;			//increments the number of active readers
	lock_release(rw->rw_lock);		//and releases the main lock to allow other readers/writers to acquire it
}

void rwlock_release_read(struct rwlock *rw)
{
	lock_acquire(rw->rw_rlock);		//secondary lock for allowing readers to exit, but not enter, and to protect rw_numreads
	rw->rw_numreads--;			//reader has finished, so decrement the number of readers
	lock_release(rw->rw_rlock);
}

void rwlock_acquire_write(struct rwlock *rw)
{
	lock_acquire(rw->rw_lock);		//block new readers from starting by acquiring the main lock
	while (rw->rw_numreads != 0) {		//wait until all active readers finish
		continue;
	}
}

void rwlock_release_write(struct rwlock *rw)
{
	lock_release(rw->rw_lock);		//release the main lock
}
