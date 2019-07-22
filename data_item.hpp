#pragma once

// You can rely on the fields local_item.
// Do not rely on the implementation of shared_item!

// It's important that the constant below (now 64) is the size in bits of shared_item.shared_item_data.
#define ITER_SHIFT (64 - NUM_CPUS_SHIFT)
#define ITER_MASK ((1l << ITER_SHIFT) - 1l)

//A shared_item is the data written in the CAS object.
typedef uint64_t shared_item;

#define EMPTY_SHARED_ITEM 0
#define DUMMY_SHARED_ITEM ((uint64_t) (-1l))

//A local_item is an element of some thread's record.
struct local_item {
    shared_item val;
    op_type op;
    uint64_t loc;
#ifdef TIME
    timespec start;
    timespec end;
#endif
};

#define EMPTY_LOCAL_ITEM {EMPTY_SHARED_ITEM}
#define DUMMY_LOCAL_ITEM {DUMMY_SHARED_ITEM}

shared_item make_shared_item(uint64_t worker, uint64_t iter) {
    return (worker << ITER_SHIFT) | iter;
}

void increment_shared_item(shared_item &item) {
    item++;
}

uint64_t shared_item_worker(shared_item item) {
    return item >> ITER_SHIFT;
}

uint64_t shared_item_iter(shared_item item) {
    return item & ITER_MASK;
}

bool is_shared_item_empty(shared_item item) {
    return item == EMPTY_SHARED_ITEM;
}

bool is_local_item_dummy(local_item item) {
    return item.val == DUMMY_SHARED_ITEM;
}
