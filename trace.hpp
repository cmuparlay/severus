#pragma once

#include "run.hpp"
#include <sysexits.h>

//iter refers to iteration in local log, index refers to index of success in global log
struct local_info_item {
    uint64_t loc_index;
    // local_item is a success, and this is its index.
    uint64_t index;
    // local_item is not a successful CAS, and this is the index it saw.
    uint64_t index_seen;
};

struct local_info {
    // uint64_t iter_start;
    // uint64_t iter_end;
    local_info_item* items;
};

#ifdef XADD
//make traces for xadd use case.
static void xadd_trace(sized_array<shared_item>* traces,
                       const std::vector<uint64_t>& workers,
                       shared_item** shared_data,
                       sized_array<local_item>* local_data,
                       uint64_t num_locs) {
    debugf("xadd_trace\n");
    for(uint64_t mapping_index = 0; mapping_index < NUM_MAPPINGS; mapping_index++) {
        for(uint64_t vloc = 0; vloc < num_locs; vloc++) {
            const uint64_t loc_index = mapping_index * num_locs + vloc;
            uint64_t worker_index[NUM_CPUS]; //keep track of how far we've gone in each worker's log
            for (uint64_t w = 0; w < NUM_CPUS; w++){
                worker_index[w] = 1;
            }
            uint64_t current = 0;
#ifdef MAPPING
            shared_item item = *(physical_loc_mapping(shared_data[mapping_index], vloc));
#else
            shared_item item = *(physical_loc(shared_data, vloc));
#endif
            debugf("xadd end val is %lu\n", item);
            while (current < item && current < NUM_TRACE) {
                // debugf("xadd current val is %lu\n", current);
                for (auto& worker: workers) {
                    sized_array<local_item> log = local_data[worker];
                    if (log.array[NUM_LOCAL_DATA - 1].val != DUMMY_SHARED_ITEM) {
                        // We used the entire log, so we started overflowing.
                        eprintf("Error: log overflow\n");
                        exit(EX_SOFTWARE);
                    }
                    local_item entry = log.array[worker_index[worker]];
                    //find next relevant entry
                    while (entry.loc != vloc && worker_index[worker] < log.len-1) {
                        worker_index[worker]++;
                        entry = log.array[worker_index[worker]];
                    }
                    if (entry.loc != vloc) {
                        continue;
                    }
                    // debugf("Worker is %lu, and entry val is %lu\n", worker, entry.val);
                    //we now know that entry is to this location.
                    if (entry.val == current) {
                        traces[loc_index].array[current] = worker;
                        current++;
                        if (worker_index[worker] < log.len-1) {
                            worker_index[worker]++;
                        }
                        break;
                    }
                }
            }
            traces[loc_index].len = current;
        }
    }
}

static void print_xadd_trace(sized_array<shared_item>* traces, uint64_t num_locs, uint64_t length){
    mtca::list_open();
    //looking at the log relevant to one location at a time
    for (uint64_t loc_index = 0; loc_index < NUM_MAPPINGS * num_locs; loc_index++) {
        if (traces[loc_index].len < length + 2) {
            debugf("print_xadd_trace skipping loc_index %lu length %lu\n", loc_index, traces[loc_index].len);
            continue;
        }
        const uint64_t index_max = traces[loc_index].len/2 + length/2;
        const uint64_t index_min = index_max - length;
        debugf("print_xadd_trace loc_index %lu index_max %lu index_min %lu\n",
               loc_index, index_max, index_min);
        mtca::list_item_open();
        mtca::assoc_open();

        mtca::assoc_item("loc", loc_index);

        mtca::assoc_item_open("trace");
        mtca::list_open();
        for (uint64_t index = index_min; index < index_max; index++) {
            mtca::list_item(traces[loc_index].array[index]);
        }
        mtca::list_close();
        mtca::assoc_item_close();
        mtca::assoc_close();
        mtca::list_item_close();
    }
    mtca::list_close();
}

#else // not XADD

// //Run after follow_trace has populated successful cas trace.
// //Populates local_infos for all log entries with the index that it read and CASed.
// static void follow_attempts(sized_array<shared_item>* traces,
//                             const std::vector<uint64_t>& workers,
//                             local_info* infos,
//                             shared_item** shared_data,
//                             sized_array<local_item>* local_data,
//                             uint64_t num_locs,
//                             uint64_t length) {
//     debugf("follow_attempts START\n");
//     const uint64_t index_max = traces[0].len/2 + length/2;
//     for (auto& worker : workers) {
//         debugf("follow_attempts worker %lu\n", worker);
//         local_item* log = local_data[worker].array;
//         local_info_item* items = infos[worker].items;
//         uint64_t iter = local_data[worker].len - 1;
//         bool* stop_flags;
//         stop_flags = new bool[num_locs]();
//         uint64_t counter = 0;
//         //traverse record of each worker from end to beginning
//         while (iter > 0 && counter < num_locs) {
//             local_item entry = log[iter];
//             shared_item entry_val = entry.val;
//             //populate local_info_item for all entries in the record
//             //this depends on the local_info_item for successful attempts already being populated
//             uint64_t entry_worker = shared_item_worker(entry_val);
//             uint64_t entry_iter = shared_item_iter(entry_val);
//             items[iter].index_seen = infos[entry_worker].items[entry_iter].index;
//             items[iter].loc_index = infos[entry_worker].items[entry_iter].loc_index;
//             //want to get all operations up to index_max number of successes for each location
//             //index_seen depends on the location of the operation we're looking at right now
//             if (items[iter].index_seen > index_max) {
//                 if (stop_flags[items[iter].loc_index] == false) {
//                     stop_flags[items[iter].loc_index] = true;
//                     counter++;
//                 }
//             }
//             iter--;
//         }
//         delete stop_flags;
//     }
//     debugf("follow_attempts END\n");
// }

