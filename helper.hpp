#pragma once

#include <numa.h>

void pass() {}

enum op_type {rd, cas, ccas, add};

static const char* op_type_name_array[] = {"Read", "CAS", "CCAS", "Add"};

const char* op_type_name(op_type op) {
    return op_type_name_array[op];
}

static uint64_t cpu_node(uint64_t cpu_id) {
    return numa_node_of_cpu(cpu_id);
}

uint64_t buddy(uint64_t cpu_id) {
    if (ROUND_ROBIN) {
        uint64_t halfway = NUM_CPUS / 2;
        return cpu_id >= halfway ? cpu_id - halfway : cpu_id + halfway;
    } else if (BLOCKS) {
        return cpu_id % 2 == 0 ? cpu_id + 1 : cpu_id - 1;
    } else {
        // We don't know who our buddy is.
        return cpu_id;
    }
}

bool is_second_buddy(uint64_t cpu_id) {
    if (ROUND_ROBIN) {
        return cpu_id >= NUM_CPUS / 2;
    } else if (BLOCKS) {
        return cpu_id % 2 == 1;
    } else {
        // We don't know whether we're the first or second buddy,
        // but it doesn't matter because the --no-smt isn't supported.
        return false;
    }
}

uint64_t worker_index_within_node(uint64_t cpu_id) {
    if (ROUND_ROBIN) {
        uint64_t halfway = NUM_CPUS / 2;
        return 2 * ((cpu_id % halfway) / NUM_NODES) + (cpu_id >= halfway ? 1 : 0);
    } else if (BLOCKS) {
        return cpu_id % NUM_NODES;
    } else {
        uint64_t index = 0;
        for (uint64_t i = 0; i < cpu_id; i++) {
            if (cpu_node(i) == cpu_node(cpu_id)) {
                index++;
            }
        }
        return index;
    }
}
