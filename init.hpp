#pragma once

#include <iostream>
#include <numa.h>
#include "data_item.hpp"
#include "run.hpp"
#include "sized_array.hpp"

static bool use_worker(const std::vector<uint64_t> nodes, bool use_buddies, uint64_t worker) {
    const bool node_okay =
        nodes.size() == 0 ||
        std::find(nodes.begin(), nodes.end(), cpu_node(worker)) != nodes.end();
    const bool buddy_okay = use_buddies || !is_second_buddy(worker);
    return node_okay && buddy_okay;
}

static std::vector<uint64_t>* init_workers (const std::vector<uint64_t> nodes, bool use_buddies, const std::vector<uint64_t> workers) {
    debugf("init_workers START\n");
    std::vector<uint64_t> *new_workers = new std::vector<uint64_t>();
    if (workers.size() == 0) {
        for (uint64_t worker = 0; worker < NUM_CPUS; worker++) {
            if (use_worker(nodes, use_buddies, worker)) {
                new_workers->push_back(worker);
            }
        }
    } else {
        for (uint64_t worker : workers) {
            if (use_worker(nodes, use_buddies, worker)) {
                new_workers->push_back(worker);
            }
        }
    }
    debugf("init_workers END\n");
    return new_workers;
}

static void init_shared_data (shared_item **shared_data) {
    debugf("init_shared_data START\n");
    for (uint64_t node = 0; node < NUM_NODES; node++) {
        debugf("init_shared_data node %lu\n", node);
        shared_data[node] = (shared_item*) numa_alloc_onnode(sizeof(shared_item) * NUM_SHARED_DATA, node);
        for (uint64_t i = 0; i < NUM_SHARED_DATA; i++) {
            shared_data[node][i] = EMPTY_SHARED_ITEM;
        }
    }
    debugf("init_shared_data END\n");
}

static void init_local_data (sized_array<local_item> *local_data) {
    debugf("init_local_data START\n");
    for (uint64_t worker = 0; worker < NUM_CPUS; worker++) {
        uint64_t node = cpu_node(worker);
        local_data[worker].array = (local_item*) numa_alloc_onnode(sizeof(local_item) * NUM_LOCAL_DATA, node);
        // If this worker is not used, its log should have length 0.
        local_data[worker].array[0] = DUMMY_LOCAL_ITEM;
        // TODO: need this?
        local_data[worker].array[1] = DUMMY_LOCAL_ITEM;
        local_data[worker].len = 0;
    }
    debugf("init_local_data END\n");
}

// Initializes following fields of p: workers, shared_data, local_data, stop_now
static void init(test_params &p, const std::vector<uint64_t>& nodes, bool use_buddies, const std::vector<uint64_t>& workers) {
    debugf("init START\n");

    p.workers = *init_workers(nodes, use_buddies, workers);
    init_shared_data(p.shared_data);
    init_local_data(p.local_data);
    p.stop_now = false;

    debugf("init END\n");
}
