#pragma once

#include <time.h>
#include "run.hpp"
#include "mathematica.hpp"

// struct histogram {
//     uint64_t read[NUM_CPUS];
// uint64_t failure[NUM_CPUS];
// uint64_t success[NUM_CPUS];
// #ifdef TIME
// uint64_t read_time[NUM_CPUS];
// uint64_t failure_time[NUM_CPUS];
// uint64_t success_time[NUM_CPUS];
// #endif
// };

// Keep track of how many times someone from a given node read the id of someone from node X (or itself or its hyperthread buddy), and saw the id of someone from node Y (or itself or its buddy) when it next CASed
struct histogram3d {
    uint64_t all[NUM_NODES + 2][NUM_NODES + 2];
    uint64_t success[NUM_NODES + 2][NUM_NODES + 2];
    uint64_t failure[NUM_NODES + 2][NUM_NODES + 2];
};

// #ifdef TIME
// static uint64_t dumb_div(uint64_t num, uint64_t den) {
//     if (den == 0) {
//         return 0;
//     }
//     return num/den;
// };
// #endif

// static void print_histos(histogram *histo, const std::vector<uint64_t>& workers) {
//     // Assumes that each element of workers is < NUM_CPUS.
//     mtca::list_open();
//     for (const auto& i: workers) {
//         for (const auto& j: workers) {
//             mtca::list_item_open();
//             mtca::assoc_open();
//             mtca::assoc_item("workerLooking", i);
//             mtca::assoc_item("workerSeen", j);
//             mtca::assoc_item("numAttempt", histo[i].read[j]);
//             mtca::assoc_item("numSuccess", histo[i].success[j]);
//             mtca::assoc_item("numFailure", histo[i].failure[j]);
// #ifdef TIME
//             mtca::assoc_item("timeRead", dumb_div(histo[i].read_time[j], histo[i].read[j]));
//             mtca::assoc_item("timeCasSuccess", dumb_div(histo[i].success_time[j], histo[i].success[j]));
//             mtca::assoc_item("timeCasFailure", dumb_div(histo[i].failure_time[j], histo[i].failure[j]));
// #endif
//             mtca::assoc_close();
//             mtca::list_item_close();
//         }
//     }
//     mtca::list_close();
// }

static void print_histos3d(histogram3d *histo) {
    mtca::list_open();
    for (uint64_t i = 0; i < NUM_NODES; i++) {
        for (uint64_t j = 0; j < NUM_NODES + 2; j++) {
            for(uint64_t k = 0; k < NUM_NODES + 2; k++) {
                mtca::list_item_open();
                mtca::assoc_open();
                mtca::assoc_item("nodeLooking", i);
                mtca::assoc_item("nodeSeenRead", j);
                mtca::assoc_item("nodeSeenCas", k);
                mtca::assoc_item("numAttempt", histo[i].all[j][k]);
                mtca::assoc_item("numSuccess", histo[i].success[j][k]);
                mtca::assoc_item("numFailure", histo[i].failure[j][k]);
                mtca::assoc_close();
                mtca::list_item_close();
            }
        }
    }
    mtca::list_close();
}

#ifdef TIME
static uint64_t nsec_elapsed(timespec start, timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
}
#endif

// static void tally(histogram *histos, sized_array<local_item> *local_data) {
//     debugf("tally START\n");
//     for (uint64_t worker = 0; worker < NUM_CPUS; worker++) {
//         local_item *log = local_data[worker].array;
//         // Starts at 1 because 0 is reserved for the initial shared_item.
//         uint64_t iter = 1;
//         while (!is_local_item_dummy(log[iter])) {
//             local_item entry = log[iter];
//             uint64_t other_worker = shared_item_worker(entry.val);
//             if (entry.op == rd) {
//                 histos[worker].read[other_worker]++;
// #ifdef TIME
//                 uint64_t read_time = nsec_elapsed(entry.start, entry.end);
//                 histos[worker].read_time[other_worker] += read_time;
// #endif
//             }
//             else if (entry.op == add){ //cannot log histo for xadd, so skipping
//                 iter++;
//                 continue;
//             }
//             else {
// #ifdef TIME
//                 uint64_t cas_time = nsec_elapsed(entry.start, entry.end);
// #endif
//                 if (log[iter-1].val == entry.val) {
//                     histos[worker].success[other_worker]++;
// #ifdef TIME
//                     histos[worker].success_time[other_worker] += cas_time;
// #endif
//                 } else {
//                     histos[worker].failure[other_worker]++;
// #ifdef TIME
//                     histos[worker].failure_time[other_worker] += cas_time;
// #endif
//                 }
//             }
//             iter++;
//         }
//         debugf("tally counted %lu items for worker %lu\n", iter, worker);
//     }
//     debugf("tally END\n");
// }

static uint64_t node_index(uint64_t worker, uint64_t observed) {
    if (worker == observed) {
        return NUM_NODES;
    }
    if (observed == buddy(worker)) {
        return NUM_NODES + 1;
    }
    return cpu_node(observed);
}

static void tally3d(histogram3d *histos, sized_array<local_item> *local_data) {
    for (uint64_t worker = 0; worker < NUM_CPUS; worker++) {
        local_item *log = local_data[worker].array;
        // Starts at 1 because 0 is reserved for the initial shared_item.
        uint64_t iter = 1;
        while (!is_local_item_dummy(log[iter])) {
            local_item entry = log[iter];
            local_item next_entry = log[iter+1];
            uint64_t worker_seen = shared_item_worker(entry.val);
            uint64_t node_seen = node_index(worker, worker_seen);
            uint64_t next_worker_seen = shared_item_worker(next_entry.val);
            uint64_t next_node_seen = node_index(worker, next_worker_seen);
            histos[cpu_node(worker)].all[node_seen][next_node_seen]++;
            if (next_entry.op == cas || next_entry.op == ccas) {

                if (entry.val == next_entry.val) {
                    histos[cpu_node(worker)].success[node_seen][next_node_seen]++;
                } else {
                    histos[cpu_node(worker)].failure[node_seen][next_node_seen]++;
                }
            }
            iter++;
        }
    }
}