// // Populates traces based on shared_data and local_data.
// // traces must be an array of size (loc_mask + 1). It will be populated with trace of successes per location.
// static void follow_success_trace(sized_array<shared_item>* traces,
//                          const std::vector<uint64_t>& workers,
//                          local_info* infos,
//                          shared_item** shared_data,
//                          sized_array<local_item>* local_data,
//                          uint64_t num_locs,
//                          uint64_t length) {
//     debugf("follow_trace START\n");
//     for (uint64_t mapping_index = 0; mapping_index < NUM_MAPPINGS; mapping_index++) {
//         for (uint64_t vloc = 0; vloc < num_locs; vloc++) {
//             debugf("follow_trace mapping_index %lu vloc %lu\n", mapping_index, vloc);
//             const uint64_t loc_index = mapping_index * num_locs + vloc;
//             //current value at the CAS location gives us the last thread to have successfully CASed.
//             //we follow trace of scuccesses backwards from there.
// #ifdef MAPPING
//             shared_item item = *(physical_loc_mapping(shared_data[mapping_index], vloc));
// #else
//             shared_item item = *(physical_loc(shared_data, vloc));
// #endif
//             uint64_t index = 0;
//             while (!is_shared_item_empty(item) && index < (NUM_TRACE / 2)) {
//                 const uint64_t worker = shared_item_worker(item);
//                 const uint64_t iter = shared_item_iter(item);
//                 traces[loc_index].array[index] = item;
//                 infos[worker].items[iter].loc_index = loc_index;
//                 // Need to set both for is_local_info_item_failure to work.
//                 infos[worker].items[iter].index = index;
//                 // Go to next item.
//                 item = local_data[worker].array[iter].val;
//                 index++;
//             }
//             traces[loc_index].len = index;
//         }
//     }
//     debugf("follow_trace END\n");
// }

// static void make_trace(sized_array<shared_item>* traces,
//                        const std::vector<uint64_t>& workers,
//                        local_info* infos,
//                        shared_item** shared_data,
//                        sized_array<local_item>* local_data,
//                        uint64_t num_locs,
//                        uint64_t length) {
//     follow_success_trace(traces, workers, infos, shared_data, local_data, num_locs, length);
//     follow_attempts(traces, workers, infos, shared_data, local_data, num_locs, length);
// }

// static void print_trace(sized_array<shared_item>* traces,
//                         const std::vector<uint64_t>& workers,
//                         local_info* infos,
//                         sized_array<local_item>* local_data,
//                         uint64_t num_locs,
//                         uint64_t length) {
//     const uint64_t index_max = traces[0].len/2 + length/2;
//     const uint64_t index_min = index_max - length;
//     debugf("print_trace index_max %lu index_min %lu\n",
//            index_max, index_min);

//     mtca::list_open();
//     //looking at the log relevant to one location at a time
//     for (uint64_t vloc = 0; vloc < num_locs; vloc++) {
//         mtca::list_item_open();
//         mtca::assoc_open();

//         mtca::assoc_item("loc", vloc);

//         mtca::assoc_item_open("trace");
//         mtca::list_open();
//         for (uint64_t index = index_max - 1; index >= index_min; index--) {
//             mtca::list_item(shared_item_worker(traces[vloc].array[index]));
//         }
//         mtca::list_close();
//         mtca::assoc_item_close();

//         mtca::assoc_item_open("local");
//         mtca::list_open();
//         //looking at log of one worker at a time
//         for (auto& worker : workers) {
//             mtca::list_item_open();
//             mtca::assoc_open();

//             mtca::assoc_item("worker", worker);

//             mtca::assoc_item_open("attempts");
//             mtca::list_open();
//             uint64_t iter = local_data[worker].len - 1;
//             //go through log to find the iteration at which we want to start printing.
//             while (iter > 0) {
//                 const local_info_item info = infos[worker].items[iter];
//                 if (info.index_seen > index_min) {
//                     break;
//                 }
//                 iter--;
//             }
//             debugf("print_trace local worker %lu len %lu iter %lu\n",
//                    worker, local_data[worker].len, iter);
//             while (iter > 0) {
//                 const local_info_item info = infos[worker].items[iter];
//                 if (info.index_seen >= index_max) { //was index_read
//                     break;
//                 }
//                 mtca::list_item_open();
//                 mtca::assoc_open();
//                 mtca::assoc_item("opType", op_type_name(local_data[worker].array[iter].op));
//                 mtca::assoc_item("index", index_max - 1 - info.index_seen);
// #ifndef ONELOC
//                 mtca::assoc_item("location", info.loc_index);
// #endif
//                 //mtca::assoc_item("indexCas", index_max - 1 - info.index_cas);
//                 mtca::assoc_close();
//                 mtca::list_item_close();
//                 iter--;
//             }
//             mtca::list_close();
//             mtca::assoc_item_close();

//             mtca::assoc_close();
//             mtca::list_item_close();
//         }
//         mtca::list_close();
//         mtca::assoc_item_close();

//         mtca::assoc_close();
//         mtca::list_item_close();
//     }
//     mtca::list_close();
// }
#endif
