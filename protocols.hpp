#pragma once

#include <time.h>
#include "helper.hpp"
#include "data_item.hpp"

#define MAX_DELAY 0x80000000
#define THE_CLOCK CLOCK_MONOTONIC

#ifdef LOG
#define log_val(x, y) x = y
#else
#define log_val(x, y) pass()
#endif

#if defined(LOG) && defined(TIME)
#define log_time(x) clock_gettime(THE_CLOCK, x)
#else
#define log_time(x) pass()
#endif

static inline uint64_t read(uint64_t *variable, local_item* log, uint64_t* iter) {
    log_time(&(log[*iter].start));
    uint64_t val = *variable;
    log_time(&(log[*iter].end));
    log_val(log[*iter].val, val);
    log_val(log[*iter].op, rd);
    *iter = *iter + 1;
    return val;
}
static inline uint64_t xadd(uint64_t *variable, uint64_t value, local_item* log, uint64_t* iter, uint64_t vloc) {
   log_time(&(log[*iter].start));
   uint64_t result = __atomic_fetch_add(variable, value, __ATOMIC_RELAXED);
   //asm volatile(
   //             "lock; xadd %%eax, %2;"
   //             :"=a" (value)                   //Output
   //             : "a" (value), "m" (*variable)  //Input
   //             :"memory" );
   log_time(&(log[*iter].end));
   log_val(log[*iter].val, result);
   log_val(log[*iter].op, add);
   log_val(log[*iter].loc, vloc);
   *iter = *iter + 1;
   return result;
}
#define FAA_primitive(shared, inc) xadd(shared, inc)
#define CAS_primitive(shared, guess, write) __atomic_compare_exchange_n(shared, guess, write, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
#ifdef CCAS
static bool CAS(uint64_t* shared, uint64_t* guess, uint64_t write, local_item* log, uint64_t* iter) {
    bool success;
    log_time(&(log[*iter].start));
    uint64_t read = *shared;
    if (read != *guess) {
        *guess = read;
        success = false;
    } else {
        success = CAS_primitive(shared, guess, write);
    }
    log_time(&(log[*iter].end));
    log_val(log[*iter].val, *guess);
    log_val(log[*iter].op, ccas);
    *iter = *iter + 1;
    return success;
}
#else
static bool CAS(uint64_t* shared, uint64_t* guess, uint64_t write, local_item* log, uint64_t* iter) {
    bool success;
    log_time(&(log[*iter].start));
    success = CAS_primitive(shared, guess, write);
    log_time(&(log[*iter].end));
    log_val(log[*iter].val, *guess);
    log_val(log[*iter].op, cas);
    *iter = *iter + 1;
    return success;
}
#endif

// Trying to confuse the compiler/hardware.
static void wait(uint64_t delay) {
#ifndef NODELAY
    volatile uint64_t count = 0;
    while (count++ < delay) {}
#endif
}

static void naive(uint64_t worker, uint64_t *iter, /*uint64_t *iter_streak,*/ shared_item *x, local_item* log, uint64_t len_atom, uint64_t len_prep, uint64_t vloc) {
#ifdef XADD
    xadd(x, 1, log, iter, vloc);
    wait(len_prep);
    return;
#endif
    // const uint64_t ourBuddy = buddy(worker);
    bool success = false;
    // bool new_streak = false;
#ifdef ONEREAD
    uint64_t start_iter = *iter;
#endif
    shared_item new_val;
    shared_item val;
#ifdef NOREAD
    val = make_shared_item(worker, *iter-1);
#endif
    do {
#if !defined(NOREAD) && !defined(ONEREAD)
        // Just CAS'd
        wait(len_prep);
        // Read
        val = read(x, log, iter);
        // uint64_t them = shared_item_worker(val);
        // if (!new_streak && worker != them && ourBuddy != them) {
        //     new_streak = true;
        // }
        wait(len_atom);
#endif
#ifdef ONEREAD
        if (start_iter == *iter) {
            val = read(x, log, iter);
            wait(len_atom);
        }
#endif
        new_val = make_shared_item(worker, *iter);
        success = CAS(x, &val, new_val, log, iter);
#if defined(NOREAD) || defined(ONEREAD)
        wait(len_prep);
#endif
    } while (!success);
    // *iter_streak += (uint64_t) new_streak;
}
