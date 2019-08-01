#pragma once

#include <unistd.h>
#include "parallel.hpp"
#include "get_time.hpp"
#include "protocols.hpp"
#include "sized_array.hpp"
#ifndef ONELOC
#include "hash.hpp"
#endif

#define READY_DELAY 4
#define FETCH_ADD(x, y) __atomic_fetch_add(x, y, __ATOMIC_RELAXED)

static timer bt;

struct test_params {
    uint64_t len_atom;
    uint64_t len_prep;
    uint64_t len_wait;
    uint64_t loc_mask;
    uint32_t time;
    std::vector<uint64_t> workers;
#ifdef MAPPING
    std::vector<uint64_t> mapping;
#ifdef MAPPING2
    std::vector<uint64_t> mapping2;
    double split;
#endif
#endif
    shared_item* shared_data[NUM_NODES];
    sized_array<local_item> local_data[NUM_CPUS];
    pthread_barrier_t start_barrier;
    volatile bool stop_now;
    uint64_t num_read[NUM_CPUS];
    uint64_t num_success[NUM_CPUS];
    uint64_t num_streak[NUM_CPUS];
};

static uint64_t virtual_loc(uint64_t i, uint64_t loc_mask) {
#ifdef ONELOC
    return 0;
#else
    return hash32(i) & loc_mask;
#endif
}

static uint64_t physical_index(uint64_t vloc) {
#ifdef ONELOC
    return 0;
#else
    // TODO: double check that this spaces enough.
    return vloc << (NUM_SHARED_DATA_SHIFT - MAX_SHARED_DATA_CAP_SHIFT);
#endif
}

#ifndef MAPPING
static shared_item* physical_loc(shared_item** shared_data, uint64_t vloc) {
    const uint64_t node = vloc & NUM_NODES_MASK;
    const uint64_t index = physical_index(vloc);
    return &(shared_data[node][index]);
}
#endif

#ifdef MAPPING
// shared_data_mapping is shared_data[mapping[my_node]]
static shared_item* physical_loc_mapping(shared_item* shared_data_mapping, uint64_t vloc) {
    const uint64_t index = physical_index(vloc);
    return &(shared_data_mapping[index]);
}
#ifdef MAPPING2
bool use_mapping2(double split, uint64_t worker) {
    return 1 - split < ((double) worker_index_within_node(worker) + 0.5) / ((double) (NUM_CPUS / NUM_NODES));
}
#endif
#endif

static void* run_one(par::with_worker<test_params *> *wx) {
    const uint64_t worker = wx->worker;
    test_params p = *(wx->value);
    debugf("run_one %lu START\n", worker);
    for (uint64_t i = 0; i < NUM_LOCAL_DATA; i++) {
        p.local_data[worker].array[i] = DUMMY_LOCAL_ITEM;
    }
    debugf("run_one %lu READY\n", worker);
    pthread_barrier_wait(&(wx->value->start_barrier));
    debugf("run_one %lu GOING\n", worker);
    // TODO: what should iter_success start at?
    const uint64_t iter_success_start = worker << 24;
    uint64_t iter_success = iter_success_start;
    // Starts at 1 because 0 is reserved for the initial shared_item.
    uint64_t iter_attempt = 1;
    uint64_t iter_streak = 0;
    // p.stop_now is "frozen", so we need to use wx->value->stop.
#ifdef MAPPING
    shared_item* shared_data_mapping = p.shared_data[p.mapping[cpu_node(worker)]];
#ifdef MAPPING2
    if (use_mapping2(p.split, worker)) {
        shared_data_mapping = p.shared_data[p.mapping2[cpu_node(worker)]];
    }
#endif
#endif
    while (!wx->value->stop_now) {
        // This is the section that's repeated millions of times.
        // TODO: share hashing work?
        const uint64_t vloc = virtual_loc(iter_success, p.loc_mask);
#ifdef MAPPING
        shared_item *loc = physical_loc_mapping(shared_data_mapping, vloc);
#else
        shared_item *loc = physical_loc(p.shared_data, vloc);
#endif
        naive(worker, &iter_attempt, &iter_streak, loc, p.local_data[worker].array, p.len_atom, p.len_prep, vloc);
        wait(p.len_wait);
        iter_success++;
    }
    wx->value->local_data[worker].len = iter_attempt;
    wx->value->num_read[worker] =
        // Subtract 1 because iter_attempt starts at 1.
#ifdef XADD
        iter_attempt - 1
#else
        // Divide by 2 because both read and CAS increment iter_attempt.
        (iter_attempt - 1) / 2
#endif
        ;
    wx->value->num_success[worker] = iter_success - iter_success_start;
    wx->value->num_streak[worker] = iter_streak;
    debugf("run_one %lu END\n", worker);
    return NULL;
}

struct set_after_time_t {
    uint32_t time;
    volatile bool *flag;
    bool value;
};

static void* set_after_time_helper(void *params) {
    set_after_time_t p = *((set_after_time_t *)params);
    usleep(p.time);
    *(p.flag) = p.value;
    free(params);
    debugf("set after time %u %p %i\n", p.time, p.flag, p.value);
    return NULL;
}

static void set_after_time(uint32_t time, volatile bool* flag, bool value) {
    pthread_t thread;
    set_after_time_t *params = (set_after_time_t*) malloc(sizeof(set_after_time_t));
    params->time = time;
    params->flag = flag;
    params->value = value;
    pthread_create(&thread, NULL, set_after_time_helper, (void *)params);
}

static double run(test_params &p) {
    debugf("run START\n");
    uint64_t wxs_len = p.workers.size();
    pthread_barrier_init(&(p.start_barrier), NULL, wxs_len + 1);
    par::with_worker<test_params *> wxs[NUM_CPUS] = {};
    for (uint64_t i = 0; i < wxs_len; i++) {
        wxs[i].worker = p.workers[i];
        wxs[i].value = &p;
    }
    par::create_for(wxs, wxs_len, run_one);
    debugf("run READY\n");
    pthread_barrier_wait(&(p.start_barrier));
    set_after_time(p.time, &(p.stop_now), true);
    double actual_time = par::join_for();
    debugf("run END\n");
    return actual_time;
}
